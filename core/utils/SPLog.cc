/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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

#include "SPLog.h"

namespace stappler::log {

static const constexpr int MAX_LOG_FUNC = 16;

static void DefaultLog2(const StringView &tag, const StringView &text) {
#if MSYS
	std::cout << '[' << tag << "] " << text << '\n';
	std::cout.flush();
#else
	std::cerr << '[' << tag << "] " << text << '\n';
	std::cerr.flush();
#endif
}

static void DefaultLog(const StringView &tag, CustomLog::Type t, CustomLog::VA &va) {
	if (t == CustomLog::Text) {
		DefaultLog2(tag, va.text);
	} else {
		char stackBuf[1_KiB];
		va_list tmpList;
		va_copy(tmpList, va.format.args);
		int size = vsnprintf(stackBuf, size_t(1_KiB - 1), va.format.format, tmpList);
		va_end(tmpList);
		if (size > int(1_KiB - 1)) {
			char *buf = new char[size + 1];
			size = vsnprintf(buf, size_t(size), va.format.format, va.format.args);
			DefaultLog2(tag, StringView(buf, size));
			delete [] buf;
		} else if (size >= 0) {
			DefaultLog2(tag, StringView(stackBuf, size));
		} else {
			DefaultLog2(tag, "Log error");
		}
	}
}

struct CustomLogManager : RefBase<memory::StandartInterface> {
	CustomLog::log_fn logFuncArr[MAX_LOG_FUNC] = { 0 };
	std::atomic<int> logFuncCount;
	std::mutex logFuncMutex;

	static Rc<CustomLogManager> get() {
		static std::mutex s_mutex;
		static Rc<CustomLogManager> ptr;

		std::unique_lock<std::mutex> lock(s_mutex);
		if (!ptr) {
			ptr = Rc<CustomLogManager>::alloc();
		}
		return ptr;
	}

	void insert(CustomLog::log_fn fn) {
		logFuncMutex.lock();
		if (logFuncCount.load() < MAX_LOG_FUNC) {
			logFuncArr[logFuncCount] = fn;
			++ logFuncCount;
		}
		logFuncMutex.unlock();
	}

	void remove(CustomLog::log_fn fn) {
		logFuncMutex.lock();
		int count = logFuncCount.load();
		for (int i = 0; i < count; i++) {
			if (logFuncArr[i] == fn) {
				if (i != count - 1) {
					memmove(&logFuncArr[i], &logFuncArr[i + 1], (count - i - 1) * sizeof(CustomLog::log_fn));
				}
				-- logFuncCount;
				break;
			}
		}
		logFuncMutex.unlock();
	}

	void log(const StringView tag, CustomLog::Type t, CustomLog::VA &va) {
		int count = logFuncCount.load();
		if (count == 0) {
			DefaultLog(tag, t, va);
		} else {
			logFuncMutex.lock();
			count = logFuncCount.load();
			for (int i = 0; i < count; i++) {
				logFuncArr[i](tag, t, va);
			}
			logFuncMutex.unlock();
		}
	}
};

CustomLog::CustomLog(log_fn fn) : fn(fn) {
	manager = CustomLogManager::get();
	if (fn) {
		((CustomLogManager *)manager.get())->insert(fn);
	}
}

CustomLog::~CustomLog() {
	if (fn) {
		((CustomLogManager *)manager.get())->remove(fn);
	}
}

CustomLog::CustomLog(CustomLog && other) : fn(other.fn) {
	other.fn = nullptr;
}
CustomLog& CustomLog::operator=(CustomLog && other) {
	fn = other.fn;
	other.fn = nullptr;
	return *this;
}

void format(const StringView &tag, const char *fmt, ...) {
	CustomLog::VA va;
    va_start(va.format.args, fmt);
    va.format.format = fmt;

    auto m = CustomLogManager::get();
	m->log(tag, CustomLog::Format, va);

    va_end(va.format.args);
}

void text(const StringView &tag, const StringView &text) {
	CustomLog::VA va;
	va.text = text;
    auto m = CustomLogManager::get();
	m->log(tag, CustomLog::Text, va);
}

}
