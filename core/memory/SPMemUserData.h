/**
Copyright (c) 2019-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_CORE_MEMORY_SPMEMUSERDATA_H_
#define STAPPLER_CORE_MEMORY_SPMEMUSERDATA_H_

#include "SPMemPoolApi.h"
#include "SPStringView.h"
#include "SPCommon.h"

namespace STAPPLER_VERSIONIZED stappler::memory::pool {

void store(pool_t *, void *ptr, const StringView &key, memory::function<void()> && = nullptr);

template <typename T = void>
inline T *get(pool_t *pool, const StringView &key) {
	struct Handle : AllocPool {
		void *pointer;
		memory::function<void()> callback;
	};

	void *ptr = nullptr;
	if (pool::userdata_get(&ptr, key.data(), key.size(), pool) == SUCCESS) {
		if (ptr) {
			return (T *)((Handle *)ptr)->pointer;
		}
	}
	return nullptr;
}

inline void store(void *ptr, const StringView &key, memory::function<void()> &&cb = nullptr) {
	store(acquire(), ptr, key, move(cb));
}

template <typename T = void>
inline T *get(const StringView &key) {
	return get<T>(acquire(), key);
}

}

#endif /* STAPPLER_CORE_MEMORY_SPMEMUSERDATA_H_ */
