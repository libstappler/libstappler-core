/**
Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023-2025 Stappler LLC <admin@stappler.dev>

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
#include "SPLog.h"

namespace STAPPLER_VERSIONIZED stappler::filesystem {

enum Access {
	Exists,
	Read,
	Write,
	Execute
};

enum class ProtectionFlags : uint32_t {
	None,
	Read = 1 << 0,
	Write = 1 << 1,
	Exec = 1 << 2,
};

SP_DEFINE_ENUM_AS_MASK(ProtectionFlags)

enum class MappingType {
	Private,
	Shared
};

struct Stat {
	size_t size = 0;
	Time atime;
	Time ctime;
	Time mtime;
	bool isDir = false;
};

class SP_PUBLIC File final {
public:
	enum class Flags {
		None,
		DelOnClose
	};

	using traits_type = std::char_traits<char>;
	using streamsize = std::streamsize;
	using int_type = typename traits_type::int_type;

	static File open_tmp(StringView prefix, bool delOnClose = true);

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
	explicit operator bool() const { return is_open(); }

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

class MemoryMappedRegion final {
public:
	using PlatformStorage = std::array<uint8_t, 16>;

	static MemoryMappedRegion mapFile(StringView, MappingType, ProtectionFlags,
			size_t offset = 0, size_t len = maxOf<size_t>());

	~MemoryMappedRegion();

	MemoryMappedRegion(MemoryMappedRegion &&);
	MemoryMappedRegion & operator=(MemoryMappedRegion &&);

	MemoryMappedRegion(const MemoryMappedRegion &) = delete;
	MemoryMappedRegion & operator=(const MemoryMappedRegion &) = delete;

	MappingType getType() const { return _type; }
	ProtectionFlags getProtectionFlags() const { return _prot; }

	uint8_t * getRegion() const { return _region; }

	operator bool() const { return _region != nullptr; }

	void sync();

protected:
	MemoryMappedRegion();
	MemoryMappedRegion(PlatformStorage &&, uint8_t *, MappingType, ProtectionFlags);

	PlatformStorage _storage;
	uint8_t *_region;
	MappingType _type;
	ProtectionFlags _prot;
};

// Check if file at path exists
SP_PUBLIC bool exists(StringView path);

SP_PUBLIC bool stat(StringView path, Stat &);

// create dir at path (just mkdir, not mkdir -p)
SP_PUBLIC bool mkdir(StringView path);

// mkdir -p (
SP_PUBLIC bool mkdir_recursive(StringView path, bool appWide = true);

// touch (set mtime to now) file
SP_PUBLIC bool touch(StringView path);

// move file from source to dest (tries to rename file, then copy-remove, rename will be successful only if both path is on single physical drive)
SP_PUBLIC bool move(StringView source, StringView dest);

// copy file or directory to dest; use ftw_b for dirs, no directory tree check
SP_PUBLIC bool copy(StringView source, StringView dest, bool stopOnError = true);

// remove file or directory
// if not recursive, only single file or empty dir will be removed
// if withDirs == false, only file s in directory tree will be removed
SP_PUBLIC bool remove(StringView path, bool recursive = false, bool withDirs = false);

// file-tree-walk, walk across directory tree at path, callback will be called for each file or directory
// path in callback is absolute
// depth = -1 - unlimited
// dirFirst == true - directory will be shown before files inside them, useful for listings and copy
// dirFirst == false - directory will be shown after files, useful for remove
SP_PUBLIC void ftw(StringView path, const Callback<void(StringView path, bool isFile)> &, int depth = -1, bool dirFirst = false);

// same as ftw, but iteration can be stopped by returning false from callback
SP_PUBLIC bool ftw_b(StringView path, const Callback<bool(StringView path, bool isFile)> &, int depth = -1, bool dirFirst = false);

// returns application writable path (or path inside writable dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Writable path should be used for sharable, but not valuable contents,
// or caches, that should not be removed, when application is running or in background
// On android, writable path is on same drive or device, that used for application file
// This library use writable path to store fonts, icons caches and assets
template <typename Interface>
SP_PUBLIC auto writablePath(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto writablePathReadOnly(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

// returns application documents path (or path inside documents dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Documents path should be used for valuable data, like documents, created by user,
// or content, that will be hard to recreate
// This library stores StoreKit and purchases data in documents dir
template <typename Interface>
SP_PUBLIC auto documentsPath(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto documentsPathReadOnly(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

// returns application current work dir from getcwd (or path inside current dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Current work dir is technical concept. Use it only if there is good reason for it
template <typename Interface>
SP_PUBLIC auto currentDir(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

// returns application caches dir (or path inside caches dir, if path is set
// if relative == false - do not merge paths, if provided path is absolute
//
// Caches dir used to store caches or content, that can be easily recreated,
// and that can be removed/erased, when application is active or in background
// On android, caches will be placed on SD card, if it's available
template <typename Interface>
SP_PUBLIC auto cachesPath(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto cachesPathReadOnly(StringView path = StringView(), bool relative = false) -> typename Interface::StringType;

// returns path, from which loadable resource can be read (from application bundle or dedicated resource directory)
template <typename Interface>
SP_PUBLIC auto loadableResourcePath(StringView path) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto loadableResourcePath(FilePath path) -> typename Interface::StringType;


// write data into file on path
SP_PUBLIC bool write(StringView path, const uint8_t *data, size_t len);

template <typename BytesView>
inline bool write(StringView path, const BytesView &view) {
	return write(path, reinterpret_cast<const uint8_t *>(view.data()), size_t(view.size()));
}

SP_PUBLIC File openForReading(StringView path);

// read file to string (if it was a binary file, string will be invalid)
template <typename Interface>
SP_PUBLIC auto readTextFile(StringView path) -> typename Interface::StringType;

SP_PUBLIC bool readIntoBuffer(uint8_t *buf, StringView path, size_t off = 0, size_t size = maxOf<size_t>());
SP_PUBLIC bool readWithConsumer(const io::Consumer &stream, uint8_t *buf, size_t bsize, StringView path, size_t off, size_t size);

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

SP_PUBLIC StringView detectMimeType(StringView path);

}

// *::platform::* functions handles some FS actions in platform-specific way
// (like inside Android apk or MacOS app bundle)
namespace STAPPLER_VERSIONIZED stappler::filesystem::platform {

#if ANDROID
SP_PUBLIC void Android_initializeFilesystem(void *assetManager, StringView filesDir, StringView cachesDir, StringView apkPath);
SP_PUBLIC void Android_terminateFilesystem();
SP_PUBLIC StringView Android_getApkPath();
#endif

template <typename Interface>
SP_PUBLIC auto _getApplicationPath() -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto _getWritablePath(bool readOnly) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto _getDocumentsPath(bool readOnly) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto _getCachesPath(bool readOnly) -> typename Interface::StringType;

SP_PUBLIC bool _exists(StringView path, bool assetsRoot = true);
SP_PUBLIC bool _stat(StringView path, Stat &, bool assetsRoot = true);

SP_PUBLIC File _openForReading(StringView);
SP_PUBLIC size_t _read(void *, uint8_t *buf, size_t nbytes);
SP_PUBLIC size_t _seek(void *, int64_t offset, io::Seek s);
SP_PUBLIC size_t _tell(void *);
SP_PUBLIC bool _eof(void *);
SP_PUBLIC void _close(void *);

SP_PUBLIC void _ftw(StringView path, const Callback<void(StringView path, bool isFile)> &, int depth, bool dirFirst, bool assetsRoot = true);

SP_PUBLIC bool _ftw_b(StringView path, const Callback<bool(StringView path, bool isFile)> &, int depth, bool dirFirst, bool assetsRoot = true);

SP_PUBLIC uint32_t _getMemoryPageSize();
SP_PUBLIC uint8_t *_mapFile(uint8_t storage[16], StringView path, MappingType type, ProtectionFlags prot, size_t offset, size_t len);
SP_PUBLIC bool _unmapFile(uint8_t *region, uint8_t storage[16]);
SP_PUBLIC bool _syncMappedRegion(uint8_t *region, uint8_t storage[16]);

}

// *::native::* functions (in contrast with ::platform::) works with native platform filesystem directly
// (so, not in bundle or archive)
//
// libstappler uses posix path scheme, it should be transformed when transferred from/to other apps/libraries
// No transformation required within libstappler itself
namespace STAPPLER_VERSIONIZED stappler::filesystem::native {

// C:\dirname\filename -> /c/dirname/filename
template <typename Interface>
SP_PUBLIC auto nativeToPosix(StringView path) -> typename Interface::StringType;

// /c/dirname/filename -> C:\dirname\filename
// be sure that path is absolute
template <typename Interface>
SP_PUBLIC auto posixToNative(StringView path) -> typename Interface::StringType;

template <typename Interface>
SP_PUBLIC auto getcwd_fn() -> typename Interface::StringType;

SP_PUBLIC bool remove_fn(StringView path);
SP_PUBLIC bool unlink_fn(StringView path);
SP_PUBLIC bool mkdir_fn(StringView path);

SP_PUBLIC bool access_fn(StringView path, Access mode);

SP_PUBLIC bool stat_fn(StringView path, Stat &);

SP_PUBLIC bool touch_fn(StringView path);

SP_PUBLIC void ftw_fn(StringView path, const Callback<void(StringView path, bool isFile)> &, int depth, bool dirFirst);
SP_PUBLIC bool ftw_b_fn(StringView path, const Callback<bool(StringView path, bool isFile)> &, int depth, bool dirFirst);

SP_PUBLIC bool rename_fn(StringView source, StringView dest);

SP_PUBLIC FILE *fopen_fn(StringView, StringView mode);

SP_PUBLIC bool write_fn(StringView, const unsigned char *data, size_t len);

}

namespace STAPPLER_VERSIONIZED stappler::filesystem {

template <typename Interface>
SP_PUBLIC inline auto writablePath(StringView path, bool relative) -> typename Interface::StringType {
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
SP_PUBLIC inline auto writablePathReadOnly(StringView path, bool relative) -> typename Interface::StringType {
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
SP_PUBLIC inline auto cachesPath(StringView path, bool relative) -> typename Interface::StringType {
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
SP_PUBLIC inline auto cachesPathReadOnly(StringView path, bool relative) -> typename Interface::StringType {
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
SP_PUBLIC inline auto documentsPath(StringView path, bool relative) -> typename Interface::StringType {
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
SP_PUBLIC inline auto documentsPathReadOnly(StringView path, bool relative) -> typename Interface::StringType {
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
SP_PUBLIC inline auto currentDir(StringView path, bool relative) -> typename Interface::StringType {
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
SP_PUBLIC inline auto loadableResourcePath(StringView path) -> typename Interface::StringType {
	if (filepath::isAbsolute(path)) {
		// absolute path handled by native functions
		if (filesystem::native::access_fn(path, Access::Exists)) {
			return path.str<Interface>();
		}
	} else if (filepath::isBundled(path) && filesystem::platform::_exists(path)) {
		// path forced to be bundled via %PLATFORM% prefix
		return path.str<Interface>();
	} else if (!filepath::isAboveRoot(path)) {
		if (filesystem::platform::_exists(path)) {
			// path can be found within bundle
			return path.str<Interface>();
		} else {
			typename Interface::StringType npath;

			// Try application-bin-relative
			// On windows desktop apps it conventionally like app bundle
			npath = filepath::merge<Interface>(filesystem::platform::_getApplicationPath<Interface>(), path);
			if (filesystem::exists(npath)) {
				return npath;
			}

			// Try writable path
			npath = filesystem::writablePathReadOnly<Interface>(path);
			if (filesystem::exists(npath)) {
				return npath;
			}

			// Try current-dir relative
			npath = filesystem::currentDir<Interface>(path);
			if (filesystem::exists(npath)) {
				return npath;
			}
		}
	}

	log::warn("filesystem", "No path found for resource: ", path);
	// not a loadable resource path
	return typename Interface::StringType();
}

template <typename Interface>
SP_PUBLIC auto loadableResourcePath(FilePath path) -> typename Interface::StringType {
	return loadableResourcePath<Interface>(path.get());
}

template <typename Interface>
SP_PUBLIC auto readTextFile(StringView ipath) -> typename Interface::StringType {
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

namespace STAPPLER_VERSIONIZED stappler::io {

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
