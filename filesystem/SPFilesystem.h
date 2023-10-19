/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

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

#ifndef STAPPLER_FILESYSTEM_SPFILESYSTEM_H_
#define STAPPLER_FILESYSTEM_SPFILESYSTEM_H_

#include "SPIO.h"
#include "SPTime.h"
#include "SPFilepath.h"

namespace stappler::filesystem {

enum Access {
	Exists,
	Read,
	Write,
	Execute
};

struct Stat {
	size_t size = 0;
	Time atime;
	Time ctime;
	Time mtime;
	bool isDir = false;
};

class File {
public:
	enum class Flags {
		None,
		DelOnClose
	};

	using traits_type = std::char_traits<char>;
	using streamsize = std::streamsize;
	using int_type = typename traits_type::int_type;

	static File open_tmp(const char *prefix, bool delOnClose = true);

	File();
	explicit File(FILE *, Flags = Flags::None);
	explicit File(void *);
	explicit File(void *, size_t);

	~File();

	File(File &&);
	File & operator=(File &&);

	File(const File &) = delete;
	File & operator=(const File &) = delete;

	size_t read(uint8_t *buf, size_t nbytes);
	size_t seek(int64_t offset, io::Seek s);

	size_t tell() const;
	size_t size() const;

	int_type xsgetc();
	int_type xsputc(int_type c);

	streamsize xsputn(const char* s, streamsize n);
	streamsize xsgetn(char* s, streamsize n);

	bool eof() const;
	void close();
	void close_remove();

	bool close_rename(StringView);

	bool is_open() const;
	operator bool() const { return is_open(); }

	const char *path() const;

protected:
	void set_tmp_path(const char *);

	bool _isBundled = false;
	size_t _size = 0;
	Flags _flags = Flags::None;
	char _buf[256] = { 0 };
	union {
		FILE *_nativeFile;
		void *_platformFile;
	};
};

// Check if file at path exists
bool exists(StringView path);

bool stat(StringView path, Stat &);

// create dir at path (just mkdir, not mkdir -p)
bool mkdir(StringView path);

// mkdir -p (
bool mkdir_recursive(StringView path, bool appWide = true);

// touch (set mtime to now) file
bool touch(StringView path);

// move file from source to dest (tries to rename file, then copy-remove, rename will be successful only if both path is on single physical drive)
bool move(StringView source, StringView dest);

// copy file or directory to dest; use ftw_b for dirs, no directory tree check
bool copy(StringView source, StringView dest, bool stopOnError = true);

// remove file or directory
// if not recursive, only single file or empty dir will be removed
// if withDirs == false, only file s in directory tree will be removed
bool remove(StringView path, bool recursive = false, bool withDirs = false);

// file-tree-walk, walk across directory tree at path, callback will be called for each file or directory
// path in callback is absolute
// depth = -1 - unlimited
// dirFirst == true - directory will be shown before files inside them, useful for listings and copy
// dirFirst == false - directory will be shown after files, useful for remove
void ftw(StringView path, const Callback<void(StringView path, bool isFile)> &, int depth = -1, bool dirFirst = false);

// same as ftw, but iteration can be stopped by returning false from callback
bool ftw_b(StringView path, const Callback<bool(StringView path, bool isFile)> &, int depth = -1, bool dirFirst = false);

// returns application writable path (or path inside writable dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Writable path should be used for sharable, but not valuable contents,
// or caches, that should not be removed, when application is running or in background
// On android, writable path is on same drive or device, that used for application file
// This library use writable path to store fonts, icons caches and assets
template <typename Interface>
auto writablePath(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

template <typename Interface>
auto writablePathReadOnly(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

// returns application documents path (or path inside documents dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Documents path should be used for valuable data, like documents, created by user,
// or content, that will be hard to recreate
// This library stores StoreKit and purchases data in documents dir
template <typename Interface>
auto documentsPath(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

template <typename Interface>
auto documentsPathReadOnly(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

// returns application current work dir from getcwd (or path inside current dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Current work dir is technical concept. Use it only if there is good reason for it
template <typename Interface>
auto currentDir(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

// returns application caches dir (or path inside caches dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Caches dir used to store caches or content, that can be easily recreated,
// and that can be removed/erased, when application is active or in background
// On android, caches will be placed on SD card, if it's available
template <typename Interface>
auto cachesPath(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

template <typename Interface>
auto cachesPathReadOnly(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

// write data into file on path
bool write(StringView path, const uint8_t *data, size_t len);

template <typename BytesView>
inline bool write(StringView path, const BytesView &view) {
	return write(path, reinterpret_cast<const uint8_t *>(view.data()), size_t(view.size()));
}

File openForReading(StringView path);

// read file to string (if it was a binary file, string will be invalid)
template <typename Interface>
auto readTextFile(StringView path) -> typename Interface::StringType;

bool readIntoBuffer(uint8_t *buf, StringView path, size_t off = 0, size_t size = maxOf<size_t>());
bool readWithConsumer(const io::Consumer &stream, uint8_t *buf, size_t bsize, StringView path, size_t off, size_t size);

template <size_t Buffer = 1_KiB>
bool readWithConsumer(const io::Consumer &stream, StringView path,
		size_t off = 0, size_t size = maxOf<size_t>()) {
	uint8_t b[Buffer];
	return readWithConsumer(stream, b, Buffer, path, off, size);
}

template <typename Interface>
auto readIntoMemory(StringView ipath, size_t off = 0, size_t size = maxOf<size_t>()) -> typename Interface::BytesType {
	auto f = openForReading(ipath);
	if (f) {
		auto fsize = f.size();
		if (fsize <= off) {
			f.close();
			return typename Interface::BytesType();
		}
		if (fsize - off < size) {
			size = fsize - off;
		}
		typename Interface::BytesType ret; ret.resize(size);
		f.seek(off, io::Seek::Set);
		f.read(ret.data(), size);
		f.close();
		return ret;
	}
	return typename Interface::BytesType();
}

}


namespace stappler::filesystem::platform {

#if ANDROID
void Android_initializeFilesystem(void *assetManager, StringView filesDir, StringView cachesDir, StringView apkPath);
void Android_terminateFilesystem();
StringView Android_getApkPath();
#endif

template <typename Interface>
auto _getApplicationPath() -> typename Interface::StringType;

template <typename Interface>
auto _getWritablePath(bool readOnly) -> typename Interface::StringType;

template <typename Interface>
auto _getDocumentsPath(bool readOnly) -> typename Interface::StringType;

template <typename Interface>
auto _getCachesPath(bool readOnly) -> typename Interface::StringType;

bool _exists(StringView path, bool assetsRoot = true);
bool _stat(StringView path, Stat &, bool assetsRoot = true);

File _openForReading(StringView);
size_t _read(void *, uint8_t *buf, size_t nbytes);
size_t _seek(void *, int64_t offset, io::Seek s);
size_t _tell(void *);
bool _eof(void *);
void _close(void *);

void _ftw(StringView path, const Callback<void(StringView path, bool isFile)> &, int depth, bool dirFirst, bool assetsRoot = true);

bool _ftw_b(StringView path, const Callback<bool(StringView path, bool isFile)> &, int depth, bool dirFirst, bool assetsRoot = true);

}


// functions to access native filesystem directly
// libstappler uses posix path scheme, it should be transformed when transferred from/to other apps/libraries
// no transformation required within libstappler itself
namespace stappler::filesystem::native {

// C:\dirname\filename -> /c/dirname/filename
template <typename Interface>
auto nativeToPosix(StringView path) -> typename Interface::StringType;

// /c/dirname/filename -> C:\dirname\filename
// be sure that path is absolute
template <typename Interface>
auto posixToNative(StringView path) -> typename Interface::StringType;

template <typename Interface>
auto getcwd_fn() -> typename Interface::StringType;

bool remove_fn(StringView path);
bool unlink_fn(StringView path);
bool mkdir_fn(StringView path);

bool access_fn(StringView path, Access mode);

bool stat_fn(StringView path, Stat &);

bool touch_fn(StringView path);

void ftw_fn(StringView path, const Callback<void(StringView path, bool isFile)> &, int depth, bool dirFirst);
bool ftw_b_fn(StringView path, const Callback<bool(StringView path, bool isFile)> &, int depth, bool dirFirst);

bool rename_fn(StringView source, StringView dest);

FILE *fopen_fn(StringView, StringView mode);

bool write_fn(StringView, const unsigned char *data, size_t len);

}

namespace stappler::filesystem {

template <typename Interface>
auto writablePath(StringView path, bool relative) -> typename Interface::StringType {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str<Interface>();
	}
	auto wpath =  filesystem::platform::_getWritablePath<Interface>(true);
	if (path.empty()) {
		return wpath;
	}
	return filepath::merge<Interface>(wpath, path);
}

template <typename Interface>
auto writablePathReadOnly(StringView path, bool relative) -> typename Interface::StringType {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str<Interface>();
	}
	auto wpath =  filesystem::platform::_getWritablePath<Interface>(false);
	if (path.empty()) {
		return wpath;
	}
	return filepath::merge<Interface>(wpath, path);
}

template <typename Interface>
auto cachesPath(StringView path, bool relative) -> typename Interface::StringType {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str<Interface>();
	}
	auto cpath =  filesystem::platform::_getCachesPath<Interface>(true);
	if (path.empty()) {
		return cpath;
	}
	return filepath::merge<Interface>(cpath, path);
}

template <typename Interface>
auto cachesPathReadOnly(StringView path, bool relative) -> typename Interface::StringType {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str<Interface>();
	}
	auto cpath =  filesystem::platform::_getCachesPath<Interface>(false);
	if (path.empty()) {
		return cpath;
	}
	return filepath::merge<Interface>(cpath, path);
}

template <typename Interface>
auto documentsPath(StringView path, bool relative) -> typename Interface::StringType {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str<Interface>();
	}
	auto dpath =  filesystem::platform::_getDocumentsPath<Interface>(true);
	if (path.empty()) {
		return dpath;
	}
	return filepath::merge<Interface>(dpath, path);
}

template <typename Interface>
auto documentsPathReadOnly(StringView path, bool relative) -> typename Interface::StringType {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str<Interface>();
	}
	auto dpath =  filesystem::platform::_getDocumentsPath<Interface>(false);
	if (path.empty()) {
		return dpath;
	}
	return filepath::merge<Interface>(dpath, path);
}

template <typename Interface>
auto currentDir(StringView path, bool relative) -> typename Interface::StringType {
	if (!path.empty() && !relative && filepath::isAbsolute(path)) {
		return path.str<Interface>();
	}
	auto cwd = filesystem::native::getcwd_fn<Interface>();
	if (!cwd.empty()) {
		if (path.empty()) {
			return cwd;
		} else {
			return filepath::merge<Interface>(cwd, path);
		}
	}
	return typename Interface::StringType();
}

template <typename Interface>
auto readTextFile(StringView ipath) -> typename Interface::StringType {
	auto f = openForReading(ipath);
	if (f) {
		auto fsize = f.size();
		typename Interface::StringType ret; ret.resize(fsize);
		f.read((uint8_t *)ret.data(), fsize);
		f.close();
		return ret;
	}
	return typename Interface::StringType();
}

}

namespace stappler::io {

template <>
struct ProducerTraits<filesystem::File> {
	using type = filesystem::File;
	static size_t ReadFn(void *ptr, uint8_t *buf, size_t nbytes) {
		return ((type *)ptr)->read(buf, nbytes);
	}

	static size_t SeekFn(void *ptr, int64_t offset, Seek s) {
		return ((type *)ptr)->seek(offset, s);
	}
	static size_t TellFn(void *ptr) {
		return ((type *)ptr)->tell();
	}
};

}

#endif /* STAPPLER_FILESYSTEM_SPFILESYSTEM_H_ */
