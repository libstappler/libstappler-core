/**
 Copyright (c) 2025 Stappler LLC <admin@stappler.dev>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#include "SPEventHandle.h"
#include "SPEventTimerHandle.h"
#include "SPEventFileHandle.h"
#include "SPEventThreadHandle.h"
#include "detail/SPEventQueueData.h"

namespace STAPPLER_VERSIONIZED stappler::event {

Handle::~Handle() { }

Handle::Handle() {
	::memset(_data, 0, DataSize);
}

bool Handle::init(QueueRef *q, QueueData *data) {
	_queue = q;
	_queueData = data;
	return true;
}

Status Handle::cancel(Status st) {
	if (!isValidCancelStatus(st)) {
		log::warn("event::Handle", "Handle::cancel should be called with Status::Done or one of the error statuses."
				" It's undefined behavior otherwise");
		return Status::ErrorInvalidArguemnt;
	}

	if (isValidCancelStatus(_status)) {
		return Status::ErrorAlreadyPerformed;
	}

	if (_status == Status::Ok) {
		if (_suspendFn) {
			return _suspendFn(this);
		}
		return Status::ErrorNotPermitted;
	} else if (_status == Status::Suspended || _status == Status::Declined) {
		_status = st;
		finalize(_status);
		if (isResumable()) {
			// Disable auto-resume
			_queueData->cancel(this);
		}
		if (_status == Status::Done) {
			// Run pending on queue
			for (auto &it : _pendingHandles) {
				_queueData->runHandle(it);
			}
		} else {
			// Cancel all pending
			for (auto &it : _pendingHandles) {
				it->cancel(Status::ErrorCancelled);
			}
		}
		_queue->submitPending();
		_pendingHandles.clear();
		return Status::Ok;
	} else {
		return Status::ErrorAlreadyPerformed;
	}
}

Status Handle::addPending(Rc<Handle> &&handle) {
	if (_status == Status::Done) {
		return _queueData->runHandle(handle);
	} else if (isSuccessful(_status)) {
		_pendingHandles.emplace_back(move(handle));
		return Status::Suspended;
	} else {
		return Status::ErrorInvalidArguemnt;
	}
}

Status Handle::run() {
	if (_runFn) {
		return _runFn(this);
	}
	return Status::ErrorNotImplemented;
}

Status Handle::suspend() {
	if (_suspendFn) {
		return _suspendFn(this);
	}
	return Status::ErrorNotImplemented;
}

Status Handle::resume() {
	if (_resumeFn) {
		return _resumeFn(this);
	}
	return Status::ErrorNotImplemented;
}

Status Handle::prepareRearm() {
	if (_status == Status::Ok) {
		return Status::ErrorAlreadyPerformed;
	}

	if (_status != Status::Declined && _status != Status::Suspended) {
		return Status::ErrorNotPermitted;
	}

	_status = Status::Ok;
	return _status;
}

Status Handle::prepareDisarm(bool suspend) {
	if (_status != Status::Ok) {
		return Status::ErrorAlreadyPerformed;
	}

	if (suspend) {
		_status = Status::Suspended;
	}

	return Status::Ok;
}

void Handle::sendCompletion(uint32_t value, Status status) {
	if (_completion.fn) {
		_completion.fn(_completion.userdata, this, value, status);
	}
}

void Handle::finalize(Status status) {
	sendCompletion(_value, status);
	_completion.fn = nullptr;
	_completion.userdata = nullptr;
}

bool TimerHandle::init(QueueRef *q, QueueData *d, TimerInfo &&info) {
	if (!Handle::init(q, d)) {
		return false;
	}

	_count = info.count;
	_completion = move(info.completion);

	return true;
}

bool FileOpHandle::init(QueueRef *q, QueueData *d, FileOpInfo &&info) {
	if (!Handle::init(q, d)) {
		return false;
	}

	_root = info.root;
	_pathname = info.path.str<memory::StandartInterface>();
	return true;
}

bool StatHandle::init(QueueRef *q, QueueData *d, StatOpInfo &&info) {
	if (!FileOpHandle::init(q, d, move(info.file))) {
		return false;
	}

	_completion = move(info.completion);
	return true;
}

bool DirHandle::init(QueueRef *q, QueueData *d, OpenDirInfo &&info) {
	if (!FileOpHandle::init(q, d, move(info.file))) {
		return false;
	}

	_completion = move(info.completion);
	return true;
}

void ThreadHandle::performAll(const Callback<void()> &unlockCallback) {
	auto stack = sp::move(_outputQueue);
	auto callbacks = sp::move(_outputCallbacks);

	_outputQueue.clear();
	_outputCallbacks.clear();

	unlockCallback();

	memory::pool::perform_temporary([&] {
		for (auto &task : stack) {
			task->onComplete();
		}

		for (auto &task : callbacks) {
			task.first();
		}
	});

	stack.clear();
	callbacks.clear();
}

}
