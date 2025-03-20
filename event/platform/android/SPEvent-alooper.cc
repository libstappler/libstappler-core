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

#include "SPEvent-alooper.h"
#include "SPEvent-android.h"

namespace STAPPLER_VERSIONIZED stappler::event {

Status ALooperData::add(int fd, int events, Handle *handle) {
	if (ALooper_addFd(_looper, fd, 0, events, [] (int fd, int events, void *ptr) {
			NotifyData data;
			auto h = (Handle *)ptr;

			auto refId = h->retain();

			data.result = fd;
			data.queueFlags = events;
			data.userFlags = 0;

			h->_class->info->data->notify(h, data);

			auto st = h->getStatus();

			h->release(refId);

			if (st == Status::Ok) {
				return 1;
			} else {
				return 0;
			}
		}, handle) == 1) {

		return Status::Ok;
	}
	return Status::ErrorUnknown;
}

Status ALooperData::remove(int fd) {
	auto ret = ALooper_removeFd(_looper, fd);
	if (ret == 1) {
		return Status::Ok;
	} else if (ret == 0) {
		return Status::Declined;
	}
	return Status::ErrorUnknown;
}

Status ALooperData::submit() {
	return Status::Ok;
}

uint32_t ALooperData::poll() {
	auto ret = ALooper_pollOnce(0, nullptr, nullptr, nullptr);
	while (ret == ALOOPER_POLL_CALLBACK) {
		return 1;
	}
	return 0;
}

uint32_t ALooperData::wait(TimeInterval ival) {
	auto ret = ALooper_pollOnce(ival.toMillis(), nullptr, nullptr, nullptr);
	if (ret == ALOOPER_POLL_CALLBACK) {
		return 1;
	}
	return 0;
}

Status ALooperData::run(TimeInterval ival, WakeupFlags wakeupFlags, TimeInterval wakeupTimeout) {
	RunContext ctx;

	ctx.wakeupStatus = Status::Suspended;
	ctx.wakeupTimeout.store(wakeupTimeout.toMicros());
	ctx.runWakeupFlags = wakeupFlags;

	Rc<Handle> timerHandle;
	if (ival) {
		// set timeout
		timerHandle = _queue->get()->schedule(ival, [this, wakeupFlags] (Handle *handle, bool success) {
			if (success) {
				doWakeupInterrupt(wakeupFlags, false);
			}
		});
	}

	ctx.shouldWakeup.test_and_set();

	std::unique_lock lock(_runMutex);
	ctx.prev = _runContext;
	_runContext = &ctx;
	lock.unlock();

	int ret = 0;

	while (ctx.shouldWakeup.test_and_set()) {
		ret = ALooper_pollOnce(-1, nullptr, nullptr, nullptr);
		if (ret == ALOOPER_POLL_ERROR) {
			log::error("event::ALooperData", "ALooper error: ", ret);
			ctx.wakeupStatus = Status::ErrorUnknown;
			break;
		}
	}

	if (ret == ALOOPER_POLL_ERROR) {
		log::error("event::Queue", "ALooper failed with error");
	}

	auto wFlags = WakeupFlags(_runContext->wakeupFlags.load());
	if (hasFlag(wFlags, WakeupFlags::Graceful)) {
		if (suspendHandles() == Status::Done) {
			_runContext->wakeupStatus = Status::Ok;
		}
	}

	if (ival) {
		// remove timeout if set
		timerHandle->cancel();
	}

	lock.lock();
	_runContext = ctx.prev;
	lock.unlock();

	return ctx.wakeupStatus;
}

Status ALooperData::wakeup(WakeupFlags flags, TimeInterval gracefulTimeout) {
	std::unique_lock lock(_runMutex);
	if (auto v = _runContext) {
		v->wakeupFlags |= toInt(flags);
		v->wakeupTimeout = gracefulTimeout.toMicros();
	}
	_runMutex.unlock();
	_eventFd->write(1);
	return Status::Ok;
}

Status ALooperData::suspendHandles() {
	_runContext->wakeupStatus = Status::Suspended;

	auto nhandles = _data->suspendAll();
	_runContext->wakeupCounter = nhandles;

	return Status::Done;
}

Status ALooperData::doWakeupInterrupt(WakeupFlags flags, bool externalCall) {
	if (!_runContext) {
		return Status::ErrorInvalidArguemnt;
	}

	if (hasFlag(flags, WakeupFlags::Graceful)) {
		if (suspendHandles() == Status::Done) {
			_runContext->shouldWakeup.clear();
			_runContext->wakeupStatus = Status::Ok; // graceful wakeup
		}
		return Status::Suspended; // do not rearm eventfd
	} else {
		_runContext->shouldWakeup.clear();
		_runContext->wakeupStatus = externalCall ? Status::Suspended : Status::Done; // forced wakeup
		return Status::Ok; // rearm eventfd
	}
}

void ALooperData::runInternalHandles() {
	_data->runHandle(_eventFd);
}

void ALooperData::cancel() {
	ALooper_wake(_looper);
}

ALooperData::ALooperData(QueueRef *q, Queue::Data *data, const QueueInfo &info, SpanView<int> sigs)
: _queue(q), _data(data), _flags(info.flags) {

	if (hasFlag(_flags, QueueFlags::Protected)) {
		log::warn("event::Queue", "QueueFlags::Protected is not supported by ALooper queue, ignored");
	}

	_looper = ALooper_prepare(0);

	_eventFd = Rc<EventFdALooperHandle>::create(&data->_alooperEventFdClass,
			CompletionHandle<EventFdALooperHandle>::create<ALooperData>(this,
					[] (ALooperData *data, EventFdALooperHandle *h, uint32_t value, Status st) {
		if (st == Status::Ok && data->_runContext) {
			data->doWakeupInterrupt(WakeupFlags(data->_runContext->wakeupFlags.exchange(0)), true);
		}
	}));
}

ALooperData::~ALooperData() {
	if (_looper) {
		ALooper_release(_looper);
		_looper = nullptr;
	}
}

}
