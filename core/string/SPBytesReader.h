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

#ifndef STAPPLER_CORE_STRING_SPBYTESREADER_H_
#define STAPPLER_CORE_STRING_SPBYTESREADER_H_

#include "SPMemInterface.h"
#include "SPCharGroup.h"
#include "SPStatus.h"
#include "SPByteOrder.h"

namespace STAPPLER_VERSIONIZED stappler {

template <typename _CharType>
class BytesReader {
public:
	using CharType = _CharType;

	template <CharType... Args>
	using Chars = chars::Chars<CharType, Args...>;

	template <CharType First, CharType Last>
	using Range = chars::Range<CharType, First, Last>;

	template <CharGroupId Group>
	using CharGroup = chars::CharGroup<CharType, Group>;

	template <typename... Args>
	using Compose = chars::Compose<CharType, Args...>;

	constexpr BytesReader() = default;
	constexpr BytesReader(const CharType *p, size_t l) : ptr(p), len(l) { }

	BytesReader &set(const CharType *p, size_t l) {
		ptr = p;
		len = l;
		return *this;
	}

	void offset(size_t l) {
		if (l > len) {
			len = 0;
		} else {
			ptr += l;
			len -= l;
		}
	}

	bool equals(const CharType *d, size_t l) const {
		return (l == len && memcmp(ptr, d, l * sizeof(CharType)) == 0);
	}
	bool equals(const CharType *d) const {
		return equals(d, std::char_traits<CharType>::length(d));
	}

	bool prefix(const CharType *d, size_t l) const {
		return (l <= len && memcmp(ptr, d, l * sizeof(CharType)) == 0);
	}

	bool starts_with(const BytesReader<CharType> &str) const {
		return prefix(str.data(), str.size());
	}
	bool starts_with(const CharType *d, size_t l) const { return prefix(d, l); }
	bool starts_with(const CharType *d) const {
		return prefix(d, std::char_traits<CharType>::length(d));
	}
	bool starts_with(CharType c) const { return is(c); }

	bool ends_with(const BytesReader<CharType> &str) const {
		return ends_with(str.data(), str.size());
	}
	bool ends_with(const CharType *d, size_t l) const {
		return (l <= len && memcmp(ptr + (len - l), d, l * sizeof(CharType)) == 0);
	}
	bool ends_with(const CharType *d) const {
		return ends_with(d, std::char_traits<CharType>::length(d));
	}
	bool ends_with(CharType c) const { return len > 0 && ptr[len - 1] == c; }

	constexpr const CharType *data() const { return ptr; }

	constexpr size_t size() const { return len; }

	size_t find(const CharType *s, size_t pos, size_t n) const;
	size_t find(CharType c, size_t pos = 0) const;

	size_t rfind(const CharType *s, size_t pos, size_t n) const;
	size_t rfind(CharType c, size_t pos = maxOf<size_t>()) const;

	size_t find(const BytesReader<CharType> &str, size_t pos = 0) const {
		return this->find(str.data(), pos, str.size());
	}
	size_t find(const CharType *s, size_t pos = 0) const {
		return this->find(s, pos, std::char_traits<CharType>::length(s));
	}

	size_t rfind(const BytesReader<CharType> &str, size_t pos = maxOf<size_t>()) const {
		return this->rfind(str.data(), pos, str.size());
	}
	size_t rfind(const CharType *s, size_t pos = maxOf<size_t>()) const {
		return this->rfind(s, pos, std::char_traits<CharType>::length(s));
	}

	bool is(const CharType &c) const { return len > 0 && *ptr == c; };

	bool operator>(const size_t &val) const { return len > val; }
	bool operator>=(const size_t &val) const { return len >= val; }
	bool operator<(const size_t &val) const { return len < val; }
	bool operator<=(const size_t &val) const { return len <= val; }

	const CharType &front() const { return *ptr; }
	const CharType &back() const { return ptr[len - 1]; }

	const CharType &at(const size_t &s) const { return ptr[s]; }
	const CharType &operator[](const size_t &s) const { return ptr[s]; }
	const CharType &operator*() const { return *ptr; }

	void clear() { len = 0; }
	bool empty() const { return len == 0 || !ptr; }

	bool terminated() const { return ptr[len] == 0; }

protected:
	const CharType *ptr = nullptr;
	size_t len = 0;
};


template <typename _CharType>
class StringViewBase : public BytesReader<_CharType> {
public:
	using Self = StringViewBase;
	using MatchCharType = _CharType;
	using CharType = _CharType;
	using value_type = _CharType;
	using TraitsType = typename std::char_traits<CharType>;

	using PoolString = typename memory::PoolInterface::template BasicStringType<CharType>;
	using StdString = typename memory::StandartInterface::template BasicStringType<CharType>;

	template <CharType... Args>
	using MatchChars = chars::Chars<CharType, Args...>;

	template <char First, char Last>
	using MatchRange = chars::Range<CharType, First, Last>;

	template <CharGroupId Group>
	using MatchCharGroup = chars::CharGroup<CharType, Group>;

	// CharGroup shortcuts
	using Numbers = MatchCharGroup<CharGroupId::Numbers>;
	using Latin = MatchCharGroup<CharGroupId::Latin>;
	using WhiteSpace = MatchCharGroup<CharGroupId::WhiteSpace>;
	using LatinLowercase = MatchCharGroup<CharGroupId::LatinLowercase>;
	using LatinUppercase = MatchCharGroup<CharGroupId::LatinUppercase>;
	using Alphanumeric = MatchCharGroup<CharGroupId::Alphanumeric>;
	using Hexadecimial = MatchCharGroup<CharGroupId::Hexadecimial>;
	using Base64 = MatchCharGroup<CharGroupId::Base64>;

	template <typename Interface, typename... Args>
	static auto merge(Args &&...args) -> typename Interface::template BasicStringType<CharType>;

	template <typename Interface, _CharType c, typename... Args>
	static auto merge(Args &&...args) -> typename Interface::template BasicStringType<CharType>;

	constexpr StringViewBase() = default;
	constexpr StringViewBase(const CharType *ptr, size_t len = maxOf<size_t>());
	constexpr StringViewBase(const CharType *ptr, size_t pos, size_t len);
	constexpr StringViewBase(const Self &, size_t pos, size_t len);
	constexpr StringViewBase(const Self &, size_t len);
	StringViewBase(const PoolString &str);
	StringViewBase(const StdString &str);

	Self &operator=(const PoolString &str);
	Self &operator=(const StdString &str);
	Self &operator=(const Self &str) = default;

	Self &set(const PoolString &str);
	Self &set(const StdString &str);
	Self &set(const Self &str);

	// unsafe set, without length-check
	Self &set(const CharType *p, size_t l);

	bool is(const CharType &c) const;
	bool is(const CharType *c) const;
	bool is(const Self &c) const;

	template <_CharType C>
	bool is() const;
	template <CharGroupId G>
	bool is() const;
	template <typename M>
	bool is() const;

	Self sub(size_t pos = 0, size_t len = maxOf<size_t>()) const {
		if (pos > this->size()) {
			return Self();
		}

		return StringViewBase(*this, pos, len);
	}

	Self pdup(memory::pool_t * = nullptr) const;

	Self ptolower_c(memory::pool_t * = nullptr) const;
	Self ptoupper_c(memory::pool_t * = nullptr) const;

	template <typename Interface>
	auto str() const -> typename Interface::template BasicStringType<CharType>;

	Self &operator++();
	Self operator++(int);
	Self &operator+=(size_t l);

	Self begin() const;
	Self end() const;

	Self operator-(const Self &) const;
	Self &operator-=(const Self &) const;

	uint64_t hash() const {
		return hash::hash64((const char *)this->data(), this->size() * sizeof(CharType));
	}

	uint32_t hash32() const {
		return hash::hash32((const char *)this->data(), uint32_t(this->size() * sizeof(CharType)));
	}

public:
	Result<float> readFloat();
	Result<double> readDouble();
	Result<int64_t> readInteger(int base = 0);

public:
	template <typename... Args>
	void skipChars();
	template <typename... Args>
	void skipUntil();

	template <typename... Args>
	void backwardSkipChars();
	template <typename... Args>
	void backwardSkipUntil();

	bool skipString(const Self &str);
	bool skipUntilString(const Self &str, bool stopBeforeString = true);

	template <typename... Args>
	Self readChars();
	template <typename... Args>
	Self readUntil();

	template <typename... Args>
	Self backwardReadChars();
	template <typename... Args>
	Self backwardReadUntil();

	Self readUntilString(const Self &str);

	template <typename Separator, typename Callback>
	void split(const Callback &cb) const;

	template <typename... Args>
	void trimChars();
	template <typename... Args>
	void trimUntil();

protected:
	template <typename T>
	static size_t __size(const T &);

	static size_t __size(const CharType *);

	template <typename T, typename... Args>
	static size_t _size(T &&);

	template <typename T, typename... Args>
	static size_t _size(T &&, Args &&...args);

	template <typename Buf, typename T>
	static void __merge(Buf &, T &&t);

	template <typename Buf>
	static void __merge(Buf &, const CharType *);

	template <typename Buf, typename T, typename... Args>
	static void _merge(Buf &, T &&, Args &&...args);

	template <typename Buf, typename T>
	static void _merge(Buf &, T &&);

	template <typename Buf, _CharType C, bool Front, typename T>
	static void __mergeWithSep(Buf &, T &&t);

	template <typename Buf, _CharType C, bool Front, typename T, typename... Args>
	static void _mergeWithSep(Buf &, T &&, Args &&...args);

	template <typename Buf, _CharType C, bool Front, typename T>
	static void _mergeWithSep(Buf &, T &&);

	template <typename... Args>
	bool match(CharType c);
};

class StringViewUtf8 : public BytesReader<char> {
public:
	using Self = StringViewUtf8;
	using MatchCharType = char32_t;
	using CharType = char;
	using value_type = char;
	using TraitsType = typename std::char_traits<char>;

	using PoolString = typename memory::PoolInterface::StringType;
	using StdString = typename memory::StandartInterface::StringType;

	template <MatchCharType... Args>
	using MatchChars = chars::Chars<MatchCharType, Args...>;

	template <char First, char Last>
	using MatchRange = chars::Range<MatchCharType, First, Last>;

	template <CharGroupId Group>
	using MatchCharGroup = chars::CharGroup<MatchCharType, Group>;

	template <typename... Args>
	using MatchCompose = chars::Compose<MatchCharType, Args...>;

	template <MatchCharType... Args>
	using Chars = chars::Chars<MatchCharType, Args...>;

	template <char First, char Last>
	using Range = chars::Range<MatchCharType, First, Last>;

	template <CharGroupId Group>
	using CharGroup = chars::CharGroup<MatchCharType, Group>;

	template <typename... Args>
	using Compose = chars::Compose<MatchCharType, Args...>;

	// CharGroup shortcuts
	using Numbers = MatchCharGroup<CharGroupId::Numbers>;
	using Latin = MatchCharGroup<CharGroupId::Latin>;
	using WhiteSpace = MatchCharGroup<CharGroupId::WhiteSpace>;
	using LatinLowercase = MatchCharGroup<CharGroupId::LatinLowercase>;
	using LatinUppercase = MatchCharGroup<CharGroupId::LatinUppercase>;
	using Alphanumeric = MatchCharGroup<CharGroupId::Alphanumeric>;
	using Hexadecimial = MatchCharGroup<CharGroupId::Hexadecimial>;
	using Base64 = MatchCharGroup<CharGroupId::Base64>;

	StringViewUtf8();
	StringViewUtf8(const char *ptr, size_t len = maxOf<size_t>());
	StringViewUtf8(const char *ptr, size_t pos, size_t len);
	StringViewUtf8(const StringViewUtf8 &, size_t len);
	StringViewUtf8(const StringViewUtf8 &, size_t pos, size_t len);
	StringViewUtf8(const PoolString &str);
	StringViewUtf8(const StdString &str);
	StringViewUtf8(const StringViewBase<char> &str);

	Self &operator=(const PoolString &str);
	Self &operator=(const StdString &str);
	Self &operator=(const Self &str);

	Self &set(const PoolString &str);
	Self &set(const StdString &str);
	Self &set(const Self &str);
	Self &set(const char *p, size_t l);

	bool is(const char &c) const;
	bool is(const char16_t &c) const;
	bool is(const char32_t &c) const;
	bool is(const char *c) const;
	bool is(const Self &c) const;

	template <char32_t C>
	bool is() const;
	template <CharGroupId G>
	bool is() const;
	template <typename M>
	bool is() const;

	Self sub(size_t pos = 0, size_t len = maxOf<size_t>()) const {
		return StringViewUtf8(*this, pos, len);
	}

	Self letter() const;

	template <typename Interface>
	auto str() const -> typename Interface::StringType;

	void offset(size_t l);
	Self &operator++();
	Self operator++(int);
	Self &operator+=(size_t l);

	bool isSpace() const;

	Self begin() const;
	Self end() const;

	Self operator-(const Self &) const;
	Self &operator-=(const Self &);

	MatchCharType operator*() const;

	template <typename Callback>
	void foreach (const Callback &cb) const;

	size_t code_size() const;

	operator StringViewBase<char>() const;

	uint64_t hash() const { return hash::hash64(data(), size() * sizeof(CharType)); }

	uint64_t hash32() const { return hash::hash32(data(), uint32_t(size() * sizeof(CharType))); }

public:
	Result<float> readFloat();
	Result<double> readDouble();
	Result<int64_t> readInteger(int base = 0);

public:
	template <typename... Args>
	void skipChars();
	template <typename... Args>
	void skipUntil();

	template <typename... Args>
	void backwardSkipChars();
	template <typename... Args>
	void backwardSkipUntil();

	bool skipString(const Self &str);
	bool skipUntilString(const Self &str, bool stopBeforeString = true);

	template <typename... Args>
	Self readChars();
	template <typename... Args>
	Self readUntil();

	template <typename... Args>
	Self backwardReadChars();
	template <typename... Args>
	Self backwardReadUntil();

	Self readUntilString(const Self &str);
	template <typename Separator, typename Callback>
	void split(const Callback &cb) const;

	template <typename... Args>
	void trimChars();
	template <typename... Args>
	void trimUntil();

protected: // char-matching inline functions
	template <typename... Args>
	bool rv_match_utf8(const CharType *ptr, size_t len, uint8_t &offset);
	template <typename... Args>
	bool match(MatchCharType c);
};


template <Endian Endianess = Endian::Network>
class BytesViewTemplate : public BytesReader<uint8_t> {
public:
	template <class T>
	using Converter = byteorder::ConverterTraits<Endianess, T>;

	using Self = BytesViewTemplate<Endianess>;

	using PoolBytes = typename memory::PoolInterface::BytesType;
	using StdBytes = typename memory::StandartInterface::BytesType;

	constexpr BytesViewTemplate() = default;
	constexpr BytesViewTemplate(const uint8_t *p, size_t l);
	constexpr BytesViewTemplate(StringViewBase<char>);

	BytesViewTemplate(const PoolBytes &vec);
	BytesViewTemplate(const StdBytes &vec);

	template <size_t Size>
	constexpr BytesViewTemplate(const std::array<uint8_t, Size> &arr);

	constexpr BytesViewTemplate(const Self &vec) = default;

	template <Endian OtherEndianess>
	constexpr BytesViewTemplate(const BytesViewTemplate<OtherEndianess> &vec);

	template <Endian OtherEndianess>
	constexpr BytesViewTemplate(const BytesViewTemplate<OtherEndianess>, size_t len);

	template <Endian OtherEndianess>
	constexpr BytesViewTemplate(const BytesViewTemplate<OtherEndianess>, size_t pos, size_t len);

	Self &operator=(const PoolBytes &b);
	Self &operator=(const StdBytes &b);
	Self &operator=(const Self &b);

	Self &set(const PoolBytes &vec);
	Self &set(const StdBytes &vec);
	Self &set(const Self &vec);
	Self &set(const uint8_t *p, size_t l);

	Self &operator++();
	Self &operator++(int);
	Self &operator+=(size_t l);

	bool operator==(const Self &other) const;
	bool operator!=(const Self &other) const;

	Self pdup(memory::pool_t * = nullptr) const;

	template <typename Interface>
	auto bytes() const -> typename Interface::BytesType;

	constexpr Self sub(size_t pos = 0, size_t len = maxOf<size_t>()) const {
		return Self(*this, pos, len);
	}

private:
	template <typename T>
	auto convert(const uint8_t *data) -> T;

	bool match(CharType c) { return false; }
	template <uint8_t Arg, uint8_t... Args>
	bool match(CharType c);

public:
	template <uint8_t... Args>
	void skipChars();
	template <uint8_t... Args>
	void skipUntil();

	template <uint8_t... Args>
	void backwardSkipChars();
	template <uint8_t... Args>
	void backwardSkipUntil();

	template <uint8_t... Args>
	Self readChars();
	template <uint8_t... Args>
	Self readUntil();

	template <uint8_t... Args>
	Self backwardReadChars();
	template <uint8_t... Args>
	Self backwardReadUntil();

	template <uint8_t... Args>
	void trimChars();
	template <uint8_t... Args>
	void trimUntil();

	template <typename Separator, typename Callback>
	void split(const Callback &cb) const;

	uint64_t readUnsigned64();
	uint32_t readUnsigned32();
	uint32_t readUnsigned24();
	uint16_t readUnsigned16();
	uint8_t readUnsigned();

	double readFloat64();
	float readFloat32();
	float readFloat16();

	StringViewBase<char> readString(); // read null-terminated string
	StringViewBase<char> readString(size_t s); // read fixed-size string

	StringViewBase<char> toStringView() const;

	template <Endian OtherEndianess = Endianess>
	auto readBytes(size_t s) -> BytesViewTemplate<OtherEndianess>; // read fixed-size string
};

using StringView = StringViewBase<char>;
using WideStringView = StringViewBase<char16_t>;

using BytesView = BytesViewTemplate<Endian::Host>;
using BytesViewNetwork = BytesViewTemplate<Endian::Network>;
using BytesViewHost = BytesViewTemplate<Endian::Host>;


template <typename CharType>
inline size_t BytesReader<CharType>::find(const CharType *__s, size_t __pos, size_t __n) const {
	using traits_type = std::char_traits<CharType>;
	const size_t __size = this->size();
	const CharType *__data = data();

	if (__n == 0) {
		return __pos <= __size ? __pos : maxOf<size_t>();
	} else if (__n <= __size) {
		for (; __pos <= __size - __n; ++__pos) {
			if (traits_type::eq(__data[__pos], __s[0])
					&& traits_type::compare(__data + __pos + 1, __s + 1, __n - 1) == 0) {
				return __pos;
			}
		}
	}
	return maxOf<size_t>();
}

//
// Implementation
//

template <typename CharType>
inline size_t BytesReader<CharType>::find(CharType __c, size_t __pos) const {
	using traits_type = std::char_traits<CharType>;
	size_t __ret = maxOf<size_t>();
	const size_t __size = this->size();
	if (__pos < __size) {
		const CharType *__data = data();
		const size_t __n = __size - __pos;
		const CharType *__p = traits_type::find(__data + __pos, __n, __c);
		if (__p) {
			__ret = __p - __data;
		}
	}
	return __ret;
}

template <typename CharType>
inline size_t BytesReader<CharType>::rfind(const CharType *__s, size_t __pos, size_t __n) const {
	using traits_type = std::char_traits<CharType>;
	const size_t __size = this->size();
	if (__n <= __size) {
		__pos = min(size_t(__size - __n), __pos);
		const CharType *__data = data();
		do {
			if (traits_type::compare(__data + __pos, __s, __n) == 0) {
				return __pos;
			}
		} while (__pos-- > 0);
	}
	return maxOf<size_t>();
}

template <typename CharType>
inline size_t BytesReader<CharType>::rfind(CharType __c, size_t __pos) const {
	using traits_type = std::char_traits<CharType>;
	size_t __size = this->size();
	if (__size) {
		if (--__size > __pos) {
			__size = __pos;
		}
		for (++__size; __size-- > 0;) {
			if (traits_type::eq(data()[__size], __c)) {
				return __size;
			}
		}
	}
	return maxOf<size_t>();
}

} // namespace STAPPLER_VERSIONIZED stappler

#endif /* STAPPLER_CORE_STRING_SPBYTESREADER_H_ */
