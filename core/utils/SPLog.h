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

#ifndef STAPPLER_CORE_UTILS_SPLOG_H_
#define STAPPLER_CORE_UTILS_SPLOG_H_

#include "SPString.h"
#include "SPStringView.h"
#include "SPRef.h"

namespace stappler::log {

struct CustomLog {
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

	using log_fn = void (*) (const StringView &, Type, VA &);

	CustomLog(log_fn fn);
	~CustomLog();

	CustomLog(const CustomLog &) = delete;
	CustomLog& operator=(const CustomLog &) = delete;

	CustomLog(CustomLog &&);
	CustomLog& operator=(CustomLog &&);

	log_fn fn;
	Rc<RefBase<memory::StandartInterface>> manager;
};

void format(const StringView &tag, const char *, ...) SPPRINTF(2, 3);
void text(const StringView &tag, const StringView &);

template <typename ... Args>
void vtext(const StringView &tag, Args && ... args) {
	text(tag, StringView(mem_std::toString(std::forward<Args>(args)...)));
}

#if DEBUG
#define SPASSERT(cond, msg) do { \
	if (!(cond)) { \
		if (strlen(msg)) { ::stappler::log::format("Assert", "%s", msg);} \
		assert(cond); \
	} \
} while (0)
#else
#define SPASSERT(cond, msg)
#endif

}

#endif /* STAPPLER_CORE_UTILS_SPLOG_H_ */
