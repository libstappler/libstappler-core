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

#ifndef CORE_EVENT_PLATFORM_WINDOWS_SPEVENTTHREADIOCP_H_
#define CORE_EVENT_PLATFORM_WINDOWS_SPEVENTTHREADIOCP_H_

#include "SPEventThreadHandle.h"
#include "SPEvent-iocp.h"

namespace STAPPLER_VERSIONIZED stappler::event {

static constexpr bool IOCP_THREAD_NONBLOCK = false;

struct SP_PUBLIC ThreadIocpSource {
	HANDLE currentThread = nullptr;
	HANDLE port = nullptr;

	bool init();
	void cancel();
};

class SP_PUBLIC ThreadIocpHandle : public ThreadHandle {
public:
	virtual ~ThreadIocpHandle() = default;

	bool init(HandleClass *);

	Status rearm(IocpData *, ThreadIocpSource *);
	Status disarm(IocpData *, ThreadIocpSource *);

	void notify(IocpData *, ThreadIocpSource *, const NotifyData &);

	virtual Status perform(Rc<thread::Task> &&task) override;
	virtual Status perform(mem_std::Function<void()> &&func, Ref *target, StringView tag) override;

protected:
	std::mutex _mutex;
};

}

#endif /* CORE_EVENT_PLATFORM_WINDOWS_SPEVENTTHREADIOCP_H_ */
