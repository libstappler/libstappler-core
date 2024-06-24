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

#include "SPStringView.h"

#if WIN32
#include "SPMemory.h"
#include "SPPlatformUnistd.h"

namespace STAPPLER_VERSIONIZED stappler::platform {

class RandomSequence {
public:
	RandomSequence(void) : hProvider(NULL) {
		if (FALSE == CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, 0)) {
			// failed, should we try to create a default provider?
			if (NTE_BAD_KEYSET == GetLastError()) {
				if (FALSE == CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
					// ensure the provider is NULL so we could use a backup plan
					hProvider = NULL;
				}
			}
		}
	}

	~RandomSequence(void) {
		if (NULL != hProvider) {
			CryptReleaseContext(hProvider, 0U);
		}
	}

	BOOL generate(BYTE *buf, DWORD len) {
		if (NULL != hProvider) {
			return CryptGenRandom(hProvider, len, buf);
		}
		return FALSE;
	}

private:
	HCRYPTPROV hProvider;
};

static auto mapBuffer(WideStringView data, char16_t *buf, size_t count, int flags) {
	return LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, flags, (wchar_t *)data.data(), data.size(), (wchar_t *)buf, int(count),
				nullptr, nullptr, 0);
}

template <typename Interface>
auto mapString(WideStringView data, int flags) {
	auto bufSize = LCMapStringEx(LOCALE_NAME_SYSTEM_DEFAULT, flags, (wchar_t *)data.data(), data.size(), (wchar_t *)nullptr, 0,
		nullptr, nullptr, 0);

	typename Interface::WideStringType ret;
	ret.resize(bufSize);

	mapBuffer(data, ret.data(), ret.size(), flags);

	return ret;
}

template <typename Interface>
auto mapString(StringView data, int flags) {
	return string::toUtf8<Interface>(mapString<Interface>(string::toUtf16<Interface>(data), flags));
}

char32_t tolower(char32_t c) {
	char16_t bufA[2];
	char16_t bufB[8];

	auto size = unicode::utf16EncodeBuf(bufA, c);
	auto bufSize = mapBuffer(WideStringView(bufA, size), bufB, 8, LCMAP_LOWERCASE);
	if (bufSize > 0) {
		return unicode::utf16Decode32(bufB);
	}
	return c;
}

char32_t toupper(char32_t c) {
	char16_t bufA[2];
	char16_t bufB[8];

	auto size = unicode::utf16EncodeBuf(bufA, c);
	auto bufSize = mapBuffer(WideStringView(bufA, size), bufB, 8, LCMAP_UPPERCASE);
	if (bufSize > 0) {
		return unicode::utf16Decode32(bufB);
	}
	return c;
}

char32_t totitle(char32_t c) {
	char16_t bufA[2];
	char16_t bufB[8];

	auto size = unicode::utf16EncodeBuf(bufA, c);
	auto bufSize = mapBuffer(WideStringView(bufA, size), bufB, 8, LCMAP_TITLECASE);
	if (bufSize > 0) {
		return unicode::utf16Decode32(bufB);
	}
	return c;
}

template <>
auto tolower<memory::PoolInterface>(StringView data) -> memory::PoolInterface::StringType {
	return mapString<memory::PoolInterface>(data, LCMAP_LOWERCASE);
}

template <>
auto tolower<memory::StandartInterface>(StringView data) -> memory::StandartInterface::StringType {
	return mapString<memory::StandartInterface>(data, LCMAP_LOWERCASE);
}

template <>
auto toupper<memory::PoolInterface>(StringView data) -> memory::PoolInterface::StringType {
	return mapString<memory::PoolInterface>(data, LCMAP_UPPERCASE);
}

template <>
auto toupper<memory::StandartInterface>(StringView data) -> memory::StandartInterface::StringType {
	return mapString<memory::StandartInterface>(data, LCMAP_UPPERCASE);
}

template <>
auto totitle<memory::PoolInterface>(StringView data) -> memory::PoolInterface::StringType {
	return mapString<memory::PoolInterface>(data, LCMAP_TITLECASE);
}

template <>
auto totitle<memory::StandartInterface>(StringView data) -> memory::StandartInterface::StringType {
	return mapString<memory::StandartInterface>(data, LCMAP_TITLECASE);
}

template <>
auto tolower<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return mapString<memory::PoolInterface>(data, LCMAP_LOWERCASE);
}

template <>
auto tolower<memory::StandartInterface>(WideStringView data) -> memory::StandartInterface::WideStringType {
	return mapString<memory::StandartInterface>(data, LCMAP_LOWERCASE);
}

template <>
auto toupper<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return mapString<memory::PoolInterface>(data, LCMAP_UPPERCASE);
}

template <>
auto toupper<memory::StandartInterface>(WideStringView data) -> memory::StandartInterface::WideStringType {
	return mapString<memory::StandartInterface>(data, LCMAP_UPPERCASE);
}

template <>
auto totitle<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return mapString<memory::PoolInterface>(data, LCMAP_TITLECASE);
}

template <>
auto totitle<memory::StandartInterface>(WideStringView data) -> memory::StandartInterface::WideStringType {
	return mapString<memory::StandartInterface>(data, LCMAP_TITLECASE);
}

int compare_u(StringView l, StringView r) {
	return compare_u(string::toUtf16<memory::StandartInterface>(l), string::toUtf16<memory::StandartInterface>(r));
}

int compare_u(WideStringView l, WideStringView r) {
	return CompareStringEx(LOCALE_NAME_SYSTEM_DEFAULT, NORM_LINGUISTIC_CASING,
			(wchar_t *)l.data(), l.size(), (wchar_t *)r.data(), r.size(), NULL, NULL, 0);
}

int caseCompare_u(StringView l, StringView r) {
	return caseCompare_u(string::toUtf16<memory::StandartInterface>(l), string::toUtf16<memory::StandartInterface>(r));
}

int caseCompare_u(WideStringView l, WideStringView r) {
	return CompareStringEx(LOCALE_NAME_SYSTEM_DEFAULT, NORM_LINGUISTIC_CASING | NORM_IGNORECASE | LINGUISTIC_IGNORECASE,
			(wchar_t *)l.data(), l.size(), (wchar_t *)r.data(), r.size(), NULL, NULL, 0);
}

size_t makeRandomBytes(uint8_t *buf, size_t len) {
	RandomSequence seq;
	if (seq.generate(buf, len)) {
		return len;
	}
	return 0;
}

}

#endif
