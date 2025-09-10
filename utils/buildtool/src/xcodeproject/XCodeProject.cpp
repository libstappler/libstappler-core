/**
 Copyright (c) 2025 Stappler Team <admin@stappler.org>

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

#include "XCodeProject.h"
#include "BuildConfig.h"
#include "SPBytesReader.h"
#include "SPFilepath.h"
#include "SPFilesystem.h"
#include "xcode/SPPBXObject.h"
#include "xcode/SPPBXFile.h"
#include "xcode/SPPBXProject.h"
#include "xcode/SPPBXTarget.h"
#include "xcode/SPXCodeProject.h"
#include <random>

namespace stappler::buildtool {

struct MakefileConfig {
	makefile::String std;
	makefile::String stdxx;
	makefile::String depTarget;
	makefile::String bundleName;
	makefile::String frameworkRoot;
	makefile::String localOutdir;
	makefile::String libs;
	makefile::String cflags;
	makefile::String cxxflags;
	makefile::String cflagsExec;
	makefile::String cxxflagsExec;
	makefile::String cflagsLib;
	makefile::String cxxflagsLib;

	makefile::String headerPaths;
	makefile::String ldflags;
	makefile::String ldflagsExec;
	makefile::String ldflagsLib;

	void load(makefile::Makefile *make);
};

struct XCodeProject {
	makefile::xcode::XCodeExport xctx;
	Rc<makefile::MakefileRef> release;
	Rc<makefile::MakefileRef> debug;
	StringView frameworkPath;

	makefile::Map<const makefile::xcode::PBXFileSystemSynchronizedRootGroup *,
			makefile::Vector<makefile::String>>
			sourceDirs;
	makefile::Vector<const makefile::xcode::PBXFileReference *> sourceFiles;

	MakefileConfig releaseConfig;
	MakefileConfig debugConfig;
};

static constexpr auto s_workspaceData =
		R"(<?xml version="1.0" encoding="UTF-8"?>
<Workspace
   version = "1.0">
   <FileRef
	  location = "self:">
   </FileRef>
</Workspace>
)";

const makefile::xcode::XCBuildConfiguration *makeProjectConfiguration(makefile::Makefile *make,
		makefile::xcode::XCodeExport &xctx, StringView name, bool debug) {
	using namespace makefile;
	return xcode::XCBuildConfiguration::create(xctx, [&](xcode::XCBuildConfiguration *list) {
		list->name = name.str<Interface>();

		list->buildSettings.emplace("ALWAYS_SEARCH_USER_PATHS", false);
		list->buildSettings.emplace("ASSETCATALOG_COMPILER_GENERATE_SWIFT_ASSET_SYMBOL_EXTENSIONS",
				true);
		list->buildSettings.emplace("CLANG_ANALYZER_NONNULL", true);
		list->buildSettings.emplace("CLANG_ANALYZER_NUMBER_OBJECT_CONVERSION", "YES_AGGRESSIVE");
		list->buildSettings.emplace("CLANG_ENABLE_MODULES", true);
		list->buildSettings.emplace("CLANG_ENABLE_OBJC_ARC", true);
		list->buildSettings.emplace("CLANG_ENABLE_OBJC_WEAK", true);
		list->buildSettings.emplace("CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING", true);
		list->buildSettings.emplace("CLANG_WARN_BOOL_CONVERSION", true);
		list->buildSettings.emplace("CLANG_WARN_COMMA", true);
		list->buildSettings.emplace("CLANG_WARN_CONSTANT_CONVERSION", true);
		list->buildSettings.emplace("CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS", true);
		list->buildSettings.emplace("CLANG_WARN_DIRECT_OBJC_ISA_USAGE", "YES_ERROR");
		list->buildSettings.emplace("CLANG_WARN_DOCUMENTATION_COMMENTS", true);
		list->buildSettings.emplace("CLANG_WARN_EMPTY_BODY", true);
		list->buildSettings.emplace("CLANG_WARN_ENUM_CONVERSION", true);
		list->buildSettings.emplace("CLANG_WARN_INFINITE_RECURSION", true);
		list->buildSettings.emplace("CLANG_WARN_INT_CONVERSION", true);
		list->buildSettings.emplace("CLANG_WARN_NON_LITERAL_NULL_CONVERSION", true);
		list->buildSettings.emplace("CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF", true);
		list->buildSettings.emplace("CLANG_WARN_OBJC_LITERAL_CONVERSION", true);
		list->buildSettings.emplace("CLANG_WARN_OBJC_ROOT_CLASS", "YES_ERROR");
		list->buildSettings.emplace("CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER", true);
		list->buildSettings.emplace("CLANG_WARN_RANGE_LOOP_ANALYSIS", true);
		list->buildSettings.emplace("CLANG_WARN_STRICT_PROTOTYPES", true);
		list->buildSettings.emplace("CLANG_WARN_SUSPICIOUS_MOVE", true);
		list->buildSettings.emplace("CLANG_WARN_UNGUARDED_AVAILABILITY", "YES_AGGRESSIVE");
		list->buildSettings.emplace("CLANG_WARN_UNREACHABLE_CODE", true);
		list->buildSettings.emplace("CLANG_WARN__DUPLICATE_METHOD_MATCH", true);
		list->buildSettings.emplace("COPY_PHASE_STRIP", false);
		list->buildSettings.emplace("CURRENT_PROJECT_VERSION", 1);
		list->buildSettings.emplace("ENABLE_STRICT_OBJC_MSGSEND", true);
		list->buildSettings.emplace("ENABLE_USER_SCRIPT_SANDBOXING", true);
		list->buildSettings.emplace("GCC_NO_COMMON_BLOCKS", true);
		list->buildSettings.emplace("GCC_WARN_64_TO_32_BIT_CONVERSION", true);
		list->buildSettings.emplace("GCC_WARN_ABOUT_RETURN_TYPE", "YES_ERROR");
		list->buildSettings.emplace("GCC_WARN_UNDECLARED_SELECTOR", true);
		list->buildSettings.emplace("GCC_WARN_UNINITIALIZED_AUTOS", "YES_AGGRESSIVE");
		list->buildSettings.emplace("GCC_WARN_UNUSED_FUNCTION", true);
		list->buildSettings.emplace("GCC_WARN_UNUSED_VARIABLE", true);
		list->buildSettings.emplace("LOCALIZATION_PREFERS_STRING_CATALOGS", true);
		list->buildSettings.emplace("MTL_FAST_MATH", true);
		list->buildSettings.emplace("VERSIONING_SYSTEM", "apple-generic");
		list->buildSettings.emplace("VERSION_INFO_PREFIX", "");

		auto std = getVariable(make, "GLOBAL_STD");
		auto stdxx = getVariable(make, "GLOBAL_STDXX");

		if (!std.empty()) {
			list->buildSettings.emplace("GCC_C_LANGUAGE_STANDARD", std);
		}
		if (!stdxx.empty()) {
			list->buildSettings.emplace("CLANG_CXX_LANGUAGE_STANDARD", stdxx);
		}

		if (debug) {
			list->buildSettings.emplace("DEBUG_INFORMATION_FORMAT", "dwarf");
			list->buildSettings.emplace("ENABLE_TESTABILITY", true);
			list->buildSettings.emplace("GCC_DYNAMIC_NO_PIC", false);
			list->buildSettings.emplace("GCC_OPTIMIZATION_LEVEL", 0);
			list->buildSettings.emplace("MTL_ENABLE_DEBUG_INFO", "INCLUDE_SOURCE");
			list->buildSettings.emplace("ONLY_ACTIVE_ARCH", true);
			list->buildSettings.emplace("SWIFT_ACTIVE_COMPILATION_CONDITIONS",
					"DEBUG $(inherited)");
			list->buildSettings.emplace("SWIFT_OPTIMIZATION_LEVEL", "-Onone");
		} else {
			list->buildSettings.emplace("DEBUG_INFORMATION_FORMAT", "dwarf-with-dsym");
			list->buildSettings.emplace("ENABLE_NS_ASSERTIONS", false);
			list->buildSettings.emplace("MTL_ENABLE_DEBUG_INFO", false);
			list->buildSettings.emplace("SWIFT_COMPILATION_MODE", "wholemodule");
		}
	});
}

static void extractLibFlags(StringView flags, const CallbackStream &cb) {
	using namespace makefile;

	Vector<StringView> values;

	StringView(flags).split<StringView::WhiteSpace>([&](StringView v) { values.emplace_back(v); });

	Set<StringView> frameworks;
	Set<StringView> libs;

	// extract frameworks and system libs
	auto lIt = values.begin();
	auto lEnd = values.end();
	while (lIt != lEnd) {
		if (*lIt == "-framework") {
			auto tmp = lIt;
			++tmp;
			auto name = *tmp;
			if (frameworks.find(name) == frameworks.end()) {
				cb << "-framework" << name;
				frameworks.emplace(StringView(name));
			}
			++lIt;
			++lIt;
		} else if (lIt->starts_with("-l")) {
			auto libname = lIt->sub(2);
			if (libs.find(libname) == libs.end()) {
				cb << toString("-l", libname);
				libs.emplace(libname);
			}
			++lIt;
		} else {
			cb << *lIt;
			++lIt;
		}
	}
}

static const makefile::xcode::XCBuildConfiguration *makeMacOsFrameworkConfiguration(
		XCodeProject &xproj, StringView name, bool debug) {
	using namespace makefile;
	return xcode::XCBuildConfiguration::create(xproj.xctx, [&](xcode::XCBuildConfiguration *list) {
		list->name = name.str<Interface>();

		list->buildSettings.emplace("CODE_SIGN_STYLE", "Automatic");
		list->buildSettings.emplace("COMBINE_HIDPI_IMAGES", true);
		list->buildSettings.emplace("CURRENT_PROJECT_VERSION", 1);
		list->buildSettings.emplace("DEFINES_MODULE", false);
		list->buildSettings.emplace("DYLIB_COMPATIBILITY_VERSION", 1);
		list->buildSettings.emplace("DYLIB_CURRENT_VERSION", 1);
		list->buildSettings.emplace("DYLIB_INSTALL_NAME_BASE", "@rpath");
		list->buildSettings.emplace("DEAD_CODE_STRIPPING", true);
		list->buildSettings.emplace("ENABLE_MODULE_VERIFIER", true);
		list->buildSettings.emplace("GENERATE_INFOPLIST_FILE", true);
		list->buildSettings.emplace("INFOPLIST_KEY_NSHumanReadableCopyright", "");
		list->buildSettings.emplace("INSTALL_PATH", "$(LOCAL_LIBRARY_DIR)/Frameworks");
		list->buildSettings.emplace("MACH_O_TYPE", "staticlib");
		list->buildSettings.emplace("MARKETING_VERSION", "1.0");
		list->buildSettings.emplace("MODULE_VERIFIER_SUPPORTED_LANGUAGES",
				"objective-c objective-c++");
		list->buildSettings.emplace("SDKROOT", "macosx");
		list->buildSettings.emplace("SKIP_INSTALL", true);
		list->buildSettings.emplace("SWIFT_EMIT_LOC_STRINGS", true);
		list->buildSettings.emplace("PRODUCT_NAME", "$(TARGET_NAME:c99extidentifier)");
		list->buildSettings.emplace("SUPPORTED_PLATFORMS", "macosx");

		auto make = debug ? xproj.debug : xproj.release;
		auto &config = debug ? xproj.debugConfig : xproj.releaseConfig;

		list->buildSettings.emplace("MODULE_VERIFIER_SUPPORTED_LANGUAGE_STANDARDS",
				toString(config.std, " ", config.stdxx));
		list->buildSettings.emplace("MACOSX_DEPLOYMENT_TARGET", config.depTarget);
		list->buildSettings.emplace("PRODUCT_BUNDLE_IDENTIFIER",
				toString(config.bundleName, ".framework"));

		// process headers paths
		Value targetHeaderPaths;

		if (debug) {
			targetHeaderPaths.addString("$(PROJECT_DIR)/debug/include");
		} else {
			targetHeaderPaths.addString("$(PROJECT_DIR)/release/include");
		}

		StringView(config.headerPaths).split<StringView::WhiteSpace>([&](StringView path) {
			if (path.starts_with(StringView(config.frameworkRoot))) {
				targetHeaderPaths.addString(
						toString("$(STAPPLER_ROOT)", path.sub(config.frameworkRoot.size())));
			} else if (path.starts_with(StringView(config.localOutdir))) {
				targetHeaderPaths.addString(
						toString("$(PROJECT_DIR)/..", path.sub(config.localOutdir.size())));
			} else {
				targetHeaderPaths.addString(path);
			}
		});

		targetHeaderPaths.addString("$(STAPPLER_ROOT)/deps/mac/$(CURRENT_ARCH)/include");

		list->buildSettings.emplace("HEADER_SEARCH_PATHS", targetHeaderPaths);

		// process libflags
		Value targetldflags;

		extractLibFlags(config.ldflags, [&](StringView str) { targetldflags.addString(str); });

		if (!targetldflags.empty()) {
			list->buildSettings.emplace("OTHER_LDFLAGS", targetldflags);
		}

		Value targetLibflags;
		StringView(config.libs).split<StringView::WhiteSpace>([&](StringView path) {
			targetLibflags.addString(path);
		});
		if (!targetLibflags.empty()) {
			list->buildSettings.emplace("OTHER_LIBTOOLFLAGS", targetLibflags);
		}

		Value targetcflags;
		StringView(config.cflags).split<StringView::WhiteSpace>([&](StringView path) {
			targetcflags.addString(path);
		});
		if (!targetcflags.empty()) {
			list->buildSettings.emplace("OTHER_CFLAGS", targetcflags);
		}

		Value targetcxxflags;
		StringView(config.cxxflags).split<StringView::WhiteSpace>([&](StringView path) {
			targetcxxflags.addString(path);
		});
		if (!targetcxxflags.empty()) {
			list->buildSettings.emplace("OTHER_CPLUSPLUSFLAGS", targetcxxflags);
		}

		list->buildSettings.emplace("LIBRARY_SEARCH_PATHS",
				Value{Value("/usr/local/lib"),
					Value("$(STAPPLER_ROOT)/deps/mac/$(CURRENT_ARCH)/lib")});

		list->buildSettings.emplace("STAPPLER_ROOT", config.frameworkRoot);
	});
}

static void makeXCodeMacOsTarget(XCodeProject &xproj, makefile::xcode::PBXNativeTarget *target) {
	using namespace makefile;

	target->buildPhases.emplace_back(
			xcode::PBXHeadersBuildPhase::create(xproj.xctx, [&](xcode::PBXHeadersBuildPhase *list) {

	}));

	target->buildPhases.emplace_back(
			xcode::PBXSourcesBuildPhase::create(xproj.xctx, [&](xcode::PBXSourcesBuildPhase *list) {
		for (auto &it : xproj.sourceDirs) {
			if (!it.second.empty()) {
				it.first->exceptions.emplace_back(
						xcode::PBXFileSystemSynchronizedBuildFileExceptionSet::create(xproj.xctx,
								[&](xcode::PBXFileSystemSynchronizedBuildFileExceptionSet *set) {
					set->membershipExceptions = it.second;
					set->target = target;
				}));
			}
		}
		for (auto &it : xproj.sourceFiles) {
			list->files.emplace_back(xcode::PBXBuildFile::create(xproj.xctx,
					[&](xcode::PBXBuildFile *file) { file->file = it; }));
		}
	}));

	target->buildPhases.emplace_back(xcode::PBXFrameworksBuildPhase::create(xproj.xctx,
			[&](xcode::PBXFrameworksBuildPhase *list) {

	}));

	target->buildPhases.emplace_back(xcode::PBXResourcesBuildPhase::create(xproj.xctx,
			[&](xcode::PBXResourcesBuildPhase *list) {

	}));

	target->buildConfigurationList =
			xcode::XCConfigurationList::create(xproj.xctx, [&](xcode::XCConfigurationList *list) {
		auto debug = makeMacOsFrameworkConfiguration(xproj, "Debug", true);
		auto release = makeMacOsFrameworkConfiguration(xproj, "Release", false);

		list->buildConfigurations.emplace_back(debug);
		list->buildConfigurations.emplace_back(release);

		list->defaultConfiguration = release;
	});

	target->name = "MacOsFramework";

	target->product =
			xcode::PBXFileReference::create(xproj.xctx, [](xcode::PBXFileReference *file) {
		file->explicitFileType = "wrapper.framework";
		file->path = "MacOsFramework.framework";
		file->includeInIndex = 0;
		file->sourceTree = xcode::PBXSourceTree{xcode::PBXSourceTree::buildProductsDir};
	});

	target->productType = xcode::PBXProductType::framework;
}


static void updateFile(filesystem::File &file, StringView path) {
	if (filesystem::exists(FileInfo(path))) {
		auto oldData = filesystem::readIntoMemory<makefile::Interface>(FileInfo(path));
		auto newData = file.readIntoMemory<makefile::Interface>();

		if (oldData != newData) {
			filesystem::remove(FileInfo(path));
			file.close_rename(FileInfo(path));
		}
	} else {
		file.close_rename(FileInfo(path));
	}
}

static void makeConfigs(StringView path, makefile::Makefile *make) {
	using namespace makefile;

	auto debugincludedir = filepath::merge<Interface>(path, "include");

	filesystem::mkdir_recursive(FileInfo(debugincludedir));

	auto buildHeader = filepath::merge<Interface>(debugincludedir, "stappler-buildconfig.h");
	auto buildHeaderFile = filesystem::File::open_tmp("stappler-buildconfig-h");
	makeBuildConfigHeader(make,
			[&](StringView str) { buildHeaderFile.xsputn(str.data(), str.size()); });
	updateFile(buildHeaderFile, buildHeader);

	auto appHeader = filepath::merge<Interface>(debugincludedir, "stappler-appconfig.h");
	auto appHeaderFile = filesystem::File::open_tmp("stappler-appconfig-h");
	makeAppConfigHeader(make,
			[&](StringView str) { appHeaderFile.xsputn(str.data(), str.size()); });
	updateFile(appHeaderFile, appHeader);
}

static const makefile::xcode::PBXGroup *makeModuleGroup(XCodeProject &xproj, StringView mod) {
	using namespace makefile;

	return xcode::PBXGroup::create(xproj.xctx, [&](xcode::PBXGroup *group) {
		group->sourceTree.type = xcode::PBXSourceTree::group;
		group->name = mod.str<Interface>();

		StringView(getVariable(xproj.release, toString("$(MODULE_", mod, ")_SRCS_DIRS")))
				.split<StringView::WhiteSpace>([&](StringView str) {
			auto path = filepath::reconstructPath<Interface>(str);
			auto g = xcode::PBXFileSystemSynchronizedRootGroup::create(xproj.xctx,
					[&](xcode::PBXFileSystemSynchronizedRootGroup *g) {
				g->name = filepath::lastComponent(str).str<Interface>();
				g->path = path;
			});

			group->children.emplace_back(g);
			xproj.sourceDirs.emplace(g, Vector<String>());
		});

		auto files = getExpression(xproj.release,
				toString("$(call sp_toolkit_source_list, $($(MODULE_", mod,
						")_SRCS_DIRS), $($(MODULE_", mod, ")_SRCS_OBJS))"));

		StringView(files).split<StringView::WhiteSpace>([&](StringView str) {
			auto path = filepath::reconstructPath<Interface>(str);
			bool addedAsException = false;
			for (auto &it : xproj.sourceDirs) {
				if (StringView(path).starts_with(StringView(it.first->path))) {
					// add exception
					it.second.emplace_back(path.substr(it.first->path.size() + 1));
					addedAsException = true;
					break;
				}
			}
			if (!addedAsException) {
				auto fileRef = xcode::PBXFileReference::create(xproj.xctx,
						[&](xcode::PBXFileReference *file) {
					file->name = filepath::lastComponent(str).str<Interface>();
					file->path = str.str<Interface>();
					file->sourceTree.type = xcode::PBXSourceTree::absolute;
				});
				group->children.emplace_back(fileRef);
				xproj.sourceFiles.emplace_back(fileRef);
			}
		});
	});
}

static const makefile::xcode::PBXGroup *makeProjectGroup(XCodeProject &xproj) {
	using namespace makefile;

	return xcode::PBXGroup::create(xproj.xctx, [&](xcode::PBXGroup *group) {
		group->sourceTree.type = xcode::PBXSourceTree::group;
		group->name = "project";

		StringView(getExpression(xproj.release, "$(realpath $(LOCAL_SRCS_DIRS))"))
				.split<StringView::WhiteSpace>([&](StringView str) {
			auto path = filepath::reconstructPath<Interface>(str);
			auto g = xcode::PBXFileSystemSynchronizedRootGroup::create(xproj.xctx,
					[&](xcode::PBXFileSystemSynchronizedRootGroup *g) {
				g->name = filepath::lastComponent(str).str<Interface>();
				g->path = path;
			});

			group->children.emplace_back(g);
			xproj.sourceDirs.emplace(g, Vector<String>());
		});

		auto files = getExpression(xproj.release,
				"$(call sp_local_source_list,$(LOCAL_SRCS_DIRS),$(LOCAL_SRCS_OBJS))");

		StringView(files).split<StringView::WhiteSpace>([&](StringView str) {
			auto path = filepath::reconstructPath<Interface>(str);
			bool addedAsException = false;
			for (auto &it : xproj.sourceDirs) {
				if (StringView(path).starts_with(StringView(it.first->path))) {
					// add exception
					it.second.emplace_back(path.substr(it.first->path.size() + 1));
					addedAsException = true;
					break;
				}
			}
			if (!addedAsException) {
				auto fileRef = xcode::PBXFileReference::create(xproj.xctx,
						[&](xcode::PBXFileReference *file) {
					file->name = filepath::lastComponent(str).str<Interface>();
					file->path = str.str<Interface>();
					file->sourceTree.type = xcode::PBXSourceTree::absolute;
				});
				group->children.emplace_back(fileRef);
				xproj.sourceFiles.emplace_back(fileRef);
			}
		});

		/*auto localMain = getExpression(xproj.release, "$(realpath $(LOCAL_MAIN))");
		if (!localMain.empty()) {
			auto fileRef = xcode::PBXFileReference::create(xproj.xctx, [&] (xcode::PBXFileReference *file) {
				file->name = filepath::lastComponent(localMain).str<Interface>();
				file->path = localMain;
				file->sourceTree.type = xcode::PBXSourceTree::absolute;
			});
			group->children.emplace_back(fileRef);
			xproj.sourceFiles.emplace_back(fileRef);
		}*/

		auto fileRef =
				xcode::PBXFileReference::create(xproj.xctx, [&](xcode::PBXFileReference *file) {
			file->name = "stappler-appconfig.cpp";
			file->path = "src/stappler-appconfig.cpp";
			file->sourceTree.type = xcode::PBXSourceTree::sourceRoot;
		});
		group->children.emplace_back(fileRef);
		xproj.sourceFiles.emplace_back(fileRef);
	});
}

static void writeXCConfig(XCodeProject &xproj, const CallbackStream &cb) {
	using namespace makefile;

	cb << "//Autogenerated file\n\n";

	auto writeValue = [&](StringView name, StringView debug, StringView release) {
		if (debug == release) {
			cb << name << " = " << release << "\n";
		} else {
			cb << name << "[config=Debug] = " << debug << "\n";
			cb << name << "[config=Release] = " << release << "\n";
		}
	};

	writeValue("STAPPLER_STD", xproj.debugConfig.std, xproj.releaseConfig.std);
	writeValue("STAPPLER_STDXX", xproj.debugConfig.stdxx, xproj.releaseConfig.stdxx);
	writeValue("STAPPLER_MACOSX_DEPLOYMENT_TARGET", xproj.debugConfig.depTarget,
			xproj.releaseConfig.depTarget);
	writeValue("STAPPLER_BUNDLE_NAME", xproj.debugConfig.bundleName,
			xproj.releaseConfig.bundleName);
	writeValue("STAPPLER_ROOT", xproj.debugConfig.frameworkRoot, xproj.releaseConfig.frameworkRoot);
	writeValue("STAPPLER_OUTDIR", xproj.debugConfig.localOutdir, xproj.releaseConfig.localOutdir);
	writeValue("STAPPLER_LIBS", xproj.debugConfig.libs, xproj.releaseConfig.libs);
	writeValue("STAPPLER_GENERAL_CFLAGS", xproj.debugConfig.cflags, xproj.releaseConfig.cflags);
	writeValue("STAPPLER_GENERAL_CXXFLAGS", xproj.debugConfig.cxxflags,
			xproj.releaseConfig.cxxflags);
	writeValue("STAPPLER_EXEC_CFLAGS", xproj.debugConfig.cflagsExec,
			xproj.releaseConfig.cflagsExec);
	writeValue("STAPPLER_EXEC_CXXFLAGS", xproj.debugConfig.cxxflagsExec,
			xproj.releaseConfig.cxxflagsExec);
	writeValue("STAPPLER_LIB_CFLAGS", xproj.debugConfig.cflagsLib, xproj.releaseConfig.cflagsLib);
	writeValue("STAPPLER_LIB_CXXFLAGS", xproj.debugConfig.cxxflagsLib,
			xproj.releaseConfig.cxxflagsLib);

	cb << "STAPPLER_CONFIG_INCLUDE[config=Release] = "
		  "$(PROJECT_DIR)/$(STAPPLER_OUTDIR)/mac/release/include\n";
	cb << "STAPPLER_CONFIG_INCLUDE[config=Debug] = "
		  "$(PROJECT_DIR)/$(STAPPLER_OUTDIR)/mac/debug/include\n";

	if (xproj.debugConfig.headerPaths == xproj.releaseConfig.headerPaths) {
		Vector<String> targetHeaderPaths;
		StringView(xproj.releaseConfig.headerPaths)
				.split<StringView::WhiteSpace>([&](StringView path) {
			if (path.starts_with(StringView(xproj.releaseConfig.frameworkRoot))) {
				targetHeaderPaths.emplace_back(toString("$(STAPPLER_ROOT)",
						path.sub(xproj.releaseConfig.frameworkRoot.size())));
			} else if (path.starts_with(StringView(xproj.releaseConfig.localOutdir))) {
				targetHeaderPaths.emplace_back(toString("$(PROJECT_DIR)/$(STAPPLER_OUTDIR)",
						path.sub(xproj.releaseConfig.localOutdir.size())));
			} else {
				targetHeaderPaths.emplace_back(path.str<Interface>());
			}
		});

		targetHeaderPaths.emplace_back("$(STAPPLER_ROOT)/deps/mac/$(CURRENT_ARCH)/include");

		cb << "STAPPLER_HEADER_SEARCH_PATH = $(STAPPLER_CONFIG_INCLUDE)";
		for (auto &it : targetHeaderPaths) { cb << " " << it; }
		cb << "\n";
	} else {
		do {
			Vector<String> targetHeaderPaths;
			StringView(xproj.releaseConfig.headerPaths)
					.split<StringView::WhiteSpace>([&](StringView path) {
				if (path.starts_with(StringView(xproj.releaseConfig.frameworkRoot))) {
					targetHeaderPaths.emplace_back(toString("$(STAPPLER_ROOT)",
							path.sub(xproj.releaseConfig.frameworkRoot.size())));
				} else if (path.starts_with(StringView(xproj.releaseConfig.localOutdir))) {
					targetHeaderPaths.emplace_back(toString("$(PROJECT_DIR)/$(STAPPLER_OUTDIR)",
							path.sub(xproj.releaseConfig.localOutdir.size())));
				} else {
					targetHeaderPaths.emplace_back(path.str<Interface>());
				}
			});

			targetHeaderPaths.emplace_back("$(STAPPLER_ROOT)/deps/mac/$(CURRENT_ARCH)/include");

			cb << "STAPPLER_HEADER_SEARCH_PATH[config=Release] = $(STAPPLER_CONFIG_INCLUDE)";
			for (auto &it : targetHeaderPaths) { cb << " " << it; }
			cb << "\n";
		} while (0);

		do {
			Vector<String> targetHeaderPaths;
			StringView(xproj.debugConfig.headerPaths)
					.split<StringView::WhiteSpace>([&](StringView path) {
				if (path.starts_with(StringView(xproj.debugConfig.frameworkRoot))) {
					targetHeaderPaths.emplace_back(toString("$(STAPPLER_ROOT)",
							path.sub(xproj.debugConfig.frameworkRoot.size())));
				} else if (path.starts_with(StringView(xproj.debugConfig.localOutdir))) {
					targetHeaderPaths.emplace_back(toString("$(PROJECT_DIR)/$(STAPPLER_OUTDIR)",
							path.sub(xproj.debugConfig.localOutdir.size())));
				} else {
					targetHeaderPaths.emplace_back(path.str<Interface>());
				}
			});

			targetHeaderPaths.emplace_back("$(STAPPLER_ROOT)/deps/mac/$(CURRENT_ARCH)/include");

			cb << "STAPPLER_HEADER_SEARCH_PATH[config=Debug] = $(STAPPLER_CONFIG_INCLUDE)";
			for (auto &it : targetHeaderPaths) { cb << " " << it; }
			cb << "\n";
		} while (0);
	}

	if (xproj.debugConfig.ldflags == xproj.releaseConfig.ldflags) {
		cb << "STAPPLER_GENERAL_LDFLAGS =";
		extractLibFlags(xproj.releaseConfig.ldflags, [&](StringView str) { cb << " " << str; });
		cb << "\n";
	}

	if (xproj.debugConfig.ldflagsExec == xproj.releaseConfig.ldflagsExec) {
		cb << "STAPPLER_EXEC_LDFLAGS =";
		extractLibFlags(xproj.releaseConfig.ldflagsExec, [&](StringView str) { cb << " " << str; });
		cb << "\n";
	}

	if (xproj.debugConfig.ldflagsLib == xproj.releaseConfig.ldflagsLib) {
		cb << "STAPPLER_LIB_LDFLAGS =";
		extractLibFlags(xproj.releaseConfig.ldflagsLib, [&](StringView str) { cb << " " << str; });
		cb << "\n";
	}

	cb << "\n//Common section\n";
	cb << "CLANG_CXX_LANGUAGE_STANDARD = $(STAPPLER_STDXX)\n";
	cb << "GCC_C_LANGUAGE_STANDARD = $(STAPPLER_STD)\n";
	cb << "MODULE_VERIFIER_SUPPORTED_LANGUAGE_STANDARDS = $(STAPPLER_STD) $(STAPPLER_STDXX)\n";
	cb << "MACOSX_DEPLOYMENT_TARGET = $(STAPPLER_MACOSX_DEPLOYMENT_TARGET)\n";
	cb << "HEADER_SEARCH_PATHS = $(inherited) $(STAPPLER_HEADER_SEARCH_PATH)\n";
	cb << "LIBRARY_SEARCH_PATHS = $(inherited) /usr/local/lib "
		  "$(STAPPLER_ROOT)/deps/mac/$(CURRENT_ARCH)/lib\n";
	cb << "OTHER_LDFLAGS = $(STAPPLER_GENERAL_LDFLAGS)\n";
	cb << "OTHER_CFLAGS = $(STAPPLER_GENERAL_CFLAGS)\n";
	cb << "OTHER_CPLUSPLUSFLAGS = $(STAPPLER_GENERAL_CXXFLAGS)\n";
}

bool makeXCodeProject(StringView buildRoot, FileInfo projMakefilePath) {
	using namespace makefile;

	XCodeProject xproj;
	xproj.frameworkPath = buildRoot;

	xproj.release = Rc<MakefileRef>::create();
	xproj.release->setIncludeCallback(
			[](void *xproj, StringView path, const Makefile::PathCallback &cb) {
		Bytes data;
		if (!filepath::isAbsolute(path)) {
			auto filepath = filepath::merge<Interface>(
					reinterpret_cast<XCodeProject *>(xproj)->frameworkPath, path);
			data = filesystem::readIntoMemory<Interface>(FileInfo{filepath});
		} else {
			data = filesystem::readIntoMemory<Interface>(FileInfo{path});
		}
		cb(BytesView(data).toStringView());
	}, &xproj);

	xproj.release->assignSimpleVariable("RELEASE", makefile::Origin::CommandLine, "1");
	xproj.release->assignSimpleVariable("SPBUILDTOOL", makefile::Origin::CommandLine, "1");
	xproj.release->assignSimpleVariable("STAPPLER_TARGET", makefile::Origin::CommandLine, "host");
	xproj.release->assignSimpleVariable("MACOS", makefile::Origin::CommandLine, "1");
	xproj.release->assignSimpleVariable("STAPPLER_ARCH", makefile::Origin::CommandLine, "x86_64");
	xproj.release->assignSimpleVariable("STAPPLER_BUILD_ROOT", makefile::Origin::CommandLine,
			buildRoot);
	xproj.release->setRootPath(filepath::root(projMakefilePath));

	if (!xproj.release->include(projMakefilePath)) {
		log::source().error("XCodeProject", "Fail to load project Makefile");
		return false;
	}

	xproj.releaseConfig.load(xproj.release);

	xproj.debug = Rc<MakefileRef>::create();
	xproj.debug->setIncludeCallback(
			[](void *xproj, StringView path, const Makefile::PathCallback &cb) {
		Bytes data;
		if (!filepath::isAbsolute(path)) {
			auto filepath = filepath::merge<Interface>(
					reinterpret_cast<XCodeProject *>(xproj)->frameworkPath, path);
			data = filesystem::readIntoMemory<Interface>(FileInfo{filepath});
		} else {
			data = filesystem::readIntoMemory<Interface>(FileInfo{path});
		}
		cb(BytesView(data).toStringView());
	}, &xproj);
	xproj.debug->assignSimpleVariable("DEBUG", makefile::Origin::CommandLine, "1");
	xproj.debug->assignSimpleVariable("SPBUILDTOOL", makefile::Origin::CommandLine, "1");
	xproj.debug->assignSimpleVariable("STAPPLER_TARGET", makefile::Origin::CommandLine, "host");
	xproj.debug->assignSimpleVariable("MACOS", makefile::Origin::CommandLine, "1");
	xproj.debug->assignSimpleVariable("STAPPLER_ARCH", makefile::Origin::CommandLine, "x86_64");
	xproj.debug->assignSimpleVariable("STAPPLER_BUILD_ROOT", makefile::Origin::CommandLine,
			buildRoot);
	xproj.debug->setRootPath(filepath::root(projMakefilePath));

	if (!xproj.debug->include(projMakefilePath)) {
		log::source().error("XCodeProject", "Fail to load project Makefile");
		return false;
	}

	xproj.debugConfig.load(xproj.debug);

	auto path = filesystem::findPath<Interface>(filepath::root(projMakefilePath));

	String localOutdir = getVariable(xproj.release, "LOCAL_OUTDIR");
	String localExecutable = getVariable(xproj.release, "LOCAL_EXECUTABLE");
	String localLibrary = getVariable(xproj.release, "LOCAL_LIBRARY");

	if (localOutdir.empty() || (localExecutable.empty() && localLibrary.empty())) {
		log::source().error("XCodeProject", "Fail to detect build targets");
		return false;
	}

	auto projName = localExecutable.empty() ? localLibrary : localExecutable;
	auto outdir = filepath::merge<Interface>(path, localOutdir, "mac");

	auto debugdir = filepath::merge<Interface>(outdir, "debug");
	auto releasedir = filepath::merge<Interface>(outdir, "release");

	auto projPath = filepath::merge<Interface>(outdir, toString(projName, ".xcodeproj"));

	filesystem::mkdir_recursive(FileInfo(projPath));

	makeConfigs(debugdir, xproj.debug);
	makeConfigs(releasedir, xproj.release);

	auto srcdir = filepath::merge<Interface>(outdir, "src");
	filesystem::mkdir_recursive(FileInfo(srcdir));
	auto appSource = filepath::merge<Interface>(srcdir, "stappler-appconfig.cpp");
	auto appSourceFile = filesystem::File::open_tmp("stappler-appconfig-cpp");
	makeMergedAppConfigSource(xproj.release, xproj.debug,
			[&](StringView str) { appSourceFile.xsputn(str.data(), str.size()); });
	updateFile(appSourceFile, appSource);

	xproj.xctx.root = xcode::PBXProject::create(xproj.xctx, [&](xcode::PBXProject *proj) {
		proj->developmentRegion = "en";
		proj->hasScannedForEncodings = 0;
		proj->knownRegions = {"en", "Base"};
		proj->preferredProjectObjectVersion = 77;

		proj->mainGroup = xcode::PBXGroup::create(xproj.xctx, [&](xcode::PBXGroup *group) {
			// this will fill source files and dirs
			StringView(getVariable(xproj.release, "GLOBAL_MODULES"))
					.split<StringView::WhiteSpace>([&](StringView mod) {
				group->children.emplace_back(makeModuleGroup(xproj, mod));
			});

			group->children.emplace_back(makeProjectGroup(xproj));

			proj->targets.emplace_back(xcode::PBXNativeTarget::create(xproj.xctx,
					[&](xcode::PBXNativeTarget *target) { makeXCodeMacOsTarget(xproj, target); }));

			proj->productsGroup = xcode::PBXGroup::create(xproj.xctx, [&](xcode::PBXGroup *group) {
				for (auto &it : proj->targets) { group->children.emplace_back(it->product); }

				group->name = "Products";
			});
			group->children.emplace_back(proj->productsGroup);
		});

		proj->buildConfigurationList = xcode::XCConfigurationList::create(xproj.xctx,
				[&](xcode::XCConfigurationList *list) {
			auto debug = makeProjectConfiguration(xproj.debug, xproj.xctx, "Debug", true);
			auto release = makeProjectConfiguration(xproj.release, xproj.xctx, "Release", false);

			list->buildConfigurations.emplace_back(debug);
			list->buildConfigurations.emplace_back(release);

			list->defaultConfiguration = release;
		});
	});

	auto projFilePath = filepath::merge<Interface>(projPath, "project.pbxproj");
	auto pbxproj = filesystem::File::open_tmp("xcodeproj");
	xproj.xctx.write([&](StringView str) { pbxproj.xsputn(str.data(), str.size()); });
	updateFile(pbxproj, projFilePath);

	auto workspacePath = filepath::merge<Interface>(projPath, "project.xcworkspace");
	filesystem::mkdir(FileInfo(workspacePath));

	auto workspaceFile = filepath::merge<Interface>(workspacePath, "contents.xcworkspacedata");
	auto xcworkspacedata = filesystem::File::open_tmp("xcworkspacedata");
	xcworkspacedata.xsputn(s_workspaceData, strlen(s_workspaceData));
	updateFile(xcworkspacedata, workspaceFile);

	auto xcconfigPath = filepath::merge<Interface>(outdir, "project.xcconfig");
	auto xcconfig = filesystem::File::open_tmp("xcconfig");
	writeXCConfig(xproj, [&](StringView str) { xcconfig.xsputn(str.data(), str.size()); });
	updateFile(xcconfig, xcconfigPath);

	return true;
}

void MakefileConfig::load(makefile::Makefile *make) {
	std = getVariable(make, "GLOBAL_STD");
	stdxx = getVariable(make, "GLOBAL_STDXX");
	depTarget = getVariable(make, "MACOSX_DEPLOYMENT_TARGET");
	bundleName = getVariable(make, "APPCONFIG_BUNDLE_NAME");
	frameworkRoot = getVariable(make, "GLOBAL_ROOT");
	localOutdir = getVariable(make, "LOCAL_OUTDIR");
	libs = getVariable(make, "MACOS_LIBS");
	cflags = getVariable(make, "MACOS_GENERAL_CFLAGS");
	cxxflags = getVariable(make, "MACOS_GENERAL_CXXFLAGS");
	cflagsExec = getVariable(make, "MACOS_EXEC_CFLAGS");
	cxxflagsExec = getVariable(make, "MACOS_EXEC_CXXFLAGS");
	cflagsLib = getVariable(make, "MACOS_LIB_CFLAGS");
	cxxflagsLib = getVariable(make, "MACOS_LIB_CXXFLAGS");

	headerPaths = getVariable(make, "MACOS_HEADER_SEARCH_PATHS");
	ldflags = getVariable(make, "MACOS_GENERAL_LDFLAGS");
	ldflagsExec = getVariable(make, "MACOS_EXEC_LDFLAGS");
	ldflagsLib = getVariable(make, "MACOS_LIB_LDFLAGS");
}

} // namespace stappler::buildtool
