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

#include "SPEventSignalFd.h"
#include "../uring/SPEvent-uring.h"

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

bool SignalFdSource::init(const sigset_t *sigset) {
	auto fd = ::signalfd(-1, sigset, SFD_CLOEXEC | SFD_NONBLOCK);
	if (fd < 0) {
		return false;
	}

	return FdSource::init(fd);
}

bool SignalFdHandle::init(QueueRef *q, QueueData *d, SpanView<int> sigs) {
	static_assert(sizeof(SignalFdSource) <= DataSize && std::is_standard_layout<SignalFdSource>::value);

	sigemptyset(&_sigset);

	auto source = new (_data) SignalFdSource();
	if (!source->init(&_sigset)) {
		return false;
	}

	_extra = sigs.vec<memory::StandartInterface>();
	return true;
}

bool SignalFdHandle::read() {
	auto s = ::read(getFd(), &_info, sizeof(signalfd_siginfo));
	if (s == sizeof(signalfd_siginfo)) {
		process();
		return true;
	}
	return false;
}

bool SignalFdHandle::process() {
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

void SignalFdHandle::enable() {
	sigset_t sigset;
	::sigemptyset(&sigset);
	::sigprocmask(SIG_UNBLOCK, nullptr, &sigset);

	enable(&sigset);
}

void SignalFdHandle::enable(const sigset_t *sigset) {
	memcpy(&_sigset, sigset, sizeof(sigset_t));

	for (auto &it : _extra) {
		::sigaddset(&_sigset, it);
	}

	mem_std::StringStream signals;
	for (size_t i = 0; i < sizeof(siglist) / sizeof(siginfo); i++) {
		if (::sigismember(&_sigset, siglist[i].code)) {
			signals << " " << siglist[i].name;
		}
	}

	log::debug("event::Queue", "signalfd enabled:", signals.str());

	::signalfd(getFd(), &_sigset, SFD_NONBLOCK | SFD_CLOEXEC);
}

void SignalFdHandle::disable() {
	::sigemptyset(&_sigset);
	::signalfd(getFd(), &_sigset, SFD_NONBLOCK | SFD_CLOEXEC);
}

bool SignalFdURingHandle::init(URingData *uring, SpanView<int> sigs) {
	if (!SignalFdHandle::init(uring->_queue, uring->_data, sigs)) {
		return false;
	}

	setupURing<SignalFdURingHandle, SignalFdSource>(uring, this);

	return true;
}

Status SignalFdURingHandle::rearm(SignalFdSource *source) {
	auto status = prepareRearm();
	if (status == Status::Ok) {
		status = source->getURingData()->pushRead(source->getFd(), (uint8_t *)&_info, sizeof(signalfd_siginfo), this);
		if (status == Status::Suspended) {
			status = source->getURingData()->submit();
		}
	}
	return status;
}

Status SignalFdURingHandle::disarm(SignalFdSource *source, bool suspend) {
	auto status = prepareDisarm(suspend);
	if (status == Status::Ok) {
		status = source->getURingData()->cancelOp(this, URingUserFlags::None,
				suspend ? URingCancelFlags::Suspend : URingCancelFlags::None);
	}
	return status;
}

void SignalFdURingHandle::notify(SignalFdSource *source, int32_t res, uint32_t flags, URingUserFlags uflags) {
	if (_status != Status::Ok) {
		return;
	}

	_status = Status::Suspended;

	if (res == sizeof(signalfd_siginfo)) {
		process();
		if (_status == Status::Suspended) {
			rearm(source);
		}
	} else{
		cancel(URingData::getErrnoStatus(res));
	}
}

}
