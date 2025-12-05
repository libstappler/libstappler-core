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

#include "SPCommon.h" // IWYU pragma: keep
#include "SPFilepath.h"
#include "SPFilesystem.h"
#include "SPMemInterface.h"
#include "SPMemory.h"
#include "SPDocument.h"
#include "SPDocPageContainer.h"

#include "SPMakefile.h"
#include "XCodeProject.h"
#include "LocaleInfo.h"

#include "SPMemForwardList.h"

namespace stappler::buildtool {

// Выбираем стандартную подсистему памяти для текущего пространства имён
using namespace mem_std;

// Строка при запросе помощи по команде
static constexpr auto HELP_STRING =
		R"(stappler-build <action> - build assistant for the Stappler SDK
Actions:
	list - search for an available STAPPLER_BUILD_ROOT in the system and list them
	make [--with make-bin-path] <args> - make current work dir (forward arguments to 'make' utility)
	get-root - print STAPPLER_BUILD_ROOT to use in makefile scripts
	xcodegen <path> - generate XCode project files for project with <path>
	localeinfo [<filename>]
		- build LocaleInfo data, if filename is not specified - try to load from simplelocalize.io
	help - show this message
)";

static constexpr StringView SYSTEM_WIDE_PROFILE_D_FILE = "/etc/profile.d/stappler-sdk.sh";
static constexpr StringView SYSTEM_WIDE_ENVIRONMENT_D_FILE =
		"/etc/environment.d/50stappler-sdk.conf";

static constexpr StringView USER_ENVIRONMENT_D_FILE = "environment.d/50stappler-sdk.conf";
static constexpr StringView USER_PROFILE_D_FILE = "profile.d/stappler-sdk.sh";
static constexpr StringView USER_SDK_PROFILE = "org.stappler/sdk.json";

static void printHelp() { std::cout << HELP_STRING << "\n"; }

static bool checkCandidateDir(StringView candidate) {
	if (candidate.empty()) {
		return false;
	}

	filesystem::Stat stat;
	if (filesystem::stat(FileInfo{candidate}, stat)) {
		if (stat.type == FileType::Dir) {
			auto uPath = filepath::merge<Interface>(candidate, "universal.mk");
			if (filesystem::exists(FileInfo{uPath})) {
				auto fileData = filesystem::readIntoMemory<Interface>(FileInfo{uPath}, 0, 2_KiB);
				auto strData = BytesView(fileData).readString();

				if (strData.find("#@ STAPPLER_BUILD_ROOT") != maxOf<size_t>()) {
					return true;
				}
			}
		}
	}
	return false;
}

static String getCandidateFromShellScript(StringView data) {
	StringView tmp;
	data.skipUntilString("export STAPPLER_BUILD_ROOT=");
	if (data.starts_with("export STAPPLER_BUILD_ROOT=")) {
		if (data.is('"')) {
			++data;
			tmp = data.readUntil<StringView::Chars<'"'>>();
			return tmp.str<Interface>();
		} else if (data.is('\'')) {
			++data;
			tmp = data.readUntil<StringView::Chars<'\''>>();
			return tmp.str<Interface>();
		} else {
			tmp = data.readUntil<StringView::WhiteSpace>();
		}
	}
	return tmp.str<Interface>();
}

static String getCandidateFromEnvironmentConfig(StringView data) {
	StringView tmp;
	data.skipUntilString("STAPPLER_BUILD_ROOT=");
	if (data.starts_with("STAPPLER_BUILD_ROOT=")) {
		if (data.is('"')) {
			++data;
			tmp = data.readUntil<StringView::Chars<'"'>>();
			return tmp.str<Interface>();
		} else if (data.is('\'')) {
			++data;
			tmp = data.readUntil<StringView::Chars<'\''>>();
			return tmp.str<Interface>();
		} else {
			tmp = data.readUntil<StringView::WhiteSpace>();
		}
	}
	return tmp.str<Interface>();
}

static void findStapplerBuildRoot(const Callback<bool(StringView)> &cb) {
	String candidate;

	auto buildRoot = StringView(::getenv("STAPPLER_BUILD_ROOT"));
	if (checkCandidateDir(buildRoot)) {
		if (!cb(candidate)) {
			return;
		}
	}

	filesystem::enumeratePaths(USER_PROFILE_D_FILE, FileCategory::CommonConfig,
			filesystem::Access::Read, [&](StringView path, FileFlags flags) {
		auto fileData = filesystem::readIntoMemory<Interface>(FileInfo{path});
		auto strData = BytesView(fileData).readString();

		candidate = getCandidateFromShellScript(strData);
		if (checkCandidateDir(candidate)) {
			if (!cb(candidate)) {
				return false;
			}
		}
		return true;
	});

	filesystem::enumeratePaths(USER_ENVIRONMENT_D_FILE, FileCategory::CommonConfig,
			filesystem::Access::Read, [&](StringView path, FileFlags flags) {
		auto fileData = filesystem::readIntoMemory<Interface>(FileInfo{path});
		auto strData = BytesView(fileData).readString();

		candidate = getCandidateFromEnvironmentConfig(strData);
		if (checkCandidateDir(candidate)) {
			if (!cb(candidate)) {
				return false;
			}
		}
		return true;
	});

	filesystem::enumeratePaths(USER_SDK_PROFILE, FileCategory::CommonConfig,
			filesystem::Access::Read, [&](StringView path, FileFlags flags) {
		auto data = data::readFile<Interface>(FileInfo(path));
		if (data && data.isArray("paths")) {
			for (auto &it : data.getArray("paths")) {
				candidate = it.getString();
				if (checkCandidateDir(candidate)) {
					if (!cb(candidate)) {
						return false;
					}
				}
			}
		}
		return true;
	});

	// try system-wide configs
	if (filesystem::exists(FileInfo(SYSTEM_WIDE_PROFILE_D_FILE))) {
		auto fileData = filesystem::readIntoMemory<Interface>(FileInfo(SYSTEM_WIDE_PROFILE_D_FILE));
		auto strData = BytesView(fileData).readString();

		candidate = getCandidateFromShellScript(strData);
		if (checkCandidateDir(candidate)) {
			if (!cb(candidate)) {
				return;
			}
		}
	}

	if (filesystem::exists(FileInfo(SYSTEM_WIDE_ENVIRONMENT_D_FILE))) {
		auto fileData =
				filesystem::readIntoMemory<Interface>(FileInfo(SYSTEM_WIDE_ENVIRONMENT_D_FILE));
		auto strData = BytesView(fileData).readString();

		candidate = getCandidateFromEnvironmentConfig(strData);
		if (checkCandidateDir(candidate)) {
			if (!cb(candidate)) {
				return;
			}
		}
	}

	// assume launch from repo
	auto appPath = filesystem::platform::_getApplicationPath<Interface>();
	auto pathComponents = filepath::split<Interface>(appPath);
	std::reverse(pathComponents.begin(), pathComponents.end());

	auto it = std::find(pathComponents.begin(), pathComponents.end(), "libstappler-root");
	if (it != pathComponents.end()) {
		pathComponents.erase(pathComponents.begin(), it);
		pathComponents.emplace_back("/");

		std::reverse(pathComponents.begin(), pathComponents.end());

		auto reconstructedPath = filepath::merge<Interface>(pathComponents);

		candidate = filepath::merge<Interface>(reconstructedPath, "build/make");

		if (checkCandidateDir(candidate)) {
			if (!cb(candidate)) {
				return;
			}
		}
	}

	// check for a stappler-build/host/stappler-build
	auto preInstallDir = filepath::root(appPath, 6).str<Interface>();

	candidate = filepath::merge<Interface>(preInstallDir, "build/make");

	if (checkCandidateDir(candidate)) {
		if (!cb(candidate)) {
			return;
		}
	}

	// check for a stappler-build/host/debug/gcc/stappler-build
	auto postBuildDir = filepath::root(preInstallDir, 2).str<Interface>();
	candidate = filepath::merge<Interface>(postBuildDir, "build/make");
	if (checkCandidateDir(candidate)) {
		cb(candidate);
	}
}

static String getBuildRoot() {
	String ret;
	findStapplerBuildRoot([&](StringView str) {
		ret = str.str<Interface>();
		return false;
	});
	return ret;
}

static void printDocumentTableOfContents(const document::DocumentContentRecord &rec,
		uint32_t depth) {
	for (size_t i = 0; i < depth; ++i) { std::cout << "\t"; }

	std::cout << rec.label << " (" << rec.href << ")\n";

	for (auto &it : rec.childs) { printDocumentTableOfContents(it, depth + 1); }
}

static bool openAndReadDocument(FileInfo fileinfo) {
	auto doc = document::Document::open(fileinfo);
	if (doc) {
		std::cout << "Document: " << doc->getName() << "\n";

		std::cout << "Spine:\n";
		for (auto &it : doc->getSpine()) { std::cout << "\t" << it.file << "\n"; }

		std::cout << "Table of contents:\n";
		printDocumentTableOfContents(doc->getTableOfContents(), 1);

		std::cout << "Pages:\n";
		doc->foreachPage([](StringView str, const document::PageContainer *page) {
			std::cout << "" << page->getPath() << " \"" << page->getTitle() << "\"\n";
			std::cout << "\tMeta:\n";
			page->foreachMeta([](StringView key, StringView value) {
				std::cout << "\t\t" << key << ": " << value << "\n";
			});

			std::cout << "\tHttpEquiv:\n";
			page->foreachHttpEquiv([](StringView key, StringView value) {
				std::cout << "\t\t" << key << ": " << value << "\n";
			});

			std::cout << "\tLinks:\n";
			for (auto &it : page->getStyleLinks()) { std::cout << "\t\t" << it.href << "\n"; }

			std::cout << "\tAssets:\n";
			for (auto &it : page->getAssets()) { std::cout << "\t\t" << it << "\n"; }
		});

		return true;
	}
	return false;
}

static int printDocumentInfo(StringView path) {
	filesystem::Stat stat;
	if (filesystem::stat(FileInfo(path), stat)) {
		if (stat.type == FileType::File) {
			openAndReadDocument(FileInfo(path));
		} else if (stat.type == FileType::Dir) {
			filesystem::ftw(FileInfo(path), [](const FileInfo &info, FileType type) {
				if (type == FileType::File) {
					std::cout << "--- " << info.path << "----\n";
					openAndReadDocument(info);
				}
				return true;
			});
		}
	} else {
		slog().error("buildtool", "Fail to open document: ", path, ": not exists");
	}

	return 0;
}

SP_EXTERN_C int main(int argc, const char *argv[]) {
	if (argc < 2) {
		std::cerr << "Invalid arguments!\n\n";
		printHelp();
		return -1;
	}

	//abi::initialize(argc, argv);

	int nextArg = 1;

	auto action = StringView(argv[nextArg++]);

	// проверяем, запрошена ли помощь
	if (action == "help") {
		printHelp();
		return 0;
	}

	return perform_main(argc, argv, [&]() -> int {
		if (action == "localeinfo") {
			if (nextArg < argc) {
				auto filePath = StringView(argv[nextArg++]).str<memory::StandartInterface>();

				if (!filepath::isAbsolute(filePath)) {
					// note that filesystem::currentDir argument can not point above current dir's root
					filePath = filepath::reconstructPath<Interface>(
							filepath::merge<memory::StandartInterface>(
									filesystem::currentDir<memory::StandartInterface>(), filePath));
				}

				if (filesystem::exists(FileInfo{filePath})) {
					if (!buildLocaleInfo(FileInfo{filePath})) {
						return -1;
					}
					return 0;
				} else {
					log::source().error("main", "File not found: ", argv[nextArg++]);
					return -1;
				}
			} else {
				if (!buildLocaleInfoFromNetwork()) {
					return -1;
				}
				return 0;
			}
		} else if (action == "list") {
			Vector<String> candidates;
			findStapplerBuildRoot([&](StringView str) {
				auto it = std::find(candidates.begin(), candidates.end(), str);
				if (it == candidates.end()) {
					candidates.emplace_back(str.str<Interface>());
				}
				return true;
			});

			if (candidates.empty()) {
				std::cerr << "No SDK candidates found\n";
				return -2;
			} else {
				for (auto &it : candidates) { std::cout << it << "\n"; }
			}
			return 0;
		} else if (action == "add") {
			if (argc < 3) {
				std::cerr << "Invalid arguments!\n\n";
				return -1;
			}

			auto buildPath = String(argv[nextArg++]);

			if (!filepath::isAbsolute(buildPath)) {
				buildPath = filesystem::currentDir<Interface>(buildPath);
			}

			if (!checkCandidateDir(buildPath)) {
				std::cerr << "Invalid target path\n\n";
				return -1;
			}

			auto path = filesystem::findPath<Interface>(USER_SDK_PROFILE,
					FileCategory::CommonConfig, FileFlags::MakeWritableDir);
			if (!path.empty()) {
				if (filesystem::exists(FileInfo{path})) {
					auto data = data::readFile<Interface>(FileInfo{path});
					Set<String> paths;
					auto &arr = data.getArray("paths");
					for (auto &it : arr) { paths.emplace(it.getString()); }

					if (!paths.emplace(buildPath).second) {
						std::cout << "Already exists: " << buildPath << "\n";
						return 0;
					}

					arr.clear();
					for (auto &it : paths) {
						if (!it.empty()) {
							arr.emplace_back(Value(it));
						}
					}
					data::save(data, FileInfo{path});
				} else {
					Value data;
					data.emplace("paths").addString(buildPath);
					data::save(data, FileInfo{path});
				}
			} else {
				std::cerr << "Invalid target path\n\n";
				return -1;
			}
			std::cout << "Added: " << buildPath << "\n";
			return 0;
		} else if (action == "get-root") {
			auto root = getBuildRoot();
			if (!root.empty()) {
				std::cout << root << "\n";
			} else {
				std::cerr << "No SDK candidates found\n";
				return -2;
			}
		} else if (action == "make") {
			auto root = getBuildRoot();
			if (root.empty()) {
				std::cerr << "No SDK candidates found\n";
				return -2;
			}

			::setenv("STAPPLER_BUILD_ROOT", root.data(), 1);

			StringView makeTool("make");

			if (argc > nextArg) {
				auto extraArg = StringView(argv[nextArg]);
				if (extraArg.starts_with("--with=")) {
					extraArg += "--with="_len;
					makeTool = extraArg;
					++nextArg;
				} else if (extraArg == "--with" && argc > nextArg + 1) {
					makeTool = StringView(argv[nextArg + 1]);
					nextArg += 2;
				}
			}

			char **argumentList =
					(char **)memory::pool::palloc(memory::pool::acquire(), argc * sizeof(char *));

			memset(argumentList, 0, argc * sizeof(char *));

			auto path = filepath::isAbsolute(makeTool)
					? makeTool.str<Interface>()
					: filesystem::findPath<memory::StandartInterface>(
							  FileInfo{makeTool, FileCategory::Exec});

			argumentList[0] = path.data();

			auto argPtr = &argumentList[1];
			for (size_t i = nextArg; i < size_t(argc); ++i) { *argPtr++ = (char *)argv[i]; }

			return execvp(path.data(), argumentList);
		} else if (action == "extract") {
			auto root = getBuildRoot();
			if (root.empty()) {
				std::cerr << "No SDK candidates found\n";
				return -2;
			}

			auto platformTestMake = Rc<makefile::MakefileRef>::create();

			platformTestMake->assignSimpleVariable("STAPPLER_BUILD_ROOT",
					makefile::Origin::CommandLine, root);
			platformTestMake->assignSimpleVariable("SPBUILDTOOL", makefile::Origin::CommandLine,
					"1");

			platformTestMake->include(FileInfo{"Makefile"});

			String str;

			platformTestMake->eval([&](StringView s) { str.append(s.data(), s.size()); }, "<eval>",
					"$(print $(call sp_detect_platform,host))");

			auto make = Rc<makefile::MakefileRef>::create();
			make->assignSimpleVariable("STAPPLER_BUILD_ROOT", makefile::Origin::CommandLine, root);
			make->assignSimpleVariable("SPBUILDTOOL", makefile::Origin::CommandLine, "1");

			StringView(str).split<StringView::WhiteSpace>([&](StringView val) {
				auto name = val.readUntil<StringView::Chars<'='>>();
				if (val.is('=')) {
					++val;
					make->assignSimpleVariable(name, makefile::Origin::CommandLine, val);
					std::cout << "PLATFORM: '" << name << " = " << val << "'\n";
				}
			});

			make->include(FileInfo{"Makefile"});
			return 0;
		} else if (action == "xcodegen") {
			auto root = getBuildRoot();
			if (root.empty()) {
				std::cerr << "No SDK candidates found\n";
				return -2;
			}

			if (argc < 3) {
				std::cerr << "Invalid arguments!\n\n";
				return -1;
			}

			auto projPath = StringView(argv[nextArg++]).str<memory::StandartInterface>();

			filesystem::Stat stat;
			if (!filesystem::stat(FileInfo{projPath}, stat)) {
				std::cerr << "Invalid path to project:" << projPath << "\n";
				return -4;
			}

			if (!filepath::isAbsolute(projPath)) {
				// note that filesystem::currentDir argument can not point above current dir's root
				projPath = filepath::reconstructPath<Interface>(
						filepath::merge<memory::StandartInterface>(
								filesystem::currentDir<memory::StandartInterface>(), projPath));
			}

			if (stat.type == FileType::File) {
				if (!makeXCodeProject(root, FileInfo(projPath))) {
					return -3;
				}
			} else if (stat.type == FileType::Dir) {
				if (!makeXCodeProject(root,
							FileInfo(filepath::merge<Interface>(projPath, "Makefile")))) {
					return -3;
				}
			}

			return 0;
		} else if (action == "docinfo") {
			auto path = StringView(argv[nextArg++]).str<memory::StandartInterface>();

			if (!filepath::isAbsolute(path)) {
				// note that filesystem::currentDir argument can not point above current dir's root
				path = filepath::reconstructPath<Interface>(
						filepath::merge<memory::StandartInterface>(
								filesystem::currentDir<memory::StandartInterface>(), path));
			}

			return printDocumentInfo(path);
		} else {
			std::cerr << "Unknown action: \"" << action << "\"\n";
		}

		return -3;
	});
}

} // namespace stappler::buildtool
