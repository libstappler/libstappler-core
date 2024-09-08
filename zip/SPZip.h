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

typedef struct zip zip_t;

namespace STAPPLER_VERSIONIZED stappler {

template <typename Interface>
struct ZipBuffer {
	using Buffer = BufferTemplate<Interface>;

	Buffer data;
	Buffer buffer;
};

template <typename Interface>
class SP_PUBLIC ZipArchive : public Interface::AllocBaseType {
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
	static zip_t *createZipArchive(BytesView b, ZipBuffer<Interface> *d);

	ZipBuffer<Interface> _data;
	zip_t *_handle = nullptr;
};

template <typename Interface>
ZipArchive<Interface>::ZipArchive() : ZipArchive(BytesView()) { }

template <typename Interface>
ZipArchive<Interface>::ZipArchive(BytesView b) {
	_handle = createZipArchive(b, &_data);
}

template <typename Interface>
bool ZipArchive<Interface>::addFile(StringView name, StringView data, bool uncompressed) {
	return addFile(name, BytesView((const uint8_t *)data.data(), data.size()), uncompressed);
}

}

#endif /* STAPPLER_ZIP_SPZIP_H_ */
