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
class InputOutputHandle;
class FileOpHandle;
class DirHandle;
class StatHandle;
class FileHandle;
class TimerHandle;
class ThreadHandle;

class BufferChain;

using FileType = filesystem::FileType;
using OpenFlags = filesystem::OpenFlags;
using ProtFlags = filesystem::ProtFlags;
using Stat = filesystem::Stat;

template <typename Result = Handle>
struct SP_PUBLIC CompletionHandle {
	using Fn = void (*) (void *, Result *, uint32_t value, Status);

	template <typename T>
	static CompletionHandle create(T *ptr, void (*cb) (T *, Result *, uint32_t value, Status)) {
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

	Fn fn;
	void *userdata;
};

struct SP_PUBLIC TimerInfo {
	static constexpr uint32_t Infinite = maxOf<uint32_t>();

	using Completion = CompletionHandle<TimerHandle>;

	Completion completion;
	TimeInterval timeout;
	TimeInterval interval;
	uint32_t count = 0;
	ClockType type = ClockType::Default;
};

struct SP_PUBLIC FileOpInfo {
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
};

}

#endif /* CORE_EVENT_SPEVENT_H_ */
