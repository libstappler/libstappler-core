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

#ifndef CORE_CORE_DETAIL_SPNOTNULL_H_
#define CORE_CORE_DETAIL_SPNOTNULL_H_

#include "SPLogInit.h"
#include <type_traits>

// see: https://stackoverflow.com/questions/63493968/reproducing-clangs-builtin-assume-for-gcc

// preferred option: C++ standard attribute
#ifdef __has_cpp_attribute
#if __has_cpp_attribute(assume) >= 202'207L
#define SPASSUME(...) [[assume(__VA_ARGS__)]]
#endif
#endif
// first fallback: compiler intrinsics/attributes for assumptions
#ifndef SPASSUME
#if defined(__clang__)
#define SPASSUME(...) do { __builtin_assume(__VA_ARGS__); } while(0)
#elif defined(_MSC_VER)
#define SPASSUME(...) do { __assume(__VA_ARGS__); } while(0)
#elif defined(__GNUC__)
#if __GNUC__ >= 13
#define SPASSUME(...) __attribute__((__assume__(__VA_ARGS__)))
#endif
#endif
#endif
// second fallback: possibly evaluating uses of unreachable()
#if !defined(SPASSUME)
#if defined(__GNUC__)
#define SPASSUME(...) do { if (!bool(__VA_ARGS__)) __builtin_unreachable(); } while(0)
#elif __cpp_lib_unreachable >= 202'202L
#include <utility>
#define SPASSUME(...) do { if (!bool(__VA_ARGS__)) ::std::unreachable(); ) while(0)
#endif
#endif
// last fallback: define macro as doing nothing
#ifndef SPASSUME
#define SPASSUME(...)
#endif


#if defined(__GNUC__) && (__GNUC__ >= 4)
#define SPNONNULL __attribute__((nonnull))
#elif defined(__has_attribute)
#if __has_attribute(nonnull)
#define SPNONNULL __attribute__((nonnull))
#endif // __has_attribute(nonnull)
#else
#define SPNONNULL
#endif

namespace STAPPLER_VERSIONIZED stappler {

// based on https://github.com/microsoft/GSL/blob/main/include/gsl/pointers

template <typename T>
class NotNull {
public:
	using element_type = T *;

	constexpr NotNull(T *u) noexcept SPNONNULL : _ptr(u) {
		SPASSERT(_ptr != nullptr, "Pointer should not be null");
		SPASSUME(_ptr != nullptr);
	}

	template <typename U, typename = std::enable_if_t<std::is_convertible<U *, T *>::value>>
	constexpr NotNull(const NotNull<U> &other) noexcept : NotNull(other.get()) { }

	NotNull(const NotNull &other) = default;
	NotNull &operator=(const NotNull &other) = default;

	constexpr auto get() const { return _ptr; }

	constexpr operator T *() const { return get(); }
	constexpr decltype(auto) operator->() const { return get(); }
	constexpr decltype(auto) operator*() const { return *get(); }

	// prevents compilation when someone attempts to assign a null pointer constant
	NotNull(std::nullptr_t) = delete;
	NotNull &operator=(std::nullptr_t) = delete;

	// unwanted operators...pointers only point to single objects!
	NotNull &operator++() = delete;
	NotNull &operator--() = delete;
	NotNull operator++(int) = delete;
	NotNull operator--(int) = delete;
	NotNull &operator+=(std::ptrdiff_t) = delete;
	NotNull &operator-=(std::ptrdiff_t) = delete;
	void operator[](std::ptrdiff_t) const = delete;

	void swap(NotNull<T> &other) { std::swap(_ptr, other._ptr); }

private:
	T *_ptr;
};

} // namespace STAPPLER_VERSIONIZED stappler

#endif /* CORE_CORE_DETAIL_SPNOTNULL_H_ */
