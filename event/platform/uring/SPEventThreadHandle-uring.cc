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

#include "SPEventThreadHandle-uring.h"

#include <linux/futex.h>
#include <sys/syscall.h>

namespace STAPPLER_VERSIONIZED stappler::event {

static long futex_wake(volatile uint32_t *uaddr, uint32_t bitset, int nr_wake, uint32_t flags) {
	return syscall(SYS_futex_wake, uaddr, bitset, nr_wake, flags);
}

static long futex_wait(volatile uint32_t *uaddr, uint32_t val, uint32_t mask,
		uint32_t flags, __kernel_timespec *timespec, clockid_t clockid) {
	return syscall(SYS_futex_wait, uaddr, val, mask, flags, timespec, clockid);
}

static uint32_t cmpxchg(volatile uint32_t *futex, uint32_t expected, uint32_t desired) {
	auto ep = &expected;
	__atomic_compare_exchange(futex, ep, &desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return *ep;
}

static void atomicStoreSeq(volatile uint32_t *ptr, uint32_t value) {
	__atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static uint32_t atomicLoadSeq(volatile uint32_t *ptr) {
	uint32_t ret;
	__atomic_load(ptr, &ret, __ATOMIC_SEQ_CST);
	return ret;
}

static uint32_t atomicFetchSub(volatile uint32_t *ptr, uint32_t value) {
	return __atomic_sub_fetch(ptr, value, __ATOMIC_SEQ_CST);
}

void FutexImpl::client_lock() {
	// try to mark futex to own it
	uint32_t c = cmpxchg(&_futex, 0, 1);
	if (c != 0) {
		// failed, run wait loop
		do {
			// mark as waited (2)
			if (c == 2 || cmpxchg(&_futex, 1, 2) != 0) {
				// wait until wakeup
				futex_wait(&_futex, 2, CLIENT_MASK, FUTEX2_SIZE_U32 | FUTEX2_PRIVATE, nullptr, CLOCK_MONOTONIC);
			}
		} while ((c = cmpxchg(&_futex, 0, 2)) != 0);
	}
}

bool FutexImpl::server_try_lock() {
	// returns true if successfully locked
	return cmpxchg(&_futex, 0, 1) == 0;
}

bool FutexImpl::server_unlock() {
	// if _futex value was 2 (so, result is not 1) - we have waiters, that need to be woken
	if (atomicFetchSub(&_futex, 1) != 1) {
		atomicStoreSeq(&_futex, 0);
		return true; // signal to wake client
	}
	return false; // no waiters
}

// client will always call FUTEX_WAKE, because server is always waiting
void FutexImpl::client_unlock() {
	atomicStoreSeq(&_futex, 0);

	// wake server or clients
	futex_wake(&_futex, FULL_MASK, 1, FUTEX2_SIZE_U32 | FUTEX2_PRIVATE);
}

uint32_t FutexImpl::load() {
	return atomicLoadSeq(&_futex);
}

bool ThreadUringHandle::init(URingData *uring) {
	static_assert(sizeof(FdSource) <= DataSize && std::is_standard_layout<FdSource>::value);

	auto source = new (_data) FdSource;

	if (!source->init(-1) || !ThreadHandle::init(uring->_queue, uring->_data)) {
		return false;
	}

	setup<ThreadUringHandle, FdSource>();

	source->setURingCallback(uring, [] (Handle *h, int32_t res, uint32_t flags) {
		reinterpret_cast<ThreadUringHandle *>(h)->notify(h->getData<FdSource>(), res, flags);
	});

	return true;
}

Status ThreadUringHandle::rearm(FdSource *source, bool unlock) {
	auto status = prepareRearm();
	if (status == Status::Ok) {
		auto uring = source->getURingData();
		auto result = uring->pushSqe({IORING_OP_FUTEX_WAIT}, [&] (io_uring_sqe *sqe, uint32_t n) {
			sqe->fd = FUTEX2_SIZE_U32 | FUTEX2_PRIVATE;
			sqe->futex_flags = 0;
			sqe->len = 0;
			sqe->addr = reinterpret_cast<uintptr_t>(_futex.getAddr());
			sqe->addr2 = _futex.load(); // force to wait, prevent to immediate EAGAIN, we are still locked
			sqe->addr3 = FutexImpl::SERVER_MASK;
			uring->retainHandleForSqe(sqe, this);
		});
		if (result == Status::Suspended) {
			uring->submitPending();
		}
		if (unlock && _futex.server_unlock()) {
			// we need to wake others
			result = uring->pushSqe({IORING_OP_FUTEX_WAKE}, [&] (io_uring_sqe *sqe, uint32_t n) {
				sqe->addr = reinterpret_cast<uintptr_t>(_futex.getAddr());
				sqe->addr2 = 1;
				sqe->addr3 = FutexImpl::CLIENT_MASK;
				sqe->user_data = URING_USERDATA_IGNORED;
			});
			if (result == Status::Suspended) {
				uring->submitPending();
			}
		}
		if (isSuccessful(result)) {
			_status = Status::Ok;
		}
	} else if (unlock) {
		// force client unlock if server is dead
		_futex.client_unlock();
	}
	return status;
}

Status ThreadUringHandle::disarm(FdSource *source, bool suspend) {
	auto status = prepareDisarm(suspend);
	if (status == Status::Ok) {
		status = source->getURingData()->cancelOpRelease(this, suspend ? URingCancelFlags::Suspend : URingCancelFlags::None);
	}
	return status;
}

void ThreadUringHandle::notify(FdSource *source, int32_t res, uint32_t flags) {
	// !!! futex now should be on 0
	if (_status != Status::Ok) {
		return; // just exit
	}

	_status = Status::Suspended;

	std::cout << res << "\n";

	if (res < 0 && res != -EAGAIN) {
		cancel(URingData::getErrnoStatus(res));
		return;
	}

	// try to capture futex
	auto success = _futex.server_try_lock();
	if (!success) {
		// wait for a new wakeup
		rearm(source, false);
		return;
	} else {
		// now we own futex
		performAll([&] {
			rearm(source, true);
		});
	}
}

Status ThreadUringHandle::perform(Rc<thread::Task> &&task) {
	_futex.client_lock();

	_outputQueue.emplace_back(move(task));

	_futex.client_unlock();
	return Status::Ok;
}

Status ThreadUringHandle::perform(mem_std::Function<void()> &&func, Ref *target) {
	_futex.client_lock();

	_outputCallbacks.emplace_back(sp::move(func), target);

	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	_futex.client_unlock();
	return Status::Ok;
}


}
