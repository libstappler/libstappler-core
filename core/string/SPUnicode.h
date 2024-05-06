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

#ifndef STAPPLER_CORE_STRING_SPUNICODE_H_
#define STAPPLER_CORE_STRING_SPUNICODE_H_

#include "SPMemString.h"

namespace STAPPLER_VERSIONIZED stappler::unicode {

// Length lookup table
constexpr const uint8_t utf8_length_data[256] = {
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};

constexpr const uint8_t utf16_length_data[256] = {
	0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1
};

constexpr const uint8_t utf8_length_mask[256] = {
    0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
    0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f,
	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x03, 0x03, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x7f
};

// check if char is not start of utf8 symbol
SPINLINE constexpr inline bool isUtf8Surrogate(char c)  {
	return (c & 0xC0) == 0x80;
}

constexpr inline char32_t utf8Decode32(const char *ptr, uint8_t &offset) {
	uint8_t mask = utf8_length_mask[uint8_t(*ptr)];
	offset = utf8_length_data[uint8_t(*ptr)];
	char32_t ret = ptr[0] & mask;
	for (uint8_t c = 1; c < offset; ++c) {
		if ((ptr[c] & 0xc0) != 0x80) { ret = 0; break; }
		ret <<= 6; ret |= (ptr[c] & 0x3f);
	}
	return ret;
}

char32_t utf8HtmlDecode32(const char *ptr, uint8_t &offset);

constexpr inline char32_t utf8Decode32(const char *ptr) {
	uint8_t offset;
	return utf8Decode32(ptr, offset);
}

inline constexpr uint8_t utf8EncodeLength(char16_t c) {
	return ( c < 0x80 ? 1
		: ( c < 0x800 ? 2
			:  3
		)
	);
}

inline constexpr uint8_t utf8EncodeLength(char32_t c) {
	if (c < 0x80) {
		return 1;
	} else if (c < 0x800) {
		return 2;
	} else if (c < 0x1'0000) {
		return 3;
	} else if (c < 0x11'0000) {
		return 4;
	} else {
		return 5;
	}
}

template <typename PutCharFn>
inline uint8_t utf8EncodeCb(const PutCharFn &cb, char16_t c) {
	if (c < 0x80) {
		cb(char(c));
		return 1;
	} else if (c < 0x800) {
		cb(0xc0 | (c >> 6));
		cb(0x80 | (c & 0x3f));
		return 2;
	} else {
		cb(0xe0 | (c >> 12));
		cb(0x80 | (c >> 6 & 0x3f));
		cb(0x80 | (c & 0x3f));
		return 3;
	}
}

template <typename PutCharFn>
inline uint8_t utf8EncodeCb(const PutCharFn &cb, char32_t c) {
	if (c < 0x80) {
		cb(char(c));
		return 1;
	} else if (c < 0x800) {
		cb(0xc0 | (c >> 6));
		cb(0x80 | (c & 0x3f));
		return 2;
	} else if (c < 0x1'0000) {
		cb(0b1110'0000 | (c >> 12));
		cb(0x80 | (c >> 6 & 0x3f));
		cb(0x80 | (c & 0x3f));
		return 3;
	} else if (c < 0x11'0000) {
		cb(0b1111'0000 | (c >> 18));
		cb(0x80 | (c >> 12 & 0x3f));
		cb(0x80 | (c >> 6 & 0x3f));
		cb(0x80 | (c & 0x3f));
		return 4;
	} else {
		cb(0b1111'1000 | (c >> 24));
		cb(0x80 | (c >> 18 & 0x3f));
		cb(0x80 | (c >> 12 & 0x3f));
		cb(0x80 | (c >> 6 & 0x3f));
		cb(0x80 | (c & 0x3f));
		return 5;
	}
}

inline uint8_t utf8EncodeBuf(char *ptr, char16_t ch) {
	return utf8EncodeCb([&] (char c) {
		*ptr++ = c;
	}, ch);
}

inline uint8_t utf8EncodeBuf(char *ptr, char32_t ch) {
	return utf8EncodeCb([&] (char c) {
		*ptr++ = c;
	}, ch);
}

inline uint8_t utf8Encode(std::string &str, char16_t ch) {
	return utf8EncodeCb([&] (char c) {
		str.push_back(c);
	}, ch);
}

inline uint8_t utf8Encode(std::string &str, char32_t ch) {
	return utf8EncodeCb([&] (char c) {
		str.push_back(c);
	}, ch);
}

inline uint8_t utf8Encode(memory::string &str, char16_t ch) {
	return utf8EncodeCb([&] (char c) {
		str.push_back(c);
	}, ch);
}

inline uint8_t utf8Encode(memory::string &str, char32_t ch) {
	return utf8EncodeCb([&] (char c) {
		str.push_back(c);
	}, ch);
}

inline uint8_t utf8Encode(std::ostream &str, char16_t ch) {
	return utf8EncodeCb([&] (char c) {
		str << c;
	}, ch);
}

inline uint8_t utf8Encode(std::ostream &str, char32_t ch) {
	return utf8EncodeCb([&] (char c) {
		str << c;
	}, ch);
}

constexpr inline char32_t utf16Decode32(const char16_t *ptr, uint8_t &offset) {
	if ((*ptr & char16_t(0xD800)) != 0) {
		offset = 2;
		return char32_t(0b0000'0011'1111'1111 & ptr[0]) << 10 | char32_t(0b0000'0011'1111'1111 & ptr[1]);
	} else {
		offset = 1;
		return char32_t(*ptr);
	}
}

constexpr inline char32_t utf16Decode32(const char16_t *ptr) {
	uint8_t offset;
	return utf16Decode32(ptr, offset);
}

constexpr inline uint8_t utf16EncodeLength(char32_t c) {
	if (c < 0xD800) {
		return 1;
	} else if (c <= 0xDFFF) {
		// do nothing, wrong encoding
		return 0;
	} else if (c < 0x10000) {
		return 1;
	} else {
		return 2;
	}
}

template <typename PutCharFn>
inline uint8_t utf16EncodeCb(const PutCharFn &cb, char32_t c) {
	if (c < 0xD800) {
		cb(char16_t(c));
		return 1;
	} else if (c <= 0xDFFF) {
		return 0;
	} else if (c < 0x10000) {
		cb(char16_t(c));
		return 1;
	} else {
		cb(char16_t(((0b1111'1111'1100'0000'0000 & c) >> 10) + 0xD800));
		cb(char16_t(((0b0000'0000'0011'1111'1111 & c) >> 00) + 0xDC00));
		return 2;
	}
}

inline uint8_t utf16EncodeBuf(char16_t *ptr, char32_t ch) {
	return utf16EncodeCb([&] (char16_t c) {
		*ptr++ = c;
	}, ch);
}

inline uint8_t utf16Encode(std::u16string &str, char32_t ch) {
	return utf16EncodeCb([&] (char16_t c) {
		str.push_back(c);
	}, ch);
}

inline uint8_t utf16Encode(memory::u16string &str, char32_t ch) {
	return utf16EncodeCb([&] (char16_t c) {
		str.push_back(c);
	}, ch);
}

template <typename std::enable_if<std::is_class<std::ctype<char16_t>>::value>::type* = nullptr>
inline uint8_t utf16Encode(std::basic_ostream<char16_t> &out, char32_t ch) {
	return utf16EncodeCb([&] (char16_t c) {
		out << c;
	}, ch);
}

}


// A part of SPString.h header placed here, to be available in utilities,
// that included by SPString.h (like StringView)

namespace STAPPLER_VERSIONIZED stappler::platform {

char32_t tolower(char32_t c);
char32_t toupper(char32_t c);
char32_t totitle(char32_t c);

}

namespace STAPPLER_VERSIONIZED stappler::string {

static constexpr size_t DOUBLE_MAX_DIGITS = 27;

inline char32_t tolower(char32_t c) { return platform::tolower(c); }
inline char32_t toupper(char32_t c) { return platform::toupper(c); }
inline char32_t totitle(char32_t c) { return platform::totitle(c); }


// fast itoa implementation
// data will be written at the end of buffer, no trailing zero (do not try to use strlen on it!)
// designed to be used with StringView: StringView(buf + bufSize - ret, ret)

size_t _itoa(int64_t number, char* buffer, size_t bufSize);
size_t _itoa(uint64_t number, char* buffer, size_t bufSize);

size_t _itoa(int64_t number, char16_t* buffer, size_t bufSize);
size_t _itoa(uint64_t number, char16_t* buffer, size_t bufSize);

size_t _itoa_len(int64_t number);
size_t _itoa_len(uint64_t number);

// fast dtoa implementation
// data will be written from beginning, no trailing zero (do not try to use strlen on it!)
// designed to be used with StringView: StringView(buf, ret)

size_t _dtoa(double number, char* buffer, size_t bufSize);
size_t _dtoa(double number, char16_t* buffer, size_t bufSize);

size_t _dtoa_len(double number);

// read number from string and offset pointers

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

}

#endif /* STAPPLER_CORE_STRING_SPUNICODE_H_ */
