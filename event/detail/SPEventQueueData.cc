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

Status PerformEngine::perform(Rc<thread::Task> &&task) {
	if (!_performEnabled) {
		return Status::Declined;
	}

	mem_pool::perform([&] {
		Block *next;
		if (_emptyBlocks) {
			next = _emptyBlocks;
			_emptyBlocks = _emptyBlocks->next;
		} else {
			next = new (_pool) Block;
		}

		next->task = move(task);

		if (_pendingBlocksTail) {
			_pendingBlocksTail->next = next;
			_pendingBlocksTail = next;
		} else {
			_pendingBlocksFront = _pendingBlocksTail = next;
		}
	}, _pool);

	return Status::Ok;
}

Status PerformEngine::perform(mem_std::Function<void()> &&fn, Ref *ref) {
	if (!_performEnabled) {
		return Status::Declined;
	}

	mem_pool::perform([&] {
		Block *next;
		if (_emptyBlocks) {
			next = _emptyBlocks;
			_emptyBlocks = _emptyBlocks->next;
		} else {
			next = new (_pool) Block;
		}

		next->fn = sp::move(fn);
		next->ref = ref;

		if (_pendingBlocksTail) {
			_pendingBlocksTail->next = next;
			_pendingBlocksTail = next;
		} else {
			_pendingBlocksFront = _pendingBlocksTail = next;
		}
	}, _pool);

	return Status::Ok;
}

uint32_t PerformEngine::runAllTasks(memory::pool_t *tmpPool) {
	uint32_t nevents = 0;

	while (_pendingBlocksFront) {
		auto next = _pendingBlocksFront;

		if (_pendingBlocksFront == _pendingBlocksTail) {
			_pendingBlocksFront = _pendingBlocksTail = nullptr;
		} else {
			_pendingBlocksFront = _pendingBlocksFront->next;
		}

		mem_pool::perform_clear([&] {
			if (next->fn) {
				next->fn();
			}

			if (next->task) {
				next->task->run();
			}

			++ nevents;

			next->fn = nullptr;
			next->task = nullptr;
			next->ref = nullptr;
		}, tmpPool);

		next->next = _emptyBlocks;
		_emptyBlocks = next;
	}

	return nevents;
}

PerformEngine::PerformEngine(memory::pool_t *pool)
: _pool(pool), _tmpPool(memory::pool::create(pool)) { }

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

	_performEnabled = true;

	auto tmpPool = memory::pool::create(_tmpPool);

	mem_pool::perform_clear([&] {
		if (cl->notifyFn) {
			cl->notifyFn(cl, handle, handle->_data, data);
		}
	}, tmpPool);

	runAllTasks(tmpPool);

	memory::pool::destroy(tmpPool);

	_performEnabled = false;
}

QueueData::QueueData(QueueRef *ref, QueueFlags flags)
: PerformEngine(ref->getPool()), _info(QueueHandleClassInfo{ref, this, ref->getPool()}), _flags(flags) {
	_pendingHandles.set_memory_persistent(true);
	_suspendableHandles.set_memory_persistent(true);
}

}
