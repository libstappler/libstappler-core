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

#ifndef CORE_FILESYSTEM_DETAIL_SPFILESYSTEMRESOURCEDATA_H_
#define CORE_FILESYSTEM_DETAIL_SPFILESYSTEMRESOURCEDATA_H_

#include "SPFilepath.h"
#include "SPFilesystem.h"

namespace STAPPLER_VERSIONIZED stappler::filesystem {

struct FilesystemResourceData : InterfaceObject<memory::StandartInterface> {
	struct ResourceLocation {
		FileCategory category;
		StringView prefix;
		StringView path;
		Vector<StringView> paths;
		bool init = false;
		bool writable = false;

		// if true - can be located by detectResourceType
		bool locatable = false;
	};

	static void initialize(void *ptr);
	static void terminate(void *ptr);

	static StringView getResourcePrefix(FileCategory);

	static FilesystemResourceData &get();

	FilesystemResourceData();
	FilesystemResourceData(const FilesystemResourceData &) = delete;
	FilesystemResourceData &operator=(const FilesystemResourceData &) = delete;

	void initResource(ResourceLocation &);

	void findObject(StringView filename, const ResourceLocation &res, Access a,
			const Callback<bool(StringView)> &cb) const;
	void findObject(StringView filename, FileCategory type, Access a,
			const Callback<bool(StringView)> &cb) const;

	void enumerateReadablePaths(StringView path, FileCategory t, Access a,
			const Callback<bool(StringView)> &cb) const;
	void enumerateWritablePaths(StringView path, FileCategory t, Access a,
			const Callback<bool(StringView)> &cb);

	void init();
	void term();

	// Find resource type for a path
	// return ResourceType::Max on failure
	FileCategory detectResourceCategory(StringView,
			const Callback<void(StringView)> &cb = nullptr) const;
	FileCategory detectResourceCategory(const FileInfo &,
			const Callback<void(StringView)> &cb = nullptr) const;

	// return ResourceType::Max on failure
	FileCategory getResourceCategoryByPrefix(StringView) const;

	// follows ReadablePath rules
	// returns false if lookup was not performed
	bool enumeratePrefixedPath(StringView, Access a, const Callback<bool(StringView)> &cb) const;

	bool _initialized = false;
	bool _appPathCommon = false;

	// Is bundle accessible with native filesystem functions
	bool _bundleIsTransparent = true;

	memory::pool_t *_pool = nullptr;

	std::mutex _initMutex;
	StringView _home;
	std::array<ResourceLocation, toInt(FileCategory::Max)> _resourceLocations;
};


} // namespace stappler::filesystem

#endif /* CORE_FILESYSTEM_DETAIL_SPFILESYSTEMRESOURCEDATA_H_ */
