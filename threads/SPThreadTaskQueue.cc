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

#include "SPThreadTaskQueue.h"
#include "SPThread.h"
#include <chrono>

namespace STAPPLER_VERSIONIZED stappler::thread {

SPUNUSED static uint32_t getNextThreadId();

class Worker : public ThreadInterface<memory::StandartInterface> {
public:
	Worker(TaskQueue::WorkerContext *queue, uint32_t threadId, uint32_t workerId, StringView name);
	virtual ~Worker();

#if SP_REF_DEBUG
	virtual uint64_t retain() override;
	virtual void release(uint64_t) override;
#else
	uint64_t retain();
	void release(uint64_t);
#endif

	bool execute(Task *task);

	virtual void threadInit() override;
	virtual void threadDispose() override;
	virtual bool worker() override;

	std::thread &getThread();
	std::thread::id getThreadId() const { return _threadId; }

protected:
	uint64_t _queueRefId = 0;
	TaskQueue::WorkerContext *_queue = nullptr;
	std::thread::id _threadId;
	std::atomic<int32_t> _refCount;
	std::atomic_flag _shouldQuit;

	memory::PoolFlags _flags = memory::PoolFlags::None;
	memory::pool_t *_pool = nullptr;

	uint32_t _managerId;
	uint32_t _workerId;
	StringView _name;
	std::thread _thread;
};

struct TaskQueue::WorkerContext {
	memory::pool_t *pool;
	TaskQueue *queue;

	std::atomic<bool> finalized;

	std::vector<Worker *> workers;

	std::mutex inputMutexQueue;
	std::mutex inputMutexFree;
	memory::PriorityQueue<Rc<Task>> inputQueue;
	std::condition_variable inputCondition;

	std::mutex outputMutex;
	std::vector<Rc<Task>> outputQueue;
	std::vector<Pair<std::function<void()>, Rc<Ref>>> outputCallbacks;
	std::condition_variable outputCondition;

	std::atomic<size_t> outputCounter = 0;
	std::atomic<size_t> tasksCounter = 0;
	std::function<void()> wakeup;

	WorkerContext(TaskQueue *q, std::function<void()> &&w) : queue(q), wakeup(move(w)) {
		pool = memory::pool::create(memory::app_root_pool);

		finalized = false;

		inputQueue.setQueueLocking(inputMutexQueue);
		inputQueue.setFreeLocking(inputMutexFree);

		outputQueue.reserve(2);
		outputCallbacks.reserve(2);
	}

	~WorkerContext() {
		cancel();

		inputQueue.foreach([&] (memory::PriorityQueue<Rc<Task>>::PriorityType p, const Rc<Task> &t) {
			if (t) {
				t->setSuccessful(false);
				t->onComplete();
			}
		});
		inputQueue.clear();

		if (pool) {
			memory::pool::destroy(pool);
		}
	}

	bool isRunning() const {
		return !workers.empty();
	}

	void wait(std::unique_lock<std::mutex> &lock) {
		if (finalized.load() != true) {
			inputCondition.wait(lock);
		}
	}

	void notifyInput() {
		inputCondition.notify_one();
	}

	void notifyInputAll() {
		inputCondition.notify_all();
	}

	void notifyOutput() {
		outputCondition.notify_one();
	}

	void finalize() {
		finalized = true;
		notifyInputAll();
	}

	void spawn(uint32_t threadId, uint32_t threadCount, StringView name) {
		for (uint32_t i = 0; i < threadCount; i++) {
			workers.push_back(new Worker(this, threadId, i, name.empty() ? queue->getName() : name));
		}
	}

	void cancel() {
		for (auto &it : workers) {
			it->release(0);
		}

		notifyInputAll();

		for (auto &it : workers) {
			it->getThread().join();
			delete it;
		}

		workers.clear();
	}

	bool waitOutput(uint32_t *count) {
		std::unique_lock<std::mutex> waitLock(outputMutex);
		outputCondition.wait(waitLock);
		queue->update(count);
		return true;
	}

	bool waitOutput(TimeInterval iv, uint32_t *count) {
		std::unique_lock<std::mutex> waitLock(outputMutex);
		auto c = outputCounter.load();
		if (c > 0) {
			waitLock.unlock();
			queue->update(count);
			return true;
		}
		auto ret = outputCondition.wait_for(waitLock, std::chrono::microseconds(iv.toMicros()), [&, this] {
			return outputCounter.load() > 0;
		});
		if (!ret) {
			if (count) {
				*count = 0;
			}
			return false;
		} else {
			waitLock.unlock();
			queue->update(count);
			return true;
		}
	}

	void lockExternal() {
		outputMutex.lock();
	}

	void unlockExternal() {
		outputMutex.unlock();
	}

	void perform(Rc<Task> &&task, bool first) {
		if (!task->prepare()) {
			task->setSuccessful(false);
			onMainThread(std::move(task));
			return;
		}

		task->addRef(queue);

		++ tasksCounter;
		inputQueue.push(task->getPriority().get(), first, std::move(task));
		notifyInput();
	}

	void onMainThread(Rc<Task> &&task) {
		task->addRef(queue);

		outputMutex.lock();
		outputQueue.push_back(std::move(task));
		++ outputCounter;
		outputMutex.unlock();

		if (wakeup) {
			wakeup();
		}

		notifyOutput();
	}

	void onMainThread(std::function<void()> &&func, Ref *target) {
	    outputMutex.lock();
	    outputCallbacks.emplace_back(std::move(func), target);
	    ++ outputCounter;
		outputMutex.unlock();

		if (wakeup) {
			wakeup();
		}

		notifyOutput();
	}

	void onMainThreadWorker(Rc<Task> &&task) {
	    if (!task) {
	        return;
	    }

		if (!task->getCompleteTasks().empty()) {
			outputMutex.lock();
			outputQueue.push_back(std::move(task));
		    ++ outputCounter;
			outputMutex.unlock();

			if (wakeup) {
				wakeup();
			}
		}

		-- tasksCounter;
		notifyOutput();
	}

	Rc<Task> popTask(uint32_t idx) {
		Rc<Task> ret;
		inputQueue.pop_direct([&] (memory::PriorityQueue<Rc<Task>>::PriorityType, Rc<Task> &&task) {
			ret = move(task);
		});
		return ret;
	}

	void update(uint32_t *count) {
		outputMutex.lock();

		auto stack = std::move(outputQueue);
		auto callbacks = std::move(outputCallbacks);

		outputQueue.clear();
		outputCallbacks.clear();

		outputCounter.store(0);

		outputMutex.unlock();

		memory::pool::push(pool);

		for (Rc<Task> &task : stack) {
			task->onComplete();
		}

		for (Pair<std::function<void()>, Rc<Ref>> &task : callbacks) {
			task.first();
		}

		memory::pool::pop();
		memory::pool::clear(pool);

		if (count) {
			*count += stack.size() + callbacks.size();
		}

		if (tasksCounter.load() > 0) {
			notifyInputAll();
		}
	}
};

class _SingleTaskWorker : public ThreadInterface<memory::StandartInterface> {
public:
	_SingleTaskWorker(const Rc<TaskQueue> &q, Rc<Task> &&task)
	: _queue(q), _task(std::move(task)), _managerId(getNextThreadId()) { }

	virtual ~_SingleTaskWorker() { }

	bool execute(Task *task) {
		return task->execute();
	}

	virtual void threadInit() override {
		ThreadInfo::setThreadInfo(_managerId, 0, "Worker", true);
	}

	virtual bool worker() override {
		if (_task) {
			memory::pool::initialize();
			auto pool = memory::pool::create();

			memory::pool::push(pool);
			auto ret = execute(_task);
			memory::pool::pop();

			_task->setSuccessful(ret);
			if (!_task->getCompleteTasks().empty()) {
				_queue->onMainThread(std::move(_task));
			}

			memory::pool::destroy(pool);
			memory::pool::terminate();
		}

		delete this;
		return false;
	}

protected:
	Rc<TaskQueue> _queue;
	Rc<Task> _task;
	uint32_t _managerId;
};

TaskQueue::TaskQueue(StringView name, std::function<void()> &&wakeup) {
	_context = new WorkerContext(this, move(wakeup));

	if (!name.empty()) {
		_name = name;
	}
}

TaskQueue::~TaskQueue() {
	if (_context) {
		cancelWorkers();
		update();
	}
}

void TaskQueue::finalize() {
	if (_context) {
		_context->finalize();
	}
}

void TaskQueue::performAsync(Rc<Task> &&task) {
	if (task) {
		_SingleTaskWorker *worker = new _SingleTaskWorker(this, std::move(task));
		std::thread wThread(_SingleTaskWorker::workerThread, worker, this);
		wThread.detach();
	}
}

void TaskQueue::perform(Rc<Task> &&task, bool first) {
	if (!task) {
		return;
	}

	if (_context) {
		_context->perform(move(task), first);
	}
}

void TaskQueue::perform(std::function<void()> &&cb, Ref *ref, bool first) {
	perform(Rc<Task>::create([fn = move(cb)] (const Task &) -> bool {
		fn();
		return true;
	}, nullptr, ref), first);
}

void TaskQueue::update(uint32_t *count) {
	_context->update(count);
}

void TaskQueue::onMainThread(Rc<Task> &&task) {
    if (!task) {
        return;
    }

    _context->onMainThread(move(task));
}

void TaskQueue::onMainThread(std::function<void()> &&func, Ref *target) {
    _context->onMainThread(move(func), target);
}

std::vector<std::thread::id> TaskQueue::getThreadIds() const {
	if (!_context) {
		return std::vector<std::thread::id>();
	}

	std::vector<std::thread::id> ret;
	for (Worker *it : _context->workers) {
		ret.emplace_back(it->getThreadId());
	}
	return ret;
}

size_t TaskQueue::getOutputCounter() const {
	return _context->outputCounter.load();
}

uint16_t TaskQueue::getThreadCount() const {
	return uint16_t(_context->workers.size());
}

bool TaskQueue::spawnWorkers() {
	return spawnWorkers(getNextThreadId(), uint16_t(std::thread::hardware_concurrency()), _name);
}

bool TaskQueue::spawnWorkers(uint32_t threadId, uint16_t threadCount, StringView name) {
	if (_context->isRunning()) {
		return false;
	}

	if (threadId == maxOf<uint32_t>()) {
		threadId = getNextThreadId();
	}

	_context->spawn(threadId, threadCount, name);

	return true;
}

bool TaskQueue::cancelWorkers() {
	if (!_context) {
		return false;
	}

	_context->cancel();
	update();
	return true;
}

void TaskQueue::performAll() {
	if (!_context) {
		if (!spawnWorkers()) {
			return;
		}
	}
	if (!waitForAll()) {
		return;
	}
	cancelWorkers();
}

bool TaskQueue::waitForAll(TimeInterval iv) {
	if (!_context || !_context->isRunning()) {
		return false;
	}

	update();
	while (_context->tasksCounter.load() != 0) {
		_context->waitOutput(iv, nullptr);
	}
	return true;
}

bool TaskQueue::wait(uint32_t *count) {
	if (!_context || !_context->isRunning()) {
		return false;
	}

	return _context->waitOutput(count);
}

bool TaskQueue::wait(TimeInterval iv, uint32_t *count) {
	if (!_context || !_context->isRunning()) {
		return false;
	}

	return _context->waitOutput(iv, count);
}

void TaskQueue::lock() {
	_context->lockExternal();
}

void TaskQueue::unlock() {
	_context->unlockExternal();
}


Worker::Worker(TaskQueue::WorkerContext *queue, uint32_t threadId, uint32_t workerId, StringView name)
: _queue(queue), _refCount(1), _shouldQuit(), _managerId(threadId), _workerId(workerId), _name(name) {
	_queueRefId = _queue->queue->retain();
	_thread = std::thread(Worker::workerThread, this, queue->queue);
}

Worker::~Worker() {
	_queue->queue->release(_queueRefId);
}

uint64_t Worker::retain() {
	_refCount ++;
	return 0;
}

void Worker::release(uint64_t) {
	if (--_refCount <= 0) {
		_shouldQuit.clear();
	}
}

bool Worker::execute(Task *task) {
	memory::pool::push(_pool);
	auto ret = task->execute();
	memory::pool::pop();
	memory::pool::clear(_pool);
	return ret;
}

void Worker::threadInit() {
	memory::pool::initialize();
	_pool = memory::pool::createTagged(_name.data(), _flags);

	_shouldQuit.test_and_set();
	_threadId = std::this_thread::get_id();

	ThreadInfo::setThreadInfo(_managerId, _workerId, _name, true);
}

void Worker::threadDispose() {
	memory::pool::destroy(_pool);
	memory::pool::terminate();
}

bool Worker::worker() {
	if (!_shouldQuit.test_and_set()) {
		return false;
	} else {
		memory::pool::clear(_pool);
	}

	Rc<Task> task;

	if (!task) {
		task = _queue->popTask(_workerId);
	}

	if (!task) {
		std::unique_lock<std::mutex> lock(_queue->inputMutexQueue);
		if (_queue->tasksCounter.load() > 0) {
			// some task received after locking
			return true;
		}
		_queue->wait(lock);
		return true;
	}

	task->setSuccessful(execute(task));
	_queue->onMainThreadWorker(std::move(task));

	return true;
}

std::thread &Worker::getThread() {
	return _thread;
}

}
