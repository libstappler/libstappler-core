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
#include "../epoll/SPEvent-epoll.h"
#include "../uring/SPEventThreadHandle-uring.h"
#include "../uring/SPEventTimer-uring.h"

#include <signal.h>

namespace STAPPLER_VERSIONIZED stappler::event {

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
		if ((hasFlag(_uring->_uflags, URingFlags::TimerMultishotSupported) && info.count == TimerInfo::Infinite)
				|| info.count == 1) {
			return Rc<TimerURingHandle>::create(_uring, move(info));
		} else {
			return Rc<TimerFdURingHandle>::create(_uring, move(info));
		}
	}
	return nullptr;
}

Rc<ThreadHandle> Queue::Data::addThreadHandle() {
	return Rc<ThreadEventFdHandle>::create(_uring);
	if (_uring) {
		if constexpr (URING_THREAD_USE_FUTEX_HANDLE) {
			if (hasFlag(_uring->_uflags, URingFlags::FutexSupported)) {
				return Rc<ThreadUringHandle>::create(_uring);
			} else {
				return Rc<ThreadEventFdHandle>::create(_uring);
			}
		} else {
			return Rc<ThreadEventFdHandle>::create(_uring);
		}
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

Status Queue::Data::wakeup(QueueWakeupInfo &&info) {
	if (_uring) {
		return _uring->wakeup(info.flags, info.timeout);
	}
	return Status::ErrorNotImplemented;
}

bool Queue::Data::isValid() const {
	return _uring != nullptr;
}

void Queue::Data::cancel() {
	cleanup();
}

Queue::Data::~Data() {
	if (_uring) {
		delete _uring;
		_uring = nullptr;
	}
}

Queue::Data::Data(QueueRef *q, const QueueInfo &info) : QueueData(q, info.flags) {
	if (URingData::checkSupport()) {
		auto uring = new (memory::pool::acquire()) URingData(_queue, this, info, SignalsToIntercept);
		if (uring->_ringFd >= 0) {
			_uring = uring;
		} else {
			uring->~URingData();
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
