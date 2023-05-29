/**
Copyright 2013 BlackBerry Inc.
Copyright (c) 2014-2015 Chukong Technologies
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023 Stappler LLC <admin@stappler.dev>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

Original file from GamePlay3D: http://gameplay3d.org

This file was modified to fit the cocos2d-x project
This file was modified for stappler project
*/

#ifndef STAPPLER_GEOM_SPVEC2_H_
#define STAPPLER_GEOM_SPVEC2_H_

#include "SPCommon.h"

namespace stappler::geom {

class Mat4;
struct Size2;

class Vec2 {
public:
	/** Clamps the specified vector within the specified range and returns it in dst. */
	static void clamp(const Vec2& v, const Vec2& min, const Vec2& max, Vec2* dst);

	static float angle(const Vec2& v1, const Vec2& v2);
	static float dot(const Vec2& v1, const Vec2& v2);

	static constexpr Vec2 forAngle(const float a) {
		return Vec2(cosf(a), sinf(a));
	}

	/** A general line-line intersection test
	 @param A   the startpoint for the first line L1 = (A - B)
	 @param B   the endpoint for the first line L1 = (A - B)
	 @param C   the startpoint for the second line L2 = (C - D)
	 @param D   the endpoint for the second line L2 = (C - D)
	 @param S   the range for a hitpoint in L1 (p = A + S*(B - A))
	 @param T   the range for a hitpoint in L2 (p = C + T*(D - C))
	 @returns   whether these two lines interects. */
	static bool isLineIntersect(const Vec2 &A, const Vec2 &B, const Vec2 &C, const Vec2 &D,
			float *S = nullptr, float *T = nullptr);

	static bool isLineOverlap(const Vec2 &A, const Vec2 &B, const Vec2 &C, const Vec2 &D);

	static bool isLineParallel(const Vec2 &A, const Vec2 &B, const Vec2 &C, const Vec2 &D);

	static bool isSegmentOverlap(const Vec2 &A, const Vec2 &B, const Vec2 &C, const Vec2 &D,
			Vec2 *S = nullptr, Vec2 *E = nullptr);

	static bool isSegmentIntersect(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D);
	static Vec2 getIntersectPoint(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D);

	template <typename Callback>
	static bool getSegmentIntersectPoint(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D, const Callback &);

	static bool isCounterClockwise(const Vec2 &u, const Vec2 &v, const Vec2 &w);

	static const Vec2 ZERO;
	static const Vec2 ONE;
	static const Vec2 UNIT_X;
	static const Vec2 UNIT_Y;

	float x;
	float y;

	constexpr Vec2() : x(0.0f), y(0.0f) { }
	constexpr Vec2(float xx, float yy) : x(xx), y(yy) { }

	constexpr Vec2(const Vec2& p1, const Vec2& p2) :  x(p2.x - p1.x), y(p2.y - p1.y) { }
	constexpr Vec2(const Vec2& copy) = default;

	explicit Vec2(const Size2 &);

	constexpr bool isZero() const { return x == 0.0f && y == 0.0f; }
	constexpr bool isOne() const { return x == 1.0f && y == 1.0f; }
	constexpr bool isValid() const { return !std::isnan(x) && !std::isnan(y); }

	constexpr void add(const Vec2& v) { x += v.x; y += v.y; }
	constexpr void add(float v) { x += v; y += v; }

	constexpr float distanceSquared(const Vec2& v) const {
		const float dx = v.x - x;
		const float dy = v.y - y;
		return (dx * dx + dy * dy);
	}

	constexpr bool isWithinDistance(const Vec2& v, float val) const {
		return distanceSquared(v) < val * val;
	}

	constexpr float dot(const Vec2& v) const { return (x * v.x + y * v.y); }
	constexpr float lengthSquared() const { return (x * x + y * y); }
	constexpr void negate() { x = -x; y = -y; }
	constexpr void scale(float scalar) { x *= scalar; y *= scalar; }
	constexpr void scale(const Vec2& scale) { x *= scale.x; y *= scale.y; }
	constexpr void setZero() { x = y = 0.0f; }
	constexpr void subtract(const Vec2& v) { x -= v.x; y -= v.y; }
	constexpr void subtract(float v) { x -= v; y -= v; }

	/**
	 * Updates this vector towards the given target using a smoothing function.
	 * The given response time determines the amount of smoothing (lag). A longer
	 * response time yields a smoother result and more lag. To force this vector to
	 * follow the target closely, provide a response time that is very small relative
	 * to the given elapsed time. */
	constexpr void smooth(const Vec2& target, float elapsedTime, float responseTime) {
		if (elapsedTime > 0) {
			*this += (target - *this) * (elapsedTime / (elapsedTime + responseTime));
		}
	}

	constexpr const Vec2 operator+(const Vec2& v) const {
		Vec2 result(*this);
		result.add(v);
		return result;
	}

	constexpr const Vec2 operator+(float v) const {
		Vec2 result(*this);
		result.add(v);
		return result;
	}

	constexpr Vec2& operator+=(const Vec2& v) {
		add(v);
		return *this;
	}

	constexpr const Vec2 operator-(const Vec2& v) const {
		Vec2 result(*this);
		result.subtract(v);
		return result;
	}

	constexpr const Vec2 operator-(float v) const {
		Vec2 result(*this);
		result.subtract(v);
		return result;
	}

	constexpr Vec2& operator-=(const Vec2& v) {
		subtract(v);
		return *this;
	}

	constexpr const Vec2 operator-() const {
		Vec2 result(*this);
		result.negate();
		return result;
	}

	constexpr const Vec2 operator*(float s) const {
		Vec2 result(*this);
		result.scale(s);
		return result;
	}

	constexpr Vec2& operator*=(float s) {
		scale(s);
		return *this;
	}

	constexpr Vec2& operator/=(float s) {
		x /= s; y /= s;
		return *this;
	}

	constexpr bool operator<(const Vec2& v) const { if (x == v.x) { return y < v.y; } return x < v.x; }
	constexpr bool operator>(const Vec2& v) const { if (x == v.x) { return y > v.y; } return x > v.x; }
	constexpr bool operator==(const Vec2& v) const { return x == v.x && y == v.y; }
	constexpr bool operator!=(const Vec2& v) const { return x != v.x || y != v.y; }

	constexpr bool equals(const Vec2& target) const {
		return (std::abs(this->x - target.x) < NumericLimits<float>::epsilon())
				&& (std::abs(this->y - target.y) < NumericLimits<float>::epsilon());
	}

	constexpr bool fuzzyEquals(const Vec2& b, float var) const {
		return (x - var <= b.x && b.x <= x + var) && (y - var <= b.y && b.y <= y + var);
	}

	constexpr float getLength() const {
		return std::sqrt(x*x + y*y);
	}

	constexpr float getLengthSq() const {
		return dot(*this); //x*x + y*y;
	}

	constexpr float getDistanceSq(const Vec2& other) const {
		return (*this - other).getLengthSq();
	}

	constexpr float getDistance(const Vec2& other) const {
		return (*this - other).getLength();
	}

	constexpr float getAngle() const {
		return std::atan2(y, x);
	}

	constexpr float cross(const Vec2& other) const {
		return x*other.y - y*other.x;
	}

	/** Calculates perpendicular of v, rotated 90 degrees counter-clockwise -- cross(v, perp(v)) >= 0 */
	constexpr Vec2 getPerp() const {
		return Vec2(-y, x);
	}

	constexpr Vec2 getMidpoint(const Vec2& other) const {
		return Vec2((x + other.x) / 2.0f, (y + other.y) / 2.0f);
	}

	constexpr Vec2 getClampPoint(const Vec2& min_inclusive, const Vec2& max_inclusive) const {
		return Vec2(math::clamp(x,min_inclusive.x,max_inclusive.x), math::clamp(y, min_inclusive.y, max_inclusive.y));
	}

	/** Calculates perpendicular of v, rotated 90 degrees clockwise -- cross(v, rperp(v)) <= 0 */
	constexpr Vec2 getRPerp() const { return Vec2(y, -x); }

	/** Calculates the projection of this over other. */
	constexpr Vec2 project(const Vec2& other) const { return other * (dot(other)/other.dot(other)); }

	/** Complex multiplication of two points ("rotates" two points).
	 @return Vec2 vector with an angle of this.getAngle() + other.getAngle(),
	 and a length of this.getLength() * other.getLength(). */
	constexpr Vec2 rotate(const Vec2& other) const {
		return Vec2(x*other.x - y*other.y, x*other.y + y*other.x);
	}

	/** Unrotates two points.
	 @return Vec2 vector with an angle of this.getAngle() - other.getAngle(),
	 and a length of this.getLength() * other.getLength(). */
	constexpr Vec2 unrotate(const Vec2& other) const {
		return Vec2(x*other.x + y*other.y, y*other.x - x*other.y);
	}

	constexpr Vec2 lerp(const Vec2& other, float alpha) const {
		return *this * (1.f - alpha) + other * alpha;
	}

	constexpr Vec2 getAbs() const {
		return Vec2(std::abs(x), std::abs(y));
	}

	float getAngle(const Vec2& other) const;
	float distance(const Vec2& v) const;
	float length() const;
	Vec2 getNormalized() const;
	Vec2 rotateByAngle(const Vec2& pivot, float angle) const;

	void clamp(const Vec2& min, const Vec2& max);
	Vec2 & normalize();
	void rotate(const Vec2& point, float angle);
};


constexpr const Vec2 Vec2::ZERO(0.0f, 0.0f);
constexpr const Vec2 Vec2::ONE(1.0f, 1.0f);
constexpr const Vec2 Vec2::UNIT_X(1.0f, 0.0f);
constexpr const Vec2 Vec2::UNIT_Y(0.0f, 1.0f);


namespace Anchor {

constexpr const Vec2 Middle(0.5f, 0.5f);
constexpr const Vec2 BottomLeft(0.0f, 0.0f);
constexpr const Vec2 TopLeft(0.0f, 1.0f);
constexpr const Vec2 BottomRight(1.0f, 0.0f);
constexpr const Vec2 TopRight(1.0f, 1.0f);
constexpr const Vec2 MiddleRight(1.0f, 0.5f);
constexpr const Vec2 MiddleLeft(0.0f, 0.5f);
constexpr const Vec2 MiddleTop(0.5f, 1.0f);
constexpr const Vec2 MiddleBottom(0.5f, 0.0f);

}

constexpr inline const Vec2 operator*(float x, const Vec2 &v) {
	Vec2 result(v);
	result.scale(x);
	return result;
}

constexpr inline const Vec2 operator*(const Vec2 &a, const Vec2 &b) {
	return Vec2(a.x * b.x, a.y * b.y);
}

constexpr inline const Vec2 operator/(const Vec2 &a, const Vec2 &b) {
	return Vec2(a.x / b.x, a.y / b.y);
}

constexpr inline const Vec2 operator/(const Vec2 &a, float b) {
	return Vec2(a.x / b, a.y / b);
}

inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const Vec2 & vec) {
	os << "(x: " << std::setprecision(std::numeric_limits<float>::digits10 + 3) << vec.x << "; y: " << vec.y << ")";
	return os;
}

template <typename Callback>
inline bool Vec2::getSegmentIntersectPoint(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D, const Callback &cb) {
	float S, T;

	const auto minXAB = std::min(A.x, B.x);
	const auto minYAB = std::min(A.y, B.y);

	const auto minXCD = std::min(C.x, D.x);
	const auto minYCD = std::min(C.y, D.y);

	const auto maxXAB = std::max(A.x, B.x);
	const auto maxYAB = std::max(A.y, B.y);

	//const auto maxXCD = std::max(C.x, D.x);
	//const auto maxYCD = std::max(C.y, D.y);

	// bbox test
	// first - (minXAB, minYAB) -> (maxXAB, maxYAB)
	// second - (minXCD, minYCD) -> (maxXCD, maxYCD)

	const auto internalBoxWidth = (maxXAB - minXAB) - (minXCD - minXAB);
	const auto internalBoxheight = (maxYAB - minYAB) - (minYCD - minYAB);
	// const auto internalBoxWidth = (maxXCD - minXCD) - (maxXCD - minXCD);
	// const auto internalBoxheight = (maxYCD - minYCD) - (maxYCD - minYCD);

	if (internalBoxWidth > 0.0f && internalBoxheight > 0.0f) {
		if (isLineIntersect(A, B, C, D, &S, &T )&& (S > 0.0f && S < 1.0f && T > 0.0f && T < 1.0f)) {
			// Vec2 of intersection
			cb(Vec2(A.x + S * (B.x - A.x), A.y + S * (B.y - A.y)), S, T);
			return true;
		}
	}

	return false;
}

inline bool Vec2::isCounterClockwise(const Vec2 &u, const Vec2 &v, const Vec2 &w) {
	return (u.x * (v.y - w.y) + v.x * (w.y - u.y) + w.x * (u.y - v.y)) >= 0;
}

inline std::bitset<2> lessThanEqual(const Vec2 &l, const Vec2 &r) {
	std::bitset<2> ret;
	ret.set(0, l.x <= r.x);
	ret.set(1, l.y <= r.y);
	return ret;
}

}

#endif /* STAPPLER_GEOM_SPVEC2_H_ */
