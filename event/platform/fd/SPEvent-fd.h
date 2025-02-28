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

#ifndef CORE_EVENT_SPEVENT_FD_H_
#define CORE_EVENT_SPEVENT_FD_H_

#include "SPEventHandle.h"

#if LINUX || ANDROID

#include <sys/epoll.h>
#include <sys/signalfd.h>

namespace STAPPLER_VERSIONIZED stappler::event {

struct URingData;

class FdSource {
public:
	using URingCallback = void (*) (Handle *, int32_t res, uint32_t flags);

	virtual ~FdSource();

	bool init(int fd);

	void cancel();

	int getFd() const { return _fd; }

	uint32_t getEventMask() const { return _epoll.event.events; }
	const epoll_event *getEvent() const { return &_epoll.event; }

	URingData *getURingData() const { return _uring.uring; }
	const URingCallback &getCallback() const { return _uring.ucb; }

	void setEpollMask(uint32_t);
	void setURingCallback(URingData *, URingCallback);

protected:
	int _fd = -1;
	union {
		struct {
			epoll_event event;
		} _epoll;
		struct {
			URingData *uring;
			URingCallback ucb;
		} _uring;
	};
};

class SignalFdSource : public FdSource {
public:
	virtual ~SignalFdSource();

	bool init();
	bool init(SpanView<int>);

	bool read();
	bool process(bool panding = false);
	void enable();
	void enable(const sigset_t *);
	void disable();

	const sigset_t *getSigset() const { return &_sigset; }

	const signalfd_siginfo *getInfo() const { return &_info; }

protected:
	signalfd_siginfo _info;
	sigset_t _sigset;
	mem_std::Vector<int> _extra;
};

class EventFdSource : public FdSource {
public:
	virtual ~EventFdSource();

	bool init();
	bool read();
	bool write(uint64_t = 1);

	const uint64_t *getValue() const { return &_value; }

protected:
	uint64_t _value = 0;
};

class TimerFdSource : public FdSource {
public:
	virtual ~TimerFdSource() = default;

	bool init(TimerInfo &&info);

	void setCurrent(uint32_t c) { _current = c; }
	uint32_t getCurrent() const { return _current; }
	uint32_t getCount() const { return _count; }

protected:
	uint32_t _count = 0;
	uint32_t _current = 0;
};

class TimerFdHandle : public TimerHandle {
public:
	virtual ~TimerFdHandle() = default;

	bool init(QueueRef *, QueueData *, TimerInfo &&);

	virtual Status cancel(Status) override;

	const uint64_t *getValue() const { return &_value; }

protected:
	uint64_t _value = 0;
};

template <typename TimeSpec>
inline void setNanoTimespec(TimeSpec &ts, TimeInterval ival) {
	ts.tv_sec = ival.toSeconds();
	ts.tv_nsec = (ival.toMicros() - ts.tv_sec * 1'000'000ULL) * 1000;
}

}

#endif

#endif /* CORE_EVENT_SPEVENT_FD_H_ */
