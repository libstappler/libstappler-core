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

#ifndef CORE_EVENT_PLATFORM_FD_SPEVENTPOLLFD_H_
#define CORE_EVENT_PLATFORM_FD_SPEVENTPOLLFD_H_

#include "SPEventFd.h"

#include <poll.h>

namespace STAPPLER_VERSIONIZED stappler::event {

enum class PollFlags : uint16_t {
	None = 0,
	In = POLLIN,
	Pri = POLLPRI,
	Out = POLLOUT,
	Err = POLLERR,
	HungUp = POLLHUP,
	Invalid = POLLNVAL,

	PollMask = 0x3FFF,
	CloseFd = 0x4000,
};

SP_DEFINE_ENUM_AS_MASK(PollFlags)

class SP_PUBLIC PollFdHandle : public FdHandle {
public:
	static Rc<PollFdHandle> create(const Queue *, int fd, PollFlags, mem_std::Function<Status(int, PollFlags)> &&);

	virtual ~PollFdHandle() = default;

	bool init(QueueRef *, QueueData *, int, PollFlags, mem_std::Function<Status(int, PollFlags)> &&);

protected:
	PollFlags _flags = PollFlags::None;
	mem_std::Function<Status(int, PollFlags)> _notify;
};

class SP_PUBLIC PollFdURingHandle : public PollFdHandle {
public:
	virtual ~PollFdURingHandle() = default;

	bool init(URingData *, int, PollFlags, mem_std::Function<Status(int, PollFlags)> &&);

	Status rearm(FdSource *);
	Status disarm(FdSource *, bool suspend);

	void notify(FdSource *, int32_t res, uint32_t flags, URingUserFlags uflags);
};

}

#endif /* CORE_EVENT_PLATFORM_FD_SPEVENTPOLLFD_H_ */
