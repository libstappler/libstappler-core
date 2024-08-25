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

#include "SPSharedModule.h"
#include "SPStringView.h"

namespace STAPPLER_VERSIONIZED stappler {

struct SharedModuleManager {
	static SharedModuleManager *getInstance();

	void addModule(SharedModule *);
	void removeModule(SharedModule *);

	void *acquireSymbol(const char *module, const char *symbol) const;

	std::unordered_map<StringView, SharedModule *> _modules;
	mutable std::mutex _mutex;
};

SharedModuleManager *SharedModuleManager::getInstance() {
	static std::mutex s_mutex;
	static SharedModuleManager *s_instance = nullptr;

	std::unique_lock lock(s_mutex);
	if (!s_instance) {
		s_instance = new SharedModuleManager();
	}
	return s_instance;
}

void SharedModuleManager::addModule(SharedModule *module) {
	std::unique_lock lock(_mutex);
	_modules.emplace(module->name, module);
}

void SharedModuleManager::removeModule(SharedModule *module) {
	std::unique_lock lock(_mutex);
	_modules.erase(module->name);
}

void *SharedModuleManager::acquireSymbol(const char *module, const char *symbol) const {
	std::unique_lock lock(_mutex);
	auto it = _modules.find(StringView(module));
	if (it != _modules.end()) {
		StringView symbolView(symbol);
		auto s = it->second->symbols;
		auto c = it->second->symbolsCount;
		while (c) {
			if (s->name == symbolView) {
				return s->ptr;
			}
			++ s;
			-- c;
		}
	}
	return nullptr;
}

void *SharedModule::acquireSymbol(const char *module, const char *symbol) {
	auto manager = SharedModuleManager::getInstance();
	return manager->acquireSymbol(module, symbol);
}

SharedModule::SharedModule(const char *n, SharedSymbol *s, size_t count)
: name(n), symbols(s), symbolsCount(count) {
	SharedModuleManager::getInstance()->addModule(this);
}

SharedModule::~SharedModule() {
	SharedModuleManager::getInstance()->removeModule(this);
}

}
