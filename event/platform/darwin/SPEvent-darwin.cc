/**
 Copyright (c) 2025 Stappler Team <admin@stappler.org>

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

#include "SPEvent-darwin.h"

namespace STAPPLER_VERSIONIZED stappler::event {

Rc<TimerHandle> Queue::Data::scheduleTimer(TimerInfo &&info) {
	return nullptr;
}

Rc<ThreadHandle> Queue::Data::addThreadHandle() {
	return nullptr;
}

Status Queue::Data::submit() {
	if (_kqueue) {
		return _kqueue->submit();
	} else if (_runloop) {
		return _runloop->submit();
	}
	return Status::ErrorNotImplemented;
}

uint32_t Queue::Data::poll() {
	if (_kqueue) {
		return _kqueue->poll();
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
	return _kqueue != nullptr || _runloop != nullptr;
}

void Queue::Data::cancel() {
	if (_iocp) {
		_iocp->cancel();
	}
	cleanup();
}

Queue::Data::~Data() {
	if (_kqueue) {
		delete _kqueue;
		_kqueue = nullptr;
	}
	if (_runloop) {
		delete _runloop;
		_runloop = nullptr;
	}
}

Queue::Data::Data(QueueRef *q, const QueueInfo &info) : QueueData(q, info.flags) {
	if (hasFlag(info.engineMask, QueueEngine::KQueue)) {
		// init kqueue classes

		/*auto queue = new (memory::pool::acquire()) KQueueData(_info.queue, this, info);
		 if (queue->_kqueue) {
		 _kqueue = queue;
		 _kqueue->runInternalHandles();
		 _engine = QueueEngine::KQueue;
		 return;
		 } else {
		 queue->~KQueueData();
		 }*/
	}

	if (hasFlag(info.engineMask, QueueEngine::RunLoop)) {
		// init runloop classes

		/*auto runloop = new (memory::pool::acquire()) RunLoopData(_info.queue, this, info);
		if (runloop->_runloop) {
			_runloop = runloop;
			_runloop->runInternalHandles();
			_engine = QueueEngine::RunLoop;
			return;
		} else {
			runloop->~RunLoopData();
		}*/
	}
}

}

namespace STAPPLER_VERSIONIZED stappler::event::platform {

Rc<QueueRef> getThreadQueue(QueueInfo &&info) {
	// Allow only CFRunLoop
	info.engineMask = QueueEngine::RunLoop;

	return Queue::create(move(info));
}

}

