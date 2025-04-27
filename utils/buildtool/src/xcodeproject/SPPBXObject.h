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

#ifndef CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPBXOBJECT_H_
#define CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPBXOBJECT_H_

#include "SPCommon.h"
#include "SPMemory.h"

namespace STAPPLER_VERSIONIZED stappler::buildtool::xcode {

using namespace mem_pool;

enum class ISA {
	PBXObject,
	PBXBuildFile,
	PBXContainerItem,
	PBXFileElement,
	PBXAggregateTarget,
	PBXContainerItemProxy,
	PBXFileReference,
	PBXFileSystemSynchronizedBuildFileExceptionSet,
	PBXFileSystemSynchronizedRootGroup,
	PBXFrameworksBuildPhase,
	PBXGroup,
	PBXHeadersBuildPhase,
	PBXNativeTarget,
	PBXProject,
	PBXResourcesBuildPhase,
	PBXSourcesBuildPhase,
	PBXTargetDependency,
	XCBuildConfiguration,
	XCConfigurationList,
	XCSwiftPackageProductDependency
};

using Id = std::array<uint8_t, 12>;
using StringId = std::array<char, 12>;

enum class BuildPhase {
	carbonResources,
	copyFiles,
	frameworks,
	headers,
	resources,
	runScript,
	sources,
};

enum class PBXSourceTree {
	absolute,
	buildProductsDir,
	custom,
	developerDir,
	group,
	none,
	sdkRoot,
	sourceRoot,
};

enum class PBXProductType {
	appExtension,
	application,
	bundle,
	commandLineTool,
	driverExtension,
	dynamicLibrary,
	extensionKitExtension,
	framework,
	instrumentsPackage,
	intentsServiceExtension,
	messagesApplication,
	messagesExtension,
	metalLibrary,
	none,
	ocUnitTestBundle,
	onDemandInstallCapableApplication,
	staticFramework,
	staticLibrary,
	stickerPack,
	systemExtension,
	tvExtension,
	uiTestBundle,
	unitTestBundle,
	watch2App,
	watch2AppContainer,
	watch2Extension,
	watchApp,
	watchExtension,
	xcFramework,
	xcodeExtension,
	xpcService
};

struct PBXAggregateTarget;
struct PBXContainerItemProxy;
struct PBXFileElement;
struct PBXFileReference;
struct PBXFileSystemSynchronizedBuildFileExceptionSet;
struct PBXFileSystemSynchronizedRootGroup;
struct PBXFrameworksBuildPhase;
struct PBXGroup;
struct PBXHeadersBuildPhase;
struct PBXTarget;
struct PBXNativeTarget;
struct PBXProject;
struct PBXResourcesBuildPhase;
struct PBXSourcesBuildPhase;
struct PBXTargetDependency;
struct XCBuildConfiguration;
struct XCConfigurationList;
struct XCSwiftPackageProductDependency;

struct PBXObject : AllocBase {
	static Id generateId();
	static StringId getStringId(Id);

	ISA isa;
	Id id;
};

struct PBXContainerItem : PBXObject { };

struct XCLocalSwiftPackageReference : PBXContainerItem {
	String name;
	String relativePath;
};

struct XCRemoteSwiftPackageReference : PBXContainerItem {
	struct VersionRequirement {
		enum {
			Branch,
			Exact,
			Range,
			Revision,
			UpToNextMajorVersion,
			UpToNextMinorVersion,
		} type;
		String value1;
		String value2;
	};

	String name;
	String repositoryURL;
	VersionRequirement versionRequirement;
};

struct XCSwiftPackageProductDependency : PBXContainerItem {
	XCRemoteSwiftPackageReference *package;
	String productName;
};

} // namespace stappler::buildtool::xcode

#endif /* CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPBXOBJECT_H_ */
