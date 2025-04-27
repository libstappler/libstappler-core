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

#ifndef CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPDXTARGET_H_
#define CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPDXTARGET_H_

#include "SPPBXObject.h"
#include "SPPBXBuildPhase.h"

namespace STAPPLER_VERSIONIZED stappler::buildtool::xcode {

struct XCBuildConfiguration final : PBXObject {
	PBXFileReference *baseConfiguration;
	Map<String, Value> buildSettings;
	String name;
};

struct XCConfigurationList final : PBXObject {
	Vector<XCBuildConfiguration> buildConfigurations;
	bool defaultConfigurationIsVisible;
	String defaultConfigurationName;
};

struct PBXBuildRule final : PBXObject {
	String compilerSpec;
	String dependencyFile;
	String filePatterns;
	String fileType;
	Vector<String> inputFiles;
	bool isEditable;
	String name;
	Vector<String> outputFiles;
	Vector<String> outputFilesCompilerFlags;
	bool runOncePerArchitecture;
	String script;
};

struct PBXTargetDependency final : PBXObject {
	String name;
	String platformFilter;
	Vector<String> platformFilters;
	XCSwiftPackageProductDependency *product;
	PBXTarget *target;
	PBXContainerItemProxy *targetProxy;
};

struct PBXTarget : PBXContainerItem {
	XCConfigurationList *buildConfigurationList;
	Vector<PBXBuildPhase *> buildPhases;
	Vector<PBXBuildRule *> buildRules;
	Vector<PBXTargetDependency *> dependencies;
	Vector<PBXFileSystemSynchronizedRootGroup *> fileSystemSynchronizedGroups;
	String name;
	Vector<XCSwiftPackageProductDependency *> packageProductDependencies;
	PBXFileReference *product;
	String productName;
	PBXProductType productType;
};

struct PBXAggregateTarget final : PBXTarget {
	static PBXAggregateTarget *create();
};

struct PBXLegacyTarget final : PBXTarget {
	static PBXLegacyTarget *create();

	String buildArgumentsString;
	String buildToolPath;
	String buildWorkingDirectory;
	bool passBuildSettingsInEnvironment;
};

struct PBXNativeTarget final : PBXTarget {
	static PBXNativeTarget *create();

	String productInstallPath;
};

} // namespace stappler::buildtool::xcode

#endif /* CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPDXTARGET_H_ */
