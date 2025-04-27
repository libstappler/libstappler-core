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

#ifndef CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPBXBUILDPHASE_H_
#define CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPBXBUILDPHASE_H_

#include "SPPBXObject.h"

namespace STAPPLER_VERSIONIZED stappler::buildtool::xcode {

struct PBXBuildFile final : PBXObject {
	PBXFileElement *file;
	String platformFilter;
	Vector<String> platformFilters;
	XCSwiftPackageProductDependency product;
	Map<String, Value> settings;
};

struct PBXBuildPhase : PBXContainerItem {
	uint32_t buildActionMask;
	BuildPhase buildPhase;
	Vector<PBXBuildFile *> files;
	Vector<String> inputFileListPaths;
	Vector<String> outputFileListPaths;
	bool runOnlyForDeploymentPostprocessing;
};

struct PBXCopyFilesBuildPhase final : PBXBuildPhase {
	static PBXCopyFilesBuildPhase *create();

	enum class SubFolder {
		AbsolutePath,
		Executables,
		Frameworks,
		JavaResources,
		Other,
		Plugins,
		ProductsDirectory,
		Resources,
		SharedFrameworks,
		SharedSupport,
		Wrapper
	};

	String dstPath;
	SubFolder dstSubfolderSpec;
	String name;
};

struct PBXFrameworksBuildPhase final : PBXBuildPhase {
	static PBXFrameworksBuildPhase *create();
};

struct PBXHeadersBuildPhase final : PBXBuildPhase {
	static PBXHeadersBuildPhase *create();
};

struct PBXResourcesBuildPhase final : PBXBuildPhase {
	static PBXResourcesBuildPhase *create();
};

struct PBXRezBuildPhase final : PBXBuildPhase {
	static PBXRezBuildPhase *create();
};

struct PBXShellScriptBuildPhase final : PBXBuildPhase {
	static PBXShellScriptBuildPhase *create();

	bool alwaysOutOfDate;
	String dependencyFile;
	Vector<String> inputPaths;
	String name;
	Vector<String> outputPaths;
	String shellPath;
	String shellScript;
	bool showEnvVarsInLog;
};

struct PBXSourcesBuildPhase final : PBXBuildPhase {
	static PBXSourcesBuildPhase *create();
};

} // namespace stappler::buildtool::xcode

#endif /* CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPBXBUILDPHASE_H_ */
