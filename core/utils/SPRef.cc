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
#define SP_HAS_LIBBACKTRACE 0
#include "SPPlatformUnistd.h" // IWYU pragma: keep
#include <dbghelp.h>
#else
#define SP_HAS_LIBBACKTRACE 1
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


namespace STAPPLER_VERSIONIZED stappler::backtrace::detail {

static StringView filepath_lastComponent(StringView path) {
#if WIN32
	size_t pos = path.rfind('\\');
#else
	size_t pos = path.rfind('/');
#endif
	if (pos != maxOf<size_t>()) {
		return path.sub(pos + 1);
	} else {
		return path;
	}
}

SPUNUSED static size_t print(char *buf, size_t bufLen, uintptr_t pc, StringView filename,
		int lineno, StringView function) {
	char *target = buf;
	auto w = ::snprintf(target, bufLen, "[%p]", (void *)pc);
	bufLen -= w;
	target += w;

	if (!filename.empty()) {
		auto name = filepath_lastComponent(filename);
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

#if SP_HAS_LIBBACKTRACE

// libbacktrace info
// see https://github.com/ianlancetaylor/libbacktrace
namespace STAPPLER_VERSIONIZED stappler::backtrace::detail {

struct backtrace_state;

struct State {
	backtrace_state *state = nullptr;

	operator bool() const { return state != nullptr; }
};

using backtrace_error_callback = void (*)(void *data, const char *msg, int errnum);

using backtrace_full_callback = int (*)(void *data, uintptr_t pc, const char *filename, int lineno,
		const char *function);

SP_EXTERN_C backtrace_state *backtrace_create_state(const char *filename, int threaded,
		backtrace_error_callback error_callback, void *data);

SP_EXTERN_C void backtrace_free_state(struct backtrace_state *state,
		backtrace_error_callback error_callback, void *data);

SP_EXTERN_C int backtrace_full(backtrace_state *state, int skip, backtrace_full_callback callback,
		backtrace_error_callback error_callback, void *data);

static void debug_backtrace_error(void *data, const char *msg, int errnum) {
	log::source().error("Backtrace", msg);
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

static void initState(State &state) {
	state.state = backtrace_create_state(nullptr, 1, debug_backtrace_error, nullptr);
}

static void termState(State &state) {
	if (state.state) {
		backtrace_free_state(state.state, debug_backtrace_error, nullptr);
		state.state = nullptr;
	}
}

static void performBacktrace(State &state, size_t offset, const Callback<void(StringView)> &cb) {
	backtrace_full(state.state, int(offset), debug_backtrace_full_callback, debug_backtrace_error,
			(void *)&cb);
}

} // namespace stappler::backtrace::detail

#elif WIN32

namespace STAPPLER_VERSIONIZED stappler::backtrace::detail {

struct State {
	Dso handle;
	HANDLE hProcess = nullptr;

	decltype(&::SymSetOptions) SymSetOptions = nullptr;
	decltype(&::SymInitialize) SymInitialize = nullptr;
	decltype(&::SymCleanup) SymCleanup = nullptr;
	decltype(&::StackWalk64) StackWalk64 = nullptr;
	decltype(&::SymGetSymFromAddr64) SymGetSymFromAddr64 = nullptr;
	decltype(&::SymGetLineFromAddr64) SymGetLineFromAddr64 = nullptr;

	std::mutex mutex;

	operator bool() const { return hProcess != nullptr; }
};

struct StackFrameSym {
	IMAGEHLP_LINE64 line;
	IMAGEHLP_SYMBOL64 sym;
	char symNameBuffer[1_KiB];
	char targetNameBuffer[1_KiB];

	StackFrameSym() {
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		sym.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64) + 1_KiB;
		sym.MaxNameLength = 1_KiB;
	}
};

static void initState(State &state) {
	HANDLE hCurrentProcess;
	HANDLE hProcess;

	auto handle = Dso("Dbghelp.dll");
	if (!handle) {
		return;
	}

	state.handle = sp::move(handle);
	state.SymSetOptions = state.handle.sym<decltype(&::SymSetOptions)>("SymSetOptions");
	state.SymInitialize = state.handle.sym<decltype(&::SymInitialize)>("SymInitialize");
	state.SymCleanup = state.handle.sym<decltype(&::SymCleanup)>("SymCleanup");
	state.StackWalk64 = state.handle.sym<decltype(&::StackWalk64)>("StackWalk64");
	state.SymGetSymFromAddr64 =
			state.handle.sym<decltype(&::SymGetSymFromAddr64)>("SymGetSymFromAddr64");
	state.SymGetLineFromAddr64 =
			state.handle.sym<decltype(&::SymGetLineFromAddr64)>("SymGetLineFromAddr64");

	if (!state.SymSetOptions || !state.SymInitialize || !state.SymCleanup || !state.StackWalk64
			|| !state.SymGetSymFromAddr64 || !state.SymGetLineFromAddr64) {
		state.handle.close();
		return;
	}

	state.SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);

	hCurrentProcess = GetCurrentProcess();

	if (!DuplicateHandle(hCurrentProcess, hCurrentProcess, hCurrentProcess, &hProcess, 0, FALSE,
				DUPLICATE_SAME_ACCESS)) {
		log::source().error("Ref", "Fail to duplicate process handle");
		return;
	}

	if (!state.SymInitialize(hProcess, NULL, TRUE)) {
		log::source().error("Ref", "Fail to load symbol info");
		return;
	}

	state.hProcess = hProcess;
}

static void termState(State &state) {
	if (state.hProcess) {
		state.SymCleanup(state.hProcess);
		CloseHandle(state.hProcess);
		state.hProcess = nullptr;
	}
	state.handle.close();
}

static void performBacktrace(State &state, size_t offset, const Callback<void(StringView)> &cb) {
	auto hThread = GetCurrentThread();

	DWORD machine = 0;
	CONTEXT context;
	STACKFRAME64 frame;
	RtlCaptureContext(&context);
	/*Prepare stackframe for the first StackWalk64 call*/
	frame.AddrFrame.Mode = frame.AddrPC.Mode = frame.AddrStack.Mode = AddrModeFlat;
#if (defined _M_IX86)
	machine = IMAGE_FILE_MACHINE_I386;
	frame.AddrFrame.Offset = context.Ebp;
	frame.AddrPC.Offset = context.Eip;
	frame.AddrStack.Offset = context.Esp;
#elif (defined _M_X64)
	machine = IMAGE_FILE_MACHINE_AMD64;
	frame.AddrFrame.Offset = context.Rbp;
	frame.AddrPC.Offset = context.Rip;
	frame.AddrStack.Offset = context.Rsp;
#else
#pragma error("unsupported architecture")
#endif

	std::unique_lock lock(state.mutex);

	DWORD dwDisplacement;
	StackFrameSym stackSym;

	while (state.StackWalk64(machine, state.hProcess, hThread, &frame, &context, 0, 0, 0, 0)) {
		if (offset > 0) {
			--offset;
			continue;
		}

		BOOL hasSym = FALSE;
		BOOL hasLine = FALSE;

		hasSym = state.SymGetSymFromAddr64(state.hProcess, frame.AddrPC.Offset, nullptr,
				&stackSym.sym);
		hasLine = state.SymGetLineFromAddr64(state.hProcess, frame.AddrPC.Offset, &dwDisplacement,
				&stackSym.line);

		auto size = backtrace::detail::print(stackSym.targetNameBuffer, 1_KiB, frame.AddrPC.Offset,
				hasLine ? stackSym.line.FileName : nullptr, hasLine ? stackSym.line.LineNumber : 0,
				hasSym ? stackSym.sym.Name : nullptr);
		cb(StringView(stackSym.targetNameBuffer, size));
	}
}

} // namespace stappler::backtrace::detail

#endif

namespace STAPPLER_VERSIONIZED stappler {

struct BacktraceState {
	static void initialize(void *ptr) { reinterpret_cast<BacktraceState *>(ptr)->init(); }
	static void terminate(void *ptr) { reinterpret_cast<BacktraceState *>(ptr)->term(); }

	BacktraceState() { addInitializer(this, initialize, terminate); }

	void init() { backtrace::detail::initState(state); }

	void term() { termState(state); }

	void getBacktrace(size_t offset, const Callback<void(StringView)> &cb) {
		if (state) {
			performBacktrace(state, offset + 3, cb);
		}
	}

	backtrace::detail::State state;
};

static BacktraceState s_backtraceState;

void getBacktrace(size_t offset, const Callback<void(StringView)> &cb) {
	s_backtraceState.getBacktrace(offset, cb);
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
	getBacktrace(1, [&](StringView str) { bt.emplace_back(str.str<memory::StandartInterface>()); });
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
