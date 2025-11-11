/**
 Copyright (c) 2025 Stappler Team <admin@stappler.org>

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

#include "SPAbi.h"
#include "SPSharedModule.h"

namespace STAPPLER_VERSIONIZED stappler::abi {

struct JitCompiler {
	using FunctionPtr = void (*)();

	struct MemNode {
		void *self;
		size_t size;
		MemNode *next;
		uint8_t *current;
		size_t remains;

		BytesView allocate(size_t);
	};

	std::mutex mutex;
	MemNode *memory = nullptr;

	static MemNode *allocateMemNode(MemNode *original);

	~JitCompiler();
	JitCompiler();

	JitCompiler(JitCompiler &&) = delete;
	JitCompiler &operator=(JitCompiler &&) = delete;

	JitCompiler(const JitCompiler &) = delete;
	JitCompiler &operator=(const JitCompiler &) = delete;

	template <typename FnPtr = void (*)()>
	auto compileForeignCall(FnPtr targetFunction) -> FnPtr {
		return reinterpret_cast<FnPtr>(
				compileForeignCall(reinterpret_cast<FunctionPtr>(targetFunction)));
	}

	BytesView allocate(size_t);

	FunctionPtr compileForeignCall(FunctionPtr);
};

struct ForeignDso : SharedVirtualObject {
	static constexpr uintptr_t TypeId = 2;

	static ForeignDso *open(void *);
	static void close(ForeignDso *);

	JitCompiler compiler;
	void *handle = nullptr;
};

bool startLinuxLoader(int argc, const char *argv[]);

ForeignDso *openForeign(StringView name, DsoFlags flags, const char **err);
void closeForeign(DsoFlags flags, ForeignDso *handle);
void *symForeign(ForeignDso *h, StringView name, DsoSymFlags flags, const char **err);

void initForeignThread(memory::pool_t *, NotNull<thread::Thread>);
void disposeForeignThread(memory::pool_t *, NotNull<thread::Thread>);

} // namespace stappler::abi
