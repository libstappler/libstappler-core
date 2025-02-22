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

#if LINUX

#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

namespace STAPPLER_VERSIONIZED stappler::event {

// see https://stackoverflow.com/questions/57299119/iterating-over-a-list-of-possible-signals
struct siginfo {
	int code;
	const char *name;
	const char *description;
};

static const siginfo siglist[] = {
#ifdef SIGABRT
	{SIGABRT  , "SIGABRT"  , "Abort signal"},
#endif
#ifdef SIGALRM
	{SIGALRM  , "SIGALRM"  , "Timer signal"},
#endif
#ifdef SIGBUS
	{SIGBUS   , "SIGBUS"   , "Bus error (bad memory access)"},
#endif
#ifdef SIGCHLD
	{SIGCHLD  , "SIGCHLD"  , "Child stopped or terminated"},
#endif
#ifdef SIGCLD
	{SIGCLD   , "SIGCLD"   , "Child stopped or terminated"},
#endif
#ifdef SIGCONT
	{SIGCONT  , "SIGCONT"  , "Continue if stopped"},
#endif
#ifdef SIGEMT
	{SIGEMT   , "SIGEMT"   , "Emulator trap"},
#endif
#ifdef SIGFPE
	{SIGFPE   , "SIGFPE"   , "Floating-point exception"},
#endif
#ifdef SIGHUP
	{SIGHUP   , "SIGHUP"   , "Hangup detected on controlling terminal or death of controlling process"},
#endif
#ifdef SIGILL
	{SIGILL   , "SIGILL"   , "Illegal Instruction"},
#endif
#ifdef SIGINFO
	{SIGINFO  , "SIGINFO"  , "Power failure"},
#endif
#ifdef SIGINT
	{SIGINT   , "SIGINT"   , "Interrupt from keyboard"},
#endif
#ifdef SIGIO
	{SIGIO    , "SIGIO"    , "I/O now possible"},
#endif
#ifdef SIGIOT
	{SIGIOT   , "SIGIOT"   , "IOT trap: Abort signal"},
#endif
#ifdef SIGKILL
	{SIGKILL  , "SIGKILL"  , "Kill signal"},
#endif
#ifdef SIGLOST
	{SIGLOST  , "SIGLOST"  , "File lock lost "},
#endif
#ifdef SIGPIPE
	{SIGPIPE  , "SIGPIPE"  , "Broken pipe: write to pipe with no readers"},
#endif
#ifdef SIGPOLL
	{SIGPOLL  , "SIGPOLL"  , "Pollable event: I/O now possible"},
#endif
#ifdef SIGPROF
	{SIGPROF  , "SIGPROF"  , "Profiling timer expired"},
#endif
#ifdef SIGPWR
	{SIGPWR   , "SIGPWR"   , "Power failure"},
#endif
#ifdef SIGQUIT
	{SIGQUIT  , "SIGQUIT"  , "Quit from keyboard"},
#endif
#ifdef SIGSEGV
	{SIGSEGV  , "SIGSEGV"  , "Invalid memory reference"},
#endif
#ifdef SIGSTKFLT
	{SIGSTKFLT, "SIGSTKFLT", "Stack fault on coprocessor"},
#endif
#ifdef SIGSTOP
	{SIGSTOP  , "SIGSTOP"  , "Stop process"},
#endif
#ifdef SIGTSTP
	{SIGTSTP  , "SIGTSTP"  , "Stop typed at terminal"},
#endif
#ifdef SIGSYS
	{SIGSYS   , "SIGSYS"   , "Bad system call "},
#endif
#ifdef SIGTERM
	{SIGTERM  , "SIGTERM"  , "Termination signal"},
#endif
#ifdef SIGTRAP
	{SIGTRAP  , "SIGTRAP"  , "Trace/breakpoint trap"},
#endif
#ifdef SIGTTIN
	{SIGTTIN  , "SIGTTIN"  , "Terminal input for background process"},
#endif
#ifdef SIGTTOU
	{SIGTTOU  , "SIGTTOU"  , "Terminal output for background process"},
#endif
#ifdef SIGUNUSED
	{SIGUNUSED, "SIGUNUSED", "Bad system call "},
#endif
#ifdef SIGURG
	{SIGURG   , "SIGURG"   , "Urgent condition on socket "},
#endif
#ifdef SIGUSR1
	{SIGUSR1  , "SIGUSR1"  , "User-defined signal 1"},
#endif
#ifdef SIGUSR2
	{SIGUSR2  , "SIGUSR2"  , "User-defined signal 2"},
#endif
#ifdef SIGVTALRM
	{SIGVTALRM, "SIGVTALRM", "Virtual alarm clock "},
#endif
#ifdef SIGXCPU
	{SIGXCPU  , "SIGXCPU"  , "CPU time limit exceeded "},
#endif
#ifdef SIGXFSZ
	{SIGXFSZ  , "SIGXFSZ"  , "File size limit exceeded"},
#endif
#ifdef SIGWINCH
	{SIGWINCH , "SIGWINCH" , "Window resize signal"},
#endif
};

static const siginfo *getSignalInfo(int sig) {
	for (size_t i = 0; i < sizeof(siglist) / sizeof(siginfo); ++ i) {
		if (siglist[i].code == sig) {
			return &siglist[i];
		}
	}
	return nullptr;
}

/*
Queue::Data::~Data() {
	if (_epollFd >= 0) {
		::close(_epollFd);
		_epollFd = -1;
	}

	_signalFd = nullptr;
}

Queue::Data::Data(Queue *q) : _queue(q) {
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

	auto signalFd = Rc<SignalFdSource>::create();
	if (!signalFd) {
		cleanup();
		return;
	}

	_signalFd = Rc<Handle>::create(_queue, signalFd);
	if (!_signalFd || !updateClient(signalFd)) {
		cleanup();
		return;
	}
}

bool Queue::Data::updateClient(Source *source) {
	SourceData *sourceData = (SourceData *)source->getData();

	int err = 0;
	if (!sourceData->connected) {
		if (sourceData->event.events != 0) {
			err = ::epoll_ctl(_epollFd, EPOLL_CTL_ADD, sourceData->fd, &sourceData->event);
		}
	} else {
		if (sourceData->event.events == 0) {
			err = ::epoll_ctl(_epollFd, EPOLL_CTL_DEL, sourceData->fd, &sourceData->event);
		} else {
			err = ::epoll_ctl(_epollFd, EPOLL_CTL_MOD, sourceData->fd, &sourceData->event);
		}
	}

	if (err != 0) {
		char buf[256] = { 0 };
		log::error("event::Queue", "Failed to update client epoll_ctl(",
				sourceData->fd, "): ",  strerror_r(errno, buf, 255));
		return false;
	}
	return true;
}

bool Queue::Data::isValid() const {
	return _signalFd && _epollFd >= 0;
}

bool Queue::Data::waitForEvent(TimeInterval ival) {
	::clock_gettime(CLOCK_MONOTONIC, &_eventTs);

	while (ival && _nHandlers > 0) {
		if (!wait(ival)) {
			log::error("event::Queue", "epoll_wait failed");
			return false;
		}
	}
	return true;
}

void Queue::Data::processEvents(epoll_event &event) {
	ErrorFlags errorFlag = ErrorFlags::None;
	Source *source = (Source *)event.data.ptr;
	SourceData *sourceData = (SourceData *)source->getData();

	if ((event.events & EPOLLHUP) != 0) {
		errorFlag |= ErrorFlags::HangUp;
	}
	if ((event.events & EPOLLERR) != 0) {
		errorFlag |= ErrorFlags::GenericError;
	}

	if ((event.events & EPOLLRDHUP) != 0) {
		errorFlag |= ErrorFlags::StreamClosed;
	}

	if ((event.events & EPOLLIN) != 0 || errorFlag != ErrorFlags::None) {
		// process read chain or inform about error
		while (sourceData->inHandle->read(errorFlag)) {
			auto ret = sourceData->inHandle;

			// mark event as processed
			event.events &= ~EPOLLIN;

			if (auto next = ret->getNext()) {
				sourceData->inHandle = next;
			} else {
				// disconnect from read
				sourceData->inHandle = nullptr;
				sourceData->event.events &= ~EPOLLIN;
				updateClient(source);
			}
		}
	}

	if ((event.events & EPOLLOUT) != 0 || errorFlag != ErrorFlags::None) {
		// process write chain or inform about error
		while (sourceData->inHandle->write(errorFlag)) {
			auto ret = sourceData->inHandle;

			// mark event as processed
			event.events &= ~EPOLLOUT;

			if (auto next = ret->getNext()) {
				sourceData->outHandle = next;
			} else {
				// disconnect from write
				sourceData->outHandle = nullptr;
				sourceData->event.events &= ~EPOLLOUT;
				updateClient(source);
			}
		}
	}

	if (errorFlag != ErrorFlags::None) {
		source->setError(errorFlag);
	}
}

bool Queue::Data::wait(TimeInterval &ival) {
	auto signalFd = static_cast<SignalFdSource *>(_signalFd->getSource());

	struct sigset_t sigset;
	sigemptyset(&sigset);

	struct timespec timeout;
	timeout.tv_sec = ival.toSeconds();
	timeout.tv_nsec = (ival.toMicroseconds() - ival.toSeconds() * 1000'000) * 1000;

	ival -= TimeInterval::microseconds(_eventTs.tv_sec * 1000'000 + _eventTs.tv_nsec / 1000);

	if (!ival) {
		return true;
	}

	signalFd->enable();

	std::array<struct epoll_event, MaxEvents> events;

	int nevents = epoll_pwait2(_epollFd, events.data(), MaxEvents, &timeout, &sigset);

	signalFd->disable();

	ival -= TimeInterval::microseconds(timeout.tv_sec * 1000'000 + timeout.tv_nsec / 1000);

	::clock_gettime(CLOCK_MONOTONIC, &_eventTs);

	if (nevents == -1 && errno != EINTR) {
		char buf[256] = { 0 };
		log::error("event::Queue", "epoll_wait() failed with errno ", errno, " (", strerror_r(errno, buf, 255), ")");
		return false;
	} else if (nevents <= 0 && errno == EINTR) {
		return true;
	}

	// pre-check signalfd
	for (int i = 0; i < nevents; i++) {
		if (signalFd == events[i].data.ptr) {
			if ((events[i].events & EPOLLIN) != 0) {
				signalFd->read();
			}
			if ((events[i].events & ~EPOLLIN) != 0) {
				log::error("event::Queue", "Invalid event mask for signalfd: ", events[i].events);
				return false;
			}
		} else {
			if (!processEvents(events[i])) {
				log::error("event::Queue", "Fail to process events on fd: ", getFd(((Source *)events[i].data.ptr)));
				return false;
			}
		}
	}

	return true;
}
*/

SignalFdSource::~SignalFdSource() {
	auto fd = ((SourceData *)getData())->fd;
	if (fd >= 0) {
		::close(fd);
	}
}

bool SignalFdSource::init() {
	return init(SpanView<int>());
}

bool SignalFdSource::init(SpanView<int> sigs) {
	_extra = sigs.vec<memory::StandartInterface>();

	sigemptyset(&_sigset);

	auto fd = ::signalfd(-1, &_sigset, SFD_CLOEXEC | SFD_NONBLOCK);
	if (fd < 0) {
		return false;
	}

	auto d = (SourceData *)getData();
	d->fd = fd;
	d->event.events = EPOLLIN;
	return true;
}

bool SignalFdSource::read() {
	auto s = ::read(((SourceData *)getData())->fd, &_info, sizeof(signalfd_siginfo));
	if (s == sizeof(signalfd_siginfo)) {
		process();
		return true;
	}
	return false;
}

bool SignalFdSource::process(bool panding) {
	if (_info.ssi_signo == 0) {
		return false;
	}

	auto sig = getSignalInfo(_info.ssi_signo);
	if (sig) {
		log::info("event::Queue", "Signal intercepted with signalfd: ", sig->name, ", errno: ", _info.ssi_errno);
	} else {
		log::info("event::Queue", "Signal intercepted with signalfd: (unknown), errno: ", _info.ssi_errno);
	}

	sigset_t sigset;
	::sigemptyset(&sigset);
	::sigpending(&sigset);
	if (!sigisemptyset(&sigset) && ::sigismember(&sigset, _info.ssi_signo)) {
		::sigemptyset(&sigset);
		::sigaddset(&sigset, _info.ssi_signo);
		::sigsuspend(&sigset);
	}

	memset(&_info, 0, sizeof(signalfd_siginfo));

	return true;
}

void SignalFdSource::enable() {
	sigset_t sigset;
	::sigemptyset(&sigset);
	::sigprocmask(SIG_UNBLOCK, nullptr, &sigset);

	enable(&sigset);
}

void SignalFdSource::enable(const sigset_t *sigset) {
	memcpy(&_sigset, sigset, sizeof(sigset_t));

	for (auto &it : _extra) {
		::sigaddset(&_sigset, it);
	}

	::sigfillset(&_sigset);

	mem_std::StringStream signals;
	for (size_t i = 0; i < sizeof(siglist) / sizeof(siginfo); i++) {
		if (::sigismember(&_sigset, siglist[i].code)) {
			signals << " " << siglist[i].name;
		}
	}

	log::debug("event::Queue", "signalfd enabled:", signals.str());

	::signalfd(((SourceData *)getData())->fd, &_sigset, SFD_NONBLOCK | SFD_CLOEXEC);
}

void SignalFdSource::disable() {
	::sigemptyset(&_sigset);
	::signalfd(((SourceData *)getData())->fd, &_sigset, SFD_NONBLOCK | SFD_CLOEXEC);
}

}


#endif
