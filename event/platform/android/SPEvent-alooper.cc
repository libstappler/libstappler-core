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
	if (ret == ALOOPER_POLL_CALLBACK) {
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
	_wakeupStatus = Status::Suspended;
	_wakeupFlags.store(0);

	Rc<Handle> timerHandle;
	if (ival) {
		// set timeout
		timerHandle = _queue->get()->schedule(ival, [this, wakeupFlags] (Handle *handle, bool success) {
			if (success) {
				_wakeupStatus = Status::Done;
				wakeup(wakeupFlags, TimeInterval());
			}
		});
	}

	int ret = 0;

	while (ret != ALOOPER_POLL_WAKE && ret != ALOOPER_POLL_ERROR) {
		ret = ALooper_pollOnce(-1, nullptr, nullptr, nullptr);
	}

	if (ret == ALOOPER_POLL_ERROR) {
		log::error("event::Queue", "ALooper failed with error");
	}

	auto wFlags = WakeupFlags(_wakeupFlags.load());
	if (hasFlag(wFlags, WakeupFlags::Graceful)) {
		if (suspendHandles() == Status::Done) {
			_wakeupStatus = Status::Ok;
		}
	}

	if (ival) {
		// remove timeout if set
		timerHandle->cancel();
	}

	return _wakeupStatus;
}

Status ALooperData::wakeup(WakeupFlags flags, TimeInterval gracefulTimeout) {
	_wakeupFlags |= toInt(flags);
	ALooper_wake(_looper);
	return Status::Ok;
}

Status ALooperData::suspendHandles() {
	_wakeupStatus = Status::Suspended;

	auto nhandles = _data->suspendAll();
	_wakeupCounter = nhandles;

	return Status::Done;
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
}

ALooperData::~ALooperData() {
	if (_looper) {
		ALooper_release(_looper);
		_looper = nullptr;
	}
}

}
