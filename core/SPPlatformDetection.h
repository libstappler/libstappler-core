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

#ifndef CORE_CORE_SPPLATFORMDETECTION_H_
#define CORE_CORE_SPPLATFORMDETECTION_H_

#ifdef __MINGW32__
#define _POSIX_THREAD_SAFE_FUNCTIONS 1
#endif

// IDE-specific definition
#if __CDT_PARSER__ // Eclipse CDT parser

// enable all modules

#define MODULE_STAPPLER_DATA 1
#define MODULE_STAPPLER_FILESYSTEM 1
#define MODULE_STAPPLER_BROTLI_LIB 1
#define MODULE_STAPPLER_THREADS 1
#define MODULE_STAPPLER_IDN 1
#define MODULE_STAPPLER_CRYPTO 1
#define MODULE_STAPPLER_BITMAP 1
#define MODULE_STAPPLER_THREADS 1
#define MODULE_STAPPLER_NETWORK 1
#define MODULE_STAPPLER_GEOM 1
#define MODULE_STAPPLER_TESS 1
#define MODULE_STAPPLER_VG 1
#define MODULE_STAPPLER_SEARCH 1
#define MODULE_STAPPLER_SQL 1
#define MODULE_STAPPLER_DB 1
#define MODULE_STAPPLER_ZIP 1

#define MODULE_STAPPLER_WEBSERVER_WEBSERVER 1
#define MODULE_STAPPLER_WEBSERVER_UNIX 1

#define MODULE_XENOLITH_CORE 1
#define MODULE_XENOLITH_APPLICATION 1
#define MODULE_XENOLITH_BACKEND_VK 1
#define MODULE_XENOLITH_BACKEND_VKGUI 1
#define MODULE_XENOLITH_RENDERER_BASIC2D 1
#define MODULE_XENOLITH_RENDERER_MATERIAL2D 1

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
   //define something for Windows (32-bit and 64-bit, this part is common)
	#ifdef _WIN64
		#define WINDOWS 1
		#ifndef WIN32
			#define WIN32 1
		#endif
	#else
		#define WINDOWS 1
		#ifndef WIN32
			#define WIN32 1
		#endif
	#endif
#elif __APPLE__
#include <TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR
		#define IOS 1
	#elif TARGET_OS_MACCATALYST
		#define IOS 1
    #elif TARGET_OS_IPHONE
		#define IOS 1
	#elif TARGET_OS_MAC
		#define MACOS 1
    #else
    #   error "Unknown Apple platform"
	#endif
#elif __ANDROID__
	#define ANDROID 1
#elif __linux__
	#define LINUX 1
#else
#   error "Unknown compiler"
#endif

namespace std {

template <typename T>
using iter_reference_t = typename T::reference;

}

#if WIN32
#define __cdecl

using LPCWSTR = const wchar_t *;
#define __int64 long long;

#endif

#endif // __CDT_PARSER__

#ifndef DEBUG
#ifndef NDEBUG
#define DEBUG 1
#endif
#endif

/* SP_HAVE_THREE_WAY_COMPARISON
 * - Defined as 1 if platform have <=> and defaulted comparison operators
 */

#if __LCC__ && __LCC__ <= 127
#define SP_HAVE_THREE_WAY_COMPARISON 0
#elif __cpp_impl_three_way_comparison >= 201711
#define SP_HAVE_THREE_WAY_COMPARISON 1
#else
#define SP_HAVE_THREE_WAY_COMPARISON 0
#endif

/* SP_HAVE_DEDICATED_SIZE_T
 * - Defined as 1 if platform's size_t defined as dedicated integral type
 * (not uint/int)
 */

#if MACOS
#define SP_HAVE_DEDICATED_SIZE_T 1
#else
#define SP_HAVE_DEDICATED_SIZE_T 0
#endif

// Enable default <=> operator if we can
#if SP_HAVE_THREE_WAY_COMPARISON
#define SP_THREE_WAY_COMPARISON_TYPE(Type) auto operator<=>(const Type&) const = default;
#define SP_THREE_WAY_COMPARISON_FRIEND(Type) friend auto operator<=>(const Type&, const Type &) = default;
#define SP_THREE_WAY_COMPARISON_TYPE_CONSTEXPR(Type) constexpr auto operator<=>(const Type &) const = default;
#define SP_THREE_WAY_COMPARISON_FRIEND_CONSTEXPR(Type) friend constexpr auto operator<=>(const Type&, const Type &) = default;
#else
#if __LCC__ && __LCC__ >= 127
#define SP_THREE_WAY_COMPARISON_TYPE(Type) \
	bool operator==(const Type&) const = default;\
	bool operator!=(const Type&) const = default;\
	bool operator>(const Type&) const = default;\
	bool operator>=(const Type&) const = default;\
	bool operator<=(const Type&) const = default;\
	bool operator<(const Type&) const = default;
#define SP_THREE_WAY_COMPARISON_FRIEND(Type) \
	friend bool operator==(const Type&, const Type &) = default;\
	friend bool operator!=(const Type&, const Type &) = default;\
	friend bool operator>(const Type&, const Type &) = default;\
	friend bool operator>=(const Type&, const Type &) = default;\
	friend bool operator<=(const Type&, const Type &) = default;\
	friend bool operator<(const Type&, const Type &) = default;
#define SP_THREE_WAY_COMPARISON_TYPE_CONSTEXPR(Type) \
	constexpr bool operator==(const Type&) const = default;\
	constexpr bool operator!=(const Type&) const = default;\
	constexpr bool operator>(const Type&) const = default;\
	constexpr bool operator>=(const Type&) const = default;\
	constexpr bool operator<=(const Type&) const = default;\
	constexpr bool operator<(const Type&) const = default;
#define SP_THREE_WAY_COMPARISON_FRIEND_CONSTEXPR(Type) \
	friend constexpr bool operator==(const Type&, const Type &) = default;\
	friend constexpr bool operator!=(const Type&, const Type &) = default;\
	friend constexpr bool operator>(const Type&, const Type &) = default;\
	friend constexpr bool operator>=(const Type&, const Type &) = default;\
	friend constexpr bool operator<=(const Type&, const Type &) = default;\
	friend constexpr bool operator<(const Type&, const Type &) = default;
#else
#define SP_THREE_WAY_COMPARISON_TYPE(Type)
#define SP_THREE_WAY_COMPARISON_FRIEND(Type)
#define SP_THREE_WAY_COMPARISON_TYPE_CONSTEXPR(Type)
#define SP_THREE_WAY_COMPARISON_FRIEND_CONSTEXPR(Type)
#endif
#endif

// we want _Float16 and _Float64
#define __STDC_WANT_IEC_60559_TYPES_EXT__ 1

// Suppress windows MIN/MAX macro
// Actually, windows-specific includes should be only in SPPlatformUnistd.h with proper filters for macro leaking
#if WIN32

#define NOMINMAX 1
#define _USE_MATH_DEFINES 1
#define strncasecmp _strnicmp
#define strcasecmp _stricmp

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#endif

#ifndef STAPPLER_VERSION_PREFIX
#define STAPPLER_VERSIONIZED
#define STAPPLER_VERSIONIZED_NAMESPACE ::stappler
#else
#define STAPPLER_VERSIONIZED STAPPLER_VERSION_PREFIX::
#define STAPPLER_VERSIONIZED_NAMESPACE ::STAPPLER_VERSION_PREFIX::stappler
#endif

#endif /* CORE_CORE_SPPLATFORMDETECTION_H_ */
