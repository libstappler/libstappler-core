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

#include "SPIdn.h"
#include "SPDso.h"
#include "SPString.h"

#ifdef WIN32
#include "SPPlatformUnistd.h"
#if XWIN
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#include "winnls.h"
#pragma clang diagnostic pop
#endif
#else
#if STAPPLER_SHARED
#include <idn2.h>
#else
#if LINUX
#include <idn2.h>
#endif
#include "unicode/uidna.h"
#endif
#endif

namespace STAPPLER_VERSIONIZED stappler::idn {

struct idn_iface {
#if STAPPLER_SHARED || LINUX
	Dso obj;
	decltype(&idn2_lookup_u8) lookup_u8 = nullptr;
	decltype(&idn2_to_unicode_8z8z) to_unicode_8z8z = nullptr;

	idn_iface() {
		auto h = Dso("libidn2.so");
		if (h) {
			obj = move(h);

			lookup_u8 = obj.sym<decltype(lookup_u8)>("idn2_lookup_u8");
			to_unicode_8z8z = obj.sym<decltype(to_unicode_8z8z)>("idn2_to_unicode_8z8z");
		}
	}

	template <typename Interface>
	auto _idnToAscii(StringView source) -> typename Interface::StringType {
		uint8_t *out = nullptr;
		if (lookup_u8) {
			auto err = lookup_u8((const uint8_t *)source.data(), &out, IDN2_NFC_INPUT|IDN2_NONTRANSITIONAL);
			if (err == IDN2_OK) {
				typename Interface::StringType ret((const char *)out);
				free(out);
				return ret;
			}
		}
#ifndef STAPPLER_SHARED
		char buf[1_KiB] = { 0 };
		UErrorCode code = U_ZERO_ERROR;
		auto retLen = u_nameToASCII_UTF8(UIDNA_NONTRANSITIONAL_TO_ASCII, source.data(), source.size(), buf, 1_KiB, nullptr, &code);
		if (retLen > 0 && code == U_ZERO_ERROR) {
			typename Interface::StringType ret(buf, retLen);
			return ret;
		}
#endif
		return typename Interface::StringType();
	}

	template <typename Interface>
	auto _idnToUnicode(StringView source) -> typename Interface::StringType {
		char *out = nullptr;
		auto err = to_unicode_8z8z(source.data(), &out, 0);
		if (err == IDN2_OK) {
			typename Interface::StringType ret(out);
			free(out);
			return ret;
		}
#ifndef STAPPLER_SHARED
		char buf[1_KiB] = { 0 };
		UErrorCode code = U_ZERO_ERROR;
		auto retLen = u_nameToUnicodeUTF8(0, source.data(), source.size(), buf, 1_KiB, nullptr, &code);
		if (retLen > 0 && code == U_ZERO_ERROR) {
			typename Interface::StringType ret(buf, retLen);
			return ret;
		}
#endif
		return typename Interface::StringType();
	}
#else
#ifndef WIN32
	idn_iface() {}

	template <typename Interface>
	auto _idnToAscii(StringView source) -> typename Interface::StringType {
		char buf[1_KiB] = { 0 };
		UErrorCode code = U_ZERO_ERROR;
		auto retLen = u_nameToASCII_UTF8(UIDNA_NONTRANSITIONAL_TO_ASCII, source.data(), source.size(), buf, 1_KiB, nullptr, &code);
		if (retLen > 0 && code == U_ZERO_ERROR) {
			typename Interface::StringType ret(buf, retLen);
			return ret;
		}
		return typename Interface::StringType();
	}

	template <typename Interface>
	auto _idnToUnicode(StringView source) -> typename Interface::StringType {
		char buf[1_KiB] = { 0 };
		UErrorCode code = U_ZERO_ERROR;
		auto retLen = u_nameToUnicodeUTF8(0, source.data(), source.size(), buf, 1_KiB, nullptr, &code);
		if (retLen > 0 && code == U_ZERO_ERROR) {
			typename Interface::StringType ret(buf, retLen);
			return ret;
		}
		return typename Interface::StringType();
	}
#endif
#endif
};

#ifdef WIN32
template <typename Interface>
auto _idnToAscii_winidn(StringView source) -> typename Interface::StringType {
	auto in_w = string::toUtf16<memory::StandartInterface>(source);
	wchar_t punycode[256] = { 0 };
	int chars = IdnToAscii(0, (const wchar_t *)in_w.data(), -1, punycode, 255);
	if (chars) {
		return string::toUtf8<Interface>((char16_t *)punycode);
	}
	return typename Interface::StringType();
}

template <typename Interface>
auto _idnToUnicode_winidn(StringView source) -> typename Interface::StringType {
	auto in_w = string::toUtf16<memory::StandartInterface>(source);
	wchar_t unicode[256] = { 0 };
	int chars = IdnToUnicode(0, (const wchar_t *)in_w.data(), int(in_w.size()), unicode, 255);
	if (chars) {
		return string::toUtf8<Interface>((char16_t *)unicode);
	}
	return typename Interface::StringType();
}
#else
static idn_iface s_idnIface;
#endif

using HostUnicodeChars = chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>,
		chars::Chars<char, '.', '-'>, chars::Range<char, char(128), char(255)>>;

using HostAsciiChars = chars::Compose<char, chars::CharGroup<char, CharGroupId::Alphanumeric>,
		chars::Chars<char, '.', '-'>>;

template <typename Interface>
auto _idnToAscii(StringView source, bool validate) -> typename Interface::StringType {
	if (source.empty()) {
		return typename Interface::StringType();
	}

	if (validate) {
		StringView r(source);
		r.skipChars<HostUnicodeChars>();
		if (!r.empty()) {
			return typename Interface::StringType();
		}
	}

#ifdef WIN32
	return _idnToAscii_winidn<Interface>(source);
#else
	return s_idnIface._idnToAscii<Interface>(source);
#endif
	return typename Interface::StringType();
}

template <typename Interface>
auto _idnToUnicode(StringView source, bool validate) -> typename Interface::StringType {
	if (source.empty()) {
		return typename Interface::StringType();
	}

	if (validate) {
		StringView r(source);
		r.skipChars<HostAsciiChars>();
		if (!r.empty()) {
			return typename Interface::StringType();
		}
	}

#ifdef WIN32
	return _idnToUnicode_winidn<Interface>(source);
#else
	return s_idnIface._idnToUnicode<Interface>(source);
#endif
	return typename Interface::StringType();
}


template <>
auto toAscii<memory::PoolInterface>(StringView source, bool validate) -> memory::PoolInterface::StringType {
	return _idnToAscii<memory::PoolInterface>(source, validate);
}

template <>
auto toAscii<memory::StandartInterface>(StringView source, bool validate) -> memory::StandartInterface::StringType {
	return _idnToAscii<memory::StandartInterface>(source, validate);
}

template <>
auto toUnicode<memory::PoolInterface>(StringView source, bool validate) -> memory::PoolInterface::StringType {
	return _idnToUnicode<memory::PoolInterface>(source, validate);
}

template <>
auto toUnicode<memory::StandartInterface>(StringView source, bool validate) -> memory::StandartInterface::StringType {
	return _idnToUnicode<memory::StandartInterface>(source, validate);
}

}
