/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_CORE_STRING_SPDATAREADER_H_
#define STAPPLER_CORE_STRING_SPDATAREADER_H_

#include "SPByteOrder.h"
#include "SPStringView.h"
#include "SPHalfFloat.h"

namespace STAPPLER_VERSIONIZED stappler {

template <Endian Endianess = Endian::Network>
class BytesViewTemplate : public BytesReader<uint8_t> {
public:
	template <class T>
	using Converter = byteorder::ConverterTraits<Endianess, T>;

	using Self = BytesViewTemplate<Endianess>;

	using PoolBytes = typename memory::PoolInterface::BytesType;
	using StdBytes = typename memory::StandartInterface::BytesType;

	constexpr BytesViewTemplate();
	constexpr BytesViewTemplate(const uint8_t *p, size_t l);
	constexpr BytesViewTemplate(StringView);

	BytesViewTemplate(const PoolBytes &vec);
	BytesViewTemplate(const StdBytes &vec);

	template <size_t Size>
	constexpr BytesViewTemplate(const std::array<uint8_t, Size> &arr);

	template<Endian OtherEndianess>
	constexpr BytesViewTemplate(const BytesViewTemplate<OtherEndianess> &vec);

	template<Endian OtherEndianess>
	constexpr BytesViewTemplate(const BytesViewTemplate<OtherEndianess>, size_t len);

	template<Endian OtherEndianess>
	constexpr BytesViewTemplate(const BytesViewTemplate<OtherEndianess>, size_t pos, size_t len);

	Self & operator =(const PoolBytes &b);
	Self & operator =(const StdBytes &b);
	Self & operator =(const Self &b);

	Self & set(const PoolBytes &vec);
	Self & set(const StdBytes &vec);
	Self & set(const Self &vec);
	Self & set(const uint8_t *p, size_t l);

	Self & operator ++ ();
	Self & operator ++ (int);
	Self & operator += (size_t l);

	bool operator == (const Self &other) const;
	bool operator != (const Self &other) const;

	Self pdup(memory::pool_t * = nullptr) const;

	template <typename Interface>
	auto bytes() const -> typename Interface::BytesType;

	constexpr Self sub(size_t pos = 0, size_t len = maxOf<size_t>()) const { return Self(*this, pos, len); }

private:
	template <typename T>
	auto convert(const uint8_t *data) -> T;

public:
	uint64_t readUnsigned64();
	uint32_t readUnsigned32();
	uint32_t readUnsigned24();
	uint16_t readUnsigned16();
	uint8_t readUnsigned();

	double readFloat64();
	float readFloat32();
	float readFloat16();

	StringView readString(); // read null-terminated string
	StringView readString(size_t s); // read fixed-size string

	StringView toStringView() const;

	template<Endian OtherEndianess = Endianess>
	auto readBytes(size_t s) -> BytesViewTemplate<OtherEndianess>; // read fixed-size string
};

using BytesView = BytesViewTemplate<Endian::Host>;
using BytesViewNetwork = BytesViewTemplate<Endian::Network>;
using BytesViewHost = BytesViewTemplate<Endian::Host>;

template <Endian Endianess>
inline constexpr BytesViewTemplate<Endianess>::BytesViewTemplate() { }

template <Endian Endianess>
inline constexpr BytesViewTemplate<Endianess>::BytesViewTemplate(const uint8_t *p, size_t l)
: BytesReader(p, l) { }

template <Endian Endianess>
BytesViewTemplate<Endianess>::BytesViewTemplate(const PoolBytes &vec)
: BytesReader(vec.data(), vec.size()) { }

template <Endian Endianess>
BytesViewTemplate<Endianess>::BytesViewTemplate(const StdBytes &vec)
: BytesReader(vec.data(), vec.size()) { }

template <Endian Endianess>
inline constexpr BytesViewTemplate<Endianess>::BytesViewTemplate(StringView str)
: BytesReader((const uint8_t *)str.data(), str.size()) { }

template <Endian Endianess>
template <size_t Size>
inline constexpr BytesViewTemplate<Endianess>::BytesViewTemplate(const std::array<uint8_t, Size> &arr)
: BytesReader(arr.data(), Size) { }

template <Endian Endianess>
template<Endian OtherEndianess>
inline constexpr BytesViewTemplate<Endianess>::BytesViewTemplate(const BytesViewTemplate<OtherEndianess> &data)
: BytesReader(data.data(), data.size()) { }

template <Endian Endianess>
template<Endian OtherEndianess>
inline constexpr BytesViewTemplate<Endianess>::BytesViewTemplate(const BytesViewTemplate<OtherEndianess> ptr, size_t len)
: BytesReader(ptr.data(), min(len, ptr.size())) { }

template <Endian Endianess>
template<Endian OtherEndianess>
inline constexpr BytesViewTemplate<Endianess>::BytesViewTemplate(const BytesViewTemplate<OtherEndianess> ptr, size_t pos, size_t len)
: BytesReader(ptr.data() + pos, min(len, ptr.size() - pos)) { }


template <Endian Endianess>
auto BytesViewTemplate<Endianess>::operator =(const PoolBytes &b) -> Self & {
	return set(b);
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::operator =(const StdBytes &b) -> Self & {
	return set(b);
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::operator =(const Self &b) -> Self & {
	return set(b);
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::set(const PoolBytes &vec) -> Self & {
	ptr = vec.data();
	len = vec.size();
	return *this;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::set(const StdBytes &vec) -> Self & {
	ptr = vec.data();
	len = vec.size();
	return *this;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::set(const Self &vec) -> Self & {
	ptr = vec.data();
	len = vec.size();
	return *this;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::set(const uint8_t *p, size_t l) -> Self & {
	ptr = p;
	len = l;
	return *this;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::operator ++ () -> Self & {
	if (len > 0) {
		ptr ++; len --;
	}
	return *this;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::operator ++ (int) -> Self & {
	if (len > 0) {
		ptr ++; len --;
	}
	return *this;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::operator += (size_t l) -> Self & {
	if (len > 0) {
		offset(l);
	}
	return *this;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::operator == (const Self &other) const -> bool {
	return len == other.len && (ptr == other.ptr || memcmp(ptr, other.ptr, len * sizeof(CharType)) == 0);
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::operator != (const Self &other) const -> bool {
	return !(*this == other);
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::pdup(memory::pool_t *p) const -> Self {
	if (!p) {
		p = memory::pool::acquire();
	}
	auto buf = (uint8_t *)memory::pool::palloc(p, this->size() * sizeof(uint8_t));
	memcpy(buf, this->data(), this->size() * sizeof(uint8_t));
	return Self(buf, this->size());
}

template <Endian Endianess>
template <typename Interface>
auto BytesViewTemplate<Endianess>::bytes() const -> typename Interface::BytesType {
	return typename Interface::BytesType(data(), data() + size());
}

template <Endian Endianess>
template <typename T>
auto BytesViewTemplate<Endianess>::convert(const uint8_t *data) -> T {
	T res;
	memcpy(&res, data, sizeof(T));
	return Converter<T>::Swap(res);
};

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readUnsigned64() -> uint64_t {
	uint64_t ret = 0;
	if (len >= 8) {
		ret = convert<uint64_t>(ptr);
		ptr += 8; len -= 8;
	}
	return ret;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readUnsigned32() -> uint32_t {
	uint32_t ret = 0;
	if (len >= 4) {
		ret = convert<uint32_t>(ptr);
		ptr += 4; len -= 4;
	}
	return ret;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readUnsigned24() -> uint32_t {
	uint32_t ret = 0;
	if (len >= 3) {
		ret = (*ptr << 16) + (*(ptr + 1) << 8) + *(ptr + 2);
		ptr += 3; len -= 3;
	}
	return ret;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readUnsigned16() -> uint16_t {
	uint16_t ret = 0;
	if (len >= 2) {
		ret = convert<uint16_t>(ptr);
		ptr += 2; len -= 2;
	}
	return ret;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readUnsigned() -> uint8_t {
	uint8_t ret = 0;
	if (len > 0) { ret = *ptr; ++ ptr; -- len; }
	return ret;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readFloat64() -> double{
	double ret = 0;
	if (len >= 8) {
		ret = convert<double>(ptr);
		ptr += 8; len -= 8;
	}
	return ret;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readFloat32() -> float {
	float ret = 0; if (len >= 4) {
		ret = convert<float>(ptr);
		ptr += 4; len -= 4;
	}
	return ret;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readFloat16() -> float {
	return halffloat::decode(readUnsigned16());
}

// read null-terminated string
template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readString() -> StringView {
	size_t offset = 0; while (len - offset && ptr[offset]) { offset ++; }
	StringView ret((const char *)ptr, offset);
	ptr += offset; len -= offset;
	if (len && *ptr == 0) { ++ ptr; -- len; }
	return ret;
}

// read fixed-size string
template <Endian Endianess>
auto BytesViewTemplate<Endianess>::readString(size_t s) -> StringView {
	if (len < s) {
		s = len;
	}
	StringView ret((const char *)ptr, s);
	ptr += s; len -= s;
	return ret;
}

template <Endian Endianess>
auto BytesViewTemplate<Endianess>::toStringView() const -> StringView {
	return StringView((const char *)ptr, len);
}

template <Endian Endianess>
template <Endian Target>
auto BytesViewTemplate<Endianess>::readBytes(size_t s) -> BytesViewTemplate<Target> {
	if (len < s) {
		s = len;
	}
	BytesViewTemplate<Target> ret(ptr, s);
	ptr += s; len -= s;
	return ret;
}



template <typename Compare>
inline int compareDataRanges(const uint8_t *l, size_t __lsize,  const uint8_t *r, size_t __rsize, const Compare &cmp) {
	return std::lexicographical_compare(l, l + __lsize, r, r + __rsize, cmp);
}

template <Endian Endianess>
inline bool operator== (const memory::PoolInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return BytesViewTemplate<Endianess>(l) == r;
}

template <Endian Endianess>
inline bool operator== (const memory::StandartInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return BytesViewTemplate<Endianess>(l) == r;
}

template <Endian Endianess, size_t Size>
inline bool operator== (const std::array<uint8_t, Size> &l, const BytesViewTemplate<Endianess> &r) {
	return BytesViewTemplate<Endianess>(l) == r;
}

template <Endian Endianess>
inline bool operator== (const BytesViewTemplate<Endianess> &l, const memory::PoolInterface::BytesType &r) {
	return l == BytesViewTemplate<Endianess>(r);
}

template <Endian Endianess>
inline bool operator== (const BytesViewTemplate<Endianess> &l, const memory::StandartInterface::BytesType &r) {
	return l == BytesViewTemplate<Endianess>(r);
}

template <Endian Endianess, size_t Size>
inline bool operator== (const BytesViewTemplate<Endianess> &l, const std::array<uint8_t, Size> &r) {
	return l == BytesViewTemplate<Endianess>(r);
}


template <Endian Endianess>
inline bool operator!= (const memory::PoolInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return BytesViewTemplate<Endianess>(l) != r;
}

template <Endian Endianess>
inline bool operator!= (const memory::StandartInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return BytesViewTemplate<Endianess>(l) != r;
}

template <Endian Endianess, size_t Size>
inline bool operator!= (const std::array<uint8_t, Size> &l, const BytesViewTemplate<Endianess> &r) {
	return BytesViewTemplate<Endianess>(l) != r;
}

template <Endian Endianess>
inline bool operator!= (const BytesViewTemplate<Endianess> &l, const memory::PoolInterface::BytesType &r) {
	return l != BytesViewTemplate<Endianess>(r);
}

template <Endian Endianess>
inline bool operator!= (const BytesViewTemplate<Endianess> &l, const memory::StandartInterface::BytesType &r) {
	return l != BytesViewTemplate<Endianess>(r);
}

template <Endian Endianess, size_t Size>
inline bool operator!= (const BytesViewTemplate<Endianess> &l, const std::array<uint8_t, Size> &r) {
	return l != BytesViewTemplate<Endianess>(r);
}


template <Endian Endianess>
inline bool operator< (const BytesViewTemplate<Endianess> &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less<uint8_t>());
}

template <Endian Endianess>
inline bool operator< (const memory::PoolInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less<uint8_t>());
}

template <Endian Endianess>
inline bool operator< (const memory::StandartInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less<uint8_t>());
}

template <Endian Endianess>
inline bool operator< (const BytesViewTemplate<Endianess> &l, const memory::PoolInterface::BytesType &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less<uint8_t>());
}

template <Endian Endianess>
inline bool operator< (const BytesViewTemplate<Endianess> &l, const memory::StandartInterface::BytesType &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less<uint8_t>());
}


template <Endian Endianess>
inline bool operator<= (const BytesViewTemplate<Endianess> &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less_equal<uint8_t>());
}

template <Endian Endianess>
inline bool operator<= (const memory::PoolInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less_equal<uint8_t>());
}

template <Endian Endianess>
inline bool operator<= (const memory::StandartInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less_equal<uint8_t>());
}

template <Endian Endianess>
inline bool operator<= (const BytesViewTemplate<Endianess> &l, const memory::PoolInterface::BytesType &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less_equal<uint8_t>());
}

template <Endian Endianess>
inline bool operator<= (const BytesViewTemplate<Endianess> &l, const memory::StandartInterface::BytesType &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::less_equal<uint8_t>());
}


template <Endian Endianess>
inline bool operator> (const BytesViewTemplate<Endianess> &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater<uint8_t>());
}

template <Endian Endianess>
inline bool operator> (const memory::PoolInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater<uint8_t>());
}

template <Endian Endianess>
inline bool operator> (const memory::StandartInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater<uint8_t>());
}

template <Endian Endianess>
inline bool operator> (const BytesViewTemplate<Endianess> &l, const memory::PoolInterface::BytesType &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater<uint8_t>());
}

template <Endian Endianess>
inline bool operator> (const BytesViewTemplate<Endianess> &l, const memory::StandartInterface::BytesType &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater<uint8_t>());
}


template <Endian Endianess>
inline bool operator>= (const BytesViewTemplate<Endianess> &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater_equal<uint8_t>());
}

template <Endian Endianess>
inline bool operator>= (const memory::PoolInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater_equal<uint8_t>());
}

template <Endian Endianess>
inline bool operator>= (const memory::StandartInterface::BytesType &l, const BytesViewTemplate<Endianess> &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater_equal<uint8_t>());
}

template <Endian Endianess>
inline bool operator>= (const BytesViewTemplate<Endianess> &l, const memory::PoolInterface::BytesType &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater_equal<uint8_t>());
}

template <Endian Endianess>
inline bool operator>= (const BytesViewTemplate<Endianess> &l, const memory::StandartInterface::BytesType &r) {
	return std::lexicographical_compare(l.data(), l.data() + l.size(), r.data(), r.data() + r.size(), std::greater_equal<uint8_t>());
}

}

#endif /* STAPPLER_CORE_UTILS_SPDATAREADER_H_ */
