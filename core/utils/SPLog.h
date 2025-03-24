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

#ifndef STAPPLER_CORE_UTILS_SPLOG_H_
#define STAPPLER_CORE_UTILS_SPLOG_H_

#include "SPString.h"
#include "SPStringView.h"
#include "SPRef.h"

namespace STAPPLER_VERSIONIZED stappler::log {

enum LogType {
	Verbose,
	Debug,
	Info,
	Warn,
	Error,
	Fatal,
	Default = Debug,
};

struct SP_PUBLIC CustomLog {
	union VA {
		StringView text;
		struct {
			const char *format = nullptr;
			va_list args;
		} format;

		VA() { }
	};

	enum Type {
		Text,
		Format
	};

	using log_fn = bool (*) (LogType, StringView, Type, VA &);

	CustomLog(log_fn fn);
	~CustomLog();

	CustomLog(const CustomLog &) = delete;
	CustomLog& operator=(const CustomLog &) = delete;

	CustomLog(CustomLog &&);
	CustomLog& operator=(CustomLog &&);

	log_fn fn;
	void *manager;
};

extern std::bitset<6> None;
extern std::bitset<6> ErrorsOnly;
extern std::bitset<6> Full;

// log is suppressed if bit is set
// only default logger is affected
SP_PUBLIC void setLogFilterMask(const std::bitset<6> &);
SP_PUBLIC void setLogFilterMask(std::bitset<6> &&);
SP_PUBLIC void setLogFilterMask(InitializerList<LogType>);
SP_PUBLIC std::bitset<6> getlogFilterMask();

SP_PUBLIC void format(LogType, const StringView &tag, const char *, ...) SPPRINTF(3, 4);
SP_PUBLIC void text(LogType, const StringView &tag, const StringView &);

template <typename ... Args>
void verbose(const StringView &tag, Args && ... args) {
	text(LogType::Verbose, tag, StringView(mem_std::toString(std::forward<Args>(args)...)));
}

template <typename ... Args>
void debug(const StringView &tag, Args && ... args) {
	text(LogType::Debug, tag, StringView(mem_std::toString(std::forward<Args>(args)...)));
}

template <typename ... Args>
void info(const StringView &tag, Args && ... args) {
	text(LogType::Info, tag, StringView(mem_std::toString(std::forward<Args>(args)...)));
}

template <typename ... Args>
void warn(const StringView &tag, Args && ... args) {
	text(LogType::Warn, tag, StringView(mem_std::toString(std::forward<Args>(args)...)));
}

template <typename ... Args>
void error(const StringView &tag, Args && ... args) {
	text(LogType::Error, tag, StringView(mem_std::toString(std::forward<Args>(args)...)));
}

template <typename ... Args>
void fatal(const StringView &tag, Args && ... args) {
	text(LogType::Fatal, tag, StringView(mem_std::toString(std::forward<Args>(args)...)));
}

#if DEBUG
#define SPASSERT(cond, msg) do { \
	if (!(cond)) { \
		if (strlen(msg)) { STAPPLER_VERSIONIZED_NAMESPACE::log::format(STAPPLER_VERSIONIZED_NAMESPACE::log::LogType::Fatal, "Assert", "%s", msg);} \
		assert(cond); \
	} \
} while (0)
#else
#define SPASSERT(cond, msg)
#endif

}

#endif /* STAPPLER_CORE_UTILS_SPLOG_H_ */
