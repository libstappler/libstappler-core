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

#include "SPEvent-android.h"

#if ANDROID

#include "../fd/SPEventFd.h"
#include "../fd/SPEventTimerFd.h"
#include "../fd/SPEventDirFd.h"
#include "../fd/SPEventPollFd.h"
#include "../epoll/SPEvent-epoll.h"
#include "../epoll/SPEventThreadHandle-epoll.h"
#include "SPEventThreadHandle-alooper.h"

#include <signal.h>

namespace STAPPLER_VERSIONIZED stappler::event {

static int SignalsToIntercept[] = { SIGUSR1, SIGUSR2 };

Rc<TimerHandle> Queue::Data::scheduleTimer(TimerInfo &&info) {
	if (_epoll) {
		return Rc<TimerFdEPollHandle>::create(&_epollTimerFdClass, move(info));
	} else if (_alooper) {
		return Rc<TimerFdALooperHandle>::create(&_alooperTimerFdClass, move(info));
	}
	return nullptr;
}

Rc<ThreadHandle> Queue::Data::addThreadHandle() {
	if (_epoll) {
		return Rc<ThreadEPollHandle>::create(&_epollThreadClass);
	} else if (_alooper) {
		return Rc<ThreadALooperHandle>::create(&_alooperThreadClass);
	}
	return nullptr;
}

Status Queue::Data::submit() {
	if (_epoll) {
		return _epoll->submit();
	} else if (_alooper) {
		return _alooper->submit();
	}
	return Status::ErrorNotImplemented;
}

uint32_t Queue::Data::poll() {
	if (_epoll) {
		return _epoll->poll();
	} else if (_alooper) {
		return _alooper->poll();
	}
	return 0;
}

uint32_t Queue::Data::wait(TimeInterval ival) {
	if (_epoll) {
		return _epoll->wait(ival);
	} else if (_alooper) {
		return _alooper->wait(ival);
	}
	return 0;
}

Status Queue::Data::run(TimeInterval ival, QueueWakeupInfo &&info) {
	if (_epoll) {
		return _epoll->run(ival, info.flags, info.timeout);
	} else if (_alooper) {
		return _alooper->run(ival, info.flags, info.timeout);
	}
	return Status::ErrorNotImplemented;
}

Status Queue::Data::wakeup(QueueWakeupInfo &&info) {
	if (_epoll) {
		return _epoll->wakeup(info.flags, info.timeout);
	} else if (_alooper) {
		return _alooper->wakeup(info.flags, info.timeout);
	}
	return Status::ErrorNotImplemented;
}

bool Queue::Data::isValid() const {
	return _epoll != nullptr || _alooper != nullptr;
}

void Queue::Data::cancel() {
	if (_epoll) {
		_epoll->cancel();
	} else if (_alooper) {
		_alooper->cancel();
	}
	cleanup();
}

Queue::Data::~Data() {
	if (_epoll) {
		delete _epoll;
		_epoll = nullptr;
	}
	if (_alooper) {
		delete _alooper;
		_alooper = nullptr;
	}
}

Queue::Data::Data(QueueRef *q, const QueueInfo &info) : QueueData(q, info.flags) {
	if (hasFlag(info.engineMask, QueueEngine::EPoll)) {
		setupEpollHandleClass<TimerFdEPollHandle, TimerFdSource>(&_info, &_epollTimerFdClass, true);
		setupEpollHandleClass<ThreadEPollHandle, EventFdSource>(&_info, &_epollThreadClass, true);
		setupEpollHandleClass<EventFdEPollHandle, EventFdSource>(&_info, &_epollEventFdClass, true);
		setupEpollHandleClass<SignalFdEPollHandle, SignalFdSource>(&_info, &_epollSignalFdClass, true);
		setupEpollHandleClass<PollFdEPollHandle, PollFdSource>(&_info, &_epollPollFdClass, true);
	}

	if (hasFlag(info.engineMask, QueueEngine::ALooper)) {
		setupALooperHandleClass<TimerFdALooperHandle, TimerFdSource>(&_info, &_alooperTimerFdClass, true);
		setupALooperHandleClass<ThreadALooperHandle, EventFdSource>(&_info, &_alooperThreadClass, true);
		setupALooperHandleClass<EventFdALooperHandle, EventFdSource>(&_info, &_alooperEventFdClass, true);
		setupALooperHandleClass<SignalFdALooperHandle, SignalFdSource>(&_info, &_alooperSignalFdClass, true);
		setupALooperHandleClass<PollFdALooperHandle, PollFdSource>(&_info, &_alooperPollFdClass, true);
	}

	if (hasFlag(info.flags, QueueFlags::ThreadNative)
			&& hasFlag(info.engineMask, QueueEngine::ALooper)
			&& !hasFlag(info.flags, QueueFlags::Protected)) {
		auto alooper = new (memory::pool::acquire()) ALooperData(_info.queue, this, info, SignalsToIntercept);
		if (alooper->_looper != nullptr) {
			_alooper = alooper;
			_alooper->runInternalHandles();
			_engine = QueueEngine::ALooper;
			return;
		} else {
			alooper->~ALooperData();
		}
	}

	// try epoll if failed with ALooper
	if (!_alooper && hasFlag(info.engineMask, QueueEngine::EPoll)) {
		auto epoll = new (memory::pool::acquire()) EPollData(_info.queue, this, info, SignalsToIntercept);
		if (epoll->_epollFd >= 0) {
			_epoll = epoll;
			_epoll->runInternalHandles();
			_engine = QueueEngine::EPoll;
		} else {
			epoll->~EPollData();
		}
	}
}

}

namespace STAPPLER_VERSIONIZED stappler::event::platform {

Rc<QueueRef> getThreadQueue(QueueInfo &&info) {
	// Just create the queue, Linux has no specifics
	return Queue::create(move(info));
}

}

#endif
