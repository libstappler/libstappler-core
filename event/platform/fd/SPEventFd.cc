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

#include "SPEventFd.h"
#include "SPPlatformUnistd.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>

namespace STAPPLER_VERSIONIZED stappler::event {

bool FdSource::init(int fd) {
	_fd = fd;
	return true;
}

void FdSource::cancel() {
	if (_fd >= 0) {
		::close(_fd);
		_fd = -1;
	}
}

void FdSource::setEpollMask(uint32_t ev) {
	_epoll.event.events = ev;
	_epoll.event.data.ptr = this;
}

void FdSource::setURingCallback(URingData *r, URingCallback cb) {
	_uring.uring = r;
	_uring.ucb = cb;
}

FdHandle::~FdHandle() {
	getData<FdSource>()->cancel();
}

}
