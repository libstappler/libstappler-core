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

#ifndef STAPPLER_CORE_STRING_SPBYTEORDER_H_
#define STAPPLER_CORE_STRING_SPBYTEORDER_H_

#include "SPCore.h"

#ifndef __CDT_PARSER__
#ifndef SPINLINE
#define SPINLINE __attribute__((always_inline))
#endif
#endif

#ifndef __has_builtin         // Optional of course
  #define __has_builtin(x) 0  // Compatibility with non-clang compilers
#endif

//  Adapted code from BOOST_ENDIAN_INTRINSICS
//  GCC and Clang recent versions provide intrinsic byte swaps via builtins
#if (defined(__clang__) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64)) \
  || (defined(__GNUC__ ) && \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))

// prior to 4.8, gcc did not provide __builtin_bswap16 on some platforms so we emulate it
// see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=52624
// Clang has a similar problem, but their feature test macros make it easier to detect
namespace STAPPLER_VERSIONIZED stappler::byteorder {
# if (defined(__clang__) && __has_builtin(__builtin_bswap16)) \
  || (defined(__GNUC__) &&(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
static SPINLINE inline uint16_t bswap16(uint16_t x) { return __builtin_bswap16(x); }
# else
static SPINLINE inline uint16_t bswap16(uint16_t x) { return __builtin_bswap32(x) << 16; }
# endif
static SPINLINE inline uint32_t bswap32(uint32_t x) { return __builtin_bswap32(x); }
static SPINLINE inline uint64_t bswap64(uint64_t x) { return __builtin_bswap64(x); }
}

//  Linux systems provide the byteswap.h header, with
#elif defined(__linux__)
//  don't check for obsolete forms defined(linux) and defined(__linux) on the theory that
//  compilers that predefine only these are so old that byteswap.h probably isn't present.
# include <byteswap.h>
namespace STAPPLER_VERSIONIZED stappler::byteorder {
static SPINLINE inline uint16_t bswap16(uint16_t x) { return bswap_16(x); }
static SPINLINE inline uint32_t bswap32(uint32_t x) { return bswap_32(x); }
static SPINLINE inline uint64_t bswap64(uint64_t x) { return bswap_64(x); }
}

#elif defined(_MSC_VER)
//  Microsoft documents these as being compatible since Windows 95 and specificly
//  lists runtime library support since Visual Studio 2003 (aka 7.1).
# include <cstdlib>
namespace STAPPLER_VERSIONIZED stappler::byteorder {
static SPINLINE inline uint16_t bswap16(uint16_t x) { return _byteswap_ushort(x); }
static SPINLINE inline uint32_t bswap32(uint32_t x) { return _byteswap_ulong(x); }
static SPINLINE inline uint64_t bswap64(uint64_t x) { return _byteswap_uint64(x); }
}
#else
namespace STAPPLER_VERSIONIZED stappler::byteorder {
static SPINLINE inline uint16_t bswap16(uint16_t x) {
	return (x & 0xFF) << 8 | ((x >> 8) & 0xFF);
}

static SPINLINE inline uint32_t bswap32(uint32_t x) {
	return x & 0xFF << 24
		| (x >> 8 & 0xFF) << 16
		| (x >> 16 & 0xFF) << 8
		| (x >> 24 & 0xFF);
}
static SPINLINE inline uint64_t bswap64(uint64_t x) {
	return x & 0xFF << 56
		| (x >> 8 & 0xFF) << 48
		| (x >> 16 & 0xFF) << 40
		| (x >> 24 & 0xFF) << 32
		| (x >> 32 & 0xFF) << 24
		| (x >> 40 & 0xFF) << 16
		| (x >> 48 & 0xFF) << 8
		| (x >> 56 & 0xFF);
}
}
#endif

namespace STAPPLER_VERSIONIZED stappler {

#if __cpp_lib_endian >= 201907L
enum class Endian {
	Big,
	Little,
	Mixed,
	Network = Big,
	Host = (std::endian::native == std::endian::little) ? Little : ((std::endian::native == std::endian::big) ? Big : Mixed), // no support for custom endian
};
#else
enum class Endian {
	Big,
	Little,
	Mixed,
	Network = Big,
#if (__i386__) || (_M_IX86) || (__x86_64__) || (_M_X64) || (__arm__) || (_M_ARM) || (__arm64__) || (__arm64) || defined(__aarch64__) || defined(__e2k__)
	Host = Little,
#else
	Host = Big,
#endif
};
#endif

}

namespace STAPPLER_VERSIONIZED stappler::byteorder {

enum class ShouldSwap {
	NoSwap,
	DoSwap,
	CustomSwap,
};

static constexpr size_t Bit8Size = 1;
static constexpr size_t Bit16Size = 2;
static constexpr size_t Bit32Size = 4;
static constexpr size_t Bit64Size = 8;

template <class T, ShouldSwap ByteSwap, size_t Size>
struct Converter;

template <class T>
struct Converter<T, ShouldSwap::DoSwap, Bit8Size> {
	static inline T Swap(T value) {
		return value;
	}
};

template <class T>
struct Converter<T, ShouldSwap::DoSwap, Bit16Size> {
	static inline T Swap(T value) {
		return bit_cast<T>(bswap16(bit_cast<uint16_t>(value)));
	}
};

template <class T>
struct Converter<T, ShouldSwap::DoSwap, Bit32Size> {
	static inline T Swap(T value) {
		return bit_cast<T>(bswap32(bit_cast<uint32_t>(value)));
	}
};

template <class T>
struct Converter<T, ShouldSwap::DoSwap, Bit64Size> {
	static inline T Swap(T value) {
		return bit_cast<T>(bswap64(bit_cast<uint64_t>(value)));
	}
};

template <class T, size_t Size>
struct Converter<T, ShouldSwap::DoSwap, Size> {
	static inline T Swap(T value) {
		T ret;
		char *ptr = (char *)&ret;
		::memcpy((void *)ptr, (const void *)&value, sizeof(T));
		std::reverse(ptr, ptr + sizeof(T));
		return ret;
	}
};

template <class T, size_t Size>
struct Converter<T, ShouldSwap::NoSwap, Size> {
	static inline T Swap(T value) { return value; }
};


static constexpr ShouldSwap hostToNetwork() {
	if constexpr (Endian::Host == Endian::Network) { return ShouldSwap::NoSwap; }
	if constexpr (Endian::Host == Endian::Little) { return ShouldSwap::DoSwap; }
	return ShouldSwap::CustomSwap;
}

static constexpr ShouldSwap hostToLittle() {
	if constexpr (Endian::Host == Endian::Little) { return ShouldSwap::NoSwap; }
	if constexpr (Endian::Host == Endian::Big) { return ShouldSwap::DoSwap; }
	return ShouldSwap::CustomSwap;
}

static constexpr ShouldSwap hostToBig() {
	if constexpr (Endian::Host == Endian::Big) { return ShouldSwap::NoSwap; }
	if constexpr (Endian::Host == Endian::Little) { return ShouldSwap::DoSwap; }
	return ShouldSwap::CustomSwap;
}

static constexpr bool isLittleEndian() {
	return Endian::Host == Endian::Little;
}

template <class T>
using NetworkConverter = Converter<T, hostToNetwork(), sizeof(T)>;

template <class T>
using LittleConverter = Converter<T, hostToLittle(), sizeof(T)>;

template <class T>
using BigConverter = Converter<T, hostToBig(), sizeof(T)>;

template <class T>
using HostConverter = Converter<T, ShouldSwap::NoSwap, sizeof(T)>;

template <class T>
static inline T HostToNetwork(T value) {
	return NetworkConverter<T>::Swap(value);
}

template <class T>
static inline T NetworkToHost(T value) {
	return NetworkConverter<T>::Swap(value);
}

template <class T>
static inline T HostToLittle(T value) {
	return LittleConverter<T>::Swap(value);
}

template <class T>
static inline T LittleToHost(T value) {
	return LittleConverter<T>::Swap(value);
}

template <class T>
static inline T HostToBig(T value) {
	return BigConverter<T>::Swap(value);
}

template <class T>
static inline T BigToHost(T value) {
	return BigConverter<T>::Swap(value);
}

template <Endian Endianess, typename T>
struct ConverterTraits;

template <typename T>
struct ConverterTraits<Endian::Big, T> : BigConverter<T> { };

template <typename T>
struct ConverterTraits<Endian::Little, T> : LittleConverter<T> { };

}

#endif /* STAPPLER_CORE_STRING_SPBYTEORDER_H_ */
