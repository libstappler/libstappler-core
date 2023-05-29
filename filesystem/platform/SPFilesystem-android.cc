/**
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

#if ANDROID

#include "SPLog.h"
#include "SPZip.h"
#include <android/asset_manager.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace stappler::filesystem::platform {

struct ArchiveFile {
	using String = memory::StandartInterface::StringType;

	String name;
	size_t size;
	Time time;
};

struct ArchiveHierarchy {
	using Interface = memory::StandartInterface;
	using String = memory::StandartInterface::StringType;
	using StringStream = memory::StandartInterface::StringStreamType;

	template <typename K, typename V, typename Compare = std::less<>>
	using Map = memory::StandartInterface::MapType<K, V, Compare>;

	template <typename T>
	using Vector = memory::StandartInterface::VectorType<T>;

	ArchiveHierarchy() = default;
	ArchiveHierarchy(String &&path) : originPath(move(path)) { }

	void add(StringView path, size_t size, Time time) {
		if (path.empty()) {
			return;
		}

		StringStream stream;

		ArchiveHierarchy *target = this;
		StringView tmp;
		filepath::split(path, [&] (StringView path) {
			if (!tmp.empty()) {
				stream << (target->originPath.empty() ? "" : "/") << tmp;
				target = &target->dirs.emplace(tmp.str<Interface>(), ArchiveHierarchy{stream.str()}).first->second;
			}
			tmp = path;
		});
		if (!tmp.empty()) {
			target->files.emplace_back(ArchiveFile{tmp.str<Interface>(), size, time});
		}
	}

	ArchiveHierarchy *getTarget(StringView path) {
		ArchiveHierarchy *target = this;
		filepath::split(path, [&] (StringView path) {
			if (target) {
				auto iit = target->dirs.find(path);
				if (iit != target->dirs.end()) {
					target = &target->dirs.find(path)->second;
				} else {
					target = nullptr;
				}
			}
		});
		return target;
	}

	void ftw(StringView path, const Callback<void(StringView path, bool isFile)> &cb, int depth, bool dirFirst) {
		if (path.empty()) {
			if (dirFirst) {
				cb(originPath, false);
			}

			if (depth < 0 || depth > 0) {
				auto it = dirs.begin();
				while (it != dirs.end()) {
					it->second.ftw(StringView(), cb, depth - 1, dirFirst);
					++ it;
				}

				auto iit = files.begin();
				while (iit != files.end()) {
					if (!originPath.empty()) {
						cb(string::ToStringTraits<Interface>::toString(originPath, "/", iit->name), true);
					} else {
						cb(iit->name, true);
					}
					++ iit;
				}
			}

			if (!dirFirst) {
				cb(originPath, false);
			}
		} else {
			if (auto target = getTarget(path)) {
				target->ftw(StringView(), cb, depth, dirFirst);
			}
		}
	}

	bool ftw_b(StringView path, const Callback<bool(StringView path, bool isFile)> &cb, int depth, bool dirFirst) {
		if (path.empty()) {
			if (dirFirst) {
				if (!cb(originPath, false)) {
					return false;
				}
			}

			if (depth < 0 || depth > 0) {
				auto it = dirs.begin();
				while (it != dirs.end()) {
					if (!it->second.ftw_b(StringView(), cb, depth - 1, dirFirst)) {
						return false;
					}
					++ it;
				}

				auto iit = files.begin();
				while (iit != files.end()) {
					if (!originPath.empty()) {
						if (!cb(string::ToStringTraits<Interface>::toString(originPath, "/", iit->name), true)) {
							return false;
						}
					} else {
						if (!cb(iit->name, true)) {
							return false;
						}
					}
					++ iit;
				}
			}

			if (!dirFirst) {
				if (!cb(originPath, false)) {
					return false;
				}
			}
			return true;
		} else {
			if (auto target = getTarget(path)) {
				return target->ftw_b(StringView(), cb, depth, dirFirst);
			}
		}
		return false;
	}

	void clear() {
		originPath.clear();
		dirs.clear();
		files.clear();
	}

	bool empty() const {
		return dirs.empty() && files.empty();
	}

	bool exists(StringView path) {
		auto target = getTarget(filepath::root(path));
		if (!target) {
			return false;
		}

		auto name = filepath::lastComponent(path);
		do {
			for (auto &it : target->files) {
				if (it.name == name) {
					return true;
				}
			}
		} while (0);

		do {
			auto it = target->dirs.find(name);
			if (it != target->dirs.end()) {
				return true;
			}
		} while (0);

		return false;
	}

	bool stat(StringView path, Stat &stat) {
		auto target = getTarget(filepath::root(path));
		if (!target) {
			return false;
		}

		auto name = filepath::lastComponent(path);
		do {
			for (auto &it : target->files) {
				if (it.name == name) {
					stat.size = it.size;
					stat.mtime = stat.ctime = stat.atime = it.time;
					stat.isDir = false;
					return true;
				}
			}
		} while (0);

		do {
			auto it = target->dirs.find(name);
			if (it != target->dirs.end()) {
				stat.isDir = true;
				return true;
			}
		} while (0);

		return false;
	}

	String originPath;
	Map<String, ArchiveHierarchy> dirs;
	Vector<ArchiveFile> files;
};

struct PathSource {
	using Interface = memory::StandartInterface;
	using String = memory::StandartInterface::StringType;

	String _apkPath;
	String _appPath;
	String _cachePath;
	String _documentsPath;
	String _writablePath;

	ArchiveHierarchy _archive;

	std::mutex _mutex;

	AAssetManager *_assetManager = nullptr;

	bool _cacheInit = false;
	bool _documentsInit = false;

	static PathSource *getInstance() {
		static std::mutex s_mutex;
		static PathSource *s_paths = nullptr;
		std::unique_lock lock(s_mutex);
		if (!s_paths) {
			s_paths = new PathSource;
		}
		return s_paths;
	}

	PathSource() { }

	bool checkApkFile(StringView path) {
		int fd = ::open(path.data(), O_RDONLY);
		if (fd == -1) {
			return false;
		}

		if (auto f = fdopen(fd, "r")) {
			fclose(f);
			close(fd);
			_apkPath = path.str<Interface>();
			return true;
		}

		close(fd);
		return false;
	}

	bool initialize(AAssetManager *assetManager, StringView filesDir, StringView cachesDir, StringView apkPath) {
		_appPath = filesDir.str<Interface>();
		_writablePath = cachesDir.str<Interface>();
		_documentsPath = _writablePath + "/Documents";
		_cachePath = _writablePath + "/Caches";
		_apkPath.clear();

		if (apkPath.empty() || !checkApkFile(apkPath)) {
			char fullpath[PATH_MAX] = "/proc/self/fd/";
			char refpath[PATH_MAX] = {0};
			struct dirent *dp = nullptr;
			auto dir = ::opendir("/proc/self/fd");
			while ((dp = readdir(dir)) != NULL) {
				if (dp->d_name[0] != '.') {
					memcpy(fullpath + "/proc/self/fd/"_len, dp->d_name, strlen(dp->d_name) + 1);
					readlink(fullpath, refpath, PATH_MAX);
					StringView path(refpath);
					if (path.ends_with(".apk") && path.starts_with("/data/")) {
						if (checkApkFile(refpath)) {
							break;
						}
					}
				}
			}
			closedir(dir);
		}

		std::unique_lock lock(_mutex);
		_archive.clear();

		if (!_apkPath.empty()) {
			if (auto f = fopen(_apkPath.data(), "r")) {
				ZipArchive<Interface> a(f, true);
				a.ftw([&] (StringView path, size_t size, Time time) {
					_archive.add(path, size, time);
				});
				fclose(f);
			}
		}

		_assetManager = assetManager;
		_documentsInit = false;
		_cacheInit = false;
		return true;
	}

	void terminate() {
		std::unique_lock lock(_mutex);
		_assetManager = nullptr;
	}

	StringView getApplicationPath() const {
		return _appPath;
	}

	StringView getDocumentsPath(bool readOnly) {
		if (!readOnly) {
			if (!_documentsInit) {
				filesystem::mkdir(_documentsPath);
				_documentsInit = true;
			}
		}
		return _documentsPath;
	}
	StringView getCachePath(bool readOnly) {
		if (!readOnly) {
			if (!_cacheInit) {
				filesystem::mkdir(_cachePath);
				_cacheInit = true;
			}
		}
		return _cachePath;
	}
	StringView getWritablePath(bool readOnly) {
		return _writablePath;
	}

	StringView _getPlatformPath(StringView path) const {
		if (filepath::isBundled(path)) {
			auto tmp = path.sub("%PLATFORM%:"_len);
			while (tmp.is('/')) {
				tmp = tmp.sub(1);
			}
			return tmp;
		}
		return path;
	}

	bool exists(StringView ipath, bool assetsRoot) { // check for existance in platform-specific filesystem
		if (!_assetManager) {
			return false;
		}

		auto path = _getPlatformPath(ipath);
		if (assetsRoot) {
			return _archive.exists(string::ToStringTraits<Interface>::toString("assets/", path));
		} else {
			return _archive.exists(path);
		}
	}

	bool stat(StringView ipath, Stat &stat, bool assetsRoot) {
		if (!_assetManager) {
			return false;
		}

		auto path = _getPlatformPath(ipath);
		if (assetsRoot) {
			return _archive.stat(string::ToStringTraits<Interface>::toString("assets/", path), stat);
		} else {
			return _archive.stat(path, stat);
		}
	}

	File openForReading(StringView ipath) {
		if (!_assetManager) {
			return File();
		}

		auto path = _getPlatformPath(ipath);

		AAsset* aa = AAssetManager_open(_assetManager,
				(path.terminated()?path.data():path.str<memory::StandartInterface>().data()), AASSET_MODE_UNKNOWN);
		if (aa) {
			auto len = AAsset_getLength64(aa);
			return File((void *)aa, len);
		}

		return File();
	}

	void ftw(StringView ipath, const Callback<void(StringView path, bool isFile)> &cb, int depth, bool dirFirst, bool assetsRoot) {
		auto path = _getPlatformPath(ipath);
		if (assetsRoot) {
			_archive.ftw(string::ToStringTraits<Interface>::toString("assets/", path), cb, depth, dirFirst);
		} else {
			_archive.ftw(path, cb, depth, dirFirst);
		}
	}

	bool ftw_b(StringView ipath, const Callback<bool(StringView path, bool isFile)> &cb, int depth, bool dirFirst, bool assetsRoot) {
		auto path = _getPlatformPath(ipath);
		if (assetsRoot) {
			return _archive.ftw_b(string::ToStringTraits<Interface>::toString("assets/", path), cb, depth, dirFirst);
		} else {
			return _archive.ftw_b(path, cb, depth, dirFirst);
		}
	}
};

void Android_initializeFilesystem(void *assetManager, StringView filesDir, StringView cachesDir, StringView apkPath) {
	PathSource::getInstance()->initialize((AAssetManager *)assetManager, filesDir, cachesDir, apkPath);
}

void Android_terminateFilesystem() {
	PathSource::getInstance()->terminate();
}

StringView Android_getApkPath() {
	return PathSource::getInstance()->_apkPath;
}

template <>
auto _getApplicationPath<memory::StandartInterface>() -> memory::StandartInterface::StringType {
	return StringView(PathSource::getInstance()->getApplicationPath()).str<memory::StandartInterface>();
}

template <>
auto _getApplicationPath<memory::PoolInterface>() -> memory::PoolInterface::StringType {
	return StringView(PathSource::getInstance()->getApplicationPath()).str<memory::PoolInterface>();
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

bool _exists(StringView path, bool assetsRoot) { // check for existance in platform-specific filesystem
	if (path.empty() || path.front() == '/' || path.starts_with("..") || path.find("/..") != maxOf<size_t>()) {
		return false;
	}

	return PathSource::getInstance()->exists(path, assetsRoot);
}

bool _stat(StringView path, Stat &stat, bool assetsRoot) {
	return PathSource::getInstance()->stat(path, stat, assetsRoot);
}

File _openForReading(StringView path) {
	return PathSource::getInstance()->openForReading(path);
}

size_t _read(void *aa, uint8_t *buf, size_t nbytes) {
	auto r = AAsset_read((AAsset *)aa, buf, nbytes);
	if (r >= 0) {
		return r;
	}
	return 0;
}

size_t _seek(void *aa, int64_t offset, io::Seek s) {
	int whence = SEEK_SET;
	switch (s) {
	case io::Seek::Set: whence = SEEK_SET; break;
	case io::Seek::Current: whence = SEEK_CUR; break;
	case io::Seek::End: whence = SEEK_END; break;
	}
	if (auto r = AAsset_seek64((AAsset *)aa, off64_t(offset), whence) > 0) {
		return r;
	}
	return maxOf<size_t>();
}

size_t _tell(void *aa) {
	return AAsset_seek64((AAsset *)aa, off64_t(0), int(SEEK_CUR));
}

bool _eof(void *aa) {
	return AAsset_getRemainingLength64((AAsset *)aa) == 0;
}
void _close(void *aa) {
	AAsset_close((AAsset *)aa);
}

void _ftw(StringView path, const Callback<void(StringView path, bool isFile)> &cb, int depth, bool dirFirst, bool assetsRoot) {
	return PathSource::getInstance()->ftw(path, cb, depth, dirFirst, assetsRoot);
}

bool _ftw_b(StringView path, const Callback<bool(StringView path, bool isFile)> &cb, int depth, bool dirFirst, bool assetsRoot) {
	return PathSource::getInstance()->ftw_b(path, cb, depth, dirFirst, assetsRoot);
}

}

#endif
