/**
 Copyright (c) 2025 Stappler LLC <admin@stappler.dev>

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

#include "SPFilesystemResourceData.h"
#include "SPFilepath.h"
#include "SPFilesystem.h"
#include "SPMemInterface.h"
#include <mutex>

namespace STAPPLER_VERSIONIZED stappler::filesystem::platform {

void _initSystemPaths(FilesystemResourceData &data);

void _enumerateObjects(const FilesystemResourceData &data, StringView path, Access,
		const Callback<bool(StringView)> &);


} // namespace stappler::filesystem::platform

namespace STAPPLER_VERSIONIZED stappler::filesystem {

void FilesystemResourceData::initialize(void *ptr) {
	reinterpret_cast<FilesystemResourceData *>(ptr)->init();
}

void FilesystemResourceData::terminate(void *ptr) {
	reinterpret_cast<FilesystemResourceData *>(ptr)->term();
}

StringView FilesystemResourceData::getResourcePrefix(FileCategory cat) {
	switch (cat) {
	case FileCategory::Exec: return StringView("%EXEC%:"); break;
	case FileCategory::Library: return StringView("%LIBRARY%:"); break;

	case FileCategory::UserHome: return StringView("%USER_HOME%:"); break;
	case FileCategory::UserDesktop: return StringView("%USER_DESKTOP%:"); break;
	case FileCategory::UserDownload: return StringView("%USER_DOWNLOAD%:"); break;
	case FileCategory::UserTemplates: return StringView("%USER_TEMPLATES%:"); break;
	case FileCategory::UserPublicshare: return StringView("%USER_PUBLICSHARE%:"); break;
	case FileCategory::UserDocuments: return StringView("%USER_DOCUMENTS%:"); break;
	case FileCategory::UserMusic: return StringView("%USER_MUSIC%:"); break;
	case FileCategory::UserPictures: return StringView("%USER_PICTURES%:"); break;
	case FileCategory::UserVideos: return StringView("%USER_VIDEOS%:"); break;

	case FileCategory::CommonData: return StringView("%COMMON_DATA%:"); break;
	case FileCategory::CommonConfig: return StringView("%COMMON_CONFIG%:"); break;
	case FileCategory::CommonState: return StringView("%COMMON_STATE%:"); break;
	case FileCategory::CommonCache: return StringView("%COMMON_CACHE%:"); break;
	case FileCategory::CommonRuntime: return StringView("%COMMON_RUNTIME%:"); break;

	case FileCategory::AppData: return StringView("%APP_DATA%:"); break;
	case FileCategory::AppConfig: return StringView("%APP_CONFIG%:"); break;
	case FileCategory::AppState: return StringView("%APP_STATE%:"); break;
	case FileCategory::AppCache: return StringView("%APP_CACHE%:"); break;
	case FileCategory::AppRuntime: return StringView("%APP_RUNTIME%:"); break;

	case FileCategory::Bundled: return StringView("%PLATFORM%:"); break;
	case FileCategory::Max: break;
	}
	return StringView();
}

FilesystemResourceData::FilesystemResourceData() { addInitializer(this, &initialize, &terminate); }

void FilesystemResourceData::initResource(ResourceLocation &res) {
	if (res.path.empty()) {
		return;
	}

	auto root = filepath::root(res.path);

	filesystem::native::mkdir_fn(root);
	filesystem::native::mkdir_fn(res.path);

	if (res.writable) {
		// check if path is actually writable
		if (filesystem::native::access_fn(res.path, Access::Write) != Status::Ok) {
			res.writable = false;
		}
	}

	res.init = true;
}

void FilesystemResourceData::findObject(StringView filename, const ResourceLocation &res, Access a,
		const Callback<bool(StringView)> &cb) const {
	String path;

	if (!res.path.empty()) {
		path = filepath::merge<Interface>(res.path, filename);
		if (a == Access::None || filesystem::native::access_fn(path, a) == Status::Ok) {
			if (!cb(path)) {
				return;
			}
		}
	}

	for (auto &it : res.paths) {
		path = filepath::merge<Interface>(it, filename);
		if (a == Access::None || filesystem::native::access_fn(path, a) == Status::Ok) {
			if (!cb(path)) {
				return;
			}
		}
	}
}

void FilesystemResourceData::findObject(StringView filename, FileCategory type, Access a,
		const Callback<bool(StringView)> &cb) const {
	if (filepath::isAboveRoot(filename)) {
		return;
	}

	if (type == FileCategory::Bundled) {
		filesystem::platform::_enumerateObjects(*this, filename, a, cb);
	} else {
		findObject(filename, _resourceLocations[toInt(type)], a, cb);
	}
}

void FilesystemResourceData::enumerateReadablePaths(StringView path, FileCategory t, Access a,
		const Callback<bool(StringView)> &cb) const {
	findObject(path, t, a, cb);
}

void FilesystemResourceData::enumerateWritablePaths(StringView filename, FileCategory t, Access a,
		const Callback<bool(StringView)> &cb) {
	auto &res = _resourceLocations[toInt(t)];
	if (!res.writable) {
		return;
	}

	if (filepath::isAboveRoot(filename)) {
		return;
	}

	std::unique_lock lock(_initMutex);
	if (!res.init) {
		initResource(res);

		// re-check writable, this assumption can fail on init
		if (!res.writable) {
			return;
		}
	}

	String path;
	if (!res.path.empty()) {
		path = filepath::merge<Interface>(res.path, filename);
		if (a == Access::None || filesystem::native::access_fn(path, a) == Status::Ok) {
			if (!cb(path)) {
				return;
			}
		}
	}

	for (auto &it : res.paths) {
		path = filepath::merge<Interface>(it, filename);
		if (a == Access::None || filesystem::native::access_fn(path, a) == Status::Ok) {
			if (!cb(path)) {
				return;
			}
		}
	}
}

void FilesystemResourceData::init() {
	_pool = memory::pool::acquire();

	for (auto it : each<FileCategory>()) {
		auto &loc = _resourceLocations[toInt(it)];
		loc.category = it;
		loc.prefix = getResourcePrefix(it);
	}

	platform::_initSystemPaths(*this);

	for (auto it : each<FileCategory>()) {
		auto &loc = _resourceLocations[toInt(it)];

		loc.path.backwardSkipChars<StringView::Chars<'/'>>();
		for (auto &it : loc.paths) { it.backwardSkipChars<StringView::Chars<'/'>>(); }
	}

	_initialized = true;
}

void FilesystemResourceData::term() {
	_home = StringView();

	for (auto it : each<FileCategory>()) {
		_resourceLocations[toInt(it)].path = StringView();
		_resourceLocations[toInt(it)].paths.clear();
	}
}

FileCategory FilesystemResourceData::detectResourceCategory(StringView path,
		const Callback<void(StringView)> &cb) const {
	if (!path.is('/') && !_bundleIsTransparent) {
		if (filesystem::platform::_exists(path)) {
			return FileCategory::Bundled;
		}
		return FileCategory::Max;
	}

	// In reverse order - most concrete first
	const ResourceLocation *targetLoc = nullptr;
	uint32_t match = 0;

	auto it = _resourceLocations.crbegin();
	while (it != _resourceLocations.rend()) {
		if (it->locatable) {
			if (!it->path.empty()) {
				if (path.starts_with(it->path) && path.at(it->path.size()) == '/') {
					if (it->path.size() > match) {
						targetLoc = &(*it);
						match = it->path.size();
					}
				}
			}
			for (auto &resPath : it->paths) {
				if (path.starts_with(resPath) && path.at(resPath.size()) == '/') {
					if (resPath.size() > match) {
						targetLoc = &(*it);
						match = resPath.size();
					}
				}
			}
		}

		++it;
	}

	if (targetLoc) {
		if (cb) {
			path += match;
			path.skipChars<StringView::Chars<'/'>>();
			cb(string::toString<Interface>(targetLoc->prefix, path));
		}
		return targetLoc->category;
	}
	return FileCategory::Max;
}

FileCategory FilesystemResourceData::detectResourceCategory(const FileInfo &info,
		const Callback<void(StringView)> &cb) const {
	if (info.category == FileCategory::Custom) {
		return FileCategory::Max;
	} else if (info.category == FileCategory::Bundled) {
		if (filesystem::platform::_exists(info.path)) {
			if (cb) {
				auto path = info.path;
				path.skipChars<StringView::Chars<'/'>>();
				cb(string::toString<Interface>(
						_resourceLocations[toInt(FileCategory::Bundled)].prefix, path));
			}
		}
		return FileCategory::Max;
	} else {
		auto &res = _resourceLocations[toInt(info.category)];
		if (cb) {
			auto path = info.path;
			path.skipChars<StringView::Chars<'/'>>();
			cb(string::toString<Interface>(res.prefix, path));
		}
		return res.category;
	}
}

FileCategory FilesystemResourceData::getResourceCategoryByPrefix(StringView prefix) const {
	for (auto it : each<FileCategory>()) {
		if (prefix.starts_with(_resourceLocations[toInt(it)].prefix)) {
			return _resourceLocations[toInt(it)].category;
		}
	}

	return FileCategory::Max;
}

bool FilesystemResourceData::enumeratePrefixedPath(StringView path, Access a,
		const Callback<bool(StringView)> &cb) const {
	if (!path.starts_with("%")) {
		return false;
	}

	auto type = getResourceCategoryByPrefix(path);
	if (type != FileCategory::Max) {
		auto &res = _resourceLocations[toInt(type)];

		path += res.prefix.size();
		path.skipChars<StringView::Chars<'/'>>();

		// enumerate target dirs
		if (path.empty()) {
			if (a == Access::None) {
				enumeratePaths(type, cb);
			} else {
				return false;
			}
		}

		if (filepath::isAboveRoot(path)) {
			return false;
		}

		auto reconstructed = filepath::reconstructPath<Interface>(path);
		if (reconstructed.empty()) {
			return false;
		}

		enumerateReadablePaths(reconstructed, type, a, cb);
		return true;
	}
	return false;
}

static FilesystemResourceData s_filesystemPathData;

bool isFileResourceCategoryWritable(FileCategory t) {
	return s_filesystemPathData._resourceLocations[toInt(t)].writable;
}

// enumerate all paths, that will be used to find a resource of specific types
void enumeratePaths(FileCategory t, const Callback<bool(StringView)> &cb) {
	auto &res = s_filesystemPathData._resourceLocations[toInt(t)];
	if (!res.path.empty()) {
		if (!cb(res.path)) {
			return;
		}
	}

	for (auto &it : res.paths) {
		if (!cb(it)) {
			return;
		}
	}
}

void enumerateReadablePaths(StringView path, FileCategory t, Access a,
		const Callback<bool(StringView)> &cb) {
	if (t < FileCategory::Custom) {
		s_filesystemPathData.enumerateReadablePaths(path, t, a, cb);
	} else {
		memory::StandartInterface::StringType str;
		if (!filepath::isAbsolute(path)) {
			str = currentDir<memory::StandartInterface>(path);
			path = str;
		}

		if (a == Access::None || native::access_fn(path, a) == Status::Ok) {
			cb(path);
		}
	}
}

void enumerateReadablePaths(StringView path, FileCategory category,
		const Callback<bool(StringView)> &cb) {
	enumerateReadablePaths(path, category, Access::None, cb);
}

void enumerateReadablePaths(const FileInfo &info, Access a, const Callback<bool(StringView)> &cb) {
	enumerateReadablePaths(info.path, info.category, a, cb);
}

void enumerateReadablePaths(const FileInfo &info, const Callback<bool(StringView)> &cb) {
	enumerateReadablePaths(info.path, info.category, Access::None, cb);
}

bool enumeratePrefixedPath(StringView path, Access a, const Callback<bool(StringView)> &cb) {
	return s_filesystemPathData.enumeratePrefixedPath(path, a, cb);
}

bool enumeratePrefixedPath(StringView path, const Callback<bool(StringView)> &cb) {
	return s_filesystemPathData.enumeratePrefixedPath(path, Access::None, cb);
}

void enumerateWritablePaths(StringView path, FileCategory t, Access a,
		const Callback<bool(StringView)> &cb) {
	if (t < FileCategory::Custom) {
		s_filesystemPathData.enumerateWritablePaths(path, t, a, cb);
	} else {
		memory::StandartInterface::StringType str;
		if (!filepath::isAbsolute(path)) {
			str = currentDir<memory::StandartInterface>(path);
			path = str;
		}

		if (a == Access::None || native::access_fn(path, a) == Status::Ok) {
			cb(path);
		}
	}
}

void enumerateWritablePaths(StringView path, FileCategory category,
		const Callback<bool(StringView)> &cb) {
	enumerateWritablePaths(path, category, Access::None, cb);
}

void enumerateWritablePaths(const FileInfo &info, Access a, const Callback<bool(StringView)> &cb) {
	enumerateWritablePaths(info.path, info.category, a, cb);
}

void enumerateWritablePaths(const FileInfo &info, const Callback<bool(StringView)> &cb) {
	enumerateWritablePaths(info.path, info.category, Access::None, cb);
}

FileCategory detectResourceCategory(StringView path, const Callback<void(StringView)> &cb) {
	return s_filesystemPathData.detectResourceCategory(path, cb);
}

FileCategory detectResourceCategory(const FileInfo &info, const Callback<void(StringView)> &cb) {
	return s_filesystemPathData.detectResourceCategory(info, cb);
}


} // namespace stappler::filesystem
