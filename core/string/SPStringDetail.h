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

#ifndef CORE_CORE_STRING_SPSTRINGDETAIL_H_
#define CORE_CORE_STRING_SPSTRINGDETAIL_H_

#include "SPStringView.h"

namespace STAPPLER_VERSIONIZED stappler::platform {

SP_PUBLIC char32_t tolower(char32_t c);
SP_PUBLIC char32_t toupper(char32_t c);
SP_PUBLIC char32_t totitle(char32_t c);

template <typename Interface>
SP_PUBLIC auto tolower(StringView) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto toupper(StringView) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto totitle(StringView) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto tolower(WideStringView) -> typename Interface::WideStringType;

template <typename Interface>
SP_PUBLIC auto toupper(WideStringView) -> typename Interface::WideStringType;

template <typename Interface>
SP_PUBLIC auto totitle(WideStringView) -> typename Interface::WideStringType;

SP_PUBLIC int compare_u(StringView l, StringView r);
SP_PUBLIC int compare_u(WideStringView l, WideStringView r);

SP_PUBLIC int caseCompare_u(StringView l, StringView r);
SP_PUBLIC int caseCompare_u(WideStringView l, WideStringView r);

}

namespace STAPPLER_VERSIONIZED stappler::string::detail {

static constexpr size_t DOUBLE_MAX_DIGITS = 27;

inline char32_t tolower(char32_t c) { return platform::tolower(c); }
inline char32_t toupper(char32_t c) { return platform::toupper(c); }
inline char32_t totitle(char32_t c) { return platform::totitle(c); }


// fast itoa implementation
// data will be written at the end of buffer, no trailing zero (do not try to use strlen on it!)
// designed to be used with StringView: StringView(buf + bufSize - ret, ret)

// use nullptr buffer to calculate expected buffer length

SP_PUBLIC size_t itoa(int64_t number, char* buffer, size_t bufSize);
SP_PUBLIC size_t itoa(uint64_t number, char* buffer, size_t bufSize);

SP_PUBLIC size_t itoa(int64_t number, char16_t* buffer, size_t bufSize);
SP_PUBLIC size_t itoa(uint64_t number, char16_t* buffer, size_t bufSize);

// fast dtoa implementation
// data will be written from beginning, no trailing zero (do not try to use strlen on it!)
// designed to be used with StringView: StringView(buf, ret)

// use nullptr buffer to calculate expected buffer length

SP_PUBLIC size_t dtoa(double number, char* buffer, size_t bufSize);
SP_PUBLIC size_t dtoa(double number, char16_t* buffer, size_t bufSize);

// read number from string and offset pointers

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

template <typename T, typename Char>
inline auto readNumber(const Char *ptr, size_t len, int base, uint8_t &offset) -> Result<T> {
	// prevent to read out of bounds, copy symbols to stack buffer
	char buf[32] = { 0 }; // int64_t/scientific double character length max
	size_t m = min(size_t(31), len);
	size_t i = 0;
	for (; i < m; i++) {
		auto c = ptr[i];
		if (c < 127) {
			buf[i] = c;
		} else {
			break;
		}
	}

	// read number from internal buffer
	char * ret = nullptr;
	auto val = StringToNumber<T>(buf, &ret, base);
	if (*ret == 0) {
		// while string was used
		offset = i;
	} else if (ret && ret != buf) {
		// part of string was used
		offset = ret - buf;
	} else {
		// fail to read number
		offset = 0;
		return Result<T>();
	}
	return Result<T>(val);
}


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

template <typename CharType>
inline int compare(const CharType *l, const CharType *r, size_t len) {
	return std::use_facet<std::collate<CharType>>(std::locale::classic()).compare(l, l + len,r, r + len);
}

template <typename L, typename R, typename CharType>
inline int caseCompare_c(const L &l, const R &r) {
	auto __lsize = l.size();
	auto __rsize = r.size();
	auto __len = std::min(__lsize, __rsize);
	auto ret = compare(l.data(), r.data(), __len);
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




template <typename FunctionalStreamArg>
struct FunctionalStreamCharTraits { };

template <typename Char>
struct FunctionalStreamCharTraits<StringViewBase<Char>> {
	using CharType = Char;
};

template <>
struct FunctionalStreamCharTraits<StringViewUtf8> {
	using CharType = StringViewUtf8::CharType;
};

template <typename FunctionalStream>
struct FunctionalStreamTraits { };

template <typename Arg>
struct FunctionalStreamTraits<Callback<void(Arg)>> {
	using ArgType = Arg;
	using CharType = typename FunctionalStreamCharTraits<ArgType>::CharType;
};

template <typename Arg>
struct FunctionalStreamTraits<std::function<void(Arg)>> {
	using ArgType = Arg;
	using CharType = typename FunctionalStreamCharTraits<ArgType>::CharType;
};

template <typename Arg>
struct FunctionalStreamTraits<memory::function<void(Arg)>> {
	using ArgType = Arg;
	using CharType = typename FunctionalStreamCharTraits<ArgType>::CharType;
};

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType str) {
	stream(str);
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, const typename FunctionalStreamTraits<FunctionalStream>::CharType *str) {
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(str));
}

template <typename FunctionalStream, size_t N>
inline void streamWrite(const FunctionalStream &stream, const typename FunctionalStreamTraits<FunctionalStream>::CharType str[N]) {
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(str, N));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, double d) {
	std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, string::detail::DOUBLE_MAX_DIGITS> buf = { 0 };
	auto ret = string::detail::dtoa(d, buf.data(), buf.size());
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data(), ret));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, float f) {
	streamWrite(stream, double(f));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, int64_t i) {
	std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, std::numeric_limits<int64_t>::digits10 + 2> buf = { 0 };
	auto ret = string::detail::itoa(i, buf.data(), buf.size());
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data() + buf.size() - ret, ret));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, uint64_t i) {
	std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, std::numeric_limits<int64_t>::digits10 + 2> buf = { 0 };
	auto ret = string::detail::itoa(i, buf.data(), buf.size());
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data() + buf.size() - ret, ret));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, int32_t i) {
	streamWrite(stream, int64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, uint32_t i) {
	streamWrite(stream, uint64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, int16_t i) {
	streamWrite(stream, int64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, uint16_t i) {
	streamWrite(stream, uint64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, int8_t i) {
	streamWrite(stream, int64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, uint8_t i) {
	streamWrite(stream, uint64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, char32_t c) {
	if constexpr (sizeof(typename FunctionalStreamTraits<FunctionalStream>::CharType) == 1) {
		std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, 6> buf = { 0 };
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data(), unicode::utf8EncodeBuf(buf.data(), c)));
	} else {
		std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, 6> buf = { 0 };
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data(), unicode::utf16EncodeBuf(buf.data(), c)));
	}
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, char16_t c) {
	if constexpr (sizeof(typename FunctionalStreamTraits<FunctionalStream>::CharType) == 1) {
		std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, 4> buf = { 0 };
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data(), unicode::utf8EncodeBuf(buf.data(), c)));
	} else {
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(&c, 1));
	}
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, char c) {
	if constexpr (sizeof(typename FunctionalStreamTraits<FunctionalStream>::CharType) == 1) {
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(&c, 1));
	} else {
		char16_t ch = c;
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(&ch, 1));
	}
}

}

#endif /* CORE_CORE_STRING_SPSTRINGDETAIL_H_ */
