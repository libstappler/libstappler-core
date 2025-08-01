/**
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_DATA_SPDATADECODEJSON_H_
#define STAPPLER_DATA_SPDATADECODEJSON_H_

#include "SPDataValue.h"

namespace STAPPLER_VERSIONIZED stappler::data::json {

inline StringView decodeNumber(StringView &r, bool &isFloat) {
	auto tmp = r;
	if (r.is('-')) {
		++r;
	}
	r.skipChars<StringView::CharGroup<CharGroupId::Numbers>>();
	if (r.is('.')) {
		isFloat = true;
		++r;
		r.skipChars<StringView::CharGroup<CharGroupId::Numbers>>();
	}
	if (r.is('E') || r.is('e')) {
		isFloat = true;
		++r;
		if (r.is('+') || r.is('-')) {
			++r;
		}
		r.skipChars<StringView::CharGroup<CharGroupId::Numbers>>();
	}

	return StringView(tmp.data(), tmp.size() - r.size());
}

template <typename Interface>
struct Decoder : public Interface::AllocBaseType {
	using InterfaceType = Interface;
	using ValueType = ValueTemplate<Interface>;
	using StringType = typename InterfaceType::StringType;

	enum BackType {
		BackIsArray,
		BackIsDict,
		BackIsEmpty
	};

	Decoder(StringView &r, bool v) : validate(v), backType(BackIsEmpty), r(r), back(nullptr) {
		stack.reserve(10);
	}

	inline void parseBufferString(StringType &ref);
	inline void parseJsonNumber(ValueType &ref) SPINLINE;

	inline void parseValue(ValueType &current);
	void parseJson(ValueType &val);

	inline void push(BackType t, ValueType *v) {
		++r;
		back = v;
		stack.push_back(v);
		backType = t;
	}

	inline void pop() {
		r++;
		stack.pop_back();
		if (stack.empty()) {
			back = nullptr;
			backType = BackIsEmpty;
		} else {
			back = stack.back();
			backType = (back->isArray()) ? BackIsArray : BackIsDict;
		}
	}

	bool validate;
	bool stop = false;
	BackType backType;
	StringView r;
	ValueType *back;
	StringType buf;
	typename InterfaceType::template ArrayType<ValueType *> stack;
};

template <typename Interface>
inline void Decoder<Interface>::parseBufferString(StringType &ref) {
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	static const char escape[256] = {Z16, Z16, 0, 0, '\"', 0, 0, 0, 0, '\'', 0, 0, 0, 0, 0, 0, 0,
		'/', Z16, Z16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\\', 0, 0, 0, 0, 0, '\b', 0, 0, 0, '\f',
		0, 0, 0, 0, 0, 0, 0, '\n', 0, 0, 0, '\r', 0, '\t', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Z16,
		Z16, Z16, Z16, Z16, Z16, Z16, Z16};
#undef Z16
	if (r.is('"')) {
		r++;
	}
	auto s = r.readUntil<StringView::Chars<'\\', '"'>>();
	ref.assign(s.data(), s.size());
	while (!r.empty() && !r.is('"')) {
		if (r.is('\\')) {
			++r;
			if (r.is('u')) {
				++r;
				if (r >= 4) {
					unicode::utf8Encode(ref,
							char16_t(base16::hexToChar(r[0], r[1]) << 8
									| base16::hexToChar(r[2], r[3])));
					r += 4;
				} else {
					r.clear();
				}
			} else {
				ref.push_back(escape[(uint8_t)r[0]]);
				++r;
			}
		}
		auto s = r.readUntil<StringView::Chars<'\\', '"'>>();
		ref.append(s.data(), s.size());
	}
	if (r.is('"')) {
		++r;
	}
}

template <typename Interface>
inline void Decoder<Interface>::parseJsonNumber(ValueType &result) {
	bool isFloat = false;
	auto value = decodeNumber(r, isFloat);
	if (value.empty()) {
		return;
	} else {
		if (isFloat) {
			value.readDouble().unwrap([&](double v) {
				result._type = ValueType::Type::DOUBLE;
				result.doubleVal = v;
			});
		} else {
			value.readInteger().unwrap([&](int64_t v) {
				result._type = ValueType::Type::INTEGER;
				result.intVal = v;
			});
		}
	}
}

template <typename Interface>
inline void Decoder<Interface>::parseValue(ValueType &current) {
	switch (r[0]) {
	case '"':
		current._type = ValueType::Type::CHARSTRING;
		parseBufferString(buf);
		current.strVal = new (std::nothrow) StringType(sp::move(buf));
		break;
	case 't':
		current._type = ValueType::Type::BOOLEAN;
		current.boolVal = true;
		r += 4;
		break;
	case 'f':
		current._type = ValueType::Type::BOOLEAN;
		current.boolVal = false;
		r += 5;
		break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '+':
	case '-': parseJsonNumber(current); break;
	case '[':
		current._type = ValueType::Type::ARRAY;
		current.arrayVal = new (std::nothrow) typename ValueType::ArrayType();
		//current.arrayVal->reserve(10);
		push(BackIsArray, &current);
		break;
	case '{':
		current._type = ValueType::Type::DICTIONARY;
		current.dictVal = new (std::nothrow) typename ValueType::DictionaryType();
		push(BackIsDict, &current);
		break;
	case 'n':
		if (r.is("nan")) {
			current._type = ValueType::Type::DOUBLE;
			current.doubleVal = nan();
			r += 3;
		} else {
			r += 4;
		}
		break;
	case ']':
	case '}':
	case ':':
	case ',':
		log::error("json::Decoder", "Invalid token: ", r.sub(0, 1), "; expected value");
		r.skipUntil<StringView::Chars<'"', 't', 'f', 'n', '+', '-', '[', '{', ']', '}'>,
				StringView::Range<'0', '9'>>();
		++r;
		break;
	default:
		r.skipUntil<StringView::Chars<'"', 't', 'f', 'n', '+', '-', '[', '{', ']', '}'>,
				StringView::Range<'0', '9'>>();
		break;
	}
}

template <typename Interface>
void Decoder<Interface>::parseJson(ValueType &val) {
	do {
		switch (backType) {
		case BackIsArray:
			r.skipChars<StringView::Chars<' ', '\n', '\r', '\t', ','>>();
			if (!r.is(']')) {
				back->arrayVal->emplace_back(ValueType::Type::EMPTY);
				parseValue(back->arrayVal->back());
			} else {
				back->arrayVal->shrink_to_fit();
				pop();
			}
			break;
		case BackIsDict:
			r.skipUntil<StringView::Chars<'"', '}'>>();
			if (!r.is('}')) {
				parseBufferString(buf);
				if (validate) {
					auto tmp = r.readChars<StringView::Chars<':', ' ', '\n', '\r', '\t'>>();
					tmp.template skipUntil<StringView::Chars<':'>>();
					if (!tmp.is(':')) {
						stop = true;
						return;
					}
				} else {
					r.skipChars<StringView::Chars<':', ' ', '\n', '\r', '\t'>>();
				}
				parseValue(back->dictVal->emplace(sp::move(buf), ValueType::Type::EMPTY)
								.first->second);
			} else {
				pop();
			}
			break;
		case BackIsEmpty: parseValue(val); break;
		}
	} while (!r.empty() && !stack.empty() && !stop);
}

template <typename Interface>
auto read(StringView &n, bool validate = false) -> ValueTemplate<Interface> {
	auto r = n;
	if (r.empty() || r == "null") {
		return ValueTemplate<Interface>();
	}

	r.skipChars<StringView::Chars<' ', '\n', '\r', '\t'>>();
	Decoder<Interface> dec(r, validate);
	ValueTemplate<Interface> ret;
	dec.parseJson(ret);
	n = dec.r;
	return ret;
}

template <typename Interface>
auto read(const StringView &r) -> ValueTemplate<Interface> {
	StringView tmp(r);
	return read<Interface>(tmp);
}

} // namespace stappler::data::json

#endif /* STAPPLER_DATA_SPDATADECODEJSON_H_ */
