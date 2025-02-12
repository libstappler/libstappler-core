/**
Copyright (c) 2020-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_CORE_MEMORY_POOL_SPMEMPOOLCONFIG_H_
#define STAPPLER_CORE_MEMORY_POOL_SPMEMPOOLCONFIG_H_

#include "SPCore.h"

namespace STAPPLER_VERSIONIZED stappler::mempool::apr {
#if MODULE_STAPPLER_APR
static constexpr int SP_APR_COMPATIBLE = 1;
#else
static constexpr int SP_APR_COMPATIBLE = 0;
#endif
}

namespace STAPPLER_VERSIONIZED stappler::memory {

template <typename Sig>
class function;

}


namespace STAPPLER_VERSIONIZED stappler::mempool::custom {

using Status = int;

// minimal size of block, that can be reallocated
static constexpr uint32_t BlockThreshold = 256;

// Align on a power of 2 boundary
static constexpr size_t SPALIGN(size_t size, uint32_t boundary) { return math::align<size_t>(size, boundary); }

// Default alignment, 16-bytes is compatible with SSE or other 128-bit SIMD
static constexpr size_t SPALIGN_DEFAULT(size_t size) { return SPALIGN(size, 16); }

static constexpr uint32_t BOUNDARY_INDEX ( 12 );
static constexpr uint32_t BOUNDARY_SIZE ( 1 << BOUNDARY_INDEX );

static constexpr uint32_t MIN_ALLOC (2 * BOUNDARY_SIZE);
static constexpr uint32_t MAX_INDEX ( 20 );
static constexpr uint32_t ALLOCATOR_MAX_FREE_UNLIMITED ( 0 );

// address space (not actual mem) reservation for mmap allocator
// you can not allocate more then this with mmap
static constexpr size_t ALLOCATOR_MMAP_RESERVED = size_t(64_GiB);

static constexpr Status SUCCESS = 0;

// Can be 64-bit or stripped to 32-bit
static constexpr uint64_t POOL_MAGIC = 0xDEAD7fffDEAD7fff;

}

#endif /* STAPPLER_CORE_MEMORY_POOL_SPMEMPOOLCONFIG_H_ */
