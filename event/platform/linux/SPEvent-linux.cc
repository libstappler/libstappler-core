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

#include "../epoll/SPEvent-epoll.h"
#include "../uring/SPEvent-uring.h"

namespace STAPPLER_VERSIONIZED stappler::event {

struct Queue::Data : public mem_pool::AllocBase {
	Queue *_queue = nullptr;

	URingData *_uring = nullptr;

	Rc<SignalFdSource> _signalFd;

	QueueFlags _flags = QueueFlags::None;

	Status submit();
	uint32_t poll();
	uint32_t wait(TimeInterval);
	Status run(TimeInterval);

	bool isValid() const;

	~Data();
	Data(Queue *q, const QueueInfo &info, QueueFlags flags);
};

static int SignalsToIntercept[] = { SIGUSR1, SIGUSR2 };

Status Queue::Data::submit() {
	if (_uring) {
		return _uring->submit();
	}
	return Status::NotImplemented;
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

Status Queue::Data::run(TimeInterval ival) {
	if (_uring) {
		return _uring->run(ival);
	}
	return Status::NotImplemented;
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

Queue::Data::Data(Queue *q, const QueueInfo &info, QueueFlags flags) : _queue(q), _flags(flags) {
	if (hasFlag(_flags, QueueFlags::Protected)) {
		_signalFd = Rc<SignalFdSource>::create(SignalsToIntercept);
	}

	if (URingData::checkSupport()) {
		_uring = new (memory::pool::acquire()) URingData(_signalFd, info, flags);
	}
}

}

namespace STAPPLER_VERSIONIZED stappler::event::platform {

static uint64_t getStaticMinFrameTime() {
	return 1000'000 / 60;
}

static clockid_t getClockSource() {
	struct timespec ts;

	auto minFrameNano = (getStaticMinFrameTime() * 1000) / 5; // clock should have at least 1/5 frame resolution
	if (clock_getres(CLOCK_MONOTONIC_COARSE, &ts) == 0) {
		if (ts.tv_sec == 0 && uint64_t(ts.tv_nsec) < minFrameNano) {
			return CLOCK_MONOTONIC_COARSE;
		}
	}

	if (clock_getres(CLOCK_MONOTONIC, &ts) == 0) {
		if (ts.tv_sec == 0 && uint64_t(ts.tv_nsec) < minFrameNano) {
			return CLOCK_MONOTONIC;
		}
	}

	if (clock_getres(CLOCK_MONOTONIC_RAW, &ts) == 0) {
		if (ts.tv_sec == 0 && uint64_t(ts.tv_nsec) < minFrameNano) {
			return CLOCK_MONOTONIC_RAW;
		}
	}

	return CLOCK_MONOTONIC;
}

uint64_t clock(ClockType type) {
	static clockid_t ClockSource = getClockSource();

	struct timespec ts;
	switch (type) {
	case ClockType::Default: clock_gettime(ClockSource, &ts); break;
	case ClockType::Monotonic: clock_gettime(CLOCK_MONOTONIC, &ts); break;
	case ClockType::Realtime: clock_gettime(CLOCK_REALTIME, &ts); break;
	case ClockType::Process: clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts); break;
	case ClockType::Thread: clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts); break;
	}

	return static_cast<uint64_t>(ts.tv_sec) * static_cast<uint64_t>(1000'000) + static_cast<uint64_t>(ts.tv_nsec / 1000);
}

void sleep(uint64_t microseconds) {
	usleep(useconds_t(microseconds));
}

}

#endif
