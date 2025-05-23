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

#ifndef CORE_CORE_UTILS_SPENUM_H_
#define CORE_CORE_UTILS_SPENUM_H_

#include "SPPlatformInit.h"

#include <iterator>
#include <string.h>

// A part of SPCore.h, DO NOT include this directly

namespace stappler {

// Enumeration utilities, usage like:
//
// for (auto value : each<EnumClass>()>) { }
//
// EnumClass should contains value with name Max
//

namespace detail {

template <typename E>
struct enum_iterator_end { };

template <typename E, E Last>
struct enum_iterator {
	using iterator_category = std::random_access_iterator_tag;
	using size_type = size_t;
	using pointer = E *;
	using reference = E;
	using difference_type = std::ptrdiff_t;
	using value_type = E;

	using iterator = enum_iterator<E, Last>;

	constexpr enum_iterator() noexcept : value(E(0)) {}
	constexpr enum_iterator(const enum_iterator & other) noexcept = default;

	explicit constexpr enum_iterator(E e) : value(toInt(e)) { }
	explicit constexpr enum_iterator(typename std::underlying_type<E>::type e) : value(e) { }

	constexpr iterator& operator=(const iterator &other) noexcept { value = other.value; return *this; }
	constexpr bool operator==(const iterator &other) const { return value == other.value; }
	constexpr bool operator!=(const iterator &other) const { return value != other.value; }
	constexpr bool operator<(const iterator &other) const { return value < other.value; }
	constexpr bool operator>(const iterator &other) const { return value > other.value; }
	constexpr bool operator<=(const iterator &other) const { return value <= other.value; }
	constexpr bool operator>=(const iterator &other) const { return value >= other.value; }

	constexpr bool operator==(const enum_iterator_end<E> &other) const { return value > toInt(Last); }
	constexpr bool operator!=(const enum_iterator_end<E> &other) const { return value <= toInt(Last); }

	constexpr iterator& operator++() { ++ value; return *this; }
	constexpr iterator operator++(int) { auto tmp = *this; ++ value; return tmp; }
	constexpr iterator& operator--() { --value; return *this; }
	constexpr iterator operator--(int) { auto tmp = *this; --value; return tmp; }
	constexpr iterator& operator+= (size_type n) { value += n; return *this; }
	constexpr iterator& operator-=(size_type n) { value -= n; return *this; }
	constexpr difference_type operator-(const iterator &other) const { return value - other.value; }

	constexpr reference operator*() const { return E(value); }
	constexpr pointer operator->() const { return &value; }
	constexpr reference operator[](size_type n) const { return *(value + n); }

	constexpr size_type operator-(pointer p) const { return value - p; }

	friend constexpr auto operator+(size_type n, const iterator &it) -> iterator {
		return iterator(it.value + n);
	}

	friend constexpr auto operator+(const iterator &it, size_type n) -> iterator {
		return iterator(it.value + n);
	}

	friend constexpr auto operator-(const iterator &it, size_type n) -> iterator {
		return iterator(it.value - n);
	}

	typename std::underlying_type<E>::type value;
};

template <typename E, E First, E Last>
struct enum_wrapper {
	constexpr enum_iterator<E, Last> begin() const { return enum_iterator<E, Last>(First); }
	constexpr enum_iterator_end<E> end() const { return enum_iterator_end<E>(); }
};

}

template <typename E, E First, E Last>
auto each() -> detail::enum_wrapper<E, First, Last> {
	return detail::enum_wrapper<E, First, Last>();
}

template <typename E>
auto each() -> detail::enum_wrapper<E, E(0), E(toInt(E::Max) - 1)> {
	return detail::enum_wrapper<E, E(0), E(toInt(E::Max) - 1)>();
}

}

/** SP_DEFINE_ENUM_AS_MASK is utility to make a bitwise-mask from typed enum
 * It defines a set of overloaded operators, that allow some bitwise operations
 * on this enum class
 */
#define SP_DEFINE_ENUM_AS_MASK(Type) \
	SP_COVERAGE_TRIVIAL constexpr inline Type operator | (const Type &l, const Type &r) { return Type(::sp::toInt(l) | ::sp::toInt(r)); } \
	SP_COVERAGE_TRIVIAL constexpr inline Type operator & (const Type &l, const Type &r) { return Type(::sp::toInt(l) & ::sp::toInt(r)); } \
	SP_COVERAGE_TRIVIAL constexpr inline Type operator ^ (const Type &l, const Type &r) { return Type(::sp::toInt(l) ^ ::sp::toInt(r)); } \
	SP_COVERAGE_TRIVIAL constexpr inline Type & operator |= (Type &l, const Type &r) { l = Type(::sp::toInt(l) | ::sp::toInt(r)); return l; } \
	SP_COVERAGE_TRIVIAL constexpr inline Type & operator &= (Type &l, const Type &r) { l = Type(::sp::toInt(l) & ::sp::toInt(r)); return l; } \
	SP_COVERAGE_TRIVIAL constexpr inline Type & operator ^= (Type &l, const Type &r) { l = Type(::sp::toInt(l) ^ ::sp::toInt(r)); return l; } \
	SP_COVERAGE_TRIVIAL constexpr inline bool operator == (const Type &l, const std::underlying_type<Type>::type &r) { return ::sp::toInt(l) == r; } \
	SP_COVERAGE_TRIVIAL constexpr inline bool operator == (const std::underlying_type<Type>::type &l, const Type &r) { return l == ::sp::toInt(r); } \
	SP_COVERAGE_TRIVIAL constexpr inline bool operator != (const Type &l, const std::underlying_type<Type>::type &r) { return ::sp::toInt(l) != r; } \
	SP_COVERAGE_TRIVIAL constexpr inline bool operator != (const std::underlying_type<Type>::type &l, const Type &r) { return l != ::sp::toInt(r); } \
	SP_COVERAGE_TRIVIAL constexpr inline Type operator~(const Type &t) { return Type(~::sp::toInt(t)); }

/** SP_DEFINE_ENUM_AS_INCREMENTABLE adds operator++/operator-- for enumerations */
#define SP_DEFINE_ENUM_AS_INCREMENTABLE(Type, First, Last) \
	SP_COVERAGE_TRIVIAL inline constexpr Type& operator++(Type& a) { \
		auto value = ::sp::toInt(a); \
		if (value >= ::sp::toInt(Type::Last)) { value = ::sp::toInt(Type::First); } else { ++ value; } \
		::memcpy(&a, &value, sizeof(Type)); \
		return a; \
	} \
	SP_COVERAGE_TRIVIAL inline constexpr Type& operator--(Type& a) { \
		auto value = ::sp::toInt(a); \
		if (value <= ::sp::toInt(Type::First)) { value = ::sp::toInt(Type::Last); } else { -- value; } \
		::memcpy(&a, &value, sizeof(Type)); \
		return a; \
	} \
	SP_COVERAGE_TRIVIAL inline constexpr Type operator++(Type& a, int) { \
		auto value = ::sp::toInt(a); auto result = value; \
		if (value >= ::sp::toInt(Type::Last)) { value = ::sp::toInt(Type::First); } else { ++ value; } \
		::memcpy(&a, &value, sizeof(Type)); \
		return static_cast<Type>(result); \
	} \
	SP_COVERAGE_TRIVIAL inline constexpr Type operator--(Type& a, int) { \
		auto value = ::sp::toInt(a); auto result = value; \
		if (value <= ::sp::toInt(Type::First)) { value = ::sp::toInt(Type::Last); } else { -- value; } \
		::memcpy(&a, &value, sizeof(Type)); \
		return static_cast<Type>(result); \
	} \
	SP_COVERAGE_TRIVIAL inline constexpr Type operator+(const Type &a, const typename std::underlying_type<Type>::type &b) { \
		return Type(::sp::math::add_cyclic(::sp::toInt(a), b, ::sp::toInt(Type::First), ::sp::toInt(Type::Last))); \
	} \
	SP_COVERAGE_TRIVIAL inline constexpr Type operator+=(Type &a, const typename std::underlying_type<Type>::type &b) { \
		auto value = ::sp::math::add_cyclic(::sp::toInt(a), b, ::sp::toInt(Type::First), ::sp::toInt(Type::Last)); \
		::memcpy(&a, &value, sizeof(Type)); \
		return a; \
	} \
	SP_COVERAGE_TRIVIAL inline constexpr Type operator-(const Type &a, const typename std::underlying_type<Type>::type &b) { \
		return Type(::sp::math::sub_cyclic(::sp::toInt(a), b, ::sp::toInt(Type::First), ::sp::toInt(Type::Last))); \
	} \
	SP_COVERAGE_TRIVIAL inline constexpr Type operator-=(Type &a, const typename std::underlying_type<Type>::type &b) { \
		auto value = ::sp::math::sub_cyclic(::sp::toInt(a), b, ::sp::toInt(Type::First), ::sp::toInt(Type::Last)); \
		::memcpy(&a, &value, sizeof(Type)); \
		return a; \
	}

#endif /* CORE_CORE_UTILS_SPENUM_H_ */
