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

#ifndef STAPPLER_CORE_IO_SPIOBUFFER_H_
#define STAPPLER_CORE_IO_SPIOBUFFER_H_

#include "SPIOCommon.h"

namespace STAPPLER_VERSIONIZED stappler::io {

struct SP_PUBLIC Buffer {
	template <typename T, typename Traits = BufferTraits<T>> Buffer(T &t);

	uint8_t * prepare(size_t & size) const;
	void save(uint8_t *, size_t source, size_t nbytes) const;

	size_t capacity() const;
	size_t size() const;
	uint8_t *data() const;
	void clear() const;

	void *ptr = nullptr;
	prepare_fn prepare_ptr = nullptr;
	save_fn save_ptr = nullptr;
	size_fn size_ptr = nullptr;
	size_fn capacity_ptr = nullptr;
	data_fn data_ptr = nullptr;
	clear_fn clear_ptr = nullptr;
};

template <typename T, typename Traits>
inline Buffer::Buffer(T &t)
: ptr((void *)&t)
, prepare_ptr(&Traits::PrepareFn)
, save_ptr(&Traits::SaveFn)
, size_ptr(&Traits::SizeFn)
, capacity_ptr(&Traits::CapacityFn)
, data_ptr(&Traits::DataFn)
, clear_ptr(&Traits::ClearFn) { }

// reserve memory block in buffer
inline uint8_t * Buffer::prepare(size_t & size) const { return prepare_ptr(ptr, size); }
inline void Buffer::save(uint8_t *buf, size_t source, size_t nbytes) const { save_ptr(ptr, buf, source, nbytes); }
inline size_t Buffer::capacity() const { return capacity_ptr(ptr); }
inline size_t Buffer::size() const { return size_ptr(ptr); }
inline uint8_t *Buffer::data() const { return data_ptr(ptr); }
inline void Buffer::clear() const { clear_ptr(ptr); }

}

#endif /* STAPPLER_CORE_IO_SPIOBUFFER_H_ */
