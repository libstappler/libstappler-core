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

#ifndef STAPPLER_CORE_STRING_SPSTRINGVIEW_H_
#define STAPPLER_CORE_STRING_SPSTRINGVIEW_H_

#include "SPMemInterface.h"
#include "SPBytesReader.h"
#include "SPUnicode.h"

namespace STAPPLER_VERSIONIZED stappler {

// Fast reader for char string
// Matching function based on templates
//
// Usage:
//   using StringView::Chars;
//   using StringView::Range;
//
//   reader.readUntil<Chars<' ', '\n', '\r', '\t'>>();
//   reader.readChars<Chars<'-', '+', '.', 'e'>, Range<'0', '9'>>();
//

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

	template <CharType ... Args>
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

	template <typename Interface, typename ... Args>
	static auto merge(Args && ... args) -> typename Interface::template BasicStringType<CharType>;

	template <typename Interface, _CharType c, typename ... Args>
	static auto merge(Args && ... args) -> typename Interface::template BasicStringType<CharType>;

	constexpr StringViewBase();
	constexpr StringViewBase(const CharType *ptr, size_t len = maxOf<size_t>());
	constexpr StringViewBase(const CharType *ptr, size_t pos, size_t len);
	constexpr StringViewBase(const Self &, size_t pos, size_t len);
	constexpr StringViewBase(const Self &, size_t len);
	StringViewBase(const PoolString &str);
	StringViewBase(const StdString &str);

	Self & operator =(const PoolString &str);
	Self & operator =(const StdString &str);
	Self & operator =(const Self &str);

	Self & set(const PoolString &str);
	Self & set(const StdString &str);
	Self & set(const Self &str);

	// unsafe set, without length-check
	Self & set(const CharType *p, size_t l);

	bool is(const CharType &c) const;
	bool is(const CharType *c) const;
	bool is(const Self &c) const;

	template <_CharType C> bool is() const;
	template <CharGroupId G> bool is() const;
	template <typename M> bool is() const;

	Self sub(size_t pos = 0, size_t len = maxOf<size_t>()) const { return StringViewBase(*this, pos, len); }

	Self pdup(memory::pool_t * = nullptr) const;

	Self ptolower_c(memory::pool_t * = nullptr) const;
	Self ptoupper_c(memory::pool_t * = nullptr) const;

	template <typename Interface>
	auto str() const -> typename Interface::template BasicStringType<CharType>;

	Self & operator ++ ();
	Self operator ++ (int);
	Self & operator += (size_t l);

	Self begin() const;
	Self end() const;

	Self operator - (const Self &) const;
	Self& operator -= (const Self &) const;

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
	template<typename ... Args> void skipChars();
	template<typename ... Args> void skipUntil();

	template<typename ... Args> void backwardSkipChars();
	template<typename ... Args> void backwardSkipUntil();

	bool skipString(const Self &str);
	bool skipUntilString(const Self &str, bool stopBeforeString = true);

	template<typename ... Args> Self readChars();
	template<typename ... Args> Self readUntil();

	template<typename ... Args> Self backwardReadChars();
	template<typename ... Args> Self backwardReadUntil();

	Self readUntilString(const Self &str);

	template<typename Separator, typename Callback> void split(const Callback &cb) const;

	template <typename ... Args> void trimChars();
	template <typename ... Args> void trimUntil();

protected:
    template <typename T>
    static size_t __size(const T &);

    static size_t __size(const CharType *);

	template <typename T, typename ... Args>
	static size_t _size(T &&);

	template <typename T, typename ... Args>
	static size_t _size(T &&, Args && ... args);

	template <typename Buf, typename T>
	static void __merge(Buf &, T &&t);

	template <typename Buf>
	static void __merge(Buf &, const CharType *);

	template <typename Buf, typename T, typename ... Args>
	static void _merge(Buf &, T &&, Args && ... args);

	template <typename Buf, typename T>
	static void _merge(Buf &, T &&);

	template <typename Buf, _CharType C, bool Front, typename T>
	static void __mergeWithSep(Buf &, T &&t);

	template <typename Buf, _CharType C, bool Front, typename T, typename ... Args>
	static void _mergeWithSep(Buf &, T &&, Args && ... args);

	template <typename Buf, _CharType C, bool Front, typename T>
	static void _mergeWithSep(Buf &, T &&);

	template <typename ... Args> bool match (CharType c);
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

	template <MatchCharType ... Args>
	using MatchChars = chars::Chars<MatchCharType, Args...>;

	template <char First, char Last>
	using MatchRange = chars::Range<MatchCharType, First, Last>;

	template <CharGroupId Group>
	using MatchCharGroup = chars::CharGroup<MatchCharType, Group>;

	template <typename ... Args>
	using MatchCompose = chars::Compose<MatchCharType, Args ...>;

	template <MatchCharType ... Args>
	using Chars = chars::Chars<MatchCharType, Args...>;

	template <char First, char Last>
	using Range = chars::Range<MatchCharType, First, Last>;

	template <CharGroupId Group>
	using CharGroup = chars::CharGroup<MatchCharType, Group>;

	template <typename ... Args>
	using Compose = chars::Compose<MatchCharType, Args ...>;

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

	Self & operator =(const PoolString &str);
	Self & operator =(const StdString &str);
	Self & operator =(const Self &str);

	Self & set(const PoolString &str);
	Self & set(const StdString &str);
	Self & set(const Self &str);
	Self & set(const char *p, size_t l);

	bool is(const char &c) const;
	bool is(const char16_t &c) const;
	bool is(const char32_t &c) const;
	bool is(const char *c) const;
	bool is(const Self &c) const;

	template <char32_t C> bool is() const;
	template <CharGroupId G> bool is() const;
	template <typename M> bool is() const;

	Self sub(size_t pos = 0, size_t len = maxOf<size_t>()) const { return StringViewUtf8(*this, pos, len); }

	Self letter() const;

	template <typename Interface>
	auto str() const -> typename Interface::StringType;

	void offset(size_t l);
	Self & operator ++ ();
	Self operator ++ (int);
	Self & operator += (size_t l);

	bool isSpace() const;

	Self begin() const;
	Self end() const;

	Self operator - (const Self &) const;
	Self& operator -= (const Self &);

	MatchCharType operator * () const;

	template <typename Callback>
	void foreach(const Callback &cb) const;

	size_t code_size() const;

	operator StringViewBase<char> () const;

	uint64_t hash() const {
		return hash::hash64(data(), size() * sizeof(CharType));
	}

	uint64_t hash32() const {
		return hash::hash32(data(), uint32_t(size() * sizeof(CharType)));
	}

public:
	Result<float> readFloat();
	Result<double> readDouble();
	Result<int64_t> readInteger(int base = 0);

public:
	template<typename ... Args> void skipChars();
	template<typename ... Args> void skipUntil();

	template<typename ... Args> void backwardSkipChars();
	template<typename ... Args> void backwardSkipUntil();

	bool skipString(const Self &str);
	bool skipUntilString(const Self &str, bool stopBeforeString = true);

	template<typename ... Args> Self readChars();
	template<typename ... Args> Self readUntil();

	template<typename ... Args> Self backwardReadChars();
	template<typename ... Args> Self backwardReadUntil();

	Self readUntilString(const Self &str);
	template<typename Separator, typename Callback> void split(const Callback &cb) const;

	template <typename ... Args> void trimChars();
    template <typename ... Args> void trimUntil();

protected: // char-matching inline functions
    template <typename ...Args> bool rv_match_utf8 (const CharType *ptr, size_t len, uint8_t &offset);
	template <typename ...Args> bool match (MatchCharType c);
};

using StringView = StringViewBase<char>;
using WideStringView = StringViewBase<char16_t>;

}

namespace STAPPLER_VERSIONIZED stappler::platform {

template <typename Interface>
auto tolower(StringView) -> typename Interface::StringType;

template <typename Interface>
auto toupper(StringView) -> typename Interface::StringType;

template <typename Interface>
auto totitle(StringView) -> typename Interface::StringType;

template <typename Interface>
auto tolower(WideStringView) -> typename Interface::WideStringType;

template <typename Interface>
auto toupper(WideStringView) -> typename Interface::WideStringType;

template <typename Interface>
auto totitle(WideStringView) -> typename Interface::WideStringType;

int compare_u(StringView l, StringView r);
int compare_u(WideStringView l, WideStringView r);

int caseCompare_u(StringView l, StringView r);
int caseCompare_u(WideStringView l, WideStringView r);

}

namespace STAPPLER_VERSIONIZED stappler::string {

template <typename L, typename R, typename CharType
	= typename std::enable_if<
		std::is_same< typename L::value_type, typename R::value_type >::value,
		typename L::value_type>::type>
inline int compare_c(const L &l, const R &r);

template <typename L, typename R, typename CharType
	= typename std::enable_if<
		std::is_same< typename L::value_type, typename R::value_type >::value,
		typename L::value_type>::type>
inline int compare_u(const L &l, const R &r);

template <typename L, typename R, typename CharType
	= typename std::enable_if<
		std::is_same< typename L::value_type, typename R::value_type >::value,
		typename L::value_type>::type>
inline int caseCompare_c(const L &l, const R &r);

template <typename L, typename R, typename CharType
	= typename std::enable_if<
		std::is_same< typename L::value_type, typename R::value_type >::value,
		typename L::value_type>::type>
inline int caseCompare_u(const L &l, const R &r);

template<typename _CharT>
constexpr size_t length(const _CharT *__p) {
	if (!__p) {
		return 0;
	}

	return std::char_traits<_CharT>::length(__p);
}

template<typename _CharT>
constexpr size_t length(const _CharT *__p, size_t max) {
	if (!__p) {
		return 0;
	}

	if (max == maxOf<size_t>()) {
		return std::char_traits<_CharT>::length(__p);
	} else {
		size_t __i = 0;
		while (__i < max && __p[__i] != _CharT()) {
			++__i;
		}
		return __i;
	}
}

}


namespace STAPPLER_VERSIONIZED stappler {

template <typename C> inline std::basic_ostream<C> &
operator << (std::basic_ostream<C> & os, const StringViewBase<C> & str) {
	return os.write(str.data(), str.size());
}

inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const StringViewUtf8 & str) {
	return os.write(str.data(), str.size());
}

#define SP_STRINGVIEW_COMPARE_OP(Type1, Type2) \
	template <typename C> inline bool operator == (const Type1 &l, const Type2 &r) { return string::compare_c(l, r) == 0; } \
	template <typename C> inline bool operator != (const Type1 &l, const Type2 &r) { return string::compare_c(l, r) != 0; } \
	template <typename C> inline bool operator > (const Type1 &l, const Type2 &r) { return string::compare_c(l, r) > 0; } \
	template <typename C> inline bool operator >= (const Type1 &l, const Type2 &r) { return string::compare_c(l, r) >= 0; } \
	template <typename C> inline bool operator < (const Type1 &l, const Type2 &r) { return string::compare_c(l, r) < 0; } \
	template <typename C> inline bool operator <= (const Type1 &l, const Type2 &r) { return string::compare_c(l, r) <= 0; }

#define SP_STRINGVIEW_COMPARE_OP_UTF8(Type1, Type2) \
	inline bool operator == (const Type1 &l, const Type2 &r) { return string::compare_c /* not an error! It's faster */ (l, r) == 0; } \
	inline bool operator != (const Type1 &l, const Type2 &r) { return string::compare_c /* not an error! It's faster */ (l, r) != 0; } \
	inline bool operator > (const Type1 &l, const Type2 &r) { return string::compare_u(l, r) > 0; } \
	inline bool operator >= (const Type1 &l, const Type2 &r) { return string::compare_u(l, r) >= 0; } \
	inline bool operator < (const Type1 &l, const Type2 &r) { return string::compare_u(l, r) < 0; } \
	inline bool operator <= (const Type1 &l, const Type2 &r) { return string::compare_u(l, r) <= 0; }

SP_STRINGVIEW_COMPARE_OP(StringViewBase<C>, StringViewBase<C>)

SP_STRINGVIEW_COMPARE_OP(memory::StandartInterface::BasicStringType<C>, StringViewBase<C>)
SP_STRINGVIEW_COMPARE_OP(StringViewBase<C>, memory::StandartInterface::BasicStringType<C>)

SP_STRINGVIEW_COMPARE_OP(memory::PoolInterface::BasicStringType<C>, StringViewBase<C>)
SP_STRINGVIEW_COMPARE_OP(StringViewBase<C>, memory::PoolInterface::BasicStringType<C>)

SP_STRINGVIEW_COMPARE_OP_UTF8(StringViewUtf8, StringViewUtf8)

SP_STRINGVIEW_COMPARE_OP_UTF8(memory::StandartInterface::BasicStringType<char>, StringViewUtf8)
SP_STRINGVIEW_COMPARE_OP_UTF8(StringViewUtf8, memory::StandartInterface::BasicStringType<char>)

SP_STRINGVIEW_COMPARE_OP_UTF8(memory::PoolInterface::BasicStringType<char>, StringViewUtf8)
SP_STRINGVIEW_COMPARE_OP_UTF8(StringViewUtf8, memory::PoolInterface::BasicStringType<char>)

template <typename C> inline bool operator == (const StringViewBase<C> &l, const C *r) { return string::compare_c(l, StringViewBase<C>(r)) == 0; }
template <typename C> inline bool operator != (const StringViewBase<C> &l, const C *r) { return string::compare_c(l, StringViewBase<C>(r)) != 0; }

template <typename C> inline bool operator == (const C *l, const StringViewBase<C> &r) { return string::compare_c(StringViewBase<C>(l), r) == 0; }
template <typename C> inline bool operator != (const C *l, const StringViewBase<C> &r) { return string::compare_c(StringViewBase<C>(l), r) != 0; }

inline bool operator == (const StringViewUtf8 &l, const char *r) { return string::compare_c(l, StringViewUtf8(r)) == 0; }
inline bool operator != (const StringViewUtf8 &l, const char *r) { return string::compare_c(l, StringViewUtf8(r)) != 0; }

inline bool operator == (const char *l, const StringViewUtf8 &r) { return string::compare_c(StringViewUtf8(l), r) == 0; }
inline bool operator != (const char *l, const StringViewUtf8 &r) { return string::compare_c(StringViewUtf8(l), r) != 0; }

#undef SP_STRINGVIEW_COMPARE_OP
#undef SP_STRINGVIEW_COMPARE_OP_UTF8

template <typename _CharType>
template <typename Interface, typename ... Args>
auto StringViewBase<_CharType>::merge(Args && ... args) -> typename Interface::template BasicStringType<CharType> {
	using StringType = typename Interface::template BasicStringType<CharType>;

	StringType ret; ret.reserve(_size(forward<Args>(args)...));
	_merge(ret, forward<Args>(args)...);
	return ret;
}

template <typename _CharType>
template <typename Interface, _CharType C, typename ... Args>
auto StringViewBase<_CharType>::merge(Args && ... args) -> typename Interface::template BasicStringType<CharType> {
	using StringType = typename Interface::template BasicStringType<CharType>;

	StringType ret; ret.reserve(_size(forward<Args>(args)...) + sizeof...(Args));
	_mergeWithSep<StringType, C, true>(ret, forward<Args>(args)...);
	return ret;
}


template <typename _CharType>
template <typename T>
inline size_t StringViewBase<_CharType>::__size(const T &t) {
	return t.size();
}

template <typename _CharType>
inline size_t StringViewBase<_CharType>::__size(const CharType *c) {
	return std::char_traits<_CharType>::length(c);
}

template <typename _CharType>
template <typename T, typename ... Args>
inline size_t StringViewBase<_CharType>::_size(T &&t) {
	return __size(t);
}

template <typename _CharType>
template <typename T, typename ... Args>
inline size_t StringViewBase<_CharType>::_size(T &&t, Args && ... args) {
	return __size(t) + _size(forward<Args>(args)...);
}

template <typename _CharType>
template <typename Buf, typename T>
inline void StringViewBase<_CharType>::__merge(Buf &buf, T &&t) {
	if (!t.empty()) {
		buf.append(t.data(), t.size());
	}
}

template <typename _CharType>
template <typename Buf>
inline void StringViewBase<_CharType>::__merge(Buf &buf, const CharType *c) {
	if (c) {
		buf.append(c);
	}
}

template <typename _CharType>
template <typename Buf, typename T, typename ... Args>
inline void StringViewBase<_CharType>::_merge(Buf &buf, T &&t, Args && ... args) {
	__merge(buf, std::forward<T>(t));
	_merge(buf, forward<Args>(args)...);
}

template <typename _CharType>
template <typename Buf, typename T>
inline void StringViewBase<_CharType>::_merge(Buf &buf, T &&t) {
	__merge(buf, std::forward<T>(t));
}

template <typename _CharType>
template <typename Buf, _CharType C, bool Front, typename T>
inline void StringViewBase<_CharType>::__mergeWithSep(Buf &buf, T &&t) {
	Self tmp(t);
	tmp.trimChars<typename Self::template Chars<C>>();
	if (!tmp.empty()) {
		if constexpr (Front) {
			tmp = Self(t);
			tmp.backwardSkipChars<typename Self::template Chars<C>>();

			buf.append(tmp.data(), tmp.size());
		} else {
			tmp = Self(t);
			tmp.trimChars<typename Self::template Chars<C>>();

			if (!buf.empty()) {
				buf.push_back(C);
			}

			buf.append(tmp.data(), tmp.size());
		}
	}
}

template <typename _CharType>
template <typename Buf, _CharType C, bool Front, typename T, typename ... Args>
inline void StringViewBase<_CharType>::_mergeWithSep(Buf &buf, T &&t, Args && ... args) {
	__mergeWithSep<Buf, C, Front>(buf, std::forward<T>(t));
	_mergeWithSep<Buf, C, false>(buf, std::forward<Args>(args)...);
}

template <typename _CharType>
template <typename Buf, _CharType C, bool Front, typename T>
inline void StringViewBase<_CharType>::_mergeWithSep(Buf &buf, T &&t) {
	__mergeWithSep<Buf, C, Front>(buf, std::forward<T>(t));
}


template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase() : BytesReader<_CharType>(nullptr, 0) { }

template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase(const CharType *ptr, size_t len)
: BytesReader<_CharType>(ptr, string::length(ptr, len)) { }

template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase(const CharType *ptr, size_t pos, size_t len)
: BytesReader<_CharType>(ptr + pos, string::length(ptr + pos, len)) { }

template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase(const Self &ptr, size_t pos, size_t len)
: BytesReader<_CharType>(ptr.data() + pos, min(len, ptr.size() - pos)) { }

template <typename _CharType>
inline constexpr StringViewBase<_CharType>::StringViewBase(const Self &ptr, size_t len)
: BytesReader<_CharType>(ptr.data(), min(len, ptr.size())) { }

template <typename _CharType>
StringViewBase<_CharType>::StringViewBase(const PoolString &str)
: StringViewBase(str.data(), str.size()) { }

template <typename _CharType>
StringViewBase<_CharType>::StringViewBase(const StdString &str)
: StringViewBase(str.data(), str.size()) { }

template <typename _CharType>
auto StringViewBase<_CharType>::operator =(const PoolString &str) -> Self & {
	this->set(str);
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator =(const StdString &str)-> Self & {
	this->set(str);
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator =(const Self &str)-> Self & {
	this->set(str);
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::set(const PoolString &str)-> Self & {
	this->set(str.data(),str.size());
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::set(const StdString &str)-> Self & {
	this->set(str.data(), str.size());
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::set(const Self &str)-> Self & {
	this->set(str.data(), str.size());
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::set(const CharType *p, size_t l)-> Self & {
	this->ptr = p;
	this->len = l;
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::pdup(memory::pool_t *p) const -> Self {
	if (!p) {
		p = memory::pool::acquire();
	}
	auto buf = (_CharType *)memory::pool::palloc(p, (this->size() + 1) * sizeof(_CharType));
	memcpy(buf, this->data(), this->size() * sizeof(_CharType));
	buf[this->size()] = 0;
	return Self(buf, this->size());
}

template <typename _CharType>
auto StringViewBase<_CharType>::ptolower_c(memory::pool_t *p) const -> Self {
	if (!p) {
		p = memory::pool::acquire();
	}
	auto buf = (_CharType *)memory::pool::palloc(p, (this->size() + 1) * sizeof(_CharType));
	memcpy(buf, this->data(), this->size() * sizeof(_CharType));
	for (size_t i = 0; i < this->size(); ++ i) {
		buf[i] = std::tolower(buf[i], std::locale());
	}
	buf[this->size()] = 0;
	return Self(buf, this->size());
}

template <typename _CharType>
auto StringViewBase<_CharType>::ptoupper_c(memory::pool_t *p) const -> Self {
	if (!p) {
		p = memory::pool::acquire();
	}
	auto buf = (_CharType *)memory::pool::palloc(p, (this->size() + 1) * sizeof(_CharType));
	memcpy(buf, this->data(), this->size() * sizeof(_CharType));
	for (size_t i = 0; i < this->size(); ++ i) {
		buf[i] = std::toupper(buf[i], std::locale());
	}
	buf[this->size()] = 0;
	return Self(buf, this->size());
}

template <typename _CharType>
template <typename Interface>
auto StringViewBase<_CharType>::str() const -> typename Interface::template BasicStringType<CharType> {
	if (this->ptr && this->len > 0) {
		return typename Interface::template BasicStringType<CharType>(this->ptr, this->len);
	} else {
		return typename Interface::template BasicStringType<CharType>();
	}
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator ++ () -> Self & {
	if (!this->empty()) {
		this->ptr ++; this->len --;
	}
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator ++ (int) -> Self {
	auto tmp = *this;
	if (!this->empty()) {
		this->ptr ++; this->len --;
	}
	return tmp;
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator += (size_t l) -> Self & {
	this->offset(l);
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::is(const CharType &c) const -> bool {
	return this->len > 0 && *this->ptr == c;
}

template <typename _CharType>
auto StringViewBase<_CharType>::is(const CharType *c) const -> bool {
	return this->prefix(c, TraitsType::length(c));
}

template <typename _CharType>
auto StringViewBase<_CharType>::is(const Self &c) const -> bool {
	return this->prefix(c.data(), c.size());
}

template <typename _CharType>
template <_CharType C>
auto StringViewBase<_CharType>::is() const -> bool {
	return this->len > 0 && *this->ptr ==C;
}

template <typename _CharType>
template <CharGroupId G>
auto StringViewBase<_CharType>::is() const -> bool {
	return this->len > 0 && MatchCharGroup<G>::match(*this->ptr);
}

template <typename _CharType>
template <typename M>
auto StringViewBase<_CharType>::is() const -> bool {
	return this->len > 0 && M::match(*this->ptr);
}

template <typename _CharType>
auto StringViewBase<_CharType>::begin() const -> Self {
	return Self(this->ptr, this->len);
}

template <typename _CharType>
auto StringViewBase<_CharType>::end() const -> Self {
	return Self(this->ptr + this->len, 0);
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator - (const Self &other) const -> Self {
	if (this->ptr > other.ptr && size_t(this->ptr - other.ptr) < this->len) {
		return Self(this->ptr, this->ptr - other.ptr);
	}
	return Self();
}

template <typename _CharType>
auto StringViewBase<_CharType>::operator -= (const Self &other) const -> Self & {
	if (this->ptr > other.ptr && size_t(this->ptr - other.ptr) < this->len) {
		this->len = this->ptr - other.ptr;
		return *this;
	}
	return *this;
}

template <typename _CharType>
auto StringViewBase<_CharType>::readFloat() -> Result<float> {
	Self tmp = *this;
	tmp.skipChars<typename Self::template CharGroup<CharGroupId::WhiteSpace>>();
	uint8_t offset = 0;
	auto ret = string::readNumber<float>(tmp.ptr, tmp.len, 0, offset);
	this->ptr += offset; this->len -= offset;
	return ret;
}

template <typename _CharType>
auto StringViewBase<_CharType>::readDouble() -> Result<double> {
	Self tmp = *this;
	tmp.skipChars<typename Self::template CharGroup<CharGroupId::WhiteSpace>>();
	uint8_t offset = 0;
	auto ret = string::readNumber<double>(tmp.ptr, tmp.len, 0, offset);
	this->ptr += offset; this->len -= offset;
	return ret;
}

template <typename _CharType>
auto StringViewBase<_CharType>::readInteger(int base) -> Result<int64_t> {
	Self tmp = *this;
	tmp.skipChars<typename Self::template CharGroup<CharGroupId::WhiteSpace>>();
	uint8_t offset = 0;
	auto ret = string::readNumber<int64_t>(tmp.ptr, tmp.len, 0, offset);
	this->ptr += offset; this->len -= offset;
	return ret;
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::skipChars() -> void {
	size_t offset = 0;
	while (this->len > offset && match<Args...>(this->ptr[offset])) {
		++offset;
	}
	auto off = std::min(offset, this->len);
	this->len -= off;
	this->ptr += off;
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::skipUntil() -> void {
	size_t offset = 0;
	while (this->len > offset && !match<Args...>(this->ptr[offset])) {
		++offset;
	}
	auto off = std::min(offset, this->len);
	this->len -= off;
	this->ptr += off;
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::backwardSkipChars() -> void {
	while (this->len > 0 && match<Args...>(this->ptr[this->len - 1])) {
		-- this->len;
	}
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::backwardSkipUntil() -> void {
	while (this->len > 0 && !match<Args...>(this->ptr[this->len - 1])) {
		-- this->len;
	}
}

template <typename _CharType>
auto StringViewBase<_CharType>::skipString(const Self &str) -> bool {
	if (!this->ptr) {
		return false;
	}
	if (this->prefix(str.data(), str.size())) {
		auto s = std::min(str.size(), this->len);
		this->ptr += s;
		this->len -= s;
		return true;
	}
	return false;
}

template <typename _CharType>
auto StringViewBase<_CharType>::skipUntilString(const Self &str, bool stopBeforeString) -> bool {
	if (!this->ptr) {
		return false;
	}

	while (this->len > 0 && !this->prefix(str.data(), str.size())) {
		this->ptr += 1;
		this->len -= 1;
	}
	if (this->len > 0 && *this->ptr != 0 && !stopBeforeString) {
		skipString(str);
	}

	return this->len > 0 && *this->ptr != 0;
}

template <typename _CharType>
template <typename ... Args>
auto StringViewBase<_CharType>::readChars() -> Self {
	auto tmp = *this;
	skipChars<Args ...>();
	return Self(tmp.data(), tmp.size() - this->size());
}

template <typename _CharType>
template <typename ... Args>
auto StringViewBase<_CharType>::readUntil() -> Self {
	auto tmp = *this;
	skipUntil<Args ...>();
	return Self(tmp.data(), tmp.size() - this->size());
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::backwardReadChars() -> Self {
	auto tmp = *this;
	backwardSkipChars<Args ...>();
	return Self(this->data() + this->size(), tmp.size() - this->size());
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::backwardReadUntil() -> Self {
	auto tmp = *this;
	backwardSkipUntil<Args ...>();
	return Self(this->data() + this->size(), tmp.size() - this->size());
}

template <typename _CharType>
auto StringViewBase<_CharType>::readUntilString(const Self &str) -> Self {
	auto tmp = *this;
	skipUntilString(str);
	return Self(tmp.data(), tmp.size() - this->size());
}

template <typename _CharType>
template<typename Separator, typename Callback>
auto StringViewBase<_CharType>::split(const Callback &cb) const -> void {
	Self str(*this);
	while (!str.empty()) {
		str.skipChars<Separator>();
		auto tmp = str.readUntil<Separator>();
		if (!tmp.empty()) {
			cb(tmp);
		}
	}
}

template <typename _CharType>
template<typename ... Args>
auto StringViewBase<_CharType>::trimChars() -> void {
	this->skipChars<Args...>();
	if (!this->empty()) {
		this->backwardSkipChars<Args...>();
	}
}

template <typename _CharType>
template <typename... Args>
auto StringViewBase<_CharType>::trimUntil() -> void {
	this->skipUntil<Args...>();
	if (!this->empty()) {
		this->backwardSkipUntil<Args...>();
	}
}

template <typename _CharType>
template <typename ...Args>
auto StringViewBase<_CharType>::match (CharType c) -> bool {
	return chars::Compose<CharType, Args...>::match(c);
}


inline StringViewUtf8::StringViewUtf8() : BytesReader(nullptr, 0) { }

inline StringViewUtf8::StringViewUtf8(const char *ptr, size_t len)
: BytesReader(ptr, string::length(ptr, len)) { }

inline StringViewUtf8::StringViewUtf8(const char *ptr, size_t pos, size_t len)
: BytesReader(ptr + pos, string::length(ptr + pos, len)) { }

inline StringViewUtf8::StringViewUtf8(const StringViewUtf8 &ptr, size_t len)
: StringViewUtf8(ptr, 0, len) { }

inline StringViewUtf8::StringViewUtf8(const StringViewUtf8 &ptr, size_t pos, size_t len)
: BytesReader(ptr.data() + pos, min(len, ptr.size() - pos)) { }

inline StringViewUtf8::StringViewUtf8(const PoolString &str)
: StringViewUtf8(str.data(), str.size()) { }

inline StringViewUtf8::StringViewUtf8(const StdString &str)
: StringViewUtf8(str.data(), str.size()) { }

inline StringViewUtf8::StringViewUtf8(const StringViewBase<char> &str)
: StringViewUtf8(str.data(), str.size()) { }

inline auto StringViewUtf8::operator =(const PoolString &str) -> Self & {
	this->set(str);
	return *this;
}
inline auto StringViewUtf8::operator =(const StdString &str) -> Self & {
	this->set(str);
	return *this;
}
inline auto StringViewUtf8::operator =(const Self &str) -> Self & {
	this->set(str);
	return *this;
}

inline auto StringViewUtf8::set(const PoolString &str) -> Self & {
	this->set(str.data(), str.size());
	return *this;
}
inline auto StringViewUtf8::set(const StdString &str) -> Self & {
	this->set(str.data(), str.size());
	return *this;
}
inline auto StringViewUtf8::set(const Self &str) -> Self & {
	this->set(str.data(), str.size());
	return *this;
}
inline auto StringViewUtf8::set(const char *p, size_t l) -> Self & {
	ptr = p; len = l;
	return *this;
}

inline bool StringViewUtf8::is(const char &c) const {
	return len > 0 && *ptr == c;
}
inline bool StringViewUtf8::is(const char16_t &c) const {
	return len > 0 && len >= unicode::utf8_length_data[uint8_t(*ptr)] && unicode::utf8Decode32(ptr) == c;
}
inline bool StringViewUtf8::is(const char32_t &c) const {
	return len > 0 && len >= unicode::utf8_length_data[uint8_t(*ptr)] && unicode::utf8Decode32(ptr) == c;
}
inline bool StringViewUtf8::is(const char *c) const {
	return prefix(c, std::char_traits<char>::length(c));
}
inline bool StringViewUtf8::is(const Self &c) const {
	return prefix(c.data(), c.size());
}

template <char32_t C>
inline bool StringViewUtf8::is() const {
	return len > 0 && len >= unicode::utf8_length_data[uint8_t(*ptr)] && unicode::utf8Decode32(ptr) == C;
}

template <CharGroupId G>
inline bool StringViewUtf8::is() const {
	return len > 0 && len >= unicode::utf8_length_data[uint8_t(*ptr)] && chars::CharGroup<MatchCharType, G>::match(unicode::utf8Decode32(ptr));
}

template <typename M>
inline bool StringViewUtf8::is() const {
	return len > 0 && len >= unicode::utf8_length_data[uint8_t(*ptr)] && M::match(unicode::utf8Decode32(ptr));
}

inline auto StringViewUtf8::letter() const -> Self {
	if (this->len > 0) {
		return Self(this->ptr, std::min(this->len, size_t( unicode::utf8_length_data[uint8_t(*ptr)] )));
	}
	return Self();
}
template <typename Interface>
inline auto StringViewUtf8::str() const -> typename Interface::StringType {
	if (this->ptr && this->len > 0) {
		return typename Interface::StringType(this->ptr, this->len);
	}
	return typename Interface::StringType();
}

// extend offset functions with unicode support
inline void StringViewUtf8::offset(size_t l) {
	while (l > 0 && len > 0) {
		++ (*this); -- l;
	}
}
inline auto StringViewUtf8::operator ++ () -> Self & {
	if (len > 0) {
		auto l = std::min(size_t( unicode::utf8_length_data[uint8_t(*ptr)] ), len);
		ptr += l; len -= l;
	}
	return *this;
}
inline auto StringViewUtf8::operator ++ (int) -> Self {
	auto tmp = *this;
	++ (*this);
	return tmp;
}
inline auto StringViewUtf8::operator += (size_t l) -> Self & {
	offset(l);
	return *this;
}

inline bool StringViewUtf8::isSpace() const {
	auto tmp = *this;
	tmp.skipChars<WhiteSpace>();
	return tmp.empty();
}

inline auto StringViewUtf8::begin() const -> Self {
	return Self(this->ptr, this->len);
}
inline auto StringViewUtf8::end() const -> Self {
	return Self(this->ptr + this->len, 0);
}
inline auto StringViewUtf8::operator - (const Self &other) const -> Self {
	if (this->ptr > other.ptr && size_t(this->ptr - other.ptr) < this->len) {
		return Self(this->ptr, this->ptr - other.ptr);
	}
	return Self();
}
inline auto StringViewUtf8::operator -= (const Self &other) -> Self & {
	if (this->ptr > other.ptr && size_t(this->ptr - other.ptr) < this->len) {
		this->len = this->ptr - other.ptr;
		return *this;
	}
	return *this;
}
inline auto StringViewUtf8::operator * () const -> MatchCharType {
	return unicode::utf8Decode32(ptr);
}

template <typename Callback>
inline void StringViewUtf8::foreach(const Callback &cb) const {
	auto p = ptr;
	const auto e = ptr + len;
	while (p < e) {
		const uint8_t mask = unicode::utf8_length_mask[uint8_t(*p)];
		const uint8_t len = unicode::utf8_length_data[uint8_t(*p)];
		uint32_t ret = *p++ & mask;
		for (uint8_t c = 1; c < len; ++c) {
			const auto ch =  *p++;
			if ((ch & 0xc0) != 0x80) {
				ret = 0;
				break;
			}
			ret <<= 6;
			ret |= (ch & 0x3f);
		}
		cb(MatchCharType(ret));
	}
}

inline size_t StringViewUtf8::code_size() const {
	size_t ret = 0;
	auto p = ptr;
	const auto e = ptr + len;
	while (p < e) {
		++ ret;
		p += unicode::utf8_length_data[uint8_t(*p)];
	}
	return ret;
}

inline StringViewUtf8::operator StringViewBase<char> () const {
	return StringViewBase<char>(ptr, len);
}

inline Result<float> StringViewUtf8::readFloat() {
	Self tmp = *this;
	tmp.skipChars<CharGroup<CharGroupId::WhiteSpace>>();
	uint8_t offset = 0;
	auto ret = string::readNumber<float>(tmp.ptr, tmp.len, 0, offset);
	this->ptr += offset; this->len -= offset;
	return ret;
}
inline Result<double> StringViewUtf8::readDouble() {
	Self tmp = *this;
	tmp.skipChars<CharGroup<CharGroupId::WhiteSpace>>();
	uint8_t offset = 0;
	auto ret = string::readNumber<double>(tmp.ptr, tmp.len, 0, offset);
	this->ptr += offset; this->len -= offset;
	return ret;
}
inline Result<int64_t> StringViewUtf8::readInteger(int base) {
	Self tmp = *this;
	tmp.skipChars<CharGroup<CharGroupId::WhiteSpace>>();
	uint8_t offset = 0;
	auto ret = string::readNumber<int64_t>(tmp.ptr, tmp.len, 0, offset);
	this->ptr += offset; this->len -= offset;
	return ret;
}

template<typename ... Args>
inline void StringViewUtf8::skipChars() {
	uint8_t clen = 0;
	size_t offset = 0;
	while (len > offset && match<Args...>(unicode::utf8Decode32(ptr + offset, clen)) && clen > 0) {
		offset += clen;
	}
	auto off = std::min(offset, len);
	len -= off;
	ptr += off;
}

template<typename ... Args>
inline void StringViewUtf8::skipUntil() {
	uint8_t clen = 0;
	size_t offset = 0;
	while (len > offset && !match<Args...>(unicode::utf8Decode32(ptr + offset, clen)) && clen > 0) {
		offset += clen;
	}
	auto off = std::min(offset, len);
	len -= off;
	ptr += off;
}

template<typename ... Args>
inline void StringViewUtf8::backwardSkipChars() {
	uint8_t clen = 0;
	while (this->len > 0 && rv_match_utf8<Args...>(this->ptr, this->len, clen)) {
		if (clen > 0) {
			this->len -= std::min(size_t(clen), this->len);
		} else {
			return;
		}
	}
}

template<typename ... Args>
inline void StringViewUtf8::backwardSkipUntil() {
	uint8_t clen = 0;
	while (this->len > 0 && !rv_match_utf8<Args...>(this->ptr, this->len, clen)) {
		if (clen > 0) {
			this->len -= std::min(size_t(clen), this->len);;
		} else {
			return;
		}
	}
}

inline bool StringViewUtf8::skipString(const Self &str) {
	if (!ptr) {
		return false;
	}
	if (this->prefix(str.data(), str.size())) {
		auto s = std::min(len, str.size());
		ptr += s;
		len -= s;
		return true;
	}
	return false;
}
inline bool StringViewUtf8::skipUntilString(const Self &str, bool stopBeforeString) {
	if (!ptr) {
		return false;
	}

	while (this->len > 0 && !this->prefix(str.data(), str.size())) {
		this->ptr += 1;
		this->len -= 1;
	}
	if (this->len > 0 && *this->ptr != 0 && !stopBeforeString) {
		skipString(str);
	}

	return this->len > 0 && *this->ptr != 0;
}

template<typename ... Args>
inline auto StringViewUtf8::readChars() -> Self {
	auto tmp = *this;
	skipChars<Args ...>();
	return Self(tmp.data(), tmp.size() - this->size());
}

template<typename ... Args>
inline auto StringViewUtf8::readUntil() -> Self {
	auto tmp = *this;
	skipUntil<Args ...>();
	return Self(tmp.data(), tmp.size() - this->size());
}

template<typename ... Args>
inline auto StringViewUtf8::backwardReadChars() -> Self {
	auto tmp = *this;
	backwardSkipChars<Args ...>();
	return Self(this->data() + this->size(), tmp.size() - this->size());
}

template<typename ... Args>
inline auto StringViewUtf8::backwardReadUntil() -> Self {
	auto tmp = *this;
	backwardSkipUntil<Args ...>();
	return Self(this->data() + this->size(), tmp.size() - this->size());
}

inline auto StringViewUtf8::readUntilString(const Self &str) -> Self {
	auto tmp = *this;
	skipUntilString(str);
	return Self(tmp.data(), tmp.size() - this->size());
}

template<typename Separator, typename Callback>
inline void StringViewUtf8::split(const Callback &cb) const {
	Self str(*this);
	while (!str.empty()) {
		str.skipChars<Separator>();
		auto tmp = str.readUntil<Separator>();
		if (!tmp.empty()) {
			cb(tmp);
		}
	}
}

template<typename ... Args>
inline void StringViewUtf8::trimChars() {
	this->skipChars<Args...>();
	if (!this->empty()) {
		this->backwardSkipChars<Args...>();
	}
}

template <typename... Args>
inline void StringViewUtf8::trimUntil() {
	this->skipUntil<Args...>();
	if (!this->empty()) {
		this->backwardSkipUntil<Args...>();
	}
}

template  <typename ...Args>
inline bool StringViewUtf8::rv_match_utf8 (const CharType *ptr, size_t len, uint8_t &offset) {
	while (len > 0) {
		if (!unicode::isUtf8Surrogate(ptr[len - 1])) {
			return match<Args...>(unicode::utf8Decode32(ptr + len - 1, offset));
		} else {
			-- len;
		}
	}
	offset = 0;
	return false;
}

template <typename ...Args>
inline bool StringViewUtf8::match (MatchCharType c) {
	return chars::Compose<MatchCharType, Args...>::match(c);
}

}


namespace STAPPLER_VERSIONIZED stappler::string {

template <typename L, typename R, typename CharType>
inline int compare_c(const L &l, const R &r) {
	auto __lsize = l.size();
	auto __rsize = r.size();
	auto __len = std::min(__lsize, __rsize);
	auto ret = std::char_traits<CharType>::compare(l.data(), r.data(), __len);
	if (!ret) {
		if (__lsize < __rsize) {
			return -1;
		} else if (__lsize == __rsize) {
			return 0;
		} else {
			return 1;
		}
	}
	return ret;
}

template <typename L, typename R, typename CharType>
inline int compare_u(const L &l, const R &r) {
	return platform::compare_u(l, r);
}

inline int _strncasecmp(const char *l, const char *r, size_t len) {
	return ::strncasecmp(l, r, len);
}

inline int _strncasecmp(const char16_t *l, const char16_t *r, size_t len) {
	while (len > 0) {
		auto lc = std::tolower(*l ++);
		auto rc = std::tolower(*r ++);
		if (lc != rc) {
			return (lc < rc)?-1:1;
		}
		-- len;
	}
	return 0;
}

template <typename L, typename R, typename CharType>
inline int caseCompare_c(const L &l, const R &r) {
	auto __lsize = l.size();
	auto __rsize = r.size();
	auto __len = std::min(__lsize, __rsize);
	auto ret = _strncasecmp(l.data(), r.data(), __len);
	if (!ret) {
		if (__lsize < __rsize) {
			return -1;
		} else if (__lsize == __rsize) {
			return 0;
		} else {
			return 1;
		}
	}
	return ret;
}

template <typename L, typename R, typename CharType>
inline int caseCompare_u(const L &l, const R &r) {
	return platform::caseCompare_u(l, r);
}

}


namespace std {

template <>
struct hash<STAPPLER_VERSIONIZED_NAMESPACE::StringView> {
	hash() { }

	size_t operator() (const STAPPLER_VERSIONIZED_NAMESPACE::StringView &value) const noexcept {
		return hash<string_view>()(string_view(value.data(), value.size()));
	}
};

template <>
struct hash<STAPPLER_VERSIONIZED_NAMESPACE::StringViewUtf8> {
	hash() { }

	size_t operator() (const STAPPLER_VERSIONIZED_NAMESPACE::StringViewUtf8 &value) const noexcept {
		return hash<string_view>()(string_view(value.data(), value.size()));
	}
};

template <>
struct hash<STAPPLER_VERSIONIZED_NAMESPACE::WideStringView> {
	hash() { }

	size_t operator() (const STAPPLER_VERSIONIZED_NAMESPACE::WideStringView &value) const noexcept {
		return hash<u16string_view>()(u16string_view(value.data(), value.size()));
	}
};

}

#endif /* STAPPLER_CORE_STRING_SPSTRINGVIEW_H_ */
