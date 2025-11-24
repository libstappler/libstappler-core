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

#include "SPDocument.h"
#include "SPDocPageContainer.h" // IWYU pragma: keep
#include "SPZip.h"

namespace STAPPLER_VERSIONIZED stappler::document {

class SP_PUBLIC DocumentEpub : public Document {
public:
	static bool isEpub(FileInfo);
	static bool isEpub(BytesView);

	virtual ~DocumentEpub() = default;

	virtual bool init(FileInfo, StringView ct = StringView());
	virtual bool init(BytesView, StringView ct = StringView());
	virtual bool init(memory::pool_t *, FileInfo, StringView ct = StringView());
	virtual bool init(memory::pool_t *, BytesView, StringView ct = StringView());

protected:
};


enum class EpubManifestType {
	Unknown,
	Image,
	Source,
	Css,
};

struct EpubManifestFile {
	// ZIP data
	uint64_t index;
	StringView path;
	size_t size;
	EpubManifestType type = EpubManifestType::Unknown;

	uint16_t width = 0;
	uint16_t height = 0;

	// Manifest data
	StringView id;
	StringView mime;
};

struct EpubData : DocumentData {
	ZipArchive<memory::PoolInterface> archive;
	Map<StringView, EpubManifestFile> manifestFiles;

	virtual ~EpubData();
	EpubData(memory::pool_t *, FileInfo, StringView = StringView());
	EpubData(memory::pool_t *, BytesView, StringView = StringView());

	bool init();
};

} // namespace stappler::document
