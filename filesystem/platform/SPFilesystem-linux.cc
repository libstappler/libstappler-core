/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
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
#include "detail/SPFilesystemResourceData.h"
#include "SPMemInterface.h"
#include "SPSharedModule.h"

#if LINUX

#include <limits.h>
#include <fcntl.h>

namespace STAPPLER_VERSIONIZED stappler::filesystem::platform {

static char s_execPath[PATH_MAX] = {0};

static memory::StandartInterface::StringType getEnvExt(StringView key) {
	if (key == "EXEC_DIR") {
		return filepath::root(StringView((char *)s_execPath)).str<memory::StandartInterface>();
	} else if (key == "CWD") {
		return currentDir<memory::StandartInterface>();
	} else {
		auto e = ::getenv(key.str<memory::StandartInterface>().data());
		if (e) {
			return memory::StandartInterface::StringType(e);
		}
	}
	return memory::StandartInterface::StringType();
}

static void readSingleQuoted(StringView &str, const Callback<void(StringView)> &writeCb) {
	++str;
	while (!str.empty()) {
		auto v = str.readUntil<StringView::Chars<'\'', '\\'>>();
		if (!v.empty()) {
			writeCb << v;
		}
		if (str.is('\\')) {
			++str;
			writeCb << str[0];
			++str;
		} else if (str.is<'\''>()) {
			++str;
			return;
		}
	}
}

static void readDoubleQuoted(StringView &str, const Callback<void(StringView)> &writeCb) {
	++str;
	while (!str.empty()) {
		auto v = str.readUntil<StringView::Chars<'"', '\\', '$', '\''>>();
		if (!v.empty()) {
			writeCb << v;
		}
		if (str.is('\\')) {
			++str;
			writeCb << str[0];
			++str;
		} else if (str.is('$')) {
			++str;
			auto v =
					str.readUntil<StringView::Chars<'"', '\'', '$', '/'>, StringView::WhiteSpace>();
			if (!v.empty()) {
				// we need null-terminated string
				auto env = getEnvExt(v.str<memory::StandartInterface>().data());
				if (!env.empty()) {
					writeCb << env;
				}
			}
		} else if (str.is('\'')) {
			readSingleQuoted(str, writeCb);
		} else if (str.is<'"'>()) {
			++str;
			return;
		}
	}
}

static StringView readVariable(memory::pool_t *pool, StringView str) {
	memory::StandartInterface::StringType out;

	auto writer = [&](StringView s) { out.append(s.data(), s.size()); };

	Callback<void(StringView)> writeCb(writer);

	str.trimChars<StringView::WhiteSpace>();
	while (!str.empty()) {
		if (str.is('"')) {
			readDoubleQuoted(str, writeCb);
		} else if (str.is('\'')) {
			readSingleQuoted(str, writeCb);
		} else if (str.is('$')) {
			++str;
			auto v =
					str.readUntil<StringView::Chars<'"', '\'', '$', '/'>, StringView::WhiteSpace>();
			if (!v.empty()) {
				// we need null-terminated string
				auto env = getEnvExt(v.str<memory::StandartInterface>().data());
				if (!env.empty()) {
					writeCb << env;
				}
			}
		} else {
			auto v = str.readUntil<StringView::Chars<'"', '\'', '$'>>();
			if (!v.empty()) {
				writeCb << v;
			}
		}
	}

	auto ret = StringView(out);
	ret.backwardSkipChars<StringView::Chars<'/'>>();
	return ret.pdup(pool);
}

void _initSystemPaths(FilesystemResourceData &data) {
	auto uid = ::geteuid();

	auto home = ::getenv("HOME");
	if (!home) {
		log::error("filesystem", "HOME envvar is not defined");
		return;
	}

	// read appconfig
	auto bundleName = SharedModule::acquireTypedSymbol<const char *>(
			buildconfig::MODULE_APPCONFIG_NAME, "APPCONFIG_BUNDLE_NAME");
	auto bundlePath = SharedModule::acquireTypedSymbol<const char *>(
			buildconfig::MODULE_APPCONFIG_NAME, "APPCONFIG_BUNDLE_PATH");
	auto appPathCommon = SharedModule::acquireTypedSymbol<int *>(buildconfig::MODULE_APPCONFIG_NAME,
			"APPCONFIG_APP_PATH_COMMON");

	if (appPathCommon && *appPathCommon > 0) {
		data._appPathCommon = true;
	}

	data._home = StringView(home).pdup(data._pool);

	ssize_t length = ::readlink("/proc/self/exe", s_execPath, sizeof(s_execPath) - 1);

	auto &bundledLoc = data._resourceLocations[toInt(FileCategory::Bundled)];

	bundledLoc.path = StringView(s_execPath, length).pdup(data._pool);
	bundledLoc.init = true;
	bundledLoc.locatable = true;

	if (bundlePath) {
		StringView(bundlePath).split<StringView::Chars<':'>>([&](StringView str) {
			auto value = readVariable(data._pool, str);
			if (!value.empty()) {
				bundledLoc.paths.emplace_back(value);
			}
		});
	}

	auto pathEnv = ::getenv("PATH");
	if (pathEnv) {
		StringView(pathEnv).split<StringView::Chars<':'>>([&](StringView value) {
			data._resourceLocations[toInt(FileCategory::Exec)].paths.emplace_back(
					value.pdup(data._pool));
		});
		data._resourceLocations[toInt(FileCategory::Exec)].locatable = true;
	}

	// search for XDG envvars
	auto dataHome = ::getenv("XDG_DATA_HOME");
	if (dataHome) {
		data._resourceLocations[toInt(FileCategory::CommonData)].path =
				StringView(dataHome).pdup(data._pool);
	} else {
		data._resourceLocations[toInt(FileCategory::CommonData)].path =
				StringView(filepath::merge<memory::StandartInterface>(data._home, ".local/share"))
						.pdup(data._pool);
	}

	auto dataDirs = ::getenv("XDG_DATA_DIRS");
	if (dataDirs) {
		StringView(dataDirs).split<StringView::Chars<':'>>([&](StringView value) {
			data._resourceLocations[toInt(FileCategory::CommonData)].paths.emplace_back(
					value.pdup(data._pool));
		});
	} else {
		data._resourceLocations[toInt(FileCategory::CommonData)].paths.emplace_back(
				"/usr/local/share");
		data._resourceLocations[toInt(FileCategory::CommonData)].paths.emplace_back("/usr/share");
	}

	data._resourceLocations[toInt(FileCategory::CommonData)].init = true;
	data._resourceLocations[toInt(FileCategory::CommonData)].locatable = true;

	auto configHome = ::getenv("XDG_CONFIG_HOME");
	if (configHome) {
		data._resourceLocations[toInt(FileCategory::CommonConfig)].path =
				StringView(configHome).pdup(data._pool);
	} else {
		data._resourceLocations[toInt(FileCategory::CommonConfig)].path =
				StringView(filepath::merge<memory::StandartInterface>(data._home, ".config"))
						.pdup(data._pool);
	}

	auto configDirs = ::getenv("XDG_CONFIG_DIRS");
	if (configDirs) {
		StringView(configDirs).split<StringView::Chars<':'>>([&](StringView value) {
			data._resourceLocations[toInt(FileCategory::CommonConfig)].paths.emplace_back(
					value.pdup(data._pool));
		});
	} else {
		data._resourceLocations[toInt(FileCategory::CommonConfig)].paths.emplace_back("/etc/xdg");
	}

	data._resourceLocations[toInt(FileCategory::CommonConfig)].init = true;
	data._resourceLocations[toInt(FileCategory::CommonConfig)].locatable = true;

	auto stateHome = ::getenv("XDG_STATE_HOME");
	if (stateHome) {
		data._resourceLocations[toInt(FileCategory::CommonState)].path =
				StringView(stateHome).pdup(data._pool);
	} else {
		data._resourceLocations[toInt(FileCategory::CommonState)].path =
				StringView(filepath::merge<memory::StandartInterface>(data._home, ".local/state"))
						.pdup(data._pool);
	}
	data._resourceLocations[toInt(FileCategory::CommonState)].init = true;
	data._resourceLocations[toInt(FileCategory::CommonState)].locatable = true;

	auto cacheHome = ::getenv("XDG_CACHE_HOME");
	if (cacheHome) {
		data._resourceLocations[toInt(FileCategory::CommonCache)].path =
				StringView(cacheHome).pdup(data._pool);
	} else {
		data._resourceLocations[toInt(FileCategory::CommonCache)].path =
				StringView(filepath::merge<memory::StandartInterface>(data._home, ".cache"))
						.pdup(data._pool);
	}
	data._resourceLocations[toInt(FileCategory::CommonCache)].init = true;
	data._resourceLocations[toInt(FileCategory::CommonCache)].locatable = true;

	auto runtimeDir = ::getenv("XDG_RUNTIME_DIR");
	if (runtimeDir) {
		data._resourceLocations[toInt(FileCategory::CommonRuntime)].path =
				StringView(runtimeDir).pdup(data._pool);
	} else {
		data._resourceLocations[toInt(FileCategory::CommonRuntime)].path =
				StringView(string::toString<memory::StandartInterface>("/run/user/", uid))
						.pdup(data._pool);
	}
	data._resourceLocations[toInt(FileCategory::CommonRuntime)].init = true;
	data._resourceLocations[toInt(FileCategory::CommonRuntime)].locatable = true;

	for (auto it : each<FileCategory, FileCategory::UserHome, FileCategory::UserVideos>()) {
		data._resourceLocations[toInt(it)].path = data._home;
		data._resourceLocations[toInt(it)].init = true;
	}
	data._resourceLocations[toInt(FileCategory::UserHome)].locatable = true;

	bool userConfigFound = false;
	auto filedata = filesystem::readIntoMemory<memory::StandartInterface>(
			FileInfo{"user-dirs.dirs", FileCategory::CommonConfig});
	if (!filedata.empty()) {
		StringView strData(BytesView(filedata).toStringView());

		auto writeLocation = [&](FileCategory t, StringView var) {
			if (!var.empty()) {
				data._resourceLocations[toInt(t)].path = var;
				if (var != data._home) {
					data._resourceLocations[toInt(t)].locatable = true;
				}
			}
		};

		strData.split<StringView::Chars<'\n', '\r'>>([&](StringView str) {
			if (!str.is('#')) {
				auto var = str.readUntil<StringView::Chars<'='>>();
				if (str.is('=')) {
					++str;
					var.trimChars<StringView::WhiteSpace>();
					if (var == "XDG_DESKTOP_DIR") {
						writeLocation(FileCategory::UserDesktop, readVariable(data._pool, str));
					} else if (var == "XDG_DOWNLOAD_DIR") {
						writeLocation(FileCategory::UserDownload, readVariable(data._pool, str));
					} else if (var == "XDG_TEMPLATES_DIR") {
						writeLocation(FileCategory::UserTemplates, readVariable(data._pool, str));
					} else if (var == "XDG_PUBLICSHARE_DIR") {
						writeLocation(FileCategory::UserPublicshare, readVariable(data._pool, str));
					} else if (var == "XDG_DOCUMENTS_DIR") {
						writeLocation(FileCategory::UserDocuments, readVariable(data._pool, str));
					} else if (var == "XDG_MUSIC_DIR") {
						writeLocation(FileCategory::UserMusic, readVariable(data._pool, str));
					} else if (var == "XDG_PICTURES_DIR") {
						writeLocation(FileCategory::UserPictures, readVariable(data._pool, str));
					} else if (var == "XDG_VIDEOS_DIR") {
						writeLocation(FileCategory::UserVideos, readVariable(data._pool, str));
					}
				}
			}
		});
		userConfigFound = true;
	}

	if (!userConfigFound) {
		log::error("filesystem", "XDG defaults (user-dirs.dirs) not found, fallback to home dir");
	}

	if (bundleName && data._appPathCommon) {
		// create app dirs on XDG locations
		auto off = toInt(FileCategory::AppData) - toInt(FileCategory::CommonData);
		for (auto it : each<FileCategory, FileCategory::AppData, FileCategory::AppRuntime>()) {
			data._resourceLocations[toInt(it)].path =
					StringView(filepath::merge<memory::StandartInterface>(
									   data._resourceLocations[toInt(it) - off].path, bundleName))
							.pdup(data._pool);
			data._resourceLocations[toInt(it)].writable = true;
		}
	} else {
		auto bundlePath =
				filepath::root(data._resourceLocations[toInt(FileCategory::Bundled)].path);

		data._resourceLocations[toInt(FileCategory::AppData)].path =
				StringView(filepath::merge<memory::StandartInterface>(bundlePath, "AppData/data"))
						.pdup(data._pool);
		data._resourceLocations[toInt(FileCategory::AppData)].writable = true;

		data._resourceLocations[toInt(FileCategory::AppConfig)].path =
				StringView(filepath::merge<memory::StandartInterface>(bundlePath, "AppData/config"))
						.pdup(data._pool);
		data._resourceLocations[toInt(FileCategory::AppConfig)].writable = true;

		data._resourceLocations[toInt(FileCategory::AppState)].path =
				StringView(filepath::merge<memory::StandartInterface>(bundlePath, "AppData/state"))
						.pdup(data._pool);
		data._resourceLocations[toInt(FileCategory::AppState)].writable = true;

		data._resourceLocations[toInt(FileCategory::AppCache)].path =
				StringView(filepath::merge<memory::StandartInterface>(bundlePath, "AppData/cache"))
						.pdup(data._pool);
		data._resourceLocations[toInt(FileCategory::AppCache)].writable = true;

		data._resourceLocations[toInt(FileCategory::AppRuntime)].path = StringView(
				filepath::merge<memory::StandartInterface>(bundlePath, "AppData/runtime"))
																				.pdup(data._pool);
		data._resourceLocations[toInt(FileCategory::AppRuntime)].writable = true;
	}
}

void _enumerateObjects(const FilesystemResourceData &data, StringView filename, Access a,
		const Callback<bool(StringView)> &cb) {

	switch (a) {
	case Access::Execute:
	case Access::Write: return; break;
	default: break;
	}

	auto res = data._resourceLocations[toInt(FileCategory::Bundled)];

	memory::StandartInterface::StringType path;

	for (auto &it : res.paths) {
		path = filepath::merge<memory::StandartInterface>(it, filename);
		if (a == Access::None || filesystem::native::access_fn(path, a) == Status::Ok) {
			if (!cb(path)) {
				return;
			}
		}
	}
}

bool _exists(StringView path) {
	bool found = false;
	filesystem::enumerateReadablePaths(FileInfo{path, FileCategory::Bundled}, Access::Exists,
			[&](StringView str) {
		found = true;
		return false;
	});

	return found;
}

bool _stat(StringView path, Stat &stat) {
	bool found = false;
	filesystem::enumerateReadablePaths(FileInfo{path, FileCategory::Bundled}, Access::Exists,
			[&](StringView str) {
		found = filesystem::native::stat_fn(str, stat) == Status::Ok;
		return false;
	});

	return found;
}

File _openForReading(StringView path) {
	FILE *f = nullptr;
	filesystem::enumerateReadablePaths(FileInfo{path, FileCategory::Bundled}, Access::Read,
			[&](StringView str) {
		f = native::fopen_fn(str, "r");
		return false;
	});

	if (f) {
		return File(f);
	}

	return File();
}

size_t _read(void *, uint8_t *buf, size_t nbytes) { return 0; }
size_t _seek(void *, int64_t offset, io::Seek s) { return maxOf<size_t>(); }
size_t _tell(void *) { return 0; }
bool _eof(void *) { return true; }
void _close(void *) { }

Status _ftw(StringView path, const Callback<bool(StringView path, FileType t)> &cb, int depth,
		bool dirFirst) {
	Status status = Status::Declined;
	filesystem::enumerateReadablePaths(FileInfo{path, FileCategory::Bundled}, Access::Exists,
			[&](StringView str) {
		filesystem::native::ftw_fn(str, cb, depth, dirFirst);
		return false;
	});

	return status;
}

} // namespace stappler::filesystem::platform

#endif
