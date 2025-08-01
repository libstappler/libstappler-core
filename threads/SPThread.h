/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023-2025 Stappler LLC <admin@stappler.dev>

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

#ifndef STAPPLER_THREADS_SPTHREAD_H_
#define STAPPLER_THREADS_SPTHREAD_H_

#include "SPNotNull.h"
#include "SPRef.h"

namespace STAPPLER_VERSIONIZED stappler::thread {

struct SP_PUBLIC ThreadInfo {
	static constexpr uint32_t DetachedWorker = maxOf<uint32_t>();

	static const ThreadInfo *getThreadInfo();

	static void setThreadInfo(StringView, uint32_t worker = DetachedWorker, bool managed = true);

	// Associate thread pool with current thread
	// Thread must not perform any actions after this pool destroyed
	// Association is permatent, returns false when thread already linked with pool
	// Threads, that was created by stappler_thread module already has internally associated pool,
	// only main and external threads has no initial pool association
	//
	// Some thread-bound utils, like event::Looper, uses thread memory pool as a lifetime definition,
	// and destroyed when thread's pool is destroyed
	static bool setThreadPool(const NotNull<memory::pool_t> &);

	uint32_t workerId = 0;
	StringView name;
	bool managed = false;

	memory::allocator_t *threadAlloc = nullptr;
	memory::pool_t *threadPool = nullptr;
	memory::pool_t *workerPool = nullptr;
};

enum class ThreadFlags {
	None = 0,
	Joinable = 1 << 0,
};

SP_DEFINE_ENUM_AS_MASK(ThreadFlags)

/* Interface for thread workers or handlers */
class SP_PUBLIC Thread : public Ref {
public:
	static void workerThread(Thread *tm);

	static const Thread *getCurrentThread();

	template <typename T>
	static const T *findSpecificThread();

	virtual ~Thread();

	virtual bool run(ThreadFlags = ThreadFlags::Joinable);
	virtual void stop();

	virtual void waitRunning();
	virtual void waitStopped();

	virtual void threadInit();
	virtual void threadDispose();
	virtual bool worker();

	// workload overload
	virtual bool performWorkload() { return false; }

	virtual bool isRunning() const { return _running.load(); }

	std::thread::id getThreadId() const { return _thisThreadId; }

	bool isOnThisThread() const;

	const Thread *getParentThread() const { return _parentThread; }

protected:
	ThreadFlags _flags = ThreadFlags::None;

	const Thread *_parentThread = nullptr;

	const std::type_info *_type = nullptr;

	std::thread _thisThread;
	std::thread::id _thisThreadId;

	std::atomic<bool> _running = false;
	std::mutex _runningMutex;
	std::condition_variable _runningVar;

	mutable std::atomic_flag _continueExecution;
};

template <typename T>
auto Thread::findSpecificThread() -> const T * {
	auto thread = getCurrentThread();

	while (thread) {
		auto d = dynamic_cast<const T *>(thread);
		if (d) {
			return d;
		}
		thread = thread->getParentThread();
	}

	return nullptr;
}

} // namespace stappler::thread

#endif /* STAPPLER_THREADS_SPTHREAD_H_ */
