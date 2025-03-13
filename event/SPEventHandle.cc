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
			if (suspend() != Status::Ok) {
				return Status::ErrorNotPermitted;
			}
		} else {
			return Status::ErrorNotPermitted;
		}
	}
	if (_status == Status::Suspended || _status == Status::Declined) {
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
		_queue = nullptr;
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
		_status = _runFn(this);
		if (_status != Status::Ok && _status != Status::Done) {
			log::error("event::Handle", "Fail to run handle: ", _status);
		} else {
			_status = Status::Ok;
		}
		return _status;
	}
	return Status::ErrorNotImplemented;
}

Status Handle::suspend() {
	if (_suspendFn) {
		_status = _suspendFn(this);
		if (_status != Status::Ok && _status != Status::Done) {
			log::error("event::Handle", "Fail to suspend handle: ", _status);
		} else {
			_status = Status::Suspended;
			return Status::Ok;
		}
		return _status;
	}
	return Status::ErrorNotImplemented;
}

Status Handle::resume() {
	if (_resumeFn) {
		_status = _runFn(this);
		if (_status != Status::Ok && _status != Status::Done) {
			log::error("event::Handle", "Fail to resume handle: ", _status);
		} else {
			_status = Status::Ok;
		}
		return _status;
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

bool ThreadHandle::init(QueueRef *q, QueueData *d) {
	if (!Handle::init(q, d)) {
		return false;
	}

	_pool = Rc<PoolRef>::alloc(memory::app_root_pool);

	_outputQueue.reserve(2);
	_outputCallbacks.reserve(2);

	return true;
}

uint32_t ThreadHandle::performAll(const Callback<void(uint32_t)> &unlockCallback) {
	auto stack = sp::move(_outputQueue);
	auto callbacks = sp::move(_outputCallbacks);

	_outputQueue.clear();
	_outputCallbacks.clear();

	unlockCallback(static_cast<uint32_t>(stack.size() + callbacks.size()));

	uint32_t nevents = 0;
	memory::pool::perform_clear([&] {
		for (auto &task : stack) {
			task->run();
			++ nevents;
		}

		for (auto &task : callbacks) {
			task.first();
			++ nevents;
		}

		for (auto &task : _unsafeQueue) {
			task->run();
			++ nevents;
		}

		for (auto &task : _unsafeCallbacks) {
			task.first();
			++ nevents;
		}
	}, _pool->getPool());

	stack.clear();
	callbacks.clear();
	_unsafeQueue.clear();
	_unsafeCallbacks.clear();
	return nevents;
}

}
