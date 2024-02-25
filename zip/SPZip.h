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

#ifndef STAPPLER_ZIP_SPZIP_H_
#define STAPPLER_ZIP_SPZIP_H_

#include "SPBytesView.h"
#include "SPBuffer.h"
#include "SPTime.h"
#include "SPLog.h"
#include "zip.h"

namespace STAPPLER_VERSIONIZED stappler {

template <typename Interface>
class ZipArchive : public Interface::AllocBaseType {
public:
	using Bytes = typename Interface::BytesType;
	using Buffer = BufferTemplate<Interface>;

	ZipArchive();
	ZipArchive(BytesView);
	ZipArchive(FILE *, bool readonly);

	~ZipArchive();

	bool addDir(StringView name);
	bool addFile(StringView name, BytesView data, bool uncompressed = false);
	bool addFile(StringView name, StringView data, bool uncompressed = false);

	Buffer save();

	explicit operator bool () { return _handle != nullptr; }

	size_t size(bool original = false) const;
	StringView getName(size_t idx, bool original = false) const;

	void ftw(const Callback<void(StringView path, size_t size, Time time)> &, bool original = false) const;

protected:
	Buffer _data;
	Buffer _buffer;

	zip_t *_handle = nullptr;
};

template <typename Interface>
ZipArchive<Interface>::ZipArchive() : ZipArchive(BytesView()) { }

template <typename Interface>
ZipArchive<Interface>::ZipArchive(BytesView b) {
	if (!b.empty()) {
		_data.put(b.data(), b.size());
	}
	auto source = zip_source_function_create([] (void *ud, void *data, zip_uint64_t size, zip_source_cmd_t cmd) -> zip_int64_t {
		auto d = (ZipArchive<Interface> *)ud;
		switch (cmd) {
		case ZIP_SOURCE_REMOVE:
		case ZIP_SOURCE_OPEN:
		case ZIP_SOURCE_CLOSE:
		case ZIP_SOURCE_FREE:
			/* do nothing */
			return 0;
			break;
		case ZIP_SOURCE_READ: {
			auto v = d->_data.read(size);
			memcpy(data, v.data(), v.size());
			return v.size();
			break;
		}
		case ZIP_SOURCE_STAT: {
			zip_stat_t *stat = (zip_stat_t *)data;
			zip_stat_init(stat);
			stat->valid = ZIP_STAT_SIZE;
			stat->size = d->_data.input();
			return sizeof(struct zip_stat);
			break; /* get meta information */
		}
		case ZIP_SOURCE_ERROR: {
			int * errdata = (int *)data;
			errdata[0] = ZIP_ER_INTERNAL;
			errdata[1] = EINVAL;
			break; /* get error information */
		}
		case ZIP_SOURCE_SEEK_WRITE:
			d->_buffer.seek(zip_source_seek_compute_offset(d->_buffer.size(), d->_buffer.input(), data, size, nullptr));
			return 0;
			break; /* get write position */
		case ZIP_SOURCE_SEEK:
			d->_data.seek(zip_source_seek_compute_offset(d->_data.size(), d->_data.input(), data, size, nullptr));
			return 0;
			break;  /* set position for reading */
		case ZIP_SOURCE_TELL_WRITE:
			return d->_buffer.size();
			break; /* get write position */
		case ZIP_SOURCE_TELL:
			return d->_data.size();
			break; /* get read position */
		case ZIP_SOURCE_SUPPORTS: {
			auto supports = zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT, ZIP_SOURCE_ERROR,
					ZIP_SOURCE_FREE, ZIP_SOURCE_SEEK, ZIP_SOURCE_TELL, ZIP_SOURCE_SUPPORTS, ZIP_SOURCE_BEGIN_WRITE, ZIP_SOURCE_COMMIT_WRITE,
					ZIP_SOURCE_ROLLBACK_WRITE, ZIP_SOURCE_SEEK_WRITE, ZIP_SOURCE_TELL_WRITE, ZIP_SOURCE_REMOVE, ZIP_SOURCE_WRITE);
			return supports;
			break;   /* check whether source supports command */
		}
		case ZIP_SOURCE_BEGIN_WRITE:
			d->_buffer.clear();
			d->_buffer = d->_data;
			return 0;
			break; /* prepare for writing */
		case ZIP_SOURCE_COMMIT_WRITE:
			d->_data = move(d->_buffer);
			d->_buffer.clear();
			return 0;
			break; /* writing is done */
		case ZIP_SOURCE_ROLLBACK_WRITE:
			d->_buffer.clear();
			return 0;
			break; /* discard written changes */
		case ZIP_SOURCE_WRITE:
			return d->_buffer.put((const uint8_t *)data, size);
			break; /* write data */
		default:
			break;
		}
		return -1;
	}, this, nullptr);

	zip_error_t err;

	_handle = zip_open_from_source(source, ZIP_CREATE | ZIP_TRUNCATE, &err);
	if (!_handle) {
		log::warn("ZipArchive", "Fail to create archive: ", err.str);
	}
}

template <typename Interface>
ZipArchive<Interface>::ZipArchive(FILE *file, bool readonly) {
	auto source = zip_source_filep_create(file, 0, -1, nullptr);

	_handle = zip_open_from_source(source, readonly ? ZIP_RDONLY : 0, nullptr);
}

template <typename Interface>
ZipArchive<Interface>::~ZipArchive() {
	if (_handle) {
		zip_discard(_handle);
		_handle = nullptr;
	}
}

template <typename Interface>
bool ZipArchive<Interface>::addDir(StringView name) {
	return zip_dir_add(_handle, name.terminated() ? name.data() : name.str<Interface>().data(), ZIP_FL_ENC_UTF_8) >= 0;
}

template <typename Interface>
bool ZipArchive<Interface>::addFile(StringView name, BytesView data, bool uncompressed) {
	zip_source_t *source = nullptr;
	uint8_t *buf = nullptr;

	if constexpr (std::is_same<Interface, memory::PoolInterface>::value) {
		buf = (uint8_t *)memory::pool::palloc(memory::pool::acquire(), data.size());
		memcpy(buf, data.data(), data.size());
		source = zip_source_buffer(_handle, buf, data.size(), 0);
	} else {
		buf = new uint8_t[data.size()];
		memcpy(buf, data.data(), data.size());
		source = zip_source_buffer(_handle, buf, data.size(), 1);
	}
	if (source) {
		auto idx = zip_file_add(_handle, name.terminated() ? name.data() : name.str<Interface>().data(), source, ZIP_FL_ENC_UTF_8);
		if (idx < 0) {
			auto err = zip_get_error(_handle);
			if (err) {
				std::cout << "ZIP error: " << zip_error_strerror(err) << "\n";
			}

			zip_source_free(source);
			return false;
		}

		if (uncompressed) {
			zip_set_file_compression(_handle, idx, ZIP_CM_STORE, 0);
		}

		return true;
	}
	return false;
}

template <typename Interface>
bool ZipArchive<Interface>::addFile(StringView name, StringView data, bool uncompressed) {
	return addFile(name, BytesView((const uint8_t *)data.data(), data.size()), uncompressed);
}

template <typename Interface>
auto ZipArchive<Interface>::save() -> ZipArchive<Interface>::Buffer {
	auto err = zip_close(_handle);
	if (err < 0) {
		zip_discard(_handle);
		return Buffer();
	}
	_handle = nullptr;
	return move(_data);
}

template <typename Interface>
size_t ZipArchive<Interface>::size(bool original) const {
	return zip_get_num_entries(_handle, original ? ZIP_FL_UNCHANGED : 0);
}

template <typename Interface>
StringView ZipArchive<Interface>::getName(size_t idx, bool original) const {
	return zip_get_name(_handle, idx, original ? ZIP_FL_UNCHANGED | ZIP_FL_ENC_GUESS : ZIP_FL_ENC_GUESS);
}

template <typename Interface>
void ZipArchive<Interface>::ftw(const Callback<void(StringView path, size_t size, Time time)> &cb, bool original) const {
	zip_stat_t stat;
	for (size_t i = 0; i < size(original); ++ i) {
		zip_stat_index(_handle, i, ZIP_STAT_SIZE | ZIP_STAT_MTIME | ZIP_STAT_NAME, &stat);
		cb(stat.name, stat.size, Time::seconds(stat.mtime));
	}
}

}

#endif /* STAPPLER_ZIP_SPZIP_H_ */
