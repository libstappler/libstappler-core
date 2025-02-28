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

#include "SPEventQueue.h"
#include "SPPlatformUnistd.h"

#if LINUX

#include "SPEvent-uring.h"
#include <signal.h>

namespace STAPPLER_VERSIONIZED stappler::event {

static int io_uring_setup(unsigned entries, struct io_uring_params *p) {
	return (int) syscall(__NR_io_uring_setup, entries, p);
}

static int io_uring_enter(int ring_fd, unsigned int to_submit, unsigned int min_complete,
		unsigned int flags, const sigset_t *sig = nullptr) {
	return (int) syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete, flags, sig, 0);
}

static int io_uring_enter2(int ring_fd, unsigned int to_submit, unsigned int min_complete,
		unsigned int flags, void *arg = nullptr, size_t argsize = 0) {
	return (int) syscall(__NR_io_uring_enter, ring_fd, to_submit, min_complete, flags, arg, argsize);
}

static int io_uring_register(unsigned int fd, unsigned int opcode, const void *arg, unsigned int nr_args) {
	return (int) syscall(__NR_io_uring_register, fd, opcode, arg, nr_args);
}

static unsigned atomicLoadAcquire(unsigned *ptr) {
	unsigned result;
	__atomic_load(ptr, &result, __ATOMIC_ACQUIRE);
	return result;
}

static unsigned atomicLoadRelaxed(unsigned *ptr) {
	unsigned result;
	__atomic_load(ptr, &result, __ATOMIC_RELAXED);
	return result;
}

static void atomicStoreRelease(unsigned *ptr, unsigned value) {
	__atomic_store_n(ptr, value, __ATOMIC_RELEASE);
}

// Ignore -1 error in debug mode - it indicated debugger interrupt
#if DEBUG
static constexpr int DEBUG_ERROR_THRESHOLD = -1;
#else
static constexpr int DEBUG_ERROR_THRESHOLD = 0;
#endif

static constexpr uint64_t URING_USERDATA_IGNORED = maxOf<uint64_t>() & URingData::UserdataPointerMask;
static constexpr uint64_t URING_USERDATA_DRAIN = maxOf<uint64_t>() & (URingData::UserdataPointerMask << 1);
static constexpr uint64_t URING_USERDATA_TIMEOUT = maxOf<uint64_t>() & (URingData::UserdataPointerMask << 2);

bool URingData::checkSupport() {
	struct utsname buffer;

	if (uname(&buffer) != 0) {
		log::info("event::URingData", "Fail to detect kernel version");
		return false;
	}

	if (strverscmp(buffer.release, "5.15.0") < 0) {
		log::info("event::URingData", "io_uring backend available since 5.15 kernel release, current release: ", buffer.release);
		return false;
	}

	if (syscall(__NR_io_uring_register, 0, IORING_UNREGISTER_BUFFERS, NULL, 0) && errno == ENOSYS) {
		log::info("event::URingData", "io_uring disabled in OS");
		return false;
	} else {
		return true;
	}
}

unsigned URingData::getUnprocessedSqeCount() {
	unsigned head;

	if (_params.flags & IORING_SETUP_SQPOLL) {
		head = atomicLoadAcquire(sq.head);
	} else {
		head = *sq.head;
	}

	/* always use real head, to avoid losing sync for short submit */
	return sq.userspaceTail - head;
}

unsigned URingData::flushSqe() {
	unsigned tail = sq.userspaceTail;

	if (sq.userspaceHead != tail) {
		sq.userspaceHead = tail;

		if ((_params.flags & IORING_SETUP_SQPOLL) == 0) {
			*sq.tail = tail;
		} else {
			atomicStoreRelease(sq.tail, tail);
		}
	}

	return tail - atomicLoadRelaxed(sq.head);
}

URingData::SqeBlock URingData::tryGetNextSqe(uint32_t count) {
	unsigned int head, first = sq.userspaceTail, next = sq.userspaceTail + count;

	if ((_params.flags & IORING_SETUP_SQPOLL) == 0) {
		head = *sq.head;
	} else {
		head = atomicLoadAcquire(sq.head);
	}

	if (next - head <= *sq.entries) {
		struct io_uring_sqe *sqe;

		auto index = (sq.userspaceTail & *sq.mask);
		sqe = &sq.sqes[index];

		sq.array[index] = index;
		sq.userspaceTail = next;

		memset(sqe, 0, sizeof(io_uring_sqe));

		return SqeBlock{sqe, first, count, Status::Ok};
	}

	return SqeBlock{nullptr, 0, 0, Status::ErrorBusy};
}

URingData::SqeBlock URingData::getNextSqe(uint32_t count) {
	if (count > _params.sq_entries) {
		return SqeBlock{nullptr, 0, 0, Status::ErrorInvalidArguemnt};
	}

	auto sqe = tryGetNextSqe(count);
	if (!sqe) {
		int ret = submitSqe(flushSqe(), 0, true);
		if (ret < 0) {
			log::error("event::URingData", "getNextSqe(): io_uring_enter failed: ", ret);
			return SqeBlock{nullptr, 0, 0, Status(getErrnoStatus(ret))};
		}
		sqe = tryGetNextSqe(count);
		if (!sqe) {
			log::warn("event::URingData", "getNextSqe(): io_uring_enter on timoeut (possible IORING_SETUP_SQPOLL overload)");
			return SqeBlock{nullptr, 0, 0, Status::ErrorBusy};
		}
	}
	return sqe;
}

Status URingData::pushSqe(std::initializer_list<uint8_t> ops, const Callback<void(io_uring_sqe *, uint32_t n)> &cb) {
	for (auto &it : ops) {
		if (!_probe.isOpcodeSupported(it)) {
			return Status::ErrorNotImplemented;
		}
	}

	auto sqe = getNextSqe(uint32_t(ops.size()));
	if (sqe) {
		uint32_t n = 0;
		while (n < sqe.count) {
			auto index = ((sqe.first ++) & *sq.mask);
			auto ptr = &sq.sqes[index];

			sq.array[index] = index;

			ptr->opcode = *(ops.begin() + n);
			ptr->flags = 0;
			cb(ptr, n ++);

			if (n < sqe.count) {
				ptr->flags |= IOSQE_IO_LINK;
			}
		}

		// submit now for IORING_SETUP_SQPOLL
		if ((_params.flags & IORING_SETUP_SQPOLL) != 0) {
			submitPending();
			return Status::Done;
		}
		return Status::Suspended;
	}
	return sqe.status;
}

Status URingData::pushSqe(uint8_t op, const Callback<void(io_uring_sqe *)> &cb, const __kernel_timespec *ts) {
	if (ts) {
		return pushSqe({op, uint8_t(IORING_OP_LINK_TIMEOUT)}, [&] (io_uring_sqe *sqe, uint32_t n) {
			switch (n) {
			case 0: cb(sqe); break;
			case 1:
				sqe->addr = reinterpret_cast<uintptr_t>(ts);
				sqe->len = 1;
				sqe->timeout_flags = 0;
				sqe->off = 0;
				sqe->user_data = URING_USERDATA_TIMEOUT;
				break;
			default: break;
			}
		});
	} else {
		return pushSqe({op}, [&] (io_uring_sqe *sqe, uint32_t) {
			cb(sqe);
		});
	}
}

inline void updateIoSqe(io_uring_sqe *sqe, int fd, const void *addr, unsigned len, uint64_t offset, uint64_t udata) {
	sqe->fd = fd;
	sqe->len = len;
	sqe->off = offset;
	sqe->addr = reinterpret_cast<uintptr_t>(addr);
	sqe->user_data = udata;
}

inline void updateIoSqe(io_uring_sqe *sqe, int fd, uint64_t addr, unsigned len, uint64_t offset, uint64_t udata) {
	sqe->fd = fd;
	sqe->len = len;
	sqe->off = offset;
	sqe->addr = addr;
	sqe->user_data = udata;
}

static bool isCqePending(unsigned flags) {
	return (flags & (IORING_SQ_CQ_OVERFLOW
#ifdef IORING_SQ_TASKRUN
			| IORING_SQ_TASKRUN
#endif
			)) != 0;
}

int URingData::submitSqe(unsigned sub, unsigned wait, bool waitAvailable) {
	std::atomic_thread_fence(std::memory_order_seq_cst);

	unsigned targetFlags = 0;
	unsigned sourceFlags = atomicLoadRelaxed(sq.flags);
	if (wait || isCqePending(sourceFlags)) {
		targetFlags |= IORING_ENTER_GETEVENTS;
	}

	if (sub > 0) {
		if (_params.flags & IORING_SETUP_SQPOLL) {
			if ((sourceFlags & IORING_SQ_NEED_WAKEUP) != 0) {
				targetFlags |= IORING_ENTER_SQ_WAKEUP;
			}
		} else {
			targetFlags |= IORING_ENTER_GETEVENTS;
		}
	}

	if (waitAvailable && (_params.flags & IORING_SETUP_SQPOLL) != 0) {
		targetFlags |= IORING_ENTER_SQ_WAIT;
	}

	if (targetFlags != 0) {
		return enter(sub, 0, targetFlags, nullptr);
	}
	return sub;
}

int URingData::submitPending() {
	return submitSqe(flushSqe(), 0, false);
}

Status URingData::pushRead(int fd, uint8_t *buf, size_t bsize, uint64_t userdata) {
	return pushSqe({IORING_OP_READ}, [&] (io_uring_sqe *sqe, uint32_t) {
		updateIoSqe(sqe, fd, buf, unsigned(bsize), -1, userdata);
	});
}

Status URingData::pushReadRetain(int fd, uint8_t *buf, size_t bsize, Handle *ref) {
	return pushSqe({IORING_OP_READ}, [&] (io_uring_sqe *sqe, uint32_t) {
		ref->retain(reinterpret_cast<uintptr_t>(this) ^ reinterpret_cast<uintptr_t>(ref));
		updateIoSqe(sqe, fd, buf, unsigned(bsize), -1,
				uint64_t(reinterpret_cast<uintptr_t>(ref)) | toInt(URingUserFlags::Retained));
	});
}

Status URingData::pushWrite(int fd, const uint8_t *buf, size_t bsize, uint64_t userdata) {
	return pushSqe({IORING_OP_WRITE}, [&] (io_uring_sqe *sqe, uint32_t) {
		updateIoSqe(sqe, fd, buf, unsigned(bsize), -1, userdata);
	});
}

Status URingData::pushWriteRetain(int fd, const uint8_t *buf, size_t bsize, Handle *ref) {
	return pushSqe({IORING_OP_READ}, [&] (io_uring_sqe *sqe, uint32_t) {
		ref->retain(reinterpret_cast<uintptr_t>(this) ^ reinterpret_cast<uintptr_t>(ref));
		updateIoSqe(sqe, fd, buf, unsigned(bsize), -1,
				uint64_t(reinterpret_cast<uintptr_t>(ref)) | toInt(URingUserFlags::Retained));
	});
}

Status URingData::cancelOp(void *ptr, URingUserFlags ptrFlags, URingCancelFlags cancelFlags) {
	if ((hasFlag(cancelFlags, URingCancelFlags::All) || hasFlag(cancelFlags, URingCancelFlags::Any))
			&& !hasFlag(_uflags, URingFlags::AsyncCancelAnyAllSupported)) {
		return Status::ErrorNotImplemented;
	}
	return pushSqe({IORING_OP_ASYNC_CANCEL}, [&] (io_uring_sqe *sqe, uint32_t) {
		updateIoSqe(sqe, -1, reinterpret_cast<uintptr_t>(ptr) | toInt(ptrFlags), 0, 0,
				hasFlag(cancelFlags, URingCancelFlags::Drain) ? URING_USERDATA_DRAIN : URING_USERDATA_IGNORED);
		sqe->cancel_flags =
				(hasFlag(cancelFlags, URingCancelFlags::All) ? IORING_ASYNC_CANCEL_ALL : 0)
				| (hasFlag(cancelFlags, URingCancelFlags::Any) ? IORING_ASYNC_CANCEL_ANY : 0);
		sqe->flags =
				(hasFlag(cancelFlags, URingCancelFlags::Drain) ? IOSQE_IO_DRAIN : 0);
	});
}

Status URingData::cancelFd(int fd, URingCancelFlags cancelFlags) {
	if ((hasFlag(cancelFlags, URingCancelFlags::All) || hasFlag(cancelFlags, URingCancelFlags::Any))
			&& !hasFlag(_uflags, URingFlags::AsyncCancelAnyAllSupported)) {
		return Status::ErrorNotImplemented;
	}
	if (hasFlag(cancelFlags, URingCancelFlags::FixedFile) && !hasFlag(_uflags, URingFlags::AsyncCancelFdFixedSupported)) {
		return Status::ErrorNotImplemented;
	}
	return pushSqe({IORING_OP_ASYNC_CANCEL}, [&] (io_uring_sqe *sqe, uint32_t) {
		updateIoSqe(sqe, fd, nullptr, 0, 0,
				hasFlag(cancelFlags, URingCancelFlags::Drain) ? URING_USERDATA_DRAIN : URING_USERDATA_IGNORED);
		sqe->cancel_flags =
				(hasFlag(cancelFlags, URingCancelFlags::All) ? IORING_ASYNC_CANCEL_ALL : 0)
				| (hasFlag(cancelFlags, URingCancelFlags::Any) ? IORING_ASYNC_CANCEL_ANY : 0)
				| (hasFlag(cancelFlags, URingCancelFlags::FixedFile) ? IORING_ASYNC_CANCEL_FD_FIXED : 0);
		sqe->flags =
				(hasFlag(cancelFlags, URingCancelFlags::Drain) ? IOSQE_IO_DRAIN : 0);
	});
}

uint32_t URingData::pop() {
	auto count = atomicLoadAcquire(cq.tail) - *cq.head;
	if (count) {
		auto cqes = _out.data();

		unsigned head = *cq.head;
		unsigned mask = *cq.mask;
		unsigned last;
		unsigned i = 0;

		last = head + count;
		for (;head != last; head++, i++)
			cqes[i] = cq.cqes[(head & mask)];

		atomicStoreRelease(cq.head, last);

		bool scheduleEnter = isCqePending(atomicLoadRelaxed(sq.flags));
		if (scheduleEnter) {
			_uflags |= URingFlags::PendingGetEvents;
		}

		// processEvent can schedule new submissions
		for (i = 0; i < count; ++ i) {
			auto &cqe = cqes[i];
			processEvent(cqe.res, cqe.flags, cqe.user_data);
		}

		if (hasFlag(_uflags, URingFlags::PendingGetEvents)) {
			enter(0, 0, IORING_ENTER_GETEVENTS, NULL);
		}
	}
	return count;
}

void URingData::processEvent(int32_t res, uint32_t flags, uint64_t userdata) {
	auto userFlags = URingUserFlags(userdata & UserdataFlagsMask);
	auto userptr = userdata & UserdataPointerMask;

	if (userdata == URING_USERDATA_IGNORED) {
		std::cout << "URING_USERDATA_IGNORED: " << res << " : " << flags << "\n";
		// do nothing
	} else if (userdata == URING_USERDATA_TIMEOUT) {
		std::cout << "URING_USERDATA_TIMEOUT: " << res << " : " << flags << "\n";
		// do nothing

	} else if (userdata == URING_USERDATA_DRAIN) {
		std::cout << "URING_USERDATA_DRAIN: " << res << " : " << flags << "\n";
		// drain event complete, perform wakeup
		if (res == 0) {
			_shouldWakeup.clear();
		} else {
			_shouldWakeup.clear();
		}
	} else if (userdata == reinterpret_cast<uintptr_t>(_signalFd)) {
		_signalFd->process();

		pushRead(_signalFd->getFd(), (uint8_t *)_signalFd->getInfo(), sizeof(signalfd_siginfo), reinterpret_cast<uintptr_t>(_signalFd));
		if ((_params.flags & IORING_SETUP_SQPOLL) == 0) {
			submitPending();
		}
	} else if (userdata == reinterpret_cast<uintptr_t>(_eventFd)) {
		auto flags = QueueWakeupFlags(_wakeupFlags.exchange(0));

		if (hasFlag(flags, QueueWakeupFlags::Graceful)) {
			_data->suspendAll();
			suspendSystemHandles();
		} else {
			_shouldWakeup.clear();
			pushRead(_eventFd->getFd(), (uint8_t *)_eventFd->getValue(), sizeof(uint64_t), reinterpret_cast<uintptr_t>(_eventFd));
		}
	} else if (userdata) {
		auto h = reinterpret_cast<Handle *>(userptr);

		auto d = (FdSource *)h->getData();
		d->getCallback()(h, res, flags);
		if (hasFlag(userFlags, URingUserFlags::Retained)) {
			h->release(reinterpret_cast<uintptr_t>(this) ^ reinterpret_cast<uintptr_t>(h));
		}
	} else {
		log::info("URingData", "no userdata: ", res, " ", flags);
	}
}

Status URingData::submit() {
	auto ret = submitSqe(flushSqe(), 0, false);
	if (ret >= 0) {
		return Status::Ok;
	} else {
		return Status::Declined;
	}
}

uint32_t URingData::poll() {
	uint32_t ret = 0;
	while (auto v = pop()) {
		ret += v;
	}
	return ret;
}

uint32_t URingData::wait(TimeInterval ival) {
	_events = poll();
	if (_events == 0) {
		while (true) {
			__kernel_timespec ts;
			setNanoTimespec(ts, ival);

			int err = enter(0, 1, IORING_ENTER_GETEVENTS, ival ? &ts : nullptr);

			_events += poll();

			if (err < DEBUG_ERROR_THRESHOLD) {
				log::error("event::URingData", "io_uring_enter: ", -err);
				break;
			} else if (err >= 0) {
				break;
			}
		}
	}
	return _events;
}

Status URingData::run(TimeInterval ival) {
	Status result = Status::Suspended;
	bool hasInterval = ival ? true : false;
	__kernel_timespec ts;

	_events = poll();
	_shouldWakeup.test_and_set();
	_start = _now = hasInterval ? platform::clock(ClockType::Monotonic) : 0;

	while (_shouldWakeup.test_and_set()) {
		if (hasInterval) {
			if (ival.toMicros() > _now - _start) {
				ival -= TimeInterval::microseconds(_now - _start);
			} else {
				result = Status::Done;
				break;
			}
		}

		_start = _now;

		setNanoTimespec(ts, ival);

		int err = enter(0, 1, IORING_ENTER_GETEVENTS, ival ? &ts : nullptr);

		_events += poll();

		if (err < DEBUG_ERROR_THRESHOLD) {
			log::error("event::URingData", "io_uring_enter: ", -err);
			result = Status::Declined;
			break;
		}

		if (hasInterval) {
			_now = platform::clock(ClockType::Monotonic);
		}
	}

	return result;
}

int URingData::enter(unsigned sub, unsigned wait, unsigned flags, __kernel_timespec *ts) {
	const sigset_t *sigset = nullptr;

	if (hasFlag(_flags, QueueFlags::Protected)) {
		sigset = _signalFd->getSigset();
	}

	_uflags &= ~URingFlags::PendingGetEvents;

	if ((sigset || ts) && (_params.features & IORING_FEAT_EXT_ARG) != 0) {
		struct io_uring_getevents_arg arg = {
			.sigmask = reinterpret_cast<uint64_t>(sigset),
			.sigmask_sz = sigset ? uint32_t(_NSIG / 8) : uint32_t(0),
			.ts = reinterpret_cast<uint64_t>(ts)
		};

		return io_uring_enter2(_ringFd, sub, wait, flags | IORING_ENTER_EXT_ARG, &arg, sizeof(io_uring_getevents_arg));
	} else {
		return io_uring_enter(_ringFd, sub, wait, flags, sigset);
	}
}

void URingData::suspendSystemHandles() {
	if (_signalFd && hasFlag(_flags, QueueFlags::Protected)) {
		cancelOp(_signalFd, URingUserFlags::None, URingCancelFlags::None);
	}

	auto wakeupTimeout = _wakeupTimeout.load();
	if (wakeupTimeout > 0) {
		setNanoTimespec(_wakeupTimespec, wakeupTimeout);
		pushSqe({IORING_OP_TIMEOUT, IORING_OP_TIMEOUT_REMOVE}, [&] (io_uring_sqe *sqe, uint32_t n) {
			switch (n) {
			case 0:
				sqe->addr = reinterpret_cast<uintptr_t>(&_wakeupTimespec);
				sqe->len = 1;
				sqe->off = 0;
				sqe->timeout_flags = 0;
				sqe->user_data = URING_USERDATA_TIMEOUT;
				sqe->flags = IOSQE_IO_HARDLINK;
				break;
			case 1:
				sqe->addr = URING_USERDATA_DRAIN;
				sqe->user_data = URING_USERDATA_IGNORED;
				break;
			default: break;
			}
		});
	}

	pushSqe({IORING_OP_NOP}, [&] (io_uring_sqe *sqe, uint32_t n) {
		sqe->user_data = URING_USERDATA_DRAIN;
		sqe->flags = IOSQE_IO_DRAIN;
	});

	submitPending();
}

void URingData::resumeSystemHandles() {
	pushRead(_signalFd->getFd(), (uint8_t *)_signalFd->getInfo(), sizeof(signalfd_siginfo), uint64_t(_signalFd));
	if (pushRead(_eventFd->getFd(), (uint8_t *)_eventFd->getValue(), sizeof(uint64_t), uint64_t(_eventFd)) == Status::Suspended) {
		submitPending();
	}
}

URingData::URingData(QueueRef *q, Queue::Data *data, SignalFdSource *sigFd, EventFdSource *evFd, const QueueInfo &info, QueueFlags f)
: _queue(q), _data(data), _flags(f) {
	int ringFd = -1;

	auto cleanup = [&] () {
		if (ringFd >= 0) {
			::close(ringFd);
		}
	};

	memset(&_params, 0, sizeof(_params));

	// check available features
	struct utsname buffer;
	if (uname(&buffer) != 0) {
		log::info("event::URingData", "Fail to detect kernel version");
		return;
	}

#ifdef IORING_SETUP_SUBMIT_ALL
	if (strverscmp(buffer.release, "5.18.0") <= 0) {
		_uflags |= URingFlags::SubmitAllSupported;
		_params.flags |= IORING_SETUP_SUBMIT_ALL;
	}
#endif

#ifdef IORING_SETUP_COOP_TASKRUN
	if (strverscmp(buffer.release, "5.19.0") <= 0) {
		_uflags |= URingFlags::CoopTaskrunSupported;
		_uflags |= URingFlags::AsyncCancelFdSupported;
		_params.flags |= (IORING_SETUP_COOP_TASKRUN | IORING_SETUP_TASKRUN_FLAG);
	}
#endif

#ifdef IORING_SETUP_SINGLE_ISSUER
	if (strverscmp(buffer.release, "6.0.0") <= 0) {
		_uflags |= URingFlags::SingleIssuerSupported;
		_uflags |= URingFlags::AsyncCancelFdFixedSupported;
		_params.flags |= IORING_SETUP_SINGLE_ISSUER;
	}
#endif

#ifdef IORING_SETUP_DEFER_TASKRUN
	if (strverscmp(buffer.release, "6.1.0") <= 0) {
		_uflags |= URingFlags::DeferTaskrunSupported;
		_params.flags |= IORING_SETUP_DEFER_TASKRUN;
	}
#endif

	if (hasFlag(_flags, QueueFlags::SubmitImmediate)) {
		_params.flags |= IORING_SETUP_SQPOLL;

		if (info.osIdleInterval) {
			_params.sq_thread_idle = DefaultIdleInterval;
		} else {
			_params.sq_thread_idle = static_cast<uint32_t>(info.osIdleInterval.toMillis());
		}
	}

	if (info.completeQueueSize != 0) {
		_params.flags |= IORING_SETUP_CQSIZE;
		_params.cq_entries = info.completeQueueSize;
	}

	_params.flags |= IORING_SETUP_CLAMP;

	ringFd = io_uring_setup(math::npot(info.submitQueueSize), &_params);
	if (ringFd < 0) {
		log::error("event::URingData", "io_uring_setup: Fail to setup io_uring instance");
		return;
	}

	mem_std::StringStream features;
	if (_params.features & IORING_FEAT_SINGLE_MMAP) { features << " " << "IORING_FEAT_SINGLE_MMAP"; }
	if (_params.features & IORING_FEAT_NODROP) { features << " " << "IORING_FEAT_NODROP"; }
	if (_params.features & IORING_FEAT_SUBMIT_STABLE) { features << " " << "IORING_FEAT_SUBMIT_STABLE"; }
	if (_params.features & IORING_FEAT_RW_CUR_POS) { features << " " << "IORING_FEAT_RW_CUR_POS"; }
	if (_params.features & IORING_FEAT_CUR_PERSONALITY) { features << " " << "IORING_FEAT_CUR_PERSONALITY"; }
	if (_params.features & IORING_FEAT_FAST_POLL) { features << " " << "IORING_FEAT_FAST_POLL"; }
	if (_params.features & IORING_FEAT_POLL_32BITS) { features << " " << "IORING_FEAT_POLL_32BITS"; }
	if (_params.features & IORING_FEAT_SQPOLL_NONFIXED) { features << " " << "IORING_FEAT_SQPOLL_NONFIXED"; }
	if (_params.features & IORING_FEAT_EXT_ARG) { features << " " << "IORING_FEAT_EXT_ARG"; }
	if (_params.features & IORING_FEAT_NATIVE_WORKERS) { features << " " << "IORING_FEAT_NATIVE_WORKERS"; }
	if (_params.features & IORING_FEAT_RSRC_TAGS) { features << " " << "IORING_FEAT_RSRC_TAGS"; }

#ifdef IORING_FEAT_CQE_SKIP
	if (_params.features & IORING_FEAT_CQE_SKIP) { features << " " << "IORING_FEAT_CQE_SKIP"; }
#endif
#ifdef IORING_FEAT_LINKED_FILE
	if (_params.features & IORING_FEAT_LINKED_FILE) { features << " " << "IORING_FEAT_LINKED_FILE"; }
#endif
#ifdef IORING_FEAT_REG_REG_RING
	if (_params.features & IORING_FEAT_REG_REG_RING) { features << " " << "IORING_FEAT_REG_REG_RING"; }
#endif
#ifdef IORING_FEAT_RECVSEND_BUNDLE
	if (_params.features & IORING_FEAT_RECVSEND_BUNDLE) { features << " " << "IORING_FEAT_RECVSEND_BUNDLE"; }
#endif

	log::info("event::URingData", "io_uring features: ", features.str());

	sq.ringSize = _params.sq_off.array + _params.sq_entries * sizeof(unsigned);
	cq.ringSize = _params.cq_off.cqes + _params.cq_entries * CQESize;

	if (_params.features & IORING_FEAT_SINGLE_MMAP) {
		cq.ringSize = sq.ringSize = std::max(sq.ringSize, cq.ringSize);
	}

	sq.ring = (uint8_t *)::mmap(0, sq.ringSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ringFd, IORING_OFF_SQ_RING);
	if (sq.ring == MAP_FAILED) {
		log::error("event::URingData", "Fail to mmap SQ");
		cleanup();
		return;
	}

	if (_params.features & IORING_FEAT_SINGLE_MMAP) {
		cq.ring = sq.ring;
	} else {
		/* Map in the completion queue ring buffer in older kernels separately */
		cq.ring = (uint8_t *)::mmap(0, cq.ringSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ringFd, IORING_OFF_CQ_RING);
		if (cq.ring == MAP_FAILED) {
			log::error("event::URingData", "Fail to mmap CQ");
			cleanup();
			return;
		}
	}

	/* Map in the submission queue entries array */
	sq.sqes = (io_uring_sqe *)::mmap(0, _params.sq_entries * sizeof(io_uring_sqe), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
			ringFd, IORING_OFF_SQES);
	if (sq.sqes == MAP_FAILED) {
		log::error("event::URingData", "Fail to mmap SQE");
		cleanup();
		return;
	}

	// Register probe
	memset(&_probe, 0, sizeof(URingProbe));
	auto err = io_uring_register(ringFd, IORING_REGISTER_PROBE, &_probe, URingProbe::OpcodeCount);
	if (err < 0) {
		log::error("event::URingData", "Fail to register probe: ", err);
		cleanup();
		return;
	}

	sq.head = reinterpret_cast<unsigned *>(sq.ring + _params.sq_off.head);
	sq.tail = reinterpret_cast<unsigned *>(sq.ring + _params.sq_off.tail);
	sq.mask = reinterpret_cast<unsigned *>(sq.ring + _params.sq_off.ring_mask);
	sq.entries = reinterpret_cast<unsigned *>(sq.ring + _params.sq_off.ring_entries);
	sq.flags = reinterpret_cast<unsigned *>(sq.ring + _params.sq_off.flags);
	sq.dropped = reinterpret_cast<unsigned *>(sq.ring + _params.sq_off.dropped);
	sq.array = reinterpret_cast<unsigned *>(sq.ring + _params.sq_off.array);

	cq.head = reinterpret_cast<unsigned *>(cq.ring + _params.cq_off.head);
	cq.tail = reinterpret_cast<unsigned *>(cq.ring + _params.cq_off.tail);
	cq.mask = reinterpret_cast<unsigned *>(cq.ring + _params.cq_off.ring_mask);
	cq.entries = reinterpret_cast<unsigned *>(cq.ring + _params.cq_off.ring_entries);
	cq.overflow = reinterpret_cast<unsigned *>(cq.ring + _params.cq_off.overflow);
	cq.cqes = reinterpret_cast<io_uring_cqe *>(cq.ring + _params.cq_off.cqes);
	if (_params.cq_off.flags) {
		cq.flags = reinterpret_cast<unsigned *>(cq.ring + _params.cq_off.flags);
	}

	_out.resize(_params.cq_entries);

	for (unsigned index = 0; index < *sq.entries; index++) {
		sq.array[index] = index;
	}

	_ringFd = ringFd;
	_signalFd = sigFd;
	_eventFd = evFd;

	if (_signalFd && hasFlag(_flags, QueueFlags::Protected)) {
		// enable with current mask
		_signalFd->enable();

		pushRead(_signalFd->getFd(), (uint8_t *)_signalFd->getInfo(), sizeof(signalfd_siginfo), uint64_t(_signalFd));
	}

	if (_eventFd) {
		pushRead(_eventFd->getFd(), (uint8_t *)_eventFd->getValue(), sizeof(uint64_t), uint64_t(_eventFd));
	}

	_shouldWakeup.test_and_set();
}

URingData::~URingData() {
	if (_ringFd >= 0) {
		::close(_ringFd);
		_ringFd = -1;
	}
}

bool TimerFdURingHandle::init(URingData *uring, TimerInfo &&info) {
	if (!TimerFdHandle::init(uring->_queue, uring->_data, move(info))) {
		return false;
	}

	_runFn = [] (Handle *h) -> Status {
		return ((TimerFdURingHandle *)h)->rearm();
	};

	_suspendFn = [] (Handle *h) -> Status {
		return ((TimerFdURingHandle *)h)->disarm(true);
	};

	_resumeFn = [] (Handle *h) -> Status {
		return ((TimerFdURingHandle *)h)->rearm();
	};

	auto source = (TimerFdSource *)getData();
	source->setURingCallback(uring, [] (Handle *h, int32_t res, uint32_t flags) {
		reinterpret_cast<TimerFdURingHandle *>(h)->notify(res, flags);
	});

	return true;
}

Status TimerFdURingHandle::rearm() {
	auto source = (TimerFdSource *)getData();

	if (_status == Status::Ok) {
		return Status::ErrorAlreadyPerformed;
	}

	if (_status != Status::Declined && _status != Status::Suspended) {
		return Status::ErrorNotPermitted;
	}

	_status = Status::Ok;
	_value = 0;

	auto status = source->getURingData()->pushReadRetain(source->getFd(), (uint8_t *)&_value, sizeof(uint64_t), this);
	if (status == Status::Suspended) {
		status = source->getURingData()->submit();
	}
	return status;
}

Status TimerFdURingHandle::disarm(bool suspend) {
	auto source = (TimerFdSource *)getData();

	if (_status != Status::Ok) {
		return Status::ErrorAlreadyPerformed;
	}

	if (suspend) {
		_status = Status::Suspended;
	}

	auto status = source->getURingData()->cancelOp(this, URingUserFlags::Retained);
	if (status == Status::Suspended) {
		status = source->getURingData()->submit();
	}
	return status;
}

void TimerFdURingHandle::notify(int32_t res, uint32_t flags) {
	auto source = (TimerFdSource *)getData();

	if (_status != Status::Ok) {
		return;
	}

	_status = Status::Suspended;

	if (res < 0) {
		cancel(URingData::getErrnoStatus(res));
	}

	auto count = source->getCount();
	auto current = source->getCurrent();

	if (res == sizeof(uint64_t)) {
		// successful read
		current += _value;
		if (count != maxOf<uint32_t>()) {
			source->setCurrent(current);
		}
	}

	if (current >= count && _status == Status::Suspended) {
		cancel(Status::Done);
	}

	sendCompletion(uint32_t(std::min(current, count)), _status == Status::Suspended ? Status::Ok : _status);

	if (_status == Status::Suspended && (count == maxOf<uint32_t>() || current < count)) {
		rearm();
	}
}

Status TimerFdURingHandle::cancel(Status st) {
	if (!isValidCancelStatus(st)) {
		return Status::ErrorInvalidArguemnt;
	}

	if (_status == Status::Ok) {
		return disarm(false);
	} else if (_status == Status::Suspended || _status == Status::Declined) {
		return TimerFdHandle::cancel(st);
	} else {
		return Status::ErrorAlreadyPerformed;
	}
}

}

#endif
