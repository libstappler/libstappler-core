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

#include "SPEventSource.h"
#include "SPEventHandle.h"

#if LINUX

#include "../epoll/SPEvent-epoll.h"

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
};

SP_DEFINE_ENUM_AS_MASK(URingFlags)

struct URingData : public mem_pool::AllocBase {
	static constexpr size_t CQESize = sizeof(struct io_uring_cqe);
	static constexpr uint32_t DefaultIdleInterval = 500;

	static bool checkSupport();

	QueueFlags _flags = QueueFlags::None;
	URingFlags _uflags = URingFlags::None;
	int _ringFd = -1;
	SignalFdSource *_signalFd = nullptr;

	io_uring_params _params;
	URingSq sq;
	URingCq cq;
	URingProbe _probe;

	mem_pool::Vector<io_uring_cqe> _out;

	uint64_t _start = 0;
	uint64_t _now = 0;
	uint32_t _events = 0;
	std::atomic_flag _shouldWakeup;

	unsigned getUnprocessedSqeCount();

	unsigned flushSqe();

	io_uring_sqe *tryGetNextSqe();
	io_uring_sqe *getNextSqe();
	int submitSqe(unsigned sub, unsigned wait, bool waitAvailable);
	int submitPending();

	Status pushRead(int fd, uint8_t *buf, size_t bsize, uint64_t userdata);
	Status pushWrite(int fd, const uint8_t *buf, size_t bsize, uint64_t userdata);

	//bool push(uint8_t opcode, int fd, uint64_t userdata);
	uint32_t pop();
	void processEvent(int32_t res, uint32_t flags, uint64_t userdata);

	Status submit();
	uint32_t poll();
	uint32_t wait(TimeInterval);
	Status run(TimeInterval);

	int enter(unsigned sub, unsigned wait, unsigned flags, __kernel_timespec *);

	URingData(SignalFdSource *, const QueueInfo &info, QueueFlags);
	~URingData();

};

}

#endif

#endif /* CORE_EVENT_PLATFORM_SPEVENT_URING_H_ */
