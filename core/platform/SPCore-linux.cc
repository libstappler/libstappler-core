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
#include "SPDso.h"

#if LINUX

namespace stappler::platform {

struct i18n {
	using icu_case_fn = int32_t (*) (char16_t *dest, int32_t destCapacity, const char16_t *src, int32_t srcLength,
			const char *locale, int *pErrorCode);
	using icu_case_iter_fn = int32_t (*) (char16_t *dest, int32_t destCapacity, const char16_t *src, int32_t srcLength,
			void *iter, const char *locale, int *pErrorCode);

	using ustr_u8_case_fn = uint8_t * (*) (const uint8_t *s, size_t n, const char *iso639_language, void *nf, uint8_t *resultbuf, size_t *lengthp);
	using ustr_u16_case_fn = uint16_t* (*) (const uint16_t *s, size_t n, const char *iso639_language, void *nf, uint16_t *resultbuf, size_t *lengthp);

	static i18n *getInstance() {
		static i18n s_instance;
		return &s_instance;
	}

	static void *loadIcu(Dso &h, const char *name, StringView ver) {
		char buf[256] = { 0 };
		auto ret = h.sym<void *>(name);
		if (!ret && !ver.empty()) {
			strcpy(buf, name);
			strcat(buf, "_");
			strncat(buf, ver.data(), ver.size());

			ret = h.sym<void *>(buf);
		}
		return ret;
	}

	i18n() {
		// try unistring
		_handle = Dso("libunistring.so");
		if (_handle) {
			tolower_fn = _handle.sym<decltype(tolower_fn)>("uc_toupper");
			toupper_fn = _handle.sym<decltype(toupper_fn)>("uc_tolower");
			totitle_fn = _handle.sym<decltype(totitle_fn)>("uc_totitle");

			uc_locale_language = _handle.sym<decltype(uc_locale_language)>("uc_locale_language");

			u8_toupper = _handle.sym<decltype(u8_toupper)>("u8_toupper");
			u8_tolower = _handle.sym<decltype(u8_tolower)>("u8_tolower");
			u8_totitle = _handle.sym<decltype(u8_totitle)>("u8_totitle");

			u16_toupper = _handle.sym<decltype(u16_toupper)>("u16_toupper");
			u16_tolower = _handle.sym<decltype(u16_tolower)>("u16_tolower");
			u16_totitle = _handle.sym<decltype(u16_totitle)>("u16_totitle");

			if (tolower_fn && toupper_fn && totitle_fn && uc_locale_language
					&& u8_toupper && u8_tolower && u8_totitle
					&& u16_toupper && u16_tolower && u16_totitle) {
				return;
			} else {
				_handle.close();
			}
		}

		// try ICU
		char buf[256] = { 0 };
		const char *paramName;
		StringView verSuffix;

		auto dbg = Dso("libicutu.so");
		if (dbg) {
			auto getSystemParameterNameByIndex = dbg.sym<const char *(*)(int32_t)>("udbg_getSystemParameterNameByIndex");
			auto getSystemParameterValueByIndex = dbg.sym<int32_t (*) (int32_t i, char *, int32_t, int *)>("udbg_getSystemParameterValueByIndex");

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

		_handle = Dso("libicuuc.so");
		if (_handle) {
			tolower_fn = reinterpret_cast<decltype(tolower_fn)>(loadIcu(_handle, "u_tolower", verSuffix));
			toupper_fn = reinterpret_cast<decltype(toupper_fn)>(loadIcu(_handle, "u_toupper", verSuffix));
			totitle_fn = reinterpret_cast<decltype(totitle_fn)>(loadIcu(_handle, "u_totitle", verSuffix));
			strToLower_fn = reinterpret_cast<decltype(strToLower_fn)>(loadIcu(_handle, "u_strToLower", verSuffix));
			strToUpper_fn = reinterpret_cast<decltype(strToUpper_fn)>(loadIcu(_handle, "u_strToUpper", verSuffix));
			strToTitle_fn = reinterpret_cast<decltype(strToTitle_fn)>(loadIcu(_handle, "u_strToTitle", verSuffix));

			if (tolower_fn && toupper_fn && totitle_fn
					&& strToLower_fn && strToUpper_fn && strToTitle_fn) {
				return;
			} else {
				strToLower_fn = nullptr;
				strToUpper_fn = nullptr;
				strToTitle_fn = nullptr;
			}

		}
		_handle.close();
	}

	~i18n() { }

	char32_t tolower(char32_t c) {
		return _handle ? char32_t(tolower_fn(int32_t(c))) : 0;
	}

	char32_t toupper(char32_t c) {
		return _handle ? char32_t(toupper_fn(int32_t(c))) : 0;
	}

	char32_t totitle(char32_t c) {
		return _handle ? char32_t(totitle_fn(int32_t(c))) : 0;
	}

	template <typename Interface>
	auto applyIcoFunction(WideStringView data, icu_case_fn icuFn) {
		typename Interface::WideStringType ret;
		ret.resize(data.size());

		int status = 0;
		auto len = icuFn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		if (len <= int32_t(ret.size())) {
			ret.resize(len);
		} else {
			ret.resize(len);
			icuFn(ret.data(), ret.size(), data.data(), data.size(), NULL, &status);
		}
		return ret;
	}

	template <typename Interface>
	auto applyUnistringFunction(StringView data, ustr_u8_case_fn ustrFn) {
		typename Interface::StringType ret;
		ret.resize(data.size());

		size_t targetSize = ret.size();
		auto buf = ustrFn((const uint8_t *)data.data(), data.size(), uc_locale_language(), NULL,
				(uint8_t *)ret.data(), &targetSize);
		if (targetSize > ret.size()) {
			ret = typename Interface::StringType((char *)buf);
			::free(buf);
		} else {
			ret.resize(targetSize);
		}
		return ret;
	}

	template <typename Interface>
	auto applyUnistringFunction(WideStringView data, ustr_u16_case_fn ustrFn) {
		typename Interface::WideStringType ret;
		ret.resize(data.size());

		size_t targetSize = ret.size();
		auto buf = ustrFn((const uint16_t *)data.data(), data.size(), uc_locale_language(), NULL,
				(uint16_t *)ret.data(), &targetSize);
		if (targetSize > ret.size()) {
			ret = typename Interface::WideStringType((char16_t *)buf);
			::free(buf);
		} else {
			ret.resize(targetSize);
		}
		return ret;
	}

	template <typename Interface>
	auto applyFunction(StringView data, icu_case_fn icuFn, ustr_u8_case_fn ustrFn) {
		if (!_handle) {
			return data.str<Interface>();
		}

		if (icuFn) {
			return string::toUtf8<Interface>(
				applyIcoFunction<Interface>(
					string::toUtf16<Interface>(data),
					icuFn));
		} else {
			return applyUnistringFunction<Interface>(data, ustrFn);
		}
	}

	template <typename Interface>
	auto applyFunction(WideStringView data, icu_case_fn icuFn, ustr_u16_case_fn ustrFn) {
		if (!_handle) {
			return data.str<Interface>();
		}

		if (icuFn) {
			return applyIcoFunction<Interface>(data, icuFn);
		} else {
			return applyUnistringFunction<Interface>(data, ustrFn);
		}
	}

	template <typename Interface>
	auto tolower(StringView data) {
		return applyFunction<Interface>(data, strToLower_fn, u8_tolower);
	}

	template <typename Interface>
	auto tolower(WideStringView data) {
		return applyFunction<Interface>(data, strToLower_fn, u16_tolower);
	}

	template <typename Interface>
	auto toupper(StringView data) {
		return applyFunction<Interface>(data, strToUpper_fn, u8_toupper);
	}

	template <typename Interface>
	auto toupper(WideStringView data) {
		return applyFunction<Interface>(data, strToUpper_fn, u16_toupper);
	}

	template <typename Interface>
	auto totitle(StringView data) {
		if (!_handle) {
			return data.str<Interface>();
		}

		if (strToTitle_fn) {
			return string::toUtf8<Interface>(totitle<Interface>(string::toUtf16<Interface>(data)));
		} else {
			return applyUnistringFunction<Interface>(data, u8_totitle);
		}
	}

	template <typename Interface>
	auto totitle(WideStringView data) {
		if (!_handle) {
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
			return applyUnistringFunction<Interface>(data, u16_totitle);
		}

		return ret;
	}

	int32_t (*tolower_fn) (int32_t) = nullptr;
	int32_t (*toupper_fn) (int32_t) = nullptr;
	int32_t (*totitle_fn) (int32_t) = nullptr;

	icu_case_fn strToLower_fn = nullptr;
	icu_case_fn strToUpper_fn = nullptr;
	icu_case_iter_fn strToTitle_fn = nullptr;

	const char * (* uc_locale_language) () = nullptr;

	ustr_u8_case_fn u8_toupper = nullptr;
	ustr_u8_case_fn u8_tolower = nullptr;
	ustr_u8_case_fn u8_totitle = nullptr;

	ustr_u16_case_fn u16_toupper = nullptr;
	ustr_u16_case_fn u16_tolower = nullptr;
	ustr_u16_case_fn u16_totitle = nullptr;

	Dso _handle;
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
