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
#include "SPFilesystem.h"
#include "SPStatus.h"

namespace STAPPLER_VERSIONIZED stappler::event {

class Looper;
class Queue;
class Handle;

class TimerHandle;
class ThreadHandle;
class PollHandle;

struct BufferChain;

#if WIN32
using NativeHandle = void *;
#else
using NativeHandle = int;
#endif

using OpenFlags = filesystem::OpenFlags;
using ProtFlags = filesystem::ProtFlags;
using Stat = filesystem::Stat;

enum class PollFlags : uint16_t {
	None = 0,
	In = 0x001, /* There is data to read.  */
	Pri = 0x002, /* There is urgent data to read.  */
	Out = 0x004, /* Writing now will not block.  */
	Err = 0X008, /* ERROR CONDITION.  */
	HungUp = 0X010, /* HUNG UP.  */
	Invalid = 0X020, /* INVALID POLLING REQUEST.  */

	PollMask = 0x3FFF,
	CloseFd = 0x4000,
	AllowMulti = 0x8000, // Allow edge-triggered multishot setups
};

SP_DEFINE_ENUM_AS_MASK(PollFlags)

template <typename Result = Handle>
struct SP_PUBLIC CompletionHandle {
	using Fn = void (*)(void *, Result *, uint32_t value, Status);

	template <typename T>
	static CompletionHandle create(T *ptr, void (*cb)(T *, Result *, uint32_t value, Status)) {
		CompletionHandle ret;
		ret.userdata = reinterpret_cast<void *>(ptr);
		ret.fn = reinterpret_cast<Fn>(cb);
		return ret;
	}

	template <typename Other>
	CompletionHandle &operator=(const CompletionHandle<Other> &other) {
		fn = reinterpret_cast<Fn>(other.fn);
		userdata = other.userdata;
		return *this;
	}

	template <typename Other>
	operator CompletionHandle<Other>() const {
		CompletionHandle<Other> ret;
		ret.fn = reinterpret_cast<typename CompletionHandle<Other>::Fn>(fn);
		ret.userdata = userdata;
		return ret;
	}

	operator bool() const { return fn; }

	Fn fn = nullptr;
	void *userdata = nullptr;
};

struct SP_PUBLIC TimerInfo {
	static constexpr uint32_t Infinite = maxOf<uint32_t>();

	using Completion = CompletionHandle<TimerHandle>;

	Completion completion;
	TimeInterval timeout;
	TimeInterval interval;
	uint32_t count = 0;

	// ClockType for a timer only partially usable on non-Linux systems
	// Just leave it Default
	ClockType type = ClockType::Default;

	// Set this = true if you want to use `TimerHandle::reset`.
	//
	// Without this flag, you can still call `TimerHandle::reset`, but it can be
	// only partially available, some variants of TimerInfo can be rejected
	// (`TimerHandle::reset` returns `false`)
	//
	// Note: that resetable timer CAN be less performant then regular as a timer,
	// but 'reset' for this timer can save some syscalls and kernel resources
	bool resetable = false;
};

/*struct SP_PUBLIC FileOpInfo {
	DirHandle *root = nullptr;
	StringView path;
};

struct SP_PUBLIC OpenDirInfo {
	using Completion = CompletionHandle<DirHandle>;

	Completion completion;
	FileOpInfo file;
};

struct SP_PUBLIC StatOpInfo {
	using Completion = CompletionHandle<StatHandle>;

	Completion completion;
	FileOpInfo file;
};

struct SP_PUBLIC OpenFileInfo {
	using Completion = CompletionHandle<FileHandle>;

	Completion completion;
	DirHandle *dir = nullptr;
	StringView path;
	OpenFlags flags = OpenFlags::None;
	ProtFlags prot = ProtFlags::None;
};*/

} // namespace stappler::event

#endif /* CORE_EVENT_SPEVENT_H_ */
