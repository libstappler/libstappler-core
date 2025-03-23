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

#include "SPEvent-linux.h"

#if LINUX

#include "../fd/SPEventFd.h"
#include "../fd/SPEventTimerFd.h"
#include "../fd/SPEventDirFd.h"
#include "../fd/SPEventPollFd.h"
#include "../epoll/SPEvent-epoll.h"
#include "../epoll/SPEventThreadHandle-epoll.h"
#include "../uring/SPEventThreadHandle-uring.h"
#include "../uring/SPEventTimer-uring.h"

#include <signal.h>

namespace STAPPLER_VERSIONIZED stappler::event {

static int SignalsToIntercept[] = { SIGUSR1, SIGUSR2 };

/*Rc<DirHandle> Queue::Data::openDir(OpenDirInfo &&info) {
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
}*/

Rc<TimerHandle> Queue::Data::scheduleTimer(TimerInfo &&info) {
	if (_uring) {
		if ((hasFlag(_uring->_uflags, URingFlags::TimerMultishotSupported) && info.count == TimerInfo::Infinite)
				|| info.count == 1) {
			return Rc<TimerURingHandle>::create(&_uringTimerClass, move(info));
		} else {
			return Rc<TimerFdURingHandle>::create(&_uringTimerFdClass, move(info));
		}
	} else if (_epoll) {
		return Rc<TimerFdEPollHandle>::create(&_epollTimerFdClass, move(info));
	}
	return nullptr;
}

Rc<ThreadHandle> Queue::Data::addThreadHandle() {
	if (_uring) {
		if constexpr (URING_THREAD_USE_FUTEX_HANDLE) {
			if (hasFlag(_uring->_uflags, URingFlags::FutexSupported)) {
				return Rc<ThreadUringHandle>::create(&_uringThreadFenceClass);
			} else {
				return Rc<ThreadEventFdHandle>::create(&_uringThreadEventFdClass);
			}
		} else {
			return Rc<ThreadEventFdHandle>::create(&_uringThreadEventFdClass);
		}
	} else if (_epoll) {
		return Rc<ThreadEPollHandle>::create(&_epollThreadClass);
	}
	return nullptr;
}

Status Queue::Data::submit() {
	if (_uring) {
		return _uring->submit();
	} else if (_epoll) {
		return _epoll->submit();
	}
	return Status::ErrorNotImplemented;
}

uint32_t Queue::Data::poll() {
	if (_uring) {
		return _uring->poll();
	} else if (_epoll) {
		return _epoll->poll();
	}
	return 0;
}

uint32_t Queue::Data::wait(TimeInterval ival) {
	if (_uring) {
		return _uring->wait(ival);
	} else if (_epoll) {
		return _epoll->wait(ival);
	}
	return 0;
}

Status Queue::Data::run(TimeInterval ival, QueueWakeupInfo &&info) {
	if (_uring) {
		return _uring->run(ival, info.flags, info.timeout);
	} else if (_epoll) {
		return _epoll->run(ival, info.flags, info.timeout);
	}
	return Status::ErrorNotImplemented;
}

Status Queue::Data::wakeup(QueueWakeupInfo &&info) {
	if (_uring) {
		return _uring->wakeup(info.flags, info.timeout);
	} else if (_epoll) {
		return _epoll->wakeup(info.flags, info.timeout);
	}
	return Status::ErrorNotImplemented;
}

bool Queue::Data::isValid() const {
	return _uring != nullptr || _epoll != nullptr;
}

void Queue::Data::cancel() {
	if (_uring) {
		_uring->cancel();
	} else if (_epoll) {
		_epoll->cancel();
	}
	cleanup();
}

Queue::Data::~Data() {
	if (_uring) {
		delete _uring;
		_uring = nullptr;
	}
	if (_epoll) {
		delete _epoll;
		_epoll = nullptr;
	}
}

Queue::Data::Data(QueueRef *q, const QueueInfo &info) : QueueData(q, info.flags) {
	if (hasFlag(info.engineMask, QueueEngine::URing)) {
		setupUringHandleClass<TimerFdURingHandle, TimerFdSource>(&_info, &_uringTimerFdClass, true);
		setupUringHandleClass<TimerURingHandle, TimerUringSource>(&_info, &_uringTimerClass, true);
		setupUringHandleClass<ThreadEventFdHandle, EventFdSource>(&_info, &_uringThreadEventFdClass, true);
		setupUringHandleClass<ThreadUringHandle, ThreadUringSource>(&_info, &_uringThreadFenceClass, true);

		setupUringHandleClass<EventFdURingHandle, EventFdSource>(&_info, &_uringEventFdClass, true);
		setupUringHandleClass<SignalFdURingHandle, SignalFdSource>(&_info, &_uringSignalFdClass, true);

		setupUringHandleClass<PollFdURingHandle, PollFdSource>(&_info, &_uringPollFdClass, true);
	}

	if (hasFlag(info.engineMask, QueueEngine::EPoll)) {
		setupEpollHandleClass<TimerFdEPollHandle, TimerFdSource>(&_info, &_epollTimerFdClass, true);
		setupEpollHandleClass<ThreadEPollHandle, EventFdSource>(&_info, &_epollThreadClass, true);
		setupEpollHandleClass<EventFdEPollHandle, EventFdSource>(&_info, &_epollEventFdClass, true);
		setupEpollHandleClass<SignalFdEPollHandle, SignalFdSource>(&_info, &_epollSignalFdClass, true);
		setupEpollHandleClass<PollFdEPollHandle, PollFdSource>(&_info, &_epollPollFdClass, true);
	}

	if (hasFlag(info.engineMask, QueueEngine::URing) && URingData::checkSupport()) {
		auto uring = new (memory::pool::acquire()) URingData(_info.queue, this, info, SignalsToIntercept);
		if (uring->_ringFd >= 0) {
			_uring = uring;
			_uring->runInternalHandles();
			_engine = QueueEngine::URing;
		} else {
			uring->~URingData();
		}
	} else if (hasFlag(info.engineMask, QueueEngine::EPoll)) {
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
