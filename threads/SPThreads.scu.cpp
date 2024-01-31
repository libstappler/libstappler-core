/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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

#include "SPCommon.h"
#include "SPThread.h"

namespace STAPPLER_VERSIONIZED stappler::thread {

struct ThreadCallbacks {
	void (*init) (void *);
	void (*dispose) (void *);
	bool (*worker) (void *);
};

void _workerThread(const ThreadCallbacks &tm, void *);

static std::atomic<uint32_t> s_threadId(1);

SPUNUSED static uint32_t getNextThreadId() {
	auto id = s_threadId.fetch_add(1);
	return (id % 0xFFFF) + (1 << 16);
}

thread_local const void *tl_owner = nullptr;

template <>
const void *ThreadInterface<memory::StandartInterface>::getOwner() {
	return tl_owner;
}

template <>
const void *ThreadInterface<memory::PoolInterface>::getOwner() {
	return tl_owner;
}

template <>
void ThreadInterface<memory::StandartInterface>::workerThread(ThreadInterface<memory::StandartInterface> *tm, const void *q) {
	tl_owner = q;

	ThreadCallbacks cb;
	cb.init = [] (void *obj) {
		((ThreadInterface<memory::StandartInterface> *)obj)->threadInit();
	};

	cb.dispose = [] (void *obj) {
		((ThreadInterface<memory::StandartInterface> *)obj)->threadDispose();
	};

	cb.worker = [] (void *obj) -> bool {
		return ((ThreadInterface<memory::StandartInterface> *)obj)->worker();
	};

	memory::pool::initialize();
	_workerThread(cb, tm);
	memory::pool::terminate();
}

template <>
void ThreadInterface<memory::PoolInterface>::workerThread(ThreadInterface<memory::PoolInterface> *tm, const void *q) {
	tl_owner = q;

	ThreadCallbacks cb;
	cb.init = [] (void *obj) {
		((ThreadInterface<memory::PoolInterface> *)obj)->threadInit();
	};

	cb.dispose = [] (void *obj) {
		((ThreadInterface<memory::PoolInterface> *)obj)->threadDispose();
	};

	cb.worker = [] (void *obj) -> bool {
		return ((ThreadInterface<memory::PoolInterface> *)obj)->worker();
	};

	memory::pool::initialize();
	_workerThread(cb, tm);
	memory::pool::terminate();
}

}

#include "SPThreads-android.cc"
#include "SPThreads-linux.cc"
#include "SPThreads-win32.cc"
#include "SPThreadTask.cc"
#include "SPThreadTaskQueue.cc"

namespace STAPPLER_VERSIONIZED stappler::thread {

thread_local ThreadInfo tl_threadInfo;

ThreadInfo *ThreadInfo::getThreadLocal() {
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
#if LINUX
	pthread_setname_np(pthread_self(), name.data());
#endif
	tl_threadInfo.threadId = t;
	tl_threadInfo.workerId = w;
	tl_threadInfo.name = name;
	tl_threadInfo.managed = m;
}

void ThreadInfo::setThreadInfo(StringView name) {
#if LINUX
	pthread_setname_np(pthread_self(), name.data());
#endif
	tl_threadInfo.threadId = 0;
	tl_threadInfo.workerId = 0;
	tl_threadInfo.name = name;
	tl_threadInfo.managed = true;
	tl_threadInfo.detouched = true;
}

const TaskQueue *TaskQueue::getOwner() {
	return (const TaskQueue *)tl_owner;
}

static void ThreadCallbacks_init(const ThreadCallbacks &cb, void *tm) {
	cb.init(tm);
}

static bool ThreadCallbacks_worker(const ThreadCallbacks &cb, void *tm) {
	return cb.worker(tm);
}

static void ThreadCallbacks_dispose(const ThreadCallbacks &cb, void *tm) {
	cb.dispose(tm);
}

}
