/**
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

#include "SPStringView.h"

#if ANDROID

#include "SPDso.h"

#include <unicode/uchar.h>
#include <unicode/urename.h>
#include <unicode/ustring.h>

#include <jni.h>

namespace STAPPLER_VERSIONIZED stappler::platform {

struct IcuJave {
	bool attached = false;
	JavaVM *vm = nullptr;
	JNIEnv *env = nullptr;
	jclass cl = nullptr;
	jmethodID toLowerChar = nullptr;
	jmethodID toUpperChar = nullptr;
	jmethodID toTitleChar = nullptr;

	jmethodID toLowerString = nullptr;
	jmethodID toUpperString = nullptr;
	jmethodID toTitleString = nullptr;

	~IcuJave() {
		if (env && cl) {
			env->DeleteGlobalRef(cl);
		}
		if (attached) {
			vm->DetachCurrentThread();
		}
	}

	void init(JavaVM *v, JNIEnv *e, bool a) {
		vm = v;
		env = e;
		attached = a;

		auto tmp = env->FindClass("android/icu/lang/UCharacter");
		cl = static_cast<jclass>(env->NewGlobalRef(static_cast<jobject>(tmp)));
		toLowerChar = env->GetStaticMethodID(cl, "toLowerCase", "(I)I");
		toUpperChar = env->GetStaticMethodID(cl, "toUpperCase", "(I)I");
		toTitleChar = env->GetStaticMethodID(cl, "toTitleCase", "(I)I");

		toLowerString = env->GetStaticMethodID(cl, "toLowerCase", "(Ljava/lang/String;)Ljava/lang/String;");
		toUpperString = env->GetStaticMethodID(cl, "toUpperCase", "(Ljava/lang/String;)Ljava/lang/String;");
		toTitleString = env->GetStaticMethodID(cl, "toTitleCase", "(Ljava/lang/String;Landroid/icu/text/BreakIterator;)Ljava/lang/String;");
	}

	char32_t tolower(char32_t c) {
		if (!env) {
			return c;
		}

		return char32_t(env->CallStaticIntMethod(cl, toLowerChar, jint(c)));
	}

	char32_t toupper(char32_t c) {
		if (!env) {
			return c;
		}

		return char32_t(env->CallStaticIntMethod(cl, toUpperChar, jint(c)));
	}

	char32_t totitle(char32_t c) {
		if (!env) {
			return c;
		}

		return char32_t(env->CallStaticIntMethod(cl, toTitleChar, jint(c)));
	}

	template <typename Interface>
	auto tolower(WideStringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto str = env->NewString((jchar *)data.data(), data.size());
		auto ret = static_cast<jstring>(env->CallStaticObjectMethod(cl, toLowerString, str));
		auto chars = env->GetStringChars(ret, nullptr);

		auto result = typename Interface::WideStringType((char16_t *)chars, env->GetStringLength(ret));

		env->ReleaseStringChars(ret, chars);
		env->DeleteLocalRef(ret);
		env->DeleteLocalRef(str);
		return result;
	}

	template <typename Interface>
	auto tolower(StringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto str = data.terminated() ? env->NewStringUTF(data.data())
				: env->NewStringUTF(data.str<memory::StandartInterface>().data());
		auto ret = static_cast<jstring>(env->CallStaticObjectMethod(cl, toLowerString, str));
		auto chars = (char *)env->GetStringUTFChars(ret, nullptr);

		auto result = typename Interface::StringType(chars, env->GetStringUTFLength(ret));

		env->ReleaseStringUTFChars(ret, chars);
		env->DeleteLocalRef(ret);
		env->DeleteLocalRef(str);
		return result;
	}

	template <typename Interface>
	auto toupper(WideStringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto str = env->NewString((jchar *)data.data(), data.size());
		auto ret = static_cast<jstring>(env->CallStaticObjectMethod(cl, toUpperString, str));
		auto chars = env->GetStringChars(ret, nullptr);

		auto result = typename Interface::WideStringType((char16_t *)chars, env->GetStringLength(ret));

		env->ReleaseStringChars(ret, chars);
		env->DeleteLocalRef(ret);
		env->DeleteLocalRef(str);
		return result;
	}

	template <typename Interface>
	auto toupper(StringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto str = data.terminated() ? env->NewStringUTF(data.data())
									 : env->NewStringUTF(data.str<memory::StandartInterface>().data());
		auto ret = static_cast<jstring>(env->CallStaticObjectMethod(cl, toUpperString, str));
		auto chars = (char *)env->GetStringUTFChars(ret, nullptr);

		auto result = typename Interface::StringType(chars, env->GetStringUTFLength(ret));

		env->ReleaseStringUTFChars(ret, chars);
		env->DeleteLocalRef(ret);
		env->DeleteLocalRef(str);
		return result;
	}

	template <typename Interface>
	auto totitle(WideStringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto str = env->NewString((jchar *)data.data(), data.size());
		auto ret = static_cast<jstring>(env->CallStaticObjectMethod(cl, toTitleString, str, nullptr));
		auto chars = env->GetStringChars(ret, nullptr);

		auto result = typename Interface::WideStringType((char16_t *)chars, env->GetStringLength(ret));

		env->ReleaseStringChars(ret, chars);
		env->DeleteLocalRef(ret);
		env->DeleteLocalRef(str);
		return result;
	}

	template <typename Interface>
	auto totitle(StringView data) {
		if (!env) {
			return data.str<Interface>();
		}

		auto str = data.terminated() ? env->NewStringUTF(data.data())
									 : env->NewStringUTF(data.str<memory::StandartInterface>().data());
		auto ret = static_cast<jstring>(env->CallStaticObjectMethod(cl, toTitleString, str, nullptr));
		auto chars = (char *)env->GetStringUTFChars(ret, nullptr);

		auto result = typename Interface::StringType(chars, env->GetStringUTFLength(ret));

		env->ReleaseStringUTFChars(ret, chars);
		env->DeleteLocalRef(ret);
		env->DeleteLocalRef(str);
		return result;
	}
};

namespace i18n {
#if (0)
	static char32_t tolower(char32_t c) {
		return u_tolower(int32_t(c));
	}

	static char32_t toupper(char32_t c) {
		return u_toupper(int32_t(c));
	}

	static char32_t totitle(char32_t c) {
		return u_totitle(int32_t(c));
	}

	template <typename Interface>
	static auto tolower(WideStringView data) {
		typename Interface::WideStringType ret;
		ret.resize(data.size());

		UErrorCode status = U_ZERO_ERROR;
		auto len = u_strToLower(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		if (len <= int32_t(ret.size())) {
			ret.resize(len);
		} else {
			ret.resize(len);
			u_strToLower(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		}
		return ret;
	}

	template <typename Interface>
	static auto tolower(StringView data) {
		return string::toUtf8<Interface>(tolower<Interface>(string::toUtf16<Interface>(data)));
	}

	template <typename Interface>
	static auto toupper(WideStringView data) {
		typename Interface::WideStringType ret;
		ret.resize(data.size());

		UErrorCode status = U_ZERO_ERROR;
		auto len = u_strToUpper(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		if (len <= int32_t(ret.size())) {
			ret.resize(len);
		} else {
			ret.resize(len);
			u_strToUpper(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		}
		return ret;
	}

	template <typename Interface>
	static auto toupper(StringView data) {
		return string::toUtf8<Interface>(toupper<Interface>(string::toUtf16<Interface>(data)));
	}

	template <typename Interface>
	static auto totitle(WideStringView data) {
		typename Interface::WideStringType ret;
		ret.resize(data.size());

		UErrorCode status = U_ZERO_ERROR;
		auto len = u_strToTitle(ret.data(), ret.size(), data.data(), data.size(), NULL, NULL, &status);
		if (len <= int32_t(ret.size())) {
			ret.resize(len);
		} else {
			ret.resize(len);
			u_strToTitle(ret.data(), ret.size(), data.data(), data.size(), NULL, NULL, &status);
		}
		return ret;
	}

	template <typename Interface>
	static auto totitle(StringView data) {
		return string::toUtf8<Interface>(totitle<Interface>(string::toUtf16<Interface>(data)));
	}
#endif

	static JavaVM *s_vm = nullptr;
	static int32_t s_sdk = 0;
	static thread_local IcuJave tl_interface;
	static Dso s_icu;

	static int32_t (*tolower_fn) (int32_t) = nullptr;
	static int32_t (*toupper_fn) (int32_t) = nullptr;
	static int32_t (*totitle_fn) (int32_t) = nullptr;

	static int32_t (*strToLower_fn)(char16_t *dest, int32_t destCapacity, const char16_t *src, int32_t srcLength,
			const char *locale, int *pErrorCode) = nullptr;

	static int32_t (*strToUpper_fn)(char16_t *dest, int32_t destCapacity, const char16_t *src, int32_t srcLength,
			const char *locale, int *pErrorCode) = nullptr;

	static int32_t (*strToTitle_fn)(char16_t *dest, int32_t destCapacity, const char16_t *src, int32_t srcLength,
			void *iter, const char *locale, int *pErrorCode) = nullptr;

	static JNIEnv *getEnv() {
		void *ret = nullptr;

		s_vm->GetEnv(&ret, JNI_VERSION_1_6);
		return reinterpret_cast<JNIEnv*>(ret);
	}

	static IcuJave *getInerface() {
		if (!tl_interface.env) {
			auto env = getEnv();
			if (env) {
				tl_interface.init(s_vm, env, false);
			} else {
				JNIEnv *env = nullptr;
				s_vm->AttachCurrentThread(&env, nullptr);
				if (env) {
					tl_interface.init(s_vm, env, true);
				}
			}
		}
		return &tl_interface;
	};

	void loadJava(JavaVM *vm, int32_t sdk) {
		s_vm = vm;
		s_sdk = sdk;

		s_icu = Dso("libicu.so");
		if (s_icu) {
			tolower_fn = s_icu.sym<decltype(tolower_fn)>("u_tolower");
			toupper_fn = s_icu.sym<decltype(toupper_fn)>("u_toupper");
			totitle_fn = s_icu.sym<decltype(totitle_fn)>("u_totitle");
			strToLower_fn = s_icu.sym<decltype(strToLower_fn)>("u_strToLower");
			strToUpper_fn = s_icu.sym<decltype(strToUpper_fn)>("u_strToUpper");
			strToTitle_fn = s_icu.sym<decltype(strToTitle_fn)>("u_strToTitle");
		}
	}

	void finalizeJava() {
		s_vm = nullptr;
		s_icu.close();
	}

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
			auto len = strToTitle_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, NULL, &status);
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
}

char32_t tolower(char32_t c) {
	return i18n::tolower(c);
}

char32_t toupper(char32_t c) {
	return i18n::toupper(c);
}

char32_t totitle(char32_t c) {
	return i18n::totitle(c);
}

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
auto tolower<memory::StandartInterface>(WideStringView data) -> memory::StandartInterface::WideStringType {
	return i18n::tolower<memory::StandartInterface>(data);
}

template <>
auto toupper<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return i18n::toupper<memory::PoolInterface>(data);
}

template <>
auto toupper<memory::StandartInterface>(WideStringView data) -> memory::StandartInterface::WideStringType {
	return i18n::toupper<memory::StandartInterface>(data);
}

template <>
auto totitle<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return i18n::totitle<memory::PoolInterface>(data);
}

template <>
auto totitle<memory::StandartInterface>(WideStringView data) -> memory::StandartInterface::WideStringType {
	return i18n::totitle<memory::StandartInterface>(data);
}

}

#endif
