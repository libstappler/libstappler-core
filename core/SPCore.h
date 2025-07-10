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

#ifndef STAPPLER_CORE_SPCORE_H_
#define STAPPLER_CORE_SPCORE_H_

/* Stappler Core header: common includes and functions
 * Enables SDK-specific syntactic sugar and functions, should be first included header
 * (already true it you include any of SDK's functional headers)
 *
 * To enable precompiled headers, use SPCommon.h as a first include in translation unit instead
 */

#include "stappler-buildconfig.h"

namespace stappler::buildconfig {

constexpr auto MODULE_APPCONFIG_NAME = "appconfig";

}

#include "detail/SPPlatformInit.h"

// From C++ standard library:
#include <type_traits>
#include <typeindex>
#include <iterator>
#include <limits>
#include <utility>
#include <iterator>
#include <algorithm>
#include <tuple>
#include <cmath>
#include <locale>

#include <tuple>
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <forward_list>
#include <array>
#include <deque>
#include <bit>
#include <bitset>

#include <istream>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <future>
#include <thread>
#include <condition_variable>
#include <initializer_list>
#include <optional>
#include <variant>
#include <chrono>

#if __cplusplus >= 202'002L
#include <source_location>
#endif

// From C standard library:
#include <stdint.h> // uint32_t, int32_t, etc
#include <string.h> // memset, memcpy, memmove
#include <stdarg.h> // va_arg
#include <stdlib.h> // strto*
#include <assert.h> // assert macro

#if SP_HAVE_THREE_WAY_COMPARISON
#include <compare>
#endif

#include "detail/SPHash.h"
#include "detail/SPMath.h"
#include "detail/SPValueWrapper.h"
#include "detail/SPEnum.h"
#include "detail/SPNotNull.h"

#include "detail/SPPlatformCleanup.h"

namespace STAPPLER_VERSIONIZED stappler {

inline constexpr uint32_t SP_MAKE_API_VERSION(uint32_t variant, uint32_t major, uint32_t minor,
		uint32_t patch) {
	return (uint32_t(variant) << 29) | (uint32_t(major) << 22) | (uint32_t(minor) << 12)
			| uint32_t(patch & 0b1111'1111'1111);
}

// Use sp::move instead of std::move to enable additional diagnostics
template <typename T,
		typename std::enable_if_t<
				// Attempt to move pointer is most likely an reference count error
				// Use move_unsafe if it's not an error (like, in template context)
				!std::is_pointer_v<std::remove_reference_t<T>>, bool> = true>
[[nodiscard]]
constexpr typename std::remove_reference<T>::type &&move(T &&value) noexcept {
	return static_cast<typename std::remove_reference<T>::type &&>(value);
}

// Behaves like std::move
template <typename T>
[[nodiscard]]
constexpr typename std::remove_reference<T>::type &&move_unsafe(T &&value) noexcept {
	return static_cast<typename std::remove_reference<T>::type &&>(value);
}

// Stappler requires only C++17, backport for std::bit_cast
// Previously referenced as as `reinterpretValue`;
template <class To, class From>
std::enable_if_t< sizeof(To) == sizeof(From)
				&& std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>,
		To>
bit_cast(const From &src) noexcept {
	static_assert(std::is_trivially_constructible_v<To>,
        "This implementation additionally requires "
        "destination type to be trivially constructible");

	To dst;
	::memcpy(&dst, &src, sizeof(To));
	return dst;
}

using std::forward;
using std::min;
using std::max;

using nullptr_t = std::nullptr_t;

/*
 *   User Defined literals
 *
 *   Functions:
 *   - _len / _length     - string literal length
 *   - _GiB / _MiB / _KiB - binary size numbers
 *   - _c8 / _c16         - convert integer literal to character
 */

// string length (useful for comparation: memcmp(str, "Test", "Test"_len) )
constexpr size_t operator""_length(const char *str, size_t len) { return len; }
constexpr size_t operator""_length(const char16_t *str, size_t len) { return len; }
constexpr size_t operator""_len(const char *str, size_t len) { return len; }
constexpr size_t operator""_len(const char16_t *str, size_t len) { return len; }

constexpr unsigned long long int operator""_GiB(unsigned long long int val) {
	return val * 1'024 * 1'024 * 1'024;
}
constexpr unsigned long long int operator""_MiB(unsigned long long int val) {
	return val * 1'024 * 1'024;
}
constexpr unsigned long long int operator""_KiB(unsigned long long int val) { return val * 1'024; }

constexpr char16_t operator""_c16(unsigned long long int val) { return (char16_t)val; }
constexpr char operator""_c8(unsigned long long int val) { return (char)val; }

template <typename... Args>
inline constexpr auto pair(Args &&...args) -> decltype(std::make_pair(forward<Args>(args)...)) {
	return std::make_pair(forward<Args>(args)...);
}

template <typename T, typename V>
using Pair = std::pair<T, V>;

template <typename T>
using InitializerList = std::initializer_list<T>;

/** Functions for enum flags */
template <typename T>
bool hasFlag(T mask, T flag) {
	return (mask & flag) != T(0);
}


/*
 * 		Invoker/CallTest macro
 * Tests when some method of class C is defined
 */

#define InvokerCallTest_MakeCallTest(Name, Success, Failure) \
	private: \
		template <typename C> static Success CallTest_ ## Name( typeof(&C::Name) ); \
		template <typename C> static Failure CallTest_ ## Name(...); \
	public: \
		static constexpr bool Name = sizeof(CallTest_ ## Name<T>(0)) == sizeof(success);


/*
 * Initialization API
 *
 * call `initialize` when main application thread is started
 * call `terminate`` when main thread is stopped
 *
 * initialize returns false when application should not try to run,
 * and set appropriate resultCode to return from application's main
 *
 * if initialize return true - app can be run as usual
 *
 * or use perform_main from SPMemory.h when possible
*/

SP_PUBLIC bool initialize(int &resultCode);
SP_PUBLIC void terminate();

// `init` will be called in FIFO order, `term` - in reverse (LIFO) order
// if `initialize` was already called, `init` will be called in place
SP_PUBLIC bool addInitializer(void *ptr, NotNull<void(void *)> init, NotNull<void(void *)> term);

/*
 * SDK Version API
 */

SP_PUBLIC const char *getStapplerVersionString();

SP_PUBLIC uint32_t getStapplerVersionIndex();

SP_PUBLIC uint32_t getStapplerVersionVariant();

// API version number
SP_PUBLIC uint32_t getStapplerVersionApi();

// Build revision version number
SP_PUBLIC uint32_t getStapplerVersionRev();

// Build number
SP_PUBLIC uint32_t getStapplerVersionBuild();

/*
 * Appconfig API
 * 
 * Appconfig uses SharedModule appconfig, that should be created by build system when building the application
 *
 * Use `getVersionDescription<Interface>(getAppconfigVersionIndex())` for a version string
 */

// Returns NULL when appconfig is not defined
SP_PUBLIC const char *getAppconfigBundleName();

// Returns NULL when appconfig is not defined
SP_PUBLIC const char *getAppconfigAppName();

SP_PUBLIC uint32_t getAppconfigVersionIndex();

SP_PUBLIC uint32_t getAppconfigVersionVariant();

SP_PUBLIC uint32_t getAppconfigVersionApi();

SP_PUBLIC uint32_t getAppconfigVersionRev();

SP_PUBLIC uint32_t getAppconfigVersionBuild();

} // namespace STAPPLER_VERSIONIZED stappler

namespace STAPPLER_VERSIONIZED sp = STAPPLER_VERSIONIZED_NAMESPACE;

#endif /* STAPPLER_CORE_SPCORE_H_ */
