/**
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2014 Chukong Technologies
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_GEOM_SPGEOMETRY_H_
#define STAPPLER_GEOM_SPGEOMETRY_H_

#include "SPVec2.h"
#include "SPVec3.h"

namespace stappler::geom {

struct Size2 {
	static const Size2 ZERO;

	float width = 0.0f;
	float height = 0.0f;

	constexpr Size2() = default;
	constexpr Size2(float w, float h) : width(w), height(h) { }
	constexpr Size2(const Size2 &other) = default;
	constexpr explicit Size2(const Vec2 &point) : width(point.x), height(point.y) { }

	constexpr operator Vec2() const { return Vec2(width, height); }

	constexpr bool operator==(const Size2 &s) const { return equals(s); }
	constexpr bool operator!=(const Size2 &s) const { return !equals(s); }

	constexpr Size2& operator= (const Size2 &other) = default;
	constexpr Size2& operator= (const Vec2 &point) {
		this->width = point.x; this->height = point.y;
		return *this;
	}

	constexpr Size2 operator+(const Size2 &right) const {
		return Size2(this->width + right.width, this->height + right.height);
	}
	constexpr Size2 operator-(const Size2 &right) const {
		return Size2(this->width - right.width, this->height - right.height);
	}
	constexpr Size2 operator*(float a) const {
		return Size2(this->width * a, this->height * a);
	}
	constexpr Size2 operator/(float a) const {
		return Size2(this->width / a, this->height / a);
	}

	constexpr void setSize(float w, float h) {  }

	constexpr bool equals(const Size2 &target) const {
		return (std::fabs(this->width - target.width) < NumericLimits<float>::epsilon())
				&& (std::fabs(this->height - target.height) < NumericLimits<float>::epsilon());
	}
};

constexpr Size2 Size2::ZERO(0.0f, 0.0f);


struct Size3 {
	static const Size3 ZERO;

	float width = 0.0f;
	float height = 0.0f;
	float depth = 0.0f;

	constexpr Size3() = default;
	constexpr Size3(float w, float h, float d) : width(w), height(h), depth(d) { }
	constexpr Size3(const Size3& other) = default;
	constexpr explicit Size3(const Vec3& point) : width(point.x), height(point.y), depth(point.z) { }

	constexpr operator Vec3() const { return Vec3(width, height, depth); }

	constexpr bool operator==(const Size3 &s) const { return equals(s); }
	constexpr bool operator!=(const Size3 &s) const { return !equals(s); }

	constexpr Size3& operator= (const Size3& other) = default;
	constexpr Size3& operator= (const Vec3& point) {
		width = point.x;
		height = point.y;
		depth = point.z;
		return *this;
	}

	constexpr Size3 operator+(const Size3& right) const {
		Size3 ret(*this);
		ret.width += right.width;
		ret.height += right.height;
		ret.depth += right.depth;
		return ret;
	}
	constexpr Size3 operator-(const Size3& right) const {
		Size3 ret(*this);
		ret.width -= right.width;
		ret.height -= right.height;
		ret.depth -= right.depth;
		return ret;
	}
	constexpr Size3 operator*(float a) const {
		Size3 ret(*this);
		ret.width *= a;
		ret.height *= a;
		ret.depth *= a;
		return ret;
	}
	constexpr Size3 operator/(float a) const {
		Size3 ret(*this);
		ret.width /= a;
		ret.height /= a;
		ret.depth /= a;
		return ret;
	}

	constexpr bool equals(const Size3& target) const {
		return (std::fabs(this->width - target.width) < NumericLimits<float>::epsilon())
				&& (std::fabs(this->height - target.height) < NumericLimits<float>::epsilon())
				&& (std::fabs(this->depth - target.depth) < NumericLimits<float>::epsilon());
	}
};

constexpr Size3 Size3::ZERO = Size3(0.0f, 0.0f, 0.0f);


struct Extent2 {
	static const Extent2 ZERO;

	uint32_t width = 0;
	uint32_t height = 0;

	constexpr Extent2() = default;
	constexpr Extent2(uint32_t w, uint32_t h) : width(w), height(h) { }

	constexpr Extent2(const Extent2 &other) = default;
	constexpr Extent2& operator= (const Extent2 &other) = default;

	constexpr explicit Extent2(const Size2 &size) : width(size.width), height(size.height) { }
	constexpr explicit Extent2(const Vec2 &point) : width(point.x), height(point.y) { }

	constexpr Extent2& operator= (const Size2 &size) { width = size.width; height = size.width; return *this; }
	constexpr Extent2& operator= (const Vec2 &other) { width = other.x; height = other.y; return *this; }

	constexpr bool operator==(const Extent2 &other) const = default;
	constexpr bool operator!=(const Extent2 &other) const = default;
	constexpr auto operator<=>(const Extent2 &other) const = default;

	constexpr operator Size2 () const { return Size2(width, height); }
};

constexpr Extent2 Extent2::ZERO = Extent2(0, 0);


struct Extent3 {
	static const Extent3 ZERO;

	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t depth = 0;

	constexpr Extent3() = default;
	constexpr Extent3(uint32_t w, uint32_t h, uint32_t d) : width(w), height(h), depth(d) { }
	constexpr Extent3(Extent2 e, uint32_t d) : width(e.width), height(e.height), depth(d) { }

	constexpr Extent3(const Extent3& other) = default;
	constexpr Extent3& operator= (const Extent3& other) = default;

	constexpr Extent3(const Extent2& other) : width(other.width), height(other.height), depth(1) { }
	constexpr Extent3& operator= (const Extent2& other) { width = other.width; height = other.height; depth = 1; return *this; }

	constexpr explicit Extent3(const Size3 &size) : width(size.width), height(size.height), depth(size.depth) { }
	constexpr explicit Extent3(const Vec3 &point) : width(point.x), height(point.y), depth(point.z) { }

	constexpr Extent3& operator= (const Size3 &size) { width = size.width; height = size.width; depth = size.depth; return *this; }
	constexpr Extent3& operator= (const Vec3 &other) { width = other.x; height = other.y; depth = other.z; return *this; }

	constexpr bool operator==(const Extent3 &other) const = default;
	constexpr bool operator!=(const Extent3 &other) const = default;
	constexpr auto operator<=>(const Extent3 &other) const = default;

	constexpr operator Size3 () const { return Size3(width, height, depth); }
};

constexpr Extent3 Extent3::ZERO = Extent3(0, 0, 0);


struct Rect {
	static const Rect ZERO;

	Vec2 origin;
	Size2 size;

	constexpr Rect() = default;
	constexpr Rect(float x, float y, float width, float height) : origin(x, y), size(width, height) { }
	constexpr Rect(const Vec2 &origin, const Size2 &size) : origin(origin), size(size) { }
	constexpr Rect(const Rect& other) = default;

	constexpr Rect& operator= (const Rect& other) = default;

	constexpr float getMaxX() const { return origin.x + size.width; }
	constexpr float getMidX() const { return origin.x + size.width / 2.0f; }
	constexpr float getMinX() const { return origin.x; }
	constexpr float getMaxY() const { return origin.y + size.height; }
	constexpr float getMidY() const { return origin.y + size.height / 2.0f; }
	constexpr float getMinY() const { return origin.y; }

	constexpr bool equals(const Rect& rect) const {
		return (origin.equals(rect.origin) && size.equals(rect.size));
	}

	bool containsPoint(const Vec2& point) const;
	bool intersectsRect(const Rect& rect) const;
	bool intersectsCircle(const Vec2& center, float radius) const;

	/** Get the min rect which can contain this and rect. */
	Rect unionWithRect(const Rect & rect) const;

	/** Compute the min rect which can contain this and rect, assign it to this. */
	void merge(const Rect& rect);
};

constexpr Rect Rect::ZERO = Rect(0.0f, 0.0f, 0.0f, 0.0f);

struct UVec2 {
	uint32_t x;
	uint32_t y;

	constexpr bool operator==(const UVec2 &) const = default;
	constexpr bool operator!=(const UVec2 &) const = default;
};

struct URect {
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t width = 0;
	uint32_t height = 0;

	constexpr UVec2 origin() const { return UVec2{x, y}; }

	constexpr bool operator==(const URect &) const = default;
	constexpr bool operator!=(const URect &) const = default;

	constexpr float getMaxX() const { return x + width; }
	constexpr float getMidX() const { return x + width / 2.0f; }
	constexpr float getMinX() const { return x; }
	constexpr float getMaxY() const { return y + height; }
	constexpr float getMidY() const { return y + height / 2.0f; }
	constexpr float getMinY() const { return y; }

	bool containsPoint(const UVec2 &point) const;
	bool intersectsRect(const URect &rect) const;
};

Rect TransformRect(const Rect& rect, const Mat4& transform);

inline std::ostream & operator<<(std::ostream & stream, const Rect & obj) {
	stream << "Rect(x:" << obj.origin.x << " y:" << obj.origin.y
			<< " width:" << obj.size.width << " height:" << obj.size.height << ");";
	return stream;
}

inline std::ostream & operator<<(std::ostream & stream, const Size2 & obj) {
	stream << "Size2(width:" << obj.width << " height:" << obj.height << ");";
	return stream;
}

inline std::ostream & operator<<(std::ostream & stream, const Size3 & obj) {
	stream << "Size3(width:" << obj.width << " height:" << obj.height << " depth:" << obj.depth << ");";
	return stream;
}

inline std::ostream & operator<<(std::ostream & stream, const Extent2 & obj) {
	stream << "Extent2(width:" << obj.width << " height:" << obj.height << ");";
	return stream;
}

inline std::ostream & operator<<(std::ostream & stream, const Extent3 & obj) {
	stream << "Extent3(width:" << obj.width << " height:" << obj.height << " depth:" << obj.depth << ");";
	return stream;
}

}

#endif /* STAPPLER_GEOM_SPGEOMETRY_H_ */
