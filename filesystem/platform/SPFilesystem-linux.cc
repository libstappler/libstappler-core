/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
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

#include "SPFilesystem.h"

#if LINUX

#include <limits.h>

#if LINUX
#include <unistd.h>
#endif

namespace STAPPLER_VERSIONIZED stappler::filesystem::platform {

template <>
auto _getApplicationPath<memory::StandartInterface>() -> memory::StandartInterface::StringType {
	char fullpath[PATH_MAX] = {0};
	ssize_t length = ::readlink("/proc/self/exe", fullpath, sizeof(fullpath)-1);
	return memory::StandartInterface::StringType(fullpath, length);
}

template <>
auto _getApplicationPath<memory::PoolInterface>() -> memory::PoolInterface::StringType {
	char fullpath[PATH_MAX] = {0};
	ssize_t length = ::readlink("/proc/self/exe", fullpath, sizeof(fullpath)-1);
	return memory::PoolInterface::StringType(fullpath, length);
}

struct PathSource {
	memory::StandartInterface::StringType _appPath;
	memory::StandartInterface::StringType _platformPath;
	memory::StandartInterface::StringType _cachePath;
	memory::StandartInterface::StringType _documentsPath;
	memory::StandartInterface::StringType _writablePath;

	bool _platformInit = false;
	bool _cacheInit = false;
	bool _documentsInit = false;
	bool _writableInit = false;

	static PathSource *getInstance() {
		static PathSource *s_paths = nullptr;
		if (!s_paths) {
			s_paths = new PathSource;
		}
		return s_paths;
	}

	PathSource() {
		_appPath = _getApplicationPath<memory::StandartInterface>();
		if (!_appPath.empty()) {
			_writablePath = _platformPath = _appPath.substr(0, _appPath.find_last_of("/")) + "/AppData";
			_documentsPath = _platformPath + "/Documents";
			_cachePath = _platformPath + "/Caches";
		}

		auto newWD = ::getenv("SP_CWD_OVERRIDE");
		if (newWD && ::strlen(newWD) != 0) {
			if (filesystem::native::access_fn(newWD, Access::Exists)) {
				::chdir(newWD);
			}
		}
	}

	StringView getPlatformPath(bool readOnly) {
		if (!readOnly) {
			if (!_platformInit) {
				filesystem::mkdir(_platformPath);
				_platformInit = true;
			}
		}
		return _platformPath;
	}
	StringView getDocumentsPath(bool readOnly) {
		if (!readOnly) {
			if (!_platformInit) {
				filesystem::mkdir(_platformPath);
				_platformInit = true;
			}
			if (!_documentsInit) {
				filesystem::mkdir(_documentsPath);
				_documentsInit = true;
			}
		}
		return _documentsPath;
	}
	StringView getCachePath(bool readOnly) {
		if (!readOnly) {
			if (!_platformInit) {
				filesystem::mkdir(_platformPath);
				_platformInit = true;
			}
			if (!_cacheInit) {
				filesystem::mkdir(_cachePath);
				_cacheInit = true;
			}
		}
		return _cachePath;
	}
	StringView getWritablePath(bool readOnly) {
		if (!readOnly) {
			if (!_platformInit) {
				filesystem::mkdir(_writablePath);
				_platformInit = true;
			}
		}
		return _writablePath;
	}
};

template <typename Interface>
auto _getPlatformPath(StringView path, bool readOnly) -> typename Interface::StringType {
	if (filepath::isBundled(path)) {
		return filepath::merge<Interface>(PathSource::getInstance()->getPlatformPath(readOnly), path.sub("%PLATFORM%:"_len));
	}
	return filepath::merge<Interface>(PathSource::getInstance()->getPlatformPath(readOnly), path);
}

template <>
auto _getWritablePath<memory::StandartInterface>(bool readOnly) -> typename memory::StandartInterface::StringType {
	return PathSource::getInstance()->getWritablePath(readOnly).str<memory::StandartInterface>();
}

template <>
auto _getWritablePath<memory::PoolInterface>(bool readOnly) -> typename memory::PoolInterface::StringType {
	return StringView(PathSource::getInstance()->getWritablePath(readOnly)).str<memory::PoolInterface>();
}

template <>
auto _getDocumentsPath<memory::StandartInterface>(bool readOnly) -> typename memory::StandartInterface::StringType {
	return PathSource::getInstance()->getDocumentsPath(readOnly).str<memory::StandartInterface>();
}

template <>
auto _getDocumentsPath<memory::PoolInterface>(bool readOnly) -> typename memory::PoolInterface::StringType {
	return StringView(PathSource::getInstance()->getDocumentsPath(readOnly)).str<memory::PoolInterface>();
}

template <>
auto _getCachesPath<memory::StandartInterface>(bool readOnly) -> typename memory::StandartInterface::StringType {
	return PathSource::getInstance()->getCachePath(readOnly).str<memory::StandartInterface>();
}

template <>
auto _getCachesPath<memory::PoolInterface>(bool readOnly) -> typename memory::PoolInterface::StringType {
	return StringView(PathSource::getInstance()->getCachePath(readOnly)).str<memory::PoolInterface>();
}

bool _exists(StringView path, bool) {
	if (path.empty() || path.front() == '/' || path.starts_with("..", 2) || path.find("/..") != maxOf<size_t>()) {
		return false;
	}

	return ::access(_getPlatformPath<memory::StandartInterface>(path, false).data(), F_OK) != -1;
}

bool _stat(StringView ipath, Stat &stat, bool) {
	auto path = _getPlatformPath<memory::StandartInterface>(ipath, false);
	return filesystem::native::stat_fn(path, stat);
}

File _openForReading(StringView path) {
	return filesystem::openForReading(_getPlatformPath<memory::StandartInterface>(path, false));
}

size_t _read(void *, uint8_t *buf, size_t nbytes) { return 0; }
size_t _seek(void *, int64_t offset, io::Seek s) { return maxOf<size_t>(); }
size_t _tell(void *) { return 0; }
bool _eof(void *) { return true; }
void _close(void *) { }

void _ftw(StringView path, const Callback<void(StringView path, bool isFile)> &cb, int depth, bool dirFirst, bool assetsRoot) {
	return filesystem::native::ftw_fn(path, cb, depth, dirFirst);
}

bool _ftw_b(StringView path, const Callback<bool(StringView path, bool isFile)> &cb, int depth, bool dirFirst, bool assetsRoot) {
	return filesystem::native::ftw_b_fn(path, cb, depth, dirFirst);
}

}

#endif
