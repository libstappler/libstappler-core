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

#include "SPMemPoolStruct.h"

namespace STAPPLER_VERSIONIZED stappler::mempool::base {

using pool_t = custom::Pool;
using status_t = custom::Status;
using allocator_t = custom::Allocator;

using cleanup_fn = status_t(*)(void *);

using PoolFlags = mempool::custom::PoolFlags;

// use when you need to create pool from application root pool
constexpr pool_t *app_root_pool = nullptr;

size_t get_mapped_regions_count();
void *sp_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int sp_munmap(void *addr, size_t length);

}


namespace STAPPLER_VERSIONIZED stappler::mempool::base::pool {

pool_t *acquire();
Pair<uint32_t, const void *> info();

void push(pool_t *);
void push(pool_t *, uint32_t, const void * = nullptr);
void pop();

void foreach_info(void *, bool(*)(void *, pool_t *, uint32_t, const void *));

}


namespace STAPPLER_VERSIONIZED stappler::mempool::base::allocator {

allocator_t *create(bool custom = false);
allocator_t *create(void *mutex);

// always custom
allocator_t *createWithMmap(uint32_t initialPages = 0);

void owner_set(allocator_t *alloc, pool_t *pool);
pool_t * owner_get(allocator_t *alloc);
void max_free_set(allocator_t *alloc, size_t size);

void destroy(allocator_t *);

}


namespace STAPPLER_VERSIONIZED stappler::mempool::base::pool {

using PoolFlags = custom::PoolFlags;

void initialize();
void terminate();

// creates unmanaged pool
pool_t *create(PoolFlags = PoolFlags::Default);
pool_t *create(allocator_t *, PoolFlags = PoolFlags::Default);

// creates managed pool (managed by root, if parent in mullptr)
pool_t *create(pool_t *);

// creates unmanaged pool
pool_t *createTagged(const char *, PoolFlags = PoolFlags::Default);

// creates managed pool (managed by root, if parent in mullptr)
pool_t *createTagged(pool_t *, const char *);

void destroy(pool_t *);
void clear(pool_t *);

void *alloc(pool_t *, size_t &);
void *palloc(pool_t *, size_t);
void *calloc(pool_t *, size_t count, size_t eltsize);
void free(pool_t *, void *ptr, size_t size);

void cleanup_kill(pool_t *, void *, cleanup_fn);
void cleanup_register(pool_t *, void *, cleanup_fn);
void cleanup_register(pool_t *p, memory::function<void()> &&cb);
void pre_cleanup_register(pool_t *, void *, cleanup_fn);
void pre_cleanup_register(pool_t *p, memory::function<void()> &&cb);

void foreach_info(void *, bool(*)(void *, pool_t *, uint32_t, const void *));

status_t userdata_set(const void *data, const char *key, cleanup_fn, pool_t *);
status_t userdata_setn(const void *data, const char *key, cleanup_fn, pool_t *);
status_t userdata_get(void **data, const char *key, pool_t *);
status_t userdata_get(void **data, const char *key, size_t, pool_t *);

allocator_t *get_allocator(pool_t *);

void *pmemdup(pool_t *a, const void *m, size_t n);
char *pstrdup(pool_t *a, const char *s);

bool isThreadSafeForAllocations(pool_t *pool);
bool isThreadSafeAsParent(pool_t *pool);

const char *get_tag(pool_t *);

// debug counters
size_t get_allocated_bytes(pool_t *);
size_t get_return_bytes(pool_t *);
size_t get_active_count();

// start recording additional pool info on creation
bool debug_begin(pool_t *pool = nullptr);

// stop recording and return info
std::map<pool_t *, const char **, std::less<void>> debug_end();

void debug_foreach(void *, void(*)(void *, pool_t *));

}

#endif /* STAPPLER_CORE_MEMORY_POOL_SPMEMPOOLINTERFACE_H_ */
