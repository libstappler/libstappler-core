/**
Copyright (c) 2023-2025 Stappler LLC <admin@stappler.dev>

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

#include "SPCore.h"
#include "SPFilepath.h"
#include "SPFilesystem.h"

#if ANDROID

#include "detail/SPFilesystemResourceData.h"

#include "SPLog.h"
#include "SPZip.h"
#include "SPJni.h"
#include <android/asset_manager.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

namespace STAPPLER_VERSIONIZED stappler::filesystem::platform {

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
		filepath::split(path, [&](StringView path) {
			if (!tmp.empty()) {
				stream << (target->originPath.empty() ? "" : "/") << tmp;
				target = &target->dirs.emplace(tmp.str<Interface>(), ArchiveHierarchy{stream.str()})
								  .first->second;
			}
			tmp = path;
		});
		if (!tmp.empty()) {
			target->files.emplace_back(ArchiveFile{tmp.str<Interface>(), size, time});
		}
	}

	ArchiveHierarchy *getTarget(StringView path) {
		ArchiveHierarchy *target = this;
		filepath::split(path, [&](StringView path) {
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

	Status ftw(StringView path, const Callback<bool(StringView path, FileType type)> &cb, int depth,
			bool dirFirst) {
		if (path.empty()) {
			if (dirFirst) {
				if (!cb(originPath, FileType::Dir)) {
					return Status::Suspended;
				}
			}

			if (depth < 0 || depth > 0) {
				auto it = dirs.begin();
				while (it != dirs.end()) {
					if (it->second.ftw(StringView(), cb, depth - 1, dirFirst) != Status::Ok) {
						return Status::Suspended;
					}
					++it;
				}

				auto iit = files.begin();
				while (iit != files.end()) {
					if (!originPath.empty()) {
						if (!cb(string::toString<Interface>(originPath, "/", iit->name),
									FileType::File)) {
							return Status::Suspended;
						}
					} else {
						if (!cb(iit->name, FileType::File)) {
							return Status::Suspended;
						}
					}
					++iit;
				}
			}

			if (!dirFirst) {
				if (!cb(originPath, FileType::Dir)) {
					return Status::Suspended;
				}
			}
			return Status::Ok;
		} else {
			if (auto target = getTarget(path)) {
				return target->ftw(StringView(), cb, depth, dirFirst);
			}
		}
		return Status::Declined;
	}

	void clear() {
		originPath.clear();
		dirs.clear();
		files.clear();
	}

	bool empty() const { return dirs.empty() && files.empty(); }

	bool access(StringView path, Access a) {
		if (hasFlag(a, Access::Execute) || hasFlag(a, Access::Write)) {
			return false;
		}

		auto target = getTarget(filepath::root(path));
		if (!target) {
			return hasFlag(a, Access::Empty);
		}

		auto name = filepath::lastComponent(path);
		do {
			for (auto &it : target->files) {
				if (it.name == name) {
					if (hasFlag(a, Access::Empty)) {
						return false;
					}
					return hasFlag(a, Access::Exists) || hasFlag(a, Access::Read);
				}
			}
		} while (0);

		do {
			auto it = target->dirs.find(name);
			if (it != target->dirs.end()) {
				if (hasFlag(a, Access::Empty) || hasFlag(a, Access::Read)) {
					return false;
				}
				return hasFlag(a, Access::Exists);
			}
		} while (0);

		return hasFlag(a, Access::Empty);
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
					stat.type = FileType::File;
					return true;
				}
			}
		} while (0);

		do {
			auto it = target->dirs.find(name);
			if (it != target->dirs.end()) {
				stat.type = FileType::Dir;
				return true;
			}
		} while (0);

		return false;
	}

	String originPath;
	Map<String, ArchiveHierarchy> dirs;
	Vector<ArchiveFile> files;
};

static constexpr auto MEDIA_MOUNTED = "mounted";
static constexpr auto MEDIA_MOUNTED_READ_ONLY = "mounted_ro";

struct PathSource {
	using Interface = memory::StandartInterface;
	using String = memory::StandartInterface::StringType;

	String _apkPath;
	String _filesDir;
	String _cacheDir;
	String _externalFilesDir;
	String _externalCacheDir;

	ArchiveHierarchy _archive;

	jni::Global _context = nullptr;
	jmethodID _externalFilesDirMethod = nullptr;
	jmethodID _getAbsolutePathMethod = nullptr;

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

	Access getExternalStorageState() {
		auto env = jni::Env::getEnv();
		auto envClass = env.findClass("android/os/Environment");
		if (envClass) {
			auto getExternalStorageStateMethod =
					envClass.getStaticMethodID("getExternalStorageState", "()Ljava/lang/String;");
			auto str = envClass.callStaticMethod<jstring>(getExternalStorageStateMethod);
			if (str) {
				auto v = str.getString();
				if (v == MEDIA_MOUNTED) {
					return Access::Read | Access::Write | Access::Exists;
				} else if (v == MEDIA_MOUNTED_READ_ONLY) {
					return Access::Read | Access::Exists;
				}
			}
		}
		return Access::None;
	}

	bool initialize(AAssetManager *assetManager, const jni::Ref &ctx, StringView apkPath) {
		jni::Env env(ctx.getEnv());
		_context = ctx.getGlobal();

		auto contextClass = ctx.getClass();

		auto fileClass = env.findClass("java/io/File");
		_getAbsolutePathMethod = fileClass.getMethodID("getAbsolutePath", "()Ljava/lang/String;");

		auto filesDirMethod = contextClass.getMethodID("getFilesDir", "()Ljava/io/File;");
		auto cacheDirMethod = contextClass.getMethodID("getCacheDir", "()Ljava/io/File;");
		auto externalFilesDirMethod = contextClass.getMethodID("getExternalFilesDir",
				"(Ljava/lang/String;)Ljava/io/File;");
		auto externalCacheDirMethod =
				contextClass.getMethodID("getExternalCacheDir", "()Ljava/io/File;");

		auto filesDir = _context.callMethod<jobject>(filesDirMethod);
		auto cacheDir = _context.callMethod<jobject>(cacheDirMethod);
		auto externalFilesDir = _context.callMethod<jobject>(externalFilesDirMethod, nullptr);
		auto externalCacheDir = _context.callMethod<jobject>(externalCacheDirMethod);

		_externalFilesDirMethod = externalFilesDirMethod;

		if (filesDir) {
			auto str = filesDir.callMethod<jstring>(_getAbsolutePathMethod);
			if (str) {
				_filesDir = str.getString().str<memory::StandartInterface>();
			}
		}

		if (cacheDir) {
			auto str = cacheDir.callMethod<jstring>(_getAbsolutePathMethod);
			if (str) {
				_cacheDir = str.getString().str<memory::StandartInterface>();
			}
		}

		if (externalFilesDir) {
			auto str = externalFilesDir.callMethod<jstring>(_getAbsolutePathMethod);
			if (str) {
				_externalFilesDir = str.getString().str<memory::StandartInterface>();
			}
		}

		if (externalCacheDir) {
			auto str = externalCacheDir.callMethod<jstring>(_getAbsolutePathMethod);
			if (str) {
				_externalCacheDir = str.getString().str<memory::StandartInterface>();
			}
		}

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
				a.ftw([&](StringView path, size_t size, Time time) {
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
		_context = nullptr;
		_assetManager = nullptr;
	}

	void initSystemPaths(FilesystemResourceData &data) {
		auto &resBundled = data._resourceLocations[toInt(FileCategory::Bundled)];
		resBundled.paths.emplace_back(_apkPath, FileFlags::None);
		resBundled.init = false;
		resBundled.flags = CategoryFlags::PlatformSpecific;

		auto externalState = getExternalStorageState();
		auto externalFlags = CategoryFlags::Locateable | CategoryFlags::Removable;
		auto externalFileFlags = FileFlags::Public;

		if (hasFlag(externalState, Access::Write)) {
			externalFileFlags |= FileFlags::Writable;
		}

		if (hasFlag(externalState, Access::Read)) {
			if (!_externalFilesDir.empty()) {
				auto configPath =
						filepath::merge<memory::StandartInterface>(_externalFilesDir, "config");
				auto &resConfig = data._resourceLocations[toInt(FileCategory::AppConfig)];
				resConfig.paths.emplace_back(StringView(configPath).pdup(data._pool),
						externalFileFlags);
				resConfig.init = false;
				resConfig.flags = externalFlags;

				auto dataPath =
						filepath::merge<memory::StandartInterface>(_externalFilesDir, "data");
				auto &resData = data._resourceLocations[toInt(FileCategory::AppData)];
				resData.paths.emplace_back(StringView(dataPath).pdup(data._pool),
						externalFileFlags);
				resData.init = false;
				resData.flags = externalFlags;

				auto statePath =
						filepath::merge<memory::StandartInterface>(_externalFilesDir, "state");
				auto &resState = data._resourceLocations[toInt(FileCategory::AppState)];
				resState.paths.emplace_back(StringView(statePath).pdup(data._pool),
						externalFileFlags);
				resState.init = false;
				resState.flags = externalFlags;
			}

			if (!_externalCacheDir.empty()) {
				auto cachePath =
						filepath::merge<memory::StandartInterface>(_externalCacheDir, "cache");
				auto &resCache = data._resourceLocations[toInt(FileCategory::AppCache)];
				resCache.paths.emplace_back(StringView(cachePath).pdup(data._pool),
						externalFileFlags);
				resCache.init = false;
				resCache.flags = externalFlags;

				auto runtimePath =
						filepath::merge<memory::StandartInterface>(_externalCacheDir, "runtime");
				auto &resRuntime = data._resourceLocations[toInt(FileCategory::CommonRuntime)];
				resRuntime.paths.emplace_back(StringView(runtimePath).pdup(data._pool),
						externalFileFlags);
				resRuntime.init = false;
				resRuntime.flags = externalFlags;
			}
		}

		if (!_filesDir.empty()) {
			auto &resConfig = data._resourceLocations[toInt(FileCategory::AppConfig)];
			resConfig.paths.emplace_back(
					StringView(filepath::merge<memory::StandartInterface>(_filesDir, "config"))
							.pdup(data._pool),
					FileFlags::Writable | FileFlags::Private);
			resConfig.init = false;
			resConfig.flags = CategoryFlags::Locateable;

			auto &resData = data._resourceLocations[toInt(FileCategory::AppData)];
			resData.paths.emplace_back(
					StringView(filepath::merge<memory::StandartInterface>(_filesDir, "data"))
							.pdup(data._pool),
					FileFlags::Writable | FileFlags::Private);
			resData.init = false;
			resData.flags = CategoryFlags::Locateable;

			auto &resState = data._resourceLocations[toInt(FileCategory::AppData)];
			resState.paths.emplace_back(
					StringView(filepath::merge<memory::StandartInterface>(_filesDir, "state"))
							.pdup(data._pool),
					FileFlags::Writable | FileFlags::Private);
			resState.init = false;
			resState.flags = CategoryFlags::Locateable;
		}

		if (!_cacheDir.empty()) {
			auto &resCache = data._resourceLocations[toInt(FileCategory::AppCache)];
			resCache.paths.emplace_back(
					StringView(filepath::merge<memory::StandartInterface>(_cacheDir, "cache"))
							.pdup(data._pool),
					FileFlags::Writable | FileFlags::Private);
			resCache.init = false;
			resCache.flags = CategoryFlags::Locateable;

			auto &resRuntime = data._resourceLocations[toInt(FileCategory::AppRuntime)];
			resRuntime.paths.emplace_back(
					StringView(filepath::merge<memory::StandartInterface>(_cacheDir, "runtime"))
							.pdup(data._pool),
					FileFlags::Writable | FileFlags::Private);
			resRuntime.init = false;
			resRuntime.flags = CategoryFlags::Locateable;
		}

		if (!hasFlag(externalState, Access::Read)) {
			return;
		}
		auto env = jni::Env::getEnv();
		auto envClass = env.findClass("android/os/Environment");
		if (envClass) {
			auto getExternalStorageDirectoryMethod =
					envClass.getStaticMethodID("getExternalStorageDirectory", "()Ljava/io/File;");
			auto getExternalStoragePublicDirectoryMethod = envClass.getStaticMethodID(
					"getExternalStoragePublicDirectory", "(Ljava/lang/String;)Ljava/io/File;");
			auto storageDir = envClass.callStaticMethod<jobject>(getExternalStorageDirectoryMethod);
			if (storageDir) {
				auto path = storageDir.callMethod<jstring>(_getAbsolutePathMethod);
				if (path) {
					auto &res = data._resourceLocations[toInt(FileCategory::UserHome)];
					res.paths.emplace_back(path.getString().pdup(data._pool), FileFlags::Shared);
					res.init = false;
					res.flags = externalFlags;
				}
			}

			auto DIRECTORY_DOWNLOADS = envClass.getStaticField<jstring>("DIRECTORY_DOWNLOADS");
			auto DIRECTORY_DOCUMENTS = envClass.getStaticField<jstring>("DIRECTORY_DOCUMENTS");
			auto DIRECTORY_MUSIC = envClass.getStaticField<jstring>("DIRECTORY_MUSIC");
			auto DIRECTORY_PICTURES = envClass.getStaticField<jstring>("DIRECTORY_PICTURES");
			auto DIRECTORY_MOVIES = envClass.getStaticField<jstring>("DIRECTORY_MOVIES");

			auto context = jni::Ref(_context.get(), env);

			auto updatePath = [&](jni::LocalString &str, FileCategory cat) {
				auto &res = data._resourceLocations[toInt(cat)];
				auto _public = context.callMethod<jobject>(_externalFilesDirMethod, str);
				if (_public) {
					auto path = _public.callMethod<jstring>(_getAbsolutePathMethod);
					if (path) {
						res.paths.emplace_back(path.getString().pdup(data._pool),
								FileFlags::Public);
					}
				}

				auto shared = envClass.callStaticMethod<jobject>(
						getExternalStoragePublicDirectoryMethod, str);
				if (shared) {
					auto path = shared.callMethod<jstring>(_getAbsolutePathMethod);
					if (path) {
						res.paths.emplace_back(path.getString().pdup(data._pool),
								FileFlags::Shared);
					}
				}

				if (!res.paths.empty()) {
					res.init = false;
					res.flags = externalFlags;
				}
			};

			updatePath(DIRECTORY_PICTURES, FileCategory::UserPictures);
			updatePath(DIRECTORY_MUSIC, FileCategory::UserMusic);
			updatePath(DIRECTORY_DOCUMENTS, FileCategory::UserDocuments);
			updatePath(DIRECTORY_DOWNLOADS, FileCategory::UserDownload);
			updatePath(DIRECTORY_MOVIES, FileCategory::UserVideos);
		}
	}

	StringView getApplicationPath() const { return _apkPath; }

	StringView _getPlatformPath(StringView path) const {
		if (filepath::isBundled(path)) {
			auto tmp = path.sub("%PLATFORM%:"_len);
			while (tmp.is('/')) { tmp = tmp.sub(1); }
			return tmp;
		}
		return path;
	}

	// check for existance in platform-specific filesystem
	bool access(FileCategory cat, StringView ipath, Access a) {
		if (!_assetManager) {
			return false;
		}

		if (cat == FileCategory::Bundled) {
			auto path = _getPlatformPath(ipath);
			return _archive.access(string::toString<Interface>("assets/", path), a);
		}
		return false;
	}

	void enumerateObjects(FileCategory cat, StringView ipath, FileFlags flags, Access a,
			const Callback<bool(StringView, FileFlags)> &cb) {
		if (cat == FileCategory::Bundled) {
			auto path = _getPlatformPath(ipath);
			if (hasFlag(flags, FileFlags::Writable) || hasFlag(flags, FileFlags::Public)
					|| hasFlag(flags, FileFlags::Shared)) {
				return;
			}
			if (a == Access::None || access(cat, path, a)) {
				cb(path, FileFlags::None);
			}
		}
	}

	bool stat(FileCategory cat, StringView ipath, Stat &stat) {
		if (!_assetManager) {
			return false;
		}

		if (cat == FileCategory::Bundled) {
			auto path = _getPlatformPath(ipath);
			return _archive.stat(string::toString<Interface>("assets/", path), stat);
		}
		return false;
	}

	File openForReading(FileCategory cat, StringView ipath) {
		if (!_assetManager) {
			return File();
		}

		if (cat == FileCategory::Bundled) {
			auto path = _getPlatformPath(ipath);

			AAsset *aa = AAssetManager_open(_assetManager,
					(path.terminated() ? path.data()
									   : path.str<memory::StandartInterface>().data()),
					AASSET_MODE_UNKNOWN);
			if (aa) {
				auto len = AAsset_getLength64(aa);
				return File((void *)aa, len);
			}
		}

		return File();
	}

	Status ftw(FileCategory cat, StringView ipath,
			const Callback<bool(StringView path, FileType)> &cb, int depth, bool dirFirst) {
		if (cat == FileCategory::Bundled) {
			auto path = _getPlatformPath(ipath);
			StringView prefix = "assets/";
			return _archive.ftw(string::toString<Interface>(prefix, path),
					[&](StringView path, FileType type) {
				if (path.starts_with(prefix)) {
					path = path.sub(prefix.size());
				}
				return cb(string::toString<Interface>("%PLATFORM%:", path), type);
			}, depth, dirFirst);
		}
		return Status::Declined;
	}
};

void Android_initializeFilesystem(void *assetManager, const jni::Ref &ref, StringView apkPath) {
	PathSource::getInstance()->initialize((AAssetManager *)assetManager, ref, apkPath);
}

void Android_terminateFilesystem() { PathSource::getInstance()->terminate(); }

StringView Android_getApkPath() { return PathSource::getInstance()->_apkPath; }

template <>
auto _getApplicationPath<memory::StandartInterface>() -> memory::StandartInterface::StringType {
	return StringView(PathSource::getInstance()->getApplicationPath())
			.str<memory::StandartInterface>();
}

template <>
auto _getApplicationPath<memory::PoolInterface>() -> memory::PoolInterface::StringType {
	return StringView(PathSource::getInstance()->getApplicationPath()).str<memory::PoolInterface>();
}

void _initSystemPaths(FilesystemResourceData &data) {
	PathSource::getInstance()->initSystemPaths(data);
}

void _enumerateObjects(const FilesystemResourceData &data, FileCategory cat, StringView path,
		FileFlags flags, Access a, const Callback<bool(StringView, FileFlags)> &cb) {
	PathSource::getInstance()->enumerateObjects(cat, path, flags, a, cb);
}

bool _access(FileCategory cat, StringView path, Access a) {
	if (path.empty() || path.front() == '/' || path.starts_with("..")
			|| path.find("/..") != maxOf<size_t>()) {
		return false;
	}

	return PathSource::getInstance()->access(cat, path, a);
}

bool _stat(FileCategory cat, StringView path, Stat &stat) {
	return PathSource::getInstance()->stat(cat, path, stat);
}

File _openForReading(FileCategory cat, StringView path) {
	return PathSource::getInstance()->openForReading(cat, path);
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

size_t _tell(void *aa) { return AAsset_seek64((AAsset *)aa, off64_t(0), int(SEEK_CUR)); }

bool _eof(void *aa) { return AAsset_getRemainingLength64((AAsset *)aa) == 0; }
void _close(void *aa) { AAsset_close((AAsset *)aa); }

Status _ftw(FileCategory cat, StringView path, const Callback<bool(StringView, FileType)> &cb,
		int depth, bool dirFirst) {
	return PathSource::getInstance()->ftw(cat, path, cb, depth, dirFirst);
}

} // namespace stappler::filesystem::platform

#endif
