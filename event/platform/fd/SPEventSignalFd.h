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

#ifndef CORE_EVENT_PLATFORM_FD_SPEVENTSIGNALFD_H_
#define CORE_EVENT_PLATFORM_FD_SPEVENTSIGNALFD_H_

#include "SPEventFd.h"

namespace STAPPLER_VERSIONIZED stappler::event {

class SignalFdSource : public FdSource {
public:
	bool init(const sigset_t *);
};

class SignalFdHandle : public FdHandle {
public:
	virtual ~SignalFdHandle() = default;

	bool init(QueueRef *, QueueData *, SpanView<int>);

	bool read();
	bool process();
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

class SignalFdURingHandle : public SignalFdHandle {
public:
	virtual ~SignalFdURingHandle() = default;

	bool init(URingData *, SpanView<int>);

	Status rearm(SignalFdSource *);
	Status disarm(SignalFdSource *, bool suspend);

	void notify(SignalFdSource *, int32_t res, uint32_t flags);
};

}

#endif /* CORE_EVENT_PLATFORM_FD_SPEVENTSIGNALFD_H_ */
