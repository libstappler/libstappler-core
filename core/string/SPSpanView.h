/**
Copyright (c) 2020-2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023-2025 Stappler LLC <admin@stappler.dev>
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

#ifndef STAPPLER_CORE_STRING_SPSPANVIEW_H_
#define STAPPLER_CORE_STRING_SPSPANVIEW_H_

#include "SPMemInterface.h"
#include "SPBytesView.h"

namespace STAPPLER_VERSIONIZED stappler {

template <typename _Type>
class SpanView {
public:
	using Type = _Type;
	using Self = SpanView<Type>;
	using iterator = memory::pointer_iterator<const Type, const Type *, const Type &>;
	using reverse_iterator = std::reverse_iterator<iterator>;

	constexpr SpanView() = default;
	constexpr SpanView(const Type *p, size_t l) : ptr(p), len(l) { }
	constexpr SpanView(const Type *begin, const Type *end) : ptr(begin), len(end - begin) { }

	static Self alloc(memory::pool_t *pool, size_t count) {
		auto mem = (Type *)memory::pool::palloc(pool, count * sizeof(Type));
		return Self(mem, count);
	}

	template <size_t N>
	SpanView(const _Type value[N]) : ptr(&value[0]), len(N) { }

	template < typename InputIt,
			typename = std::enable_if_t<std::is_convertible<_Type *, InputIt>::value> >
	SpanView(InputIt first, InputIt last) : ptr(&(*first)), len(std::distance(first, last)) { }

	SpanView(InitializerList<Type> il) : ptr(il.begin()), len(il.size()) { }

	SpanView(const std::vector<Type> &vec) : ptr(vec.data()), len(vec.size()) { }
	SpanView(const std::vector<Type> &vec, size_t count)
	: ptr(vec.data()), len(std::min(vec.size(), count)) { }
	SpanView(const std::vector<Type> &vec, size_t off, size_t count)
	: ptr(vec.data() + off), len(std::min(vec.size() - off, count)) { }

	SpanView(const memory::vector<Type> &vec) : ptr(vec.data()), len(vec.size()) { }
	SpanView(const memory::vector<Type> &vec, size_t count)
	: ptr(vec.data()), len(std::min(vec.size(), count)) { }
	SpanView(const memory::vector<Type> &vec, size_t off, size_t count)
	: ptr(vec.data() + off), len(std::min(vec.size() - off, count)) { }

	template <size_t Size>
	SpanView(const Type (&array)[Size]) : ptr(&array[0]), len(Size) { }

	template <size_t Size>
	SpanView(const std::array<Type, Size> &arr) : ptr(arr.data()), len(arr.size()) { }

	SpanView(const Self &v) = default;
	SpanView(const Self &v, size_t len) : ptr(v.data()), len(std::min(len, v.size())) { }
	SpanView(const Self &v, size_t pos, size_t len)
	: ptr(v.data() + pos), len(std::min(len, v.size() - pos)) { }

	Self &operator=(const memory::vector<Type> &vec) {
		ptr = vec.data();
		len = vec.size();
		return *this;
	}
	Self &operator=(const std::vector<Type> &vec) {
		ptr = vec.data();
		len = vec.size();
		return *this;
	}

	template <size_t Size>
	Self &operator=(const std::array<Type, Size> &arr) {
		ptr = arr.data();
		len = arr.size();
		return *this;
	}
	constexpr Self &operator=(const Self &v) = default;

	Self &set(const Type *p, size_t l) {
		ptr = p;
		len = l;
		return *this;
	}

	void offset(size_t l) {
		if (l > len) {
			len = 0;
		} else {
			ptr += l;
			len -= l;
		}
	}

	const Type *data() const { return ptr; }
	size_t size() const { return len; }

	iterator begin() const noexcept { return iterator(ptr); }
	iterator end() const noexcept { return iterator(ptr + len); }

	reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }
	reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

	bool operator>(const size_t &val) const { return len > val; }
	bool operator>=(const size_t &val) const { return len >= val; }
	bool operator<(const size_t &val) const { return len < val; }
	bool operator<=(const size_t &val) const { return len <= val; }

	Self &operator++() {
		if (len > 0) {
			ptr++;
			len--;
		}
		return *this;
	}
	Self operator++(int) {
		auto tmp = *this;
		if (len > 0) {
			ptr++;
			len--;
		}
		return tmp;
	}
	Self &operator+=(size_t l) {
		if (len > 0) {
			offset(l);
		}
		return *this;
	}

	bool operator==(const Self &other) const {
		return len == other.size() && std::equal(begin(), end(), other.begin());
	}
	bool operator!=(const Self &other) const {
		return len != other.size() || !std::equal(begin(), end(), other.begin());
	}

	const Type &front() const { return *ptr; }
	const Type &back() const { return ptr[len - 1]; }

	const Type &at(const size_t &s) const { return ptr[s]; }
	const Type &operator[](const size_t &s) const { return ptr[s]; }
	const Type &operator*() const { return *ptr; }

	void clear() { len = 0; }
	bool empty() const { return len == 0 || !ptr; }

	Self first(size_t count) const { return Self(ptr, std::min(count, len)); }
	Self last(size_t count) const {
		return (count < len) ? Self(ptr + len - count, count) : Self(ptr, len);
	}

	Self pop_front(size_t count = 1) {
		auto ret = first(count);
		offset(count);
		return ret;
	}
	Self pop_back(size_t count = 1) {
		auto ret = last(count);
		len -= ret.size();
		return ret;
	}

	template <typename Interface>
	auto vec() const -> typename Interface::template VectorType<Type> {
		return typename Interface::template VectorType<Type>(ptr, ptr + len);
	}

	BytesView bytes() const { return BytesView((uint8_t *)ptr, len * sizeof(Type)); }

	Self pdup(memory::pool_t *p = nullptr) const {
		if (!p) {
			p = memory::pool::acquire();
		}
		auto buf = (Type *)memory::pool::palloc(p, this->size() * sizeof(Type));
		memcpy((void *)buf, this->data(), this->size() * sizeof(Type));
		return Self(buf, this->size());
	}

	size_t hash() const {
		if constexpr (sizeof(size_t) == 4) {
			return hash::hash32((const char *)data(), size() * sizeof(_Type));
		} else {
			return hash::hash64((const char *)data(), size() * sizeof(_Type));
		}
	}

	Self sub(size_t pos = 0, size_t len = maxOf<size_t>()) const { return Self(*this, pos, len); }

protected:
	const Type *ptr = nullptr;
	size_t len = 0;
};

template <typename _Tp>
inline bool operator<(const SpanView<_Tp> &__x, const SpanView<_Tp> &__y) {
	return std::lexicographical_compare(__x.begin(), __x.end(), __y.begin(), __y.end());
}

/// Based on operator<
template <typename _Tp>
inline bool operator>(const SpanView<_Tp> &__x, const SpanView<_Tp> &__y) {
	return __y < __x;
}

/// Based on operator<
template <typename _Tp>
inline bool operator<=(const SpanView<_Tp> &__x, const SpanView<_Tp> &__y) {
	return !(__y < __x);
}

/// Based on operator<
template <typename _Tp>
inline bool operator>=(const SpanView<_Tp> &__x, const SpanView<_Tp> &__y) {
	return !(__x < __y);
}

template <typename Type>
auto makeSpanView(const std::vector<Type> &vec) -> SpanView<Type> {
	return SpanView<Type>(vec);
}

template <typename Type>
auto makeSpanView(const memory::vector<Type> &vec) -> SpanView<Type> {
	return SpanView<Type>(vec);
}

template <typename Type, size_t Size>
auto makeSpanView(const std::array<Type, Size> &vec) -> SpanView<Type> {
	return SpanView<Type>(vec);
}

template <typename Type>
auto makeSpanView(const Type *ptr, size_t size) -> SpanView<Type> {
	return SpanView<Type>(ptr, size);
}

template <typename Type, size_t Size>
auto makeSpanView(const Type (&array)[Size]) -> SpanView<Type> {
	return SpanView<Type>(array, Size);
}

} // namespace STAPPLER_VERSIONIZED stappler

namespace std {

template <typename Value>
struct hash<STAPPLER_VERSIONIZED_NAMESPACE::SpanView<Value>> {
	size_t operator()(const STAPPLER_VERSIONIZED_NAMESPACE::SpanView<Value> &value) {
		return value.hash();
	}
};

} // namespace std

#endif /* STAPPLER_CORE_STRING_SPSPANVIEW_H_ */
