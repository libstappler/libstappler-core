/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023-2024 Stappler LLC <admin@stappler.dev>

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

#ifndef STAPPLER_BITMAP_SPBITMAP_H_
#define STAPPLER_BITMAP_SPBITMAP_H_

#include "SPBitmapFormat.h"
#include "SPFilepath.h"
#include "SPLog.h" // for SPASSERT

namespace STAPPLER_VERSIONIZED stappler::bitmap {

enum class ResampleFilter {
	Box, // "box"
	Tent, // "tent"
	Bell, // "bell"
	BSpline, // "b-spline"
	Mitchell, // "mitchell"
	Lanczos3, // "lanczos3"
	Blackman, // "blackman"
	Lanczos4, // "Lanczos4"
	Lanczos6, // "lanczos6"
	Lanczos12, // "lanczos12"
	Kaiser, // "kaiser"
	Gaussian, // "gaussian"
	Catmullrom, // "catmullrom"
	QuadInterp, // "quadratic_interp"
	QuadApprox, // "quadratic_approx"
	QuadMix, // "quadratic_mix"

	Default = Lanczos4,
};

template <typename Interface>
class SP_PUBLIC BitmapTemplate : public Interface::AllocBaseType {
public:
	BitmapTemplate();

	// init with jpeg, png or another formatted data
	BitmapTemplate(BytesView, const StrideFn &strideFn = nullptr);

	BitmapTemplate(const uint8_t *, size_t, const StrideFn &strideFn = nullptr);

	// init with raw data
	BitmapTemplate(const uint8_t *d, uint32_t width, uint32_t height,
			PixelFormat c = PixelFormat::RGBA8888, AlphaFormat a = AlphaFormat::Premultiplied, uint32_t stride = 0);

	BitmapTemplate(BytesView d, uint32_t width, uint32_t height,
			PixelFormat c = PixelFormat::RGBA8888, AlphaFormat a = AlphaFormat::Premultiplied, uint32_t stride = 0);

	BitmapTemplate(typename Interface::BytesType &&d, uint32_t width, uint32_t height,
			PixelFormat c = PixelFormat::RGBA8888, AlphaFormat a = AlphaFormat::Premultiplied, uint32_t stride = 0);

	BitmapTemplate(const BitmapTemplate &) = delete;
	BitmapTemplate &operator =(const BitmapTemplate &) = delete;

	BitmapTemplate(BitmapTemplate &&);
	BitmapTemplate &operator =(BitmapTemplate &&);

	uint32_t width() const { return _width; }
	uint32_t height() const { return _height; }
	uint32_t stride() const { return _stride; }

	bool hasStrideOffset() const { return _width * getBytesPerPixel(_color) < _stride; }

	AlphaFormat alpha() const { return _alpha; }
	PixelFormat format() const { return _color; }

	BytesView data() const { return _data; }

	uint8_t *dataPtr() { return _data.data(); }
	const uint8_t *dataPtr() const { return _data.data(); }

	bool updateStride(const StrideFn &strideFn);
	bool convert(PixelFormat, const StrideFn &strideFn = nullptr);
	bool truncate(PixelFormat, const StrideFn &strideFn = nullptr);

	// target should be large enough
	bool convertWithTarget(uint8_t *target, PixelFormat, const StrideFn &strideFn = nullptr) const;

	// init with jpeg or png data
	bool loadData(const uint8_t * data, size_t dataLen, const StrideFn &strideFn = nullptr);
	bool loadData(BytesView, const StrideFn &strideFn = nullptr);

	// init with raw data
	void loadBitmap(const uint8_t *d, uint32_t w, uint32_t h,
			PixelFormat = PixelFormat::RGBA8888, AlphaFormat a = AlphaFormat::Unpremultiplied, uint32_t stride = 0);
	void loadBitmap(BytesView d, uint32_t w, uint32_t h,
			PixelFormat = PixelFormat::RGBA8888, AlphaFormat a = AlphaFormat::Unpremultiplied, uint32_t stride = 0);
	void loadBitmap(typename Interface::BytesType &&d, uint32_t w, uint32_t h,
			PixelFormat = PixelFormat::RGBA8888, AlphaFormat a = AlphaFormat::Unpremultiplied, uint32_t stride = 0);

	void alloc(uint32_t w, uint32_t h,
			PixelFormat = PixelFormat::RGBA8888, AlphaFormat a = AlphaFormat::Unpremultiplied, uint32_t stride = 0);

	explicit operator bool () const { return _data.size() != 0; }

	void clear() { _data.clear(); }
	bool empty() const { return _data.empty(); }
	size_t size() const { return _data.size(); }

	FileFormat getOriginalFormat() const { return _originalFormat; }
	StringView getOriginalFormatName() const { return _originalFormatName; }

	bool save(StringView, bool invert = false);
	bool save(FileFormat, StringView, bool invert = false);
	bool save(StringView name, StringView path, bool invert = false);

	auto write(FileFormat = FileFormat::Png, bool invert = false) -> typename Interface::BytesType;
	auto write(StringView name, bool invert = false) -> typename Interface::BytesType;

	// resample with default filter (usually Lanczos4)
	BitmapTemplate resample(uint32_t width, uint32_t height, uint32_t stride = 0) const;

	BitmapTemplate resample(ResampleFilter, uint32_t width, uint32_t height, uint32_t stride = 0) const;

protected:
	void setInfo(uint32_t w, uint32_t h, PixelFormat c, AlphaFormat a = AlphaFormat::Unpremultiplied, uint32_t stride = 0);

	PixelFormat _color = PixelFormat::RGBA8888;
	AlphaFormat _alpha = AlphaFormat::Opaque;

	uint32_t _width = 0;
	uint32_t _height = 0;
	uint32_t _stride = 0; // in bytes
	typename Interface::BytesType _data;

	FileFormat _originalFormat = FileFormat::Custom;
	StringView _originalFormatName;
};


template <typename Interface>
BitmapTemplate<Interface>::BitmapTemplate() { }

template <typename Interface>
BitmapTemplate<Interface>::BitmapTemplate(BytesView data, const StrideFn &strideFn)
: BitmapTemplate(data.data(), data.size(), strideFn) { }


template <typename Interface>
BitmapTemplate<Interface>::BitmapTemplate(const uint8_t *data, size_t size, const StrideFn &strideFn) {
	if (!loadData(data, size, strideFn)) {
		_data.clear();
	}
}

template <typename Interface>
BitmapTemplate<Interface>::BitmapTemplate(const uint8_t *d, uint32_t width, uint32_t height,
		PixelFormat c, AlphaFormat a, uint32_t stride)
: _color(c), _alpha(a), _width(width), _height(height)
, _stride(max(stride, width * getBytesPerPixel(c))), _data(d, d + _stride * height) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
}

template <typename Interface>
BitmapTemplate<Interface>::BitmapTemplate(BytesView d, uint32_t width, uint32_t height,
		PixelFormat c, AlphaFormat a, uint32_t stride)
: _color(c), _alpha(a), _width(width), _height(height)
, _stride(max(stride, width * getBytesPerPixel(c))), _data(d.bytes<Interface>()) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
}

template <typename Interface>
BitmapTemplate<Interface>::BitmapTemplate(typename Interface::BytesType &&d, uint32_t width, uint32_t height,
		PixelFormat c, AlphaFormat a, uint32_t stride)
: _color(c), _alpha(a), _width(width), _height(height)
, _stride(max(stride, width * getBytesPerPixel(c))), _data(std::move(d)) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
}

template <typename Interface>
BitmapTemplate<Interface>::BitmapTemplate(BitmapTemplate &&other)
: _color(other._color), _alpha(other._alpha)
, _width(other._width), _height(other._height), _stride(other._stride)
, _data(std::move(other._data))
, _originalFormat(other._originalFormat)
, _originalFormatName(move(other._originalFormatName)) {
	other._data.clear();
}

template <typename Interface>
auto BitmapTemplate<Interface>::operator =(BitmapTemplate &&other) -> BitmapTemplate & {
	_alpha = other._alpha;
	_color = other._color;
	_width = other._width;
	_height = other._height;
	_stride = other._stride;
	_data = std::move(other._data);
	_originalFormat = other._originalFormat;
	_originalFormatName = move(other._originalFormatName);
	other._data.clear();
	return *this;
}

template <typename Interface>
void BitmapTemplate<Interface>::loadBitmap(const uint8_t *d, uint32_t w, uint32_t h, PixelFormat c, AlphaFormat a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	setInfo(w, h, c, a, stride);
	_data.clear();
	_data.resize(_stride * h);
	memcpy(_data.data(), d, _data.size());
	_originalFormat = FileFormat::Custom;
	_originalFormatName.clear();
}

template <typename Interface>
void BitmapTemplate<Interface>::loadBitmap(BytesView d, uint32_t w, uint32_t h, PixelFormat c, AlphaFormat a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	setInfo(w, h, c, a, stride);
	_data = d.bytes<Interface>();
	_originalFormat = FileFormat::Custom;
	_originalFormatName.clear();
}

template <typename Interface>
void BitmapTemplate<Interface>::loadBitmap(typename Interface::BytesType &&d, uint32_t w, uint32_t h, PixelFormat c, AlphaFormat a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	setInfo(w, h, c, a, stride);
	_data = std::move(d);
	_originalFormat = FileFormat::Custom;
	_originalFormatName.clear();
}

template <typename Interface>
void BitmapTemplate<Interface>::alloc(uint32_t w, uint32_t h, PixelFormat c, AlphaFormat a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	setInfo(w, h, c, a, stride);
	_data.clear();
	_data.resize(_stride * h);
	_originalFormat = FileFormat::Custom;
	_originalFormatName.clear();
}

template <typename Interface>
void BitmapTemplate<Interface>::setInfo(uint32_t w, uint32_t h, PixelFormat c, AlphaFormat a, uint32_t stride) {
	SPASSERT(c != PixelFormat::Auto, "Bitmap: Format::Auto should not be used with Bitmap directly");
	_width = w;
	_height = h;
	_stride = max(stride, w * getBytesPerPixel(c));
	_color = c;
	_alpha = a;
}

template <typename Interface>
bool BitmapTemplate<Interface>::updateStride(const StrideFn &strideFn) {
	uint32_t outStride = (strideFn != nullptr) ? max(strideFn(_color, _width), _width * getBytesPerPixel(_color)):_width * getBytesPerPixel(_color);
	if (outStride != _stride) {
		typename Interface::BytesType out;
		out.resize(_height * outStride);
		size_t minStride = _width * getBytesPerPixel(_color);
		for (size_t j = 0; j < _height; j ++) {
			memcpy(out.data() + j * outStride, _data.data() + j * _stride, minStride);
		}
		_data = std::move(out);
		_stride = outStride;
		return true;
	}
	return true;
}

template <typename Interface>
bool BitmapTemplate<Interface>::convert(PixelFormat color, const StrideFn &strideFn) {
	if (color == PixelFormat::Auto) {
		color = _color;
	}
	if (_color == color) {
		return updateStride(strideFn);
	}

	bool ret = false;
	typename Interface::BytesType out;
	uint32_t outStride = (strideFn != nullptr) ? max(strideFn(color, _width), _width * getBytesPerPixel(color)) : _width * getBytesPerPixel(color);
	out.resize(_height * outStride);

	ret = convertWithTarget(out.data(), color, strideFn);

	if (ret) {
		_color = color;
		_data = std::move(out);
		_stride = outStride;
	}

	return ret;
}

template <typename Interface>
bool BitmapTemplate<Interface>::truncate(PixelFormat color, const StrideFn &strideFn) {
	if (color == PixelFormat::Auto) {
		color = _color;
	}
	if (_color == color) {
		return updateStride(strideFn);
	}

	if (getBytesPerPixel(color) == getBytesPerPixel(_color)) {
		_color = color;
		return true;
	}

	auto height = _height;
	auto data = _data.data();
	auto bppIn = getBytesPerPixel(_color);

	typename Interface::BytesType out;
	uint32_t outStride = (strideFn != nullptr) ? max(strideFn(color, _width), _width * getBytesPerPixel(color)) : _width * getBytesPerPixel(color);
	out.resize(height * outStride);
	auto bppOut = getBytesPerPixel(color);

	auto fillBytes = min(bppIn, bppOut);
	auto clearBytes = bppOut - fillBytes;

	for (size_t j = 0; j < height; j ++) {
		auto inData = data + _stride * j;
		auto outData = out.data() + outStride * j;
	    for (size_t i = 0; i < _stride; i += bppIn) {
	    	for (uint8_t k = 0; k < fillBytes; ++ k) {
		        *outData++ = inData[i * bppIn + k];
	    	}
	    	for (uint8_t k = 0; k < clearBytes; ++ k) {
		        *outData++ = 0;
	    	}
	    }
	}

	_color = color;
	_data = std::move(out);
	_stride = outStride;

	return true;
}

template <typename Interface>
bool BitmapTemplate<Interface>::convertWithTarget(uint8_t *target, PixelFormat color, const StrideFn &strideFn) const {
	bool ret = false;
	uint32_t outStride = (strideFn != nullptr) ? max(strideFn(color, _width), _width * getBytesPerPixel(color)) : _width * getBytesPerPixel(color);
	BytesView out(target, _height * outStride);

	switch (_color) {
	case PixelFormat::A8:
		switch (color) {
		case PixelFormat::A8: return true; break;
		case PixelFormat::I8: return true; break;
		case PixelFormat::IA88: ret = convertData<PixelFormat::A8, PixelFormat::IA88>(_data, out, _stride, outStride); break;
		case PixelFormat::RGB888: ret = convertData<PixelFormat::A8, PixelFormat::RGB888>(_data, out, _stride, outStride); break;
		case PixelFormat::RGBA8888: ret = convertData<PixelFormat::A8, PixelFormat::RGBA8888>(_data, out, _stride, outStride); break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::I8:
		switch (color) {
		case PixelFormat::A8: return true; break;
		case PixelFormat::I8: return true; break;
		case PixelFormat::IA88: ret = convertData<PixelFormat::I8, PixelFormat::IA88>(_data, out, _stride, outStride); break;
		case PixelFormat::RGB888: ret = convertData<PixelFormat::I8, PixelFormat::RGB888>(_data, out, _stride, outStride); break;
		case PixelFormat::RGBA8888: ret = convertData<PixelFormat::I8, PixelFormat::RGBA8888>(_data, out, _stride, outStride); break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::IA88:
		switch (color) {
		case PixelFormat::A8: ret = convertData<PixelFormat::IA88, PixelFormat::A8>(_data, out, _stride, outStride); break;
		case PixelFormat::I8: ret = convertData<PixelFormat::IA88, PixelFormat::I8>(_data, out, _stride, outStride); break;
		case PixelFormat::IA88: return true; break;
		case PixelFormat::RGB888: ret = convertData<PixelFormat::IA88, PixelFormat::RGB888>(_data, out, _stride, outStride); break;
		case PixelFormat::RGBA8888: ret = convertData<PixelFormat::IA88, PixelFormat::RGBA8888>(_data, out, _stride, outStride); break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::RGB888:
		switch (color) {
		case PixelFormat::A8: ret = convertData<PixelFormat::RGB888, PixelFormat::A8>(_data, out, _stride, outStride); break;
		case PixelFormat::I8: ret = convertData<PixelFormat::RGB888, PixelFormat::I8>(_data, out, _stride, outStride); break;
		case PixelFormat::IA88: ret = convertData<PixelFormat::RGB888, PixelFormat::IA88>(_data, out, _stride, outStride); break;
		case PixelFormat::RGB888: return true; break;
		case PixelFormat::RGBA8888: ret = convertData<PixelFormat::RGB888, PixelFormat::RGBA8888>(_data, out, _stride, outStride); break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::RGBA8888:
		switch (color) {
		case PixelFormat::A8: ret = convertData<PixelFormat::RGBA8888, PixelFormat::A8>(_data, out, _stride, outStride); break;
		case PixelFormat::I8: ret = convertData<PixelFormat::RGBA8888, PixelFormat::I8>(_data, out, _stride, outStride); break;
		case PixelFormat::IA88: ret = convertData<PixelFormat::RGBA8888, PixelFormat::IA88>(_data, out, _stride, outStride); break;
		case PixelFormat::RGB888: ret = convertData<PixelFormat::RGBA8888, PixelFormat::RGB888>(_data, out, _stride, outStride); break;
		case PixelFormat::RGBA8888: return true; break;
		case PixelFormat::Auto: return false; break;
		}
		break;
	case PixelFormat::Auto: return false; break;
	}

	return ret;
}

template <typename Interface>
bool BitmapTemplate<Interface>::save(StringView path, bool invert) {
	FileFormat fmt = FileFormat::Png;
	auto ext = filepath::lastExtension(path);
	if (ext == "png") {
		fmt = FileFormat::Png;
	} else if (ext == "jpeg" || ext == "jpg") {
		fmt = FileFormat::Jpeg;
	} else if (ext == "webp") {
		fmt = FileFormat::WebpLossless;
	}
	return save(fmt, path, invert);
}

template <typename Interface>
bool BitmapTemplate<Interface>::loadData(BytesView d, const StrideFn &strideFn) {
	return loadData(d.data(), d.size(), strideFn);
}

}

namespace STAPPLER_VERSIONIZED stappler::mem_std {

using Bitmap = bitmap::BitmapTemplate<memory::StandartInterface>;

}

namespace STAPPLER_VERSIONIZED stappler::mem_pool {

using Bitmap = bitmap::BitmapTemplate<memory::PoolInterface>;

}

#endif /* STAPPLER_BITMAP_SPBITMAP_H_ */
