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

#ifndef CORE_EVENT_SPEVENTQUEUE_H_
#define CORE_EVENT_SPEVENTQUEUE_H_

#include "SPEvent.h"

namespace STAPPLER_VERSIONIZED stappler::event {

enum class QueueFlags {
	None,
	Protected = 1 << 0, // try to protect operations from interrupting with signals
	SubmitImmediate = 1 << 1, // submit all operations as they added, no need to call `submitPending`
};

SP_DEFINE_ENUM_AS_MASK(QueueFlags)

enum class QueueWakeupFlags {
	None,
	Graceful = 1 << 0,
};

SP_DEFINE_ENUM_AS_MASK(QueueWakeupFlags)

struct OpenFileInfo {
	StringView path;
	FileOpenFlags flags;
	FileProtFlags prot;
};

struct QueueInfo {
	static constexpr uint32_t DefaultQueueSize = 32;

	uint32_t submitQueueSize = DefaultQueueSize;
	uint32_t completeQueueSize = 0; // or 0 for default size, based on submitQueueSize
	TimeInterval osIdleInterval; // interval, on which internal OS systems will be put to sleep, if idle
};

/* Simple IO event loop interface
 *
 * Interface is single-threaded, no submission allowed from other threads
 */

class Queue : public memory::PoolObject {
public:
	struct Data; // private platform data

	virtual ~Queue();

	bool init(const QueueInfo & = QueueInfo(), QueueFlags flags = QueueFlags::None);

	Rc<OpHandle> openDir(CompletionHandle<DirHandle> &&, StringView path);
	Rc<OpHandle> openFile(OpenFileInfo &&);

	Rc<TimerHandle> scheduleTimer(TimerInfo &&);

	Rc<OpHandle> read(CompletionHandle<void> &&, InputOutputHandle *, BufferChain *, uint32_t, uint32_t);
	Rc<OpHandle> write(CompletionHandle<void> &&, InputOutputHandle *, BufferChain *, uint32_t, uint32_t);

	Status submitPending();

	// non-blocking poll
	uint32_t poll();

	// wait until next event, or timeout
	uint32_t wait(TimeInterval = TimeInterval());

	// run for some time or infinite (when no timeout)
	Status run(TimeInterval = TimeInterval());

	// wakeup queue from `run`
	// If Graceful flag is set - wait until all operations are completed, and forbid a new ones from running
	// If gracefulTimeout is set, queue will issue a graceful wakeup, but after timeout hard wakeup will be performed
	void wakeup(QueueWakeupFlags = QueueWakeupFlags::None, TimeInterval gracefulTimeout = TimeInterval());

	QueueFlags getFlags() const;

	using PoolObject::PoolObject;

protected:
	std::thread::id _ownerThread;
	Data *_data = nullptr;
};

using QueueRef = SharedRef<Queue>;

}

#endif /* CORE_EVENT_SPEVENTQUEUE_H_ */
