/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#include "SPString.h"
#include "SPFilesystem.h"

#include "SPPlatformUnistd.h"

#ifdef WIN32

namespace stappler::filesystem::native {

static void nativeToPosix_c(char *path, size_t size) {
	if (size >= 2) {
		char first = path[0];
		char second = path[1];

		if ((('a' <= first && first <= 'z') || ('A' <= first && first <= 'Z')) && second == ':') {
			path[0] = '/';
			path[1] = first;
		}
	}

	for (size_t i = 0; i < size; ++ i) {
		if (path[i] == '\\') {
			path[i] = '/';
		}
	}
}

static void posixToNative_c(char *path, size_t size) {
	if (path[0] == '/' && size >= 2) {
		path[0] = path[1];
		path[1] = ':';
	}

	for (size_t i = 0; i < size; ++ i) {
		if (path[i] == '/') {
			path[i] = '\\';
		}
	}
}

template <>
memory::PoolInterface::StringType nativeToPosix<memory::PoolInterface>(StringView ipath) {
	memory::PoolInterface::StringType path(ipath.str<memory::PoolInterface>());
	nativeToPosix_c(path.data(), path.size());
	return path;
}

template <>
memory::StandartInterface::StringType nativeToPosix<memory::StandartInterface>(StringView ipath) {
	memory::StandartInterface::StringType path(ipath.str<memory::StandartInterface>());
	nativeToPosix_c(path.data(), path.size());
	return path;
}

template <>
memory::PoolInterface::StringType posixToNative<memory::PoolInterface>(StringView ipath) {
	memory::PoolInterface::StringType path(ipath.str<memory::PoolInterface>());
	posixToNative_c(path.data(), path.size());
	return path;
}

template <>
memory::StandartInterface::StringType posixToNative<memory::StandartInterface>(StringView ipath) {
	memory::StandartInterface::StringType path(ipath.str<memory::StandartInterface>());
	posixToNative_c(path.data(), path.size());
	return path;
}

template <>
memory::PoolInterface::StringType getcwd_fn<memory::PoolInterface>() {
	wchar_t cwd[1024] = { 0 };
	if (_wgetcwd(cwd, 1024 - 1) != NULL) {
		return nativeToPosix<memory::PoolInterface>(string::toUtf8<memory::PoolInterface>((const char16_t *)cwd));
	}
	return memory::PoolInterface::StringType();
}

template <>
memory::StandartInterface::StringType getcwd_fn<memory::StandartInterface>() {
	wchar_t cwd[1024] = { 0 };
	if (_wgetcwd(cwd, 1024 - 1) != NULL) {
		return nativeToPosix<memory::StandartInterface>(string::toUtf8<memory::StandartInterface>((const char16_t *)cwd));
	}
	return memory::StandartInterface::StringType();
}

bool remove_fn(StringView path) {
	memory::StandartInterface::WideStringType str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));
	return _wremove((wchar_t *)str.c_str()) == 0;
}

bool unlink_fn(StringView path) {
	memory::StandartInterface::WideStringType str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));
	return _wunlink((wchar_t *)str.c_str()) == 0;
}

bool mkdir_fn(StringView path) {
	memory::StandartInterface::WideStringType str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));
	int oldmask, tmp;
	_umask_s(0, &oldmask);
	bool ret = _wmkdir((wchar_t *)str.c_str()) == 0;
	_umask_s(oldmask, &tmp);
    return ret;
}

bool access_fn(StringView path, Access mode) {
	// https://learn.microsoft.com/ru-ru/cpp/c-runtime-library/reference/access-waccess?view=msvc-170
	int m = 0;
	switch (mode) {
	case Access::Execute: m = 00; break;
	case Access::Exists: m = 00; break;
	case Access::Read: m = 02; break;
	case Access::Write: m = 04; break;
	}
	memory::StandartInterface::WideStringType str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));
	return _waccess((wchar_t *)str.c_str(), m) == 0;
}

bool isdir_fn(StringView path) {
	memory::StandartInterface::WideStringType str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));
	struct _stat64 s;
	if( _wstat64((wchar_t *)str.c_str(), &s) == 0 ) {
		return s.st_mode & S_IFDIR;
	} else {
		return false;
	}
}

bool stat_fn(StringView path, Stat &stat) {
	auto str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));

	struct _stat64 s;
	if( _wstat64((wchar_t *)str.c_str(), &s) == 0 ) {
		stat.size = size_t(s.st_size);
		stat.isDir = (s.st_mode & S_IFDIR);
		stat.atime = Time::seconds(s.st_atime);
		stat.ctime = Time::seconds(s.st_ctime);
		stat.mtime = Time::seconds(s.st_mtime);
		return true;
    }
	return false;
}

bool touch_fn(StringView path) {
	auto str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));
	return _wutime((wchar_t *)str.c_str(), NULL) == 0;
}

void ftw_fn(StringView path, const Callback<void(StringView path, bool isFile)> &callback, int depth, bool dirFirst) {
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	auto str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));
	hFind = FindFirstFile((const wchar_t *)str.c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		if (_waccess((const wchar_t *)str.c_str(), Access::Exists) != -1) {
			callback(path, true);
		}
	} else {
		if (dirFirst) {
			callback(path, false);
		}
		if (depth < 0 || depth > 0) {
			do {
				auto dname = string::toUtf8<memory::StandartInterface>((const char16_t *)&ffd.cFileName[0]);
				if (dname != ".." && dname != ".") {
					memory::StandartInterface::StringType newPath = filepath::merge<memory::StandartInterface>(path, dname);
					ftw_fn(newPath, callback, depth - 1, dirFirst);
				}
			} while (FindNextFile(hFind, &ffd) != 0);
		}
		if (!dirFirst) {
			callback(path, false);
		}
		FindClose(hFind);
	}
}

bool ftw_b_fn(StringView path, const Callback<bool(StringView path, bool isFile)> &callback, int depth, bool dirFirst) {
	WIN32_FIND_DATAW ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	auto str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));
	hFind = FindFirstFile((const wchar_t *)str.c_str(), &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		if (_waccess((const wchar_t *)str.c_str(), Access::Exists) != -1) {
			return callback(path, true);
		}
	} else {
		if (dirFirst) {
			if (!callback(path, false)) {
				FindClose(hFind);
				return false;
			}
		}
		if (depth < 0 || depth > 0) {
			do {
				auto dname = string::toUtf8<memory::StandartInterface>((const char16_t *)&ffd.cFileName[0]);
				if (dname != ".." && dname != ".") {
					memory::StandartInterface::StringType newPath = filepath::merge<memory::StandartInterface>(path, dname);
					if (!ftw_b_fn(newPath, callback, depth - 1, dirFirst)) {
						FindClose(hFind);
						return false;
					}
				}
			} while (FindNextFile(hFind, &ffd) != 0);
		}
		FindClose(hFind);
		if (!dirFirst) {
			return callback(path, false);
		}
	}
	return true;
}

bool rename_fn(StringView source, StringView dest) {
	memory::StandartInterface::WideStringType wsource = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(source));
	memory::StandartInterface::WideStringType wdest = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(dest));
	return _wrename((const wchar_t *)wsource.c_str(), (const wchar_t *)wdest.c_str()) == 0;
}

FILE *fopen_fn(StringView path, StringView mode) {
	FILE *ret = nullptr;
	memory::StandartInterface::WideStringType str = string::toUtf16<memory::StandartInterface>(posixToNative<memory::StandartInterface>(path));
	memory::StandartInterface::WideStringType wmode = string::toUtf16<memory::StandartInterface>(mode);
	_wfopen_s(&ret, (const wchar_t *)str.c_str(), (const wchar_t *)wmode.c_str());
	return ret;
}

bool write_fn(StringView ipath, const unsigned char *data, size_t len) {
	auto path = posixToNative<memory::StandartInterface>(ipath);
	std::ofstream f(path.data());
	if (f.is_open()) {
		f.write((const char *)data, len);
		f.close();
		return true;
	}
	return false;
}

}

#endif
