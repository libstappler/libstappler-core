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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <utime.h>
#include "SPFilesystem.h"

#if LINUX
#include <unistd.h>
#endif

#ifndef __MINGW32__

namespace stappler::filesystem::native {

template <>
memory::PoolInterface::StringType nativeToPosix<memory::PoolInterface>(StringView path) {
	return path.str<memory::PoolInterface>();
}

template <>
memory::PoolInterface::StringType posixToNative<memory::PoolInterface>(StringView path) {
	return path.str<memory::PoolInterface>();
}

template <>
memory::PoolInterface::StringType getcwd_fn<memory::PoolInterface>() {
	char cwd[1024] = { 0 };
	if (getcwd(cwd, 1024 - 1) != NULL) {
		return memory::PoolInterface::StringType((const char *)cwd);
	}
	return memory::PoolInterface::StringType();
}

template <>
memory::StandartInterface::StringType nativeToPosix<memory::StandartInterface>(StringView path) {
	return path.str<memory::StandartInterface>();
}

template <>
memory::StandartInterface::StringType posixToNative<memory::StandartInterface>(StringView path) {
	return path.str<memory::StandartInterface>();
}

template <>
memory::StandartInterface::StringType getcwd_fn<memory::StandartInterface>() {
	char cwd[1024] = { 0 };
	if (getcwd(cwd, 1024 - 1) != NULL) {
		return memory::StandartInterface::StringType((const char *)cwd);
	}
	return memory::StandartInterface::StringType();
}

#define SP_TERMINATED_DATA(view) (view.terminated()?view.data():view.str<memory::StandartInterface>().data())

bool remove_fn(StringView path) {
	return ::remove(SP_TERMINATED_DATA(path)) == 0;
}

bool mkdir_fn(StringView path) {
    mode_t process_mask = ::umask(0);
	bool ret = ::mkdir(SP_TERMINATED_DATA(path), (mode_t)(S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) == 0;
    ::umask(process_mask);
    return ret;
}

bool access_fn(StringView path, Access mode) {
	int m = 0;
	switch (mode) {
	case Access::Execute: m = X_OK; break;
	case Access::Exists: m = F_OK; break;
	case Access::Read: m = R_OK; break;
	case Access::Write: m = W_OK; break;
	}
	return ::access(SP_TERMINATED_DATA(path), m) == 0;
}

bool stat_fn(StringView path, Stat &stat) {
	struct stat s;
	if(::stat(SP_TERMINATED_DATA(path), &s) == 0 ) {
		stat.size = size_t(s.st_size);
		stat.isDir = (s.st_mode & S_IFDIR);
#if LINUX || ANDROID
		stat.atime = Time::microseconds(s.st_atime * 1000000 + s.st_atim.tv_nsec / 1000);
		stat.ctime = Time::microseconds(s.st_ctime * 1000000 + s.st_ctim.tv_nsec / 1000);
		stat.mtime = Time::microseconds(s.st_mtime * 1000000 + s.st_mtim.tv_nsec / 1000);
#else
		// some fruit systems just made by assholes
		stat.atime = Time::seconds(s.st_atime);
		stat.ctime = Time::seconds(s.st_ctime);
		stat.mtime = Time::seconds(s.st_mtime);
#endif
		return true;
	} else {
		return false;
	}
}

bool touch_fn(StringView path) {
	return utime(SP_TERMINATED_DATA(path), NULL) == 0;
}

void ftw_fn(StringView path, const Callback<void(StringView path, bool isFile)> &callback, int depth, bool dirFirst) {
	auto dp = opendir(SP_TERMINATED_DATA(path));
	if (dp == NULL) {
		if (access(SP_TERMINATED_DATA(path), F_OK) != -1) {
			callback(path, true);
		}
	} else {
		if (dirFirst) {
			callback(path, false);
		}
		if (depth < 0 || depth > 0) {
			struct dirent *entry;
			while ((entry = readdir(dp))) {
				if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0) {
					auto newPath = filepath::merge<memory::StandartInterface>(path, entry->d_name);
					ftw_fn(newPath, callback, depth - 1, dirFirst);
				}
			}
		}
		if (!dirFirst) {
			callback(path, false);
		}
		closedir(dp);
	}
}
bool ftw_b_fn(StringView path, const Callback<bool(StringView path, bool isFile)> &callback, int depth, bool dirFirst) {
	auto dp = opendir(SP_TERMINATED_DATA(path));
	if (dp == NULL) {
		if (access(SP_TERMINATED_DATA(path), F_OK) != -1) {
			return callback(path, true);
		}
	} else {
		if (dirFirst) {
			if (!callback(path, false)) {
				closedir(dp);
				return false;
			}
		}
		if (depth < 0 || depth > 0) {
			struct dirent *entry;
			while ((entry = readdir(dp))) {
				if (strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".") != 0) {
					auto newPath = filepath::merge<memory::StandartInterface>(path, entry->d_name);
					if (!ftw_b_fn(newPath, callback, depth - 1, dirFirst)) {
						closedir(dp);
						return false;
					}
				}
			}
		}
		if (!dirFirst) {
			if (!callback(path, false)) {
				closedir(dp);
				return false;
			}
		}
		closedir(dp);
	}
	return true;
}

bool rename_fn(StringView source, StringView dest) {
	return rename(SP_TERMINATED_DATA(source), SP_TERMINATED_DATA(dest)) == 0;
}

FILE *fopen_fn(StringView path, StringView mode) {
	return fopen(SP_TERMINATED_DATA(path), SP_TERMINATED_DATA(mode));
}

#undef SP_TERMINATED_DATA

}

#endif
