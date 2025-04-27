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

#include "SPFilepath.h"

#if LINUX

#include "SPFilesystem.h"
#include "SPMemInterface.h"
#include "SPSharedModule.h"
#include "detail/SPFilesystemResourceData.h"
#include <limits.h>
#include <fcntl.h>

namespace STAPPLER_VERSIONIZED stappler::filesystem::platform {

static char s_execPath[PATH_MAX] = {0};
static char s_homePath[PATH_MAX] = {0};

static memory::StandartInterface::StringType getEnvExt(StringView key) {
	if (key == "EXEC_DIR") {
		return filepath::root(StringView((char *)s_execPath)).str<memory::StandartInterface>();
	} else if (key == "CWD") {
		return currentDir<memory::StandartInterface>();
	} else if (key == "XDG_DATA_HOME") {
		auto var = ::getenv("XDG_DATA_HOME");
		if (!var || var[0] == 0) {
			return filepath::merge<memory::StandartInterface>(s_homePath, ".local/share");
		}
		return var;
	} else if (key == "XDG_CONFIG_HOME") {
		auto var = ::getenv("XDG_CONFIG_HOME");
		if (!var || var[0] == 0) {
			return filepath::merge<memory::StandartInterface>(s_homePath, ".config");
		}
		return var;
	} else if (key == "XDG_STATE_HOME") {
		auto var = ::getenv("XDG_STATE_HOME");
		if (!var || var[0] == 0) {
			return filepath::merge<memory::StandartInterface>(s_homePath, ".local/state");
		}
		return var;
	} else if (key == "XDG_CACHE_HOME") {
		auto var = ::getenv("XDG_CACHE_HOME");
		if (!var || var[0] == 0) {
			return filepath::merge<memory::StandartInterface>(s_homePath, ".cache");
		}
		return var;
	} else if (key == "XDG_RUNTIME_DIR") {
		auto var = ::getenv("XDG_RUNTIME_DIR");
		if (!var || var[0] == 0) {
			return string::toString<memory::StandartInterface>("/run/user/", geteuid());
		}
		return var;
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

	(void)::readlink("/proc/self/exe", s_execPath, sizeof(s_execPath) - 1);
	memcpy(s_homePath, home, strlen(home));

	auto &bundledLoc = data._resourceLocations[toInt(FileCategory::Bundled)];

	bundledLoc.init = true;
	bundledLoc.flags |= CategoryFlags::Locateable;

	if (bundlePath) {
		StringView(bundlePath).split<StringView::Chars<':'>>([&](StringView str) {
			auto value = readVariable(data._pool, str);
			if (!value.empty()) {
				bundledLoc.paths.emplace_back(value, FileFlags::Private);
			}
		});
	}

	auto pathEnv = ::getenv("PATH");
	if (pathEnv) {
		auto &res = data._resourceLocations[toInt(FileCategory::Exec)];
		StringView(pathEnv).split<StringView::Chars<':'>>([&](StringView value) {
			res.paths.emplace_back(value.pdup(data._pool), FileFlags::Shared);
		});
		res.flags |= CategoryFlags::Locateable;
	}

	// search for XDG envvars
	auto dataHome = getEnvExt("XDG_DATA_HOME");
	if (!dataHome.empty()) {
		auto &res = data._resourceLocations[toInt(FileCategory::CommonData)];
		res.paths.emplace_back(StringView(dataHome).pdup(data._pool), FileFlags::Shared);

		auto dataDirs = ::getenv("XDG_DATA_DIRS");
		if (dataDirs && dataDirs[0] == 0) {
			StringView(dataDirs).split<StringView::Chars<':'>>([&](StringView value) {
				res.paths.emplace_back(value.pdup(data._pool), FileFlags::Shared);
			});
		} else {
			res.paths.emplace_back("/usr/local/share", FileFlags::Shared);
			res.paths.emplace_back("/usr/share", FileFlags::Shared);
		}

		res.init = true;
		res.flags |= CategoryFlags::Locateable;
	}

	auto configHome = getEnvExt("XDG_CONFIG_HOME");
	if (!configHome.empty()) {
		auto &res = data._resourceLocations[toInt(FileCategory::CommonConfig)];
		res.paths.emplace_back(StringView(configHome).pdup(data._pool), FileFlags::Shared);

		auto configDirs = ::getenv("XDG_CONFIG_DIRS");
		if (configDirs) {
			StringView(configDirs).split<StringView::Chars<':'>>([&](StringView value) {
				res.paths.emplace_back(value.pdup(data._pool), FileFlags::Shared);
			});
		} else {
			res.paths.emplace_back("/etc/xdg", FileFlags::Shared);
		}

		res.init = true;
		res.flags |= CategoryFlags::Locateable;
	}

	auto stateHome = getEnvExt("XDG_STATE_HOME");
	if (!stateHome.empty()) {
		auto &res = data._resourceLocations[toInt(FileCategory::CommonState)];
		res.paths.emplace_back(StringView(stateHome).pdup(data._pool), FileFlags::Shared);
		res.init = true;
		res.flags |= CategoryFlags::Locateable;
	}

	auto cacheHome = getEnvExt("XDG_CACHE_HOME");
	if (!cacheHome.empty()) {
		auto &res = data._resourceLocations[toInt(FileCategory::CommonCache)];
		res.paths.emplace_back(StringView(cacheHome).pdup(data._pool), FileFlags::Shared);
		res.init = true;
		res.flags |= CategoryFlags::Locateable;
	}

	auto runtimeDir = getEnvExt("XDG_RUNTIME_DIR");
	if (!runtimeDir.empty()) {
		auto &res = data._resourceLocations[toInt(FileCategory::CommonRuntime)];
		res.paths.emplace_back(StringView(runtimeDir).pdup(data._pool), FileFlags::Shared);
		res.init = true;
		res.flags |= CategoryFlags::Locateable;
	}

	data._resourceLocations[toInt(FileCategory::UserHome)].paths.emplace_back(
			StringView(s_homePath), FileFlags::Shared);
	data._resourceLocations[toInt(FileCategory::UserHome)].flags |= CategoryFlags::Locateable;
	data._resourceLocations[toInt(FileCategory::UserHome)].init = true;

	do {
		auto &dataRes = data._resourceLocations[toInt(FileCategory::CommonData)];
		auto &res = data._resourceLocations[toInt(FileCategory::Fonts)];
		for (auto &it : dataRes.paths) {
			res.paths.emplace_back(
					StringView(filepath::merge<memory::StandartInterface>(it.first, "fonts"))
							.pdup(data._pool),
					FileFlags::Shared);
		}
		res.flags |= CategoryFlags::Locateable;
		res.init = true;
	} while (0);

	bool userConfigFound = false;
	auto filedata = filesystem::readIntoMemory<memory::StandartInterface>(
			FileInfo{"user-dirs.dirs", FileCategory::CommonConfig});
	if (!filedata.empty()) {
		StringView strData(BytesView(filedata).toStringView());

		auto writeLocation = [&](FileCategory t, StringView var) {
			auto &res = data._resourceLocations[toInt(t)];
			if (!var.empty()) {
				res.paths.emplace_back(var, FileFlags::Shared);
				if (var != StringView(s_homePath)) {
					res.flags |= CategoryFlags::Locateable;
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

	for (auto it : each<FileCategory, FileCategory::UserHome, FileCategory::UserVideos>()) {
		auto &res = data._resourceLocations[toInt(it)];
		if (res.paths.empty()) {
			res.paths.emplace_back(StringView(s_homePath), FileFlags::Shared);
			res.init = true;
		}
	}

	if (!userConfigFound) {
		log::warn("filesystem", "XDG defaults (user-dirs.dirs) not found, fallback to home dir");
	}

	if (bundleName && data._appPathCommon) {
		// create app dirs on XDG locations
		auto off = toInt(FileCategory::AppData) - toInt(FileCategory::CommonData);
		for (auto it : each<FileCategory, FileCategory::AppData, FileCategory::AppRuntime>()) {
			auto &res = data._resourceLocations[toInt(it)];
			res.paths.emplace_back(
					StringView(filepath::merge<memory::StandartInterface>(
									   data._resourceLocations[toInt(it) - off].paths.front().first,
									   bundleName))
							.pdup(data._pool),
					FileFlags::Private | FileFlags::Public);
			res.flags |= CategoryFlags::Locateable;
		}
	} else {
		auto bundlePath = filepath::root(s_execPath);
		data.initAppPaths(bundlePath);
	}
}

// No PlatformSpecific categories defined for now
void _enumerateObjects(const FilesystemResourceData &data, FileCategory, StringView path, FileFlags,
		Access, const Callback<bool(StringView, FileFlags)> &) { }

bool _access(FileCategory cat, StringView path, Access) { return false; }

bool _stat(FileCategory cat, StringView path, Stat &stat) { return false; }

File _openForReading(FileCategory cat, StringView path) { return File(); }

size_t _read(void *, uint8_t *buf, size_t nbytes) { return 0; }
size_t _seek(void *, int64_t offset, io::Seek s) { return maxOf<size_t>(); }
size_t _tell(void *) { return 0; }
bool _eof(void *) { return true; }
void _close(void *) { }

Status _ftw(FileCategory cat, StringView path,
		const Callback<bool(StringView path, FileType t)> &cb, int depth, bool dirFirst) {
	return Status::Declined;
}

template <>
auto _getApplicationPath<memory::StandartInterface>() -> memory::StandartInterface::StringType {
	return s_execPath;
}

template <>
auto _getApplicationPath<memory::PoolInterface>() -> memory::PoolInterface::StringType {
	using Interface = memory::PoolInterface;
	return StringView(s_execPath).str<Interface>();
}

} // namespace stappler::filesystem::platform

#endif
