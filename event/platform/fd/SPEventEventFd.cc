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

#include "SPEventEventFd.h"
#include "../uring/SPEvent-uring.h"

#include <sys/eventfd.h>

namespace STAPPLER_VERSIONIZED stappler::event {

bool EventFdSource::init() {
	auto fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (fd < 0) {
		return false;
	}

	return FdSource::init(fd);
}

bool EventFdHandle::init(QueueRef *q, QueueData *d, mem_std::Function<Status()> &&cb) {
	static_assert(sizeof(EventFdSource) <= DataSize && std::is_standard_layout<EventFdSource>::value);

	auto source = new (_data) EventFdSource;

	if (!source->init()) {
		return false;
	}

	_callback = sp::move(cb);

	return Handle::init(q, d);
}

bool EventFdHandle::read() {
	return ::eventfd_read(getFd(), &_target) >= 0;
}

bool EventFdHandle::write(uint64_t val) {
	return ::eventfd_write(getFd(), val) >= 0;
}

bool EventFdURingHandle::init(URingData *uring, mem_std::Function<Status()> &&cb) {
	if (!EventFdHandle::init(uring->_queue, uring->_data, sp::move(cb))) {
		return false;
	}

	setupURing<EventFdURingHandle, EventFdSource>(uring, this);

	return true;
}

Status EventFdURingHandle::rearm(EventFdSource *source) {
	auto status = prepareRearm();
	if (status == Status::Ok) {
		_target = 0;

		status = source->getURingData()->pushRead(source->getFd(), (uint8_t *)&_target, sizeof(uint64_t), this);
		if (status == Status::Suspended) {
			status = source->getURingData()->submit();
		}
	}
	return status;
}

Status EventFdURingHandle::disarm(EventFdSource *source, bool suspend) {
	auto status = prepareDisarm(suspend);
	if (status == Status::Ok) {
		status = source->getURingData()->cancelOp(this, URingUserFlags::None,
				suspend ? URingCancelFlags::Suspend : URingCancelFlags::None);
	} else if (status == Status::ErrorAlreadyPerformed) {
		return Status::Ok;
	}
	return status;
}

void EventFdURingHandle::notify(EventFdSource *source, int32_t res, uint32_t flags, URingUserFlags uflags) {
	if (_status != Status::Ok) {
		return;
	}

	_status = Status::Suspended;

	if (res == sizeof(uint64_t)) {
		if (_callback() == Status::Ok) {
			rearm(source);
		}
	} else{
		cancel(URingData::getErrnoStatus(res));
	}
}

}
