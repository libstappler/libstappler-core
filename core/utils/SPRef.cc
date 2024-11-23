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

#include "SPRef.h"
#include "SPLog.h"
#include "SPSubscription.h"
#include "SPTime.h"

#if WIN32
#else
#include <cxxabi.h>
#endif

namespace stappler {

struct RefAllocData {
	void *lastPtr = nullptr;
	std::forward_list<memory::pool_t *> delayedPools;
	std::forward_list<memory::allocator_t *> delayedAllocs;

	void clear() {
		while (!delayedPools.empty()) {
			memory::pool::destroy(delayedPools.front());
			delayedPools.pop_front();
		}

		while (!delayedAllocs.empty()) {
			memory::allocator::destroy(delayedAllocs.front());
			delayedAllocs.pop_front();
		}
	}
};

static thread_local RefAllocData tl_RefAllocData;

void * RefAlloc::operator new (size_t size, memory::pool_t* pool) noexcept {
	SPASSERT(pool, "Context pool should be defined for allocation");

	auto ptr = memory::pool::palloc(pool, size);
	tl_RefAllocData.lastPtr = ptr;
	return ptr;
}

void RefAlloc::operator delete(void *ptr) noexcept {
	if (ptr != tl_RefAllocData.lastPtr) {
		AllocBaseType::operator delete(ptr);
	}
	tl_RefAllocData.lastPtr = nullptr;
	tl_RefAllocData.clear();
}

RefAlloc::~RefAlloc() {
	if ((_referenceCount.load() & PoolAllocBit) != 0) {
		tl_RefAllocData.lastPtr = this;
	}
}

RefAlloc::RefAlloc() noexcept {
	if (tl_RefAllocData.lastPtr == this) {
		_referenceCount.fetch_or(PoolAllocBit);
	}
	tl_RefAllocData.lastPtr = nullptr;
}

void RefAlloc::destroySelfContained(memory::pool_t *pool) {
	tl_RefAllocData.delayedPools.emplace_front(pool);
}

void RefAlloc::destroySelfContained(memory::allocator_t *alloc) {
	tl_RefAllocData.delayedAllocs.emplace_front(alloc);
}

}

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

SPUNUSED static size_t print(char *buf, size_t bufLen, uintptr_t pc, StringView filename, int lineno, StringView function) {
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

}

#if MODULE_STAPPLER_BACKTRACE

#include "backtrace.h"

namespace STAPPLER_VERSIONIZED stappler {

static void debug_backtrace_error(void *data, const char *msg, int errnum) {
	std::cout << "[Backtrace] error: " << msg << "\n";
}

static int debug_backtrace_full_callback(void *data, uintptr_t pc, const char *filename, int lineno, const char *function) {
	if (pc != uintptr_t(0xffffffffffffffffLLU)) {
		auto ret = (const Callback<void(StringView)> *)data;
		char buf[1024] = { 0 };
		auto size = backtrace::print(buf, 1024, pc, filename, lineno, function);
		(*ret)(StringView(buf, size));
	}
	return 0;
}

struct BacktraceState {
	static BacktraceState *getInstance() {
		static std::mutex s_mutex;
		static BacktraceState * s_instance = nullptr;

		s_mutex.lock();
		if (!s_instance) {
			s_instance = new BacktraceState();
		}
		s_mutex.unlock();
		return s_instance;
	}

	BacktraceState() {
		_backtraceState = ::backtrace_create_state(nullptr, 1, debug_backtrace_error, nullptr);
	}

	void getBacktrace(size_t offset, const Callback<void(StringView)> &cb) {
		::backtrace_full(_backtraceState, int(2 + offset), debug_backtrace_full_callback, debug_backtrace_error, (void *)&cb);
	}

	::backtrace_state *_backtraceState;
};

void getBacktrace(size_t offset, const Callback<void(StringView)> &cb) {
	BacktraceState::getInstance()->getBacktrace(offset, cb);
}

}

#elif LINUX

#include <execinfo.h>

namespace STAPPLER_VERSIONIZED stappler {

static constexpr int LinuxBacktraceSize = 128;
static constexpr int LinuxBacktraceOffset = 2;

void getBacktrace(size_t offset, const Callback<void(StringView)> &cb) {
	void *bt[LinuxBacktraceSize + LinuxBacktraceOffset + offset];
	char **bt_syms;
	int bt_size;

	bt_size = ::backtrace(bt, LinuxBacktraceSize + LinuxBacktraceOffset + offset);
	bt_syms = ::backtrace_symbols(bt, bt_size);

	for (int i = LinuxBacktraceOffset + offset; i < bt_size; i++) {
		StringView str(bt_syms[i]);

		auto first = str.find('(');
		auto second = str.rfind('+');

		char buf[1024] = { 0 };
		auto size = backtrace::print(buf, 1024, (uintptr_t) bt[i], StringView(str, first), -1, StringView(str, first + 1, second - first - 1));

		cb(StringView(buf, size));
	}

	::free(bt_syms);
}

}

#elif __APPLE__

#include <libunwind.h>
#include <inttypes.h>

namespace STAPPLER_VERSIONIZED stappler {

void getBacktrace(size_t offset, const Callback<void(StringView)> &cb) {
	unw_cursor_t cursor;
	unw_context_t context;
	unw_getcontext(&context);
	unw_init_local(&cursor, &context);

	char buf[1024] = { 0 };

	while (unw_step(&cursor)) {
		if (offset > 0) {
			-- offset;
			continue;
		}

		unw_word_t ip, sp, off;

		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);

		char symbol[1024] = { "<unknown>" };

		unw_get_proc_name(&cursor, symbol, sizeof(symbol), &off);
		auto size = backtrace::print(buf, 1024, (uintptr_t) off, StringView(), -1, StringView(symbol));

		cb(StringView(buf, size));
	}
}

}

#else

namespace STAPPLER_VERSIONIZED stappler {

void getBacktrace(size_t offset, const Callback<void(StringView)> &cb) { }

}

#endif


namespace STAPPLER_VERSIONIZED stappler::memleak {

static std::mutex s_mutex;
static std::atomic<uint64_t> s_refId = 1;

struct BackraceInfo {
	Time t;
	std::vector<std::string> backtrace;
};

static std::map<const Ref *, std::map<uint64_t, BackraceInfo>> s_retainMap;

uint64_t getNextRefId() {
	return s_refId.fetch_add(1);
}

uint64_t retainBacktrace(const Ref *ptr) {
	auto id = getNextRefId();
	std::vector<std::string> bt;
	getBacktrace(0, [&] (StringView str) {
		bt.emplace_back(str.str<memory::StandartInterface>());
	});
	s_mutex.lock();

	auto it = s_retainMap.find(ptr);
	if (it == s_retainMap.end()) {
		it = s_retainMap.emplace(ptr, std::map<uint64_t, BackraceInfo>()).first;
	}

	auto iit = it->second.find(id);
	if (iit == it->second.end()) {
		it->second.emplace(id, BackraceInfo{Time::now(), move(bt)});
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
		for (auto &iit : it->second) {
			cb(iit.first, iit.second.t, iit.second.backtrace);
		}
	}

	s_mutex.unlock();
}

#if SP_REF_DEBUG

uint64_t Ref::retain() {
	incrementReferenceCount();
	if (isRetainTrackerEnabled()) {
		return memleak::retainBacktrace(this);
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

void Ref::foreachBacktrace(const Callback<void(uint64_t, Time, const std::vector<std::string> &)> &cb) const {
	memleak::foreachBacktrace(this, cb);
}

#endif

}

namespace STAPPLER_VERSIONIZED stappler {

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

}
