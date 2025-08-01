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

#ifndef STAPPLER_CORE_MEMORY_SPMEMSTORAGEMEMIMPL_H_
#define STAPPLER_CORE_MEMORY_SPMEMSTORAGEMEMIMPL_H_

#include "SPLogInit.h"
#include "SPMemAlloc.h"

namespace STAPPLER_VERSIONIZED stappler::memory::impl {

// small object optimization type (based on SSO-23: https://github.com/elliotgoodrich/SSO-23)
template <typename Type, size_t ByteCount>
struct mem_small {
	using self = mem_small<Type, ByteCount>;
	using pointer = Type *;
	using const_pointer = const Type *;
	using size_type = size_t;
	using allocator = Allocator<Type>;

	static constexpr size_type max_capacity() {
		return (sizeof(Type) < ByteCount) ? ((ByteCount - 1) / sizeof(Type)) : 0;
	}

	void assign(allocator &a, const_pointer ptr, size_type s) {
		const auto current = size();
		a.copy_rewrite(data(), size(), ptr, s);
		if (current > s) {
			a.destroy(data() + s, current - s);
		}
		set_size(s);
	}

	void move_assign(allocator &a, pointer source, size_type count) {
		const auto current = size();
		a.move_rewrite(data(), current, source, count);
		if (current > count) {
			a.destroy(data() + count, current - count);
		}
		set_size(count);
	}

	void force_clear() { set_size(0); }

	void drop_unused() {
		const auto unused = storage[ByteCount - 1];
		if (unused < max_capacity()) {
			const auto s = max_capacity() - unused;
			// data is already garbage, bypass -Wclass-memaccess
			memset((void *)(storage.data() + s * sizeof(Type)), 0, unused * sizeof(Type));
		}
	}

	void set_size(size_t s) {
		storage[ByteCount - 1] = uint8_t(max_capacity() - s);
		drop_unused();
	}

	size_t modify_size(intptr_t diff) {
		storage[ByteCount - 1] = uint8_t(max_capacity() - (size() + diff));
		return size();
	}

	size_t size() const { return max_capacity() - storage[ByteCount - 1]; }

	size_t capacity() const { return max_capacity(); }

	pointer data() { return (pointer)storage.data(); }
	const_pointer data() const { return (const_pointer)storage.data(); }

	std::array<uint8_t, ByteCount> storage;
};

template <typename Type, size_t Extra = 0>
class mem_large {
public:
	using self = mem_large<Type, Extra>;
	using pointer = Type *;
	using const_pointer = const Type *;
	using size_type = size_t;
	using allocator = Allocator<Type>;

	mem_large() = default;
	mem_large(const self &) = default;
	self &operator=(const self &other) = default;

	mem_large(self &&other) {
		_ptr = other._ptr;
		_used = other._used;
		_allocated = other._allocated;
		other._ptr = nullptr;
		other._used = 0;
		other._allocated = 0;
	}

	self &operator=(self &&other) {
		_ptr = other._ptr;
		_used = other._used;
		_allocated = other._allocated;
		other._ptr = nullptr;
		other._used = 0;
		other._allocated = 0;
		return *this;
	}

	void assign(allocator &a, const_pointer ptr, size_type count) {
		if (_allocated < count) {
			reserve(a, count);
			if (!_ptr) {
				reserve(a, count);
			}
		}
		a.copy_rewrite(data(), size(), ptr, count);
		if (_used > count) {
			a.destroy(data() + count, _used - count);
		}
		_used = count;
		drop_unused();
	}

	void move_assign(allocator &a, pointer ptr, size_type count) {
		if (_allocated < count) {
			reserve(a, count);
		}
		a.move_rewrite(_ptr, _used, ptr, count);
		if (_used > count) {
			a.destroy(data() + count, _used - count);
		}
		_used = count;
		drop_unused();
	}

	void assign_weak(pointer ptr, size_type s) {
		_ptr = ptr;
		_used = s;
		_allocated = 0;
	}

	void assign_weak(const_pointer ptr, size_type s) {
		_ptr = const_cast<pointer>(ptr);
		_used = s;
		_allocated = 0;
	}

	void assign_mem(pointer ptr, size_type s, size_type nalloc) {
		_ptr = ptr;
		_used = s;
		_allocated = nalloc - Extra;
	}

	bool is_weak() const noexcept { return _used > 0 && _allocated == 0; }

	void reserve(allocator &a, size_type s) {
		grow_alloc(a, s);
		drop_unused();
	}

	void clear_dealloc(allocator &a) {
		if (_ptr) {
			if (_used) {
				a.destroy(_ptr, _used);
			}
			if (_allocated) {
				a.deallocate(_ptr, _allocated + Extra);
			}
		}
		_ptr = nullptr;
		_used = 0;
		_allocated = 0;
	}

	void force_clear() {
		_ptr = nullptr;
		_used = 0;
		_allocated = 0;
	}

	pointer extract() {
		auto ret = _ptr;
		force_clear();
		return ret;
	}

	void drop_unused() {
		if (_allocated > 0 && _allocated >= _used && _ptr) {
			// data is already garbage, bypass -Wclass-memaccess
			memset((void *)(_ptr + _used), 0, (_allocated - _used + Extra) * sizeof(Type));
		}
	}

	void grow_alloc(allocator &a, size_type newsize) {
		size_t alloc_size = newsize + Extra;

		// use extra memory if provided by allocator
		size_t allocated = 0; // real memory block size returned
		auto ptr = a.__allocate(alloc_size, allocated);

		alloc_size = allocated / sizeof(Type);

		if (_used > 0 && _ptr) {
			a.move(ptr, _ptr, _used);
		}

		if (_ptr && _allocated > 0) {
			a.deallocate(_ptr, _allocated);
		}

		_ptr = ptr;
		_allocated = alloc_size - Extra;
	}

	size_t modify_size(intptr_t diff) {
		_used += diff;
		return _used;
	}

	void set_size(size_t s) {
		if (s < _used) {
			_used = s;
			drop_unused();
		} else {
			_used = s;
		}
	}

	size_t size() const noexcept { return _used; }
	size_t capacity() const noexcept { return _allocated; }

	pointer data() noexcept { return _ptr; }
	const_pointer data() const noexcept { return _ptr; }

	bool empty() const noexcept { return _ptr == nullptr || _used == 0; }

protected:
	pointer _ptr = nullptr;
	size_type _used = 0; // in elements
	size_type _allocated = 0; // in elements
};

template <typename Type, size_t Extra, bool UseSoo>
class mem_soo_iface;

template <typename Type, size_t Extra>
class mem_soo_iface<Type, Extra, false> : public mem_large<Type, Extra> {
public:
	using base = mem_large<Type, Extra>;
	using pointer = typename base::pointer;
	using const_pointer = typename base::const_pointer;
	using size_type = typename base::size_type;
	using allocator = typename base::allocator;

	static constexpr size_type get_soo_size() { return 0; }

	mem_soo_iface(const allocator &alloc) : _allocator(alloc) {
		SPASSERT(_allocator, "Allocator should be defined");
	}

	~mem_soo_iface() noexcept { clear_dealloc(_allocator); }

	void assign(const_pointer ptr, size_type size) {
		reserve(size, false);
		_allocator.copy_rewrite(_ptr, _used, ptr, size);
		if (_used > size) {
			_allocator.destroy(data() + size, _used - size);
		}
		_used = size;
		drop_unused();
	}

	using base::assign_weak;
	using base::assign_mem;
	using base::is_weak;

	using base::data;
	using base::size;
	using base::capacity;

	// reserve memory block, optimal for realloc/free
	// useful for small temporary buffers
	// this memory block can be reused by next temporary buffer of same size
	// so, no pool memory will be leaked
	pointer reserve_block_optimal() {
		auto target = mempool::custom::BlockThreshold / sizeof(Type) + 1;
		return reserve(target);
	}

	pointer reserve(size_type s, bool grow = false) {
		if (s > 0 && s > _allocated) {
			auto newmem = (grow ? max(s, _allocated * 2) : s);
			base::reserve(_allocator, newmem);
		}
		return _ptr;
	}

	void clear() {
		if (_used > 0 && _allocated > 0) {
			if (_ptr) {
				_allocator.destroy(_ptr, _used);
			}
		} else {
			if (_allocated == 0) {
				_ptr = nullptr;
			}
		}
		_used = 0;
	}

	using base::force_clear;
	using base::extract;

protected:
	void perform_move(mem_soo_iface &&other) {
		*(static_cast<base *>(this)) = sp::move_unsafe(other);
	}

	using base::clear_dealloc;
	using base::modify_size;
	using base::set_size;
	using base::drop_unused;

	using base::_ptr;
	using base::_allocated;
	using base::_used;
	allocator _allocator;
};

template <typename Type, size_t Extra>
class mem_soo_iface<Type, Extra, true> {
public:
	using pointer = Type *;
	using const_pointer = const Type *;

	using size_type = size_t;
	using allocator = Allocator<Type>;

	using large_mem = mem_large<Type, Extra>;
	using small_mem = mem_small<Type, sizeof(large_mem)>;

	static constexpr size_type get_soo_size() { return small_mem::max_capacity(); }

	mem_soo_iface(const allocator &alloc) : _allocator(alloc) {
		SPASSERT(_allocator, "Allocator should be defined");
		set_large_flag();
		_large = large_mem();
	}

	~mem_soo_iface() noexcept {
		if (is_large()) {
			_large.clear_dealloc(_allocator);
		}
	}

	void assign(const_pointer ptr, size_type size) {
		if (!ptr || size == 0) {
			clear();
		} else if (size <= small_mem::max_capacity() && (is_small() || empty())) {
			set_small_flag();
			_small.assign(_allocator, ptr, size);
		} else if (size > small_mem::max_capacity() && (is_large() || empty())) {
			set_large_flag();
			_large.assign(_allocator, ptr, size);
		} else {
			if (is_small()) {
				large_mem new_large;
				new_large.assign(_allocator, ptr, size);
				set_large_flag_force();
				_large = sp::move_unsafe(new_large);
			} else {
				large_mem old_large(sp::move_unsafe(_large));
				_small.force_clear();
				set_small_flag();
				_small.assign(_allocator, ptr, size);
				old_large.clear_dealloc(_allocator);
			}
		}
	}

	void assign_weak(pointer ptr, size_type s) {
		if (s <= small_mem::max_capacity()) {
			assign(ptr, s);
		} else {
			set_large_flag_force();
			_large.assign_weak(ptr, s);
		}
	}

	void assign_weak(const_pointer ptr, size_type s) {
		if (s <= small_mem::max_capacity()) {
			assign(ptr, s);
		} else {
			set_large_flag_force();
			_large.assign_weak(ptr, s);
		}
	}

	void assign_mem(pointer ptr, size_type s, size_type nalloc) {
		set_large_flag_force();
		_large.assign_mem(ptr, s, nalloc);
	}

	// РџСЂРѕРІРµСЂРєР°, РІР»Р°РґРµРµРј Р»Рё Р±Р»РѕРєРѕРј РїР°РјСЏС‚Рё
	bool is_weak() const noexcept { return is_large() && _large.is_weak(); }

	pointer reserve(size_type s, bool grow = false) {
		const auto _allocated = capacity();
		const auto _used = size();
		if (s > _allocated) {
			const auto smallMaxCap = small_mem::max_capacity();
			if (s <= smallMaxCap && _used <= smallMaxCap) {
				if (_allocated == 0) { // РїР°РјСЏС‚СЊ РїСѓСЃС‚Р°СЏ РёР»Рё CoW
					set_small_flag();
					if (_large.data() != nullptr) { // CoW
						_small.move_assign(_allocator, _large.data(), _large.size());
					} else { // empty
						_small.force_clear();
					}
				}
				return _small.data();
			} else if (s > 0) {
				auto newmem = (grow ? max(s, _allocated * 2) : s);
				if (is_small() && newmem > small_mem::max_capacity()) {
					large_mem new_large;
					new_large.reserve(_allocator, newmem);
					new_large.move_assign(_allocator, _small.data(), _small.size());
					set_large_flag();
					_large = sp::move_unsafe(new_large);
				} else {
					_large.reserve(_allocator, newmem);
				}
				return _large.data();
			}
		}
		return data();
	}

	void clear() {
		const auto _used = size();
		const auto _allocated = capacity();
		auto _ptr = data();
		if (_used > 0 && _allocated > 0 && _ptr) {
			_allocator.destroy(_ptr, _used);
		} else {
			if (_allocated == 0 && is_large()) { // РїСЂРѕРІРµСЂСЏРµРј Рё РѕС‡РёС‰Р°РµРј CoW
				_large.force_clear();
			}
		}

		if (is_large()) {
			_large.set_size(0);
		} else {
			set_large_flag();
			_large.force_clear();
		}
	}

	void force_clear() {
		if (is_small()) {
			set_large_flag();
		}
		_large.force_clear();
	}

	pointer extract() {
		if (is_large()) {
			return _large.extract();
		} else {
			auto s = _small.size();
			auto ptr = _allocator.allocate(s + Extra);
			_allocator.move(ptr, _small.data(), s);
			if constexpr (Extra) {
				// zero-terminated, bypass -Wclass-memaccess
				memset((void *)(ptr + s), 0, Extra * sizeof(Type));
			}
			force_clear();
			return ptr;
		}
	}

	pointer data() noexcept { return is_large() ? _large.data() : _small.data(); }
	const_pointer data() const noexcept { return is_large() ? _large.data() : _small.data(); }

	size_type size() const noexcept { return is_large() ? _large.size() : _small.size(); }
	size_type capacity() const noexcept {
		return is_large() ? _large.capacity() : _small.capacity();
	}

	bool empty() const noexcept { return is_large() ? _large.empty() : (_small.size() == 0); }

protected:
	void perform_move(mem_soo_iface &&other) {
		if (other.is_small()) {
			set_small_flag();
			_small.force_clear();
			_small.move_assign(this->_allocator, other.data(), other.size());
			other._small.force_clear();
		} else {
			set_large_flag();
			_large = sp::move_unsafe(other._large);
		}
	}

	void clear_dealloc(allocator &a) {
		if (is_large()) {
			_large.clear_dealloc(a);
		} else {
			clear();
		}
	}

	size_type modify_size(intptr_t diff) {
		return is_large() ? _large.modify_size(diff) : _small.modify_size(diff);
	}

	void set_size(size_type s) {
		if (is_large()) {
			_large.set_size(s);
		} else {
			_small.set_size(s);
		}
	}

protected:
	allocator _allocator;

private:
	bool is_small() const { return this->_allocator.test(allocator::FirstFlag); }
	bool is_large() const { return !this->_allocator.test(allocator::FirstFlag); }

	void set_large_flag() { this->_allocator.reset(allocator::FirstFlag); }
	void set_large_flag_force() {
		clear(); // discard content
		set_large_flag();
	}

	void set_small_flag() { this->_allocator.set(allocator::FirstFlag); }

	union {
		large_mem _large;
		small_mem _small;
	};
};

} // namespace stappler::memory::impl

#endif /* STAPPLER_CORE_MEMORY_SPMEMSTORAGEMEMIMPL_H_ */
