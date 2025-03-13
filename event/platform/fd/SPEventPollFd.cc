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

#include "SPEventPollFd.h"
#include "../linux/SPEvent-linux.h"

namespace STAPPLER_VERSIONIZED stappler::event {

Rc<PollFdHandle> PollFdHandle::create(const Queue *q, int fd, PollFlags flags, mem_std::Function<Status(int, PollFlags)> &&cb) {
	auto d = q->getData();
	if (d->_uring) {
		return Rc<PollFdURingHandle>::create(d->_uring, fd, flags, sp::move(cb));
	}
	return nullptr;
}

bool PollFdHandle::init(QueueRef *q, QueueData *d, int fd, PollFlags flags, mem_std::Function<Status(int, PollFlags)> &&cb) {
	static_assert(sizeof(FdSource) <= DataSize && std::is_standard_layout<FdSource>::value);

	auto source = new (_data) FdSource;

	if (!source->init(fd) || !FdHandle::init(q, d)) {
		return false;
	}

	source->setCloseFd((flags & PollFlags::CloseFd) != PollFlags::None);

	_flags = (flags & PollFlags::PollMask);
	_notify = sp::move(cb);

	return true;
}


bool PollFdURingHandle::init(URingData *uring, int fd, PollFlags flags, mem_std::Function<Status(int, PollFlags)> &&cb) {
	if (!PollFdHandle::init(uring->_queue, uring->_data, fd, flags, sp::move(cb))) {
		return false;
	}

	setupURing<PollFdURingHandle, FdSource>(uring, this);

	return true;
}

Status PollFdURingHandle::rearm(FdSource *source) {
	auto status = prepareRearm();
	if (status == Status::Ok) {
		auto uring = source->getURingData();
		uring->pushSqe({IORING_OP_POLL_ADD}, [&] (io_uring_sqe *sqe, uint32_t n) {
			sqe->fd = source->getFd();
			sqe->poll_events = toInt(_flags & PollFlags::PollMask);
			uring->retainHandleForSqe(sqe, this);
		}, URingPushFlags::Submit);
	}
	return status;
}

Status PollFdURingHandle::disarm(FdSource *source, bool suspend) {
	auto status = prepareDisarm(suspend);
	if (status == Status::Ok) {
		source->getURingData()->pushSqe({IORING_OP_POLL_REMOVE}, [&] (io_uring_sqe *sqe, uint32_t n) {
			sqe->fd = source->getFd();
		}, URingPushFlags::Submit);
	}
	return status;
}

void PollFdURingHandle::notify(FdSource *source, int32_t res, uint32_t flags, URingUserFlags uflags) {
	if (_status != Status::Ok) {
		return; // just exit
	}

	_status = Status::Suspended;

	if (res < 0 && res != -EAGAIN) {
		cancel(URingData::getErrnoStatus(res));
		return;
	}

	auto status = _notify(source->getFd(), PollFlags(res));
	if (status == Status::Ok) {
		rearm(source);
	} else {
		cancel(status);
	}
}

}
