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

#include "SPEventTimer-uring.h"

namespace STAPPLER_VERSIONIZED stappler::event {

bool TimerUringSource::init(const TimerInfo &info) {
	if (!FdSource::init(-1)) {
		return false;
	}

	setTimeoutInterval(info.timeout, info.interval);

	return true;
}

bool TimerURingHandle::init(URingData *uring, TimerInfo &&info) {
	static_assert(sizeof(TimerUringSource) <= DataSize && std::is_standard_layout<TimerUringSource>::value);

	auto source = new (_data) TimerUringSource;

	if (info.count == 1) {
		info.interval = info.timeout;
	}

	if (!source->init(info) || !TimerHandle::init(uring->_queue, uring->_data, move(info))) {
		return false;
	}

	setup<TimerURingHandle, TimerUringSource>();

	source->setURingCallback(uring, [] (Handle *h, int32_t res, uint32_t flags, URingUserFlags uflags) {
		reinterpret_cast<TimerURingHandle *>(h)->notify(h->getData<TimerUringSource>(), res, flags, uflags);
	});

	return true;
}

static bool timespecIsEqual(const __kernel_timespec &l, const __kernel_timespec &r) {
	return l.tv_nsec == r.tv_nsec && l.tv_sec == r.tv_sec;
}

Status TimerURingHandle::rearm(TimerUringSource *source) {
	auto status = prepareRearm();
	if (status == Status::Ok) {
		if (getValue() == 0 && !timespecIsEqual(source->getTimeout(), source->getInterval())) {
			auto uring = source->getURingData();
			status = uring->pushSqe({IORING_OP_TIMEOUT}, [&] (io_uring_sqe *sqe, uint32_t n) {
				sqe->len = 1;
				sqe->addr = reinterpret_cast<uintptr_t>(&source->getTimeout());
				sqe->off = 0;
				sqe->timeout_flags = IORING_TIMEOUT_ETIME_SUCCESS;
				uring->retainHandleForSqe(sqe, this);
			}, URingPushFlags::Submit);
		} else {
			auto uring = source->getURingData();
			status = uring->pushSqe({IORING_OP_TIMEOUT}, [&] (io_uring_sqe *sqe, uint32_t n) {
				sqe->fd = -1;
				sqe->len = 1;
				sqe->addr = reinterpret_cast<uintptr_t>(&source->getInterval());
				sqe->off = (_count == TimerInfo::Infinite ? 0 : _count - getValue());
				sqe->timeout_flags = IORING_TIMEOUT_MULTISHOT | IORING_TIMEOUT_ETIME_SUCCESS;
				uring->retainHandleForSqe(sqe, this);
			}, URingPushFlags::Submit);
		}
	}
	return status;
}

Status TimerURingHandle::disarm(TimerUringSource *source, bool suspend) {
	auto status = prepareDisarm(suspend);
	if (status == Status::Ok) {
		auto uring = source->getURingData();
		status = uring->pushSqe({IORING_OP_TIMEOUT_REMOVE}, [&] (io_uring_sqe *sqe, uint32_t n) {
			sqe->len = 0;
			sqe->addr = reinterpret_cast<uintptr_t>(this);
			sqe->off = 0;
			sqe->user_data = URING_USERDATA_IGNORED;
		}, URingPushFlags::Submit);
	}
	return status;
}

void TimerURingHandle::notify(TimerUringSource *source, int32_t res, uint32_t flags, URingUserFlags uflags) {
	if (_status != Status::Ok) {
		return;
	}

	_status = Status::Suspended;

	bool more = (flags & IORING_CQE_F_MORE);

	if (res != -ETIME && res < 0) {
		cancel(URingData::getErrnoStatus(res));
	}

	auto count = getCount();
	auto current = getValue();

	bool isFirst = (current == 0);

	++ current;
	setValue(current);

	if ((isFirst && !timespecIsEqual(source->getTimeout(), source->getInterval()))
			|| (!more)) {
		if (count == TimerInfo::Infinite || current < count) {
			// rearm if spatial
			rearm(source);
		} else {
			cancel(Status::Done);
		}
	}

	sendCompletion(current, _status == Status::Suspended ? Status::Ok : _status);

	if (more) {
		_status = Status::Ok;
	}
}

}
