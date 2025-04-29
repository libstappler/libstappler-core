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

#include "SPEventLooper.h"
#include "SPMemPoolInterface.h"
#include "SPThread.h"

namespace STAPPLER_VERSIONIZED stappler::event::platform {

Rc<QueueRef> getThreadQueue(QueueInfo &&);

}

namespace STAPPLER_VERSIONIZED stappler::event {

static thread_local Looper *tl_looper;

struct Looper::Data : public memory::AllocPool {
	thread::ThreadPoolInfo threadPoolInfo;

	Rc<QueueRef> queue;
	Rc<ThreadHandle> threadHandle;
	Rc<thread::ThreadPool> threadPool;
	const thread::ThreadInfo *threadInfo = nullptr;
	memory::pool_t *threadMemPool = nullptr;
	std::thread::id thisThreadId;
	bool suspendThreadsOnWakeup = false;

	thread::ThreadPool *getThreadPool() {
		if (!threadPool) {
			threadPool = Rc<thread::ThreadPool>::create(thread::ThreadPoolInfo(threadPoolInfo));
		}
		return threadPool;
	}

	void cleanup() {
		if (!queue) {
			return;
		}

		auto q = queue;
		if (threadPool) {
			threadPool->cancel();
			threadPool = nullptr;
		}

		threadPoolInfo.ref = nullptr;

		if (threadHandle) {
			threadHandle->cancel();
			threadHandle = nullptr;
		}
		queue = nullptr; // this possibly destroys Queue with it's pool and _data

		q->cancel();
#if SP_REF_DEBUG
		if (q->getRef()->getReferenceCount() > 1) {
			auto tmp = q->getRef();
			q = nullptr;

			tmp->foreachBacktrace([](uint64_t id, Time time, const std::vector<std::string> &vec) {
				mem_std::StringStream stream;
				stream << "[" << id << ":" << time.toHttp<memory::StandartInterface>() << "]:\n";
				for (auto &it : vec) { stream << "\t" << it << "\n"; }
				log::debug("event::Queue", stream.str());
			});
		}
		queue = nullptr;
#endif
	}
};

Looper *Looper::acquire(LooperInfo &&info) {
	return acquire(move(info),
			QueueInfo{
				.flags = QueueFlags::SubmitImmediate | QueueFlags::ThreadNative,
				.engineMask = info.engineMask,
				.osIdleInterval = TimeInterval::milliseconds(100),
			});
}

Looper *Looper::acquire(LooperInfo &&info, QueueInfo &&qinfo) {
	if (tl_looper) {
		return tl_looper;
	}

	auto threadInfo = thread::ThreadInfo::getThreadInfo();
	if (threadInfo) {
		qinfo.pool = threadInfo->threadPool;
	}

	auto q = platform::getThreadQueue(move(qinfo));
	if (!q) {
		return nullptr;
	}

	tl_looper = new Looper(move(info), move(q));

	return tl_looper;
}

Looper *Looper::getIfExists() { return tl_looper; }

Looper::~Looper() {
	if (_data) {
		_data->cleanup();
		_data = nullptr;
	}
}

Rc<TimerHandle> Looper::scheduleTimer(TimerInfo &&info, Ref *ref) {
	return _data->queue->scheduleTimer(move(info), ref);
}

Rc<Handle> Looper::schedule(TimeInterval timeout,
		mem_std::Function<void(Handle *, bool success)> &&fn, Ref *ref) {
	return _data->queue->schedule(timeout, sp::move(fn), ref);
}

Status Looper::performOnThread(Rc<thread::Task> &&task, bool immediate) {
	bool isOnThread = isOnThisThread();
	if (immediate && isOnThread) {
		task->run();
		return Status::Ok;
	}
	if (!isOnThread || _data->queue->performNext(move(task)) == Status::Declined) {
		return _data->threadHandle->perform(move(task));
	} else {
		return Status::Ok;
	}
}

Status Looper::performOnThread(mem_std::Function<void()> &&func, Ref *target, bool immediate,
		StringView tag) {
	bool isOnThread = isOnThisThread();
	if (immediate && isOnThread) {
		func();
		return Status::Ok;
	}
	if (!isOnThread || _data->queue->performNext(sp::move(func), target, tag) == Status::Declined) {
		return _data->threadHandle->perform(sp::move(func), target, tag);
	} else {
		return Status::Ok;
	}
}

Status Looper::performAsync(Rc<thread::Task> &&task, bool first) {
	return _data->getThreadPool()->perform(move(task), first);
}

Status Looper::performAsync(mem_std::Function<void()> &&func, Ref *target, bool first,
		StringView tag) {
	return _data->getThreadPool()->perform(sp::move(func), target, first, tag);
}

Status Looper::performHandle(Handle *h) { return _data->queue->runHandle(h); }

uint32_t Looper::poll() { return _data->queue->poll(); }

uint32_t Looper::wait(TimeInterval ival) { return _data->queue->wait(ival); }

Status Looper::run(TimeInterval ival, QueueWakeupInfo &&info) {
	_data->threadHandle->wakeup();

	auto ret = _data->queue->run(ival, move(info));

	if (_data->suspendThreadsOnWakeup) {
		_data->suspendThreadsOnWakeup = false;

		_data->threadPool->cancel();
		_data->threadPool = nullptr;
	}

	return ret;
}

Status Looper::wakeup(QueueWakeupInfo &&info) {
	if (hasFlag(info.flags, WakeupFlags::SuspendThreads)) {
		_data->suspendThreadsOnWakeup = true;
	}
	return _data->queue->wakeup(move(info));
}

uint16_t Looper::getWorkersCount() const { return _data->threadPool->getInfo().threadCount; }

memory::pool_t *Looper::getThreadMemPool() const { return _data->threadMemPool; }

const event::Queue *Looper::getQueue() const { return _data->queue; }

thread::ThreadPool *Looper::getThreadPool() const { return _data->threadPool; }

bool Looper::isOnThisThread() const { return _data->thisThreadId == std::this_thread::get_id(); }

Looper::Looper(LooperInfo &&info, Rc<QueueRef> &&q) {
	auto pool = q->getPool();
	mem_pool::perform([&] {
		_data = new (pool) Data;
		_data->queue = move(q);
		_data->threadHandle = _data->queue->addThreadHandle();
		_data->threadPoolInfo = thread::ThreadPoolInfo{.flags = info.workersFlags,
			.name = info.name,
			.threadCount = info.workersCount,
			.complete = _data->threadHandle.get(),
			.ref = _data->threadHandle};

		_data->threadPoolInfo.name = StringView(mem_pool::toString(info.name, ":Worker")).pdup();

		thread::ThreadInfo::setThreadInfo(info.name);

		_data->threadInfo = thread::ThreadInfo::getThreadInfo();
		if (!_data->threadInfo || !_data->threadInfo->threadPool) {
			_data->threadMemPool = pool;
		} else {
			_data->threadMemPool = _data->threadInfo->threadPool;
			memory::pool::cleanup_register(_data->threadMemPool, this, [](void *d) -> Status {
				auto l = (Looper *)d;
				l->_data->cleanup();
				l->_data = nullptr;
				tl_looper = nullptr;
				return Status::Ok;
			});
		}

		_data->thisThreadId = std::this_thread::get_id();
	}, pool);
}

} // namespace stappler::event
