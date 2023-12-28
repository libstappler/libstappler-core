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

#if LINUX

#include <dlfcn.h>

namespace stappler::platform {

struct i18n {
	static i18n *getInstance() {
		static i18n s_instance;
		return &s_instance;
	}

	static void *loadIcu(void *h, const char *name, StringView ver) {
		char buf[256] = { 0 };
		auto ret = ::dlsym(h, name);
		if (!ret && !ver.empty()) {
			strcpy(buf, name);
			strcat(buf, "_");
			strncat(buf, ver.data(), ver.size());

			ret = ::dlsym(h, buf);
		}
		return ret;
	}

	i18n() {
		// try unistring
		auto lib = ::dlopen("libunistring.so", RTLD_LAZY);
		if (lib) {
			tolower_fn = reinterpret_cast<decltype(tolower_fn)>(::dlsym(lib, "uc_toupper"));
			toupper_fn = reinterpret_cast<decltype(toupper_fn)>(::dlsym(lib, "uc_tolower"));
			totitle_fn = reinterpret_cast<decltype(totitle_fn)>(::dlsym(lib, "uc_totitle"));

			uc_locale_language = reinterpret_cast<decltype(uc_locale_language)>(::dlsym(lib, "uc_locale_language"));

			u8_toupper = reinterpret_cast<decltype(u8_toupper)>(::dlsym(lib, "u8_toupper"));
			u8_tolower = reinterpret_cast<decltype(u8_tolower)>(::dlsym(lib, "u8_tolower"));
			u8_totitle = reinterpret_cast<decltype(u8_totitle)>(::dlsym(lib, "u8_totitle"));

			u16_toupper = reinterpret_cast<decltype(u16_toupper)>(::dlsym(lib, "u16_toupper"));
			u16_tolower = reinterpret_cast<decltype(u16_tolower)>(::dlsym(lib, "u16_tolower"));
			u16_totitle = reinterpret_cast<decltype(u16_totitle)>(::dlsym(lib, "u16_totitle"));

			if (tolower_fn && toupper_fn && totitle_fn && uc_locale_language
					&& u8_toupper && u8_tolower && u8_totitle
					&& u16_toupper && u16_tolower && u16_totitle) {
				handle = lib;
				return;
			} else {
				::dlclose(lib);
			}
		}

		// try ICU
		char buf[256] = { 0 };
		const char *paramName;
		StringView verSuffix;

		auto dbg = ::dlopen("libicutu.so", RTLD_LAZY);
		if (dbg) {
			auto getSystemParameterNameByIndex = reinterpret_cast<const char *(*)(int32_t)>(::dlsym(dbg, "udbg_getSystemParameterNameByIndex"));
			auto getSystemParameterValueByIndex = reinterpret_cast<int32_t (*) (int32_t i, char *, int32_t, int *)>(::dlsym(dbg, "udbg_getSystemParameterValueByIndex"));

			if (getSystemParameterNameByIndex && getSystemParameterValueByIndex) {
				int status;
				for (int32_t i = 0; (paramName = getSystemParameterNameByIndex(i)) != nullptr; ++i) {
					getSystemParameterValueByIndex(i, buf, 256, &status);
					if (StringView(paramName) == "version") {
						break;
					}
				}
			}
		}

		if (StringView(paramName) == "version") {
			verSuffix = StringView(buf).readUntil<StringView::Chars<'.'>>();
		}

		lib = ::dlopen("libicuuc.so", RTLD_LAZY);
		if (lib) {
			tolower_fn = reinterpret_cast<decltype(tolower_fn)>(loadIcu(lib, "u_tolower", verSuffix));
			toupper_fn = reinterpret_cast<decltype(toupper_fn)>(loadIcu(lib, "u_toupper", verSuffix));
			totitle_fn = reinterpret_cast<decltype(totitle_fn)>(loadIcu(lib, "u_totitle", verSuffix));
			strToLower_fn = reinterpret_cast<decltype(strToLower_fn)>(loadIcu(lib, "u_strToLower", verSuffix));
			strToUpper_fn = reinterpret_cast<decltype(strToUpper_fn)>(loadIcu(lib, "u_strToUpper", verSuffix));
			strToTitle_fn = reinterpret_cast<decltype(strToTitle_fn)>(loadIcu(lib, "u_strToTitle", verSuffix));

			if (tolower_fn && toupper_fn && totitle_fn
					&& strToLower_fn && strToUpper_fn && strToTitle_fn) {
				handle = lib;
			} else {
				::dlclose(lib);
				strToLower_fn = nullptr;
				strToUpper_fn = nullptr;
				strToTitle_fn = nullptr;
			}

		}

		if (dbg) {
			::dlclose(dbg);
		}
	}

	~i18n() {
		if (handle) {
			::dlclose(handle);
		}
	}

	char32_t tolower(char32_t c) {
		return handle ? char32_t(tolower_fn(int32_t(c))) : 0;
	}

	char32_t toupper(char32_t c) {
		return handle ? char32_t(toupper_fn(int32_t(c))) : 0;
	}

	char32_t totitle(char32_t c) {
		return handle ? char32_t(totitle_fn(int32_t(c))) : 0;
	}

	template <typename Interface>
	auto tolower(StringView data) {
		if (!handle) {
			return data.str<Interface>();
		}

		if (strToLower_fn) {
			return string::toUtf8<Interface>(tolower<Interface>(string::toUtf16<Interface>(data)));
		} else {
			typename Interface::StringType ret;
			ret.resize(data.size());

			size_t targetSize = ret.size();
			auto buf = u8_tolower((const uint8_t *)data.data(), data.size(), uc_locale_language(), NULL,
					(uint8_t *)ret.data(), &targetSize);
			if (targetSize > ret.size()) {
				ret = typename Interface::StringType((char *)buf);
				::free(buf);
			} else {
				ret.resize(targetSize);
			}
			return ret;
		}
	}

	template <typename Interface>
	auto tolower(WideStringView data) {
		if (!handle) {
			return data.str<Interface>();
		}

		typename Interface::WideStringType ret;
		ret.resize(data.size());

		if (strToLower_fn) {
			int status = 0;
			auto len = strToLower_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
			if (len <= int32_t(ret.size())) {
				ret.resize(len);
			} else {
				ret.resize(len);
				strToLower_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
			}
		} else {
			size_t targetSize = ret.size();
			auto buf = u16_tolower((const uint16_t *)data.data(), data.size(), uc_locale_language(), NULL,
					(uint16_t *)ret.data(), &targetSize);
			if (targetSize > ret.size()) {
				ret = typename Interface::WideStringType((char16_t *)buf);
				::free(buf);
			} else {
				ret.resize(targetSize);
			}
		}

		return ret;
	}

	template <typename Interface>
	auto toupper(StringView data) {
		if (!handle) {
			return data.str<Interface>();
		}

		if (strToUpper_fn) {
			return string::toUtf8<Interface>(toupper<Interface>(string::toUtf16<Interface>(data)));
		} else {
			typename Interface::StringType ret;
			ret.resize(data.size());

			size_t targetSize = ret.size();
			auto buf = u8_toupper((const uint8_t *)data.data(), data.size(), uc_locale_language(), NULL,
					(uint8_t *)ret.data(), &targetSize);
			if (targetSize > ret.size()) {
				ret = typename Interface::StringType((char *)buf);
				::free(buf);
			} else {
				ret.resize(targetSize);
			}
			return ret;
		}
	}

	template <typename Interface>
	auto toupper(WideStringView data) {
		if (!handle) {
			return data.str<Interface>();
		}

		typename Interface::WideStringType ret;
		ret.resize(data.size());

		if (strToUpper_fn) {
			int status = 0;
			auto len = strToUpper_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
			if (len <= int32_t(ret.size())) {
				ret.resize(len);
			} else {
				ret.resize(len);
				strToUpper_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
			}
		} else {
			size_t targetSize = ret.size();
			auto buf = u16_toupper((const uint16_t *)data.data(), data.size(), uc_locale_language(), NULL,
					(uint16_t *)ret.data(), &targetSize);
			if (targetSize > ret.size()) {
				ret = typename Interface::WideStringType((char16_t *)buf);
				::free(buf);
			} else {
				ret.resize(targetSize);
			}
		}

		return ret;
	}

	template <typename Interface>
	auto totitle(StringView data) {
		if (!handle) {
			return data.str<Interface>();
		}

		if (strToTitle_fn) {
			return string::toUtf8<Interface>(totitle<Interface>(string::toUtf16<Interface>(data)));
		} else {
			typename Interface::StringType ret;
			ret.resize(data.size());

			size_t targetSize = ret.size();
			auto buf = u8_totitle((const uint8_t *)data.data(), data.size(), uc_locale_language(), NULL,
					(uint8_t *)ret.data(), &targetSize);
			if (targetSize > ret.size()) {
				ret = typename Interface::StringType((char *)buf);
				::free(buf);
			} else {
				ret.resize(targetSize);
			}
			return ret;
		}
	}

	template <typename Interface>
	auto totitle(WideStringView data) {
		if (!handle) {
			return data.str<Interface>();
		}

		typename Interface::WideStringType ret;
		ret.resize(data.size());

		if (strToTitle_fn) {
			int status = 0;
			auto len = strToTitle_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, NULL, &status);
			if (len <= int32_t(ret.size())) {
				ret.resize(len);
			} else {
				ret.resize(len);
				strToTitle_fn(ret.data(), ret.size(), data.data(), data.size(), NULL, NULL, &status);
			}
		} else {
			size_t targetSize = ret.size();
			auto buf = u16_totitle((const uint16_t *)data.data(), data.size(), uc_locale_language(), NULL,
					(uint16_t *)ret.data(), &targetSize);
			if (targetSize > ret.size()) {
				ret = typename Interface::WideStringType((char16_t *)buf);
				::free(buf);
			} else {
				ret.resize(targetSize);
			}
		}

		return ret;
	}

	int32_t (*tolower_fn) (int32_t) = nullptr;
	int32_t (*toupper_fn) (int32_t) = nullptr;
	int32_t (*totitle_fn) (int32_t) = nullptr;

	int32_t (*strToLower_fn)(char16_t *dest, int32_t destCapacity, const char16_t *src, int32_t srcLength,
			const char *locale, int *pErrorCode) = nullptr;

	int32_t (*strToUpper_fn)(char16_t *dest, int32_t destCapacity, const char16_t *src, int32_t srcLength,
			const char *locale, int *pErrorCode) = nullptr;

	int32_t (*strToTitle_fn)(char16_t *dest, int32_t destCapacity, const char16_t *src, int32_t srcLength,
			void *iter, const char *locale, int *pErrorCode) = nullptr;

	const char * (* uc_locale_language) () = nullptr;

	uint8_t * (* u8_toupper) (const uint8_t *s, size_t n, const char *iso639_language, void *nf, uint8_t *resultbuf, size_t *lengthp) = nullptr;
	uint8_t * (* u8_tolower) (const uint8_t *s, size_t n, const char *iso639_language, void *nf, uint8_t *resultbuf, size_t *lengthp) = nullptr;
	uint8_t * (* u8_totitle) (const uint8_t *s, size_t n, const char *iso639_language, void *nf, uint8_t *resultbuf, size_t *lengthp) = nullptr;

	uint16_t* (*u16_toupper) (const uint16_t *s, size_t n, const char *iso639_language, void *nf, uint16_t *resultbuf, size_t *lengthp) = nullptr;
	uint16_t* (*u16_tolower) (const uint16_t *s, size_t n, const char *iso639_language, void *nf, uint16_t *resultbuf, size_t *lengthp) = nullptr;
	uint16_t* (*u16_totitle) (const uint16_t *s, size_t n, const char *iso639_language, void *nf, uint16_t *resultbuf, size_t *lengthp) = nullptr;

	void *handle = nullptr;
};

static i18n *s_instance = i18n::getInstance();

char32_t tolower(char32_t c) {
	return s_instance->tolower(c);
}

char32_t toupper(char32_t c) {
	return s_instance->toupper(c);
}

char32_t totitle(char32_t c) {
	return s_instance->totitle(c);
}

template <>
auto tolower<memory::PoolInterface>(StringView data) -> memory::PoolInterface::StringType {
	return s_instance->tolower<memory::PoolInterface>(data);
}

template <>
auto tolower<memory::StandartInterface>(StringView data) -> memory::StandartInterface::StringType {
	return s_instance->tolower<memory::StandartInterface>(data);
}

template <>
auto toupper<memory::PoolInterface>(StringView data) -> memory::PoolInterface::StringType {
	return s_instance->toupper<memory::PoolInterface>(data);
}

template <>
auto toupper<memory::StandartInterface>(StringView data) -> memory::StandartInterface::StringType {
	return s_instance->toupper<memory::StandartInterface>(data);
}

template <>
auto totitle<memory::PoolInterface>(StringView data) -> memory::PoolInterface::StringType {
	return s_instance->totitle<memory::PoolInterface>(data);
}

template <>
auto totitle<memory::StandartInterface>(StringView data) -> memory::StandartInterface::StringType {
	return s_instance->totitle<memory::StandartInterface>(data);
}

template <>
auto tolower<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return s_instance->tolower<memory::PoolInterface>(data);
}

template <>
auto tolower<memory::StandartInterface>(WideStringView data) -> memory::StandartInterface::WideStringType {
	return s_instance->tolower<memory::StandartInterface>(data);
}

template <>
auto toupper<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return s_instance->toupper<memory::PoolInterface>(data);
}

template <>
auto toupper<memory::StandartInterface>(WideStringView data) -> memory::StandartInterface::WideStringType {
	return s_instance->toupper<memory::StandartInterface>(data);
}

template <>
auto totitle<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return s_instance->totitle<memory::PoolInterface>(data);
}

template <>
auto totitle<memory::StandartInterface>(WideStringView data) -> memory::StandartInterface::WideStringType {
	return s_instance->totitle<memory::StandartInterface>(data);
}

}

#endif
