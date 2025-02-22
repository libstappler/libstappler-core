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
#include "SPEventPlatform.h"

namespace STAPPLER_VERSIONIZED stappler::event {

enum class Status : int {
	Ok = 0,
	Declined = -1,
	Done = -2,
	Suspended = -3,
	Invalid = -4,
	NotImplemented = -5,
};

enum class ErrorFlags {
	None,
	GenericError = 1 << 0,
	HangUp = 1 << 1,
	StreamClosed = 1 << 2,
};

SP_DEFINE_ENUM_AS_MASK(ErrorFlags)

enum class Type {
	None,
	Read = 1 << 0,
	Write = 1 << 1,
	Shutdown = 1 << 2,
};

SP_DEFINE_ENUM_AS_MASK(Type)

}

#endif /* CORE_EVENT_SPEVENT_H_ */
