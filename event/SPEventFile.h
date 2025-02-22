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

#ifndef CORE_EVENT_SPEVENTFILE_H_
#define CORE_EVENT_SPEVENTFILE_H_

#include "SPIO.h"
#include "SPEventSource.h"

namespace STAPPLER_VERSIONIZED stappler::event {

enum class FileOpenFlags {
	None,
	Read = 1 << 0,
	Write = 1 << 1,
	Create = 1 << 2,
	Append = 1 << 3,
	Truncate = 1 << 4,
	CreateExclusive = 1 << 5,
	DelOnClose = 1 << 6,
};

SP_DEFINE_ENUM_AS_MASK(FileOpenFlags)

enum class FileProtFlags {
	UserSetId = 0x8000,
	UserRead = 0x0400,
	UserWrite = 0x0200,
	UserExecute = 0x0100,
	GroupSetId = 0x4000,
	GroupRead = 0x0040,
	GroupWrite = 0x0020,
	GroupExecute = 0x0010,
	AllRead = 0x0004,
	AllWrite = 0x0002,
	AllExecute = 0x0001,
	Default = 0x0FFF,
};

SP_DEFINE_ENUM_AS_MASK(FileProtFlags)

class File : public Source {
public:
	virtual ~File();

	virtual bool init(StringView path, FileOpenFlags, FileProtFlags);

	virtual Status read(uint8_t *, size_t &) override;

	virtual Status write(const uint8_t *, size_t &) override;

	virtual size_t seek(int64_t offset, io::Seek s);
	virtual size_t tell() const;
	virtual size_t size() const;

	virtual void close() override;

protected:
	FileOpenFlags _openFlags = FileOpenFlags::None;
	bool _eof = false;
};

}

#endif /* CORE_EVENT_SPEVENTFILE_H_ */
