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

struct XCodeProject {
	makefile::xcode::XCodeExport xctx;
	Rc<makefile::MakefileRef> release;
	Rc<makefile::MakefileRef> debug;
	StringView frameworkPath;
};

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
			list->buildSettings.emplace("GCC_PREPROCESSOR_DEFINITIONS",
					Value({Value("DEBUG=1"), Value("$(inherited)")}));
			list->buildSettings.emplace("MTL_ENABLE_DEBUG_INFO", "INCLUDE_SOURCE");
			list->buildSettings.emplace("ONLY_ACTIVE_ARCH", true);
			list->buildSettings.emplace("SWIFT_ACTIVE_COMPILATION_CONDITIONS",
					"DEBUG $(inherited)");
			list->buildSettings.emplace("SWIFT_OPTIMIZATION_LEVEL", "-Onone");
		} else {
			list->buildSettings.emplace("GCC_PREPROCESSOR_DEFINITIONS",
					Value({Value("NDEBUG=1"), Value("$(inherited)")}));
			list->buildSettings.emplace("DEBUG_INFORMATION_FORMAT", "dwarf-with-dsym");
			list->buildSettings.emplace("ENABLE_NS_ASSERTIONS", false);
			list->buildSettings.emplace("MTL_ENABLE_DEBUG_INFO", false);
			list->buildSettings.emplace("SWIFT_COMPILATION_MODE", "wholemodule");
		}
	});
}


const makefile::xcode::XCBuildConfiguration *makeMacOsFrameworkConfiguration(XCodeProject &xproj,
		StringView name, bool debug) {
	using namespace makefile;
	return xcode::XCBuildConfiguration::create(xproj.xctx, [&](xcode::XCBuildConfiguration *list) {
		list->name = name.str<Interface>();

		list->buildSettings.emplace("CODE_SIGN_STYLE", "Automatic");
		list->buildSettings.emplace("COMBINE_HIDPI_IMAGES", true);
		list->buildSettings.emplace("CURRENT_PROJECT_VERSION", 1);
		list->buildSettings.emplace("DEFINES_MODULE", true);
		list->buildSettings.emplace("DYLIB_COMPATIBILITY_VERSION", 1);
		list->buildSettings.emplace("DYLIB_CURRENT_VERSION", 1);
		list->buildSettings.emplace("DYLIB_INSTALL_NAME_BASE", "@rpath");
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

		auto std = getVariable(make, "GLOBAL_STD");
		auto stdxx = getVariable(make, "GLOBAL_STDXX");
		auto depTarget = getVariable(make, "MACOSX_DEPLOYMENT_TARGET");
		auto bundleName = getVariable(make, "APPCONFIG_BUNDLE_NAME");

		list->buildSettings.emplace("MODULE_VERIFIER_SUPPORTED_LANGUAGE_STANDARDS",
				toString(std, " ", stdxx));
		list->buildSettings.emplace("MACOSX_DEPLOYMENT_TARGET", depTarget);
		list->buildSettings.emplace("PRODUCT_BUNDLE_IDENTIFIER",
				toString(bundleName, ".framework"));

		auto frameworkRoot = getVariable(make, "GLOBAL_ROOT");
		auto headerPaths = getVariable(make, "MACOS_HEADER_SEARCH_PATHS");

		StringStream fixedHeaderPaths;

		if (debug) {
			fixedHeaderPaths << "$(PROJECT_DIR)/debug/include";
		} else {
			fixedHeaderPaths << "$(PROJECT_DIR)/release/include";
		}

		StringView(headerPaths).split<StringView::WhiteSpace>([&](StringView path) {
			if (path.starts_with(StringView(frameworkRoot))) {
				fixedHeaderPaths << " $(STAPPLER_ROOT)" << path.sub(frameworkRoot.size());
			} else {
				fixedHeaderPaths << " " << path;
			}
		});

		fixedHeaderPaths << " $(STAPPLER_ROOT)/deps/mac/$(CURRENT_ARCH)/include";

		list->buildSettings.emplace("HEADER_SEARCH_PATHS", fixedHeaderPaths.str());
		list->buildSettings.emplace("LIBRARY_SEARCH_PATHS",
				toString("/usr/local/lib $(STAPPLER_ROOT)/deps/mac/$(CURRENT_ARCH)/lib"));
		list->buildSettings.emplace("STAPPLER_ROOT", frameworkRoot);

		list->buildSettings.emplace("OTHER_LDFLAGS", getVariable(make, "MACOS_GENERAL_LDFLAGS"));
		list->buildSettings.emplace("OTHER_LIBTOOLFLAGS", getVariable(make, "MACOS_LIBS"));
		list->buildSettings.emplace("OTHER_CFLAGS", getVariable(make, "MACOS_GENERAL_CFLAGS"));
		list->buildSettings.emplace("OTHER_CPLUSPLUSFLAGS",
				getVariable(make, "MACOS_GENERAL_CXXFLAGS"));
	});
}

static void makeXCodeMacOsTarget(XCodeProject &xproj, makefile::xcode::PBXNativeTarget *target) {
	using namespace makefile;

	target->buildPhases.emplace_back(
			xcode::PBXHeadersBuildPhase::create(xproj.xctx, [&](xcode::PBXHeadersBuildPhase *list) {

	}));

	target->buildPhases.emplace_back(
			xcode::PBXSourcesBuildPhase::create(xproj.xctx, [&](xcode::PBXSourcesBuildPhase *list) {

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

	target->product =
			xcode::PBXFileReference::create(xproj.xctx, [](xcode::PBXFileReference *file) {
		file->explicitFileType = "wrapper.framework";
		file->path = "MacOsFramework.framework";
		file->includeInIndex = 0;
		file->sourceTree = xcode::PBXSourceTree{xcode::PBXSourceTree::buildProductsDir};
	});

	target->productType = xcode::PBXProductType::framework;
}

static void makeConfigs(StringView path, makefile::Makefile *make) {
	using namespace makefile;

	auto updateFile = [](filesystem::File &file, StringView path) {
		if (filesystem::exists(FileInfo(path))) {
			auto oldData = filesystem::readIntoMemory<Interface>(FileInfo(path));
			auto newData = file.readIntoMemory<Interface>();

			if (oldData != newData) {
				filesystem::remove(FileInfo(path));
				file.close_rename(FileInfo(path));
			}
		} else {
			file.close_rename(FileInfo(path));
		}
	};

	auto debugsrcdir = filepath::merge<Interface>(path, "src");
	auto debugincludedir = filepath::merge<Interface>(path, "include");

	filesystem::mkdir_recursive(FileInfo(debugsrcdir));
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

	auto appSource = filepath::merge<Interface>(debugsrcdir, "stappler-appconfig.cpp");
	auto appSourceFile = filesystem::File::open_tmp("stappler-appconfig-cpp");
	makeAppConfigSource(make,
			[&](StringView str) { appSourceFile.xsputn(str.data(), str.size()); });
	updateFile(appSourceFile, appSource);
}

bool makeXCodeProject(StringView buildRoot, FileInfo projMakefilePath) {
	using namespace makefile;

	XCodeProject xproj;

	xproj.release = Rc<MakefileRef>::create();
	xproj.release->assignSimpleVariable("RELEASE", makefile::Origin::CommandLine, "1");
	xproj.release->assignSimpleVariable("SPBUILDTOOL", makefile::Origin::CommandLine, "1");
	xproj.release->assignSimpleVariable("STAPPLER_TARGET", makefile::Origin::CommandLine, "host");
	xproj.release->assignSimpleVariable("MACOS", makefile::Origin::CommandLine, "1");
	xproj.release->assignSimpleVariable("STAPPLER_ARCH", makefile::Origin::CommandLine, "x86_64");
	xproj.release->assignSimpleVariable("STAPPLER_BUILD_ROOT", makefile::Origin::CommandLine,
			buildRoot);

	if (!xproj.release->include(projMakefilePath)) {
		log::error("XCodeProject", "Fail to load project Makefile");
		return false;
	}

	xproj.debug = Rc<MakefileRef>::create();
	xproj.debug->assignSimpleVariable("DEBUG", makefile::Origin::CommandLine, "1");
	xproj.debug->assignSimpleVariable("SPBUILDTOOL", makefile::Origin::CommandLine, "1");
	xproj.debug->assignSimpleVariable("STAPPLER_TARGET", makefile::Origin::CommandLine, "host");
	xproj.debug->assignSimpleVariable("MACOS", makefile::Origin::CommandLine, "1");
	xproj.debug->assignSimpleVariable("STAPPLER_ARCH", makefile::Origin::CommandLine, "x86_64");
	xproj.debug->assignSimpleVariable("STAPPLER_BUILD_ROOT", makefile::Origin::CommandLine,
			buildRoot);

	if (!xproj.debug->include(projMakefilePath)) {
		log::error("XCodeProject", "Fail to load project Makefile");
		return false;
	}

	auto path = filesystem::findPath<Interface>(filepath::root(projMakefilePath));

	String localOutdir = getVariable(xproj.release, "LOCAL_OUTDIR");
	String localExecutable = getVariable(xproj.release, "LOCAL_EXECUTABLE");
	String localLibrary = getVariable(xproj.release, "LOCAL_LIBRARY");

	if (localOutdir.empty() || (localExecutable.empty() && localLibrary.empty())) {
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

	auto projFilePath = filepath::merge<Interface>(projPath, "project.pbxproj");

	xproj.xctx.root = xcode::PBXProject::create(xproj.xctx, [&](xcode::PBXProject *proj) {
		proj->developmentRegion = "en";
		proj->hasScannedForEncodings = 0;
		proj->knownRegions = {"en", "Base"};
		proj->preferredProjectObjectVersion = 77;

		proj->targets.emplace_back(xcode::PBXNativeTarget::create(xproj.xctx,
				[&](xcode::PBXNativeTarget *target) { makeXCodeMacOsTarget(xproj, target); }));

		proj->productsGroup = xcode::PBXGroup::create(xproj.xctx, [&](xcode::PBXGroup *group) {
			for (auto &it : proj->targets) { group->children.emplace_back(it->product); }

			group->name = "Products";
		});

		proj->buildConfigurationList = xcode::XCConfigurationList::create(xproj.xctx,
				[&](xcode::XCConfigurationList *list) {
			auto debug = makeProjectConfiguration(xproj.debug, xproj.xctx, "Debug", true);
			auto release = makeProjectConfiguration(xproj.release, xproj.xctx, "Release", false);

			list->buildConfigurations.emplace_back(debug);
			list->buildConfigurations.emplace_back(release);

			list->defaultConfiguration = release;
		});

		proj->mainGroup = xcode::PBXGroup::create(xproj.xctx,
				[&](xcode::PBXGroup *group) { group->children.emplace_back(proj->productsGroup); });
	});

	auto file = filesystem::File::open_tmp("xcodeproj");

	xproj.xctx.write([&](StringView str) { file.xsputn(str.data(), str.size()); });

	filesystem::remove(FileInfo(projFilePath));
	file.close_rename(FileInfo(projFilePath));

	return true;
}

/*

$(MACOS_PROJECT_DIR)/%.xcconfig:
	@$(GLOBAL_MKDIR) $(dir $@)
	@echo '// Autogenerated by makefile' > $@
	@echo 'STAPPLER_MODULE_NAME = $*' >> $@
	@echo 'STAPPLER_MODULES_ENABLED = $(foreach module,$(GLOBAL_MODULES),$(MODULE_$(module)))' >> $@
	@echo 'STAPPLER_MODULES_CONSUMED = $($(MODULE_$*)_SHARED_CONSUME)' >> $@
	@echo 'STAPPLER_MACOS_GENERAL_CFLAGS = $(MACOS_GENERAL_CFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_GENERAL_CXXFLAGS = $(MACOS_GENERAL_CXXFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_GENERAL_LDFLAGS = $($(MODULE_$*)_GENERAL_LDFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_CONDUMED_LDFLAGS = $(foreach module,$($(MODULE_$*)_SHARED_CONSUME),$($(MODULE_$(module))_GENERAL_LDFLAGS))' >> $@
	@echo 'STAPPLER_MACOS_EXEC_CFLAGS = $(MACOS_EXEC_CFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_EXEC_CXXFLAGS = $(MACOS_EXEC_CXXFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_LIB_CFLAGS = $(MACOS_LIB_CFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_LIB_CXXFLAGS = $(MACOS_LIB_CXXFLAGS)' >> $@
	@echo 'OTHER_LDFLAGS = $(GLOBAL_GENERAL_LDFLAGS) $(LOCAL_LDFLAGS) $(call sp_toolkit_transform_lib_ldflag, $($(MODULE_$*)_LIBS))' >> $@
	@echo 'OTHER_LIBTOOLFLAGS = $(call sp_toolkit_transform_lib_ldflag, $($(MODULE_$*)_LIBS) $(foreach module,$($(MODULE_$*)_SHARED_CONSUME),$($(MODULE_$(module))_LIBS)))' >> $@
	@echo 'OTHER_CFLAGS = $$(STAPPLER_MACOS_GENERAL_CFLAGS)' >> $@
	@echo 'OTHER_CPLUSPLUSFLAGS = $$(STAPPLER_MACOS_GENERAL_CXXFLAGS)' >> $@
	@echo 'HEADER_SEARCH_PATHS = $(MACOS_HEADER_SEARCH_PATHS)' >> $@
	@echo 'LIBRARY_SEARCH_PATHS = $(MACOS_LIBRARY_SEARCH_PATHS)' >> $@
	@echo 'SDKROOT = macOS' >> $@
	@echo 'SUPPORTED_PLATFORMS = macosx' >> $@
	@echo 'GCC_PREPROCESSOR_DEFINITIONS[config=Debug] = DEBUG=1' >> $@
	@echo 'GCC_PREPROCESSOR_DEFINITIONS[config=Release] = NDEBUG=1' >> $@
	@echo 'MACOSX_DEPLOYMENT_TARGET = $(MACOSX_DEPLOYMENT_TARGET)' >> $@
	@echo 'CLANG_CXX_LANGUAGE_STANDARD = $(GLOBAL_STDXX)' >> $@
	@echo 'GCC_C_LANGUAGE_STANDARD = $(GLOBAL_STD)' >> $@
	@echo 'MACH_O_TYPE = staticlib' >> $@
	@echo 'MAKE_MERGEABLE = YES' >> $@
    @echo 'MERGEABLE_LIBRARY = YES' >> $@
    @echo 'ONLY_ACTIVE_ARCH = NO' >> $@

$(MACOS_PROJECT_DIR)/macos.projectconfig.xcconfig.tmp:
	@$(GLOBAL_MKDIR) $(dir $@)
	@echo '// Autogenerated by makefile' > $@
	@echo 'STAPPLER_SRCS = $(foreach include,$(TOOLKIT_SRCS),$(call sp_relpath_config, $(include)))' >> $@
	@echo 'STAPPLER_MODULES_ENABLED = $(foreach module,$(GLOBAL_MODULES),$(MODULE_$(module)))' >> $@
	@echo 'STAPPLER_MODULES_DEFS = $(foreach module,$(GLOBAL_MODULES),-D$(MODULE_$(module)))' >> $@
	@echo 'STAPPLER_MACOS_GENERAL_CFLAGS = $(MACOS_GENERAL_CFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_GENERAL_CXXFLAGS = $(MACOS_GENERAL_CXXFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_GENERAL_LDFLAGS = $(MACOS_GENERAL_LDFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_EXEC_CFLAGS = $(MACOS_EXEC_CFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_EXEC_CXXFLAGS = $(MACOS_EXEC_CXXFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_EXEC_LDFLAGS = $(MACOS_EXEC_LDFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_LIB_CFLAGS = $(MACOS_LIB_CFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_LIB_CXXFLAGS = $(MACOS_LIB_CXXFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_LIB_LDFLAGS = $(MACOS_LIB_LDFLAGS)' >> $@
	@echo 'STAPPLER_MACOS_LIBS = $(MACOS_LIBS)' >> $@
	@echo 'STAPPLER_MACOS_SHADERS = $(sort $(foreach include,$(BUILD_SHADERS_EMBEDDED) $(TOOLKIT_SHADERS_EMBEDDED),$(call sp_relpath_config, $(include))))' >> $@

	@echo 'GCC_PREPROCESSOR_DEFINITIONS[config=Debug] = DEBUG=1' >> $@
	@echo 'GCC_PREPROCESSOR_DEFINITIONS[config=Release] = NDEBUG=1' >> $@


*/


} // namespace stappler::buildtool
