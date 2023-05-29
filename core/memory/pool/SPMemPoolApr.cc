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
#include "SPMemPoolApi.h"

#if SPAPR
#define SP_APR_EXPORT SP_EXTERN_C
#else
#define SP_APR_EXPORT static
#endif

using apr_status_t = int;
using apr_size_t = size_t;
using apr_abortfunc_t = int (*)(int retcode);

typedef struct apr_allocator_t apr_allocator_t;
typedef struct apr_pool_t apr_pool_t;
typedef struct apr_thread_mutex_t apr_thread_mutex_t;

typedef struct serenity_memaddr_t serenity_memaddr_t;
typedef struct serenity_allocmngr_t serenity_allocmngr_t;

SP_APR_EXPORT apr_status_t apr_allocator_create(apr_allocator_t **allocator) __attribute__((nonnull(1)));
SP_APR_EXPORT void apr_allocator_destroy(apr_allocator_t *allocator) __attribute__((nonnull(1)));
SP_APR_EXPORT void apr_allocator_mutex_set(apr_allocator_t *allocator, apr_thread_mutex_t *mutex) __attribute__((nonnull(1)));
SP_APR_EXPORT void apr_allocator_owner_set(apr_allocator_t *allocator, apr_pool_t *pool) __attribute__((nonnull(1)));
SP_APR_EXPORT apr_pool_t * apr_allocator_owner_get(apr_allocator_t *allocator) __attribute__((nonnull(1)));
SP_APR_EXPORT void apr_allocator_max_free_set(apr_allocator_t *allocator, apr_size_t size) __attribute__((nonnull(1)));

SP_APR_EXPORT void apr_pool_initialize();
SP_APR_EXPORT void apr_pool_terminate();

SP_APR_EXPORT serenity_allocmngr_t *serenity_allocmngr_get(apr_pool_t *);
SP_APR_EXPORT const char *serenity_pool_get_tag(apr_pool_t *);

SP_APR_EXPORT apr_status_t apr_pool_create_unmanaged_ex(apr_pool_t **newpool,
		apr_abortfunc_t abort_fn, apr_allocator_t *allocator) __attribute__((nonnull(1)));

SP_APR_EXPORT apr_status_t apr_pool_create_ex(apr_pool_t **newpool, apr_pool_t *parent,
		apr_abortfunc_t abort_fn, apr_allocator_t *allocator) __attribute__((nonnull(1)));

SP_APR_EXPORT void apr_pool_tag(apr_pool_t *pool, const char *tag) __attribute__((nonnull(1)));
SP_APR_EXPORT void apr_pool_destroy(apr_pool_t *p) __attribute__((nonnull(1)));
SP_APR_EXPORT void apr_pool_clear(apr_pool_t *p) __attribute__((nonnull(1)));
SP_APR_EXPORT void *apr_palloc(apr_pool_t *p, apr_size_t size) __attribute__((alloc_size(2))) __attribute__((nonnull(1)));

SP_APR_EXPORT void apr_pool_cleanup_kill(apr_pool_t *p, const void *data,
		apr_status_t (*cleanup)(void *)) __attribute__((nonnull(3)));

SP_APR_EXPORT apr_status_t apr_pool_cleanup_null(void *data);

SP_APR_EXPORT void apr_pool_cleanup_register( apr_pool_t *p, const void *data,
		apr_status_t (*plain_cleanup)(void *), apr_status_t (*child_cleanup)(void *)) __attribute__((nonnull(3,4)));

SP_APR_EXPORT apr_status_t apr_pool_userdata_set(const void *data, const char *key,
		apr_status_t (*cleanup)(void *), apr_pool_t *pool) __attribute__((nonnull(2,4)));

SP_APR_EXPORT apr_status_t apr_pool_userdata_setn(const void *data, const char *key,
		apr_status_t (*cleanup)(void *), apr_pool_t *pool) __attribute__((nonnull(2,4)));

SP_APR_EXPORT apr_status_t apr_pool_userdata_get(void **data, const char *key, apr_pool_t *pool) __attribute__((nonnull(1,2,3)));

SP_APR_EXPORT apr_allocator_t *apr_pool_allocator_get(apr_pool_t *pool) __attribute__((nonnull(1)));

SP_APR_EXPORT void * apr_pmemdup(apr_pool_t *p, const void *m, apr_size_t n) __attribute__((alloc_size(3)));
SP_APR_EXPORT char * apr_pstrdup(apr_pool_t *p, const char *s);

SP_APR_EXPORT apr_thread_mutex_t *apr_allocator_mutex_get(apr_allocator_t *allocator) __attribute__((nonnull(1)));

namespace stappler::mempool::apr {

using pool_t = apr_pool_t;
using status_t = apr_status_t;
using allocator_t = apr_allocator_t;
using cleanup_fn = status_t(*)(void *);

}

namespace stappler::mempool::apr::allocator {

static allocator_t *create() {
	allocator_t *ret = nullptr;
	apr_allocator_create(&ret);
	return ret;
}

static allocator_t *create(void *mutex) {
	if (!mutex) {
		abort();
	}
	allocator_t *ret = nullptr;
	apr_allocator_create(&ret);
	apr_allocator_mutex_set(ret, (apr_thread_mutex_t *)mutex);
	return ret;
}

static void destroy(allocator_t *alloc) {
	apr_allocator_destroy(alloc);
}
static void owner_set(allocator_t *alloc, pool_t *pool) {
	apr_allocator_owner_set(alloc, pool);
}
static pool_t * owner_get(allocator_t *alloc) {
	return apr_allocator_owner_get(alloc);
}
static void max_free_set(allocator_t *alloc, size_t size) {
	apr_allocator_max_free_set(alloc, size);
}

}


namespace stappler::mempool::apr::pool {

static custom::AllocManager *allocmngr_get(pool_t *pool) {
	return (custom::AllocManager *)serenity_allocmngr_get(pool);
}

static void initialize() {
	apr_pool_initialize();
}

static void terminate() {
	apr_pool_terminate();
}

static pool_t *create() {
	pool_t *ret = nullptr;
	apr_pool_create_unmanaged_ex(&ret, NULL, NULL);
	return ret;
}

static pool_t *create(apr_allocator_t *alloc) {
	pool_t *ret = nullptr;
	apr_pool_create_unmanaged_ex(&ret, NULL, alloc);
	return ret;
}

static pool_t *create(pool_t *p) {
	pool_t *ret = nullptr;
	if (!p) {
		apr_pool_create_ex(&ret, nullptr, nullptr, nullptr);
	} else {
		apr_pool_create_ex(&ret, p, nullptr, nullptr);
	}
	return ret;
}

static pool_t *createTagged(const char *tag) {
	auto ret = create();
	apr_pool_tag(ret, tag);
	return ret;
}

static pool_t *createTagged(pool_t *p, const char *tag) {
	auto ret = create(p);
	apr_pool_tag(ret, tag);
	return ret;
}

static void destroy(pool_t *p) {
	apr_pool_destroy(p);
}

static void clear(pool_t *p) {
	apr_pool_clear(p);
}

static void *alloc(pool_t *p, size_t &size) {
	auto mngr = allocmngr_get(p);
	if (size >= custom::BlockThreshold) {
		return mngr->alloc(size, [] (void *p, size_t s) { return apr_palloc((pool_t *)p, s); });
	} else {
		mngr->increment_alloc(size);
		return apr_palloc(p, size);
	}
}
static void free(pool_t *p, void *ptr, size_t size) {
	if (size >= custom::BlockThreshold) {
		return allocmngr_get(p)->free(ptr, size, [] (void *p, size_t s) { return apr_palloc((pool_t *)p, s); });
	}
}

static void *palloc(pool_t *p, size_t size) {
	return pool::alloc(p, size);
}
static void *calloc(pool_t *p, size_t count, size_t eltsize) {
	size_t s = count * eltsize;
	auto ptr = pool::alloc(p, s);
	memset(ptr, 0, s);
	return ptr;
}

static void cleanup_kill(pool_t *p, void *ptr, status_t(*cb)(void *)) {
	apr_pool_cleanup_kill(p, ptr, cb);
}

struct __CleaupData {
	void *data;
	pool_t *pool;
	status_t(*callback)(void *);

	static status_t doCleanup(void *data) {
		if (auto d = (__CleaupData *)data) {
			memory::pool::push((memory::pool_t *)d->pool);
			d->callback(d->data);
			memory::pool::pop();
		}
		return 0;
	}
};

static void cleanup_register(pool_t *p, void *ptr, status_t(*cb)(void *)) {
	auto data = (__CleaupData *)apr_palloc(p, sizeof(__CleaupData));
	data->data = ptr;
	data->pool = p;
	data->callback = cb;
	apr_pool_cleanup_register(p, data, &__CleaupData::doCleanup, apr_pool_cleanup_null);
}

static status_t userdata_set(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return apr_pool_userdata_set(data, key, cb, pool);
}

static status_t userdata_setn(const void *data, const char *key, cleanup_fn cb, pool_t *pool) {
	return apr_pool_userdata_setn(data, key, cb, pool);
}

static status_t userdata_get(void **data, const char *key, pool_t *pool) {
	return apr_pool_userdata_get(data, key, pool);
}

static size_t get_allocated_bytes(pool_t *p) {
	return allocmngr_get(p)->get_alloc();
}
static size_t get_return_bytes(pool_t *p) {
	return allocmngr_get(p)->get_return();
}

static allocator_t *get_allocator(pool_t *p) {
	return apr_pool_allocator_get(p);
}

static void *pmemdup(pool_t *a, const void *m, size_t n) { return apr_pmemdup(a, m, n); }
static char *pstrdup(pool_t *a, const char *s) { return apr_pstrdup(a, s); }

static void setPoolInfo(pool_t *p, uint32_t tag, const void *ptr) {
	if (auto mngr = allocmngr_get(p)) {
		if (tag > mngr->tag) {
			mngr->tag = tag;
		}
		mngr->ptr = ptr;
	}
}

static bool isThreadSafeAsParent(pool_t *pool) {
	if (auto a = apr_pool_allocator_get(pool)) {
		return apr_allocator_mutex_get(a) != nullptr;
	}
	return false;
}

static const char *get_tag(pool_t *pool) {
	return serenity_pool_get_tag(pool);
}

}

#ifndef SPAPR

SP_APR_EXPORT apr_status_t apr_allocator_create(apr_allocator_t **allocator) { return 0; }
SP_APR_EXPORT void apr_allocator_destroy(apr_allocator_t *allocator) { }
SP_APR_EXPORT void apr_allocator_mutex_set(apr_allocator_t *allocator, apr_thread_mutex_t *mutex) { }
SP_APR_EXPORT void apr_allocator_owner_set(apr_allocator_t *allocator, apr_pool_t *pool) { }
SP_APR_EXPORT apr_pool_t * apr_allocator_owner_get(apr_allocator_t *allocator) { return nullptr; }
SP_APR_EXPORT void apr_allocator_max_free_set(apr_allocator_t *allocator, apr_size_t size) { }

SP_APR_EXPORT void apr_pool_initialize() { }
SP_APR_EXPORT void apr_pool_terminate() { }

SP_APR_EXPORT serenity_allocmngr_t *serenity_allocmngr_get(apr_pool_t *) { return nullptr; }
SP_APR_EXPORT const char *serenity_pool_get_tag(apr_pool_t *) { return nullptr; }

SP_APR_EXPORT apr_status_t apr_pool_create_unmanaged_ex(apr_pool_t **newpool,
		apr_abortfunc_t abort_fn, apr_allocator_t *allocator) { return 0; }

SP_APR_EXPORT apr_status_t apr_pool_create_ex(apr_pool_t **newpool, apr_pool_t *parent,
		apr_abortfunc_t abort_fn, apr_allocator_t *allocator) { return 0; }

SP_APR_EXPORT void apr_pool_tag(apr_pool_t *pool, const char *tag) { }
SP_APR_EXPORT void apr_pool_destroy(apr_pool_t *p) { }
SP_APR_EXPORT void apr_pool_clear(apr_pool_t *p) { }
SP_APR_EXPORT void *apr_palloc(apr_pool_t *p, apr_size_t size) { return nullptr; }

SP_APR_EXPORT void apr_pool_cleanup_kill(apr_pool_t *p, const void *data,
		apr_status_t (*cleanup)(void *)) { }

SP_APR_EXPORT apr_status_t apr_pool_cleanup_null(void *data) { return 0; }

SP_APR_EXPORT void apr_pool_cleanup_register( apr_pool_t *p, const void *data,
		apr_status_t (*plain_cleanup)(void *), apr_status_t (*child_cleanup)(void *)) { }

SP_APR_EXPORT apr_status_t apr_pool_userdata_set(const void *data, const char *key,
		apr_status_t (*cleanup)(void *), apr_pool_t *pool) { return 0; }

SP_APR_EXPORT apr_status_t apr_pool_userdata_setn(const void *data, const char *key,
		apr_status_t (*cleanup)(void *), apr_pool_t *pool) { return 0; }

SP_APR_EXPORT apr_status_t apr_pool_userdata_get(void **data, const char *key, apr_pool_t *pool) { return 0; }

SP_APR_EXPORT apr_allocator_t *apr_pool_allocator_get(apr_pool_t *pool) { return nullptr; }

SP_APR_EXPORT void * apr_pmemdup(apr_pool_t *p, const void *m, apr_size_t n) { return nullptr; }
SP_APR_EXPORT char * apr_pstrdup(apr_pool_t *p, const char *s) { return nullptr; }

SP_APR_EXPORT apr_thread_mutex_t *apr_allocator_mutex_get(apr_allocator_t *allocator) { return nullptr; }

#endif
