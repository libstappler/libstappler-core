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

#include "SPFilesystem.h"

#if __APPLE__
#include <unistd.h>
#endif

#if ANDROID
#include <unistd.h>
#endif

#if LINUX
#include <unistd.h>
#endif

namespace stappler::filepath {

static bool inAppBundle(StringView path);

}

namespace stappler::filesystem {

File File::open_tmp(const char *prefix, bool delOnClose) {
	if (prefix == nullptr) {
		prefix = "sa.tmp";
	}
	char buf[256] = { 0 };
	const char *tmp = P_tmpdir;
	size_t len = strlen(tmp);
	strcpy(&buf[0], tmp);
	strcpy(&buf[len], "/");
	strcpy(&buf[len + 1], prefix);
	len += strlen(prefix);
	strcpy(&buf[len + 1], "XXXXXX");

	if (auto fd = ::mkstemp(buf)) {
		if (auto f = ::fdopen(fd, "wb+")) {
			auto ret = File(f, delOnClose ? Flags::DelOnClose : Flags::None);
			ret.set_tmp_path(buf);
			return ret;
		}
	}
	return File();
}

File::File() : _isBundled(false), _nativeFile(nullptr) { }
File::File(FILE *f, Flags flags) : _isBundled(false), _flags(flags), _nativeFile(f) {
	if (is_open()) {
		auto pos = seek(0, io::Seek::Current);
		auto size = seek(0, io::Seek::End);
		if (pos != maxOf<size_t>()) {
			seek(pos, io::Seek::Set);
		}
		_size = (size != maxOf<size_t>())?size:0;
	}
}
File::File(void *f) : _isBundled(true), _platformFile(f) {
	if (is_open()) {
		auto pos = seek(0, io::Seek::Current);
		auto size = seek(0, io::Seek::End);
		if (pos != maxOf<size_t>()) {
			seek(pos, io::Seek::Set);
		}
		_size = (size != maxOf<size_t>())?size:0;
	}
}

File::File(void *f, size_t s) : _isBundled(true), _size(s), _platformFile(f) { }

File::File(File && f) : _isBundled(f._isBundled), _size(f._size) {
	if (_isBundled) {
		_platformFile = f._platformFile;
		f._platformFile = nullptr;
	} else {
		_nativeFile = f._nativeFile;
		f._nativeFile = nullptr;
	}
	f._size = 0;
	if (f._buf[0] != 0) {
		memcpy(_buf, f._buf, 256);
	}
}

File & File::operator=(File && f) {
	_isBundled = f._isBundled;
	_size = f._size;
	if (_isBundled) {
		_platformFile = f._platformFile;
		f._platformFile = nullptr;
	} else {
		_nativeFile = f._nativeFile;
		f._nativeFile = nullptr;
	}
	f._size = 0;
	if (f._buf[0] != 0) {
		memcpy(_buf, f._buf, 256);
	}
	return *this;
}

File::~File() {
	close();
}

size_t File::read(uint8_t *buf, size_t nbytes) {
	if (is_open()) {
		if (!_isBundled) {
			size_t remains = _size - ftell(_nativeFile);
			if (nbytes > remains) {
				nbytes = remains;
			}
			if (fread(buf, nbytes, 1, _nativeFile) == 1) {
				return nbytes;
			}
		} else {
			return filesystem::platform::_read(_platformFile, buf, nbytes);
		}
	}
	return 0;
}

size_t File::seek(int64_t offset, io::Seek s) {
	if (is_open()) {
		if (!_isBundled) {
			int whence = SEEK_SET;
			switch (s) {
			case io::Seek::Set: whence = SEEK_SET; break;
			case io::Seek::Current: whence = SEEK_CUR; break;
			case io::Seek::End: whence = SEEK_END; break;
			}

			if (offset != 0 || s != io::Seek::Current) {
				if (fseek(_nativeFile, long(offset), whence) != 0) {
					return maxOf<size_t>();
				}
			}
			auto p = ftell(_nativeFile);
			if (p >= 0){
				return (size_t)p;
			} else {
				return maxOf<size_t>();
			}
		} else {
			return filesystem::platform::_seek(_platformFile, offset, s);
		}
	}
	return maxOf<size_t>();
}

size_t File::tell() const {
	if (!_isBundled) {
		auto p = ftell(_nativeFile);
		if (p >= 0) {
			return (size_t)p;
		} else {
			return maxOf<size_t>();
		}
	} else {
		return filesystem::platform::_tell(_platformFile);
	}
}

size_t File::size() const {
	return _size;
}

typename File::int_type File::xsgetc() {
	int_type ret = traits_type::eof();
	if (is_open()) {
		if (!_isBundled) {
			ret = fgetc(_nativeFile);
		} else {
			uint8_t buf = 0;
			if (read(&buf, 1) == 1) {
				ret = buf;
			}
		}
	}
	return ret;
}

typename File::int_type File::xsputc(int_type c) {
	int_type ret = traits_type::eof();
	if (is_open() && !_isBundled) {
		ret = fputc(c, _nativeFile);
	}
	return ret;
}

typename File::streamsize File::xsputn(const char* s, streamsize n) {
	streamsize ret = -1;
	if (is_open() && !_isBundled) {
		if (fwrite(s, n, 1, _nativeFile) == 1) {
			ret = n;
		}
	}
	return ret;
}

typename File::streamsize File::xsgetn(char* s, streamsize n) {
	streamsize ret = -1;
	if (is_open()) {
		ret = read((uint8_t *)s, n);
	}
	return ret;
}

bool File::eof() const {
	if (is_open()) {
		if (!_isBundled) {
			return feof(_nativeFile) != 0;
		} else {
			return filesystem::platform::_eof(_platformFile);
		}
	}
	return true;
}

void File::close() {
	if (is_open()) {
		if (!_isBundled) {
			fclose(_nativeFile);
			if (_flags != Flags::DelOnClose && _buf[0] != 0) {
				::unlink(_buf);
			}
			memset(_buf, 0, 256);
			_nativeFile = nullptr;
		} else {
			filesystem::platform::_close(_platformFile);
			_platformFile = nullptr;
		}
	}
}

void File::close_remove() {
	if (is_open()) {
		if (!_isBundled) {
			fclose(_nativeFile);
			if (_buf[0] != 0) {
				::unlink(_buf);
			}
			memset(_buf, 0, 256);
			_nativeFile = nullptr;
		} else {
			filesystem::platform::_close(_platformFile);
			_platformFile = nullptr;
		}
	}
}

bool File::close_rename(StringView path) {
	if (is_open()) {
		if (!_isBundled && _buf[0] != 0) {
			fclose(_nativeFile);
			_nativeFile = nullptr;
			if (move(_buf, path)) {
				memset(_buf, 0, 256);
				return true;
			} else {
				_nativeFile = fopen(_buf, "wb+");
			}
		}
	}
	return false;
}

bool File::is_open() const {
	return _nativeFile != nullptr || _platformFile != nullptr;
}

const char *File::path() const {
	if (_buf[0] == 0) {
		return nullptr;
	} else {
		return _buf;
	}
}

void File::set_tmp_path(const char *buf) {
	memcpy(_buf, buf, 256);
}

bool exists(StringView ipath) {
	if (filepath::isAbsolute(ipath)) {
		return filesystem::native::access_fn(ipath, Access::Exists);
	} else if (filepath::isBundled(ipath)) {
		return filesystem::platform::_exists(ipath);
	} else if (!filepath::isAboveRoot(ipath) && filesystem::platform::_exists(ipath)) {
		return true;
	} else {
		auto path = filepath::absolute<memory::StandartInterface>(ipath);
		return filesystem::native::access_fn(path, Access::Exists);
	}
}

bool stat(StringView path, Stat &stat) {
	if (filepath::inAppBundle(path)) {
		return filesystem::platform::_stat(path, stat);
	}

	return filesystem::native::stat_fn(path, stat);
}

bool remove(StringView ipath, bool recursive, bool withDirs) {
	if (filepath::inAppBundle(ipath)) {
		return false;
	}

	auto path = filepath::absolute<memory::StandartInterface>(ipath, true);
	if (!recursive) {
		return filesystem::native::remove_fn(path);
	} else {
		return ftw_b(path, [withDirs] (const StringView &fpath, bool isFile) -> bool {
			if (isFile || withDirs) {
				return remove(fpath);
			}
			return true;
		});
	}
}

bool touch(StringView ipath) {
	auto path = filepath::absolute<memory::StandartInterface>(ipath, true);
	if (filepath::isBundled(path)) {
		return false;
	}
	return filesystem::native::touch_fn(path);
}

bool mkdir(StringView ipath) {
	auto path = filepath::absolute<memory::StandartInterface>(ipath, true);
	return filesystem::native::mkdir_fn(path);
}

bool mkdir_recursive(StringView ipath, bool appWide) {
	auto path = filepath::absolute<memory::StandartInterface>(ipath, true);

	memory::StandartInterface::StringType appWideLimit;
	if (appWide) {
		do {
			auto testPath = cachesPath<memory::StandartInterface>();
			if (path.compare(0, std::min(path.size(), testPath.size()), testPath) == 0) {
				appWideLimit = std::move(testPath);
				break;
			}

			testPath = writablePath<memory::StandartInterface>();
			if (path.compare(0, std::min(path.size(), testPath.size()), testPath) == 0) {
				appWideLimit = std::move(testPath);
				break;
			}

			testPath = documentsPath<memory::StandartInterface>();
			if (path.compare(0, std::min(path.size(), testPath.size()), testPath) == 0) {
				appWideLimit = std::move(testPath);
				break;
			}

			testPath = currentDir<memory::StandartInterface>();
			if (path.compare(0, std::min(path.size(), testPath.size()), testPath) == 0) {
				appWideLimit = std::move(testPath);
				break;
			}
		} while (0);

		if (appWideLimit.empty()) {
			return false;
		}
	}

	auto components = filepath::split<memory::StandartInterface>(path);
	if (!components.empty()) {
		bool control = false;
		memory::StandartInterface::StringType construct("/");
		for (auto &it : components) {
			construct.append(it.data(), it.size());
			if (!appWide || construct.compare(0, std::min(construct.size(), appWideLimit.size()), appWideLimit) == 0) {
				Stat stat;
				if (control || !filesystem::native::stat_fn(path, stat)) {
					control = true;
					if (!filesystem::native::mkdir_fn(construct)) {
						return false;
					}
				} else if (!stat.isDir) {
					return false;
				}
			}
			construct.append("/");
		}
	}
	return true;
}

void ftw(StringView ipath, const Callback<void(StringView path, bool isFile)> &callback, int depth, bool dir_first) {
	auto path = filepath::absolute<memory::StandartInterface>(ipath, true);
	if (filepath::isBundled(path)) {
		filesystem::platform::_ftw(path, callback, depth, dir_first);
	} else {
		filesystem::native::ftw_fn(path, callback, depth, dir_first);
	}
}

bool ftw_b(StringView ipath, const Callback<bool(StringView path, bool isFile)> &callback, int depth, bool dir_first) {
	auto path = filepath::absolute<memory::StandartInterface>(ipath, true);
	if (filepath::isBundled(path)) {
		return filesystem::platform::_ftw_b(path, callback, depth, dir_first);
	} else {
		return filesystem::native::ftw_b_fn(path, callback, depth, dir_first);
	}
}

bool move(StringView isource, StringView idest) {
	auto source = filepath::absolute<memory::StandartInterface>(isource, true);
	auto dest = filepath::absolute<memory::StandartInterface>(idest, true);

	if (!filesystem::native::rename_fn(source, dest)) {
		if (copy(source, dest)) {
			return remove(source);
		}
		return false;
	}
	return true;
}

static bool performCopy(StringView source, StringView dest) {
	if (stappler::filesystem::exists(dest)) {
		stappler::filesystem::remove(dest);
	}
	if (!stappler::filesystem::exists(dest)) {
		std::ofstream destStream(dest.data(), std::ios::binary);
		auto f = openForReading(source);
		if (f && destStream.is_open()) {
			if (io::read(f, io::Consumer(destStream)) > 0) {
				return true;
			}
		}
		destStream.close();
	}
	return false;
}

static bool isdir(StringView path) {
	Stat stat;
	native::stat_fn(path, stat);
	return stat.isDir;
}

bool copy(StringView isource, StringView idest, bool stopOnError) {
	auto source = filepath::absolute<memory::StandartInterface>(isource, true);
	auto dest = filepath::absolute<memory::StandartInterface>(idest, true);
	if (dest.back() == '/') {
		dest = filepath::merge<memory::StandartInterface>(dest, filepath::lastComponent(source));
	} else if (isdir(dest) && filepath::lastComponent(source) != filepath::lastComponent(dest)) {
		dest = filepath::merge<memory::StandartInterface>(dest, filepath::lastComponent(source));
	}
	if (!isdir(source)) {
		return performCopy(source, dest);
	} else {
		return ftw_b(source, [source, dest, stopOnError] (StringView path, bool isFile) -> bool {
			if (!isFile) {
				if (path == source) {
					mkdir(filepath::replace<memory::StandartInterface>(path, source, dest));
					return true;
				}
				bool ret = mkdir(filepath::replace<memory::StandartInterface>(path, source, dest));
				if (stopOnError) {
					return ret;
				} else {
					return true;
				}
			} else {
				bool ret = performCopy(path, filepath::replace<memory::StandartInterface>(path, source, dest));
				if (stopOnError) {
					return ret;
				} else {
					return true;
				}
			}
		}, -1, true);
	}
}

bool write(StringView ipath, const unsigned char *data, size_t len) {
	auto path = filepath::absolute<memory::StandartInterface>(ipath, true);
	std::ofstream f(path.data());
	if (f.is_open()) {
		f.write((const char *)data, len);
		f.close();
		return true;
	}
	return false;
}

File openForReading(StringView ipath) {
	if (filepath::inAppBundle(ipath)) {
		return filesystem::platform::_openForReading(ipath);
	}
	auto path = filepath::absolute<memory::StandartInterface>(ipath);
	auto f = filesystem::native::fopen_fn(path, "rb");
	if (f) {
		return File(f);
	}
	return File();
}

bool readIntoBuffer(uint8_t *buf, const StringView &ipath, size_t off, size_t size) {
	auto f = openForReading(ipath);
	if (f) {
		size_t fsize = f.size();
		if (fsize <= off) {
			f.close();
			return false;
		}
		if (fsize - off < size) {
			size = fsize - off;
		}

		bool ret = true;
		f.seek(off, io::Seek::Set);
		if (f.read(buf, size) != size) {
			ret = false;
		}
		f.close();
		return ret;
	}
	return false;
}

bool readWithConsumer(const io::Consumer &stream, uint8_t *buf, size_t bsize, const StringView &ipath, size_t off, size_t size) {
	auto f = openForReading(ipath);
	if (f) {
		size_t fsize = f.size();
		if (fsize <= off) {
			f.close();
			return false;
		}
		if (fsize - off < size) {
			size = fsize - off;
		}

		bool ret = true;
		f.seek(off, io::Seek::Set);
		while (size > 0) {
			auto read = min(size, bsize);
			if (f.read(buf, read) == read) {
				stream.write(buf, read);
			} else {
				ret = false;
				break;
			}
			size -= read;
		}
		f.close();
		return ret;
	}
	return false;
}

}
