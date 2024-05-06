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

#ifndef CORE_CORE_STRING_SPSTRINGSTREAM_H_
#define CORE_CORE_STRING_SPSTRINGSTREAM_H_

#include "SPStringView.h"

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

namespace detail {

template <typename FunctionalStreamArg>
struct FunctionalStreamCharTraits { };

template <typename Char>
struct FunctionalStreamCharTraits<StringViewBase<Char>> {
	using CharType = Char;
};

template <>
struct FunctionalStreamCharTraits<StringViewUtf8> {
	using CharType = StringViewUtf8::CharType;
};

template <typename FunctionalStream>
struct FunctionalStreamTraits { };

template <typename Arg>
struct FunctionalStreamTraits<Callback<void(Arg)>> {
	using ArgType = Arg;
	using CharType = typename FunctionalStreamCharTraits<ArgType>::CharType;
};

template <typename Arg>
struct FunctionalStreamTraits<std::function<void(Arg)>> {
	using ArgType = Arg;
	using CharType = typename FunctionalStreamCharTraits<ArgType>::CharType;
};

template <typename Arg>
struct FunctionalStreamTraits<memory::function<void(Arg)>> {
	using ArgType = Arg;
	using CharType = typename FunctionalStreamCharTraits<ArgType>::CharType;
};

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType str) {
	stream(str);
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, const typename FunctionalStreamTraits<FunctionalStream>::CharType *str) {
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(str));
}

template <typename FunctionalStream, size_t N>
inline void streamWrite(const FunctionalStream &stream, const typename FunctionalStreamTraits<FunctionalStream>::CharType str[N]) {
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(str, N));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, double d) {
	std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, string::DOUBLE_MAX_DIGITS> buf = { 0 };
	auto ret = string::_dtoa(d, buf.data(), buf.size());
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data(), ret));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, float f) {
	streamWrite(stream, double(f));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, int64_t i) {
	std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, std::numeric_limits<int64_t>::digits10 + 2> buf = { 0 };
	auto ret = string::_itoa(i, buf.data(), buf.size());
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data() + buf.size() - ret, ret));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, uint64_t i) {
	std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, std::numeric_limits<int64_t>::digits10 + 2> buf = { 0 };
	auto ret = string::_itoa(i, buf.data(), buf.size());
	streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data() + buf.size() - ret, ret));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, int32_t i) {
	streamWrite(stream, int64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, uint32_t i) {
	streamWrite(stream, uint64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, int16_t i) {
	streamWrite(stream, int64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, uint16_t i) {
	streamWrite(stream, uint64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, int8_t i) {
	streamWrite(stream, int64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, uint8_t i) {
	streamWrite(stream, uint64_t(i));
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, char32_t c) {
	if constexpr (sizeof(typename FunctionalStreamTraits<FunctionalStream>::CharType) == 1) {
		std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, 6> buf = { 0 };
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data(), unicode::utf8EncodeBuf(buf.data(), c)));
	} else {
		std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, 6> buf = { 0 };
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data(), unicode::utf16EncodeBuf(buf.data(), c)));
	}
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, char16_t c) {
	if constexpr (sizeof(typename FunctionalStreamTraits<FunctionalStream>::CharType) == 1) {
		std::array<typename FunctionalStreamTraits<FunctionalStream>::CharType, 4> buf = { 0 };
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(buf.data(), unicode::utf8EncodeBuf(buf.data(), c)));
	} else {
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(&c, 1));
	}
}

template <typename FunctionalStream>
inline void streamWrite(const FunctionalStream &stream, char c) {
	if constexpr (sizeof(typename FunctionalStreamTraits<FunctionalStream>::CharType) == 1) {
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(&c, 1));
	} else {
		char16_t ch = c;
		streamWrite(stream, typename FunctionalStreamTraits<FunctionalStream>::ArgType(&ch, 1));
	}
}

}

#define SP_DEFINE_STREAM_OVERLOADS(Type, Char) \
	inline auto operator<<(const Type &stream, const Char *val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	template <size_t N> inline auto operator<<(const Type &stream, const Char val[N]) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, double val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, float val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, int64_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, uint64_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, int32_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, uint32_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, int16_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, uint16_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, int8_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, uint8_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, char32_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, char16_t val) -> const Type & { detail::streamWrite(stream, val); return stream; } \
	inline auto operator<<(const Type &stream, char val) -> const Type & { detail::streamWrite(stream, val); return stream; } \

inline auto operator<<(const Callback<void(StringViewBase<char>)> &cb, const StringViewBase<char> &val) -> const Callback<void(StringViewBase<char>)> & {
	detail::streamWrite(cb, val);
	return cb;
}

inline auto operator<<(const Callback<void(StringViewBase<char16_t>)> &cb, const StringViewBase<char16_t> &val) -> const Callback<void(StringViewBase<char16_t>)> & {
	detail::streamWrite(cb, val);
	return cb;
}

inline auto operator<<(const Callback<void(StringViewUtf8)> &cb, const StringViewUtf8 &val) -> const Callback<void(StringViewUtf8)> & {
	detail::streamWrite(cb, val);
	return cb;
}

inline auto operator<<(const std::function<void(StringViewBase<char>)> &cb, const StringViewBase<char> &val) -> const std::function<void(StringViewBase<char>)> & {
	detail::streamWrite(cb, val);
	return cb;
}

inline auto operator<<(const std::function<void(StringViewBase<char16_t>)> &cb, const StringViewBase<char16_t> &val) -> const std::function<void(StringViewBase<char16_t>)> & {
	detail::streamWrite(cb, val);
	return cb;
}

inline auto operator<<(const std::function<void(StringViewUtf8)> &cb, const StringViewUtf8 &val) -> const std::function<void(StringViewUtf8)> & {
	detail::streamWrite(cb, val);
	return cb;
}

inline auto operator<<(const memory::function<void(StringViewBase<char>)> &cb, const StringViewBase<char> &val) -> const memory::function<void(StringViewBase<char>)> & {
	detail::streamWrite(cb, val);
	return cb;
}

inline auto operator<<(const memory::function<void(StringViewBase<char16_t>)> &cb, const StringViewBase<char16_t> &val) -> const memory::function<void(StringViewBase<char16_t>)> & {
	detail::streamWrite(cb, val);
	return cb;
}

inline auto operator<<(const memory::function<void(StringViewUtf8)> &cb, const StringViewUtf8 &val) -> const memory::function<void(StringViewUtf8)> & {
	detail::streamWrite(cb, val);
	return cb;
}

inline auto operator<<(const Callback<void(BytesView)> &cb, const BytesView &val) -> const Callback<void(BytesView)> & {
	cb(val);
	return cb;
}

inline auto operator<<(const Callback<void(BytesView)> &cb, const uint8_t &val) -> const Callback<void(BytesView)> & {
	cb(BytesView(&val, 1));
	return cb;
}

inline auto operator<<(const std::function<void(BytesView)> &cb, const BytesView &val) -> const std::function<void(BytesView)> & {
	cb(val);
	return cb;
}

inline auto operator<<(const std::function<void(BytesView)> &cb, const uint8_t &val) -> const std::function<void(BytesView)> & {
	cb(BytesView(&val, 1));
	return cb;
}

inline auto operator<<(const memory::function<void(BytesView)> &cb, const BytesView &val) -> const memory::function<void(BytesView)> & {
	cb(val);
	return cb;
}

inline auto operator<<(const memory::function<void(BytesView)> &cb, const uint8_t &val) -> const memory::function<void(BytesView)> & {
	cb(BytesView(&val, 1));
	return cb;
}

SP_DEFINE_STREAM_OVERLOADS(Callback<void(StringViewBase<char>)>,char)
SP_DEFINE_STREAM_OVERLOADS(Callback<void(StringViewBase<char16_t>)>,char16_t)
SP_DEFINE_STREAM_OVERLOADS(Callback<void(StringViewUtf8)>,char)

SP_DEFINE_STREAM_OVERLOADS(std::function<void(StringViewBase<char>)>,char)
SP_DEFINE_STREAM_OVERLOADS(std::function<void(StringViewBase<char16_t>)>,char16_t)
SP_DEFINE_STREAM_OVERLOADS(std::function<void(StringViewUtf8)>,char)

SP_DEFINE_STREAM_OVERLOADS(memory::function<void(StringViewBase<char>)>,char)
SP_DEFINE_STREAM_OVERLOADS(memory::function<void(StringViewBase<char16_t>)>,char16_t)
SP_DEFINE_STREAM_OVERLOADS(memory::function<void(StringViewUtf8)>,char)

#undef SP_DEFINE_STREAM_OVERLOADS

namespace string {

namespace detail {

inline size_t toStringValue(char *target, double val) {
	return string::_dtoa(val, target, string::DOUBLE_MAX_DIGITS);
}

inline size_t toStringValue(char *target, float val) {
	return string::_dtoa(val, target, string::DOUBLE_MAX_DIGITS);
}

inline size_t toStringValue(char *target, int64_t val) {
	auto len = string::_itoa_len(val);
	return string::_itoa(val, target, len);
}

inline size_t toStringValue(char *target, uint64_t val) {
	auto len = string::_itoa_len(val);
	return string::_itoa(val, target, len);
}

inline size_t toStringValue(char *target, int32_t val) {
	auto len = string::_itoa_len(int64_t(val));
	return string::_itoa(int64_t(val), target, len);
}

inline size_t toStringValue(char *target, uint32_t val) {
	auto len = string::_itoa_len(uint64_t(val));
	return string::_itoa(uint64_t(val), target, len);
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
	memcpy(target, val, len);
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
inline size_t getBufferSizeValue(const int64_t &value) { return string::_itoa_len(value); }
inline size_t getBufferSizeValue(const uint64_t &value) { return string::_itoa_len(value); }
inline size_t getBufferSizeValue(const int32_t &value) { return string::_itoa_len(int64_t(value)); }
inline size_t getBufferSizeValue(const uint32_t &value) { return string::_itoa_len(uint64_t(value)); }
inline size_t getBufferSizeValue(const int16_t &value) { return string::_itoa_len(int64_t(value)); }
inline size_t getBufferSizeValue(const uint16_t &value) { return string::_itoa_len(uint64_t(value)); }
inline size_t getBufferSizeValue(const int8_t &value) { return string::_itoa_len(int64_t(value)); }
inline size_t getBufferSizeValue(const uint8_t &value) { return string::_itoa_len(uint64_t(value)); }
inline size_t getBufferSizeValue(const double &value) { return string::_dtoa_len(value); }
inline size_t getBufferSizeValue(const float &value) { return string::_dtoa_len(value); }

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

template <typename Interface, typename... Args>
static auto toString(Args && ... args) -> typename Interface::StringType {
	if constexpr (detail::IsFastToStringAvailable<Args ...>::value) {
		// fast toString with preallocated buffer

		auto size = detail::getBufferSize(std::forward<Args>(args)...);
		typename Interface::StringType ret; ret.resize(size);

		auto s = detail::writeBuffer(ret.data(), std::forward<Args>(args)...);
		if (s != size) {
			std::cout << "Invalid buffer size for toString<fast>\n";
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

}

}

#endif /* CORE_CORE_STRING_SPSTRINGSTREAM_H_ */
