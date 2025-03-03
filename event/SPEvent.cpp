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

#include "SPCommon.h"


#include "platform/linux/SPEvent-linux.cc"
#include "platform/uring/SPEventThreadHandle-uring.cc"
#include "platform/uring/SPEvent-uring.cc"
#include "platform/epoll/SPEvent-epoll.cc"
//#include "platform/SPEvent-epoll.cc"
#include "platform/fd/SPEventFd.cc"
#include "platform/fd/SPEventFdStat.cc"
#include "platform/fd/SPEventEventFd.cc"
#include "platform/fd/SPEventSignalFd.cc"
#include "platform/fd/SPEventTimerFd.cc"
#include "platform/fd/SPEventDirFd.cc"

#include "detail/SPEventQueueData.cc"
#include "SPEventBufferChain.cc"
#include "SPEventHandle.cc"
#include "SPEventQueue.cc"

namespace STAPPLER_VERSIONIZED stappler::event {

std::ostream &operator<<(std::ostream &stream, Status status) {
	switch (status) {
	case Status::Ok: stream << "Status::Ok"; break;
	case Status::Declined: stream << "Status::Declined"; break;
	case Status::Done: stream << "Status::Done"; break;
	case Status::Suspended: stream << "Status::Suspended"; break;

	// errors
	case Status::ErrorNotPermitted: stream << "Status::ErrorNotPermitted"; break;
	case Status::ErrorNotFound: stream << "Status::ErrorNotFound"; break;
	case Status::ErrorInvalidArguemnt: stream << "Status::ErrorInvalidArguemnt"; break;
	case Status::ErrorAgain: stream << "Status::ErrorAgain"; break;
	case Status::ErrorBusy: stream << "Status::ErrorBusy"; break;
	case Status::ErrorNotImplemented: stream << "Status::ErrorNotImplemented"; break;
	case Status::ErrorAlreadyPerformed: stream << "Status::ErrorAlreadyPerformed"; break;
	case Status::ErrorInProgress: stream << "Status::ErrorInProgress"; break;
	case Status::ErrorCancelled: stream << "Status::ErrorCancelled"; break;
	default:
		if (toInt(status) < STATUS_ERRNO_OFFSET) {
			stream << "Status::Errno(" << -(toInt(status) + STATUS_ERRNO_OFFSET) << ")"; break;
		} else {
			stream << "Status::Unknown(" << toInt(status) << ")"; break;
		}
	}
	return stream;
}

}
