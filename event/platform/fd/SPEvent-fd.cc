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

#include "SPEvent-fd.h"

#if LINUX || ANDROID

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
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

FdSource::~FdSource() {
	cancel();
}

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

SignalFdSource::~SignalFdSource() { }

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

	return FdSource::init(fd);
}

bool SignalFdSource::read() {
	auto s = ::read(getFd(), &_info, sizeof(signalfd_siginfo));
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

	::signalfd(getFd(), &_sigset, SFD_NONBLOCK | SFD_CLOEXEC);
}

void SignalFdSource::disable() {
	::sigemptyset(&_sigset);
	::signalfd(getFd(), &_sigset, SFD_NONBLOCK | SFD_CLOEXEC);
}


EventFdSource::~EventFdSource() { }

bool EventFdSource::init() {
	auto fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (fd < 0) {
		return false;
	}

	return FdSource::init(fd);
}

bool EventFdSource::read() {
	return ::eventfd_read(getFd(), &_value) >= 0;
}

bool EventFdSource::write(uint64_t val) {
	return ::eventfd_write(getFd(), val) >= 0;
}

bool TimerFdSource::init(TimerInfo &&info) {
	__clockid_t clockid = CLOCK_MONOTONIC;

	switch (info.type) {
	case ClockType::Default:
	case ClockType::Monotonic:
		clockid = CLOCK_MONOTONIC;
		break;
	case ClockType::Realtime:
		clockid = CLOCK_REALTIME;
		break;
	case ClockType::Process:
		log::error("event::Queue", "ClockType::Thread is not supported for a timer on this system");
		return false;
		break;
	case ClockType::Thread:
		log::error("event::Queue", "ClockType::Thread is not supported for a timer on this system");
		return false;
		break;
	}

	auto fd = ::timerfd_create(clockid, TFD_NONBLOCK | TFD_CLOEXEC);
	if (fd < 0) {
		log::error("event::Queue", "fail to timerfd_create");
		return false;
	}

	itimerspec spec;
	if (info.timeout) {
		setNanoTimespec(spec.it_value, info.timeout);
	} else {
		setNanoTimespec(spec.it_value, info.interval);
	}

	setNanoTimespec(spec.it_interval, info.interval);

	_count = info.count;

	::timerfd_settime(fd, 0, &spec, nullptr);

	return FdSource::init(fd);
}

bool TimerFdHandle::init(QueueRef *q, QueueData *d, TimerInfo &&info) {
	auto source = new (_data) TimerFdSource;

	if (!source->init(move(info))) {
		return false;
	}

	setCompletion(std::move(info.completion));

	return TimerHandle::init(q, d);
}

Status TimerFdHandle::cancel(Status st) {
	auto source = (TimerFdSource *)getData();
	source->cancel();

	return TimerHandle::cancel(st);
}

}

#endif
