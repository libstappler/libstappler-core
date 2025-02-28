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

#include "SPPlatformUnistd.h"
#include "platform/fd/SPEvent-fd.h"

#if LINUX || ANDROID || MACOS

#include <fcntl.h>

namespace STAPPLER_VERSIONIZED stappler::event {

#define SP_TERMINATED_DATA(view) (view.terminated()?view.data():view.str<memory::StandartInterface>().data())

mode_t getUnixProt(FileProtFlags perms) {
	mode_t mode = 0;

	if (hasFlag(perms, FileProtFlags::UserSetId)) { mode |= S_ISUID; }
	if (hasFlag(perms, FileProtFlags::UserRead)) { mode |= S_IRUSR; }
	if (hasFlag(perms, FileProtFlags::UserWrite)) { mode |= S_IWUSR; }
	if (hasFlag(perms, FileProtFlags::UserExecute)) { mode |= S_IXUSR; }

	if (hasFlag(perms, FileProtFlags::GroupSetId)) { mode |= S_ISGID; }
	if (hasFlag(perms, FileProtFlags::GroupRead)) { mode |= S_IRGRP; }
	if (hasFlag(perms, FileProtFlags::GroupWrite)) { mode |= S_IWGRP; }
	if (hasFlag(perms, FileProtFlags::GroupExecute)) { mode |= S_IXGRP; }

	if (hasFlag(perms, FileProtFlags::AllRead)) { mode |= S_IROTH; }
	if (hasFlag(perms, FileProtFlags::AllWrite)) { mode |= S_IWOTH; }
	if (hasFlag(perms, FileProtFlags::AllExecute)) { mode |= S_IXOTH; }

	return mode;
}

bool File::init(StringView path, FileOpenFlags flags, FileProtFlags prot) {
	int fd;
	int oflags = O_CLOEXEC;
#if defined(O_LARGEFILE)
	oflags |= O_LARGEFILE;
#endif

	if (hasFlag(flags, FileOpenFlags::Read) && hasFlag(flags, FileOpenFlags::Write)) {
		oflags = O_RDWR;
	} else if (hasFlag(flags, FileOpenFlags::Read)) {
		oflags = O_RDONLY;
	} else if (hasFlag(flags, FileOpenFlags::Write)) {
		oflags = O_WRONLY;
	} else {
		log::warn("event::File", "Invalid FileOpenFlags for ", path);
		return false;
	}

	if (hasFlag(flags, FileOpenFlags::Create)) {
		oflags |= O_CREAT;
		if (hasFlag(flags, FileOpenFlags::CreateExclusive)) {
			oflags |= O_EXCL;
		}
	}
	if (hasFlag(flags, FileOpenFlags::CreateExclusive) && !(hasFlag(flags, FileOpenFlags::Create))) {
		log::warn("event::File", "Invalid FileOpenFlags for ", path);
		return false;
	}

	if (hasFlag(flags, FileOpenFlags::Append)) {
		oflags |= O_APPEND;
	}
	if (hasFlag(flags, FileOpenFlags::Truncate)) {
		oflags |= O_TRUNC;
	}

	if (prot == FileProtFlags::Default) {
		fd = ::open(SP_TERMINATED_DATA(path), oflags, 0666);
	} else {
		fd = ::open(SP_TERMINATED_DATA(path), oflags, getUnixProt(prot));
	}
	if (fd < 0) {
		log::warn("event::File", "Fail to open file ", path, " with error: ", errno);
		return false;
	}

	((SourceData *)_data)->fd = fd;
	_openFlags = flags;
	return true;
}

bool File::isSupported(HandleOp op) const {
	switch (op) {
	case HandleOp::Read:
	case HandleOp::Write:
		return true;
		break;
	default:
		return false;
		break;
	}
	return false;
}

Status File::read(uint8_t *data, size_t &size) {
	auto fd = ((SourceData *)_data)->fd;

	auto ret = ::read(fd, data, size);
	if (ret >= 0) {
		size = ret;
		if (size == 0) {
			_eof = true;
		}
		return (size > 0) ? Status::Ok : Status::Done;
	} else {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			size = 0;
			return Status::Suspended;
		}
	}
	return Status::Declined;
}

Status File::write(const uint8_t *data, size_t &size) {
	auto fd = ((SourceData *)_data)->fd;

	auto targetSize = size;
	auto ret = ::write(fd, data, targetSize);
	if (ret >= 0) {
		size = ret;
		return (size == targetSize) ? Status::Ok : Status::Done;
	} else {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			size = 0;
			return Status::Suspended;
		}
	}
	return Status::Declined;
}

size_t File::seek(int64_t offset, io::Seek s) {
	auto fd = ((SourceData *)_data)->fd;
	int whence = 0;
	switch (s) {
	case io::Seek::Current: whence = SEEK_CUR; break;
	case io::Seek::Set: whence = SEEK_SET; break;
	case io::Seek::End: whence = SEEK_END; break;
	}

	_eof = false;

	return size_t(::lseek(fd, offset, whence));
}

size_t File::tell() const {
	auto fd = ((SourceData *)_data)->fd;
	return size_t(lseek(fd, 0, SEEK_CUR));
}

size_t File::size() const {
	auto fd = ((SourceData *)_data)->fd;

	auto pos = tell();
	auto size = ::lseek(fd, 0, SEEK_END);
	::lseek(fd, off_t(pos), SEEK_SET);
	return size;
}

void File::close() {
	if (_openFlags == FileOpenFlags::None) {
		return;
	}

	auto fd = ((SourceData *)_data)->fd;
	if (fd >= 0) {
		::close(fd);
	}

	_openFlags = FileOpenFlags::None;
	((SourceData *)_data)->fd = -1;
}

#undef SP_TERMINATED_DATA

}

#endif
