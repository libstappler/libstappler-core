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

#ifndef CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPBXFILE_H_
#define CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPBXFILE_H_

#include "SPPBXObject.h"

namespace STAPPLER_VERSIONIZED stappler::buildtool::xcode {

struct PBXFileSystemSynchronizedBuildFileExceptionSet : PBXObject {
	Map<String, String> additionalCompilerFlagsByRelativePath;
	Map<String, String> attributesByRelativePath;
	Vector<String> membershipExceptions;
	Vector<String> privateHeaders;
	Vector<String> publicHeaders;
	PBXTarget *target;
};

struct PBXBuildFile : PBXObject {
	PBXFileElement *file;
	String platformFilter;
	Vector<String> platformFilters;
	XCSwiftPackageProductDependency *product;
	Vector<String> settings;
};

struct PBXContainerItemProxy : PBXObject {
	struct ContainerPortal {
		enum {
			FileReference,
			Project,
			UnknownObject
		} type;

		union {
			PBXFileReference *file;
			PBXProject *project;
			PBXObject *object;
		};
	};

	enum class ProxyType {
		NativeTarget,
		Other,
		Reference
	};

	struct RemoteGlobalID {
		enum {
			StringType,
			ObjectType
		} type;

		PBXObject *object;
		String string;
	};

	ContainerPortal containerPortal;
	ProxyType proxyType;
	RemoteGlobalID remoteGlobalID;
};

struct PBXFileElement : PBXContainerItem {
	bool includeInIndex;
	uint32_t indentWidth;
	String name;
	PBXFileElement *parent;
	String path;
	PBXSourceTree sourceTree;
	uint32_t tabWidth;
	bool usesTabs;
	bool wrapsLines;
};

struct PBXFileReference final : PBXFileElement {
	static PBXFileReference *create();

	String explicitFileType;
	uint32_t fileEncoding;
	String languageSpecificationIdentifier;
	String lastKnownFileType;
	uint32_t lineEnding;
	String plistStructureDefinitionIdentifier;
	String xcLanguageSpecificationIdentifier;
};

struct PBXFileSystemSynchronizedRootGroup final : PBXFileElement {
	static PBXFileSystemSynchronizedRootGroup *create();

	Vector<PBXFileSystemSynchronizedBuildFileExceptionSet *> exceptions;
	Map<String, String> explicitFileTypes;
	Vector<String> explicitFolders;
};

struct PBXReferenceProxy final : PBXFileElement {
	String fileType;
	PBXContainerItemProxy *remote;
};

struct PBXGroup : PBXFileElement {
	Vector<PBXFileElement *> children;
};

struct PBXVariantGroup final : PBXGroup {
	static PBXVariantGroup *create();
};

struct XCVersionGroup final : PBXGroup {
	static XCVersionGroup *create();

	PBXFileReference *currentVersion;
	String versionGroupType;
};

} // namespace stappler::buildtool::xcode

#endif /* CORE_UTILS_BUILDTOOL_SRC_XCODEPROJECT_SPPBXFILE_H_ */
