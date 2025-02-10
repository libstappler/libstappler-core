/**
Copyright (c) 2019-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_CORE_SPMEMORY_H_
#define STAPPLER_CORE_SPMEMORY_H_

#include "SPMemInterface.h"
#include "SPSpanView.h"
#include "SPString.h"
#include "SPStringView.h"
#include "SPTime.h"
#include "SPRef.h"

#ifdef MODULE_STAPPLER_DATA
#include "SPData.h"
#endif

namespace STAPPLER_VERSIONIZED stappler {

/** VectorAdapter<Type> - унифицированный адаптер для доступа к типу Vector
 * вне зависимости от интерфейса памяти
 *
 * Адаптер захватывает Vector<Type> и использует его для чтения и записи
 */

template <typename T>
class SP_PUBLIC VectorAdapter {
public:
	size_t size() const { return size_fn(target); }
	T & back() const { return back_fn(target); }
	T & front() const { return front_fn(target); }
	bool empty() const { return empty_fn(target); }
	T & at(size_t pos) const { return at_fn(target, pos); }
	T & emplace_back(T &&v) const { return emplace_back_fn(target, move(v)); }

	T *begin() const { return begin_fn(target); }
	T *end() const { return end_fn(target); }

	void clear() const { clear_fn(target); }
	void reserve(size_t count) const { reserve_fn(target, count); }
	void resize(size_t count) const { resize_fn(target, count); }

	explicit operator bool () const noexcept { return target != nullptr; }

	VectorAdapter() noexcept = default;

	VectorAdapter(memory::StandartInterface::VectorType<T> &vec) noexcept;
	VectorAdapter(memory::PoolInterface::VectorType<T> &vec) noexcept;

public:
	void *target = nullptr;
	size_t (*size_fn) (void *) = nullptr;
	T & (*back_fn) (void *) = nullptr;
	T & (*front_fn) (void *) = nullptr;
	bool (*empty_fn) (void *) = nullptr;
	T & (*at_fn) (void *, size_t) = nullptr;
	T & (*emplace_back_fn) (void *, T&&) = nullptr;
	T * (*begin_fn) (void *) = nullptr;
	T * (*end_fn) (void *) = nullptr;
	void (*clear_fn) (void *) = nullptr;
	void (*reserve_fn) (void *, size_t) = nullptr;
	void (*resize_fn) (void *, size_t) = nullptr;
};

}

namespace STAPPLER_VERSIONIZED stappler::mem_pool {

namespace pool = memory::pool;
namespace allocator = memory::allocator;

using CharGroupId = stappler::CharGroupId;

using memory::allocator_t;
using memory::pool_t;

using stappler::Time;
using stappler::TimeInterval;

using stappler::StringView;
using stappler::StringViewUtf8;
using stappler::WideStringView;
using stappler::BytesView;
using stappler::SpanView;

using AllocBase = stappler::memory::AllocPool;

using String = stappler::memory::string;
using WideString = stappler::memory::u16string;
using Bytes = stappler::memory::vector<uint8_t>;

using StringStream = stappler::memory::ostringstream;
using OutputStream = std::ostream;

template <typename T>
using Vector = stappler::memory::vector<T>;

template <typename K, typename V, typename Compare = std::less<void>>
using Map = stappler::memory::map<K, V, Compare>;

template <typename T, typename Compare = std::less<void>>
using Set = stappler::memory::set<T, Compare>;

template <typename T>
using Function = stappler::memory::function<T>;

using stappler::Callback;

using stappler::Pair;

template <typename T, typename V, typename Compare = std::less<void>>
using dict = stappler::memory::dict<T, V, Compare>;

using Mutex = std::mutex;

using stappler::makeSpanView;

using memory::pool::perform;
using memory::pool::perform_clear;
using memory::pool::perform_temporary;

template <typename Container, typename T>
bool emplace_ordered(Container &vec, T val);

template <typename Container, typename T>
bool exists_ordered(const Container &vec, const T & val);

}


namespace STAPPLER_VERSIONIZED stappler::mem_std {

namespace pool = memory::pool;
namespace allocator = memory::allocator;

using memory::allocator_t;
using memory::pool_t;

using stappler::Time;
using stappler::TimeInterval;

using stappler::StringView;
using stappler::StringViewUtf8;
using stappler::WideStringView;
using stappler::BytesView;
using stappler::SpanView;

using AllocBase = stappler::memory::StandartInterface::AllocBaseType;

using String = std::string;
using WideString = std::u16string;
using Bytes = std::vector<uint8_t>;

using StringStream = std::stringstream;
using OutputStream = std::ostream;

template <typename T>
using Vector = std::vector<T>;

template <typename K, typename V, typename Compare = std::less<void>>
using Map = std::map<K, V, Compare>;

template <typename T, typename Compare = std::less<void>>
using Set = std::set<T, Compare>;

template <typename T, typename V>
using HashMap = std::unordered_map<T, V, std::hash<T>, std::equal_to<T>>;

template <typename T>
using HashSet = std::unordered_set<T, std::hash<T>, std::equal_to<T>, std::allocator<T>>;

template <typename T>
using Function = std::function<T>;

using stappler::Callback;

using stappler::Pair;

using Mutex = std::mutex;

using stappler::makeSpanView;

using memory::pool::perform;
using memory::pool::perform_clear;
using memory::pool::perform_temporary;

template <typename Container, typename T>
bool emplace_ordered(Container &vec, T val);

template <typename Container, typename T>
bool exists_ordered(const Container &vec, const T & val);

}


//
// MODULE_STAPPLER_DATA extension
//

#ifdef MODULE_STAPPLER_DATA

namespace STAPPLER_VERSIONIZED stappler::mem_pool {

using Value = stappler::data::ValueTemplate<stappler::memory::PoolInterface>;
using Array = Value::ArrayType;
using Dictionary = Value::DictionaryType;
using EncodeFormat = stappler::data::EncodeFormat;

inline bool emplace_ordered(Vector<Value> &vec, const Value &val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val, [&] (const Value &l, const Value &r) {
		return l.getInteger() < r.getInteger();
	});
	if (lb == vec.end()) {
		vec.emplace_back(val);
		return true;
	} else if (*lb != val) {
		vec.emplace(lb, val);
		return true;
	}
	return false;
}

}


namespace STAPPLER_VERSIONIZED stappler::mem_std {

using Value = data::ValueTemplate<stappler::memory::StandartInterface>;
using Array = Value::ArrayType;
using Dictionary = Value::DictionaryType;
using EncodeFormat = stappler::data::EncodeFormat;

inline bool emplace_ordered(Vector<Value> &vec, const Value &val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val, [&] (const Value &l, const Value &r) {
		return l.getInteger() < r.getInteger();
	});
	if (lb == vec.end()) {
		vec.emplace_back(val);
		return true;
	} else if (*lb != val) {
		vec.emplace(lb, val);
		return true;
	}
	return false;
}

}

#endif // MODULE_STAPPLER_DATA


//
// Implementation details
//

namespace STAPPLER_VERSIONIZED stappler::mem_pool {

template <typename Container, typename T>
inline bool emplace_ordered(Container &vec, T val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val);
	if (lb == vec.end()) {
		vec.emplace_back(val);
		return true;
	} else if (*lb != val) {
		vec.emplace(lb, val);
		return true;
	}
	return false;
}

template <typename Container, typename T>
inline bool exists_ordered(const Container &vec, const T & val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val);
	if (lb == vec.end() || *lb != val) {
		return false;
	}
	return true;
}

}


namespace STAPPLER_VERSIONIZED stappler::mem_std {

template <typename Container, typename T>
inline bool emplace_ordered(Container &vec, T val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val);
	if (lb == vec.end()) {
		vec.emplace_back(val);
		return true;
	} else if (*lb != val) {
		vec.emplace(lb, val);
		return true;
	}
	return false;
}

template <typename Container, typename T>
inline bool exists_ordered(const Container &vec, const T & val) {
	auto lb = std::lower_bound(vec.begin(), vec.end(), val);
	if (lb == vec.end() || *lb != val) {
		return false;
	}
	return true;
}

}

namespace STAPPLER_VERSIONIZED stappler {

template <typename T>
VectorAdapter<T>::VectorAdapter(memory::StandartInterface::VectorType<T> &vec) noexcept
: target(&vec), size_fn([] (void *target) {
	return ((mem_std::Vector<T> *)target)->size();
}), back_fn([] (void *target) -> T & {
	return ((mem_std::Vector<T> *)target)->back();
}), front_fn([] (void *target) -> T & {
	return ((mem_std::Vector<T> *)target)->front();
}), empty_fn([] (void *target) {
	return ((mem_std::Vector<T> *)target)->empty();
}), at_fn([] (void *target, size_t pos) -> T & {
	return ((mem_std::Vector<T> *)target)->at(pos);
}), emplace_back_fn([] (void *target, T &&v) -> T & {
	return ((mem_std::Vector<T> *)target)->emplace_back(move(v));
}), begin_fn([] (void *target) -> T * {
	return &*((mem_std::Vector<T> *)target)->begin();
}), end_fn([] (void *target) -> T * {
	return &*((mem_std::Vector<T> *)target)->end();
}), clear_fn([] (void *target) {
	((mem_std::Vector<T> *)target)->clear();
}), reserve_fn([] (void *target, size_t s) {
	((mem_std::Vector<T> *)target)->reserve(s);
}), resize_fn([] (void *target, size_t s) {
	((mem_std::Vector<T> *)target)->resize(s);
}) { }

template <typename T>
VectorAdapter<T>::VectorAdapter(memory::PoolInterface::VectorType<T> &vec) noexcept
: target(&vec), size_fn([] (void *target) {
	return ((mem_pool::Vector<T> *)target)->size();
}), back_fn([] (void *target) -> T & {
	return ((mem_pool::Vector<T> *)target)->back();
}), front_fn([] (void *target) -> T & {
	return ((mem_pool::Vector<T> *)target)->front();
}), empty_fn([] (void *target) {
	return ((mem_pool::Vector<T> *)target)->empty();
}), at_fn([] (void *target, size_t pos) -> T & {
	return ((mem_pool::Vector<T> *)target)->at(pos);
}), emplace_back_fn([] (void *target, T &&v) -> T & {
	return ((mem_pool::Vector<T> *)target)->emplace_back(move(v));
}), begin_fn([] (void *target) -> T * {
	return &*((mem_pool::Vector<T> *)target)->begin();
}), end_fn([] (void *target) -> T * {
	return &*((mem_pool::Vector<T> *)target)->end();
}), clear_fn([] (void *target) {
	((mem_pool::Vector<T> *)target)->clear();
}), reserve_fn([] (void *target, size_t s) {
	((mem_pool::Vector<T> *)target)->reserve(s);
}), resize_fn([] (void *target, size_t s) {
	((mem_pool::Vector<T> *)target)->resize(s);
}) { }

}

#endif /* STAPPLER_CORE_SPMEMORY_H_ */
