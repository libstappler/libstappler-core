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

#ifndef CORE_EVENT_PLATFORM_WINDOWS_SPEVENT_WINDOWS_CC_
#define CORE_EVENT_PLATFORM_WINDOWS_SPEVENT_WINDOWS_CC_

#include "SPEvent-windows.h"
#include "SPEventTimerIocp.h"
#include "SPEventThreadIocp.h"

namespace STAPPLER_VERSIONIZED stappler::event {

Rc<TimerHandle> Queue::Data::scheduleTimer(TimerInfo &&info) {
	if (_iocp) {
		return Rc<TimerIocpHandle>::create(&_iocpTimerClass, move(info));
	}
	return nullptr;
}

Rc<ThreadHandle> Queue::Data::addThreadHandle() {
	if (_iocp) {
		return Rc<ThreadIocpHandle>::create(&_iocpThreadClass);
	}
	return nullptr;
}

Status Queue::Data::submit() {
	if (_iocp) {
		return _iocp->submit();
	}
	return Status::ErrorNotImplemented;
}

uint32_t Queue::Data::poll() {
	if (_iocp) {
		return _iocp->poll();
	}
	return 0;
}

uint32_t Queue::Data::wait(TimeInterval ival) {
	if (_iocp) {
		return _iocp->wait(ival);
	}
	return 0;
}

Status Queue::Data::run(TimeInterval ival, QueueWakeupInfo &&info) {
	if (_iocp) {
		return _iocp->run(ival, info.flags, info.timeout);
	}
	return Status::ErrorNotImplemented;
}

Status Queue::Data::wakeup(QueueWakeupInfo &&info) {
	if (_iocp) {
		return _iocp->wakeup(info.flags, info.timeout);
	}
	return Status::ErrorNotImplemented;
}

bool Queue::Data::isValid() const {
	return _iocp != nullptr;
}

void Queue::Data::cancel() {
	if (_iocp) {
		_iocp->cancel();
	}
	cleanup();
}

Queue::Data::~Data() {
	if (_iocp) {
		delete _iocp;
		_iocp = nullptr;
	}
}

Queue::Data::Data(QueueRef *q, const QueueInfo &info) : QueueData(q, info.flags) {
	if (hasFlag(info.engineMask, QueueEngine::IOCP)) {
		setupIocpHandleClass<TimerIocpHandle, TimerIocpSource>(&_info, &_iocpTimerClass, true);
		setupIocpHandleClass<ThreadIocpHandle, ThreadIocpSource>(&_info, &_iocpThreadClass, true);
	}

	if (hasFlag(info.engineMask, QueueEngine::IOCP)) {
		auto iocp = new (memory::pool::acquire()) IocpData(_info.queue, this, info);
		if (iocp->_port) {
			_iocp = iocp;
			_iocp->runInternalHandles();
			_engine = QueueEngine::IOCP;
		} else {
			iocp->~IocpData();
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

#endif /* CORE_EVENT_PLATFORM_WINDOWS_SPEVENT_WINDOWS_CC_ */
