/**
Copyright (c) 2020-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_CORE_MEMORY_SPMEMINTERFACE_H_
#define STAPPLER_CORE_MEMORY_SPMEMINTERFACE_H_

#include "SPMemDict.h"
#include "SPMemFunction.h"
#include "SPMemMap.h"
#include "SPMemSet.h"
#include "SPMemString.h"
#include "SPMemStringStream.h"
#include "SPMemVector.h"

namespace STAPPLER_VERSIONIZED stappler::memory {

struct PoolInterface : public memory::AllocPool {
	using AllocBaseType = memory::AllocPool;
	using StringType = memory::string;
	using WideStringType = memory::u16string;
	using BytesType = memory::vector<uint8_t>;

	template <typename Value> using BasicStringType = memory::basic_string<Value>;
	template <typename Value> using ArrayType = memory::vector<Value>;
	template <typename Value> using DictionaryType = memory::map<StringType, Value, std::less<>>;
	template <typename Value> using VectorType = memory::vector<Value>;

	template <typename K, typename V, typename Compare = std::less<>>
	using MapType = memory::map<K, V, Compare>;

	template <typename T, typename Compare = std::less<>>
	using SetType = memory::set<T, Compare>;

	template <typename T>
	using FunctionType = memory::function<T>;

	using StringStreamType = memory::ostringstream;

	static constexpr bool usesMemoryPool() { return true; }
};

struct StandartInterface : public memory::AllocBase {
	using AllocBaseType = memory::AllocBase;
	using StringType = std::string;
	using WideStringType = std::u16string;
	using BytesType = std::vector<uint8_t>;

	template <typename Value> using BasicStringType = std::basic_string<Value>;
	template <typename Value> using ArrayType = std::vector<Value>;
	template <typename Value> using DictionaryType = std::map<StringType, Value, std::less<>>;
	template <typename Value> using VectorType = std::vector<Value>;

	template <typename K, typename V, typename Compare = std::less<>>
	using MapType = std::map<K, V, Compare>;

	template <typename T, typename Compare = std::less<>>
	using SetType = std::set<T, Compare>;

	template <typename T>
	using FunctionType = std::function<T>;

	using StringStreamType = std::ostringstream;

	static constexpr bool usesMemoryPool() { return false; }
};

}

namespace STAPPLER_VERSIONIZED stappler {

template <typename InterfaceType>
struct InterfaceObject {
	using Interface = InterfaceType;

	using String = typename Interface::StringType;
	using WideString = typename Interface::WideStringType;
	using Bytes = typename Interface::BytesType;

	template <typename Value>
	using BasicString = typename Interface::template BasicStringType<Value>;

	template <typename Value>
	using Vector = typename Interface::template VectorType<Value>;

	template <typename K, typename V, typename Compare = std::less<>>
	using Map = typename Interface::template MapType<K, V, Compare>;

	template <typename T, typename Compare = std::less<>>
	using Set = typename Interface::template SetType<T, Compare>;

	template <typename T>
	using Function = typename Interface::template FunctionType<T>;

	using StringStream = typename Interface::StringStreamType;
};

}

namespace STAPPLER_VERSIONIZED stappler::traits {

template <typename StringType>
struct SelectStringStream;

template <>
struct SelectStringStream<std::string> {
	using Type = std::ostringstream;
};

template <>
struct SelectStringStream<std::u16string> {
	using Type = std::basic_ostringstream<char16_t>;
};

template <>
struct SelectStringStream<memory::string> {
	using Type = memory::ostringstream;
};

template <>
struct SelectStringStream<memory::basic_string<char16_t>> {
	using Type = memory::basic_ostringstream<char16_t>;
};


}

namespace STAPPLER_VERSIONIZED stappler {

template <typename T>
using Callback = memory::callback<T>;

template <typename T> auto StringToNumber(const memory::StandartInterface::StringType &str) -> T {
	return StringToNumber<T>(str.data(), nullptr, 0);
}

template <typename T> auto StringToNumber(const memory::PoolInterface::StringType &str) -> T {
	return StringToNumber<T>(str.data(), nullptr, 0);
}

template <typename T> auto StringToNumber(const char *str) -> T {
	return StringToNumber<T>(str, nullptr, 0);
}

}

#endif /* STAPPLER_CORE_MEMORY_SPMEMINTERFACE_H_ */
