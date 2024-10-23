/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023-2024 Stappler LLC <admin@stappler.dev>

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
#include "SPLog.h"

#if MACOS

#import <Foundation/Foundation.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <mach-o/dyld.h>

namespace STAPPLER_VERSIONIZED stappler::filesystem::platform {

template <>
auto _getApplicationPath<memory::StandartInterface>() -> memory::StandartInterface::StringType {
	char fullpath[PATH_MAX] = {0};
	uint32_t bufsize = PATH_MAX;

	if (_NSGetExecutablePath(fullpath, &bufsize) == 0) {
		return memory::StandartInterface::StringType(fullpath, bufsize);
	}
	return memory::StandartInterface::StringType();
}

template <>
auto _getApplicationPath<memory::PoolInterface>() -> memory::PoolInterface::StringType {
	char fullpath[PATH_MAX] = {0};
	uint32_t bufsize = PATH_MAX;

	if (_NSGetExecutablePath(fullpath, &bufsize) == 0) {
		return memory::PoolInterface::StringType(fullpath, bufsize);
	}
	return memory::PoolInterface::StringType();
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
	bool _isBundle = false;
	bool _isSandboxed = false;

	static PathSource *getInstance() {
		static PathSource *s_paths = nullptr;
		if (!s_paths) {
			s_paths = new PathSource;
		}
		return s_paths;
	}

	PathSource() {
		CFBundleRef bundle = CFBundleGetMainBundle();
		CFURLRef bundleUrl = CFBundleCopyBundleURL(bundle);

		CFStringRef uti;
		CFURLCopyResourcePropertyForKey(bundleUrl, kCFURLTypeIdentifierKey, &uti, NULL);
		if (uti) {
			if ([UTTypeApplicationBundle conformsToType: [UTType typeWithIdentifier:(__bridge NSString *)uti]]) {
				// within app bundle
				_isBundle = true;
			}
		}

		CFRelease(bundleUrl);

		auto environment = [[NSProcessInfo processInfo] environment];
		if ([environment objectForKey:@"APP_SANDBOX_CONTAINER_ID"] != nil) {
			_isSandboxed = true;
		}

		if (!_isBundle) {
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
		} else {
			auto getDirPath = [] (NSSearchPathDirectory dir) -> NSString * {
				auto dirs = NSSearchPathForDirectoriesInDomains(dir, NSUserDomainMask, YES);
				if (dirs && [dirs count] > 0) {
					return [dirs objectAtIndex:0];
				}
				return nil;
			};

			NSString *nsBundlePath = [[NSBundle mainBundle] bundlePath];
			NSString *nsSupportPath = getDirPath(NSApplicationSupportDirectory);
			NSString *nsCachesPath = getDirPath(NSCachesDirectory);
			NSString *nsDocumentsPath = getDirPath(NSDocumentDirectory);

			if (!nsSupportPath || !nsCachesPath || !nsDocumentsPath || !nsBundlePath) {
				::abort();
			}

			_platformPath = nsBundlePath.UTF8String;
			_platformInit = true;
			
			std::string supportPath = nsSupportPath.UTF8String;
			std::string cachesPath = nsCachesPath.UTF8String;
			std::string documentsPath = nsDocumentsPath.UTF8String;
			
			if (!filesystem::exists(supportPath) && !filesystem::mkdir(supportPath)) { log::error("filesystem", "Fail to create dir: ", supportPath); }
			if (!filesystem::exists(cachesPath) && !filesystem::mkdir(cachesPath)) { log::error("filesystem", "Fail to create dir: ", cachesPath); }
			if (!filesystem::exists(documentsPath) && !filesystem::mkdir(documentsPath)) { log::error("filesystem", "Fail to create dir: ", documentsPath); }

			if (!_isSandboxed) {
				auto bundleId = std::string(CFStringGetCStringPtr(CFBundleGetIdentifier(bundle), kCFStringEncodingUTF8));

				supportPath = filepath::merge<memory::StandartInterface>(supportPath, bundleId);
				cachesPath = filepath::merge<memory::StandartInterface>(cachesPath, bundleId);
				documentsPath = filepath::merge<memory::StandartInterface>(documentsPath, bundleId);

				filesystem::mkdir(supportPath);
				filesystem::mkdir(cachesPath);
				filesystem::mkdir(documentsPath);
			}
			
			_writablePath = supportPath;
			_documentsPath = documentsPath; _documentsInit = true;
			_cachePath = cachesPath; _cacheInit = true;
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
			if (!_writableInit) {
				filesystem::mkdir(_writablePath);
				_writableInit = true;
			}
		}
		return _writablePath;
	}
	
	template <typename Interface>
	auto _getPlatformPath(StringView path, bool readOnly) -> typename Interface::StringType {
		if (filepath::isBundled(path)) {
			return filepath::merge<Interface>(getPlatformPath(readOnly), path.sub("%PLATFORM%:"_len));
		}
		return filepath::merge<Interface>(getPlatformPath(readOnly), path);
	}

	std::string getNsPath(StringView path) {
		auto resourceName = filepath::lastComponent(path);
		auto resourceRoot = filepath::root(path);
		
		auto bundle = [NSBundle mainBundle];
		auto nsPath = [bundle pathForResource:[NSString stringWithUTF8String:resourceName.str<memory::StandartInterface>().data()]										   ofType:nil inDirectory: [NSString stringWithUTF8String:resourceRoot.str<memory::StandartInterface>().data()]];
		return nsPath ? std::string(nsPath.UTF8String) : std::string();
	}
	
	bool exists(StringView path) {
		if (_isBundle) {
			return !getNsPath(path).empty();
		} else {
			return ::access(_getPlatformPath<memory::StandartInterface>(path, true).data(), F_OK) != -1;
		}
	}
	
	bool stat(StringView ipath, Stat &stat) {
		if (_isBundle) {
			auto path = getNsPath(ipath);
			return filesystem::native::stat_fn(path, stat);
		} else {
			auto path = _getPlatformPath<memory::StandartInterface>(ipath, false);
			return filesystem::native::stat_fn(path, stat);
		}
	}

	File openForReading(StringView path) {
		if (_isBundle) {
			auto nspath = getNsPath(path);
			return filesystem::openForReading(nspath);
		} else {
			return filesystem::openForReading(_getPlatformPath<memory::StandartInterface>(path, false));
		}
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

static bool checkPlatformPath(StringView &path, bool assetsRoot) {
	if (path.empty() || (!assetsRoot && path.front() == '/') || path.starts_with("..", 2) || path.find("/..") != maxOf<size_t>()) {
		return false;
	}

	while (path.front() == '/') {
		path = path.sub(1);
	}

	if (path.empty()) {
		return false;
	}

	return true;
}

bool _exists(StringView path, bool assetsRoot) {
	if (!checkPlatformPath(path, assetsRoot)) {
		return false;
	}

	return PathSource::getInstance()->exists(path);
}

bool _stat(StringView ipath, Stat &stat, bool assetsRoot) {
	if (!checkPlatformPath(ipath, assetsRoot)) {
		return false;
	}

	return PathSource::getInstance()->stat(ipath, stat);
}

File _openForReading(StringView path) {
	return PathSource::getInstance()->openForReading(path);
}

size_t _read(void *, uint8_t *buf, size_t nbytes) { return 0; }
size_t _seek(void *, int64_t offset, io::Seek s) { return maxOf<size_t>(); }
size_t _tell(void *) { return 0; }
bool _eof(void *) { return true; }
void _close(void *) { }

void _ftw(StringView path, const Callback<void(StringView path, bool isFile)> &cb, int depth, bool dirFirst, bool assetsRoot) {
	if (!checkPlatformPath(path, assetsRoot)) {
		return;
	}
	
	return filesystem::native::ftw_fn(path, cb, depth, dirFirst);
}

bool _ftw_b(StringView path, const Callback<bool(StringView path, bool isFile)> &cb, int depth, bool dirFirst, bool assetsRoot) {
	if (!checkPlatformPath(path, assetsRoot)) {
		return false;
	}

	return filesystem::native::ftw_b_fn(path, cb, depth, dirFirst);
}

}

#endif
