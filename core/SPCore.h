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

#ifndef STAPPLER_CORE_SPCORE_H_
#define STAPPLER_CORE_SPCORE_H_

/* Stappler Core header: common includes and functions
 *
 * this file should be included before any other files to enable
 * precompiled-header compilation optimization
 * (if no other specific precompiled header needed)
 */

#include "SPPlatformInit.h"

// From C++ standard library:
#include <type_traits>
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
#include <bit>

#include <istream>
#include <ostream>
#include <array>
#include <iostream>
#include <istream>
#include <ostream>
#include <iomanip>
#include <mutex>
#include <atomic>
#include <deque>
#include <thread>
#include <condition_variable>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <forward_list>

// From C standard library:
#include <stdint.h> // uint32_t, int32_t, etc
#include <string.h> // memset, memcpy, memmove
#include <stdarg.h> // va_arg
#include <assert.h> // assert macro

#if SP_HAVE_THREE_WAY_COMPARISON
#include <compare>
#endif

#include "SPPlatformCleanup.h"

/*
 *   User Defined literals
 *
 *   Functions:
 *   - _hash / _tag       - FNV-1 32-bit compile-time hashing
 *   - _hash64 / _tag64   - FNV-1 64-bit compile-time hashing
 *   - _len / _length     - string literal length
 *   - _to_rad            - convert degrees to radians
 *   - _GiB / _MiB / _KiB - binary size numbers
 *   - _c8 / _c16         - convert integer literal to character
 */

#include "SPHash.h"

namespace STAPPLER_VERSIONIZED stappler {

using std::forward;
using std::move;
using std::min;
using std::max;

using nullptr_t = std::nullptr_t;

namespace numbers {
template<typename T> inline constexpr T pi_v =
		std::enable_if_t<std::is_floating_point_v<T>, T>(3.141592653589793238462643383279502884L);

inline constexpr double pi = pi_v<double>;

}

// used for naming/hashing (like "MyTag"_tag)
constexpr uint32_t operator"" _hash ( const char* str, size_t len) {
	return stappler::hash::hash32(str, uint32_t(len));
}
constexpr uint32_t operator"" _tag ( const char* str, size_t len) {
	return stappler::hash::hash32(str, uint32_t(len));
}

constexpr uint64_t operator"" _hash64 ( const char* str, size_t len) {
	return stappler::hash::hash64(str, len);
}
constexpr uint64_t operator"" _tag64 ( const char* str, size_t len) {
	return stappler::hash::hash64(str, len);
}

constexpr long double operator"" _to_rad ( long double val ) { return val * numbers::pi / 180.0; }
constexpr long double operator"" _to_rad ( unsigned long long int val ) { return val * numbers::pi / 180.0; }

// string length (useful for comparation: memcmp(str, "Test", "Test"_len) )
constexpr size_t operator"" _length ( const char* str, size_t len) { return len; }
constexpr size_t operator"" _length ( const char16_t* str, size_t len) { return len; }
constexpr size_t operator"" _len ( const char* str, size_t len) { return len; }
constexpr size_t operator"" _len ( const char16_t* str, size_t len) { return len; }

constexpr unsigned long long int operator"" _GiB ( unsigned long long int val ) { return val * 1024 * 1024 * 1024; }
constexpr unsigned long long int operator"" _MiB ( unsigned long long int val ) { return val * 1024 * 1024; }
constexpr unsigned long long int operator"" _KiB ( unsigned long long int val ) { return val * 1024; }

constexpr char16_t operator"" _c16 (unsigned long long int val) { return (char16_t)val; }
constexpr char operator"" _c8 (unsigned long long int val) { return (char)val; }


/*
 *   Misc templates
 *
 *   Functions:
 *   - nan<float / double>() - shortcut for Not A Number value
 *   - minOf<T>() / maxOf<T>() - shortcuts for minimal/maximal values
 *   - toInt<T>(EnumVal) - extract integer from strong-typed enum
 *
 *   - T reinterpretValue(V) - relaxed version of reinterpret_cast,
 *       often used to convert signed to unsigned
 *
 *   - T StringToNumber(const char *ptr, char ** tail) - read specified
 *       numeric type from string in `strtod` style
 *
 *   - progress(A, B, p) - linear interpolation between A and B, defined by float
 *       between 0.0f and 1.0f
 *
 *   - SP_DEFINE_ENUM_AS_MASK(Type) - defines strongly-typed enum class
 *       as bitmask
 */

template <typename... Args>
inline constexpr auto pair(Args&&... args) -> decltype(std::make_pair(forward<Args>(args)...)) {
	return std::make_pair(forward<Args>(args)...);
}

template <typename T, typename V>
using Pair = std::pair<T, V>;

template <typename T>
using NumericLimits = std::numeric_limits<T>;

template <typename T>
using InitializerList = std::initializer_list<T>;

template <typename T = float>
inline constexpr auto nan() -> T {
	return NumericLimits<T>::quiet_NaN();
}

template <typename T = float>
inline constexpr auto epsilon() -> T {
	return NumericLimits<T>::epsilon();
}

template <typename T>
inline auto isnan(T && t) -> bool {
	return std::isnan(std::forward<T>(t));
}

template <class T>
inline constexpr T maxOf() {
	return NumericLimits<T>::max();
}

template <class T>
inline constexpr T minOf() {
	return NumericLimits<T>::min();
}

// For floats - use lowest: https://en.cppreference.com/w/cpp/types/numeric_limits/lowest
template <>
inline constexpr double minOf<double>() {
	return NumericLimits<double>::lowest();
}

template <>
inline constexpr float minOf<float>() {
	return NumericLimits<float>::lowest();
}

template <>
inline constexpr long double minOf<long double>() {
	return NumericLimits<long double>::lowest();
}

template <typename T, typename V>
struct HasMultiplication {
	template<class A, class B>
	static auto test(A *, B *) -> decltype(std::declval<A>() * std::declval<B>());

	template<typename, typename>
	static auto test(...) -> std::false_type;

	using type = typename std::is_same<T, decltype(test<T, V>(0, 0))>::type;
};

template <class T>
constexpr inline T progress(const T &a, const T &b, float p) { return (a * (1.0f - p) + b * p); }

template <class T, class V>
struct _ValueReinterpretator {
	inline static T reinterpret(V v) { T ret; memcpy(&ret, &v, sizeof(V)); return ret; } // follow strict aliasing rules
};

template <class T>
struct _ValueReinterpretator<T, T> {
	inline static T reinterpret(T v) { return v; }
};

template <class T, class V, typename std::enable_if<sizeof(T) == sizeof(V)>::type* = nullptr>
inline T reinterpretValue(V v) {
	return _ValueReinterpretator<T, V>::reinterpret(v);
}

template <typename E>
constexpr typename std::underlying_type<E>::type toInt(const E &e) {
    return static_cast<typename std::underlying_type<E>::type>(e);
}

template <typename T> auto StringToNumber(const char *ptr, char ** tail, int base) -> T;

template <> inline auto
StringToNumber<unsigned int>(const char *ptr, char ** tail, int base) -> unsigned int {
	if (ptr) {
		return (unsigned int)strtoul(ptr, tail, base);
	}
	return 0;
}

template <> inline auto
StringToNumber<unsigned long>(const char *ptr, char ** tail, int base) -> unsigned long {
	if (ptr) {
		return strtoul(ptr, tail, base);
	}
	return 0;
}

template <> inline auto
StringToNumber<unsigned long long>(const char *ptr, char ** tail, int base) -> unsigned long long {
	if (ptr) {
		return strtoull(ptr, tail, base);
	}
	return 0;
}

template <> inline auto
StringToNumber<int>(const char *ptr, char ** tail, int base) -> int {
	if (ptr) {
		return (int)strtol(ptr, tail, base);
	}
	return 0;
}

template <> inline auto
StringToNumber<long>(const char *ptr, char ** tail, int base) -> long {
	if (ptr) {
		return strtol(ptr, tail, base);
	}
	return 0;
}

template <> inline auto
StringToNumber<long long>(const char *ptr, char ** tail, int base) -> long long {
	if (ptr) {
		return strtoll(ptr, tail, base);
	}
	return 0;
}

template <> inline auto
StringToNumber<float>(const char *ptr, char ** tail, int base) -> float {
	if (ptr) {
		return strtof(ptr, tail);
	}
	return 0.0f;
}

template <> inline auto
StringToNumber<double>(const char *ptr, char ** tail, int base) -> double {
	if (ptr) {
		return strtod(ptr, tail);
	}
	return 0.0;
}

/** SP_DEFINE_ENUM_AS_MASK is utility to make a bitwise-mask from typed enum
 * It defines a set of overloaded operators, that allow some bitwise operations
 * on this enum class
 */
#define SP_DEFINE_ENUM_AS_MASK(Type) \
	SP_COVERAGE_TRIVIAL constexpr inline Type operator | (const Type &l, const Type &r) { return Type(stappler::toInt(l) | stappler::toInt(r)); } \
	SP_COVERAGE_TRIVIAL constexpr inline Type operator & (const Type &l, const Type &r) { return Type(stappler::toInt(l) & stappler::toInt(r)); } \
	SP_COVERAGE_TRIVIAL constexpr inline Type operator ^ (const Type &l, const Type &r) { return Type(stappler::toInt(l) ^ stappler::toInt(r)); } \
	SP_COVERAGE_TRIVIAL constexpr inline Type & operator |= (Type &l, const Type &r) { l = Type(stappler::toInt(l) | stappler::toInt(r)); return l; } \
	SP_COVERAGE_TRIVIAL constexpr inline Type & operator &= (Type &l, const Type &r) { l = Type(stappler::toInt(l) & stappler::toInt(r)); return l; } \
	SP_COVERAGE_TRIVIAL constexpr inline Type & operator ^= (Type &l, const Type &r) { l = Type(stappler::toInt(l) ^ stappler::toInt(r)); return l; } \
	SP_COVERAGE_TRIVIAL constexpr inline bool operator == (const Type &l, const std::underlying_type<Type>::type &r) { return stappler::toInt(l) == r; } \
	SP_COVERAGE_TRIVIAL constexpr inline bool operator == (const std::underlying_type<Type>::type &l, const Type &r) { return l == stappler::toInt(r); } \
	SP_COVERAGE_TRIVIAL constexpr inline bool operator != (const Type &l, const std::underlying_type<Type>::type &r) { return stappler::toInt(l) != r; } \
	SP_COVERAGE_TRIVIAL constexpr inline bool operator != (const std::underlying_type<Type>::type &l, const Type &r) { return l != stappler::toInt(r); } \
	SP_COVERAGE_TRIVIAL constexpr inline Type operator~(const Type &t) { return Type(~stappler::toInt(t)); }


/** Value wrapper is a syntactic sugar struct, that allow you to create
 * an alias type for some other type, that will be statically and uniquely
 * different from all other types
 *
 * Most common usage is type-based overload resolution, like
 *
 * using FilePath = ValueWrapper<StringView, class FilePathFlag>;
 * using DataString =  ValueWrapper<StringView, class DataStringFlag>;
 *
 * ...
 *
 * class SomeClass {
 * 	SomeClass(FilePath); // init with data from file
 * 	SomeClass(DataString); // init with data from memory
 * };
 *
 * Also, ValueWrapper used in implementation of function with named arguments
 * and function, that requires additional type-checking
 */

template <class T, class Flag>
struct ValueWrapper {
	using Type = T;

	static constexpr ValueWrapper<T, Flag> max() { return ValueWrapper<T, Flag>(NumericLimits<T>::max()); }
	static constexpr ValueWrapper<T, Flag> min() { return ValueWrapper<T, Flag>(NumericLimits<T>::min()); }
	static constexpr ValueWrapper<T, Flag> epsilon() { return ValueWrapper<T, Flag>(NumericLimits<T>::epsilon()); }
	static constexpr ValueWrapper<T, Flag> zero() { return ValueWrapper<T, Flag>(0); }

	inline constexpr ValueWrapper() noexcept = default;
	inline explicit constexpr ValueWrapper(const T &val) noexcept : value(val) { }
	inline explicit constexpr ValueWrapper(T &&val) noexcept : value(std::move(val)) { }

	inline ValueWrapper(const ValueWrapper<T, Flag> &other) noexcept = default;
	inline ValueWrapper<T, Flag> &operator=(const ValueWrapper<T, Flag> &other) noexcept = default;

	inline ValueWrapper(ValueWrapper<T, Flag> &&other) noexcept = default;
	inline ValueWrapper<T, Flag> &operator=(ValueWrapper<T, Flag> &&other) noexcept = default;

	inline void set(const T &val) { value = val; }
	inline void set(T &&val) { value = std::move(val); }
	inline constexpr T & get() { return value; }
	inline constexpr const T & get() const { return value; }
	inline constexpr bool empty() const { return value == 0; }

	inline constexpr bool operator == (const ValueWrapper<T, Flag> & other) const { return value == other.value; }
	inline constexpr bool operator != (const ValueWrapper<T, Flag> & other) const { return value != other.value; }
	inline constexpr bool operator > (const ValueWrapper<T, Flag> & other) const { return value > other.value; }
	inline constexpr bool operator < (const ValueWrapper<T, Flag> & other) const { return value < other.value; }
	inline constexpr bool operator >= (const ValueWrapper<T, Flag> & other) const { return value >= other.value; }
	inline constexpr bool operator <= (const ValueWrapper<T, Flag> & other) const { return value <= other.value; }

	inline constexpr void operator |= (const ValueWrapper<T, Flag> & other) { value |= other.value; }
	inline constexpr void operator &= (const ValueWrapper<T, Flag> & other) { value &= other.value; }
	inline constexpr void operator ^= (const ValueWrapper<T, Flag> & other) { value ^= other.value; }
	inline constexpr void operator += (const ValueWrapper<T, Flag> & other) { value += other.value; }
	inline constexpr void operator -= (const ValueWrapper<T, Flag> & other) { value -= other.value; }
	inline constexpr void operator *= (const ValueWrapper<T, Flag> & other) { value *= other.value; }
	inline constexpr void operator /= (const ValueWrapper<T, Flag> & other) { value /= other.value; }

	inline constexpr ValueWrapper<T, Flag> operator|(const ValueWrapper<T, Flag>& v) const { return ValueWrapper<T, Flag>(value | v.value); }
	inline constexpr ValueWrapper<T, Flag> operator&(const ValueWrapper<T, Flag>& v) const { return ValueWrapper<T, Flag>(value & v.value); }
	inline constexpr ValueWrapper<T, Flag> operator^(const ValueWrapper<T, Flag>& v) const { return ValueWrapper<T, Flag>(value ^ v.value); }
	inline constexpr ValueWrapper<T, Flag> operator+(const ValueWrapper<T, Flag>& v) const { return ValueWrapper<T, Flag>(value + v.value); }
	inline constexpr ValueWrapper<T, Flag> operator-(const ValueWrapper<T, Flag>& v) const { return ValueWrapper<T, Flag>(value - v.value); }
	inline constexpr ValueWrapper<T, Flag> operator*(const ValueWrapper<T, Flag>& v) const { return ValueWrapper<T, Flag>(value * v.value); }
	inline constexpr ValueWrapper<T, Flag> operator/(const ValueWrapper<T, Flag>& v) const { return ValueWrapper<T, Flag>(value / v.value); }
	inline constexpr ValueWrapper<T, Flag> operator-() const { return ValueWrapper<T, Flag>(-value); }

	inline ValueWrapper<T, Flag> &operator++ () { value ++; return *this; }
	inline ValueWrapper<T, Flag> &operator-- () { value --; return *this; }

	inline ValueWrapper<T, Flag> operator++ (int) { ValueWrapper<T, Flag> result(*this); ++(*this); return result; }
	inline ValueWrapper<T, Flag> operator-- (int) { ValueWrapper<T, Flag> result(*this); --(*this); return result; }

	// to enable progress
	template <typename M>
	inline constexpr std::enable_if_t<HasMultiplication<Type, M>::type::value, ValueWrapper<T, Flag>>
	operator*(const M &v) const { return ValueWrapper<T, Flag>(value * v); }

#if SP_HAVE_THREE_WAY_COMPARISON
	SP_THREE_WAY_COMPARISON_TYPE(ValueWrapper)
#endif

	T value;
};

/** Result is a helper class for functions, that returns some result
 * or fails and returns nothing. It defines several mechanisms to handle
 * error state:
 * - get with default value in case of failure (`get`)
 * - grab value into object, provided by reference, if value is valid (`grab`)
 * - call a callback with value, if it's valid (`unwrap`)
 */
template <typename T>
struct Result {
	enum Status {
		Ok,
		Error
	};

	Status status = Error;
	T result;

	static Result<T> error() { return Result(); }

	Result(T && t) noexcept : status(Ok), result(move(t)) { }
	Result(const T & t) noexcept : status(Ok), result(t) { }

	Result() noexcept = default;
	Result(const Result &) noexcept = default;
	Result(Result &&) noexcept = default;
	Result& operator=(const Result &) noexcept = default;
	Result& operator=(Result &&) noexcept = default;

	bool valid() const { return status == Ok; }

	template <typename Callback>
	bool unwrap(const Callback &cb) const {
		if (status == Ok) {
			cb(result);
			return true;
		}
		return false;
	}

	bool grab(T &value) {
		if (status == Ok) {
			value = move(result);
			return true;
		}
		return false;
	}

	const T & get() const { return result; }
	const T & get(const T &def) const { return (status == Ok) ? result : def; }
};

/*
 * 		Invoker/CallTest macro
 */

#define InvokerCallTest_MakeCallTest(Name, Success, Failure) \
	private: \
		template <typename C> static Success CallTest_ ## Name( typeof(&C::Name) ); \
		template <typename C> static Failure CallTest_ ## Name(...); \
	public: \
		static constexpr bool Name = sizeof(CallTest_ ## Name<T>(0)) == sizeof(success);

SP_PUBLIC const char * getStapplerVersionString();

// API version number
SP_PUBLIC uint32_t getStapplerVersionApi();

// Build revision version number
SP_PUBLIC uint32_t getStapplerVersionRev();

}

/*
 * 		Extra math functions
 */

namespace STAPPLER_VERSIONIZED stappler::math {

constexpr float MATH_FLOAT_SMALL = 1.0e-37f;
constexpr float MATH_TOLERANCE = 2e-37f;

/**
 * Updates this vector towards the given target using a smoothing function.
 * The given response time determines the amount of smoothing (lag). A longer
 * response time yields a smoother result and more lag. To force this vector to
 * follow the target closely, provide a response time that is very small relative
 * to the given elapsed time. */

// avoid constexpr to support SIMD-based implementation
template <typename T> inline
T smooth(const T &source, const T &target, float elapsed, float response) {
	if (elapsed > 0) {
		return source + (target - source) * (elapsed / (elapsed + response));
	}
	return source;
}

// avoid constexpr to support SIMD-based implementation
template <typename T, typename V = float> inline
T lerp(const T &a, const T &b, const V &alpha) {
	return (a * (- alpha + 1.0f) + b * alpha);
}

template<class T, class Compare> constexpr inline
const T& clamp(const T& v, const T& lo, const T& hi, Compare comp) {
	if (comp(hi, lo)) {
	    return comp(v, hi) ? hi : comp(lo, v) ? lo : v;
	} else {
	    return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
	}
}

template<class T> constexpr inline
const T& clamp(const T& v, const T& lo, const T& hi) {
    return math::clamp( v, lo, hi, std::less<T>() );
}

template<class T, class Compare> constexpr inline
T clamp_distance(const T& v, const T& lo, const T& hi, Compare comp, const T &z) {
	assert( !comp(hi, lo) );
	return comp(v, lo) ? (lo - v) : comp(hi, v) ? (v - hi) : z;
}

template<class T, class Compare> constexpr inline
T clamp_distance(const T& v, const T& lo, const T& hi, Compare comp) {
	return clamp_distance(v, lo, hi, comp, T(0));
}

template<class T> constexpr inline
T clamp_distance(const T& v, const T& lo, const T& hi, const T &z) {
	return clamp_distance(v, lo, hi, std::less<T>(), z);
}

template<class T> constexpr inline
T clamp_distance(const T& v, const T& lo, const T& hi) {
	return clamp_distance(v, lo, hi, std::less<T>(), T(0));
}

// next power of two
inline uint32_t npot(uint32_t n) {
    --n;

    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return n + 1;
}

inline uint64_t npot(uint64_t n) {
    --n;

    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;

    return n + 1;
}

// Align on a power of 2 boundary
template <typename T = uint64_t>
static constexpr auto align(T size, T boundary) -> T {
	return (((size) + ((boundary) - 1)) & ~((boundary) - 1));
}

// convert degrees to radians
template <typename T>
constexpr auto to_rad(T val) -> T {
	return T(val) * numbers::pi_v<T> / T(180);
}

// convert radians to degrees
template <typename T>
constexpr auto to_deg(T val) -> T {
	return T(val) * T(180) / numbers::pi_v<T>;
}

}

namespace std {

template <typename Value, typename Flag>
struct hash<STAPPLER_VERSIONIZED_NAMESPACE::ValueWrapper<Value, Flag>> {
	hash() { }

	size_t operator() (const STAPPLER_VERSIONIZED_NAMESPACE::ValueWrapper<Value, Flag> &value) const noexcept {
		return hash<Value>()(value.get());
	}
};

}

#endif /* STAPPLER_CORE_SPCORE_H_ */
