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

#include "SPMemPoolStruct.h"

namespace STAPPLER_VERSIONIZED stappler::mempool::custom {

static std::atomic<size_t> s_nAllocators = 0;

size_t Allocator::getAllocatorsCount() {
	return s_nAllocators.load();
}

Allocator::Allocator() {
	++ s_nAllocators;
	buf.fill(nullptr);
	mutex = new AllocMutex;
}

Allocator::~Allocator() {
	MemNode *node, **ref;

	if (mutex) {
		delete mutex;
	}

	for (uint32_t index = 0; index < MAX_INDEX; index++) {
		ref = &buf[index];
		while ((node = *ref) != nullptr) {
			*ref = node->next;
			allocated -= node->endp - (uint8_t *)node;
			::free(node);
		}
	}

	-- s_nAllocators;
}

void Allocator::set_max(size_t size) {
	std::unique_lock<Allocator> lock(*this);

	uint32_t max_free_index = uint32_t(SPALIGN(size, BOUNDARY_SIZE) >> BOUNDARY_INDEX);
	current += max_free_index;
	current -= max;
	max = max_free_index;
	if (current > max) {
		current = max;
	}
}

MemNode *Allocator::alloc(size_t in_size) {
	std::unique_lock<Allocator> lock;

	size_t size = uint32_t(SPALIGN(in_size + SIZEOF_MEMNODE, BOUNDARY_SIZE));
	if (size < in_size) {
		return nullptr;
	}
	if (size < MIN_ALLOC) {
		size = MIN_ALLOC;
	}

	uint32_t index = uint32_t(size >> BOUNDARY_INDEX) - 1;
	if (index > maxOf<uint32_t>()) {
		return nullptr;
	}

	MemNode *node = nullptr;
	MemNode **ref = nullptr;

	/* First see if there are any nodes in the area we know
	 * our node will fit into.
	 */
	lock = std::unique_lock<Allocator>(*this);
	if (index <= last) {
		/* Walk the free list to see if there are
		 * any nodes on it of the requested size
		 */
		uint32_t max_index = last;
		ref = &buf[index];
		uint32_t i = index;
		while (*ref == nullptr && i < max_index) {
			ref++;
			i++;
		}

		if ((node = *ref) != nullptr) {
			/* If we have found a node and it doesn't have any
			 * nodes waiting in line behind it _and_ we are on
			 * the highest available index, find the new highest
			 * available index
			 */
			if ((*ref = node->next) == nullptr && i >= max_index) {
				do {
					ref--;
					max_index--;
				}
				while (*ref == NULL && max_index > 0);

				last = max_index;
			}

			current += node->index + 1;
			if (current > max) {
				current = max;
			}

			node->next = nullptr;
			node->first_avail = (uint8_t *)node + SIZEOF_MEMNODE;

			return node;
		}
	} else if (buf[0]) {
		/* If we found nothing, seek the sink (at index 0), if
		 * it is not empty.
		 */

		/* Walk the free list to see if there are
		 * any nodes on it of the requested size
		 */
		ref = &buf[0];
		while ((node = *ref) != nullptr && index > node->index) {
			ref = &node->next;
		}

		if (node) {
			*ref = node->next;

			current += node->index + 1;
			if (current > max) {
				current = max;
			}

			node->next = nullptr;
			node->first_avail = (uint8_t *)node + SIZEOF_MEMNODE;

			return node;
		}
	}

	/* If we haven't got a suitable node, malloc a new one
	 * and initialize it.
	 */
	node = nullptr;

	if (lock.owns_lock()) {
		lock.unlock();
	}

	if ((node = (MemNode *)malloc(size)) == nullptr) {
		return nullptr;
	}

	allocated += size;

	node->next = nullptr;
	node->index = (uint32_t)index;
	node->first_avail = (uint8_t *)node + SIZEOF_MEMNODE;
	node->endp = (uint8_t *)node + size;

	return node;
}

void Allocator::free(MemNode *node) {
	MemNode *next, *freelist = nullptr;

	std::unique_lock<Allocator> lock(*this);

	uint32_t max_index = last;
	uint32_t max_free_index = max;
	uint32_t current_free_index = current;

	/* Walk the list of submitted nodes and free them one by one,
	 * shoving them in the right 'size' buckets as we go.
	 */
	do {
		next = node->next;
		uint32_t index = node->index;

		if (max_free_index != ALLOCATOR_MAX_FREE_UNLIMITED && index + 1 > current_free_index) {
			node->next = freelist;
			freelist = node;
		} else if (index < MAX_INDEX) {
			/* Add the node to the appropiate 'size' bucket.  Adjust
			 * the max_index when appropiate.
			 */
			if ((node->next = buf[index]) == nullptr && index > max_index) {
				max_index = index;
			}
			buf[index] = node;
			if (current_free_index >= index + 1) {
				current_free_index -= index + 1;
			} else {
				current_free_index = 0;
			}
		} else {
			/* This node is too large to keep in a specific size bucket,
			 * just add it to the sink (at index 0).
			 */
			node->next = buf[0];
			buf[0] = node;
			if (current_free_index >= index + 1) {
				current_free_index -= index + 1;
			} else {
				current_free_index = 0;
			}
		}
	} while ((node = next) != nullptr);

#if DEBUG
	int i = 0;
	auto n = buf[1];
	while (n && i < 1024 * 16) {
		n = n->next;
		++ i;
	}

	if (i >= 1024 * 128) {
		printf("ERRER: pool double-free detected!\n");
		abort();
	}
#endif

	last = max_index;
	current = current_free_index;

	if (lock.owns_lock()) {
		lock.unlock();
	}

	while (freelist != NULL) {
		node = freelist;
		freelist = node->next;
		allocated -= node->endp - (uint8_t *)node;
		::free(node);
	}
}

void Allocator::lock() {
	if (mutex) {
		mutex->lock();
	}
}

void Allocator::unlock() {
	if (mutex) {
		mutex->unlock();
	}
}

}
