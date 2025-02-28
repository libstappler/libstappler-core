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

#ifndef CORE_EVENT_PLATFORM_SPEVENT_URING_H_
#define CORE_EVENT_PLATFORM_SPEVENT_URING_H_

#include "SPEventHandle.h"

#if LINUX

#include "detail/SPEventQueueData.h"

#include "../fd/SPEvent-fd.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <linux/io_uring.h>
#include <linux/time_types.h>

namespace STAPPLER_VERSIONIZED stappler::event {

struct URingSq {
	unsigned *head = nullptr;
	unsigned *tail = nullptr;
	unsigned *mask = nullptr;
	unsigned *entries = nullptr;
	unsigned *flags = nullptr;
	unsigned *dropped = nullptr;
	unsigned *array = nullptr;
	io_uring_sqe *sqes = nullptr;

	unsigned userspaceHead = 0;
	unsigned userspaceTail = 0;

	size_t ringSize = 0;
	uint8_t *ring = nullptr;
};

struct URingCq {
	unsigned *head = nullptr;
	unsigned *tail = nullptr;
	unsigned *mask = nullptr;
	unsigned *entries = nullptr;
	unsigned *flags = nullptr;
	unsigned *overflow = nullptr;
	io_uring_cqe *cqes = nullptr;

	size_t ringSize = 0;
	uint8_t *ring = nullptr;
};

struct URingProbe {
	static constexpr size_t OpcodeCount = 256;

	uint8_t last_op;	/* last opcode supported */
	uint8_t ops_len;	/* length of ops[] array below */
	uint16_t resv;
	uint32_t resv2[3];
	io_uring_probe_op ops[OpcodeCount];

	bool isOpcodeSupported(int op) {
		if (op > last_op) {
			return false;
		}
		return (ops[op].flags & IO_URING_OP_SUPPORTED) != 0;
	}
};

enum class URingFlags {
	None,
	PendingGetEvents = 1 << 0,
	SubmitAllSupported = 1 << 1,
	CoopTaskrunSupported = 1 << 2,
	SingleIssuerSupported = 1 << 3,
	DeferTaskrunSupported = 1 << 4,
	AsyncCancelFdSupported = 1 << 5,
	AsyncCancelAnyAllSupported = AsyncCancelFdSupported,
	AsyncCancelFdFixedSupported = 1 << 6,
};

SP_DEFINE_ENUM_AS_MASK(URingFlags)

enum class URingUserFlags {
	None,
	Retained = 1 << 0,
	CancellationRequest = 1 << 1,
};

SP_DEFINE_ENUM_AS_MASK(URingUserFlags)

enum class URingCancelFlags {
	None,
	All = 1 << 0,
	Any = 1 << 1,
	Drain = 1 << 2,
	FixedFile = 1 << 3,
};

SP_DEFINE_ENUM_AS_MASK(URingCancelFlags)

struct URingData : public mem_pool::AllocBase {
	static constexpr size_t CQESize = sizeof(struct io_uring_cqe);
	static constexpr uint32_t DefaultIdleInterval = 500;

	static constexpr uint64_t UserdataPointerMask = 0xFFFF'FFFF'FFFF'FFF0; // to use last 4 bits for a flags
	static constexpr uint64_t UserdataFlagsMask = ~UserdataPointerMask; // to use last 4 bits for a flags

	static inline Status getErrnoStatus(int negErrno) {
		return Status(-STATUS_ERRNO_OFFSET + negErrno);
	}

	static bool checkSupport();

	QueueRef *_queue = nullptr;
	Queue::Data *_data = nullptr;
	QueueFlags _flags = QueueFlags::None;
	URingFlags _uflags = URingFlags::None;
	int _ringFd = -1;
	SignalFdSource *_signalFd = nullptr;
	EventFdSource *_eventFd = nullptr;

	io_uring_params _params;
	URingSq sq;
	URingCq cq;
	URingProbe _probe;

	mem_pool::Vector<io_uring_cqe> _out;

	uint64_t _start = 0;
	uint64_t _now = 0;
	uint32_t _events = 0;
	std::atomic_flag _shouldWakeup;
	std::atomic<std::underlying_type_t<QueueWakeupFlags>> _wakeupFlags = 0;
	std::atomic<uint64_t> _wakeupTimeout;

	__kernel_timespec _wakeupTimespec;

	unsigned getUnprocessedSqeCount();

	unsigned flushSqe();

	struct SqeBlock {
		io_uring_sqe *front;
		unsigned first;
		uint32_t count;
		Status status;

		explicit operator bool() const { return front != nullptr; }
	};

	SqeBlock tryGetNextSqe(uint32_t count = 1);
	SqeBlock getNextSqe(uint32_t count = 1);

	Status pushSqe(std::initializer_list<uint8_t> ops, const Callback<void(io_uring_sqe *, uint32_t n)> &);

	// owner should keep ts buffer available until operation is consumed
	// In case of SQPOLL - it's undefined, when data will be consumed, so, keep until the end
	Status pushSqe(uint8_t op, const Callback<void(io_uring_sqe *)> &, const __kernel_timespec *ts);

	int submitSqe(unsigned sub, unsigned wait, bool waitAvailable);
	int submitPending();

	Status pushRead(int fd, uint8_t *buf, size_t bsize, uint64_t userdata);
	Status pushReadRetain(int fd, uint8_t *buf, size_t bsize, Handle *);

	Status pushWrite(int fd, const uint8_t *buf, size_t bsize, uint64_t userdata);
	Status pushWriteRetain(int fd, const uint8_t *buf, size_t bsize, Handle *);

	Status cancelOp(void *, URingUserFlags ptrFlags, URingCancelFlags = URingCancelFlags::None);
	Status cancelFd(int fd, URingCancelFlags = URingCancelFlags::None);

	uint32_t pop();
	void processEvent(int32_t res, uint32_t flags, uint64_t userdata);

	Status submit();
	uint32_t poll();
	uint32_t wait(TimeInterval);
	Status run(TimeInterval);

	int enter(unsigned sub, unsigned wait, unsigned flags, __kernel_timespec *);

	void suspendSystemHandles();
	void resumeSystemHandles();

	URingData(QueueRef *, Queue::Data *data, SignalFdSource *, EventFdSource *, const QueueInfo &info, QueueFlags);
	~URingData();
};

class TimerFdURingHandle : public TimerFdHandle {
public:
	virtual ~TimerFdURingHandle() = default;

	bool init(URingData *, TimerInfo &&);

	Status rearm();
	Status disarm(bool suspend);
	void notify(int32_t res, uint32_t flags);

	virtual Status cancel(Status) override;
};

}

#endif

#endif /* CORE_EVENT_PLATFORM_SPEVENT_URING_H_ */
