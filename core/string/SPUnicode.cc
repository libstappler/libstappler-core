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

#include "SPUnicode.h"
#include "SPString.h"

#if WIN32
#include "SPPlatformUnistd.h"
#include <libloaderapi.h>
#else
#include <dlfcn.h>
#endif

namespace STAPPLER_VERSIONIZED stappler::unicode {

static char32_t Utf8DecodeHtml32(const char *ptr, uint32_t len) {
	if (ptr[0] == '#') {
		if (len > 1 && (ptr[1] == 'x' || ptr[1] == 'X')) {
			return char32_t(strtol(ptr + 2, nullptr, 16));
		}
		return char32_t(strtol(ptr + 1, nullptr, 10));
	} else if (strncmp(ptr, "amp", len) == 0) {
		return '&';
	} else if (strncmp(ptr, "nbsp", len) == 0) {
		return 0xA0;
	} else if (strncmp(ptr, "quot", len) == 0) {
		return '"';
	} else if (strncmp(ptr, "apos", len) == 0) {
		return '\'';
	} else if (strncmp(ptr, "lt", len) == 0) {
		return '<';
	} else if (strncmp(ptr, "gt", len) == 0) {
		return '>';
	} else if (strncmp(ptr, "shy", len) == 0) {
		return char32_t(0x00AD);
	}
	return 0;
}

char32_t utf8HtmlDecode32(const char *utf8, uint8_t &offset) {
	if (utf8[0] == '&') {
		uint32_t len = 0;
		while (utf8[len] && utf8[len] != ';' && len < 10) {
			len ++;
		}

		char32_t c = 0;
		if (utf8[len] == ';' && len > 2) {
			c = Utf8DecodeHtml32(utf8 + 1, len - 2);
		}

		if (c == 0) {
			return utf8Decode32(utf8, offset);
		} else {
			offset = (len + 1);
			return c;
		}
	} else {
		return utf8Decode32(utf8, offset);
	}
}

}

namespace STAPPLER_VERSIONIZED stappler::string {

SPUNUSED inline size_t Utf8CharLength(const uint8_t *ptr, uint8_t &mask);

static inline void sp_str_replace(const char *target, const char *str, char &b, char &c) {
	int i = 0;
	while (str[1] != 0) {
		if (str[0] == b && str[1] == c) {
			if (i % 2 == 0) {
				b = target[i];
				c = target[i + 1];
			}
			return;
		}
		++ i; ++ str;
	}
}

template <class T> static inline T Utf8NextChar(T p) {
	return (p + unicode::utf8_length_data[((const uint8_t *)p)[0]]);
}

template <class T> static inline T Utf8NextChar(T p, size_t &counter) {
	auto l = unicode::utf8_length_data[ ((const uint8_t *)p)[0] ];
	counter += 1;
	return (p + l);
}

bool isValidUtf8(StringView r) {
	static const uint8_t utf8_valid_data[256] = {
	//	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, a, b, c, d, e, f, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, a, b, c, d, e, f
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0
	};

	auto ptr = r.data();
	const auto end = ptr + r.size();
	while (ptr < end && *ptr != 0) {
		auto l = utf8_valid_data[ ((const uint8_t *)ptr)[0] ];
		if (l == 0) {
			return false;
		} else if (l == 1) {
			++ ptr;
		} else {
			while (l > 1) {
				-- l;
				++ ptr;

				if ((((const uint8_t *)ptr)[0] & 0b1100'0000) != 0b1000'0000) {
					return false;
				}
			}
			++ ptr;
		}
	};
	return true;
}

size_t getUtf16Length(const StringView &input) {
	size_t counter = 0;
	auto ptr = input.data();
	const auto end = ptr + input.size();
	while (ptr < end && *ptr != 0) {
		counter += unicode::utf16_length_data[ uint8_t(*ptr) ];
		ptr += unicode::utf8_length_data[ uint8_t(*ptr) ];
	};
	return counter;
}

size_t getUtf16HtmlLength(const StringView &input) {
	size_t counter = 0;
	auto ptr = input.data();
	const auto end = ptr + input.size();
	while (ptr < end && *ptr != 0) {
		if (ptr[0] == '&') {
			uint8_t len = 0;
			while (ptr[len] && ptr[len] != ';' && len < 10) {
				len ++;
			}


			if (ptr[len] == ';' && len > 2) {
				counter ++;
				ptr += len;
			} else if (ptr[len] == 0) {
				ptr += len;
			} else {
				counter += unicode::utf16_length_data[ uint8_t(*ptr) ];
				ptr += unicode::utf8_length_data[ uint8_t(*ptr) ];
			}
		} else {
			counter += unicode::utf16_length_data[ uint8_t(*ptr) ];
			ptr += unicode::utf8_length_data[ uint8_t(*ptr) ];
		}
	};
	return counter;
}

size_t getUtf8HtmlLength(const StringView &input) {
	size_t counter = 0;
	auto ptr = input.data();
	const auto end = ptr + input.size();
	while (ptr < end && *ptr != 0) {
		if (ptr[0] == '&') {
			uint8_t len = 0;
			while (ptr[len] && ptr[len] != ';' && len < 10) {
				len ++;
			}

			if (ptr[len] == ';' && len > 2) {
				auto c = unicode::Utf8DecodeHtml32(ptr + 1, len - 2);
				counter += unicode::utf8EncodeLength(c);
				ptr += len;
			} else if (ptr[len] == 0) {
				ptr += len;
			} else {
				counter += 1;
				ptr += 1;
			}
		} else {
			counter += 1;
			ptr += 1;
		}
	};
	return counter;
}

size_t getUtf8Length(const WideStringView &str) {
	const char16_t *ptr = str.data();
	const char16_t *end = ptr + str.size();
	size_t ret = 0;
	while (ptr < end) {
		auto c = *ptr++;
		if (c >= 0xD800 && c <= 0xDFFF) {
			// surrogates is 4-byte
			ret += 4;
			++ ptr;
		} else {
			ret += unicode::utf8EncodeLength(c);
		}
	}
	return ret;
}

//static constexpr const char16_t utf8_small[64] = {
//	u'А', u'Б', u'В', u'Г', u'Д', u'Е', u'Ж', u'З', u'И', u'Й', u'К', u'Л', u'М', u'Н', u'О', u'П',
//	u'Р', u'С', u'Т', u'У', u'Ф', u'Х', u'Ц', u'Ч', u'Ш', u'Щ', u'Ъ', u'Ы', u'Ь', u'Э', u'Ю', u'Я',
//	u'а', u'б', u'в', u'г', u'д', u'е', u'ж', u'з', u'и', u'й', u'к', u'л', u'м', u'н', u'о', u'п',
//	u'р', u'с', u'т', u'у', u'ф', u'х', u'ц', u'ч', u'ш', u'щ', u'ъ', u'ы', u'ь', u'э', u'ю', u'я',
//};

static constexpr const uint8_t koi8r_small[64] = {
	0xE1, 0xE2, 0xF7, 0xE7, 0xE4, 0xE5, 0xF6, 0xFA, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0,
	0xF2, 0xF3, 0xF4, 0xF5, 0xE6, 0xE8, 0xE3, 0xFE, 0xFB, 0xFD, 0xFF, 0xF9, 0xF8, 0xFC, 0xE0, 0xF1,
	0xC1, 0xC2, 0xD7, 0xC7, 0xC4, 0xC5, 0xD6, 0xDA, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
	0xD2, 0xD3, 0xD4, 0xD5, 0xC6, 0xC8, 0xC3, 0xDE, 0xDB, 0xDD, 0xDF, 0xD9, 0xD8, 0xDC, 0xC0, 0xD1,
};

char charToKoi8r(char16_t c) {
	if (c <= 0x7f) {
		return char(c & 0xFF);
	} else if (c >= u'а' && c <= u'я') {
		return char(koi8r_small[c - u'а' + 32]);
	} else if (c >= u'А' && c <= u'Я') {
		return char(koi8r_small[(c - u'A') % 64]);
	} else {
		switch (c) {
		case 0x2500: return char(0x80); break;
		case 0x2502: return char(0x81); break;
		case 0x250C: return char(0x82); break;
		case 0x2510: return char(0x83); break;
		case 0x2514: return char(0x84); break;
		case 0x2518: return char(0x85); break;
		case 0x251C: return char(0x86); break;
		case 0x2524: return char(0x87); break;
		case 0x252C: return char(0x88); break;
		case 0x2534: return char(0x89); break;
		case 0x253C: return char(0x8A); break;
		case 0x2580: return char(0x8B); break;
		case 0x2584: return char(0x8C); break;
		case 0x2588: return char(0x8D); break;
		case 0x258C: return char(0x8E); break;
		case 0x2590: return char(0x8F); break;

		case 0x2591: return char(0x90); break;
		case 0x2592: return char(0x91); break;
		case 0x2593: return char(0x92); break;
		case 0x2320: return char(0x93); break;
		case 0x25A0: return char(0x94); break;
		case 0x2219: return char(0x95); break;
		case 0x221A: return char(0x96); break;
		case 0x2248: return char(0x97); break;
		case 0x2264: return char(0x98); break;
		case 0x2265: return char(0x99); break;
		case 0x00A0: return char(0x9A); break;
		case 0x2321: return char(0x9B); break;
		case 0x00B0: return char(0x9C); break;
		case 0x00B2: return char(0x9D); break;
		case 0x00B7: return char(0x9E); break;
		case 0x00F7: return char(0x9F); break;

		case 0x2550: return char(0xA0); break;
		case 0x2551: return char(0xA1); break;
		case 0x2552: return char(0xA2); break;
		case 0x0451: return char(0xA3); break;
		case 0x2553: return char(0xA4); break;
		case 0x2554: return char(0xA5); break;
		case 0x2555: return char(0xA6); break;
		case 0x2556: return char(0xA7); break;
		case 0x2557: return char(0xA8); break;
		case 0x2558: return char(0xA9); break;
		case 0x2559: return char(0xAA); break;
		case 0x255A: return char(0xAB); break;
		case 0x255B: return char(0xAC); break;
		case 0x255C: return char(0xAD); break;
		case 0x255D: return char(0xAE); break;
		case 0x255E: return char(0xAF); break;

		case 0x255F: return char(0xB0); break;
		case 0x2560: return char(0xB1); break;
		case 0x2561: return char(0xB2); break;
		case 0x0401: return char(0xB3); break;
		case 0x2562: return char(0xB4); break;
		case 0x2563: return char(0xB5); break;
		case 0x2564: return char(0xB6); break;
		case 0x2565: return char(0xB7); break;
		case 0x2566: return char(0xB8); break;
		case 0x2567: return char(0xB9); break;
		case 0x2568: return char(0xBA); break;
		case 0x2569: return char(0xBB); break;
		case 0x256A: return char(0xBC); break;
		case 0x256B: return char(0xBD); break;
		case 0x256C: return char(0xBE); break;
		case 0x00A9: return char(0xBF); break;
		default: break;
		}
	}
	return ' ';
}

}
