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

#ifndef CORE_EVENT_PLATFORM_URING_SPEVENTTHREADHANDLE_H_
#define CORE_EVENT_PLATFORM_URING_SPEVENTTHREADHANDLE_H_

#include "SPEventThreadHandle.h"
#include "SPEvent-uring.h"

namespace STAPPLER_VERSIONIZED stappler::event {

class FutexImpl {
public:
	static constexpr uint32_t CLIENT_MASK = 0x01;
	static constexpr uint32_t SERVER_MASK = 0x02;
	static constexpr uint32_t FULL_MASK = CLIENT_MASK | SERVER_MASK;

	void client_lock();
	bool server_try_lock();

	bool server_unlock();
	void client_unlock();

	volatile uint32_t *getAddr() { return &_futex; }

	uint32_t load();

private:
	// 0 means unlocked
	// 1 means locked, no waiters
	// 2 means locked, there are waiters in lock()
	volatile uint32_t _futex = 0;
};

class ThreadUringHandle : public ThreadHandle {
public:
	virtual ~ThreadUringHandle() = default;

	bool init(URingData *);

	Status rearm(FdSource *, bool unlock = false);
	Status disarm(FdSource *, bool suspend);

	void notify(FdSource *source, int32_t res, uint32_t flags);

	Status perform(Rc<thread::Task> &&task) override;
	Status perform(mem_std::Function<void()> &&func, Ref *target) override;

protected:
	FutexImpl _futex;
};

}

#endif /* CORE_EVENT_PLATFORM_URING_SPEVENTTHREADHANDLE_H_ */
