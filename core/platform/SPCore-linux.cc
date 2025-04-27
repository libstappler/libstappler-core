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

#include "SPString.h"
#include "SPDso.h"

#if LINUX

#include <sys/random.h>

namespace STAPPLER_VERSIONIZED stappler::platform {

constexpr uint32_t U_COMPARE_CODE_POINT_ORDER = 0x8000;

struct unistring_iface {
	using u8_case_fn = uint8_t *(*)(const uint8_t *s, size_t n, const char *iso639_language,
			void *nf, uint8_t *resultbuf, size_t *lengthp);
	using u16_case_fn = uint16_t *(*)(const uint16_t *s, size_t n, const char *iso639_language,
			void *nf, uint16_t *resultbuf, size_t *lengthp);

	int32_t (*tolower_fn)(int32_t) = nullptr;
	int32_t (*toupper_fn)(int32_t) = nullptr;
	int32_t (*totitle_fn)(int32_t) = nullptr;

	const char *(*uc_locale_language)() = nullptr;

	u8_case_fn u8_toupper = nullptr;
	u8_case_fn u8_tolower = nullptr;
	u8_case_fn u8_totitle = nullptr;

	int (*u8_cmp2)(const uint8_t *s1, size_t n1, const uint8_t *s2, size_t n2) = nullptr;
	int (*u8_casecoll)(const uint8_t *s1, size_t n1, const uint8_t *s2, size_t n2,
			const char *iso639_language, void *nf, int *resultp) = nullptr;


	u16_case_fn u16_toupper = nullptr;
	u16_case_fn u16_tolower = nullptr;
	u16_case_fn u16_totitle = nullptr;

	int (*u16_cmp2)(const uint16_t *s1, size_t n1, const uint16_t *s2, size_t n2) = nullptr;
	int (*u16_casecoll)(const uint16_t *s1, size_t n1, const uint16_t *s2, size_t n2,
			const char *iso639_language, void *nf, int *resultp) = nullptr;

	void load(Dso &handle) {
		tolower_fn = handle.sym<decltype(tolower_fn)>("uc_tolower");
		toupper_fn = handle.sym<decltype(toupper_fn)>("uc_toupper");
		totitle_fn = handle.sym<decltype(totitle_fn)>("uc_totitle");

		uc_locale_language = handle.sym<decltype(uc_locale_language)>("uc_locale_language");

		u8_toupper = handle.sym<decltype(u8_toupper)>("u8_toupper");
		u8_tolower = handle.sym<decltype(u8_tolower)>("u8_tolower");
		u8_totitle = handle.sym<decltype(u8_totitle)>("u8_totitle");

		u8_cmp2 = handle.sym<decltype(u8_cmp2)>("u8_cmp2");
		u8_casecoll = handle.sym<decltype(u8_casecoll)>("u8_casecoll");

		u16_toupper = handle.sym<decltype(u16_toupper)>("u16_toupper");
		u16_tolower = handle.sym<decltype(u16_tolower)>("u16_tolower");
		u16_totitle = handle.sym<decltype(u16_totitle)>("u16_totitle");

		u16_cmp2 = handle.sym<decltype(u16_cmp2)>("u16_cmp2");
		u16_casecoll = handle.sym<decltype(u16_casecoll)>("u16_casecoll");
	}

	explicit operator bool() const {
		return uc_locale_language && tolower_fn && toupper_fn && totitle_fn && u8_toupper
				&& u8_tolower && u8_totitle && u8_cmp2 && u8_casecoll && u16_toupper && u16_tolower
				&& u16_totitle && u16_cmp2 && u16_casecoll;
	}

	void clear() {
		tolower_fn = nullptr;
		toupper_fn = nullptr;
		totitle_fn = nullptr;

		uc_locale_language = nullptr;

		u8_toupper = nullptr;
		u8_tolower = nullptr;
		u8_totitle = nullptr;

		u8_cmp2 = nullptr;
		u8_casecoll = nullptr;

		u16_toupper = nullptr;
		u16_tolower = nullptr;
		u16_totitle = nullptr;

		u16_cmp2 = nullptr;
		u16_casecoll = nullptr;
	}
};

struct icu_iface {
	using case_fn = int32_t (*)(char16_t *dest, int32_t destCapacity, const char16_t *src,
			int32_t srcLength, const char *locale, int *pErrorCode);
	using case_iter_fn = int32_t (*)(char16_t *dest, int32_t destCapacity, const char16_t *src,
			int32_t srcLength, void *iter, const char *locale, int *pErrorCode);

	using cmp_fn = int32_t (*)(const char16_t *s1, int32_t length1, const char16_t *s2,
			int32_t length2, int8_t codePointOrder);
	using case_cmp_fn = int32_t (*)(const char16_t *s1, int32_t length1, const char16_t *s2,
			int32_t length2, uint32_t options, int *pErrorCode);

	int32_t (*tolower_fn)(int32_t) = nullptr;
	int32_t (*toupper_fn)(int32_t) = nullptr;
	int32_t (*totitle_fn)(int32_t) = nullptr;

	case_fn u_strToLower = nullptr;
	case_fn u_strToUpper = nullptr;
	case_iter_fn u_strToTitle = nullptr;

	cmp_fn u_strCompare = nullptr;
	case_cmp_fn u_strCaseCompare = nullptr;

	static void *loadIcu(Dso &h, const char *name, StringView ver) {
		char buf[256] = {0};
		auto ret = h.sym<void *>(name);
		if (!ret && !ver.empty()) {
			strcpy(buf, name);
			strcat(buf, "_");
			strncat(buf, ver.data(), ver.size());

			ret = h.sym<void *>(buf);
		}
		return ret;
	}

	void load(Dso &handle, StringView verSuffix) {
		tolower_fn =
				reinterpret_cast<decltype(tolower_fn)>(loadIcu(handle, "u_tolower", verSuffix));
		toupper_fn =
				reinterpret_cast<decltype(toupper_fn)>(loadIcu(handle, "u_toupper", verSuffix));
		totitle_fn =
				reinterpret_cast<decltype(totitle_fn)>(loadIcu(handle, "u_totitle", verSuffix));
		u_strToLower = reinterpret_cast<decltype(u_strToLower)>(
				loadIcu(handle, "u_strToLower", verSuffix));
		u_strToUpper = reinterpret_cast<decltype(u_strToUpper)>(
				loadIcu(handle, "u_strToUpper", verSuffix));
		u_strToTitle = reinterpret_cast<decltype(u_strToTitle)>(
				loadIcu(handle, "u_strToTitle", verSuffix));
		u_strCompare = reinterpret_cast<decltype(u_strCompare)>(
				loadIcu(handle, "u_strCompare", verSuffix));
		u_strCaseCompare = reinterpret_cast<decltype(u_strCaseCompare)>(
				loadIcu(handle, "u_strCaseCompare", verSuffix));
	}

	explicit operator bool() const {
		return tolower_fn && toupper_fn && totitle_fn && u_strToLower && u_strToUpper
				&& u_strToTitle && u_strCompare && u_strCaseCompare;
	}

	void clear() {
		tolower_fn = nullptr;
		toupper_fn = nullptr;
		totitle_fn = nullptr;
		u_strToLower = nullptr;
		u_strToUpper = nullptr;
		u_strToTitle = nullptr;
		u_strCompare = nullptr;
		u_strCaseCompare = nullptr;
	}
};

struct i18n {
	static i18n *getInstance() {
		static i18n s_instance;
		return &s_instance;
	}

	i18n() {
		// try unistring
		// try version 0 or 1 if no general symlink
		_handle = Dso("libunistring.so");
		if (!_handle) {
			_handle = Dso("libunistring.so.1");
		}
		if (!_handle) {
			_handle = Dso("libunistring.so.0");
		}
		if (_handle) {
			unistring.load(_handle);
			if (unistring) {
				return;
			} else {
				unistring.clear();
				_handle.close();
			}
		}

		// try ICU
		char buf[256] = {0};
		const char *paramName = nullptr;
		StringView verSuffix;

		auto dbg = Dso("libicutu.so");
		if (dbg) {
			auto getSystemParameterNameByIndex =
					dbg.sym<const char *(*)(int32_t)>("udbg_getSystemParameterNameByIndex");
			auto getSystemParameterValueByIndex =
					dbg.sym<int32_t (*)(int32_t i, char *, int32_t, int *)>(
							"udbg_getSystemParameterValueByIndex");

			if (getSystemParameterNameByIndex && getSystemParameterValueByIndex) {
				int status;
				for (int32_t i = 0; (paramName = getSystemParameterNameByIndex(i)) != nullptr;
						++i) {
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
			icu.load(_handle, verSuffix);
			if (!icu) {
				icu.clear();
				_handle.close();
			}
		}
	}

	~i18n() { }

	char32_t tolower(char32_t c) {
		return _handle
				? char32_t(icu ? icu.tolower_fn(int32_t(c)) : unistring.tolower_fn(int32_t(c)))
				: 0;
	}

	char32_t toupper(char32_t c) {
		return _handle
				? char32_t(icu ? icu.toupper_fn(int32_t(c)) : unistring.toupper_fn(int32_t(c)))
				: 0;
	}

	char32_t totitle(char32_t c) {
		return _handle
				? char32_t(icu ? icu.totitle_fn(int32_t(c)) : unistring.totitle_fn(int32_t(c)))
				: 0;
	}

	template <typename Interface>
	auto applyIcoFunction(WideStringView data, icu_iface::case_fn icuFn) {
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
	auto applyUnistringFunction(StringView data, unistring_iface::u8_case_fn ustrFn) {
		typename Interface::StringType ret;
		ret.resize(data.size());

		size_t targetSize = ret.size();
		auto buf = ustrFn((const uint8_t *)data.data(), data.size(), unistring.uc_locale_language(),
				NULL, (uint8_t *)ret.data(), &targetSize);
		if (targetSize > ret.size()) {
			ret = typename Interface::StringType((char *)buf);
			::free(buf);
		} else {
			ret.resize(targetSize);
		}
		return ret;
	}

	template <typename Interface>
	auto applyUnistringFunction(WideStringView data, unistring_iface::u16_case_fn ustrFn) {
		typename Interface::WideStringType ret;
		ret.resize(data.size());

		size_t targetSize = ret.size();
		auto buf = ustrFn((const uint16_t *)data.data(), data.size(),
				unistring.uc_locale_language(), NULL, (uint16_t *)ret.data(), &targetSize);
		if (targetSize > ret.size()) {
			ret = typename Interface::WideStringType((char16_t *)buf);
			::free(buf);
		} else {
			ret.resize(targetSize);
		}
		return ret;
	}

	template <typename Interface>
	auto applyFunction(StringView data, icu_iface::case_fn icuFn,
			unistring_iface::u8_case_fn ustrFn) {
		if (!_handle) {
			return data.str<Interface>();
		}

		if (icuFn) {
			return string::toUtf8<Interface>(
					applyIcoFunction<Interface>(string::toUtf16<Interface>(data), icuFn));
		} else {
			return applyUnistringFunction<Interface>(data, ustrFn);
		}
	}

	template <typename Interface>
	auto applyFunction(WideStringView data, icu_iface::case_fn icuFn,
			unistring_iface::u16_case_fn ustrFn) {
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
		return applyFunction<Interface>(data, icu.u_strToLower, unistring.u8_tolower);
	}

	template <typename Interface>
	auto tolower(WideStringView data) {
		return applyFunction<Interface>(data, icu.u_strToLower, unistring.u16_tolower);
	}

	template <typename Interface>
	auto toupper(StringView data) {
		return applyFunction<Interface>(data, icu.u_strToUpper, unistring.u8_toupper);
	}

	template <typename Interface>
	auto toupper(WideStringView data) {
		return applyFunction<Interface>(data, icu.u_strToUpper, unistring.u16_toupper);
	}

	template <typename Interface>
	auto totitle(StringView data) {
		if (!_handle) {
			return data.str<Interface>();
		}

		if (icu.u_strToTitle) {
			return string::toUtf8<Interface>(totitle<Interface>(string::toUtf16<Interface>(data)));
		} else {
			return applyUnistringFunction<Interface>(data, unistring.u8_totitle);
		}
	}

	template <typename Interface>
	auto totitle(WideStringView data) {
		if (!_handle) {
			return data.str<Interface>();
		}

		typename Interface::WideStringType ret;
		ret.resize(data.size());

		if (icu.u_strToTitle) {
			int status = 0;
			auto len = icu.u_strToTitle(ret.data(), ret.size(), data.data(), data.size(), NULL,
					NULL, &status);
			if (len <= int32_t(ret.size())) {
				ret.resize(len);
			} else {
				ret.resize(len);
				icu.u_strToTitle(ret.data(), ret.size(), data.data(), data.size(), NULL, NULL,
						&status);
			}
		} else {
			return applyUnistringFunction<Interface>(data, unistring.u16_totitle);
		}

		return ret;
	}

	int compare(StringView l, StringView r) {
		if (unistring.u8_cmp2) {
			return unistring.u8_cmp2((const uint8_t *)l.data(), l.size(), (const uint8_t *)r.data(),
					r.size());
		} else if (icu.u_strCompare) {
			auto lStr = string::toUtf16<memory::StandartInterface>(l);
			auto rStr = string::toUtf16<memory::StandartInterface>(r);
			return icu.u_strCompare(lStr.data(), lStr.size(), rStr.data(), rStr.size(), 1);
		}
		return string::detail::compare_c(l, r);
	}

	int compare(WideStringView l, WideStringView r) {
		if (unistring.u16_cmp2) {
			return unistring.u16_cmp2((const uint16_t *)l.data(), l.size(),
					(const uint16_t *)r.data(), r.size());
		} else if (icu.u_strCompare) {
			return icu.u_strCompare(l.data(), l.size(), r.data(), r.size(), 1);
		}
		return string::detail::compare_c(l, r);
	}
	int caseCompare(StringView l, StringView r) {
		if (unistring.u8_casecoll) {
			int32_t ret = 0;
			if (unistring.u8_casecoll((const uint8_t *)l.data(), l.size(),
						(const uint8_t *)r.data(), r.size(), unistring.uc_locale_language(),
						nullptr, &ret)
					== 0) {
				return ret;
			}
		} else if (icu.u_strCaseCompare) {
			int status = 0;
			auto lStr = string::toUtf16<memory::StandartInterface>(l);
			auto rStr = string::toUtf16<memory::StandartInterface>(r);
			return icu.u_strCaseCompare(lStr.data(), lStr.size(), rStr.data(), rStr.size(),
					U_COMPARE_CODE_POINT_ORDER, &status);
		}
		return string::detail::caseCompare_c(l, r);
	}

	int caseCompare(WideStringView l, WideStringView r) {
		if (unistring.u16_casecoll) {
			int32_t ret = 0;
			if (unistring.u16_casecoll((const uint16_t *)l.data(), l.size(),
						(const uint16_t *)r.data(), r.size(), unistring.uc_locale_language(),
						nullptr, &ret)
					== 0) {
				return ret;
			}
		} else if (icu.u_strCaseCompare) {
			int status = 0;
			return icu.u_strCaseCompare(l.data(), l.size(), r.data(), r.size(),
					U_COMPARE_CODE_POINT_ORDER, &status);
		}
		return string::detail::caseCompare_c(l, r);
	}

	icu_iface icu;
	unistring_iface unistring;

	Dso _handle;
};

static i18n *s_instance = i18n::getInstance();

char32_t tolower(char32_t c) { return s_instance->tolower(c); }

char32_t toupper(char32_t c) { return s_instance->toupper(c); }

char32_t totitle(char32_t c) { return s_instance->totitle(c); }

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
auto tolower<memory::StandartInterface>(WideStringView data)
		-> memory::StandartInterface::WideStringType {
	return s_instance->tolower<memory::StandartInterface>(data);
}

template <>
auto toupper<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return s_instance->toupper<memory::PoolInterface>(data);
}

template <>
auto toupper<memory::StandartInterface>(WideStringView data)
		-> memory::StandartInterface::WideStringType {
	return s_instance->toupper<memory::StandartInterface>(data);
}

template <>
auto totitle<memory::PoolInterface>(WideStringView data) -> memory::PoolInterface::WideStringType {
	return s_instance->totitle<memory::PoolInterface>(data);
}

template <>
auto totitle<memory::StandartInterface>(WideStringView data)
		-> memory::StandartInterface::WideStringType {
	return s_instance->totitle<memory::StandartInterface>(data);
}

int compare_u(StringView l, StringView r) { return s_instance->compare(l, r); }

int compare_u(WideStringView l, WideStringView r) { return s_instance->compare(l, r); }

int caseCompare_u(StringView l, StringView r) { return s_instance->caseCompare(l, r); }

int caseCompare_u(WideStringView l, WideStringView r) { return s_instance->caseCompare(l, r); }

size_t makeRandomBytes(uint8_t *buf, size_t count) {
	size_t generated = 0;
	auto ret = ::getrandom(buf, count, GRND_RANDOM | GRND_NONBLOCK);
	if (ret < ssize_t(count)) {
		buf += ret;
		count -= ret;
		generated += ret;

		ret = ::getrandom(buf, count, GRND_NONBLOCK);
		if (ret >= 0) {
			generated += ret;
		}
	} else {
		generated += ret;
	}
	return generated;
}

bool initialize(int &resultCode) { return true; }

void terminate() { }

} // namespace stappler::platform

#endif
