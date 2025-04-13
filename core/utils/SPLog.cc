/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023-2025 Stappler LLC <admin@stappler.dev>

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

#if MODULE_STAPPLER_THREADS
#include "SPThread.h"
#endif

#if ANDROID
#include <android/log.h>
#endif

namespace STAPPLER_VERSIONIZED stappler::log {

static const constexpr int MAX_LOG_FUNC = 16;

#ifndef STAPPLER_LOG_LEVEL
#if DEBUG
static std::bitset<6> s_logMask;
#else
static std::bitset<6> s_logMask = (1 | 2 | 4 | 8);
#endif
#else
#if STAPPLER_LOG_LEVEL == 0
// restrict all
static std::bitset<6> s_logMask = (1 | 2 | 4 | 8 | 16 | 32);
#elif STAPPLER_LOG_LEVEL == 1
static std::bitset<6> s_logMask = (1 | 2 | 4 | 8);
#else
// allow all
static std::bitset<6> s_logMask = (0);
#endif
#endif

static void DefaultLog2(LogType type, const StringView &tag, const StringView &text) {
	std::stringstream stream;

#if !ANDROID
	switch (type) {
	case LogType::Verbose: stream << "[Verbose] "; break;
	case LogType::Debug: stream << "[Debug] "; break;
	case LogType::Info: stream << "[Info] "; break;
	case LogType::Warn: stream << "[Warn] "; break;
	case LogType::Error: stream << "[Error] "; break;
	case LogType::Fatal: stream << "[Fatal] "; break;
	}
#endif

#if MODULE_STAPPLER_THREADS
	if (auto local = thread::ThreadInfo::getThreadInfo()) {
		if (!local->managed) {
			stream << "[Thread:" << std::this_thread::get_id() << "] ";
		} else if (local->workerId == thread::ThreadInfo::DetachedWorker) {
			stream << "[" << local->name << "] ";
		} else {
			stream << "[" << local->name << ":" << local->workerId << "] ";
		}
	} else {
		stream << "[Log] ";
	}
#endif

#if !ANDROID
	stream << tag << ": ";
#endif
	stream << text;
#ifndef __apple__
	stream << "\n";
#endif

	auto str = stream.str();

#if ANDROID
	switch (type) {
	case LogType::Verbose: __android_log_print(ANDROID_LOG_VERBOSE, tag.data(), "%s", str.c_str()); break;
	case LogType::Debug: __android_log_print(ANDROID_LOG_DEBUG, tag.data(), "%s", str.c_str()); break;
	case LogType::Info: __android_log_print(ANDROID_LOG_INFO, tag.data(), "%s", str.c_str()); break;
	case LogType::Warn: __android_log_print(ANDROID_LOG_WARN, tag.data(), "%s", str.c_str()); break;
	case LogType::Error: __android_log_print(ANDROID_LOG_ERROR, tag.data(), "%s", str.c_str()); break;
	case LogType::Fatal: __android_log_print(ANDROID_LOG_FATAL, tag.data(), "%s", str.c_str()); break;
	}
#else
	fwrite(str.c_str(), str.length(), 1, stdout);
	fflush(stdout);
#endif // platform switch
}

static void DefaultLog(LogType type, const StringView &tag, CustomLog::Type t, CustomLog::VA &va) {
	if (t == CustomLog::Text) {
		DefaultLog2(type, tag, va.text);
	} else {
		char stackBuf[1_KiB];
		va_list tmpList;
		va_copy(tmpList, va.format.args);
		int size = vsnprintf(stackBuf, size_t(1_KiB - 1), va.format.format, tmpList);
		va_end(tmpList);
		if (size > int(1_KiB - 1)) {
			char *buf = new char[size + 1];
			size = vsnprintf(buf, size_t(size), va.format.format, va.format.args);
			DefaultLog2(type, tag, StringView(buf, size));
			delete [] buf;
		} else if (size >= 0) {
			DefaultLog2(type, tag, StringView(stackBuf, size));
		} else {
			DefaultLog2(type, tag, "Log error");
		}
	}
}

struct CustomLogManager {
	CustomLog::log_fn logFuncArr[MAX_LOG_FUNC] = { 0 };
	std::atomic<int> logFuncCount;
	std::mutex logFuncMutex;

	static CustomLogManager *get() {
		static CustomLogManager ptr;
		return &ptr;
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

	void log(LogType type, const StringView tag, CustomLog::Type t, CustomLog::VA &va) {
		if (s_logMask.test(toInt(type))) {
			return;
		}

		int count = logFuncCount.load();
		if (count == 0) {
			DefaultLog(type, tag, t, va);
		} else {
			bool success = true;
			logFuncMutex.lock();
			count = logFuncCount.load();
			for (int i = 0; i < count; i++) {
				success = logFuncArr[i](type, tag, t, va);
				if (!success) {
					break;
				}
			}
			logFuncMutex.unlock();
			if (success) {
				DefaultLog(type, tag, t, va);
			}
		}
	}
};

CustomLog::CustomLog(log_fn logfn) : fn(logfn) {
	manager = CustomLogManager::get();
	if (fn) {
		reinterpret_cast<CustomLogManager *>(manager)->insert(fn);
	}
}

CustomLog::~CustomLog() {
	if (fn) {
		reinterpret_cast<CustomLogManager *>(manager)->remove(fn);
	}
}

CustomLog::CustomLog(CustomLog && other) : fn(other.fn) {
	manager = CustomLogManager::get();
	other.fn = nullptr;
}

CustomLog& CustomLog::operator=(CustomLog && other) {
	manager = CustomLogManager::get();
	if (fn && fn != other.fn) {
		reinterpret_cast<CustomLogManager *>(manager)->remove(fn);
	}
	fn = other.fn;
	other.fn = nullptr;
	return *this;
}

static std::bitset<6> makeFilterMask(InitializerList<LogType> type) {
	std::bitset<6> mask;
	for (auto &it : type) {
		mask.set(toInt(it));
	}
	return mask;
}

std::bitset<6> None = makeFilterMask({Verbose, Debug, Info, Warn, Error, Fatal});
std::bitset<6> ErrorsOnly = makeFilterMask({Verbose, Debug, Info, Warn});
std::bitset<6> Full = std::bitset<6>(0);

void setLogFilterMask(const std::bitset<6> &mask) {
	s_logMask = mask;
}

void setLogFilterMask(std::bitset<6> &&mask) {
	s_logMask = sp::move(mask);
}

void setLogFilterMask(InitializerList<LogType> types) {
	setLogFilterMask(makeFilterMask(types));
}

std::bitset<6> getlogFilterMask() {
	return s_logMask;
}

void format(LogType type, const char *tag, const char *fmt, ...) {
	CustomLog::VA va;
    va_start(va.format.args, fmt);
    va.format.format = fmt;

    auto m = CustomLogManager::get();
	m->log(type, tag, CustomLog::Format, va);

    va_end(va.format.args);
}

void format(LogType type, const StringView &tag, const char *fmt, ...) {
	CustomLog::VA va;
    va_start(va.format.args, fmt);
    va.format.format = fmt;

    auto m = CustomLogManager::get();
	m->log(type, tag, CustomLog::Format, va);

    va_end(va.format.args);
}

void text(LogType type, const char *tag, const char *text) {
	CustomLog::VA va;
	va.text = StringView(text);
    auto m = CustomLogManager::get();
	m->log(type, tag, CustomLog::Text, va);
}

void text(LogType type, const StringView &tag, const StringView &text) {
	CustomLog::VA va;
	va.text = text;
    auto m = CustomLogManager::get();
	m->log(type, tag, CustomLog::Text, va);
}

}
