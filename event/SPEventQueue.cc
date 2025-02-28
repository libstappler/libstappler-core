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

namespace STAPPLER_VERSIONIZED stappler::event {

Queue::~Queue() {
	if (_data) {
		delete _data;
		_data = nullptr;
	}
}

bool Queue::init(const QueueInfo &info, QueueFlags flags) {
	mem_pool::perform([&] {
		_data = new (getPool()) Data(static_cast<QueueRef *>(getRef()), info, flags);
	}, getPool());
	return _data != nullptr && _data->isValid();
}

Rc<OpHandle> Queue::openDir(CompletionHandle<DirHandle> &&, StringView path) {
	return nullptr;
}

Rc<OpHandle> Queue::openFile(OpenFileInfo &&) {
	return nullptr;
}

Rc<TimerHandle> Queue::scheduleTimer(TimerInfo &&info) {
	if (info.count == 0 || (!info.interval && !info.timeout)) {
		log::error("event::Queue", "Invalid parameters for timer");
		return nullptr;
	}

	auto h = _data->scheduleTimer(move(info));
	_data->runHandle(h);
	return h;
}

Rc<OpHandle> Queue::read(CompletionHandle<void> &&, InputOutputHandle *, BufferChain *, uint32_t, uint32_t) {
	return nullptr;
}
Rc<OpHandle> Queue::write(CompletionHandle<void> &&, InputOutputHandle *, BufferChain *, uint32_t, uint32_t) {
	return nullptr;
}

Status Queue::submitPending() {
	return _data->submit();
}

// non-blocking poll
uint32_t Queue::poll() {
	return _data->poll();
}

// wait until next event or timeout
uint32_t Queue::wait(TimeInterval ival) {
	return _data->wait(ival);
}

// run for some time
Status Queue::run(TimeInterval ival) {
	submitPending();
	return _data->run(ival);
}

void Queue::wakeup(QueueWakeupFlags flags, TimeInterval gracefulTimeout) {
	_data->wakeup(flags, gracefulTimeout);
}

QueueFlags Queue::getFlags() const {
	return _data->_flags;
}

}
