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
#include "SPEvent-epoll.h"

#include <signal.h>

namespace STAPPLER_VERSIONIZED stappler::event {

Status EPollData::add(int fd, const epoll_event &ev) {
	auto ret = ::epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, const_cast<epoll_event *>(&ev));
	if (ret < 0) {
		return status::errnoToStatus(errno);
	}
	return Status::Ok;
}

Status EPollData::remove(int fd) {
	auto ret = ::epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, nullptr);
	if (ret < 0) {
		return status::errnoToStatus(errno);
	}
	return Status::Ok;
}

Status EPollData::runPoll(TimeInterval ival, bool infinite) {
	const sigset_t *sigset = nullptr;
	if (hasFlag(_flags, QueueFlags::Protected)) {
		sigset = _signalFd->getCurrentSigset();
	}

	int nevents = 0;
#if LINUX
	if (hasFlag(_eflags, EPollFlags::HaveEPollPWait2) && ival && !infinite) {
		struct timespec timeout;

		setNanoTimespec(timeout, ival);

		nevents = ::epoll_pwait2(_epollFd, _events.data(), _events.size(), &timeout, sigset);
	} else {
		nevents = ::epoll_pwait(_epollFd, _events.data(), _events.size(), infinite ? -1 : ival.toMillis(), sigset);
	}
#else
	nevents = ::epoll_pwait(_epollFd, _events.data(), _events.size(), infinite ? -1 : ival.toMillis(), sigset);
#endif

	if (nevents == 0) {
		return Status::Ok;
	} else if (nevents > 0) {
		return Status(nevents);
	} else {
		return status::errnoToStatus(errno);
	}
}

Status EPollData::processEvents(uint32_t nevents) {
	NotifyData data;
	for (uint32_t i = 0; i < nevents; ++ i) {
		auto &ev = _events.at(i);

		auto h = (Handle *)ev.data.ptr;

		auto refId = h->retain();

		data.result = 0;
		data.queueFlags = ev.events;
		data.userFlags = 0;

		_data->notify(h, data);

		h->release(refId);
	}
	return Status::Ok;
}

Status EPollData::submit() {
	return Status::Ok;
}

uint32_t EPollData::poll() {
	auto status = runPoll(TimeInterval());
	if (toInt(status) > 0) {
		uint32_t nevents = static_cast<uint32_t>(toInt(status));
		processEvents(nevents);
		return nevents;
	}
	return 0;
}

uint32_t EPollData::wait(TimeInterval ival) {
	auto status = runPoll(ival);
	if (toInt(status) > 0) {
		uint32_t nevents = static_cast<uint32_t>(toInt(status));
		processEvents(nevents);
		return nevents;
	}
	return 0;
}

Status EPollData::run(TimeInterval ival, WakeupFlags wakeupFlags, TimeInterval wakeupTimeout) {
	_wakeupStatus = Status::Suspended;
	_wakeupTimeout.store(wakeupTimeout.toMicros());
	_runWakeupFlags = wakeupFlags;

	Rc<Handle> timerHandle;
	if (ival) {
		// set timeout
		timerHandle = _queue->get()->schedule(ival, [this, wakeupFlags] (Handle *handle, bool success) {
			if (success) {
				doWakeupInterrupt(wakeupFlags, false);
			}
		});
	}

	_shouldWakeup.test_and_set();

	while (_shouldWakeup.test_and_set()) {
		auto status = runPoll(ival, true);
		if (toInt(status) >= 0) {
			auto nevents = static_cast<uint32_t>(toInt(status));
			status = processEvents(nevents);
			if (status != Status::Ok) {
				log::error("event::EPollData", "epoll error: ", status);
				_wakeupStatus = status;
				break;
			}
		} else if (status != Status::ErrorInterrupted) {
			log::error("event::EPollData", "epoll error: ", status);
			_wakeupStatus = status;
			break;
		}
	}

	if (ival) {
		// remove timeout if set
		timerHandle->cancel();
	}

	return _wakeupStatus;
}

Status EPollData::wakeup(WakeupFlags flags, TimeInterval gracefulTimeout) {
	_wakeupFlags |= toInt(flags);
	_wakeupTimeout = gracefulTimeout.toMicros();
	_eventFd->write(1);
	return Status::Ok;
}

Status EPollData::suspendHandles() {
	_wakeupStatus = Status::Suspended;

	auto nhandles = _data->suspendAll();
	_wakeupCounter = nhandles;

	return Status::Done;
}

Status EPollData::doWakeupInterrupt(WakeupFlags flags, bool externalCall) {
	if (hasFlag(flags, WakeupFlags::Graceful)) {
		if (suspendHandles() == Status::Done) {
			_shouldWakeup.clear();
			_wakeupStatus = Status::Ok; // graceful wakeup
		}
		return Status::Suspended; // do not rearm eventfd
	} else {
		_shouldWakeup.clear();
		_wakeupStatus = externalCall ? Status::Suspended : Status::Done; // forced wakeup
		return Status::Ok; // rearm eventfd
	}
}

void EPollData::runInternalHandles() {
	// run internal services
	if (_signalFd && hasFlag(_flags, QueueFlags::Protected)) {
		// enable with current mask
		_signalFd->enable();
		_data->runHandle(_signalFd);
	}

	_data->runHandle(_eventFd);
}

void EPollData::cancel() {
	doWakeupInterrupt(WakeupFlags::None, false);
}

EPollData::EPollData(QueueRef *q, Queue::Data *data, const QueueInfo &info, SpanView<int> sigs)
: _queue(q), _data(data), _flags(info.flags) {

	_eventFd = Rc<EventFdEPollHandle>::create(&data->_epollEventFdClass,
			CompletionHandle<EventFdEPollHandle>::create<EPollData>(this,
					[] (EPollData *data, EventFdEPollHandle *h, uint32_t value, Status st) {
		if (st == Status::Ok) {
			data->doWakeupInterrupt(WakeupFlags(data->_wakeupFlags.exchange(0)), true);
		}
	}));

	if (!_eventFd) {
		log::error("event::Queue", "Fail to initialize eventfd");
		return;
	}

	if (hasFlag(_flags, QueueFlags::Protected)) {
		_signalFd = Rc<SignalFdEPollHandle>::create(&data->_epollSignalFdClass, sigs);
		if (!_signalFd) {
			log::error("event::Queue", "Fail to initialize signalfd");
			return;
		}
	}

#if LINUX
	struct utsname buffer;
	if (uname(&buffer) != 0) {
		log::info("event::EPollData", "Fail to detect kernel version");
		return;
	}

	if (strverscmp(buffer.release, "5.11.0") >= 0) {
		_eflags |= EPollFlags::HaveEPollPWait2;
	}
#endif

	auto cleanup = [&] {
		if (_epollFd >= 0) {
			::close(_epollFd);
			_epollFd = -1;
		}
	};

	_epollFd = ::epoll_create1(EPOLL_CLOEXEC);
	if (_epollFd < 0) {
		cleanup();
		return;
	}

	auto size = info.completeQueueSize;

	if (size == 0) {
		size = info.submitQueueSize;
	}
	_events.resize(size);

}

EPollData::~EPollData() {
	if (_epollFd >= 0) {
		::close(_epollFd);
		_epollFd = -1;
	}
}

}
