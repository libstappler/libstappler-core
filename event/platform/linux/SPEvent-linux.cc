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

#include "../fd/SPEventFd.h"
#include "../fd/SPEventTimerFd.h"
#include "../fd/SPEventDirFd.h"
#include "../epoll/SPEvent-epoll.h"
#include "../uring/SPEvent-uring.h"
#include "../uring/SPEventThreadHandle-uring.h"

#include <signal.h>

namespace STAPPLER_VERSIONIZED stappler::event {

struct Queue::Data : public QueueData {
	URingData *_uring = nullptr;

	Rc<DirHandle> openDir(OpenDirInfo &&);
	Rc<StatHandle> stat(StatOpInfo &&);

	Rc<TimerHandle> scheduleTimer(TimerInfo &&);
	Rc<ThreadHandle> addThreadHandle();

	Status submit();
	uint32_t poll();
	uint32_t wait(TimeInterval);
	Status run(TimeInterval, QueueWakeupInfo &&);
	void wakeup(QueueWakeupInfo &&);

	bool isValid() const;

	~Data();
	Data(QueueRef *q, const QueueInfo &info, QueueFlags flags);
};

static int SignalsToIntercept[] = { SIGUSR1, SIGUSR2 };

Rc<DirHandle> Queue::Data::openDir(OpenDirInfo &&info) {
	if (_uring) {
		return Rc<DirFdURingHandle>::create(_uring, move(info));
	}
	return nullptr;
}

Rc<StatHandle> Queue::Data::stat(StatOpInfo &&info) {
	if (_uring) {
		return Rc<StatURingHandle>::create(_uring, move(info));
	}
	return nullptr;
}

Rc<TimerHandle> Queue::Data::scheduleTimer(TimerInfo &&info) {
	if (_uring) {
		return Rc<TimerFdURingHandle>::create(_uring, move(info));
	}
	return nullptr;
}

Rc<ThreadHandle> Queue::Data::addThreadHandle() {
	if (_uring && hasFlag(_uring->_uflags, URingFlags::FutexSupported)) {
		return Rc<ThreadUringHandle>::create(_uring);
	}
	return nullptr;
}

Status Queue::Data::submit() {
	if (_uring) {
		return _uring->submit();
	}
	return Status::ErrorNotImplemented;
}

uint32_t Queue::Data::poll() {
	if (_uring) {
		return _uring->poll();
	}
	return 0;
}

uint32_t Queue::Data::wait(TimeInterval ival) {
	if (_uring) {
		return _uring->wait(ival);
	}
	return 0;
}

Status Queue::Data::run(TimeInterval ival, QueueWakeupInfo &&info) {
	if (_uring) {
		return _uring->run(ival, info.flags, info.timeout);
	}
	return Status::ErrorNotImplemented;
}

void Queue::Data::wakeup(QueueWakeupInfo &&info) {
	if (_uring) {
		_uring->wakeup(info.flags, info.timeout);
	}
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
	if (URingData::checkSupport()) {
		_uring = new (memory::pool::acquire()) URingData(_queue, this, info, flags, SignalsToIntercept);
	}
}

}

#endif
