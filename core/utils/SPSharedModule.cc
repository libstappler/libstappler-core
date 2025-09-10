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
#include "SPLog.h"
#include "SPMemInterface.h"
#include "SPStringView.h"
#include <typeinfo>

#if LINUX || ANDROID || MACOS
#include <cxxabi.h>
#endif

namespace STAPPLER_VERSIONIZED stappler {

struct SharedModuleManager {
	static SharedModuleManager *getInstance();

	void addModule(SharedModule *);
	void removeModule(SharedModule *);

	const void *acquireSymbol(const char *module, const char *symbol) const;
	const void *acquireSymbol(const char *module, const char *symbol, const std::type_info *) const;

	void enumerateModules(void *userdata, void (*)(void *userdata, const char *name));

	bool enumerateSymbols(const char *module, void *userdata,
			void (*)(void *userdata, const char *name, const void *symbol));


	std::unordered_map<StringView, SharedModule *> _modules;
	mutable std::mutex _mutex;
};

static void printDemangled(std::ostream &stream, const std::type_info *t) {
#if LINUX || ANDROID || MACOS
	int status = 0;
	auto name = abi::__cxa_demangle(t->name(), nullptr, nullptr, &status);
	if (status == 0) {
		stream << name;
		::free(name);
	} else {
		stream << t->name();
	}
#else
	stream << t->name();
#endif
}

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
	auto it = _modules.find(module->name);
	if (it != _modules.end()) {
		if (hasFlag(it->second->flags, SharedModuleFlags::Extensible)) {
			module->next = it->second;
			it->second = module;
			return;
		} else {
			log::source().error("SharedModule", "Module '", module->name, "' redefined");
			abort();
			return;
		}
	}
	_modules.emplace(module->name, module);
}

void SharedModuleManager::removeModule(SharedModule *module) {
	std::unique_lock lock(_mutex);
	auto it = _modules.find(module->name);
	if (!hasFlag(it->second->flags, SharedModuleFlags::Extensible)) {
		_modules.erase(it);
	} else {
		auto target = &it->second;
		auto mod = it->second;
		while (mod && mod != module) {
			target = &mod->next;
			mod = mod->next;
		}

		if (mod == module) {
			*target = mod->next;
		}

		if (!it->second) {
			_modules.erase(it);
		}
	}
}

const void *SharedModuleManager::acquireSymbol(const char *module, const char *symbol) const {
	std::unique_lock lock(_mutex);
	auto it = _modules.find(StringView(module));
	if (it != _modules.end()) {
		StringView symbolView(symbol);

		SharedSymbol *s = nullptr;
		size_t c = 0;

		auto mod = it->second;
		while (mod) {
			s = mod->symbols;
			c = mod->symbolsCount;
			while (c) {
				if (s->name == symbolView) {
					return s->ptr;
				}
				++s;
				--c;
			}
			mod = mod->next;
		}
	} else {
		log::source().error("SharedModule", "Module \"", module, "\" is not defined");
	}
	return nullptr;
}

const void *SharedModuleManager::acquireSymbol(const char *module, const char *symbol,
		const std::type_info *t) const {
	std::unique_lock lock(_mutex);
	auto it = _modules.find(StringView(module));
	if (it != _modules.end()) {
		StringView symbolView(symbol);
		bool found = false;

		SharedSymbol *s = nullptr;
		size_t c = 0;

		auto mod = it->second;
		while (mod) {
			s = mod->symbols;
			c = mod->symbolsCount;
			while (c) {
				if (s->name == symbolView) {
					if (*s->type == *t) {
						return s->ptr;
					}
					found = true;
				}
				++s;
				--c;
			}
			mod = mod->next;
		}

		if (found) {
			memory::StandartInterface::StringStreamType err;
			err << "Module \"" << module << "\": Symbol \"" << symbol << "\" not found for: '";
			printDemangled(err, t);
			err << "'\n";

			mod = it->second;
			while (mod) {
				s = mod->symbols;
				c = mod->symbolsCount;
				while (c) {
					if (s->name == symbolView) {
						err << "\tFound: '";
						printDemangled(err, s->type);
						err << "'\n";
					}
					++s;
					--c;
				}
				mod = mod->next;
			}
			log::source().error("SharedModule", err.str());
		}
	} else {
		log::source().error("SharedModule", "Module \"", module, "\" is not defined");
	}
	return nullptr;
}

void SharedModuleManager::enumerateModules(void *userdata,
		void (*cb)(void *userdata, const char *name)) {
	if (!cb) {
		return;
	}
	for (auto &it : _modules) { cb(userdata, it.second->name); }
}

bool SharedModuleManager::enumerateSymbols(const char *module, void *userdata,
		void (*cb)(void *userdata, const char *name, const void *symbol)) {
	if (!cb) {
		return false;
	}

	auto it = _modules.find(StringView(module));
	if (it == _modules.end()) {
		return false;
	}

	SharedSymbol *s = nullptr;
	size_t c = 0;

	auto mod = it->second;
	while (mod) {
		s = mod->symbols;
		c = mod->symbolsCount;
		while (c) {
			cb(userdata, s->name, s->ptr);
			++s;
			--c;
		}
		mod = mod->next;
	}

	return true;
}

const void *SharedModule::acquireSymbol(const char *module, const char *symbol) {
	auto manager = SharedModuleManager::getInstance();
	return manager->acquireSymbol(module, symbol);
}
const void *SharedModule::acquireSymbol(const char *module, const char *symbol,
		const std::type_info &info) {
	auto manager = SharedModuleManager::getInstance();
	return manager->acquireSymbol(module, symbol, &info);
}
void SharedModule::enumerateModules(void *userdata, void (*cb)(void *userdata, const char *name)) {
	auto manager = SharedModuleManager::getInstance();
	manager->enumerateModules(userdata, cb);
}

bool SharedModule::enumerateSymbols(const char *module, void *userdata,
		void (*cb)(void *userdata, const char *name, const void *symbol)) {
	auto manager = SharedModuleManager::getInstance();
	return manager->enumerateSymbols(module, userdata, cb);
}

SharedModule::SharedModule(const char *n, SharedSymbol *s, size_t count, SharedModuleFlags f)
: name(n), symbols(s), symbolsCount(count), flags(f) {
	SharedModuleManager::getInstance()->addModule(this);
}

SharedModule::~SharedModule() { SharedModuleManager::getInstance()->removeModule(this); }

} // namespace STAPPLER_VERSIONIZED stappler
