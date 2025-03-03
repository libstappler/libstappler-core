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
#include "SPEventFileHandle.h"
#include "SPEventTimerHandle.h"

#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <linux/stat.h>

namespace STAPPLER_VERSIONIZED stappler::event {

struct URingData;

class FdSource {
public:
	using URingCallback = void (*) (Handle *, int32_t res, uint32_t flags);

	bool init(int fd);

	void cancel();

	int getFd() const { return _fd; }
	void setFd(int fd) { _fd = fd; }

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

class FdHandle : public Handle {
public:
	virtual ~FdHandle();

	int getFd() const { return getData<FdSource>()->getFd(); }

protected:
	template <typename T, typename S>
	void setupURing(URingData *uring, T *t) {
		setup<T, S>();

		auto source = getData<S>();
		source->setURingCallback(uring, [] (Handle *h, int32_t res, uint32_t flags) {
			reinterpret_cast<T *>(h)->notify(h->getData<S>(), res, flags);
		});
	}
};

class StatURingHandle : public StatHandle {
public:
	virtual ~StatURingHandle() = default;

	bool init(URingData *, StatOpInfo &&);

	Status run(FdSource *);

	void notify(FdSource *, int32_t res, uint32_t flags);

protected:
	struct statx _buffer;
};

template <typename TimeSpec>
inline void setNanoTimespec(TimeSpec &ts, TimeInterval ival) {
	ts.tv_sec = ival.toSeconds();
	ts.tv_nsec = (ival.toMicros() - ts.tv_sec * 1'000'000ULL) * 1000;
}

}

#endif /* CORE_EVENT_SPEVENT_FD_H_ */
