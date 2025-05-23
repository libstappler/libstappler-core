/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

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

#ifndef CORE_CORE_UTILS_SPSHAREDMODULE_H_
#define CORE_CORE_UTILS_SPSHAREDMODULE_H_

#include "SPCore.h"
#include <type_traits>
#include <typeinfo>

namespace STAPPLER_VERSIONIZED stappler {

struct SharedSymbol {
	const char *name = nullptr;
	const void *ptr = nullptr;
	const std::type_info *type = nullptr;

	template <typename T>
	SharedSymbol(const char *n, T *t)
	: name(n), ptr(reinterpret_cast<const void *>(t)), type(&typeid(std::remove_cv_t<T>)) { }

	SharedSymbol(const char *n, const void *p) : name(n), ptr(p) { }
};

class SP_PUBLIC SharedModule final {
public:
	static const void *acquireSymbol(const char *module, const char *symbol);
	static const void *acquireSymbol(const char *module, const char *symbol,
			const std::type_info &);

	template <typename T = const void *>
	static auto acquireTypedSymbol(const char *module, const char *symbol) {
		return reinterpret_cast<T>(const_cast<void *>(acquireSymbol(module, symbol,
				typeid(std::remove_pointer_t<typename std::remove_cv<T>::type>))));
	}

	static void enumerateModules(void *userdata, void (*)(void *userdata, const char *name));

	static bool enumerateSymbols(const char *module, void *userdata,
			void (*)(void *userdata, const char *name, const void *symbol));

	SharedModule(const char *, SharedSymbol *, size_t count);
	~SharedModule();

private:
	friend struct SharedModuleManager;

	const char *name = nullptr;
	SharedSymbol *symbols = nullptr;
	size_t symbolsCount = 0;
};

} // namespace STAPPLER_VERSIONIZED stappler

#endif /* CORE_CORE_UTILS_SPSHAREDMODULE_H_ */
