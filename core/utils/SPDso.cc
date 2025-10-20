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

#include "SPDso.h"
#include "SPMemory.h"

#if LINUX || ANDROID || MACOS

#include <dlfcn.h>

namespace STAPPLER_VERSIONIZED stappler::dso {

static void *dso_open(StringView name, DsoFlags flags, const char **err) {
	void *h = nullptr;
	int f = 0;
	if ((flags & DsoFlags::Lazy) != DsoFlags::None) {
		f |= RTLD_LAZY;
	}
	if ((flags & DsoFlags::Global) != DsoFlags::None) {
		f |= RTLD_GLOBAL;
	}
	if ((flags & DsoFlags::Self) != DsoFlags::None) {
		h = ::dlopen(nullptr, RTLD_LAZY);
	} else {
		h = ::dlopen(name.terminated() ? name.data() : name.str<mem_std::Interface>().data(), f);
	}
	if (!h) {
		*err = ::dlerror();
	}
	return h;
}

static void dso_close(DsoFlags flags, void *handle) {
	if (handle) {
		::dlclose(handle);
	}
}

static void *dso_sym(void *h, StringView name, const char **err) {
	auto s = ::dlsym(h, name.terminated() ? name.data() : name.str<mem_std::Interface>().data());
	if (!s) {
		*err = ::dlerror();
	}
	return s;
}

} // namespace stappler::dso

#endif

#if WIN32

#include "SPPlatformUnistd.h"
#include <libloaderapi.h>

namespace STAPPLER_VERSIONIZED stappler::dso {

static constexpr const char *WIN_FAIL_TO_LOAD = "Fail to load dynamic object";
static constexpr const char *WIN_SYMBOL_NOT_FOUND = "Fail to find symbol in dynamic object";

static void *dso_open(StringView name, DsoFlags flags, const char **err) {
	HMODULE h = NULL;
	if ((flags & DsoFlags::Self) != DsoFlags::None) {
		h = GetModuleHandleA(nullptr);
	} else {
		h = LoadLibraryA(
				LPCSTR(name.terminated() ? name.data() : name.str<mem_std::Interface>().data()));
	}

	if (!h) {
		*err = WIN_FAIL_TO_LOAD;
	}
	return (void *)h;
}

static void dso_close(DsoFlags flags, void *handle) {
	if (handle) {
		if ((flags & DsoFlags::Self) == DsoFlags::None) {
			FreeLibrary(HMODULE(handle));
		}
	}
}

static void *dso_sym(void *h, StringView name, const char **err) {
	auto s = GetProcAddress(HMODULE(h),
			name.terminated() ? name.data() : name.str<mem_std::Interface>().data());
	if (!s) {
		*err = WIN_SYMBOL_NOT_FOUND;
	}
	return (void *)s;
}

} // namespace stappler::dso

#endif

namespace STAPPLER_VERSIONIZED stappler {

static constexpr const char *ERROR_MOVED_OUT = "Object was moved out";
static constexpr const char *ERROR_NOT_LOADED = "Object was not loaded";

static thread_local uint32_t tl_dsoVersion = 0;

uint32_t Dso::GetCurrentVersion() { return tl_dsoVersion; }

Dso::~Dso() {
	if (_handle) {
		close();
	}
}

Dso::Dso() { }

Dso::Dso(StringView name, uint32_t v) : Dso(name, DsoFlags::Lazy, v) { }

Dso::Dso(StringView name, DsoFlags flags, uint32_t v) : _version(v) {
	auto tmp = tl_dsoVersion;
	tl_dsoVersion = _version;
	_handle = dso::dso_open(name, flags, &_error);
	tl_dsoVersion = tmp;
}

Dso::Dso(Dso &&other) {
	_flags = other._flags;
	_handle = other._handle;
	_error = other._error;
	_version = other._version;

	other._flags = DsoFlags::None;
	other._handle = nullptr;
	other._error = ERROR_MOVED_OUT;
	other._version = 0;
}

Dso &Dso::operator=(Dso &&other) {
	if (_handle) {
		close();
	}

	_flags = other._flags;
	_handle = other._handle;
	_error = other._error;
	_version = other._version;

	other._flags = DsoFlags::None;
	other._handle = nullptr;
	other._error = ERROR_MOVED_OUT;
	other._version = 0;
	return *this;
}

void Dso::close() {
	if (_handle) {
		auto tmp = tl_dsoVersion;
		tl_dsoVersion = _version;
		dso::dso_close(_flags, _handle);
		tl_dsoVersion = tmp;
		_handle = nullptr;
		_flags = DsoFlags::None;
	} else {
		_error = ERROR_NOT_LOADED;
	}
}

void *Dso::loadSym(StringView name) {
	if (_handle) {
		auto tmp = tl_dsoVersion;
		tl_dsoVersion = _version;
		auto s = dso::dso_sym(_handle, name, &_error);
		tl_dsoVersion = tmp;
		if (s) {
			_error = nullptr;
			return s;
		}
	} else {
		_error = ERROR_NOT_LOADED;
	}
	return nullptr;
}

} // namespace STAPPLER_VERSIONIZED stappler
