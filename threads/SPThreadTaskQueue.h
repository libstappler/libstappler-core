/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_THREADS_SPTHREADTASKQUEUE_H_
#define STAPPLER_THREADS_SPTHREADTASKQUEUE_H_

#include "SPThreadTask.h"

namespace STAPPLER_VERSIONIZED stappler::thread {

class Worker;

class SP_PUBLIC TaskQueue : public RefBase<memory::StandartInterface> {
public:
	using Ref = RefBase<memory::StandartInterface>;
	using TaskMap = std::map<uint32_t, std::vector<Rc<Task>>, std::less<void>>;

	struct WorkerContext;

	static const TaskQueue *getOwner();

	TaskQueue(StringView name = StringView(), std::function<void()> &&wakeup = std::function<void()>());
	~TaskQueue();

	void finalize();

	void performAsync(Rc<Task> &&task);

	void perform(Rc<Task> &&task, bool first = false);
	void perform(std::function<void()> &&, Ref * = nullptr, bool first = false);

	void update(uint32_t *count = nullptr);

	void onMainThread(Rc<Task> &&task);
	void onMainThread(std::function<void()> &&func, Ref *target);

	bool spawnWorkers();

	// maxOf<uint32_t> - set id to next available
	bool spawnWorkers(uint32_t threadId, uint16_t threadCount, StringView name = StringView());
	bool cancelWorkers();

	void performAll();
	bool waitForAll(TimeInterval = TimeInterval::seconds(1));

	bool wait(uint32_t *count = nullptr);
	bool wait(TimeInterval, uint32_t *count = nullptr);

	void lock();
	void unlock();

	StringView getName() const { return _name; }

	std::vector<std::thread::id> getThreadIds() const;

	size_t getOutputCounter() const;

	uint16_t getThreadCount() const;

protected:
	friend class Worker;

	void onMainThreadWorker(Rc<Task> &&task);

	WorkerContext *_context = nullptr;
	StringView _name = StringView("TaskQueue");
};

}

#endif /* STAPPLER_THREADS_SPTHREADTASKQUEUE_H_ */
