/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

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

#ifndef CORE_CORE_UTILS_SPDSO_H_
#define CORE_CORE_UTILS_SPDSO_H_

#include "SPStringView.h"

namespace stappler {

enum class DsoFlags : uint32_t {
	None = 0,
	Self = 1 << 0, // open caller app itself instead of target library ( Dso(StringView(), DsoFlags::Self) )
	Lazy = 1 << 1, // use lazy binding if available (default)
	Global = 1 << 2,
};

SP_DEFINE_ENUM_AS_MASK(DsoFlags)

class Dso {
public:
	~Dso();

	Dso();
	Dso(StringView); // Lazy | Local by default
	Dso(StringView, DsoFlags);

	Dso(const Dso &) = delete;
	Dso & operator=(const Dso &) = delete;

	Dso(Dso &&);
	Dso & operator=(Dso &&);

	template <typename T = void *>
	T sym(StringView name) {
		static_assert(std::is_pointer<T>::value, "Pointer type required to load from DSO");
		return reinterpret_cast<T>(loadSym(name));
	}

	operator bool() const { return _handle != nullptr; }

	DsoFlags getFlags() const { return _flags; }

	StringView getError() const { return _error; }

	void close();

	bool isSelf() const;
	bool isLazy() const;

protected:
	void *loadSym(StringView);

	DsoFlags _flags = DsoFlags::None;
	void *_handle = nullptr;
	const char *_error = nullptr;
};

}

#endif /* CORE_CORE_UTILS_SPDSO_H_ */
