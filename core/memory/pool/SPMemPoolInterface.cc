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

#include "SPMemPoolInterface.h"

#include "SPMemFunction.h"
#include "SPMemPoolApi.h"

// requires libbacktrace
#define DEBUG_BACKTRACE 0
#define DEBUG_POOL_LIST 0

#if DEBUG_BACKTRACE
#include <backtrace.h>
#endif

namespace STAPPLER_VERSIONIZED stappler::mempool::base::pool {

static constexpr size_t SP_ALLOC_STACK_SIZE = 256;

static void setPoolInfo(pool_t *p, uint32_t tag, const void *ptr);

class AllocStack {
public:
	struct Info {
		pool_t *pool;
		uint32_t tag;
		const void *ptr;
	};

	AllocStack();

	pool_t *top() const;
	Pair<uint32_t, const void *> info() const;
	const Info &back() const;

	void push(pool_t *);
	void push(pool_t *, uint32_t, const void *);
	void pop();

	void foreachInfo(void *, bool(*cb)(void *, pool_t *, uint32_t, const void *));

protected:
	template <typename T>
	struct stack {
		size_t size = 0;
		std::array<T, SP_ALLOC_STACK_SIZE> data;

		bool empty() const { return size == 0; }
#if DEBUG
		void push(const T &t) {
			if (size < data.size()) {
				data[size] = t; ++ size;
			} else {
				abort();
			}
		}
		void pop() {
			if (size > 0) {
				-- size;
			} else {
				abort();
			}
		}
		const T &get() const {
			if (size == 0) {
				abort();
			}
			return data[size - 1];
		}
#else
		void push(const T &t) { data[size ++] = t; }
		void pop() { -- size; }
		const T &get() const { return data[size - 1]; }
#endif
	};

	stack<Info> _stack;
};

AllocStack::AllocStack() {
	_stack.push(Info{nullptr, 0, nullptr});
}

pool_t *AllocStack::top() const {
	return _stack.get().pool;
}

Pair<uint32_t, const void *> AllocStack::info() const {
	return pair(_stack.get().tag, _stack.get().ptr);
}

const AllocStack::Info &AllocStack::back() const {
	return _stack.get();
}

void AllocStack::push(pool_t *p) {
	if (p) {
		_stack.push(Info{p, 0, nullptr});
	} else {
		abort();
	}
}
void AllocStack::push(pool_t *p, uint32_t tag, const void *ptr) {
	if (p) {
		_stack.push(Info{p, tag, ptr});
	} else {
		abort();
	}
}

void AllocStack::pop() {
	_stack.pop();
}

void AllocStack::foreachInfo(void *data, bool(*cb)(void *, pool_t *, uint32_t, const void *)) {
	for (size_t i = 0; i < _stack.size; ++ i) {
		auto &it = _stack.data[_stack.size - 1 - i];
		if (it.pool && !cb(data, it.pool, it.tag, it.ptr)) {
			break;
		}
	}
}

static AllocStack &get_stack() {
	static thread_local AllocStack tl_stack;
	return tl_stack;
}

pool_t *acquire() {
	return get_stack().top();
}

Pair<uint32_t, const void *> info() {
	return get_stack().info();
}

void push(pool_t *p) {
	return get_stack().push(p);
}
void push(pool_t *p, uint32_t tag, const void *ptr) {
	setPoolInfo(p, tag, ptr);
	return get_stack().push(p, tag, ptr);
}
void pop() {
	return get_stack().pop();
}

void foreach_info(void *data, bool(*cb)(void *, pool_t *, uint32_t, const void *)) {
	get_stack().foreachInfo(data, cb);
}

static inline bool isCustom(allocator_t *alloc) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (alloc && *((uintptr_t *)alloc) == custom::POOL_MAGIC) {
			return true;
		} else {
			return false;
		}
	}
	return true;
}

static inline bool isCustom(pool_t *p) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (p && *((uintptr_t *)p) == custom::POOL_MAGIC) {
			return true;
		} else {
			return false;
		}
	}
	return true;
}

}


typedef struct apr_allocator_t apr_allocator_t;
typedef struct apr_pool_t apr_pool_t;

using apr_status_t = int;

namespace STAPPLER_VERSIONIZED stappler::mempool::apr {

using allocator_t = apr_allocator_t;
using status_t = apr_status_t;
using pool_t = apr_pool_t;
using cleanup_fn = status_t(*)(void *);

}

namespace STAPPLER_VERSIONIZED stappler::mempool::apr::allocator {

SPUNUSED static allocator_t *create();
SPUNUSED static allocator_t *create(void *mutex);
SPUNUSED static void destroy(allocator_t *alloc);
SPUNUSED static void owner_set(allocator_t *alloc, pool_t *pool);
SPUNUSED static pool_t * owner_get(allocator_t *alloc);
SPUNUSED static void max_free_set(allocator_t *alloc, size_t size);

}

namespace STAPPLER_VERSIONIZED stappler::mempool::apr::pool {

SPUNUSED static void initialize();
SPUNUSED static void terminate();
SPUNUSED static pool_t *create();
SPUNUSED static pool_t *create(apr_allocator_t *alloc);
SPUNUSED static pool_t *create(pool_t *p);
SPUNUSED static pool_t *createTagged(const char *tag);
SPUNUSED static pool_t *createTagged(pool_t *p, const char *tag);
SPUNUSED static void destroy(pool_t *p);
SPUNUSED static void clear(pool_t *p);
SPUNUSED static void *alloc(pool_t *p, size_t &size);
SPUNUSED static void free(pool_t *p, void *ptr, size_t size);
SPUNUSED static void *palloc(pool_t *p, size_t size);
SPUNUSED static void *calloc(pool_t *p, size_t count, size_t eltsize);
SPUNUSED static void cleanup_kill(pool_t *p, void *ptr, status_t(*cb)(void *));
SPUNUSED static void cleanup_register(pool_t *p, void *ptr, status_t(*cb)(void *));
SPUNUSED static void pre_cleanup_register(pool_t *p, void *ptr, status_t(*cb)(void *));
SPUNUSED static status_t userdata_set(const void *data, const char *key, cleanup_fn cb, pool_t *pool);
SPUNUSED static status_t userdata_setn(const void *data, const char *key, cleanup_fn cb, pool_t *pool);
SPUNUSED static status_t userdata_get(void **data, const char *key, pool_t *pool);
SPUNUSED static size_t get_allocated_bytes(pool_t *p);
SPUNUSED static size_t get_return_bytes(pool_t *p);
SPUNUSED static allocator_t *get_allocator(pool_t *p);
SPUNUSED static void *pmemdup(pool_t *a, const void *m, size_t n);
SPUNUSED static char *pstrdup(pool_t *a, const char *s);
SPUNUSED static void setPoolInfo(pool_t *p, uint32_t tag, const void *ptr);
SPUNUSED static bool isThreadSafeAsParent(pool_t *pool);
SPUNUSED static const char *get_tag(pool_t *pool);

}


namespace STAPPLER_VERSIONIZED stappler::mempool::base::allocator {

allocator_t *create(bool custom) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!custom) {
			return (allocator_t *)apr::allocator::create();
		}
	}
	return (allocator_t *) (new custom::Allocator());
}

allocator_t *create(void *mutex) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		return (allocator_t *)apr::allocator::create(mutex);
	}
	abort(); // custom allocator with mutex is not available
	return nullptr;
}

allocator_t *createWithMmap(uint32_t initialPages) {
#if LINUX
	auto alloc = new custom::Allocator();
	alloc->run_mmap(initialPages);
	return (allocator_t *) alloc;
#endif
	return (allocator_t *) nullptr;
}

void destroy(allocator_t *alloc) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (pool::isCustom(alloc)) {
			delete (custom::Allocator *)alloc;
		} else {
			apr::allocator::destroy((apr::allocator_t *)alloc);
		}
	} else {
		delete (custom::Allocator *)alloc;
	}
}

void owner_set(allocator_t *alloc, pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (pool::isCustom(alloc)) {
			if (pool::isCustom(pool)) {
				((custom::Allocator *)alloc)->owner = (custom::Pool *)pool;
			} else {
				abort();
			}
		} else {
			apr::allocator::owner_set((apr::allocator_t *)alloc, (apr::pool_t *)pool);
		}
	} else {
		((custom::Allocator *)alloc)->owner = (custom::Pool *)pool;
	}
}

pool_t * owner_get(allocator_t *alloc) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!pool::isCustom(alloc)) {
			return (pool_t *)apr::allocator::owner_get((apr::allocator_t *)alloc);
		}
	}
	return (pool_t *)((custom::Allocator *)alloc)->owner;
}

void max_free_set(allocator_t *alloc, size_t size) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (pool::isCustom(alloc)) {
			((custom::Allocator *)alloc)->set_max(size);
		} else {
			apr::allocator::max_free_set((apr::allocator_t *)alloc, size);
		}
	} else {
		((custom::Allocator *)alloc)->set_max(size);
	}
}

}

namespace STAPPLER_VERSIONIZED stappler::mempool::base::pool {

static std::atomic<size_t> s_activePools = 0;
static std::atomic<bool> s_poolDebug = 0;
static std::mutex s_poolDebugMutex;
static pool_t *s_poolDebugTarget = nullptr;
static std::map<pool_t *, const char **, std::less<void>> s_poolDebugInfo;

#if DEBUG_POOL_LIST
static std::vector<pool_t *> s_poolList;
#endif

#if DEBUG_BACKTRACE
static ::backtrace_state *s_backtraceState;

struct debug_bt_info {
	pool_t *pool;
	const char **target;
	size_t index;
};

static void debug_backtrace_error(void *data, const char *msg, int errnum) {
	std::cout << "Backtrace error: " << msg << "\n";
}

static int debug_backtrace_full_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function) {
	auto ptr = (debug_bt_info *)data;

	std::ostringstream f;
	f << "[" << ptr->index << " - 0x" << std::hex << pc << std::dec << "]";

	if (filename) {
		auto name = filepath::name(filename);
		f << " " << name << ":" << lineno;
	}
	if (function) {
		f << " - ";
		int status = 0;
		auto ptr = abi::__cxa_demangle (function, nullptr, nullptr, &status);
    	if (ptr) {
    		f << (const char *)ptr;
			::free(ptr);
		} else {
			f << function;
		}
	}

	auto tmp = f.str();
	*ptr->target = pstrdup(ptr->pool, tmp.data());
	++ ptr->target;
	++ ptr->index;

	if (ptr->index > 20) {
		return 1;
	}

	return 0;
}

static const char **getPoolInfo(pool_t *pool) {
	static constexpr size_t len = 20;
	static constexpr size_t offset = 2;
	const char **ret = (const char **)calloc(s_poolDebugTarget, len + offset + 2, sizeof(const char *));
	size_t retIt = 0;

	do {
		std::ostringstream f;
		f << "Pool " << (void *)pool << " (" << s_activePools.load() << ")";
		auto tmp = f.str();
		ret[retIt] = pstrdup(s_poolDebugTarget, tmp.data()); ++ retIt;
	} while(0);

	debug_bt_info info;
	info.pool = s_poolDebugTarget;
	info.target = ret + 1;
	info.index = 0;

	backtrace_full(s_backtraceState, 2, debug_backtrace_full_callback, debug_backtrace_error, &info);
	return ret;
}
#else
static const char **getPoolInfo(pool_t *pool) {
	return nullptr;
}
#endif

static pool_t *pushPoolInfo(pool_t *pool) {
	if (pool) {
		++ s_activePools;
		if (s_poolDebug.load()) {
			if (auto ret = getPoolInfo(pool)) {
				s_poolDebugMutex.lock();
				s_poolDebugInfo.emplace(pool, ret);
				s_poolDebugMutex.unlock();
			}
		}
#if DEBUG_POOL_LIST
		if (isCustom(pool)) {
			s_poolDebugMutex.lock();
			s_poolList.emplace_back(pool);
			s_poolDebugMutex.unlock();
		}
#endif
	}
	return pool;
}

SPUNUSED static void popPoolInfo(pool_t *pool) {
	if (pool) {
		if (s_poolDebug.load()) {
			s_poolDebugMutex.lock();
			s_poolDebugInfo.erase(pool);
			s_poolDebugMutex.unlock();
		}
#if DEBUG_POOL_LIST
		if (isCustom(pool)) {
			s_poolDebugMutex.lock();
			auto it = std::find(s_poolList.begin(), s_poolList.end(), pool);
			if (it != s_poolList.end()) {
				s_poolList.erase(it);
			}
			s_poolDebugMutex.unlock();
		}
#endif
		-- s_activePools;
	}
}

void initialize() {
	if constexpr (apr::SP_APR_COMPATIBLE) { apr::pool::initialize(); }
	custom::initialize();
}

void terminate() {
	if constexpr (apr::SP_APR_COMPATIBLE) { apr::pool::terminate(); }
	custom::terminate();
}

pool_t *create(PoolFlags flags) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if ((flags & PoolFlags::Custom) == PoolFlags::None) {
			return pushPoolInfo((pool_t *)apr::pool::create());
		}
	}
	return pushPoolInfo((pool_t *)custom::Pool::create(nullptr, flags));
}

pool_t *create(allocator_t *alloc, PoolFlags flags) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (isCustom(alloc)) {
			return pushPoolInfo((pool_t *)custom::Pool::create((custom::Allocator *)alloc, flags));
		} else if ((flags & PoolFlags::ThreadSafePool) == PoolFlags::None) {
			return pushPoolInfo((pool_t *)apr::pool::create((apr::allocator_t *)alloc));
		} else {
			abort(); // thread-safe APR pools is not supported
		}
	}
	return pushPoolInfo((pool_t *)custom::Pool::create((custom::Allocator *)alloc, flags));
}

// creates managed pool (managed by root, if parent in mullptr)
pool_t *create(pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return pushPoolInfo((pool_t *)apr::pool::create((apr::pool_t *)pool));
		}
	}
	return pushPoolInfo((pool_t *)custom::create((custom::Pool *)pool));
}

// creates unmanaged pool
pool_t *createTagged(const char *tag, PoolFlags flags) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if ((flags & PoolFlags::Custom) == PoolFlags::None) {
			return pushPoolInfo((pool_t *)apr::pool::createTagged(tag));
		}
	}
	if (auto ret = custom::Pool::create(nullptr, flags)) {
		ret->allocmngr.name = tag;
		return pushPoolInfo((pool_t *)ret);
	}
	return nullptr;
}

pool_t *createTagged(pool_t *p, const char *tag) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(p)) {
			return pushPoolInfo((pool_t *)apr::pool::createTagged((apr::pool_t *)p, tag));
		}
	}
	if (auto ret = custom::create((custom::Pool *)p)) {
		ret->allocmngr.name = tag;
		return pushPoolInfo((pool_t *)ret);
	}
	return nullptr;
}

void destroy(pool_t *p) {
	popPoolInfo(p);
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(p)) {
			apr::pool::destroy((apr::pool_t *)p);
		} else {
			custom::destroy((custom::Pool *)p);
		}
	} else {
		custom::destroy((custom::Pool *)p);
	}
}

void clear(pool_t *p) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(p)) {
			apr::pool::clear((apr::pool_t *)p);
		} else {
			((custom::Pool *)p)->clear();
		}
	} else {
		((custom::Pool *)p)->clear();
	}
}

void *alloc(pool_t *pool, size_t &size) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::alloc((apr::pool_t *)pool, size);
		}
	}
	return ((custom::Pool *)pool)->alloc(size);
}

void *palloc(pool_t *pool, size_t size) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::palloc((apr::pool_t *)pool, size);
		}
	}
	return ((custom::Pool *)pool)->palloc(size);
}

void *calloc(pool_t *pool, size_t count, size_t eltsize) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::calloc((apr::pool_t *)pool, count, eltsize);
		}
	}
	return ((custom::Pool *)pool)->calloc(count, eltsize);
}

void free(pool_t *pool, void *ptr, size_t size) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			apr::pool::free((apr::pool_t *)pool, ptr, size);
			return;
		}
	}
	((custom::Pool *)pool)->free(ptr, size);
}

void cleanup_kill(pool_t *pool, void *ptr, cleanup_fn cb) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			apr::pool::cleanup_kill((apr::pool_t *)pool, ptr, cb);
			return;
		}
	}
	((custom::Pool *)pool)->cleanup_kill(ptr, (custom::Cleanup::Callback)cb);
}

void cleanup_register(pool_t *pool, void *ptr, cleanup_fn cb) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			apr::pool::cleanup_register((apr::pool_t *)pool, ptr, cb);
			return;
		}
	}
	((custom::Pool *)pool)->cleanup_register(ptr, (custom::Cleanup::Callback)cb);
}

void pre_cleanup_register(pool_t *pool, void *ptr, cleanup_fn cb) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			apr::pool::pre_cleanup_register((apr::pool_t *)pool, ptr, cb);
			return;
		}
	}
	((custom::Pool *)pool)->pre_cleanup_register(ptr, (custom::Cleanup::Callback)cb);
}

status_t userdata_set(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::userdata_set(data, key, cb, (apr::pool_t *)pool);
		}
	}
	return ((custom::Pool *)pool)->userdata_set(data, key, (custom::Cleanup::Callback)cb);
}

status_t userdata_setn(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::userdata_setn(data, key, cb, (apr::pool_t *)pool);
		}
	}
	return ((custom::Pool *)pool)->userdata_setn(data, key, (custom::Cleanup::Callback)cb);
}

status_t userdata_get(void **data, const char *key, pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::userdata_get(data, key, (apr::pool_t *)pool);
		}
	}
	return ((custom::Pool *)pool)->userdata_get(data, key);
}

status_t userdata_get(void **data, const char *key, size_t klen, pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			if (key[klen]) {
				return apr::pool::userdata_get(data, key, (apr::pool_t *)pool);
			} else {
				char buf[klen + 1];
				memcpy(buf, key, klen);
				buf[klen] = 0;
				return apr::pool::userdata_get(data, key, (apr::pool_t *)pool);
			}
		}
	}
	return ((custom::Pool *)pool)->userdata_get(data, key, klen);
}

// debug counters
size_t get_allocated_bytes(pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::get_allocated_bytes((apr::pool_t *)pool);
		}
	}
	return ((custom::Pool *)pool)->allocmngr.allocated;
}

size_t get_return_bytes(pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::get_return_bytes((apr::pool_t *)pool);
		}
	}
	return ((custom::Pool *)pool)->allocmngr.returned;
}

allocator_t *get_allocator(pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return (allocator_t *)apr::pool::get_allocator((apr::pool_t *)pool);
		}
	}
	return (allocator_t *)(((custom::Pool *)pool)->allocator);
}

void *pmemdup(pool_t *pool, const void *m, size_t n) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::pmemdup((apr::pool_t *)pool, m, n);
		}
	}
	return ((custom::Pool *)pool)->pmemdup(m, n);
}

char *pstrdup(pool_t *pool, const char *s) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			return apr::pool::pstrdup((apr::pool_t *)pool, s);
		}
	}
	return ((custom::Pool *)pool)->pstrdup(s);
}

bool isThreadSafeForAllocations(pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (isCustom(pool)) {
			return ((custom::Pool *)pool)->threadSafe;
		}
		return false; // APR pools can not be thread safe for allocations
	} else {
		return ((custom::Pool *)pool)->threadSafe;
	}
}

bool isThreadSafeAsParent(pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (isCustom(pool)) {
			return ((custom::Pool *)pool)->allocator->mutex != nullptr;
		} else {
			return apr::pool::isThreadSafeAsParent((apr::pool_t *)pool);
		}
	} else {
		return ((custom::Pool *)pool)->allocator->mutex != nullptr;
	}
}

const char *get_tag(pool_t *pool) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (isCustom(pool)) {
			return ((custom::Pool *)pool)->allocmngr.name;
		} else {
			return apr::pool::get_tag((apr::pool_t *)pool);
		}
	} else {
		return ((custom::Pool *)pool)->allocmngr.name;
	}
}

void setPoolInfo(pool_t *pool, uint32_t tag, const void *ptr) {
	if constexpr (apr::SP_APR_COMPATIBLE) {
		if (!isCustom(pool)) {
			apr::pool::setPoolInfo((apr::pool_t *)pool, tag, ptr);
			return;
		}
	}

	if (auto mngr = &((custom::Pool *)pool)->allocmngr) {
		if (tag > mngr->tag) {
			mngr->tag = tag;
		}
		mngr->ptr = ptr;
	}
}

static status_t cleanup_register_fn(void *ptr) {
	if (auto fn = (memory::function<void()> *)ptr) {
		(*fn)();
	}
	return 0;
}

void cleanup_register(pool_t *p, memory::function<void()> &&cb) {
	pool::push(p);
	auto fn = new (p) memory::function<void()>(move(cb));
	pool::pop();
	pool::cleanup_register(p, fn, &cleanup_register_fn);
}

void pre_cleanup_register(pool_t *p, memory::function<void()> &&cb) {
	pool::push(p);
	auto fn = new (p) memory::function<void()>(move(cb));
	pool::pop();
	pool::pre_cleanup_register(p, fn, &cleanup_register_fn);
}

size_t get_active_count() {
	return s_activePools.load();
}

bool debug_begin(pool_t *pool) {
	if (!pool) {
		pool = acquire();
	}
	bool expected = false;
	if (s_poolDebug.compare_exchange_strong(expected, true)) {
		s_poolDebugMutex.lock();
		s_poolDebugTarget = pool;
#if DEBUG_BACKTRACE
		if (!s_backtraceState) {
			s_backtraceState = backtrace_create_state(nullptr, 1, debug_backtrace_error, nullptr);
		}
#endif
		s_poolDebugInfo.clear();
		s_poolDebugMutex.unlock();
		return true;
	}
	return false;
}

std::map<pool_t *, const char **, std::less<void>> debug_end() {
	std::map<pool_t *, const char **, std::less<void>> ret;
	s_poolDebugMutex.lock();
	ret = std::move(s_poolDebugInfo);
	s_poolDebugInfo.clear();
	s_poolDebugTarget = nullptr;
	s_poolDebugMutex.unlock();
	s_poolDebug.store(false);
	return ret;
}

void debug_foreach(void *ptr, void(*cb)(void *, pool_t *)) {
#if DEBUG_POOL_LIST
	s_poolDebugMutex.lock();
	for (auto &it : s_poolList) {
		cb(ptr, it);
	}
	s_poolDebugMutex.unlock();
#endif
}

}
