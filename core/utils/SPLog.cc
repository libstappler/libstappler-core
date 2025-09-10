/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023-2025 Stappler LLC <admin@stappler.dev>
Copyright (c) 2025 Stappler Team <admin@stappler.org>

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

#if LINUX
#include <sys/stat.h>
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

struct LogFeatures {
	enum Features : uint32_t {
		None,
		AnsiCompatible = 1 << 0,
		Colors = 1 << 1,
		Bold = 1 << 2,
		Underline = 1 << 3,
		Italic = 1 << 4,
		Reverse = 1 << 5,
		Dim = 1 << 6,
	};

	char tmpbuf[256] = {0};
	char *tmpTarget = nullptr;

	Features features;
	uint32_t ncolors = 0;
	StringView drop;
	StringView bold;
	StringView underline;
	StringView italic;
	StringView reverse;
	StringView dim;

	StringView fblack;
	StringView fred;
	StringView fgreen;
	StringView fyellow;
	StringView fblue;
	StringView fmagenta;
	StringView fcyan;
	StringView fwhite;
	StringView fdef;

	StringView bblack;
	StringView bred;
	StringView bgreen;
	StringView byellow;
	StringView bblue;
	StringView bmagenta;
	StringView bcyan;
	StringView bwhite;
	StringView bdef;

	LogFeatures() {
		memset(tmpbuf, 0, 256);
		tmpTarget = tmpbuf;
	}

	StringView copy(StringView str) {
		::memcpy(tmpTarget, str.data(), str.size());
		StringView ret(tmpTarget, str.size());
		tmpTarget += str.size();
		return ret;
	}
};

SP_DEFINE_ENUM_AS_MASK(LogFeatures::Features)

struct CustomLogManager {
	static void initialize(void *ptr) { reinterpret_cast<CustomLogManager *>(ptr)->init(); }
	static void terminate(void *ptr) { reinterpret_cast<CustomLogManager *>(ptr)->term(); }

	CustomLog::log_fn logFuncArr[MAX_LOG_FUNC] = {0};
	std::atomic<int> logFuncCount;
	std::mutex logFuncMutex;

	LogFeatures features;

	CustomLogManager();

	void init();
	void term();

	void insert(CustomLog::log_fn fn);
	void remove(CustomLog::log_fn fn);
	void log(LogType type, StringView tag, const SourceLocation &source, CustomLog::Type t,
			CustomLog::VA &va);
};

static CustomLogManager s_logManager;

static void DefaultLog2(LogType type, StringView tag, const SourceLocation &source,
		StringView text) {
	std::stringstream stream;

#if !ANDROID
	stream << s_logManager.features.reverse;
	switch (type) {
	case LogType::Verbose:
		stream << s_logManager.features.fcyan << "[V]" << s_logManager.features.fdef;
		break;
	case LogType::Debug:
		stream << s_logManager.features.fblue << "[D]" << s_logManager.features.fdef;
		break;
	case LogType::Info:
		stream << s_logManager.features.fgreen << "[I]" << s_logManager.features.fdef;
		break;
	case LogType::Warn:
		stream << s_logManager.features.fyellow << "[W]" << s_logManager.features.fdef;
		break;
	case LogType::Error:
		stream << s_logManager.features.fred << "[E]" << s_logManager.features.fdef;
		break;
	case LogType::Fatal:
		stream << s_logManager.features.fred << "[F]" << s_logManager.features.fdef;
		break;
	}
	stream << s_logManager.features.drop;
#endif

#if MODULE_STAPPLER_THREADS
	stream << s_logManager.features.italic;
	if (auto local = thread::ThreadInfo::getThreadInfo()) {
		if (!local->managed) {
			stream << "[Thread:" << std::this_thread::get_id() << "]";
		} else if (local->workerId == thread::ThreadInfo::DetachedWorker) {
			stream << "[" << local->name << "]";
		} else {
			stream << "[" << local->name << ":" << local->workerId << "]";
		}
	} else {
		stream << "[Log]";
	}
	stream << s_logManager.features.drop << " ";
#endif

#if !ANDROID
	stream << tag << ": ";
#endif

	stream << text;

#if SP_SOURCE_DEBUG
	if (!source.empty()) {
		stream << " " << s_logManager.features.underline << s_logManager.features.dim
			   << source.fileName << ":" << source.line << s_logManager.features.drop;
	}
#endif

#ifndef __apple__
	stream << "\n";
#endif

	auto str = stream.str();

#if ANDROID
	switch (type) {
	case LogType::Verbose:
		__android_log_print(ANDROID_LOG_VERBOSE, tag.data(), "%s", str.c_str());
		break;
	case LogType::Debug:
		__android_log_print(ANDROID_LOG_DEBUG, tag.data(), "%s", str.c_str());
		break;
	case LogType::Info: __android_log_print(ANDROID_LOG_INFO, tag.data(), "%s", str.c_str()); break;
	case LogType::Warn: __android_log_print(ANDROID_LOG_WARN, tag.data(), "%s", str.c_str()); break;
	case LogType::Error:
		__android_log_print(ANDROID_LOG_ERROR, tag.data(), "%s", str.c_str());
		break;
	case LogType::Fatal:
		__android_log_print(ANDROID_LOG_FATAL, tag.data(), "%s", str.c_str());
		break;
	}
#else
	fwrite(str.c_str(), str.length(), 1, stdout);
	fflush(stdout);
#endif // platform switch
}

static void DefaultLog(LogType type, StringView tag, const SourceLocation &source,
		CustomLog::Type t, CustomLog::VA &va) {
	if (t == CustomLog::Text) {
		DefaultLog2(type, tag, source, va.text);
	} else {
		char stackBuf[1_KiB];
		va_list tmpList;
		va_copy(tmpList, va.format.args);
		int size = ::vsnprintf(stackBuf, size_t(1_KiB - 1), va.format.format, tmpList);
		va_end(tmpList);
		if (size > int(1_KiB - 1)) {
			char *buf = new char[size + 1];
			size = ::vsnprintf(buf, size_t(size), va.format.format, va.format.args);
			DefaultLog2(type, tag, source, StringView(buf, size));
			delete[] buf;
		} else if (size >= 0) {
			DefaultLog2(type, tag, source, StringView(stackBuf, size));
		} else {
			DefaultLog2(type, tag, source, "Log error");
		}
	}
}

CustomLogManager::CustomLogManager() { addInitializer(this, initialize, terminate); }

#if LINUX

static bool parseLogFeatures(BytesView data, LogFeatures &ret) {
	static constexpr uint32_t numbers_max_colors = 13;
	static constexpr uint16_t strings_enter_bold_mode = 27;
	static constexpr uint16_t strings_enter_dim_mode = 30;
	static constexpr uint16_t string_exit_attribute_mode = 39;
	static constexpr uint16_t strings_enter_reverse_mode = 34;
	static constexpr uint16_t strings_enter_underline_mode = 36;
	static constexpr uint16_t strings_enter_italics_mode = 311;

	StringView names;
	SpanView<int8_t> bools;
	SpanView<int16_t> numbers16;
	SpanView<int32_t> numbers32;
	SpanView<uint16_t> stringOffsets;
	BytesView strings;

	uint32_t offset = 0;
	bool useI32 = false;
	auto header = data.readSpan<uint16_t>(6);
	if (header.at(0) == 0x021e) {
		useI32 = true;
	} else if (header.at(0) != 0x011A) {
		return false;
	}

	auto namesBytes = header.at(1) == maxOf<uint16_t>() ? 0 : header.at(1);
	auto boolsBytes = header.at(2) == maxOf<uint16_t>() ? 0 : header.at(2);
	auto numbersCount = header.at(3) == maxOf<uint16_t>() ? 0 : header.at(3);
	auto stringOffsetsCount = header.at(4) == maxOf<uint16_t>() ? 0 : header.at(4);
	auto stringTableBytes = header.at(5) == maxOf<uint16_t>() ? 0 : header.at(5);

	names = BytesView(data).readString(namesBytes);
	offset += namesBytes;

	bools = BytesView(data, offset, boolsBytes).readSpan<int8_t>(boolsBytes);
	offset += boolsBytes;

	if (offset % 2 != 0) {
		++offset;
	}

	if (useI32) {
		numbers32 = BytesView(data, offset, numbersCount * sizeof(int32_t))
							.readSpan<int32_t>(numbersCount);
		offset += numbersCount * sizeof(int32_t);
	} else {
		numbers16 = BytesView(data, offset, numbersCount * sizeof(int16_t))
							.readSpan<int16_t>(numbersCount);
		offset += numbersCount * sizeof(int16_t);
	}

	stringOffsets = BytesView(data, offset, stringOffsetsCount * sizeof(uint16_t))
							.readSpan<uint16_t>(stringOffsetsCount);
	offset += stringOffsetsCount * sizeof(uint16_t);

	strings = BytesView(data, offset, stringTableBytes);
	offset += stringTableBytes;

	if (data.size() < offset) {
		return false;
	}

	auto readInteger = [&](uint16_t index) -> int32_t {
		if (index > numbersCount) {
			return 0;
		}
		if (useI32) {
			return numbers32.at(index);
		} else {
			return numbers16.at(index);
		}
	};

	// not used for now, maybe layer?
	SP_UNUSED auto readBool = [&](uint16_t index) -> bool {
		if (index > boolsBytes) {
			return false;
		}
		return bools[index] > 0;
	};

	auto readString = [&](uint16_t index) -> StringView {
		if (index > stringOffsetsCount) {
			return StringView();
		}
		auto offset = stringOffsets[index];
		if (offset < strings.size() && offset != maxOf<uint16_t>()) {
			auto d = strings;
			d += offset;
			return d.readString();
		}

		return StringView();
	};

	auto ncolors = readInteger(numbers_max_colors);
	auto bold = readString(strings_enter_bold_mode);
	auto dim = readString(strings_enter_dim_mode);
	auto underline = readString(strings_enter_underline_mode);
	auto italic = readString(strings_enter_italics_mode);
	auto reverse = readString(strings_enter_reverse_mode);
	auto drop = readString(string_exit_attribute_mode);

	if (!drop.empty()) {
		ret.drop = ret.copy(drop);
		if (ret.drop.starts_with("\033(")) {
			ret.features |= LogFeatures::AnsiCompatible;
		}
	}
	if (ncolors >= 8) {
		ret.features |= LogFeatures::Colors;
		ret.ncolors = ncolors;

		if (hasFlag(ret.features, LogFeatures::AnsiCompatible)) {
			ret.fblack = StringView("\033[30m");
			ret.fred = StringView("\033[31m");
			ret.fgreen = StringView("\033[32m");
			ret.fyellow = StringView("\033[33m");
			ret.fblue = StringView("\033[34m");
			ret.fmagenta = StringView("\033[35m");
			ret.fcyan = StringView("\033[36m");
			ret.fwhite = StringView("\033[37m");
			ret.fdef = StringView("\033[39m");

			ret.bblack = StringView("\033[40m");
			ret.bred = StringView("\033[41m");
			ret.bgreen = StringView("\033[42m");
			ret.byellow = StringView("\033[43m");
			ret.bblue = StringView("\033[44m");
			ret.bmagenta = StringView("\033[45m");
			ret.bcyan = StringView("\033[46m");
			ret.bwhite = StringView("\033[47m");
			ret.bdef = StringView("\033[49m");
		}
	}
	if (!bold.empty() && !drop.empty()) {
		ret.features |= LogFeatures::Bold;
		ret.bold = ret.copy(bold);
	}
	if (!italic.empty() && !drop.empty()) {
		ret.features |= LogFeatures::Italic;
		ret.italic = ret.copy(italic);
	}
	if (!underline.empty() && !drop.empty()) {
		ret.features |= LogFeatures::Underline;
		ret.underline = ret.copy(underline);
	}
	if (!reverse.empty() && !drop.empty()) {
		ret.features |= LogFeatures::Reverse;
		ret.reverse = ret.copy(reverse);
	}
	if (!dim.empty() && !drop.empty()) {
		ret.features |= LogFeatures::Dim;
		ret.dim = ret.copy(dim);
	}

	return true;
}

static bool checkLogFeatureWithFilename(StringView str, LogFeatures &ret) {
	bool result = false;
	struct stat s;
	auto r = ::stat(str.data(), &s);
	if (r == 0 && s.st_size > 0) {
		uint8_t buf[s.st_size];
		auto f = fopen(str.data(), "r");
		if (f) {
			if (fread(buf, s.st_size, 1, f) > 0) {
				result = parseLogFeatures(BytesView(buf, s.st_size), ret);
			}
			fclose(f);
		}
	}
	return result;
}

static void checkLogFeaturesSupport(LogFeatures &result) {
	char buf[512] = {0};
	auto envTerm = ::getenv("TERM");
	auto envTermInfo = ::getenv("TERMINFO");
	if (envTerm) {
		StringView path;

		if (envTermInfo) {
			::memset(buf, 0, 512);
			path = StringView(buf,
					string::toStringBuffer(buf, 512, envTermInfo, "/", *envTerm, "/", envTerm)
							.getValue());
			if (!path.empty()) {
				if (checkLogFeatureWithFilename(path, result)) {
					return;
				}
			}
		}

		::memset(buf, 0, 512);
		path = StringView(buf,
				string::toStringBuffer(buf, 512, "/etc/terminfo/", *envTerm, "/", envTerm)
						.getValue());
		if (!path.empty()) {
			if (checkLogFeatureWithFilename(path, result)) {
				return;
			}
		}

		::memset(buf, 0, 512);
		path = StringView(buf,
				string::toStringBuffer(buf, 512, "/usr/lib/terminfo/", *envTerm, "/", envTerm)
						.getValue());
		if (!path.empty()) {
			if (checkLogFeatureWithFilename(path, result)) {
				return;
			}
		}

		::memset(buf, 0, 512);
		path = StringView(buf,
				string::toStringBuffer(buf, 512, "/usr/share/terminfo/", *envTerm, "/", envTerm)
						.getValue());
		if (!path.empty()) {
			if (checkLogFeatureWithFilename(path, result)) {
				return;
			}
		}
	}
}
#endif

void CustomLogManager::init() { checkLogFeaturesSupport(features); }

void CustomLogManager::term() { }

void CustomLogManager::insert(CustomLog::log_fn fn) {
	logFuncMutex.lock();
	if (logFuncCount.load() < MAX_LOG_FUNC) {
		logFuncArr[logFuncCount] = fn;
		++logFuncCount;
	}
	logFuncMutex.unlock();
}

void CustomLogManager::remove(CustomLog::log_fn fn) {
	logFuncMutex.lock();
	int count = logFuncCount.load();
	for (int i = 0; i < count; i++) {
		if (logFuncArr[i] == fn) {
			if (i != count - 1) {
				memmove(&logFuncArr[i], &logFuncArr[i + 1],
						(count - i - 1) * sizeof(CustomLog::log_fn));
			}
			--logFuncCount;
			break;
		}
	}
	logFuncMutex.unlock();
}

void CustomLogManager::log(LogType type, StringView tag, const SourceLocation &source,
		CustomLog::Type t, CustomLog::VA &va) {
	if (s_logMask.test(toInt(type))) {
		return;
	}

	int count = logFuncCount.load();
	if (count == 0) {
		DefaultLog(type, tag, source, t, va);
	} else {
		bool success = true;
		logFuncMutex.lock();
		count = logFuncCount.load();
		for (int i = 0; i < count; i++) {
			success = logFuncArr[i](type, tag, source, t, va);
			if (!success) {
				break;
			}
		}
		logFuncMutex.unlock();
		if (success) {
			DefaultLog(type, tag, source, t, va);
		}
	}
}

CustomLog::CustomLog(log_fn logfn) : fn(logfn) {
	manager = &s_logManager;
	if (fn) {
		reinterpret_cast<CustomLogManager *>(manager)->insert(fn);
	}
}

CustomLog::~CustomLog() {
	if (fn) {
		reinterpret_cast<CustomLogManager *>(manager)->remove(fn);
	}
}

CustomLog::CustomLog(CustomLog &&other) : fn(other.fn) {
	manager = &s_logManager;
	other.fn = nullptr;
}

CustomLog &CustomLog::operator=(CustomLog &&other) {
	manager = &s_logManager;
	if (fn && fn != other.fn) {
		reinterpret_cast<CustomLogManager *>(manager)->remove(fn);
	}
	fn = other.fn;
	other.fn = nullptr;
	return *this;
}

static std::bitset<6> makeFilterMask(InitializerList<LogType> type) {
	std::bitset<6> mask;
	for (auto &it : type) { mask.set(toInt(it)); }
	return mask;
}

std::bitset<6> None = makeFilterMask({Verbose, Debug, Info, Warn, Error, Fatal});
std::bitset<6> ErrorsOnly = makeFilterMask({Verbose, Debug, Info, Warn});
std::bitset<6> Full = std::bitset<6>(0);

void setLogFilterMask(const std::bitset<6> &mask) { s_logMask = mask; }

void setLogFilterMask(std::bitset<6> &&mask) { s_logMask = sp::move(mask); }

void setLogFilterMask(InitializerList<LogType> types) { setLogFilterMask(makeFilterMask(types)); }

std::bitset<6> getlogFilterMask() { return s_logMask; }

void format(LogType type, const char *tag, const SourceLocation &source, const char *fmt, ...) {
	CustomLog::VA va;
	va_start(va.format.args, fmt);
	va.format.format = fmt;

	s_logManager.log(type, tag, source, CustomLog::Format, va);

	va_end(va.format.args);
}

void format(LogType type, StringView tag, const SourceLocation &source, const char *fmt, ...) {
	CustomLog::VA va;
	va_start(va.format.args, fmt);
	va.format.format = fmt;

	s_logManager.log(type, tag, source, CustomLog::Format, va);

	va_end(va.format.args);
}

void text(LogType type, const char *tag, const SourceLocation &source, const char *text) {
	CustomLog::VA va;
	va.text = StringView(text);
	s_logManager.log(type, tag, source, CustomLog::Text, va);
}

void text(LogType type, StringView tag, StringView text, const SourceLocation &source) {
	CustomLog::VA va;
	va.text = text;
	s_logManager.log(type, tag, source, CustomLog::Text, va);
}

} // namespace stappler::log
