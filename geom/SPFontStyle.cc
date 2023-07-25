/**
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

#include "SPFontStyle.h"
#include "SPMemory.h"

namespace stappler::geom {

constexpr uint32_t LAYOUT_PADDING  = 1;

bool Metric::readStyleValue(StringView r, bool resolutionMetric, bool allowEmptyMetric) {
	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();
	if (!resolutionMetric && r.starts_with("auto")) {
		r += 4;
		this->metric = Metric::Units::Auto;
		this->value = 0.0f;
		return true;
	}

	auto fRes = r.readFloat();
	if (!fRes.valid()) {
		return false;
	}

	auto fvalue = fRes.get();
	if (fvalue == 0.0f) {
		this->value = fvalue;
		this->metric = Metric::Units::Px;
		return true;
	}

	r.skipChars<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	auto str = r.readUntil<StringView::CharGroup<CharGroupId::WhiteSpace>>();

	if (!resolutionMetric) {
		if (str.is('%')) {
			++ str;
			this->value = fvalue / 100.0f;
			this->metric = Metric::Units::Percent;
			return true;
		} else if (str == "em") {
			str += 2;
			this->value = fvalue;
			this->metric = Metric::Units::Em;
			return true;
		} else if (str == "rem") {
			str += 3;
			this->value = fvalue;
			this->metric = Metric::Units::Rem;
			return true;
		} else if (str == "px") {
			str += 2;
			this->value = fvalue;
			this->metric = Metric::Units::Px;
			return true;
		} else if (str == "pt") {
			str += 2;
			this->value = fvalue * 4.0f / 3.0f;
			this->metric = Metric::Units::Px;
			return true;
		} else if (str == "pc") {
			str += 2;
			this->value = fvalue * 15.0f;
			this->metric = Metric::Units::Px;
			return true;
		} else if (str == "mm") {
			str += 2;
			this->value = fvalue * 3.543307f;
			this->metric = Metric::Units::Px;
			return true;
		} else if (str == "cm") {
			str += 2;
			this->value = fvalue * 35.43307f;
			this->metric = Metric::Units::Px;
			return true;
		} else if (str == "in") {
			str += 2;
			this->value = fvalue * 90.0f;
			this->metric = Metric::Units::Px;
			return true;
		} else if (str == "vw") {
			str += 2;
			this->value = fvalue;
			this->metric = Metric::Units::Vw;
			return true;
		} else if (str == "vh") {
			str += 2;
			this->value = fvalue;
			this->metric = Metric::Units::Vh;
			return true;
		} else if (str == "vmin") {
			str += 4;
			this->value = fvalue;
			this->metric = Metric::Units::VMin;
			return true;
		} else if (str == "vmax") {
			str += 4;
			this->value = fvalue;
			this->metric = Metric::Units::VMax;
			return true;
		}
	} else {
		if (str == "dpi") {
			str += 3;
			this->value = fvalue;
			this->metric = Metric::Units::Dpi;
			return true;
		} else if (str == "dpcm") {
			str += 4;
			this->value = fvalue / 2.54f;
			this->metric = Metric::Units::Dpi;
			return true;
		} else if (str == "dppx") {
			str += 4;
			this->value = fvalue;
			this->metric = Metric::Units::Dppx;
			return true;
		}
	}

	if (allowEmptyMetric) {
		this->value = fvalue;
		return true;
	}

	return false;
}

static void s_getSpecializationArgs(std::ostream &out, const FontSpecializationVector &vec) {
	out << "?size=" << vec.fontSize.get();
	out << "&weight=" << vec.fontWeight.get();
	out << "&width=" << vec.fontStretch.get();
	switch (vec.fontStyle.get()) {
	case FontStyle::Normal.get(): out << "&style=normal"; break;
	case FontStyle::Italic.get(): out << "&style=italic"; break;
	default: out << "&style=" << vec.fontStyle.get(); break;
	}
	out << "&density=" << vec.density;
	if (vec.fontGrade != FontGrade::Normal) {
		out << "&grade=" << vec.fontGrade.get();
	}
}

template <>
auto FontSpecializationVector::getSpecializationArgs<memory::PoolInterface>() const -> memory::PoolInterface::StringType {
	memory::PoolInterface::StringStreamType out;
	s_getSpecializationArgs(out, *this);
	return out.str();
}

template <>
auto FontSpecializationVector::getSpecializationArgs<memory::StandartInterface>() const -> memory::StandartInterface::StringType {
	memory::StandartInterface::StringStreamType out;
	s_getSpecializationArgs(out, *this);
	return out.str();
}

FontParameters FontParameters::create(StringView str, memory::pool_t *pool) {
	FontParameters ret;

	enum State {
		Family,
		Size,
		Style,
		Weight,
		Stretch,
		Overflow,
	} state = Family;

	str.split<StringView::Chars<'.'>>([&] (const StringView &ir) {
		StringView r(ir);
		switch (state) {
		case Family:
			ret.fontFamily = r.pdup(pool);
			state = Size;
			break;
		case Size:
			if (r.is("xxs")) { ret.fontSize = FontSize::XXSmall; }
			else if (r.is("xs")) { ret.fontSize = FontSize::XSmall; }
			else if (r.is("s")) { ret.fontSize = FontSize::Small; }
			else if (r.is("m")) { ret.fontSize = FontSize::Medium; }
			else if (r.is("l")) {ret.fontSize = FontSize::Large; }
			else if (r.is("xl")) { ret.fontSize = FontSize::XLarge; }
			else if (r.is("xxl")) { ret.fontSize = FontSize::XXLarge; }
			else { r.readInteger().unwrap([&] (int64_t value) { ret.fontSize = FontSize(value); }); }
			state = Style;
			break;
		case Style:
			if (r.is("i")) { ret.fontStyle = FontStyle::Italic; }
			else if (r.is("o")) { ret.fontStyle = FontStyle::Oblique; }
			else if (r.is("n")) { ret.fontStyle = FontStyle::Normal; }
			state = Weight;
			break;
		case Weight:
			ret.fontWeight = FontWeight(r.readInteger(10).get(400));
			state = Stretch;
			break;
		case Stretch:
			ret.fontStretch = FontStretch(r.readInteger(10).get(100 << 1));
			state = Overflow;
			break;
		default: break;
		}
	});
	return ret;
}

template <>
auto FontParameters::getFontConfigName<memory::PoolInterface>(StringView fontFamily, FontSize fontSize, FontStyle fontStyle, FontWeight fontWeight,
		FontStretch fontStretch, FontGrade fontGrade, FontVariant fontVariant, bool caps) -> memory::PoolInterface::StringType {
	auto size = fontSize;
	memory::PoolInterface::StringType name;
	name.reserve(fontFamily.size() + 14);
	name += fontFamily.str<memory::PoolInterface>();

	if (caps && fontVariant == FontVariant::SmallCaps) {
		size -= size / 5.0f;
	}

	name += "." + mem_pool::toString(size.get());

	switch (fontStyle.get()) {
	case FontStyle::Normal.get(): name += ".n"; break;
	case FontStyle::Italic.get(): name += ".i"; break;
	default: name += "."; name += mem_pool::toString(fontStyle.get()); break;
	}

	name += mem_pool::toString(".", fontWeight.get());
	name += mem_pool::toString(".", fontStretch.get());
	name += mem_pool::toString(".", fontGrade.get());
	return name;
}

template <>
auto FontParameters::getFontConfigName<memory::StandartInterface>(StringView fontFamily, FontSize fontSize, FontStyle fontStyle, FontWeight fontWeight,
		FontStretch fontStretch, FontGrade fontGrade, FontVariant fontVariant, bool caps) -> memory::StandartInterface::StringType {
	auto size = fontSize;
	memory::StandartInterface::StringType name;
	name.reserve(fontFamily.size() + 14);
	name += fontFamily.str<memory::StandartInterface>();

	if (caps && fontVariant == FontVariant::SmallCaps) {
		size -= size / 5.0f;
	}

	name += "." + mem_std::toString(size.get());

	switch (fontStyle.get()) {
	case FontStyle::Normal.get(): name += ".n"; break;
	case FontStyle::Italic.get(): name += ".i"; break;
	default: name += "."; name += mem_std::toString(fontStyle.get()); break;
	}

	name += mem_std::toString(".", fontWeight.get());
	name += mem_std::toString(".", fontStretch.get());
	name += mem_std::toString(".", fontGrade.get());
	return name;
}

FontParameters FontParameters::getSmallCaps() const {
	FontParameters ret = *this;
	ret.fontSize -= ret.fontSize / 5.0f;
	return ret;
}

struct LayoutNodeMemory;

struct LayoutNodeMemoryStorage {
	const EmplaceCharInterface *interface = nullptr;
	LayoutNodeMemory *free = nullptr;
	memory::pool_t *pool = nullptr;

	LayoutNodeMemoryStorage(const EmplaceCharInterface *, memory::pool_t *);

	LayoutNodeMemory *alloc(const URect &rect);
	LayoutNodeMemory *alloc(const UVec2 &origin, void *c);
	void release(LayoutNodeMemory *node);
};

struct LayoutNodeMemory : memory::AllocPool {
	LayoutNodeMemory* _child[2];
	URect _rc;
	void * _char = nullptr;

	LayoutNodeMemory(const URect &rect);
	LayoutNodeMemory(const LayoutNodeMemoryStorage &, const UVec2 &origin, void *c);
	~LayoutNodeMemory();

	bool insert(LayoutNodeMemoryStorage &, void *c);
	size_t nodes() const;
	void finalize(LayoutNodeMemoryStorage &, uint8_t tex);
};

LayoutNodeMemoryStorage::LayoutNodeMemoryStorage(const EmplaceCharInterface *i, memory::pool_t *p)
: interface(i), pool(p) { }

LayoutNodeMemory *LayoutNodeMemoryStorage::alloc(const URect &rect) {
	if (free) {
		auto node = free;
		free = node->_child[0];
		return new (node) LayoutNodeMemory(rect);
	}
	return new (pool) LayoutNodeMemory(rect);
}

LayoutNodeMemory *LayoutNodeMemoryStorage::alloc(const UVec2 &origin, void *c) {
	if (free) {
		auto node = free;
		free = node->_child[0];
		return new (node) LayoutNodeMemory(*this, origin, c);
	}
	return new (pool) LayoutNodeMemory(*this, origin, c);
}

void LayoutNodeMemoryStorage::release(LayoutNodeMemory *node) {
	node->_child[0] = nullptr;
	node->_child[1] = nullptr;
	node->_rc = URect({0, 0, 0, 0});
	node->_char = nullptr;
	if (free) {
		node->_child[0] = free;
	}
	free = node;
}

LayoutNodeMemory::LayoutNodeMemory(const URect &rect) {
	_rc = rect;
	_child[0] = nullptr;
	_child[1] = nullptr;
}

LayoutNodeMemory::LayoutNodeMemory(const LayoutNodeMemoryStorage &storage, const UVec2 &origin, void *c) {
	_child[0] = nullptr;
	_child[1] = nullptr;
	_char = c;
	_rc = URect{origin.x, origin.y, storage.interface->getWidth(c), storage.interface->getHeight(c)};
}

LayoutNodeMemory::~LayoutNodeMemory() { }

bool LayoutNodeMemory::insert(LayoutNodeMemoryStorage &storage, void *c) {
	if (_child[0] && _child[1]) {
		return _child[0]->insert(storage, c) || _child[1]->insert(storage, c);
	} else {
		if (_char) {
			return false;
		}

		const auto iwidth = storage.interface->getWidth(c);
		const auto iheight = storage.interface->getHeight(c);

		if (_rc.width < iwidth || _rc.height < iheight) {
			return false;
		}

		if (_rc.width == iwidth || _rc.height == iheight) {
			if (_rc.height == iheight) {
				_child[0] = storage.alloc(_rc.origin(), c); // new (pool) LayoutNode(_rc.origin(), c);
				_child[1] = storage.alloc(URect{uint32_t(_rc.x + iwidth + LAYOUT_PADDING), _rc.y,
					(_rc.width > iwidth + LAYOUT_PADDING)?uint32_t(_rc.width - iwidth - LAYOUT_PADDING):uint32_t(0), _rc.height});
			} else if (_rc.width == iwidth) {
				_child[0] = storage.alloc(_rc.origin(), c); // new (pool) LayoutNode(_rc.origin(), c);
				_child[1] = storage.alloc(URect{_rc.x, uint32_t(_rc.y + iheight + LAYOUT_PADDING),
					_rc.width, (_rc.height > iheight + LAYOUT_PADDING)?uint32_t(_rc.height - iheight - LAYOUT_PADDING):uint32_t(0)});
			}
			return true;
		}

		//(decide which way to split)
		int16_t dw = _rc.width - iwidth;
		int16_t dh = _rc.height - iheight;

		if (dw > dh) {
			_child[0] = storage.alloc(URect{_rc.x, _rc.y, iwidth, _rc.height});
			_child[1] = storage.alloc(URect{uint32_t(_rc.x + iwidth + LAYOUT_PADDING), _rc.y, uint32_t(dw - LAYOUT_PADDING), _rc.height});
		} else {
			_child[0] = storage.alloc(URect{_rc.x, _rc.y, _rc.width, iheight});
			_child[1] = storage.alloc(URect{_rc.x, uint32_t(_rc.y + iheight + LAYOUT_PADDING), _rc.width, uint32_t(dh - LAYOUT_PADDING)});
		}

		_child[0]->insert(storage, c);

		return true;
	}
}

size_t LayoutNodeMemory::nodes() const {
	if (_char) {
		return 1;
	} else if (_child[0] && _child[1]) {
		return _child[0]->nodes() + _child[1]->nodes();
	} else {
		return 0;
	}
}

void LayoutNodeMemory::finalize(LayoutNodeMemoryStorage &storage, uint8_t tex) {
	if (_char) {
		storage.interface->setX(_char, _rc.x);
		storage.interface->setY(_char, _rc.y);
		storage.interface->setTex(_char, tex);
	} else {
		if (_child[0]) { _child[0]->finalize(storage, tex); }
		if (_child[1]) { _child[1]->finalize(storage, tex); }
	}
	 storage.release(this);
}

Extent2 emplaceChars(const EmplaceCharInterface &iface, const SpanView<void *> &layoutData, float totalSquare) {
	if (std::isnan(totalSquare)) {
		totalSquare = 0.0f;
		for (auto &it : layoutData) {
			totalSquare += iface.getWidth(it) * iface.getHeight(it);
		}
	}

	// find potential best match square
	bool s = true;
	uint32_t h = 128, w = 128; uint32_t sq2 = h * w;
	for (; sq2 < totalSquare; sq2 *= 2, s = !s) {
		if (s) { w *= 2; } else { h *= 2; }
	}

	LayoutNodeMemoryStorage storage(&iface, memory::pool::acquire());

	while (true) {
		auto l = storage.alloc(URect{0, 0, w, h});
		for (auto &it : layoutData) {
			if (!l->insert(storage, it)) {
				break;
			}
		}

		auto nodes = l->nodes();
		if (nodes == layoutData.size()) {
			l->finalize(storage, 0);
			storage.release(l);
			break;
		} else {
			if (s) { w *= 2; } else { h *= 2; }
			sq2 *= 2; s = !s;
		}
		storage.release(l);
	}

	return Extent2(w, h);
}

uint32_t CharLayout::getObjectId(uint16_t sourceId, char16_t ch, SpriteAnchor a) {
	uint32_t ret = ch;
	ret |= (toInt(a) << (sizeof(char16_t) * 8));
	ret |= (sourceId << ((sizeof(char16_t) * 8) + 2));
	return ret;
}

uint32_t CharLayout::getObjectId(uint32_t ret, SpriteAnchor a) {
	return (ret & (~ (3 << (sizeof(char16_t) * 8)))) | (toInt(a) << (sizeof(char16_t) * 8));
}

SpriteAnchor CharLayout::getAnchorForObject(uint32_t obj) {
	return SpriteAnchor((obj >> sizeof(char16_t) * 8) & 0b11);
}

}
