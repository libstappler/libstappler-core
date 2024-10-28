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

#ifndef STAPPLER_CORE_MEMORY_SPMEMPOOLAPI_H_
#define STAPPLER_CORE_MEMORY_SPMEMPOOLAPI_H_

#include "SPMemPoolInterface.h"

namespace STAPPLER_VERSIONIZED stappler::memory {

using namespace mempool::base;

static constexpr int SUCCESS = 0;

namespace pool {

using namespace mempool::base::pool;

// RAII wrapper for pool push+pop
template<typename _Pool = pool_t *>
class context {
public:
	using pool_type = _Pool;

	enum finalize_flag {
		discard, // do nothing
		conditional, // do not push pool if current context pool is the same
		clear, // clear pool after pop
		destroy // destroy pool after pop
	};

	explicit context(const pool_type &__m, finalize_flag = discard);

	context(const pool_type &__m, uint32_t tag, void *userdata, finalize_flag = discard);
	~context();

	context(const context &) = delete;
	context& operator=(const context &) = delete;

	context(context && u) noexcept;

	context & operator=(context && u) noexcept;

	void push() noexcept;

	void push(uint32_t tag, void *userdata) noexcept;

	void pop() noexcept;

	void swap(context &u) noexcept;

	bool owns() const noexcept { return _owns; }

	operator pool_type() const noexcept { return _pool; }

private:
	pool_type _pool;
	bool _owns;
	finalize_flag _flag;
};

template <typename Callback>
inline auto perform(const Callback &cb, memory::pool_t *p);

template <typename Callback>
inline auto perform(const Callback &cb, memory::pool_t *p, uint32_t tag, void *userdata = nullptr);

template <typename Callback>
inline auto perform_conditional(const Callback &cb, memory::pool_t *p);

template <typename Callback>
inline auto perform_conditional(const Callback &cb, memory::pool_t *p, uint32_t tag, void *userdata = nullptr);

template <typename Callback>
inline auto perform_clear(const Callback &cb, memory::pool_t *p);

template <typename Callback>
inline auto perform_clear(const Callback &cb, memory::pool_t *p, uint32_t tag, void *userdata = nullptr);

template <typename Callback>
inline auto perform_temporary(const Callback &cb, memory::pool_t *p = nullptr);

template <typename Callback>
inline auto perform_temporary(const Callback &cb, memory::pool_t *p, uint32_t tag, void *userdata = nullptr);

}

}



//
// Implementation details
//

namespace STAPPLER_VERSIONIZED stappler::memory::pool {


template<typename _Pool>
context<_Pool>::context(const pool_type &__m, finalize_flag f)
: _pool(__m), _owns(false), _flag(f) {
	push();
}

template<typename _Pool>
context<_Pool>::context(const pool_type &__m, uint32_t tag, void *userdata, finalize_flag f)
: _pool(__m), _owns(false), _flag(f) {
	push(tag, userdata);
}

template<typename _Pool>
context<_Pool>::~context() {
	if (!_owns) {
		return;
	}

	pop();
}

template<typename _Pool>
context<_Pool>::context(context && u) noexcept
: _pool(u._pool), _owns(u._owns), _flag(u._flag) {
	u._pool = 0;
	u._owns = false;
}

template<typename _Pool>
auto context<_Pool>::operator=(context && u) noexcept -> context & {
	if (this == &u) {
		return *this;
	}

	if (_owns) {
		pop();
	}

	context(std::move(u)).swap(*this);

	u._pool = 0;
	u._owns = false;
	return *this;
}

template<typename _Pool>
void context<_Pool>::push() noexcept {
	if (_pool && !_owns) {
		if (_flag != conditional || pool::acquire() != _pool) {
			pool::push(_pool);
			_owns = true;
		}
	}
}

template<typename _Pool>
void context<_Pool>::push(uint32_t tag, void *userdata) noexcept {
	if (_pool && !_owns) {
		if (_flag != conditional || pool::acquire() != _pool) {
			pool::push(_pool, tag, userdata);
			_owns = true;
		}
	}
}

template<typename _Pool>
void context<_Pool>::pop() noexcept {
	if (!_owns) {
		return;
	}

	pool::pop();

	switch (_flag) {
	case discard:
	case conditional:
		break;
	case clear:
		pool::clear(_pool);
		break;
	case destroy:
		pool::destroy(_pool);
		_pool = nullptr;
		break;
	}

	_owns = false;
}

template<typename _Pool>
void context<_Pool>::swap(context &u) noexcept {
	std::swap(_pool, u._pool);
	std::swap(_owns, u._owns);
	std::swap(_flag, u._flag);
}

template <typename Callback>
inline auto perform(const Callback &cb, memory::pool_t *p) {
	context<decltype(p)> holder(p);
	return cb();
}

template <typename Callback>
inline auto perform(const Callback &cb, memory::pool_t *p, uint32_t tag, void *ptr) {
	context<decltype(p)> holder(p, tag, ptr);
	return cb();
}

template <typename Callback>
inline auto perform_conditional(const Callback &cb, memory::pool_t *p) {
	context<decltype(p)> holder(p, context<decltype(p)>::conditional);
	return cb();
}

template <typename Callback>
inline auto perform_conditional(const Callback &cb, memory::pool_t *p, uint32_t tag, void *ptr) {
	context<decltype(p)> holder(p, tag, ptr, context<decltype(p)>::conditional);
	return cb();
}

template <typename Callback>
inline auto perform_clear(const Callback &cb, memory::pool_t *p) {
	context<decltype(p)> holder(p, context<decltype(p)>::clear);
	return cb();
}

template <typename Callback>
inline auto perform_clear(const Callback &cb, memory::pool_t *p, uint32_t tag, void *ptr) {
	context<decltype(p)> holder(p, tag, ptr, context<decltype(p)>::clear);
	return cb();
}

template <typename Callback>
inline auto perform_temporary(const Callback &cb, memory::pool_t *p) {
	auto pool = memory::pool::create(p ? p : memory::pool::acquire());
	context<decltype(p)> holder(pool, context<decltype(p)>::destroy);
	return cb();
}

template <typename Callback>
inline auto perform_temporary(const Callback &cb, memory::pool_t *p, uint32_t tag, void *ptr) {
	auto pool = memory::pool::create(p ? p : memory::pool::acquire());
	context<decltype(p)> holder(pool, tag, ptr, context<decltype(p)>::destroy);
	return cb();
}

}

#endif /* STAPPLER_CORE_MEMORY_SPMEMPOOLAPI_H_ */
