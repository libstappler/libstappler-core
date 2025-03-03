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

#include "SPEventTimerFd.h"

#include "../uring/SPEvent-uring.h"

#include <sys/timerfd.h>

namespace STAPPLER_VERSIONIZED stappler::event {

bool TimerFdSource::init(const TimerInfo &info) {
	__clockid_t clockid = CLOCK_MONOTONIC;

	switch (info.type) {
	case ClockType::Default:
	case ClockType::Monotonic:
		clockid = CLOCK_MONOTONIC;
		break;
	case ClockType::Realtime:
		clockid = CLOCK_REALTIME;
		break;
	case ClockType::Process:
		log::error("event::Queue", "ClockType::Thread is not supported for a timer on this system");
		return false;
		break;
	case ClockType::Thread:
		log::error("event::Queue", "ClockType::Thread is not supported for a timer on this system");
		return false;
		break;
	}

	auto fd = ::timerfd_create(clockid, TFD_NONBLOCK | TFD_CLOEXEC);
	if (fd < 0) {
		log::error("event::Queue", "fail to timerfd_create");
		return false;
	}

	itimerspec spec;
	if (info.timeout) {
		setNanoTimespec(spec.it_value, info.timeout);
	} else {
		setNanoTimespec(spec.it_value, info.interval);
	}

	setNanoTimespec(spec.it_interval, info.interval);

	::timerfd_settime(fd, 0, &spec, nullptr);

	return FdSource::init(fd);
}

TimerFdHandle::~TimerFdHandle() {
	getData<TimerFdSource>()->cancel();
}

bool TimerFdHandle::init(QueueRef *q, QueueData *d, TimerInfo &&info) {
	static_assert(sizeof(TimerFdSource) <= DataSize && std::is_standard_layout<TimerFdSource>::value);

	auto source = new (_data) TimerFdSource;

	if (!source->init(info) || !TimerHandle::init(q, d, move(info))) {
		return false;
	}

	_count = info.count;

	return true;
}

bool TimerFdURingHandle::init(URingData *uring, TimerInfo &&info) {
	if (!TimerFdHandle::init(uring->_queue, uring->_data, move(info))) {
		return false;
	}

	setup<TimerFdURingHandle, TimerFdSource>();

	auto source = getData<TimerFdSource>();
	source->setURingCallback(uring, [] (Handle *h, int32_t res, uint32_t flags) {
		reinterpret_cast<TimerFdURingHandle *>(h)->notify(h->getData<TimerFdSource>(), res, flags);
	});

	return true;
}

Status TimerFdURingHandle::rearm(TimerFdSource *source) {
	auto status = prepareRearm();
	if (status == Status::Ok) {
		_target = 0;

		status = source->getURingData()->pushReadRetain(source->getFd(), (uint8_t *)&_target, sizeof(uint64_t), this);
		if (status == Status::Suspended) {
			status = source->getURingData()->submit();
		}
	}
	return status;
}

Status TimerFdURingHandle::disarm(TimerFdSource *source, bool suspend) {
	auto status = prepareDisarm(suspend);
	if (status == Status::Ok) {
		status = source->getURingData()->cancelOpRelease(this, suspend ? URingCancelFlags::Suspend : URingCancelFlags::None);
	}
	return status;
}

void TimerFdURingHandle::notify(TimerFdSource *source, int32_t res, uint32_t flags) {
	if (_status != Status::Ok) {
		return;
	}

	_status = Status::Suspended;

	if (res < 0) {
		cancel(URingData::getErrnoStatus(res));
	}

	auto count = getCount();
	auto current = getValue();

	if (res == sizeof(uint64_t)) {
		// successful read
		current += _target;
		if (count != maxOf<uint32_t>()) {
			setValue(std::min(current, count));
		}
	}

	if (current >= count && _status == Status::Suspended) {
		cancel(Status::Done);
	}

	sendCompletion(getValue(), _status == Status::Suspended ? Status::Ok : _status);

	if (_status == Status::Suspended && (count == maxOf<uint32_t>() || current < count)) {
		rearm(source);
	}
}

}
