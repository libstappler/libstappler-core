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

#ifndef STAPPLER_CORE_MEMORY_POOL_SPMEMPOOLINTERFACE_H_
#define STAPPLER_CORE_MEMORY_POOL_SPMEMPOOLINTERFACE_H_

#include "SPMemPoolConfig.h"

namespace STAPPLER_VERSIONIZED stappler::mempool::base {

// Hide pool implementation details from users
// Note that OpaquePool and OpaqueAllocator are never defined
class OpaquePool;
class OpaqueAllocator;

using pool_t = OpaquePool;
using allocator_t = OpaqueAllocator;

using status_t = custom::Status;

using cleanup_fn = status_t(*)(void *);

using PoolFlags = mempool::custom::PoolFlags;

// use when you need to create pool from application root pool
constexpr pool_t *app_root_pool = nullptr;

SP_PUBLIC size_t get_mapped_regions_count();
SP_PUBLIC void *sp_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
SP_PUBLIC int sp_munmap(void *addr, size_t length);

}


namespace STAPPLER_VERSIONIZED stappler::mempool::base::pool {

SP_PUBLIC pool_t *acquire();
SP_PUBLIC Pair<uint32_t, const void *> info();

SP_PUBLIC void push(pool_t *);
SP_PUBLIC void push(pool_t *, uint32_t, const void * = nullptr);
SP_PUBLIC void pop();

SP_PUBLIC void foreach_info(void *, bool(*)(void *, pool_t *, uint32_t, const void *));

}


namespace STAPPLER_VERSIONIZED stappler::mempool::base::allocator {

SP_PUBLIC allocator_t *create(bool custom = false);
SP_PUBLIC allocator_t *create(void *mutex);

// always custom
SP_PUBLIC allocator_t *createWithMmap(uint32_t initialPages = 0);

SP_PUBLIC void owner_set(allocator_t *alloc, pool_t *pool);
SP_PUBLIC pool_t * owner_get(allocator_t *alloc);
SP_PUBLIC void max_free_set(allocator_t *alloc, size_t size);

SP_PUBLIC void destroy(allocator_t *);

}


namespace STAPPLER_VERSIONIZED stappler::mempool::base::pool {

using PoolFlags = custom::PoolFlags;

SP_PUBLIC void initialize();
SP_PUBLIC void terminate();

// creates unmanaged pool
SP_PUBLIC pool_t *create(PoolFlags = PoolFlags::Default);
SP_PUBLIC pool_t *create(allocator_t *, PoolFlags = PoolFlags::Default);

// creates managed pool (managed by root, if parent in mullptr)
SP_PUBLIC pool_t *create(pool_t *);

// creates unmanaged pool
SP_PUBLIC pool_t *createTagged(const char *, PoolFlags = PoolFlags::Default);

// creates managed pool (managed by root, if parent in mullptr)
SP_PUBLIC pool_t *createTagged(pool_t *, const char *);

SP_PUBLIC void destroy(pool_t *);
SP_PUBLIC void clear(pool_t *);

SP_PUBLIC void *alloc(pool_t *, size_t &);
SP_PUBLIC void *palloc(pool_t *, size_t);
SP_PUBLIC void *calloc(pool_t *, size_t count, size_t eltsize);
SP_PUBLIC void free(pool_t *, void *ptr, size_t size);

SP_PUBLIC void cleanup_kill(pool_t *, void *, cleanup_fn);
SP_PUBLIC void cleanup_register(pool_t *, void *, cleanup_fn);
SP_PUBLIC void cleanup_register(pool_t *p, memory::function<void()> &&cb);
SP_PUBLIC void pre_cleanup_register(pool_t *, void *, cleanup_fn);
SP_PUBLIC void pre_cleanup_register(pool_t *p, memory::function<void()> &&cb);

SP_PUBLIC void foreach_info(void *, bool(*)(void *, pool_t *, uint32_t, const void *));

SP_PUBLIC status_t userdata_set(const void *data, const char *key, cleanup_fn, pool_t *);
SP_PUBLIC status_t userdata_setn(const void *data, const char *key, cleanup_fn, pool_t *);
SP_PUBLIC status_t userdata_get(void **data, const char *key, pool_t *);
SP_PUBLIC status_t userdata_get(void **data, const char *key, size_t, pool_t *);

SP_PUBLIC allocator_t *get_allocator(pool_t *);

SP_PUBLIC void *pmemdup(pool_t *a, const void *m, size_t n);
SP_PUBLIC char *pstrdup(pool_t *a, const char *s);

SP_PUBLIC bool isThreadSafeForAllocations(pool_t *pool);
SP_PUBLIC bool isThreadSafeAsParent(pool_t *pool);

SP_PUBLIC const char *get_tag(pool_t *);

// debug counters
SP_PUBLIC size_t get_allocated_bytes(pool_t *);
SP_PUBLIC size_t get_return_bytes(pool_t *);
SP_PUBLIC size_t get_active_count();

// start recording additional pool info on creation
SP_PUBLIC bool debug_begin(pool_t *pool = nullptr);

// stop recording and return info
SP_PUBLIC std::map<pool_t *, const char **, std::less<void>> debug_end();

SP_PUBLIC void debug_foreach(void *, void(*)(void *, pool_t *));

}

#endif /* STAPPLER_CORE_MEMORY_POOL_SPMEMPOOLINTERFACE_H_ */
