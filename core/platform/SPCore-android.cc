/**
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

#include "SPStringView.h"

#if ANDROID

#include "SPDso.h"
#include "platform/SPJni.h"

#include <unicode/uchar.h>
#include <unicode/urename.h>
#include <unicode/ustring.h>

#include <sys/random.h>

namespace STAPPLER_VERSIONIZED stappler::platform {

static std::mutex s_collatorMutex;

struct IcuJave {
	jni::Env env;
	jni::GlobalClass charClass = nullptr;
	jni::GlobalClass collatorClass = nullptr;

	jmethodID toLowerChar = nullptr;
	jmethodID toUpperChar = nullptr;
	jmethodID toTitleChar = nullptr;

	jmethodID toLowerString = nullptr;
	jmethodID toUpperString = nullptr;
	jmethodID toTitleString = nullptr;

	jmethodID getInstance = nullptr;
	jmethodID setStrength = nullptr;
	jmethodID _compare = nullptr;

	int PRIMARY;
	int SECONDARY;
	int TERTIARY;
	int QUATERNARY;

	~IcuJave() {
		if (env) {
			collatorClass = nullptr;
			charClass = nullptr;
		}
	}

	void init(jni::Env &&e) {
		env = move(e);

		charClass = env.findClass("android/icu/lang/UCharacter").getGlobal();

		auto tmpClass = jni::RefClass(charClass, env);

		toLowerChar = tmpClass.getStaticMethodID("toLowerCase", "(I)I");
		toUpperChar = tmpClass.getStaticMethodID("toUpperCase", "(I)I");
		toTitleChar = tmpClass.getStaticMethodID("toTitleCase", "(I)I");

		toLowerString =
				tmpClass.getStaticMethodID("toLowerCase", "(Ljava/lang/String;)Ljava/lang/String;");
		toUpperString =
				tmpClass.getStaticMethodID("toUpperCase", "(Ljava/lang/String;)Ljava/lang/String;");
		toTitleString = tmpClass.getStaticMethodID("toTitleCase",
				"(Ljava/lang/String;Landroid/icu/text/BreakIterator;)Ljava/lang/String;");

		auto tmpCollatorClass = env.findClass("android/icu/text/Collator");
		collatorClass = tmpCollatorClass;
		getInstance =
				tmpCollatorClass.getStaticMethodID("getInstance", "()Landroid/icu/text/Collator;");
		setStrength = tmpCollatorClass.getMethodID("setStrength", "(I)V");
		_compare =
				tmpCollatorClass.getMethodID("compare", "(Ljava/lang/String;Ljava/lang/String;)I");

		PRIMARY = tmpCollatorClass.getStaticField<jint>("PRIMARY");
		SECONDARY = tmpCollatorClass.getStaticField<jint>("SECONDARY");
		TERTIARY = tmpCollatorClass.getStaticField<jint>("TERTIARY");
		QUATERNARY = tmpCollatorClass.getStaticField<jint>("QUATERNARY");
	}

	char32_t tolower(char32_t c) {
		if (!env) {
			return c;
		}

		return char32_t(jni::RefClass(charClass, env).callStaticMethod<jint>(toLowerChar, jint(c)));
	}

	char32_t toupper(char32_t c) {
		if (!env) {
			return c;
		}

		return char32_t(jni::RefClass(charClass, env).callStaticMethod<jint>(toUpperChar, jint(c)));
	}

	char32_t totitle(char32_t c) {
		if (!env) {
			return c;
		}

		return char32_t(jni::RefClass(charClass, env).callStaticMethod<jint>(toTitleChar, jint(c)));
	}

	template <typename Interface>
	auto tolower(WideStringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto ret = jni::RefClass(charClass, env)
						   .callStaticMethod<jstring>(toLowerString, env.newString(data));
		return ret.getWideString().str<Interface>();
	}

	template <typename Interface>
	auto tolower(StringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto ret = jni::RefClass(charClass, env)
						   .callStaticMethod<jstring>(toLowerString, env.newString(data));
		return ret.getString().str<Interface>();
	}

	template <typename Interface>
	auto toupper(WideStringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto ret = jni::RefClass(charClass, env)
						   .callStaticMethod<jstring>(toUpperString, env.newString(data));
		return ret.getWideString().str<Interface>();
	}

	template <typename Interface>
	auto toupper(StringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto ret = jni::RefClass(charClass, env)
						   .callStaticMethod<jstring>(toUpperString, env.newString(data));
		return ret.getString().str<Interface>();
	}

	template <typename Interface>
	auto totitle(WideStringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto ret = jni::RefClass(charClass, env)
						   .callStaticMethod<jstring>(toTitleString, env.newString(data));
		return ret.getWideString().str<Interface>();
	}

	template <typename Interface>
	auto totitle(StringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto ret = jni::RefClass(charClass, env)
						   .callStaticMethod<jstring>(toTitleString, env.newString(data));
		return ret.getString().str<Interface>();
	}

	int compare(StringView l, StringView r, bool caseInsensetive) {
		if (!env) {
			return string::detail::compare_c(l, r);
		}

		int ret = 0;
		auto strL = env.newString(l);
		auto strR = env.newString(r);

		auto coll = jni::RefClass(collatorClass, env).callStaticMethod<jobject>(getInstance);
		if (coll) {
			coll.callMethod<void>(setStrength, caseInsensetive ? SECONDARY : TERTIARY);
			ret = coll.callMethod<jint>(_compare, strL, strR);
		} else {
			ret = string::detail::compare_c(l, r);
		}
		return ret;
	}

	int compare(WideStringView l, WideStringView r, bool caseInsensetive) {
		if (!env) {
			return string::detail::compare_c(l, r);
		}

		int ret = 0;
		auto strL = env.newString(l);
		auto strR = env.newString(r);

		auto coll = jni::RefClass(collatorClass, env).callStaticMethod<jobject>(getInstance);
		if (coll) {
			coll.callMethod<void>(setStrength, caseInsensetive ? SECONDARY : TERTIARY);
			ret = coll.callMethod<jint>(_compare, strL, strR);
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

static thread_local IcuJave tl_interface;
static Dso s_icu;

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

static IcuJave *getInerface() {
	if (!tl_interface.env) {
		tl_interface.init(jni::Env::getEnv());
	}
	return &tl_interface;
};

void load(JavaVM *vm, int32_t sdk) {
	s_icu = Dso("libicu.so");
	if (s_icu) {
		tolower_fn = s_icu.sym<decltype(tolower_fn)>("u_tolower");
		toupper_fn = s_icu.sym<decltype(toupper_fn)>("u_toupper");
		totitle_fn = s_icu.sym<decltype(totitle_fn)>("u_totitle");
		strToLower_fn = s_icu.sym<decltype(strToLower_fn)>("u_strToLower");
		strToUpper_fn = s_icu.sym<decltype(strToUpper_fn)>("u_strToUpper");
		strToTitle_fn = s_icu.sym<decltype(strToTitle_fn)>("u_strToTitle");

		u_strCompare = s_icu.sym<decltype(u_strCompare)>("u_strCompare");
		u_strCaseCompare = s_icu.sym<decltype(u_strCaseCompare)>("u_strCaseCompare");
	}
}

void finalize() { s_icu.close(); }

static char32_t tolower(char32_t c) {
	if (s_icu) {
		return char32_t(tolower_fn(int32_t(c)));
	}
	return getInerface()->tolower(c);
}

static char32_t toupper(char32_t c) {
	if (s_icu) {
		return char32_t(toupper_fn(int32_t(c)));
	}
	return getInerface()->toupper(c);
}

static char32_t totitle(char32_t c) {
	if (s_icu) {
		return char32_t(totitle_fn(int32_t(c)));
	}
	return getInerface()->totitle(c);
}

template <typename Interface>
static auto tolower(WideStringView data) {
	if (s_icu) {
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
	return getInerface()->tolower<Interface>(data);
}

template <typename Interface>
static auto tolower(StringView data) {
	if (s_icu) {
		return string::toUtf8<Interface>(tolower<Interface>(string::toUtf16<Interface>(data)));
	}
	return getInerface()->tolower<Interface>(data);
}

template <typename Interface>
static auto toupper(WideStringView data) {
	if (s_icu) {
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
	return getInerface()->toupper<Interface>(data);
}

template <typename Interface>
static auto toupper(StringView data) {
	if (s_icu) {
		return string::toUtf8<Interface>(toupper<Interface>(string::toUtf16<Interface>(data)));
	}
	return getInerface()->toupper<Interface>(data);
}

template <typename Interface>
static auto totitle(WideStringView data) {
	if (s_icu) {
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
	return getInerface()->totitle<Interface>(data);
}

template <typename Interface>
static auto totitle(StringView data) {
	if (s_icu) {
		return string::toUtf8<Interface>(totitle<Interface>(string::toUtf16<Interface>(data)));
	}
	return getInerface()->totitle<Interface>(data);
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
	return i18n::getInerface()->compare(l, r, false);
}

int compare_u(WideStringView l, WideStringView r) {
	if (i18n::u_strCompare) {
		return i18n::u_strCompare(l.data(), l.size(), r.data(), r.size(), 1);
	}
	return i18n::getInerface()->compare(l, r, false);
}
int caseCompare_u(StringView l, StringView r) {
	if (i18n::u_strCaseCompare) {
		int status = 0;
		auto lStr = string::toUtf16<memory::StandartInterface>(l);
		auto rStr = string::toUtf16<memory::StandartInterface>(r);
		return i18n::u_strCaseCompare(lStr.data(), lStr.size(), rStr.data(), rStr.size(),
				U_COMPARE_CODE_POINT_ORDER, &status);
	}
	return i18n::getInerface()->compare(l, r, true);
}

int caseCompare_u(WideStringView l, WideStringView r) {
	if (i18n::u_strCaseCompare) {
		int status = 0;
		return i18n::u_strCaseCompare(l.data(), l.size(), r.data(), r.size(),
				U_COMPARE_CODE_POINT_ORDER, &status);
	}
	return i18n::getInerface()->compare(l, r, true);
}

size_t makeRandomBytes(uint8_t *buf, size_t count) {
	::arc4random_buf(buf, count);
	return count;
}

bool initialize(int &resultCode) { return true; }

void terminate() { }

} // namespace stappler::platform

#endif
