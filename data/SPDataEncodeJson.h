/**
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_DATA_SPDATAENCODEJSON_H_
#define STAPPLER_DATA_SPDATAENCODEJSON_H_

#include "SPDataValue.h"
#include "SPFilesystem.h"

namespace STAPPLER_VERSIONIZED stappler::data::json {

template <typename StringType>
inline void encodeString(std::ostream &stream, const StringType &str) {
	stream << '"';
	for (auto &i : str) {
		switch (i) {
		case '\n' : stream << "\\n"; break;
		case '\r' : stream << "\\r"; break;
		case '\t' : stream << "\\t"; break;
		case '\f' : stream << "\\f"; break;
		case '\b' : stream << "\\b"; break;
		case '\\' : stream << "\\\\"; break;
		case '\"' : stream << "\\\""; break;
		case ' ' : stream << " "; break;
		default:
			if (i >= 0 && i <= 0x20) {
				stream << "\\u" << std::setfill('0') << std::setw(4)
					<< std::hex << (int32_t)i << std::dec << std::setw(1) << std::setfill(' ');
			} else {
				stream << i;
			}
			break;
		}
	}
	stream << '"';
}

template <typename Interface>
struct RawEncoder : public Interface::AllocBaseType {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;

	inline RawEncoder(std::ostream *s) : stream(s) { }

	inline void writeData(const char *data, size_t size) {
		stream->write(data, size);
	}

	inline void writeData(const char *data) {
		writeData(data, strlen(data));
	}

	inline void writeChar(char c) {
		stream->put(c);
	}

	inline void write(nullptr_t) { writeData("null", "null"_len); }
	inline void write(bool value) { writeData((value)?"true":"false"); }
	inline void write(int64_t value) { (*stream) << value; }
	inline void write(double value) {
		// default stream output strips '.0' at the end, that broke CBOR compatibility
		std::array<char, string::DOUBLE_MAX_DIGITS> buf = { 0 };
		stream->write(buf.data(), string::_dtoa(value, buf.data(), buf.size()));
	}

	inline void write(const typename ValueType::StringType &str) {
		encodeString(*stream, str);
	}

	inline void write(const typename ValueType::BytesType &data) {
		(*stream) << '"' << "BASE64:";
		base64url::encode(*stream, data);
		(*stream) << '"';
	}
	inline void onBeginArray(const typename ValueType::ArrayType &arr) { (*stream) << '['; }
	inline void onEndArray(const typename ValueType::ArrayType &arr) { (*stream) << ']'; }
	inline void onBeginDict(const typename ValueType::DictionaryType &dict) { (*stream) << '{'; }
	inline void onEndDict(const typename ValueType::DictionaryType &dict) { (*stream) << '}'; }
	inline void onKey(const typename ValueType::StringType &str) { write(str); (*stream) << ':'; }
	inline void onNextValue() { (*stream) << ','; }

	std::ostream *stream = nullptr;
};

template <typename Interface>
struct PrettyEncoder : public Interface::AllocBaseType {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;

	PrettyEncoder(std::ostream *s, bool tM = false) : timeMarkers(tM), stream(s) { }

	void write(nullptr_t) { (*stream) << "null"; offsetted = false; }
	void write(bool value) { (*stream) << ((value)?"true":"false"); offsetted = false; }
	void write(int64_t value) {
		(*stream) << value; offsetted = false;
		if (timeMarkers
			&& (lastKey.find("time") != maxOf<size_t>()
					|| lastKey.find("Time") != maxOf<size_t>()
					|| lastKey.find("TIME") != maxOf<size_t>()
					|| lastKey.find("date") != maxOf<size_t>()
					|| lastKey.find("Date") != maxOf<size_t>())
			&& (value > 1000000000000000 && value < 10000000000000000)) {
			(*stream) << " /* " << Time::microseconds(value).toHttp<Interface>() << " */";
		}
	}
	void write(double value) {
		// default stream output strips '.0' at the end, that broke CBOR compatibility
		std::array<char, string::DOUBLE_MAX_DIGITS> buf = { 0 };
		stream->write(buf.data(), string::_dtoa(value, buf.data(), buf.size()));
		offsetted = false;
	}

	void write(const typename ValueType::StringType &str) {
		encodeString(*stream, str);
		offsetted = false;
	}

	void write(const typename ValueType::BytesType &data) {
		(*stream) << '"'  << "BASE64:";
		base64url::encode(*stream, data);
		(*stream) << '"';
		offsetted = false;
	}

	bool isObjectArray(const typename ValueType::ArrayType &arr) {
		for (auto &it : arr) {
			if (!it.isDictionary()) {
				return false;
			}
		}
		return true;
	}

	void onBeginArray(const typename ValueType::ArrayType &arr) {
		(*stream) << '[';
		if (!isObjectArray(arr)) {
			++ depth;
			bstack.push_back(false);
			offsetted = false;
		} else {
			bstack.push_back(true);
		}
	}

	void onEndArray(const typename ValueType::ArrayType &arr) {
		if (!bstack.empty()) {
			if (!bstack.back()) {
				-- depth;
				(*stream) << '\n';
				for (size_t i = 0; i < depth; i++) {
					(*stream) << '\t';
				}
			}
			bstack.pop_back();
		} else {
			-- depth;
			(*stream) << '\n';
			for (size_t i = 0; i < depth; i++) {
				(*stream) << '\t';
			}
		}
		(*stream) << ']';
		popComplex = true;
	}

	void onBeginDict(const typename ValueType::DictionaryType &dict) {
		lastKey = StringView();
		(*stream) << '{';
		++ depth;
	}

	void onEndDict(const typename ValueType::DictionaryType &dict) {
		lastKey = StringView();
		-- depth;
		(*stream) << '\n';
		for (size_t i = 0; i < depth; i++) {
			(*stream) << '\t';
		}
		(*stream) << '}';
		popComplex = true;
	}

	void onKey(const typename ValueType::StringType &str) {
		lastKey = str;
		(*stream) << '\n';
		for (size_t i = 0; i < depth; i++) {
			(*stream) << '\t';
		}
		write(str);
		offsetted = true;
		(*stream) << ':' << ' ';
	}

	void onNextValue() {
		lastKey = StringView();
		(*stream) << ',';
	}

	void onValue(const ValueType &val) {
		if (depth > 0) {
			if (popComplex && (val.isArray() || val.isDictionary())) {
				(*stream) << ' ';
			} else {
				if (!offsetted) {
					(*stream) << '\n';
					for (size_t i = 0; i < depth; i++) {
						(*stream) << '\t';
					}
					offsetted = true;
				}
			}
			popComplex = false;
		}
	}

	size_t depth = 0;
	bool popComplex = false;
	bool offsetted = false;
	bool timeMarkers = false;
	std::ostream *stream;
	StringView lastKey;
	typename Interface::template ArrayType<bool> bstack;
};

template <typename Interface>
inline void write(std::ostream &stream, const ValueTemplate<Interface> &val, bool pretty, bool timeMarkers = false) {
	if (pretty) {
		PrettyEncoder<Interface> encoder(&stream, timeMarkers);
		val.encode(encoder);
	} else {
		RawEncoder<Interface> encoder(&stream);
		val.encode(encoder);
	}
}

template <typename Interface>
inline auto write(const ValueTemplate<Interface> &val, bool pretty = false, bool timeMarkers = false) -> typename Interface::StringType {
	typename Interface::StringStreamType stream;
	write<Interface>(stream, val, pretty, timeMarkers);
	return stream.str();
}

template <typename Interface>
bool save(const ValueTemplate<Interface> &val, StringView ipath, bool pretty, bool timeMarkers = false) {
	auto path = filesystem::native::posixToNative<Interface>(ipath);
	std::ofstream stream(path.data());
	if (stream.is_open()) {
		write(stream, val, pretty, timeMarkers);
		stream.flush();
		stream.close();
		return true;
	}
	return false;
}

template <typename Interface>
auto toString(const ValueTemplate<Interface> &data, bool pretty) -> typename Interface::StringType {
	return write(data, pretty);
}

}

#endif /* STAPPLER_DATA_SPDATAENCODEJSON_H_ */
