/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>
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

#ifndef CORE_CORE_UTILS_SPDSO_H_
#define CORE_CORE_UTILS_SPDSO_H_

#include "SPStringView.h" // IWYU pragma: keep

namespace STAPPLER_VERSIONIZED stappler {

enum class DsoFlags : uint32_t {
	None = 0,
	Self = 1
			<< 0, // open caller app itself instead of target library ( Dso(StringView(), DsoFlags::Self) )
	Lazy = 1 << 1, // use lazy binding if available (default)
	Global = 1 << 2,

	UserFlags = Self | Lazy | Global,

	StapplerAbi = 1
			<< 30, // set by implementation for Dso, opened with stappler-abi module instead of actual OS DSO
};

SP_DEFINE_ENUM_AS_MASK(DsoFlags)

enum class DsoSymFlags : uint32_t {
	None = 0,
	Executable = 1 << 0, // Symbol is executable
	Loader = 1 << 1, // Symbol is loader for other symbols
};

SP_DEFINE_ENUM_AS_MASK(DsoSymFlags)

struct SP_PUBLIC DsoLoaderInfo {
	void *(*open)(StringView, int flags) = nullptr;
	Status (*close)(void *) = nullptr;
	void *(*sym)(void *handle, StringView symbol) = nullptr;
	StringView (*error)(void) = nullptr;
};

class SP_PUBLIC Dso {
public:
	// Version number for shared modules, defined, when DSO loaded
	// This is actual when called within some DSO operation (open/close/sym)
	static uint32_t GetCurrentVersion();

	~Dso();

	Dso();
	Dso(StringView, uint32_t v = 0); // Lazy | Local by default
	Dso(StringView, DsoFlags, uint32_t v = 0);

	Dso(const Dso &) = delete;
	Dso &operator=(const Dso &) = delete;

	Dso(Dso &&);
	Dso &operator=(Dso &&);

	template <typename T = void *>
	T sym(StringView name, DsoSymFlags flags = DsoSymFlags::None) {
		static_assert(std::is_pointer<T>::value, "Pointer type required to load from DSO");
		if constexpr (std::is_function_v<std::remove_pointer_t<T>>) {
			flags |= DsoSymFlags::Executable;
		}
		return reinterpret_cast<T>(loadSym(name, flags));
	}

	explicit operator bool() const { return _handle != nullptr; }

	DsoFlags getFlags() const { return _flags; }
	StringView getError() const { return _error; }
	uint32_t getVersion() const { return _version; }

	void close();

	bool isSelf() const;
	bool isLazy() const;

protected:
	void *loadSym(StringView, DsoSymFlags);

	DsoFlags _flags = DsoFlags::None;
	void *_handle = nullptr;
	const char *_error = nullptr;
	uint32_t _version = 0;
};

} // namespace STAPPLER_VERSIONIZED stappler

#endif /* CORE_CORE_UTILS_SPDSO_H_ */
