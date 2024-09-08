/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

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

#include "SPCommon.h"
#include "SPZip.h"
#include "zip.h"

namespace STAPPLER_VERSIONIZED stappler {

template <typename Interface>
static zip_t *_createZipArchive(BytesView b, ZipBuffer<Interface> *d) {
	if (!b.empty()) {
		d->data.put(b.data(), b.size());
	}
	auto source = zip_source_function_create([] (void *ud, void *data, zip_uint64_t size, zip_source_cmd_t cmd) -> zip_int64_t {
		auto d = (ZipBuffer<Interface> *)ud;
		switch (cmd) {
		case ZIP_SOURCE_REMOVE:
		case ZIP_SOURCE_OPEN:
		case ZIP_SOURCE_CLOSE:
		case ZIP_SOURCE_FREE:
			/* do nothing */
			return 0;
			break;
		case ZIP_SOURCE_READ: {
			auto v = d->data.read(size);
			memcpy(data, v.data(), v.size());
			return v.size();
			break;
		}
		case ZIP_SOURCE_STAT: {
			zip_stat_t *stat = (zip_stat_t *)data;
			zip_stat_init(stat);
			stat->valid = ZIP_STAT_SIZE;
			stat->size = d->data.input();
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
			d->buffer.seek(zip_source_seek_compute_offset(d->buffer.size(), d->buffer.input(), data, size, nullptr));
			return 0;
			break; /* get write position */
		case ZIP_SOURCE_SEEK:
			d->data.seek(zip_source_seek_compute_offset(d->data.size(), d->data.input(), data, size, nullptr));
			return 0;
			break;  /* set position for reading */
		case ZIP_SOURCE_TELL_WRITE:
			return d->buffer.size();
			break; /* get write position */
		case ZIP_SOURCE_TELL:
			return d->data.size();
			break; /* get read position */
		case ZIP_SOURCE_SUPPORTS: {
			auto supports = zip_source_make_command_bitmap(ZIP_SOURCE_OPEN, ZIP_SOURCE_READ, ZIP_SOURCE_CLOSE, ZIP_SOURCE_STAT, ZIP_SOURCE_ERROR,
					ZIP_SOURCE_FREE, ZIP_SOURCE_SEEK, ZIP_SOURCE_TELL, ZIP_SOURCE_SUPPORTS, ZIP_SOURCE_BEGIN_WRITE, ZIP_SOURCE_COMMIT_WRITE,
					ZIP_SOURCE_ROLLBACK_WRITE, ZIP_SOURCE_SEEK_WRITE, ZIP_SOURCE_TELL_WRITE, ZIP_SOURCE_REMOVE, ZIP_SOURCE_WRITE);
			return supports;
			break;   /* check whether source supports command */
		}
		case ZIP_SOURCE_BEGIN_WRITE:
			d->buffer.clear();
			d->buffer = d->data;
			return 0;
			break; /* prepare for writing */
		case ZIP_SOURCE_COMMIT_WRITE:
			d->data = move(d->buffer);
			d->buffer.clear();
			return 0;
			break; /* writing is done */
		case ZIP_SOURCE_ROLLBACK_WRITE:
			d->buffer.clear();
			return 0;
			break; /* discard written changes */
		case ZIP_SOURCE_WRITE:
			return d->buffer.put((const uint8_t *)data, size);
			break; /* write data */
		default:
			break;
		}
		return -1;
	}, d, nullptr);

	zip_error_t err;
	auto handle = zip_open_from_source(source, ZIP_CREATE | ZIP_TRUNCATE, &err);
	if (!handle) {
		log::warn("ZipArchive", "Fail to create archive: ", err.str);
	}
	return handle;
}

static zip_t *_createZipArchive(FILE *file, bool readonly) {
	auto source = zip_source_filep_create(file, 0, -1, nullptr);
	return zip_open_from_source(source, readonly ? ZIP_RDONLY : 0, nullptr);
}

template <typename Interface>
static bool addFileToArchive(zip_t *_handle, StringView name, BytesView data, bool uncompressed) {
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

template <>
zip_t *ZipArchive<memory::StandartInterface>::createZipArchive(BytesView b, ZipBuffer<memory::StandartInterface> *d) {
	return _createZipArchive(b, d);
}

template <>
zip_t *ZipArchive<memory::PoolInterface>::createZipArchive(BytesView b, ZipBuffer<memory::PoolInterface> *d) {
	return _createZipArchive(b, d);
}

template <>
ZipArchive<memory::StandartInterface>::ZipArchive(FILE *file, bool readonly) {
	_handle = _createZipArchive(file, readonly);
}

template <>
ZipArchive<memory::PoolInterface>::ZipArchive(FILE *file, bool readonly) {
	_handle = _createZipArchive(file, readonly);
}

template <>
ZipArchive<memory::StandartInterface>::~ZipArchive() {
	if (_handle) {
		zip_discard(_handle);
		_handle = nullptr;
	}
}

template <>
ZipArchive<memory::PoolInterface>::~ZipArchive() {
	if (_handle) {
		zip_discard(_handle);
		_handle = nullptr;
	}
}

template <>
bool ZipArchive<memory::StandartInterface>::addDir(StringView name) {
	return zip_dir_add(_handle, name.terminated() ? name.data() : name.str<memory::StandartInterface>().data(), ZIP_FL_ENC_UTF_8) >= 0;
}

template <>
bool ZipArchive<memory::PoolInterface>::addDir(StringView name) {
	return zip_dir_add(_handle, name.terminated() ? name.data() : name.str<memory::PoolInterface>().data(), ZIP_FL_ENC_UTF_8) >= 0;
}

template <>
bool ZipArchive<memory::StandartInterface>::addFile(StringView name, BytesView data, bool uncompressed) {
	return addFileToArchive<memory::StandartInterface>(_handle, name, data, uncompressed);
}

template <>
bool ZipArchive<memory::PoolInterface>::addFile(StringView name, BytesView data, bool uncompressed) {
	return addFileToArchive<memory::PoolInterface>(_handle, name, data, uncompressed);
}

template <>
auto ZipArchive<memory::StandartInterface>::save() -> BufferTemplate<memory::StandartInterface> {
	auto err = zip_close(_handle);
	if (err < 0) {
		zip_discard(_handle);
		return Buffer();
	}
	_handle = nullptr;
	return move(_data.data);
}

template <>
auto ZipArchive<memory::PoolInterface>::save() -> BufferTemplate<memory::PoolInterface> {
	auto err = zip_close(_handle);
	if (err < 0) {
		zip_discard(_handle);
		return Buffer();
	}
	_handle = nullptr;
	return move(_data.data);
}

template <>
size_t ZipArchive<memory::StandartInterface>::size(bool original) const {
	return zip_get_num_entries(_handle, original ? ZIP_FL_UNCHANGED : 0);
}

template <>
size_t ZipArchive<memory::PoolInterface>::size(bool original) const {
	return zip_get_num_entries(_handle, original ? ZIP_FL_UNCHANGED : 0);
}

template <>
StringView ZipArchive<memory::StandartInterface>::getName(size_t idx, bool original) const {
	return zip_get_name(_handle, idx, original ? ZIP_FL_UNCHANGED | ZIP_FL_ENC_GUESS : ZIP_FL_ENC_GUESS);
}

template <>
StringView ZipArchive<memory::PoolInterface>::getName(size_t idx, bool original) const {
	return zip_get_name(_handle, idx, original ? ZIP_FL_UNCHANGED | ZIP_FL_ENC_GUESS : ZIP_FL_ENC_GUESS);
}

template <>
void ZipArchive<memory::StandartInterface>::ftw(const Callback<void(StringView path, size_t size, Time time)> &cb, bool original) const {
	zip_stat_t stat;
	for (size_t i = 0; i < size(original); ++ i) {
		zip_stat_index(_handle, i, ZIP_STAT_SIZE | ZIP_STAT_MTIME | ZIP_STAT_NAME, &stat);
		cb(stat.name, stat.size, Time::seconds(stat.mtime));
	}
}

template <>
void ZipArchive<memory::PoolInterface>::ftw(const Callback<void(StringView path, size_t size, Time time)> &cb, bool original) const {
	zip_stat_t stat;
	for (size_t i = 0; i < size(original); ++ i) {
		zip_stat_index(_handle, i, ZIP_STAT_SIZE | ZIP_STAT_MTIME | ZIP_STAT_NAME, &stat);
		cb(stat.name, stat.size, Time::seconds(stat.mtime));
	}
}

}
