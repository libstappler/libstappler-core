/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

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

#include "SPThread.h"
#include "SPLog.h"
#include "SPThreadTaskQueue.h"

namespace STAPPLER_VERSIONIZED stappler::thread {

struct ThreadCallbacks {
	void (*init) (ThreadInterface *);
	void (*dispose) (ThreadInterface *);
	bool (*worker) (ThreadInterface *);
};

static void _workerThread(const ThreadCallbacks &tm, ThreadInterface *);
static void _setThreadName(StringView name);

static std::atomic<uint32_t> s_threadId(1);

thread_local ThreadInfo tl_threadInfo;
thread_local const ThreadInterface *tl_owner = nullptr;

static void ThreadCallbacks_init(const ThreadCallbacks &cb, ThreadInterface *tm) {
	memory::pool::initialize();

	tl_threadInfo.threadAlloc = memory::allocator::create();
	tl_threadInfo.threadPool = memory::pool::create(tl_threadInfo.threadAlloc);
	tl_threadInfo.workerPool = memory::pool::create(tl_threadInfo.threadPool);

	memory::pool::perform([&] {
		tm->retain();
		cb.init(tm);
	}, tl_threadInfo.threadPool);
}

static bool ThreadCallbacks_worker(const ThreadCallbacks &cb, ThreadInterface *tm) {
	SPASSERT(tl_threadInfo.workerPool, "Thread pool should be initialized");
	bool ret = false;

	memory::pool::perform_clear([&] {
		ret = cb.worker(tm);
	}, tl_threadInfo.workerPool);

	return ret;
}

static void ThreadCallbacks_dispose(const ThreadCallbacks &cb, ThreadInterface *tm) {
	memory::pool::perform([&] {
		cb.dispose(tm);
		tm->release(0);
	}, tl_threadInfo.threadPool);

	memory::pool::destroy(tl_threadInfo.workerPool);
	memory::pool::destroy(tl_threadInfo.threadPool);
	memory::allocator::destroy(tl_threadInfo.threadAlloc);

	memory::pool::terminate();
}

ThreadInfo *ThreadInfo::getThreadInfo() {
	if (!tl_threadInfo.managed) {
		return nullptr;
	}

	return &tl_threadInfo;
}

void ThreadInfo::setMainThread() {
	tl_threadInfo.threadId = mainThreadId;
	tl_threadInfo.workerId = 0;
	tl_threadInfo.name = StringView("Main");
	tl_threadInfo.managed = true;
}

void ThreadInfo::setThreadInfo(uint32_t t, uint32_t w, StringView name, bool m) {
	_setThreadName(name);

	tl_threadInfo.threadId = t;
	tl_threadInfo.workerId = w;
	tl_threadInfo.name = name;
	tl_threadInfo.managed = m;
}

void ThreadInfo::setThreadInfo(StringView name) {
	_setThreadName(name);

	tl_threadInfo.threadId = 0;
	tl_threadInfo.workerId = 0;
	tl_threadInfo.name = name;
	tl_threadInfo.managed = true;
	tl_threadInfo.detouched = true;
}

SPUNUSED static uint32_t getNextThreadId() {
	auto id = s_threadId.fetch_add(1);
	return (id % 0xFFFF) + (1 << 16);
}

void ThreadInterface::workerThread(ThreadInterface *tm) {
	tl_owner = tm;

	ThreadCallbacks cb;
	cb.init = [] (ThreadInterface *obj) {
		obj->threadInit();
	};

	cb.dispose = [] (ThreadInterface *obj) {
		obj->threadDispose();
	};

	cb.worker = [] (ThreadInterface *obj) -> bool {
		return obj->worker();
	};

	memory::pool::initialize();
	_workerThread(cb, tm);
	memory::pool::terminate();
}

const Thread *Thread::getCurrentThread() {
	return dynamic_cast<const Thread *>(tl_owner);
}

Thread::~Thread() {
	if (std::this_thread::get_id() == _thisThreadId) {
		return;
	}

	if ((_flags & ThreadFlags::Joinable) != ThreadFlags::None) {
		_continueExecution.clear();
		_thisThread.join();
	}
}

bool Thread::run(ThreadFlags flags) {
	if (_type != nullptr) {
		log::error("Thread", "Thread already started");
		return false;
	}
	_flags = flags;
	_type = &(typeid(*this));
	_continueExecution.test_and_set();
	_parentThread = getCurrentThread();
	_thisThread = std::thread(Thread::workerThread, this);
	if ((flags & ThreadFlags::Joinable) == ThreadFlags::None) {
		_thisThread.detach();
	}
	return true;
}

void Thread::stop() {
	_continueExecution.clear();
}

void Thread::waitRunning() {
	if (_running.load()) {
		return;
	}

	std::unique_lock lock(_runningMutex);
	if (_running.load()) {
		return;
	}

	_runningVar.wait(lock, [&] {
		return _running.load();
	});
}

void Thread::waitStopped() {
	_thisThread.join();
	_flags &= ~ThreadFlags::Joinable;
}

void Thread::threadInit() {
	_thisThreadId = std::this_thread::get_id();

	std::unique_lock lock(_runningMutex);
	_running.store(true);
	_runningVar.notify_all();
}

void Thread::threadDispose() {

}

bool Thread::worker() {
	return performWorkload() && _continueExecution.test_and_set();
}

bool Thread::isOnThisThread() const {
	return _thisThreadId == std::this_thread::get_id();
}

}
