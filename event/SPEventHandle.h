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

#ifndef CORE_EVENT_SPEVENTHANDLE_H_
#define CORE_EVENT_SPEVENTHANDLE_H_

#include "SPEventBufferChain.h"
#include "SPEventQueue.h"

namespace STAPPLER_VERSIONIZED stappler::event {

class Queue;
struct QueueData;

class Handle : public Ref {
public:
	static constexpr size_t DataSize = 64;

	static inline bool isValidCancelStatus(Status st) {
		return st == Status::Done || !isSuccessful(st);
	}

	virtual ~Handle();

	Handle();

	bool init(QueueRef *, QueueData *);

	Queue *getQueue() const { return _queue; }

	// Initially, handle in Declined state
	// When handle run within queue - it's on Ok state
	// When handle completes it's execution, it's on Done state
	// When handle's execution were suspended (like on graceful wakeup) - it's on Suspended state
	Status getStatus() const { return _status; }

	bool isResumable() const { return _suspendFn != nullptr && _resumeFn != nullptr; }

	template <typename T = uint8_t>
	T *getData() const { return (T *)_data; }

	// Cancel handle operation (performed asynchronically)
	Status cancel(Status);

	uint32_t getValue() const { return _value; }

protected:
	friend class Queue;
	friend struct QueueData;

	void setValue(uint32_t c) { _value = c; }

	Status addPending(Rc<Handle> &&);

	Status run();
	Status suspend();
	Status resume();

	template <typename T, typename S>
	void setup() {
		_runFn = [] (Handle *h) -> Status {
			return ((T *)h)->rearm(h->getData<S>());
		};

		_suspendFn = [] (Handle *h) -> Status {
			return ((T *)h)->disarm(h->getData<S>(), true);
		};

		_resumeFn = [] (Handle *h) -> Status {
			return ((T *)h)->rearm(h->getData<S>());
		};
	}

	Status prepareRearm();
	Status prepareDisarm(bool suspend);

	void sendCompletion(uint32_t value, Status);
	void finalize(Status);

	Rc<QueueRef> _queue;
	mem_std::Vector<Rc<Handle>> _pendingHandles;

	QueueData *_queueData = nullptr;

	Status _status = Status::Declined;

	Status (*_runFn) (Handle *) = nullptr;
	Status (*_suspendFn) (Handle *) = nullptr;
	Status (*_resumeFn) (Handle *) = nullptr;

	CompletionHandle<void> _completion;
	uint32_t _value = 0;

	// platform data block
	alignas(void *) uint8_t _data[DataSize];
};

}

#endif /* CORE_EVENT_SPEVENTHANDLE_H_ */
