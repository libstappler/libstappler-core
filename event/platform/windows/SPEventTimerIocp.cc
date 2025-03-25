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

#include "SPEventTimerIocp.h"
#include "SPStatus.h"
#include "platform/windows/SPEvent-iocp.h"
#include <errhandlingapi.h>
#include <synchapi.h>

namespace STAPPLER_VERSIONIZED stappler::event {

static void timeToFileTime(LARGE_INTEGER &ftime, TimeInterval ival) {
	// ticks in 100ns
	auto ticks = ival.toMicros() * 10;
	ftime.QuadPart = - ticks;
}

bool TimerIocpSource::init(const TimerInfo &info) {
	handle = CreateWaitableTimerEx(0, 0, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	if (!handle) {
		return false;
	}

	interval = info.interval;
	count = info.count;

	BOOL result = 1;
	LARGE_INTEGER dueDate;

	if (interval.toMicros() < 1000) {
		subintervals = true;
	}

	if (info.count == 1) {
		// oneshot timer
		timeToFileTime(dueDate, info.timeout);
		result = SetWaitableTimerEx (handle, &dueDate, 0, 0, 0, 0, 0);
	} else if (!subintervals) {
		timeToFileTime(dueDate, info.timeout);
		result = SetWaitableTimerEx (handle, &dueDate, interval.toMillis(), 0, 0, 0, 0);
	} else {
		result = SetWaitableTimerEx (handle, &dueDate, 0, 0, 0, 0, 0);
	}
	if (!result) {
		log::error("event::Queue", "Fail to create WaitableTimer: ", status::lastErrorToStatus(GetLastError()));
	}
	return result;
}

void TimerIocpSource::reset() {
	LARGE_INTEGER dueDate;
	if (subintervals && handle) {
		timeToFileTime(dueDate, interval);
		SetWaitableTimer(handle, &dueDate, 0, 0, 0, 0);
	}
}

void TimerIocpSource::cancel() {
	if (handle) {
		CancelWaitableTimer(handle);
		handle = nullptr;
	}	
}

bool TimerIocpHandle::init(HandleClass *cl, TimerInfo &&info) {
	static_assert(sizeof(TimerIocpSource) <= DataSize && std::is_standard_layout<TimerIocpSource>::value);

	if (!TimerHandle::init(cl, info.completion)) {
		return false;
	}

	if (info.count == 1) {
		info.interval = info.timeout;
	} else if (!info.timeout) {
		info.timeout = info.interval;
	}

	auto source = new (_data) TimerIocpSource();
	return source->init(info);
}

Status TimerIocpHandle::rearm(IocpData *iocp, TimerIocpSource *source) {
	auto status = prepareRearm();
	if (status == Status::Ok) {
		if (!source->event) {
			source->event = ReportEventAsCompletion(iocp->_port, source->handle, 0,
				reinterpret_cast<uintptr_t>(this), nullptr);
				if (!source->event) {
					return status::lastErrorToStatus(GetLastError());
				}
		} else {
			if (!RestartEventCompletion(source->event, iocp->_port, source->handle,0, reinterpret_cast<uintptr_t>(this), nullptr)) {
				return status::lastErrorToStatus(GetLastError());
			}
		}
	}
	return status;
}

Status TimerIocpHandle::disarm(IocpData *iocp, TimerIocpSource *source) {
	auto status = prepareDisarm();
	if (status == Status::Ok) {
		if (source->event) {
			CancelEventCompletion(source->event, true);
			source->event = nullptr;
		}
		++ _timeline;
	} else if (status == Status::ErrorAlreadyPerformed) {
		return Status::Ok;
	}
	return status;
}

void TimerIocpHandle::notify(IocpData *iocp, TimerIocpSource *source, const NotifyData &data) {
	if (_status != Status::Ok) {
		return;
	}

	// event handling is suspended when we receive notification
	_status = Status::Suspended;

	auto count = source->count;
	auto current = source->value;

	bool isFirst = (current == 0);

	++ current;
	source->value = current;

	if (count == TimerInfo::Infinite || current < count) {
		if (source->subintervals) {
			source->reset();
		}

		rearm(iocp, source);
	} else {
		cancel(Status::Done, source->value);
	}

	sendCompletion(current, _status == Status::Suspended ? Status::Ok : _status);
}

}
