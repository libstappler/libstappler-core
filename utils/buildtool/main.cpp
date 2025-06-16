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

#include "SPCommon.h" // << Prefix header
#include "SPBytesView.h"
#include "SPFilepath.h"
#include "SPFilesystem.h"
#include "SPMemInterface.h"
#include "SPMemPoolInterface.h"
#include "SPMemory.h"

#include "SPMakefile.h"
#include "XCodeProject.h"

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
	help - show this message
)";

static constexpr StringView SYSTEM_WIDE_PROFILE_D_FILE = "/etc/profile.d/stappler-sdk.sh";
static constexpr StringView SYSTEM_WIDE_ENVIRONMENT_D_FILE =
		"/etc/environment.d/50stappler-sdk.conf";

static constexpr StringView USER_ENVIRONMENT_D_FILE = ".config/environment.d/50stappler-sdk.conf";
static constexpr StringView USER_PROFILE_D_FILE = ".config/profile.d/stappler-sdk.sh";

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

	auto home = StringView(::getenv("HOME"));
	if (!home.empty()) {
		auto profilePath = filepath::merge<Interface>(home, USER_PROFILE_D_FILE);
		if (filesystem::exists(FileInfo{profilePath})) {
			auto fileData = filesystem::readIntoMemory<Interface>(FileInfo{profilePath});
			auto strData = BytesView(fileData).readString();

			candidate = getCandidateFromShellScript(strData);
			if (checkCandidateDir(candidate)) {
				if (!cb(candidate)) {
					return;
				}
			}
		}

		auto envPath = filepath::merge<Interface>(home, USER_ENVIRONMENT_D_FILE);
		if (filesystem::exists(FileInfo{envPath})) {
			auto fileData = filesystem::readIntoMemory<Interface>(FileInfo{envPath});
			auto strData = BytesView(fileData).readString();

			candidate = getCandidateFromEnvironmentConfig(strData);
			if (checkCandidateDir(candidate)) {
				if (!cb(candidate)) {
					return;
				}
			}
		}
	}

	// try system-wide configs
	if (filesystem::exists(SYSTEM_WIDE_PROFILE_D_FILE)) {
		auto fileData = filesystem::readIntoMemory<Interface>(SYSTEM_WIDE_PROFILE_D_FILE);
		auto strData = BytesView(fileData).readString();

		candidate = getCandidateFromShellScript(strData);
		if (checkCandidateDir(candidate)) {
			if (!cb(candidate)) {
				return;
			}
		}
	}

	if (filesystem::exists(SYSTEM_WIDE_ENVIRONMENT_D_FILE)) {
		auto fileData = filesystem::readIntoMemory<Interface>(SYSTEM_WIDE_ENVIRONMENT_D_FILE);
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

SP_EXTERN_C int main(int argc, const char *argv[]) {
	if (argc < 2) {
		std::cerr << "Invalid arguments!\n\n";
		printHelp();
		return -1;
	}

	size_t nextArg = 1;

	auto action = StringView(argv[nextArg++]);

	// проверяем, запрошена ли помощь
	if (action == "help") {
		printHelp();
		return 0;
	}

	return perform_main([&]() -> int {
		if (action == "list") {
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

			if (size_t(argc) > nextArg) {
				auto extraArg = StringView(argv[nextArg]);
				if (extraArg.starts_with("--with=")) {
					extraArg += "--with="_len;
					makeTool = extraArg;
					++nextArg;
				} else if (extraArg == "--with" && size_t(argc) > nextArg + 1) {
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

			auto projPath = StringView(argv[nextArg++]);

			if (!makeXCodeProject(root, FileInfo(projPath))) {
				return -3;
			}

			return 0;
		} else {
			std::cerr << "Unknown action: \"" << action << "\"\n";
		}

		return -3;
	});
}

} // namespace stappler::buildtool
