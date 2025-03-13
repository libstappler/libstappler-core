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
#include <sys/eventfd.h>

namespace STAPPLER_VERSIONIZED stappler::event {

// futex implementation based on https://github.com/eliben/code-for-blog/blob/main/2018/futex-basics/mutex-using-futex.cpp

static long futex_wake(volatile uint32_t *uaddr, uint32_t bitset, int nr_wake, uint32_t flags) {
	return syscall(SYS_futex_wake, uaddr, bitset, nr_wake, flags);
}

static long futex_wait(volatile uint32_t *uaddr, uint32_t val, uint32_t mask,
		uint32_t flags, __kernel_timespec *timespec, clockid_t clockid) {
	return syscall(SYS_futex_wait, uaddr, val, mask, flags, timespec, clockid);
}

static uint32_t atomicLoadSeq(volatile uint32_t *ptr) {
	uint32_t ret;
	__atomic_load(ptr, &ret, __ATOMIC_SEQ_CST);
	return ret;
}

static uint32_t atomicFetchOr(volatile uint32_t *ptr, uint32_t value) {
	return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST);
}

static uint32_t atomicFetchAnd(volatile uint32_t *ptr, uint32_t value) {
	return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST);
}

static uint32_t atomicExchange(volatile uint32_t *ptr, uint32_t value) {
	return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static constexpr uint32_t LOCK_VALUE =   0b0001;
static constexpr uint32_t WAIT_VALUE =   0b0010;
static constexpr uint32_t SIGNAL_VALUE = 0b0100;
static constexpr uint32_t FULL_VALUE = LOCK_VALUE | SIGNAL_VALUE | WAIT_VALUE;

void FutexImpl::client_lock() {
	// try to mark futex to own it
	uint32_t c = atomicFetchOr(&_futex, LOCK_VALUE | SIGNAL_VALUE);
	if ((c & LOCK_VALUE) != 0) {
		// prev value already has LOCK flag, wait
		do {
			// if wait flag is set already or we still locked
			if ((c & WAIT_VALUE) != 0 || (atomicFetchOr(&_futex, WAIT_VALUE) & LOCK_VALUE) != 0) {
				// futex should have all three flags set at this moment
				// wait for unlock
				futex_wait(&_futex, FULL_VALUE,
						CLIENT_MASK, FUTEX2_SIZE_U32 | FUTEX2_PRIVATE, nullptr, CLOCK_MONOTONIC);
			}
			// check if lock still in place by fetching value and set all flags
		} while (((c = atomicFetchOr(&_futex, FULL_VALUE)) & LOCK_VALUE) != 0);
	}
}

bool FutexImpl::client_try_lock() {
	// returns true if successfully locked
	// set LOCK flag, return true if it was not set (so, we successfully locked)
	return (atomicFetchOr(&_futex, LOCK_VALUE | SIGNAL_VALUE) & LOCK_VALUE) == 0;
}

// client will always call FUTEX_WAKE, because server is always waiting
void FutexImpl::client_unlock() {
	// drop LOCK flag, leave WAIT and SIGNAL in place
	if ((atomicFetchAnd(&_futex, SIGNAL_VALUE | WAIT_VALUE) & SIGNAL_VALUE) == 0) {
		std::cout << "+";
	}

	// wake server or clients
	futex_wake(&_futex, FULL_MASK, 1, FUTEX2_SIZE_U32 | FUTEX2_PRIVATE);
}

bool FutexImpl::server_try_lock() {
	// returns true if successfully locked
	// set LOCK flag, return true if it was not set (so, we successfully locked)
	return (atomicFetchOr(&_futex, LOCK_VALUE) & LOCK_VALUE) == 0;
}

bool FutexImpl::server_unlock() {
	// unset all, return true if WAIT was set
	if ((atomicExchange(&_futex, 0) & WAIT_VALUE) != 0) {
		futex_wake(&_futex, CLIENT_MASK, 1, FUTEX2_SIZE_U32 | FUTEX2_PRIVATE);
		return true;
	}
	return false;
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

	source->setTimeoutInterval(TimeInterval(), TimeInterval::microseconds(50000));

	source->setURingCallback(uring, [] (Handle *h, int32_t res, uint32_t flags, URingUserFlags uflags) {
		reinterpret_cast<ThreadUringHandle *>(h)->notify(h->getData<FdSource>(), res, flags, uflags);
	});

	return true;
}

Status ThreadUringHandle::rearm(FdSource *source, bool unlock, bool init) {
	auto status = prepareRearm();
	if (status == Status::Ok) {
		auto uring = source->getURingData();

		if (unlock) {
			_futex.server_unlock();
		}

		if (init) {
			_thisThread = std::this_thread::get_id();
		}

		if (!_failsafe) {
			rearmFailsafe(source);
		}

		auto result = uring->pushSqe({IORING_OP_FUTEX_WAIT}, [&] (io_uring_sqe *sqe, uint32_t n) {
			sqe->fd = FUTEX2_SIZE_U32 | FUTEX2_PRIVATE;
			sqe->futex_flags = 0;
			sqe->len = 0;
			sqe->addr = reinterpret_cast<uintptr_t>(_futex.getAddr());
			sqe->addr2 = 0; // we can lock only on 0
			sqe->addr3 = FutexImpl::SERVER_MASK;
			uring->retainHandleForSqe(sqe, this);
		}, URingPushFlags::Submit);

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
		auto uring = source->getURingData();
		if (_failsafe) {
			uring->pushSqe({IORING_OP_TIMEOUT_REMOVE}, [&] (io_uring_sqe *sqe, uint32_t n) {
				sqe->len = 0;
				sqe->addr = reinterpret_cast<uintptr_t>(this) | uintptr_t(toInt(URingUserFlags::Alternative));
				sqe->off = 0;
				sqe->user_data = URING_USERDATA_IGNORED;
			}, URingPushFlags::None);
			_failsafe = false;
		}

		status = uring->cancelOpRelease(this, suspend ? URingCancelFlags::Suspend : URingCancelFlags::None);
	}
	return status;
}

void ThreadUringHandle::notify(FdSource *source, int32_t res, uint32_t flags, URingUserFlags uflags) {
	// !!! futex now should be on 0
	if (_status != Status::Ok) {
		return; // just exit
	}

	if (hasFlag(uflags, URingUserFlags::Alternative)) {
		if ((flags & IORING_CQE_F_MORE) == 0) {
			// rearm failsafe
			std::cout << "Thread: IORING_CQE_F_MORE\n";
			rearmFailsafe(source);
		}
		if (_futex.server_try_lock()) {
			auto ev = performAll([&] (uint32_t count) {
				if constexpr (URING_THREAD_DEBUG_SWITCH_TIMER) {
					if (count == 1) {
						log::info("event::ThreadUringHandle", "B ", sp::platform::nanoclock(ClockType::Monotonic) - _switchTimer);
					}
				}
				_futex.server_unlock();
			});
			if (ev > 0) {
				//log::info("event::ThreadUringHandle", "events was processed with failsafe timer: ", ev);
			}
		}
	} else {
		_status = Status::Suspended;

		if (res < 0 && res != -EAGAIN) {
			cancel(URingData::getErrnoStatus(res));
			return;
		}

		// try to capture futex
		auto success = _futex.server_try_lock();
		if (!success) {
			// wait for a new wakeup
			rearm(source, false, false);
			return;
		} else {
			// now we own futex
			auto ev = performAll([&] (uint32_t count) {
				if constexpr (URING_THREAD_DEBUG_SWITCH_TIMER) {
					if (count == 1) {
						log::info("event::ThreadUringHandle", "A ", sp::platform::nanoclock(ClockType::Monotonic) - _switchTimer);
					}
				}
				rearm(source, true, false);
			});
			if (ev > 0) {
				//log::info("event::ThreadUringHandle", "events was processed with futex: ", ev);
			}
		}
	}
}

Status ThreadUringHandle::perform(Rc<thread::Task> &&task) {
	if (std::this_thread::get_id() == _thisThread) {
		// Ensure that server will be notified by setting SIGNAL flag.
		// Other thread will issue FITEX_WAKE, if we fail to lock, so
		// just add task into non-protected queue
		if (_futex.client_try_lock()) {
			_outputQueue.emplace_back(move(task));
			_futex.client_unlock();
		} else {
			_unsafeQueue.emplace_back(move(task));
		}
	} else {
		_futex.client_lock();
		_outputQueue.emplace_back(move(task));
		if constexpr (URING_THREAD_DEBUG_SWITCH_TIMER) {
			_switchTimer = sp::platform::nanoclock(ClockType::Monotonic);
		}
		_futex.client_unlock();
	}
	return Status::Ok;
}

Status ThreadUringHandle::perform(mem_std::Function<void()> &&func, Ref *target) {
	if (std::this_thread::get_id() == _thisThread) {
		// Ensure that server will be notified by setting SIGNAL flag.
		// Other thread will issue FITEX_WAKE, if we fail to lock, so
		// just add task into non-protected queue
		if (_futex.client_try_lock()) {
			_outputCallbacks.emplace_back(sp::move(func), target);
			_futex.client_unlock();
		} else {
			_unsafeCallbacks.emplace_back(sp::move(func), target);
		}
	} else {
		_futex.client_lock();
		_outputCallbacks.emplace_back(sp::move(func), target);
		if constexpr (URING_THREAD_DEBUG_SWITCH_TIMER) {
			_switchTimer = sp::platform::nanoclock(ClockType::Monotonic);
		}
		_futex.client_unlock();
	}
	return Status::Ok;
}

void ThreadUringHandle::rearmFailsafe(FdSource *source) {
	auto uring = source->getURingData();
	uring->pushSqe({IORING_OP_TIMEOUT}, [&] (io_uring_sqe *sqe, uint32_t n) {
		sqe->fd = -1;
		sqe->len = 1;
		sqe->addr = reinterpret_cast<uintptr_t>(&source->getInterval());
		sqe->off = 0;
		sqe->user_data = reinterpret_cast<uintptr_t>(this) | uintptr_t(toInt(URingUserFlags::Alternative));
		sqe->timeout_flags = IORING_TIMEOUT_MULTISHOT | IORING_TIMEOUT_ETIME_SUCCESS;
	}, URingPushFlags::Submit);
	_failsafe = true;
}

bool ThreadEventFdHandle::init(URingData *uring) {
	static_assert(sizeof(EventFdSource) <= DataSize && std::is_standard_layout<EventFdSource>::value);

	auto source = new (_data) EventFdSource;

	if (!source->init() || !ThreadHandle::init(uring->_queue, uring->_data)) {
		return false;
	}

	setup<ThreadEventFdHandle, EventFdSource>();

	source->setURingCallback(uring, [] (Handle *h, int32_t res, uint32_t flags, URingUserFlags uflags) {
		reinterpret_cast<ThreadEventFdHandle *>(h)->notify(h->getData<EventFdSource>(), res, flags, uflags);
	});

	return true;
}

Status ThreadEventFdHandle::rearm(EventFdSource *source, bool updateBuffers) {
	auto status = prepareRearm();
	if (status == Status::Ok) {
		auto uring = source->getURingData();

		if (hasFlag(uring->_uflags, URingFlags::ReadMultishotSupported)) {
			auto fillMultishot = [&] (io_uring_sqe *sqe) {
				sqe->fd = source->getFd();
				sqe->buf_group = _bufferGroup;
				sqe->off = -1;
				sqe->user_data = reinterpret_cast<uintptr_t>(this);
				sqe->flags |= IOSQE_BUFFER_SELECT;
			};

			if (!_bufferGroup) {
				_bufferGroup = uring->registerBufferGroup(TARGET_BUFFER_COUNT,
					sizeof(uint64_t), (uint8_t *)_targetBuf);
			} else if (updateBuffers) {
				_bufferGroup = uring->reloadBufferGroup(_bufferGroup, TARGET_BUFFER_COUNT,
					sizeof(uint64_t), (uint8_t *)_targetBuf);
			}

			status = uring->pushSqe({IORING_OP_READ_MULTISHOT}, [&] (io_uring_sqe *sqe, uint32_t) {
				fillMultishot(sqe);
			}, URingPushFlags::Submit);
		} else {
			status = uring->pushSqe({IORING_OP_READ}, [&] (io_uring_sqe *sqe, uint32_t) {
				sqe->fd = source->getFd();
				sqe->addr = reinterpret_cast<uintptr_t>(_targetBuf);
				sqe->len = sizeof(uint64_t);
				sqe->off = -1;
				sqe->user_data = reinterpret_cast<uintptr_t>(this);
			}, URingPushFlags::Submit);
		}
	}
	return status;
}

Status ThreadEventFdHandle::disarm(EventFdSource *source, bool suspend) {
	auto status = prepareDisarm(suspend);
	if (status == Status::Ok) {
		auto uring = source->getURingData();
		status = source->getURingData()->cancelOp(this, URingUserFlags::None,
				suspend ? URingCancelFlags::Suspend : URingCancelFlags::None);

		if (_bufferGroup) {
			uring->unregisterBufferGroup(_bufferGroup, TARGET_BUFFER_COUNT);
			_bufferGroup = 0;
		}
	}
	return status;
}

void ThreadEventFdHandle::notify(EventFdSource *source, int32_t res, uint32_t flags, URingUserFlags uflags) {
	if (_status != Status::Ok) {
		return;
	}

	//// we don't need this result here
	// uint64_t target = &_targetBuf[0];
	// if (flags & IORING_CQE_F_BUFFER) {
	// 	target = &_targetBuf[(flags >> 16) & 0xFFFF];
	// }

	_status = Status::Suspended;

	auto performUnlock = [&] {
		performAll([&] (uint32_t count) {
			if constexpr (URING_THREAD_DEBUG_SWITCH_TIMER) {
				if (count == 1) {
					log::info("event::ThreadUringHandle", "C ", sp::platform::nanoclock(ClockType::Monotonic) - _switchTimer);
				}
			}
			_mutex.unlock();
		});

		if ((flags & IORING_CQE_F_MORE) == 0) {
			rearm(source);
		} else {
			// no need to rearm, handle is still armed
			_status = Status::Ok;
		}
	};

	if (res == sizeof(uint64_t)) {
		if constexpr (URING_THREAD_NONBLOCK) {
			if (_mutex.try_lock()) {
				performUnlock();
			} else {
				if ((flags & IORING_CQE_F_MORE) == 0) {
					rearm(source);
				}
				uint64_t value = 1;
				::eventfd_write(getData<EventFdSource>()->getFd(), value);
			}
		} else {
			_mutex.lock();
			performUnlock();
		}
	} else if (res == -ENOBUFS) {
		// Rearm buffers only when multishot is failed
		rearm(source, true);
	} else {
		cancel(URingData::getErrnoStatus(res));
	}
}

Status ThreadEventFdHandle::perform(Rc<thread::Task> &&task) {
	std::unique_lock lock(_mutex);
	_outputQueue.emplace_back(move(task));

	if constexpr (URING_THREAD_DEBUG_SWITCH_TIMER) {
		_switchTimer = sp::platform::nanoclock(ClockType::Monotonic);
	}

	uint64_t value = 1;
	::eventfd_write(getData<EventFdSource>()->getFd(), value);
	return Status::Ok;
}

Status ThreadEventFdHandle::perform(mem_std::Function<void()> &&func, Ref *target) {
	std::unique_lock lock(_mutex);
	_outputCallbacks.emplace_back(sp::move(func), target);

	if constexpr (URING_THREAD_DEBUG_SWITCH_TIMER) {
		_switchTimer = sp::platform::nanoclock(ClockType::Monotonic);
	}

	uint64_t value = 1;
	::eventfd_write(getData<EventFdSource>()->getFd(), value);
	return Status::Ok;
}

}
