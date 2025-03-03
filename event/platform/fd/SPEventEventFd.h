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

#ifndef CORE_EVENT_PLATFORM_FD_SPEVENTEVENTFD_H_
#define CORE_EVENT_PLATFORM_FD_SPEVENTEVENTFD_H_

#include "SPEventFd.h"

namespace STAPPLER_VERSIONIZED stappler::event {

class EventFdSource : public FdSource {
public:
	bool init();
};

class EventFdHandle : public FdHandle {
public:
	virtual ~EventFdHandle() = default;

	bool init(QueueRef *, QueueData *, mem_std::Function<Status()> &&);

	bool read();
	bool write(uint64_t = 1);

	const uint64_t *getValue() const { return &_value; }

protected:
	uint64_t _value = 0;
	mem_std::Function<Status()> _callback;
};

class EventFdURingHandle : public EventFdHandle {
public:
	virtual ~EventFdURingHandle() = default;

	bool init(URingData *, mem_std::Function<Status()> &&);

	Status rearm(EventFdSource *);
	Status disarm(EventFdSource *, bool suspend);

	void notify(EventFdSource *, int32_t res, uint32_t flags);
};

}

#endif /* CORE_EVENT_PLATFORM_FD_SPEVENTEVENTFD_H_ */
