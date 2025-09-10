/**
 Copyright (c) 2024-2025 Stappler LLC <admin@stappler.dev>
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

// clang-format off

#ifndef CORE_CORE_STRING_SPSTRINGSTREAM_H_
#define CORE_CORE_STRING_SPSTRINGSTREAM_H_

#include "SPStringView.h" // IWYU pragma: keep
#include "SPBytesView.h" // IWYU pragma: keep
#include "SPStringDetail.h"
#include <type_traits>

namespace STAPPLER_VERSIONIZED stappler {

namespace string {

template <typename Interface>
auto toUtf16(const StringView &data) -> typename Interface::WideStringType;

template <typename Interface>
auto toUtf16(char32_t) -> typename Interface::WideStringType;

template <typename Interface>
auto toUtf16Html(const StringView &data) -> typename Interface::WideStringType;

template <typename Interface>
auto toUtf8(const WideStringView &data) -> typename Interface::StringType;

template <typename Interface>
auto toUtf8(char16_t c) -> typename Interface::StringType;

template <typename Interface>
auto toUtf8(char32_t c) -> typename Interface::StringType;

}

namespace string {

namespace detail {

inline size_t toStringValue(char *target, double val) {
	return string::detail::dtoa(val, target, string::detail::DOUBLE_MAX_DIGITS);
}

inline size_t toStringValue(char *target, float val) {
	return string::detail::dtoa(val, target, string::detail::DOUBLE_MAX_DIGITS);
}

inline size_t toStringValue(char *target, int64_t val) {
	auto len = string::detail::itoa(val, (char *)nullptr, 0);
	return string::detail::itoa(val, target, len);
}

inline size_t toStringValue(char *target, uint64_t val) {
	auto len = string::detail::itoa(val, (char *)nullptr, 0);
	return string::detail::itoa(val, target, len);
}

inline size_t toStringValue(char *target, int32_t val) {
	auto len = string::detail::itoa(int64_t(val), (char *)nullptr, 0);
	return string::detail::itoa(int64_t(val), target, len);
}

inline size_t toStringValue(char *target, uint32_t val) {
	auto len = string::detail::itoa(uint64_t(val), (char *)nullptr, 0);
	return string::detail::itoa(uint64_t(val), target, len);
}

inline size_t toStringValue(char *target, char32_t val) {
	return unicode::utf8EncodeBuf(target, val);
}

inline size_t toStringValue(char *target, char16_t val) {
	return unicode::utf8EncodeBuf(target, val);
}

inline size_t toStringValue(char *target, char val) {
	*target = val;
	return 1;
}

inline size_t toStringValue(char *target, const StringView &val) {
	memcpy(target, val.data(), val.size());
	return val.size();
}

inline size_t toStringValue(char *target, const char *val) {
	auto len = ::strlen(val);
	if (len) {
		memcpy(target, val, len);
	}
	return len;
}

template <size_t N>
inline size_t toStringValue(char *target, const char val[N]) {
	memcpy(target, val, N);
	return N;
}

inline size_t toStringValue(char *target, const std::string &val) {
	memcpy(target, val.data(), val.size());
	return val.size();
}

inline size_t toStringValue(char *target, const memory::string &val) {
	memcpy(target, val.data(), val.size());
	return val.size();
}

inline size_t toStringValue(char *target, const StringViewUtf8 &val) {
	memcpy(target, val.data(), val.size());
	return val.size();
}

template <typename Interface>
inline auto toStringType(const char16_t *val) -> typename Interface::StringType {
	return string::toUtf8<Interface>(WideStringView(val));
}

template <typename Interface>
static auto toStringType(const WideStringView &val) -> typename Interface::StringType {
	return string::toUtf8<Interface>(val);
}

template <typename Interface>
static auto toStringType(const std::u16string &val) -> typename Interface::StringType {
	return string::toUtf8<Interface>(val);
}

template <typename Interface>
static auto toStringType(const memory::u16string &val) -> typename Interface::StringType {
	return string::toUtf8<Interface>(val);
}

template <typename Interface, typename T>
static auto toStringType(const T &t) -> typename Interface::StringType {
	typename Interface::StringStreamType stream;
	stream << t;
	return stream.str();
}

template <typename T>
struct IsFastToStringAvailableValue {
	static constexpr bool value = false;
};

template <size_t N> struct IsFastToStringAvailableValue<char[N]> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<char *> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<StringView> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<std::string> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<memory::string> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<char> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<char16_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<char32_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<int64_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<uint64_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<int32_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<uint32_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<int16_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<uint16_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<int8_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<uint8_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<double> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<float> { static constexpr bool value = true; };

template <typename ... Args>
struct IsFastToStringAvailable { };

template <typename T, typename ... Args>
struct IsFastToStringAvailable<T, Args...> {
	static constexpr bool value = IsFastToStringAvailableValue<std::remove_cv_t<std::remove_reference_t<T>>>::value
			&& IsFastToStringAvailable<Args...>::value;
};

template <typename T>
struct IsFastToStringAvailable<T> {
	static constexpr bool value = IsFastToStringAvailableValue<std::remove_cv_t<std::remove_reference_t<T>>>::value;
};

inline size_t getBufferSizeValue(const char *value) { return ::strlen(value); }
template <size_t N> inline size_t getBufferSizeValue(const char value[N]) { return N; }
inline size_t getBufferSizeValue(const StringView &value) { return value.size(); }
inline size_t getBufferSizeValue(const StringViewUtf8 &value) { return value.size(); }
inline size_t getBufferSizeValue(const std::string &value) { return value.size(); }
inline size_t getBufferSizeValue(const memory::string &value) { return value.size(); }
inline size_t getBufferSizeValue(const char &value) { return 1; }
inline size_t getBufferSizeValue(const char16_t &value) { return unicode::utf8EncodeLength(value); }
inline size_t getBufferSizeValue(const char32_t &value) { return unicode::utf8EncodeLength(value); }
inline size_t getBufferSizeValue(const int64_t &value) { return string::detail::itoa(value, (char *)nullptr, 0); }
inline size_t getBufferSizeValue(const uint64_t &value) { return string::detail::itoa(value, (char *)nullptr, 0); }
inline size_t getBufferSizeValue(const int32_t &value) { return string::detail::itoa(int64_t(value), (char *)nullptr, 0); }
inline size_t getBufferSizeValue(const uint32_t &value) { return string::detail::itoa(uint64_t(value), (char *)nullptr, 0); }
inline size_t getBufferSizeValue(const int16_t &value) { return string::detail::itoa(int64_t(value), (char *)nullptr, 0); }
inline size_t getBufferSizeValue(const uint16_t &value) { return string::detail::itoa(uint64_t(value), (char *)nullptr, 0); }
inline size_t getBufferSizeValue(const int8_t &value) { return string::detail::itoa(int64_t(value), (char *)nullptr, 0); }
inline size_t getBufferSizeValue(const uint8_t &value) { return string::detail::itoa(uint64_t(value), (char *)nullptr, 0); }
inline size_t getBufferSizeValue(const double &value) { return string::detail::dtoa(value, (char *)nullptr, 0); }
inline size_t getBufferSizeValue(const float &value) { return string::detail::dtoa(value, (char *)nullptr, 0); }

inline size_t getBufferSize() {
	return 0;
}

template <typename T>
inline size_t getBufferSize(const T &t) {
	return getBufferSizeValue(t);
}

template <typename T, typename ... Args>
inline size_t getBufferSize(T &&t, Args && ...args) {
	return getBufferSizeValue(t) + getBufferSize(std::forward<Args>(args)...);
}

inline size_t writeBuffer(char *target) {
	return 0;
}

template <typename T>
inline size_t writeBuffer(char *target, const T &t) {
	return toStringValue(target, t);
}

template <typename T, typename ... Args>
inline size_t writeBuffer(char *target, T &&t, Args && ...args) {
	auto off = toStringValue(target, t);
	target += off;
	return off + writeBuffer(target, std::forward<Args>(args)...);
}

template <typename Stream>
inline void toStringStream(Stream &stream) { }

template <typename Stream>
inline void toStringStream(Stream &stream, char16_t val) {
	unicode::utf8Encode(stream, val);
}

template <typename Stream>
inline void toStringStream(Stream &stream, const char16_t *val) {
	while (*val != char16_t(0)) {
		unicode::utf8Encode(stream, *val++);
	}
}

template <typename Stream>
static void toStringStream(Stream &stream, const WideStringView &val) {
	for (auto &it : val) {
		unicode::utf8Encode(stream, it);
	}
}

template <typename Stream>
static void toStringStream(Stream &stream, const std::u16string &val) {
	for (auto &it : val) {
		unicode::utf8Encode(stream, it);
	}
}

template <typename Stream>
static void toStringStream(Stream &stream, const memory::u16string &val) {
	for (auto &it : val) {
		unicode::utf8Encode(stream, it);
	}
}

template <typename Stream, typename T>
inline void toStringStream(Stream &stream, T value) {
	stream << value;
}

template <typename Stream, typename T, typename... Args>
inline void toStringStream(Stream &stream, T value, Args && ... args) {
	stream << value;
	toStringStream(stream, std::forward<Args>(args)...);
}

}

template <typename... Args>
static StatusValue<uint32_t> toStringBuffer(char *buf, size_t bufLen, Args && ... args)  {
	if constexpr (detail::IsFastToStringAvailable<Args ...>::value) {
		auto size = detail::getBufferSize(std::forward<Args>(args)...);
		if (size > bufLen) {
			return Status::ErrorBufferOverflow;
		}

		auto s = detail::writeBuffer(buf, std::forward<Args>(args)...);
		if (s != size) {
			std::cout << "[core]: Invalid buffer size for toString<fast>\n";
			abort();
		}

		return StatusValue<uint32_t>(uint32_t(s));
	} else {
		return Status::ErrorInvalidArguemnt;
	}
}

template <typename Interface, typename... Args>
static auto toString(Args && ... args) -> typename Interface::StringType {
	if constexpr (detail::IsFastToStringAvailable<Args ...>::value) {
		// fast toString with preallocated buffer

		auto size = detail::getBufferSize(std::forward<Args>(args)...);
		typename Interface::StringType ret; ret.resize(size);

		auto s = detail::writeBuffer(ret.data(), std::forward<Args>(args)...);
		if (s != size) {
			std::cout << "[core]: Invalid buffer size for toString<fast>\n";
			abort();
		}
		ret.resize(size);
		return ret;
	} else {
		if constexpr (sizeof...(args) == 1) {
			// optimal toString for single argument
			return detail::toStringType<Interface>(std::forward<Args>(args)...);
		}

		// fallback to StringStream
		typename Interface::StringStreamType stream;
		detail::toStringStream(stream, std::forward<Args>(args)...);
		return stream.str();
	}
}


namespace wdetail {

inline size_t toStringValue(char16_t *target, double val) {
	return string::detail::dtoa(val, target, string::detail::DOUBLE_MAX_DIGITS);
}

inline size_t toStringValue(char16_t *target, float val) {
	return string::detail::dtoa(val, target, string::detail::DOUBLE_MAX_DIGITS);
}

inline size_t toStringValue(char16_t *target, int64_t val) {
	auto len = string::detail::itoa(val, (char *)nullptr, 0);
	return string::detail::itoa(val, target, len);
}

inline size_t toStringValue(char16_t *target, uint64_t val) {
	auto len = string::detail::itoa(val, (char *)nullptr, 0);
	return string::detail::itoa(val, target, len);
}

inline size_t toStringValue(char16_t *target, int32_t val) {
	auto len = string::detail::itoa(int64_t(val), (char *)nullptr, 0);
	return string::detail::itoa(int64_t(val), target, len);
}

inline size_t toStringValue(char16_t *target, uint32_t val) {
	auto len = string::detail::itoa(uint64_t(val), (char *)nullptr, 0);
	return string::detail::itoa(uint64_t(val), target, len);
}

inline size_t toStringValue(char16_t *target, char32_t val) {
	return unicode::utf16EncodeBuf(target, val);
}

inline size_t toStringValue(char16_t *target, char16_t val) {
	*target = val;
	return 1;
}

inline size_t toStringValue(char16_t *target, char val) {
	*target = val;
	return 1;
}

inline size_t toStringValue(char16_t *target, const WideStringView &val) {
	memcpy(target, val.data(), val.size() * sizeof(char16_t));
	return val.size();
}

inline size_t toStringValue(char16_t *target, const char16_t *val) {
	auto len = string::detail::length<char16_t>(val);
	if (len) {
		memcpy(target, val, len * sizeof(char16_t));
	}
	return len;
}

template <size_t N>
inline size_t toStringValue(char16_t *target, const char16_t val[N]) {
	memcpy(target, val, N * sizeof(char16_t));
	return N;
}

inline size_t toStringValue(char16_t *target, const std::u16string &val) {
	memcpy(target, val.data(), val.size() * sizeof(char16_t));
	return val.size();
}

inline size_t toStringValue(char16_t *target, const memory::u16string &val) {
	memcpy(target, val.data(), val.size() * sizeof(char16_t));
	return val.size();
}

template <typename Interface>
inline auto toStringType(const char *val) -> typename Interface::WideStringType {
	return string::toUtf16<Interface>(StringView(val));
}

template <typename Interface>
static auto toStringType(const StringView &val) -> typename Interface::WideStringType {
	return string::toUtf16<Interface>(val);
}

template <typename Interface>
static auto toStringType(const std::string &val) -> typename Interface::WideStringType {
	return string::toUtf16<Interface>(val);
}

template <typename Interface>
static auto toStringType(const memory::string &val) -> typename Interface::WideStringType {
	return string::toUtf16<Interface>(val);
}

template <typename Interface, typename T>
static auto toStringType(const T &t) -> typename Interface::WideStringType {
	typename Interface::WideStringStreamType stream;
	stream << t;
	return stream.str();
}

template <typename T>
struct IsFastToStringAvailableValue {
	static constexpr bool value = false;
};

template <size_t N> struct IsFastToStringAvailableValue<char[N]> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<char16_t *> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<WideStringView> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<std::u16string> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<memory::u16string> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<char> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<char16_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<char32_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<int64_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<uint64_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<int32_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<uint32_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<int16_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<uint16_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<int8_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<uint8_t> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<double> { static constexpr bool value = true; };
template <> struct IsFastToStringAvailableValue<float> { static constexpr bool value = true; };

template <typename ... Args>
struct IsFastToStringAvailable { };

template <typename T, typename ... Args>
struct IsFastToStringAvailable<T, Args...> {
	static constexpr bool value = IsFastToStringAvailableValue<std::remove_cv_t<std::remove_reference_t<T>>>::value
			&& IsFastToStringAvailable<Args...>::value;
};

template <typename T>
struct IsFastToStringAvailable<T> {
	static constexpr bool value = IsFastToStringAvailableValue<std::remove_cv_t<std::remove_reference_t<T>>>::value;
};

inline size_t getBufferSizeValue(const char16_t *value) { return string::detail::length(value); }
template <size_t N> inline size_t getBufferSizeValue(const char16_t value[N]) { return N; }
inline size_t getBufferSizeValue(const WideStringView &value) { return value.size(); }
inline size_t getBufferSizeValue(const std::u16string &value) { return value.size(); }
inline size_t getBufferSizeValue(const memory::u16string &value) { return value.size(); }
inline size_t getBufferSizeValue(const char &value) { return 1; }
inline size_t getBufferSizeValue(const char16_t &value) { return 1; }
inline size_t getBufferSizeValue(const char32_t &value) { return unicode::utf16EncodeLength(value); }
inline size_t getBufferSizeValue(const int64_t &value) { return string::detail::itoa(value, (char16_t *)nullptr, 0); }
inline size_t getBufferSizeValue(const uint64_t &value) { return string::detail::itoa(value, (char16_t *)nullptr, 0); }
inline size_t getBufferSizeValue(const int32_t &value) { return string::detail::itoa(int64_t(value), (char16_t *)nullptr, 0); }
inline size_t getBufferSizeValue(const uint32_t &value) { return string::detail::itoa(uint64_t(value), (char16_t *)nullptr, 0); }
inline size_t getBufferSizeValue(const int16_t &value) { return string::detail::itoa(int64_t(value), (char16_t *)nullptr, 0); }
inline size_t getBufferSizeValue(const uint16_t &value) { return string::detail::itoa(uint64_t(value), (char16_t *)nullptr, 0); }
inline size_t getBufferSizeValue(const int8_t &value) { return string::detail::itoa(int64_t(value), (char16_t *)nullptr, 0); }
inline size_t getBufferSizeValue(const uint8_t &value) { return string::detail::itoa(uint64_t(value), (char16_t *)nullptr, 0); }
inline size_t getBufferSizeValue(const double &value) { return string::detail::dtoa(value, (char16_t *)nullptr, 0); }
inline size_t getBufferSizeValue(const float &value) { return string::detail::dtoa(value, (char16_t *)nullptr, 0); }

inline size_t getBufferSize() {
	return 0;
}

template <typename T>
inline size_t getBufferSize(const T &t) {
	return getBufferSizeValue(t);
}

template <typename T, typename ... Args>
inline size_t getBufferSize(T &&t, Args && ...args) {
	return getBufferSizeValue(t) + getBufferSize(std::forward<Args>(args)...);
}

inline size_t writeBuffer(char16_t *target) {
	return 0;
}

template <typename T>
inline size_t writeBuffer(char16_t *target, const T &t) {
	return toStringValue(target, t);
}

template <typename T, typename ... Args>
inline size_t writeBuffer(char16_t *target, T &&t, Args && ...args) {
	auto off = toStringValue(target, t);
	target += off;
	return off + writeBuffer(target, std::forward<Args>(args)...);
}

template <typename Stream>
inline void toStringStream(Stream &stream) { }

template <typename Stream>
inline void toStringStream(Stream &stream, char16_t val) {
	unicode::utf8Encode(stream, val);
}

template <typename Stream>
inline void toStringStream(Stream &stream, const char16_t *val) {
	while (*val != char16_t(0)) {
		unicode::utf8Encode(stream, *val++);
	}
}

template <typename Stream>
static void toStringStream(Stream &stream, const WideStringView &val) {
	for (auto &it : val) {
		unicode::utf8Encode(stream, it);
	}
}

template <typename Stream>
static void toStringStream(Stream &stream, const std::u16string &val) {
	for (auto &it : val) {
		unicode::utf8Encode(stream, it);
	}
}

template <typename Stream>
static void toStringStream(Stream &stream, const memory::u16string &val) {
	for (auto &it : val) {
		unicode::utf8Encode(stream, it);
	}
}

template <typename Stream, typename T>
inline void toStringStream(Stream &stream, T value) {
	stream << value;
}

template <typename Stream, typename T, typename... Args>
inline void toStringStream(Stream &stream, T value, Args && ... args) {
	stream << value;
	toStringStream(stream, std::forward<Args>(args)...);
}

}

template <typename... Args>
static StatusValue<uint32_t> toWideStringBuffer(char16_t *buf, size_t bufLen, Args && ... args)  {
	if constexpr (wdetail::IsFastToStringAvailable<Args ...>::value) {
		auto size = wdetail::getBufferSize(std::forward<Args>(args)...);
		if (size > bufLen) {
			return Status::ErrorBufferOverflow;
		}

		auto s = wdetail::writeBuffer(buf, std::forward<Args>(args)...);
		if (s != size) {
			std::cout << "[core]: Invalid buffer size for toString<fast>\n";
			abort();
		}

		return StatusValue<uint32_t>(uint32_t(s));
	} else {
		return Status::ErrorInvalidArguemnt;
	}
}

template <typename Interface, typename... Args>
static auto toWideString(Args && ... args) -> typename Interface::WideStringType {
	if constexpr (wdetail::IsFastToStringAvailable<Args ...>::value) {
		// fast toString with preallocated buffer

		auto size = wdetail::getBufferSize(std::forward<Args>(args)...);
		typename Interface::WideStringType ret; ret.resize(size);

		auto s = wdetail::writeBuffer(ret.data(), std::forward<Args>(args)...);
		if (s != size) {
			std::cout << "[core]: Invalid buffer size for toString<fast>\n";
			abort();
		}
		ret.resize(size);
		return ret;
	} else {
		if constexpr (sizeof...(args) == 1) {
			// optimal toString for single argument
			return wdetail::toStringType<Interface>(std::forward<Args>(args)...);
		}

		// fallback to StringStream
		typename Interface::WideStringStreamType stream;
		wdetail::toStringStream(stream, std::forward<Args>(args)...);
		return stream.str();
	}
}

}

}

namespace STAPPLER_VERSIONIZED stappler::memory {

template <typename T, typename ReturnType, typename... ArgumentTypes>
const callback<ReturnType(ArgumentTypes...)> &operator<<(const callback<ReturnType(ArgumentTypes...)> &cb, const T &val) {
	static_assert(sizeof...(ArgumentTypes) == 1, "Functional stream should accept only one argument");

	using BaseType = std::tuple_element_t<0, std::tuple<ArgumentTypes...>>;

	static_assert(std::is_same<BaseType, StringViewBase<char>>::value
		|| std::is_same<BaseType, StringViewBase<char16_t>>::value
		|| std::is_same<BaseType, StringViewUtf8>::value
		|| std::is_same<BaseType, BytesView>::value,
		"Functional stream argument should be one of StringView, WideStringView, StringViewUtf8, BytesView");

	stappler::string::detail::streamWrite(cb, val);
	return cb;
}

}

#endif /* CORE_CORE_STRING_SPSTRINGSTREAM_H_ */
