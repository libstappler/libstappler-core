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

#include "detail/SPEventQueueData.h"

#include "../fd/SPEvent-fd.h"
#include "../epoll/SPEvent-epoll.h"
#include "../uring/SPEvent-uring.h"

#include <signal.h>

namespace STAPPLER_VERSIONIZED stappler::event {

struct Queue::Data : public QueueData {
	URingData *_uring = nullptr;

	SignalFdSource _signalFd;
	EventFdSource _eventFd;

	Rc<TimerHandle> scheduleTimer(TimerInfo &&);

	Status submit();
	uint32_t poll();
	uint32_t wait(TimeInterval);
	Status run(TimeInterval);
	void wakeup(QueueWakeupFlags, TimeInterval);

	bool isValid() const;

	~Data();
	Data(QueueRef *q, const QueueInfo &info, QueueFlags flags);
};

static int SignalsToIntercept[] = { SIGUSR1, SIGUSR2 };

Rc<TimerHandle> Queue::Data::scheduleTimer(TimerInfo &&info) {
	if (_uring) {
		return Rc<TimerFdURingHandle>::create(_uring, move(info));
	}
	return nullptr;
}

Status Queue::Data::submit() {
	resumeAll();
	if (_uring) {
		return _uring->submit();
	}
	return Status::ErrorNotImplemented;
}

uint32_t Queue::Data::poll() {
	resumeAll();
	if (_uring) {
		return _uring->poll();
	}
	return 0;
}

uint32_t Queue::Data::wait(TimeInterval ival) {
	resumeAll();
	if (_uring) {
		return _uring->wait(ival);
	}
	return 0;
}

Status Queue::Data::run(TimeInterval ival) {
	resumeAll();
	if (_uring) {
		return _uring->run(ival);
	}
	return Status::ErrorNotImplemented;
}

void Queue::Data::wakeup(QueueWakeupFlags flags, TimeInterval gracefulTimeout) {
	if (_uring) {
		_uring->_wakeupFlags |= toInt(flags);
		_uring->_wakeupTimeout = gracefulTimeout.toMicros();
	}
	_eventFd.write(1);
}

bool Queue::Data::isValid() const {
	return _uring != nullptr;
}

Queue::Data::~Data() {
	if (_uring) {
		delete _uring;
		_uring = nullptr;
	}
}

Queue::Data::Data(QueueRef *q, const QueueInfo &info, QueueFlags flags) : QueueData(q, flags) {
	if (!_eventFd.init()) {
		log::error("event::Queue", "Fail to initialize eventfd");
		return;
	}

	if (hasFlag(_flags, QueueFlags::Protected)) {
		if (!_signalFd.init(SignalsToIntercept)) {
			log::error("event::Queue", "Fail to initialize signalfd");
			return;
		}
	}

	if (URingData::checkSupport()) {
		_uring = new (memory::pool::acquire()) URingData(_queue, this, &_signalFd, &_eventFd, info, flags);
	}
}

}

#endif
