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

#include "SPEventQueueData.h"
#include "SPEventHandle.h"

namespace STAPPLER_VERSIONIZED stappler::event {

uint32_t QueueData::suspendAll() {
	uint32_t ret = 0;
	_running = false;
	for (auto &it : _suspendableHandles) {
		if (it->getStatus() == Status::Ok || it->getStatus() == Status::Suspended) {
			if (isSuccessful(it->suspend())) {
				++ ret;
			}
		} else if (it->getStatus() != Status::Declined) {
			log::error("event::QueueData", "suspendAll: Invalid status for a resumable handle: ", it->getStatus());
		}
	}
	return ret;
}

uint32_t QueueData::resumeAll() {
	if (_running) {
		return 0;
	}

	_running = true;

	uint32_t ret = 0;
	for (auto &it : _suspendableHandles) {
		if (it->getStatus() == Status::Suspended) {
			if (it->resume() == Status::Ok) {
				++ ret;
			}
		} else if (it->getStatus() != Status::Declined) {
			log::error("event::QueueData", "resumeAll: Invalid status for a resumable handle: ", it->getStatus());
		}
	}
	for (auto &it : _pendingHandles) {
		auto status = it->run();
		if (!isSuccessful(status)) {
			it->cancel(status);
		} else {
			++ ret;
		}
	}
	_pendingHandles.clear();
	return ret;
}

Status QueueData::runHandle(Handle *h) {
	if (_running) {
		auto status = h->run();
		if (!isSuccessful(status)) {
			h->cancel(status);
		}
		return status;
	} else {
		_pendingHandles.emplace(h);
		return Status::Suspended;
	}
}

void QueueData::cancel(Handle *h) {
	_suspendableHandles.erase(h);
}

void QueueData::cleanup() {
	mem_pool::perform([&] {
		mem_pool::Set<Rc<Handle>> tmpHandles;
		for (auto &it : _suspendableHandles) {
			tmpHandles.emplace(it);
		}

		for (auto &it : _pendingHandles) {
			tmpHandles.emplace(it);
		}

		for (auto &it : tmpHandles) {
			it->cancel();
		}
	}, _tmpPool);

	_suspendableHandles.clear();
	_pendingHandles.clear();
}

void QueueData::notify(Handle *handle, const NotifyData &data) {
	auto cl = handle->_class;

	if (cl->notifyFn) {
		cl->notifyFn(cl, handle, handle->_data, data);
	}
}

QueueData::QueueData(QueueRef *ref, QueueFlags flags)
: _info(QueueHandleClassInfo{ref, this, ref->getPool()}), _flags(flags), _tmpPool(memory::pool::create(ref->getPool())) {
	_pendingHandles.set_memory_persistent(true);
	_suspendableHandles.set_memory_persistent(true);
}

}
