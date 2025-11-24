/**
 Copyright (c) 2023-2025 Stappler LLC <admin@stappler.dev>
 Copyright (c) 2025 Stappler Team <admin@stappler.org>

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

#include "SPString.h"

#if ANDROID

#include "SPDso.h"
#include "SPIdn.h"
#include "platform/SPJni.h"

#include <unicode/uchar.h>
#include <unicode/urename.h>
#include <unicode/ustring.h>

#include <sys/random.h>
#include <android/configuration.h>

namespace STAPPLER_VERSIONIZED stappler::platform {

static std::mutex s_collatorMutex;

struct IcuJava : Ref {
	struct UCharacterProxy : jni::ClassProxy {
		jni::StaticMethod<"toLowerCase", jint(jint)> toLowerChar = this;
		jni::StaticMethod<"toUpperCase", jint(jint)> toUpperChar = this;
		jni::StaticMethod<"toTitleCase", jint(jint)> toTitleChar = this;

		jni::StaticMethod<"toLowerCase", jstring(jstring)> toLowerString = this;
		jni::StaticMethod<"toUpperCase", jstring(jstring)> toUpperString = this;
		jni::StaticMethod<"toTitleCase", jstring(jstring)> toTitleString = this;

		using jni::ClassProxy::ClassProxy;
	} UCharacter = "android/icu/lang/UCharacter";

	struct CollatorProxy : jni::ClassProxy {
		jni::StaticField<"PRIMARY", jint> PRIMARY = this;
		jni::StaticField<"SECONDARY", jint> SECONDARY = this;
		jni::StaticField<"TERTIARY", jint> TERTIARY = this;
		jni::StaticField<"QUATERNARY", jint> QUATERNARY = this;

		jni::StaticMethod<"getInstance", jni::L<"android/icu/text/Collator;">()> getInstance = this;
		jni::Method<"setStrength", void(jint)> setStrength = this;
		jni::Method<"compare", jint(jstring, jstring)> _compare = this;

		using jni::ClassProxy::ClassProxy;
	} Collator = "android/icu/text/Collator";

	virtual ~IcuJava() = default;

	char32_t tolower(char32_t c) {
		return UCharacter.toLowerChar(UCharacter.getClass().ref(), jint(c));
	}

	char32_t toupper(char32_t c) {
		return UCharacter.toUpperChar(UCharacter.getClass().ref(), jint(c));
	}

	char32_t totitle(char32_t c) {
		return UCharacter.toTitleChar(UCharacter.getClass().ref(), jint(c));
	}

	template <typename Interface>
	auto tolower(WideStringView data) {
		auto env = jni::Env::getEnv();
		return UCharacter.toLowerString(UCharacter.getClass().ref(env), env.newString(data))
				.getWideString()
				.str<Interface>();
	}

	template <typename Interface>
	auto tolower(StringView data) {
		auto env = jni::Env::getEnv();
		return UCharacter.toLowerString(UCharacter.getClass().ref(env), env.newString(data))
				.getString()
				.str<Interface>();
	}

	template <typename Interface>
	auto toupper(WideStringView data) {
		auto env = jni::Env::getEnv();
		return UCharacter.toUpperString(UCharacter.getClass().ref(env), env.newString(data))
				.getWideString()
				.str<Interface>();
	}

	template <typename Interface>
	auto toupper(StringView data) {
		auto env = jni::Env::getEnv();
		return UCharacter.toUpperString(UCharacter.getClass().ref(env), env.newString(data))
				.getString()
				.str<Interface>();
	}

	template <typename Interface>
	auto totitle(WideStringView data) {
		auto env = jni::Env::getEnv();
		return UCharacter.toTitleString(UCharacter.getClass().ref(env), env.newString(data))
				.getWideString()
				.str<Interface>();
	}

	template <typename Interface>
	auto totitle(StringView data) {
		auto env = jni::Env::getEnv();
		return UCharacter.toTitleString(UCharacter.getClass().ref(env), env.newString(data))
				.getString()
				.str<Interface>();
	}

	int compare(StringView l, StringView r, bool caseInsensetive) {
		auto env = jni::Env::getEnv();

		int ret = 0;
		auto strL = env.newString(l);
		auto strR = env.newString(r);

		auto coll = Collator.getInstance(Collator.getClass().ref(env));
		if (coll) {
			Collator.setStrength(coll,
					jint(caseInsensetive ? Collator.SECONDARY() : Collator.TERTIARY()));
			ret = Collator._compare(coll, strL, strR);
		} else {
			ret = string::detail::compare_c(l, r);
		}
		return ret;
	}

	int compare(WideStringView l, WideStringView r, bool caseInsensetive) {
		auto env = jni::Env::getEnv();

		int ret = 0;
		auto strL = env.newString(l);
		auto strR = env.newString(r);

		auto coll = Collator.getInstance(Collator.getClass().ref(env));
		if (coll) {
			Collator.setStrength(coll,
					jint(caseInsensetive ? Collator.SECONDARY() : Collator.TERTIARY()));
			ret = Collator._compare(coll, strL, strR);
		} else {
			ret = string::detail::compare_c(l, r);
		}
		return ret;
	}
};

namespace i18n {

using cmp_fn = int32_t (*)(const char16_t *s1, int32_t length1, const char16_t *s2, int32_t length2,
		int8_t codePointOrder);
using case_cmp_fn = int32_t (*)(const char16_t *s1, int32_t length1, const char16_t *s2,
		int32_t length2, uint32_t options, int *pErrorCode);

static Dso s_icuNative;
static Rc<IcuJava> s_icuJava;

static int32_t (*tolower_fn)(int32_t) = nullptr;
static int32_t (*toupper_fn)(int32_t) = nullptr;
static int32_t (*totitle_fn)(int32_t) = nullptr;

static int32_t (*strToLower_fn)(char16_t *dest, int32_t destCapacity, const char16_t *src,
		int32_t srcLength, const char *locale, int *pErrorCode) = nullptr;

static int32_t (*strToUpper_fn)(char16_t *dest, int32_t destCapacity, const char16_t *src,
		int32_t srcLength, const char *locale, int *pErrorCode) = nullptr;

static int32_t (*strToTitle_fn)(char16_t *dest, int32_t destCapacity, const char16_t *src,
		int32_t srcLength, void *iter, const char *locale, int *pErrorCode) = nullptr;

static cmp_fn u_strCompare = nullptr;
static case_cmp_fn u_strCaseCompare = nullptr;

static char32_t tolower(char32_t c) {
	if (s_icuNative) {
		return char32_t(tolower_fn(int32_t(c)));
	}
	return s_icuJava->tolower(c);
}

static char32_t toupper(char32_t c) {
	if (s_icuNative) {
		return char32_t(toupper_fn(int32_t(c)));
	}
	return s_icuJava->toupper(c);
}

static char32_t totitle(char32_t c) {
	if (s_icuNative) {
		return char32_t(totitle_fn(int32_t(c)));
	}
	return s_icuJava->totitle(c);
}

template <typename Interface>
static auto tolower(WideStringView data) {
	if (s_icuNative) {
		typename Interface::WideStringType ret;
		ret.resize(data.size());

		int status = 0;
		auto len = strToLower_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		if (len <= int32_t(ret.size())) {
			ret.resize(len);
		} else {
			ret.resize(len);
			strToLower_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		}
		return ret;
	}
	return s_icuJava->tolower<Interface>(data);
}

template <typename Interface>
static auto tolower(StringView data) {
	if (s_icuNative) {
		return string::toUtf8<Interface>(tolower<Interface>(string::toUtf16<Interface>(data)));
	}
	return s_icuJava->tolower<Interface>(data);
}

template <typename Interface>
static auto toupper(WideStringView data) {
	if (s_icuNative) {
		typename Interface::WideStringType ret;
		ret.resize(data.size());

		int status = 0;
		auto len = strToUpper_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		if (len <= int32_t(ret.size())) {
			ret.resize(len);
		} else {
			ret.resize(len);
			strToUpper_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		}
		return ret;
	}
	return s_icuJava->toupper<Interface>(data);
}

template <typename Interface>
static auto toupper(StringView data) {
	if (s_icuNative) {
		return string::toUtf8<Interface>(toupper<Interface>(string::toUtf16<Interface>(data)));
	}
	return s_icuJava->toupper<Interface>(data);
}

template <typename Interface>
static auto totitle(WideStringView data) {
	if (s_icuNative) {
		typename Interface::WideStringType ret;
		ret.resize(data.size());

		int status = 0;
		auto len = strToTitle_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, NULL,
				&status);
		if (len <= int32_t(ret.size())) {
			ret.resize(len);
		} else {
			ret.resize(len);
			strToTitle_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, NULL, &status);
		}
		return ret;
	}
	return s_icuJava->totitle<Interface>(data);
}

template <typename Interface>
static auto totitle(StringView data) {
	if (s_icuNative) {
		return string::toUtf8<Interface>(totitle<Interface>(string::toUtf16<Interface>(data)));
	}
	return s_icuJava->totitle<Interface>(data);
}
} // namespace i18n

char32_t tolower(char32_t c) { return i18n::tolower(c); }

char32_t toupper(char32_t c) { return i18n::toupper(c); }

char32_t totitle(char32_t c) { return i18n::totitle(c); }

template <>
auto tolower<memory::PoolInterface>(StringView data) -> memory::PoolInterface::StringType {
	return i18n::tolower<memory::PoolInterface>(data);
}

template <>
auto tolower<memory::StandartInterface>(StringView data) -> memory::StandartInterface::StringType {
	return i18n::tolower<memory::StandartInterface>(data);
}

template <>
auto toupper<memory::PoolInterface>(StringView data) -> memory::PoolInterface::StringType {
	return i18n::toupper<memory::PoolInterface>(data);
}

template <>
auto toupper<memory::StandartInterface>(StringView data) -> memory::StandartInterface::StringType {
	return i18n::toupper<memory::StandartInterface>(data);
}

template <>
auto totitle<memory::PoolInterface>(StringView data) -> memory::PoolInterface::StringType {
	return i18n::totitle<memory::PoolInterface>(data);
}

template <>
auto totitle<memory::StandartInterface>(StringView data) -> memory::StandartInterface::StringType {
	return i18n::totitle<memory::StandartInterface>(data);
}

template <>
auto tolower<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return i18n::tolower<memory::PoolInterface>(data);
}

template <>
auto tolower<memory::StandartInterface>(WideStringView data)
		-> memory::StandartInterface::WideStringType {
	return i18n::tolower<memory::StandartInterface>(data);
}

template <>
auto toupper<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return i18n::toupper<memory::PoolInterface>(data);
}

template <>
auto toupper<memory::StandartInterface>(WideStringView data)
		-> memory::StandartInterface::WideStringType {
	return i18n::toupper<memory::StandartInterface>(data);
}

template <>
auto totitle<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return i18n::totitle<memory::PoolInterface>(data);
}

template <>
auto totitle<memory::StandartInterface>(WideStringView data)
		-> memory::StandartInterface::WideStringType {
	return i18n::totitle<memory::StandartInterface>(data);
}

int compare_u(StringView l, StringView r) {
	if (i18n::u_strCompare) {
		auto lStr = string::toUtf16<memory::StandartInterface>(l);
		auto rStr = string::toUtf16<memory::StandartInterface>(r);
		return i18n::u_strCompare(lStr.data(), lStr.size(), rStr.data(), rStr.size(), 1);
	}
	return i18n::s_icuJava->compare(l, r, false);
}

int compare_u(WideStringView l, WideStringView r) {
	if (i18n::u_strCompare) {
		return i18n::u_strCompare(l.data(), l.size(), r.data(), r.size(), 1);
	}
	return i18n::s_icuJava->compare(l, r, false);
}
int caseCompare_u(StringView l, StringView r) {
	if (i18n::u_strCaseCompare) {
		int status = 0;
		auto lStr = string::toUtf16<memory::StandartInterface>(l);
		auto rStr = string::toUtf16<memory::StandartInterface>(r);
		return i18n::u_strCaseCompare(lStr.data(), lStr.size(), rStr.data(), rStr.size(),
				U_COMPARE_CODE_POINT_ORDER, &status);
	}
	return i18n::s_icuJava->compare(l, r, true);
}

int caseCompare_u(WideStringView l, WideStringView r) {
	if (i18n::u_strCaseCompare) {
		int status = 0;
		return i18n::u_strCaseCompare(l.data(), l.size(), r.data(), r.size(),
				U_COMPARE_CODE_POINT_ORDER, &status);
	}
	return i18n::s_icuJava->compare(l, r, true);
}

size_t makeRandomBytes(uint8_t *buf, size_t count) {
	::arc4random_buf(buf, count);
	return count;
}

static char s_locale[6] = "en-us";

bool initialize(int &resultCode) {
	// init locale
	auto cfg = jni::Env::getApp()->config;
	if (cfg) {
		AConfiguration_getLanguage(cfg, s_locale);
		AConfiguration_getCountry(cfg, s_locale + 3);
	}

	i18n::s_icuNative = Dso("libicu.so");
	if (i18n::s_icuNative) {
		i18n::tolower_fn = i18n::s_icuNative.sym<decltype(i18n::tolower_fn)>("u_tolower");
		i18n::toupper_fn = i18n::s_icuNative.sym<decltype(i18n::toupper_fn)>("u_toupper");
		i18n::totitle_fn = i18n::s_icuNative.sym<decltype(i18n::totitle_fn)>("u_totitle");
		i18n::strToLower_fn = i18n::s_icuNative.sym<decltype(i18n::strToLower_fn)>("u_strToLower");
		i18n::strToUpper_fn = i18n::s_icuNative.sym<decltype(i18n::strToUpper_fn)>("u_strToUpper");
		i18n::strToTitle_fn = i18n::s_icuNative.sym<decltype(i18n::strToTitle_fn)>("u_strToTitle");

		i18n::u_strCompare = i18n::s_icuNative.sym<decltype(i18n::u_strCompare)>("u_strCompare");
		i18n::u_strCaseCompare =
				i18n::s_icuNative.sym<decltype(i18n::u_strCaseCompare)>("u_strCaseCompare");
	} else {
		// init with Java
		i18n::s_icuJava = Rc<IcuJava>::create();
	}

	return true;
}

void terminate() {
	i18n::s_icuNative.close();
	i18n::s_icuJava = nullptr;
}

StringView getOsLocale() { return StringView(s_locale); }

} // namespace stappler::platform

// Mimic IDN2 API for CURL
namespace STAPPLER_VERSIONIZED stappler::idn {

static constexpr auto IDN2_VERSION = "2.3.2-lisbstappler";

static constexpr auto IDN2_OK = 0;

static constexpr auto IDN2_JNI = -1'000;
static constexpr auto IDN2_JNI_STR = "Fail to acquire JNI context";

static constexpr auto IDN2_CONV_TOASCII = -1'001;
static constexpr auto IDN2_CONV_TOASCII_STR = "Fail to call java.net.IDN.toASCII";

static constexpr auto IDN2_CONV_TOUNICODE = -1'002;
static constexpr auto IDN2_CONV_TOUNICODE_STR = "Fail to call java.net.IDN.toUnicode";

static constexpr auto IDN2_ALLOW_UNASSIGNED = 16;
static constexpr auto IDN2_USE_STD3_ASCII_RULES = 32;

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

	auto app = jni::Env::getApp();
	auto env = jni::Env::getEnv();

	if (app && env) {
		auto str = app->IDN.toASCII(app->IDN.getClass().ref(env), env.newString(source), 0);
		if (str) {
			return str.getString().str<Interface>();
		}
	}

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

	auto app = jni::Env::getApp();
	auto env = jni::Env::getEnv();

	if (app && env) {
		auto str = app->IDN.toUnicode(app->IDN.getClass().ref(env), env.newString(source), 0);
		if (str) {
			return str.getString().str<Interface>();
		}
	}

	return typename Interface::StringType();
}

template <>
auto toAscii<memory::PoolInterface>(StringView source, bool validate)
		-> memory::PoolInterface::StringType {
	return _idnToAscii<memory::PoolInterface>(source, validate);
}

template <>
auto toAscii<memory::StandartInterface>(StringView source, bool validate)
		-> memory::StandartInterface::StringType {
	return _idnToAscii<memory::StandartInterface>(source, validate);
}

template <>
auto toUnicode<memory::PoolInterface>(StringView source, bool validate)
		-> memory::PoolInterface::StringType {
	return _idnToUnicode<memory::PoolInterface>(source, validate);
}

template <>
auto toUnicode<memory::StandartInterface>(StringView source, bool validate)
		-> memory::StandartInterface::StringType {
	return _idnToUnicode<memory::StandartInterface>(source, validate);
}
extern "C" SP_PUBLIC int idn2_lookup_u8(const uint8_t *src, uint8_t **lookupname, int flags) {
	if (!src) {
		if (lookupname) {
			*lookupname = nullptr;
		}
		return IDN2_OK;
	}

	auto app = jni::Env::getApp();
	auto env = jni::Env::getEnv();
	if (!app || !env) {
		return IDN2_JNI;
	}

	jint options = 0;

	if (flags & IDN2_USE_STD3_ASCII_RULES) {
		options |= app->IDN.USE_STD3_ASCII_RULES();
	}

	if (flags & IDN2_ALLOW_UNASSIGNED) {
		options |= app->IDN.ALLOW_UNASSIGNED();
	}

	auto str = app->IDN.toASCII(app->IDN.getClass().ref(env),
			env.newString(StringView((const char *)src)), jint(options));
	if (str) {
		auto out = str.getString();

		auto buf = new char[out.size() + 1];
		::memcpy(buf, out.data(), out.size());
		buf[out.size()] = 0;

		*lookupname = (uint8_t *)buf;
		return IDN2_JNI;
	} else {
		return IDN2_CONV_TOASCII;
	}
}

extern "C" SP_PUBLIC int idn2_lookup_ul(const char *src, char **lookupname, int flags) {
	if (!src) {
		if (lookupname) {
			*lookupname = nullptr;
		}
		return IDN2_OK;
	}

	auto app = jni::Env::getApp();
	auto env = jni::Env::getEnv();
	if (!app || !env) {
		return IDN2_JNI;
	}

	jint options = 0;

	if (flags & IDN2_USE_STD3_ASCII_RULES) {
		options |= app->IDN.USE_STD3_ASCII_RULES();
	}

	if (flags & IDN2_ALLOW_UNASSIGNED) {
		options |= app->IDN.ALLOW_UNASSIGNED();
	}

	auto str = app->IDN.toASCII(app->IDN.getClass().ref(env), env.newString(StringView(src)),
			jint(options));
	if (str) {
		auto out = str.getString();

		auto buf = new char[out.size() + 1];
		::memcpy(buf, out.data(), out.size());
		buf[out.size()] = 0;

		*lookupname = buf;
		return IDN2_JNI;
	} else {
		return IDN2_CONV_TOASCII;
	}
}

extern "C" SP_PUBLIC int idn2_to_unicode_8z8z(const char *src, char **lookupname, int flags) {
	if (!src) {
		if (lookupname) {
			*lookupname = nullptr;
		}
		return IDN2_OK;
	}

	auto app = jni::Env::getApp();
	auto env = jni::Env::getEnv();
	if (!app || !env) {
		return IDN2_JNI;
	}

	jint options = 0;

	if (flags & IDN2_USE_STD3_ASCII_RULES) {
		options |= app->IDN.USE_STD3_ASCII_RULES();
	}

	if (flags & IDN2_ALLOW_UNASSIGNED) {
		options |= app->IDN.ALLOW_UNASSIGNED();
	}

	auto str = app->IDN.toUnicode(app->IDN.getClass().ref(env), env.newString(StringView(src)),
			jint(options));
	if (str) {
		auto out = str.getString();

		auto buf = new char[out.size() + 1];
		::memcpy(buf, out.data(), out.size());
		buf[out.size()] = 0;

		*lookupname = buf;
		return IDN2_JNI;
	} else {
		return IDN2_CONV_TOUNICODE;
	}
}

extern "C" SP_PUBLIC const char *idn2_strerror(int rc) {
	switch (rc) {
	case IDN2_OK: return "Success"; break;
	case IDN2_JNI: return IDN2_JNI_STR; break;
	case IDN2_CONV_TOASCII: return IDN2_CONV_TOASCII_STR; break;
	case IDN2_CONV_TOUNICODE: return IDN2_CONV_TOUNICODE_STR; break;
	}
	return nullptr;
}

extern "C" SP_PUBLIC const char *idn2_strerror_name(int rc) {
	switch (rc) {
	case IDN2_OK: return "IDN2_OK"; break;
	case IDN2_JNI: return "IDN2_JNI"; break;
	case IDN2_CONV_TOASCII: return "IDN2_CONV_TOASCII"; break;
	case IDN2_CONV_TOUNICODE: return "IDN2_CONV_TOUNICODE"; break;
	}
	return nullptr;
}

extern "C" SP_PUBLIC void idn2_free(void *ptr) {
	if (ptr) {
		delete[] (char *)ptr;
	}
}

extern "C" SP_PUBLIC const char *idn2_check_version(const char *req_version) {
	if (!req_version || strcmp(req_version, IDN2_VERSION) <= 0) {
		return IDN2_VERSION;
	}

	return NULL;
}

} // namespace stappler::idn

#endif
