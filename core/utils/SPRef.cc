/**
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#include "SPRef.h"
#include "SPLog.h"
#include "SPPlatformInit.h"
#include "SPSubscription.h"
#include "SPTime.h"
#include "SPDso.h"

#ifdef __cpp_lib_stacktrace
#warning Has stacktrace
#include <stacktrace>
#endif

#if WIN32
#else
#include <cxxabi.h>
#endif

namespace stappler {

struct RefAllocData {
	void *lastPtr = nullptr;
	std::forward_list<memory::pool_t *> delayedPools;
	std::forward_list<memory::allocator_t *> delayedAllocs;

	static RefAllocData *get() {
		static thread_local RefAllocData tl_RefAllocData;

		return &tl_RefAllocData;
	}

	~RefAllocData() { clear(); }

	void clear() {
		while (!delayedPools.empty()) {
			memory::pool::destroy(delayedPools.front());
			delayedPools.pop_front();
		}

		delayedPools.clear();

		while (!delayedAllocs.empty()) {
			memory::allocator::destroy(delayedAllocs.front());
			delayedAllocs.pop_front();
		}

		delayedAllocs.clear();
	}
};

void *RefAlloc::operator new(size_t size, memory::pool_t *pool) noexcept {
	SPASSERT(pool, "Context pool should be defined for allocation");

	auto ptr = memory::pool::palloc(pool, size);
	RefAllocData::get()->lastPtr = ptr;
	return ptr;
}

void RefAlloc::operator delete(void *ptr) noexcept {
	auto d = RefAllocData::get();
	if (ptr != d->lastPtr) {
		AllocBaseType::operator delete(ptr);
	}
	d->lastPtr = nullptr;
	d->clear();
}

void RefAlloc::operator delete(void *ptr, std::align_val_t al) noexcept {
	auto d = RefAllocData::get();
	if (ptr != d->lastPtr) {
		AllocBaseType::operator delete(ptr, al);
	}
	d->lastPtr = nullptr;
	d->clear();
}

RefAlloc::~RefAlloc() {
	if ((_referenceCount.load() & PoolAllocBit) != 0) {
		RefAllocData::get()->lastPtr = this;
	}
}

RefAlloc::RefAlloc() noexcept {
	auto d = RefAllocData::get();
	if (d->lastPtr == this) {
		_referenceCount.fetch_or(PoolAllocBit);
	}
	d->lastPtr = nullptr;
}

void RefAlloc::destroySelfContained(memory::pool_t *pool) {
	RefAllocData::get()->delayedPools.emplace_front(pool);
}

void RefAlloc::destroySelfContained(memory::allocator_t *alloc) {
	RefAllocData::get()->delayedAllocs.emplace_front(alloc);
}

} // namespace stappler

namespace STAPPLER_VERSIONIZED stappler::backtrace {

static StringView filepath_lastComponent(StringView path) {
	size_t pos = path.rfind('/');
	if (pos != maxOf<size_t>()) {
		return path.sub(pos + 1);
	} else {
		return path;
	}
}

static StringView filepath_name(StringView path) {
	auto cmp = filepath_lastComponent(path);

	size_t pos = cmp.find('.');
	if (pos == maxOf<size_t>()) {
		return cmp;
	} else {
		return cmp.sub(0, pos);
	}
}

} // namespace stappler::backtrace

// libbacktrace info
// see https://github.com/ianlancetaylor/libbacktrace
namespace stappler::backtrace::detail {

struct backtrace_state;

using backtrace_error_callback = void (*)(void *data, const char *msg, int errnum);

using backtrace_full_callback = int (*)(void *data, uintptr_t pc, const char *filename, int lineno,
		const char *function);

SP_EXTERN_C backtrace_state *backtrace_create_state(const char *filename, int threaded,
		backtrace_error_callback error_callback, void *data);

SP_EXTERN_C int backtrace_full(backtrace_state *state, int skip,
		backtrace_full_callback callback, backtrace_error_callback error_callback, void *data);

SPUNUSED static size_t print(char *buf, size_t bufLen, uintptr_t pc, StringView filename,
		int lineno, StringView function) {
	char *target = buf;
	auto w = ::snprintf(target, bufLen, "[%p]", (void *)pc);
	bufLen -= w;
	target += w;

	if (!filename.empty()) {
		auto name = filepath_name(filename);
		if (lineno >= 0) {
			w = ::snprintf(target, bufLen, " %.*s:%d", int(name.size()), name.data(), lineno);
		} else {
			w = ::snprintf(target, bufLen, " %.*s", int(name.size()), name.data());
		}
		bufLen -= w;
		target += w;
	}

	if (!function.empty()) {
#if WIN32
		w = ::snprintf(target, bufLen, " - %.*s", int(function.size()), function.data());
		bufLen -= w;
		target += w;
#else
		int status = 0;
		auto ptr = abi::__cxa_demangle(function.data(), nullptr, nullptr, &status);
		if (ptr) {
			w = ::snprintf(target, bufLen, " - %s", ptr);
			bufLen -= w;
			target += w;
			::free(ptr);
		} else {
			w = ::snprintf(target, bufLen, " - %.*s", int(function.size()), function.data());
			bufLen -= w;
			target += w;
		}
#endif
	}
	return target - buf;
}

} // namespace stappler::backtrace::detail

namespace STAPPLER_VERSIONIZED stappler {

static void debug_backtrace_error(void *data, const char *msg, int errnum) {
	log::error("Backtrace", msg);
}

static int debug_backtrace_full_callback(void *data, uintptr_t pc, const char *filename, int lineno,
		const char *function) {
	if (pc != uintptr_t(0xffff'ffff'ffff'ffffLLU)) {
		auto ret = (const Callback<void(StringView)> *)data;
		char buf[1'024] = {0};
		auto size = backtrace::detail::print(buf, 1'024, pc, filename, lineno, function);
		(*ret)(StringView(buf, size));
	}
	return 0;
}

struct BacktraceState {
	static BacktraceState *getInstance() {
		static std::mutex s_mutex;
		static BacktraceState *s_instance = nullptr;

		s_mutex.lock();
		if (!s_instance) {
			s_instance = new BacktraceState();
		}
		s_mutex.unlock();
		return s_instance;
	}

	BacktraceState() {
		backtraceState = backtrace::detail::backtrace_create_state(nullptr, 1, debug_backtrace_error, nullptr);
	}

	void getBacktrace(size_t offset, const Callback<void(StringView)> &cb) {
		backtrace::detail::backtrace_full(backtraceState, int(2 + offset), debug_backtrace_full_callback,
				debug_backtrace_error, (void *)&cb);
	}

	backtrace::detail::backtrace_state *backtraceState = nullptr;
};

void getBacktrace(size_t offset, const Callback<void(StringView)> &cb) {
	//auto stacktrace = std::stacktrace::current();

	auto backtraceLib = BacktraceState::getInstance();
	if (backtraceLib->backtraceState) {
		backtraceLib->getBacktrace(offset, cb);
		return;
	}
}

} // namespace STAPPLER_VERSIONIZED stappler


namespace STAPPLER_VERSIONIZED stappler::memleak {

static std::mutex s_mutex;
static std::atomic<uint64_t> s_refId = 1;

struct BackraceInfo {
	Time t;
	std::vector<std::string> backtrace;
};

static std::map<const Ref *, std::map<uint64_t, BackraceInfo>> s_retainMap;

uint64_t getNextRefId() { return s_refId.fetch_add(1); }

uint64_t retainBacktrace(const Ref *ptr, uint64_t id) {
	if (id == maxOf<uint64_t>()) {
		id = getNextRefId();
	}
	std::vector<std::string> bt;
	getBacktrace(0, [&](StringView str) { bt.emplace_back(str.str<memory::StandartInterface>()); });
	s_mutex.lock();

	auto it = s_retainMap.find(ptr);
	if (it == s_retainMap.end()) {
		it = s_retainMap.emplace(ptr, std::map<uint64_t, BackraceInfo>()).first;
	}

	auto iit = it->second.find(id);
	if (iit == it->second.end()) {
		it->second.emplace(id, BackraceInfo{Time::now(), sp::move(bt)});
	}

	s_mutex.unlock();
	return id;
}

void releaseBacktrace(const Ref *ptr, uint64_t id) {
	if (!id) {
		return;
	}

	s_mutex.lock();

	auto it = s_retainMap.find(ptr);
	if (it != s_retainMap.end()) {
		auto iit = it->second.find(id);
		if (iit != it->second.end()) {
			it->second.erase(iit);
		}
		if (it->second.size() == 0) {
			s_retainMap.erase(it);
		}
	}

	s_mutex.unlock();
}

void foreachBacktrace(const Ref *ptr,
		const Callback<void(uint64_t, Time, const std::vector<std::string> &)> &cb) {
	s_mutex.lock();

	auto it = s_retainMap.find(ptr);
	if (it != s_retainMap.end()) {
		for (auto &iit : it->second) { cb(iit.first, iit.second.t, iit.second.backtrace); }
	}

	s_mutex.unlock();
}

} // namespace stappler::memleak

namespace STAPPLER_VERSIONIZED stappler {

#if SP_REF_DEBUG

uint64_t Ref::retain(uint64_t value) {
	incrementReferenceCount();
	if (isRetainTrackerEnabled()) {
		return memleak::retainBacktrace(this, value);
	}
	return 0;
}

void Ref::release(uint64_t v) {
	if (isRetainTrackerEnabled()) {
		memleak::releaseBacktrace(this, v);
	}
	if (decrementReferenceCount()) {
		delete this;
	}
}

void Ref::foreachBacktrace(
		const Callback<void(uint64_t, Time, const std::vector<std::string> &)> &cb) const {
	memleak::foreachBacktrace(this, cb);
}

#endif

template <>
SubscriptionId SubscriptionTemplate<memory::PoolInterface>::getNextId() {
	static std::atomic<SubscriptionId::Type> nextId(0);
	return Id(nextId.fetch_add(1));
}

template <>
SubscriptionId SubscriptionTemplate<memory::StandartInterface>::getNextId() {
	static std::atomic<SubscriptionId::Type> nextId(0);
	return Id(nextId.fetch_add(1));
}

} // namespace STAPPLER_VERSIONIZED stappler
