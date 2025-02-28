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

#ifndef CORE_EVENT_SPEVENT_H_
#define CORE_EVENT_SPEVENT_H_

#include "SPMemory.h"
#include "SPRef.h"
#include "SPPlatform.h"

namespace STAPPLER_VERSIONIZED stappler::event {

class Queue;
class Handle;
class OpHandle;
class InputOutputHandle;
class DirHandle;
class FileHandle;
class TimerHandle;

class BufferChain;

static constexpr int STATUS_ERRNO_OFFSET = 0xFFFF;

static constexpr inline int ERROR_NUMBER(int __errno) {
	return - STATUS_ERRNO_OFFSET - __errno;
}

enum class Status : int {
	// general return values
	Ok = 0,
	Declined = -1,
	Done = -2,
	Suspended = -3,

	// errors
	ErrorNumber =			ERROR_NUMBER(0),
	ErrorNotPermitted =		ERROR_NUMBER(1), // EPERM
	ErrorAgain =			ERROR_NUMBER(11), // EAGAIN
	ErrorBusy =				ERROR_NUMBER(16), // EBUSY
	ErrorInvalidArguemnt =	ERROR_NUMBER(22), // EINVAL
	ErrorNotImplemented =	ERROR_NUMBER(38), // ENOSYS
	ErrorAlreadyPerformed =	ERROR_NUMBER(114), // EALREADY
	ErrorCancelled =		ERROR_NUMBER(125), // ECANCELED
};

inline bool isSuccessful(Status st) {
	switch (st) {
	case Status::Ok:
	case Status::Done:
	case Status::Suspended:
		return true;
		break;
	default:
		break;
	}
	return false;
}

inline int toErrno(Status st) {
	if (toInt(st) > toInt(Status::ErrorNumber)) {
		return 0;
	}

	return -(toInt(st) - toInt(Status::ErrorNumber));
}

std::ostream &operator<<(std::ostream &, Status);

enum class ErrorFlags {
	None,
	GenericError = 1 << 0,
	HangUp = 1 << 1,
	StreamClosed = 1 << 2,
};

SP_DEFINE_ENUM_AS_MASK(ErrorFlags)

enum class FileOpenFlags {
	None,
	Read = 1 << 0,
	Write = 1 << 1,
	Create = 1 << 2,
	Append = 1 << 3,
	Truncate = 1 << 4,
	CreateExclusive = 1 << 5,
	DelOnClose = 1 << 6,
};

SP_DEFINE_ENUM_AS_MASK(FileOpenFlags)

enum class FileProtFlags {
	UserSetId = 0x8000,
	UserRead = 0x0400,
	UserWrite = 0x0200,
	UserExecute = 0x0100,
	GroupSetId = 0x4000,
	GroupRead = 0x0040,
	GroupWrite = 0x0020,
	GroupExecute = 0x0010,
	AllRead = 0x0004,
	AllWrite = 0x0002,
	AllExecute = 0x0001,
	Default = 0x0FFF,
};

SP_DEFINE_ENUM_AS_MASK(FileProtFlags)

template <typename Result = Handle>
struct CompletionHandle {
	using Fn = void (*) (void *, Result *, uint32_t value, Status);

	template <typename T>
	static CompletionHandle create(T *ptr, void (*cb) (T *, Result *, uint32_t value, Status)) {
		CompletionHandle ret;
		ret.userdata = reinterpret_cast<void *>(ptr);
		ret.fn = reinterpret_cast<Fn>(cb);
		return ret;
	}

	Fn fn;
	void *userdata;
};

struct TimerInfo {
	using Completion = CompletionHandle<TimerHandle>;

	Completion completion;
	TimeInterval timeout;
	TimeInterval interval;
	uint32_t count = 0;
	ClockType type = ClockType::Default;
};

}

#endif /* CORE_EVENT_SPEVENT_H_ */
