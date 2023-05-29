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

#ifndef STAPPLER_GEOM_SPVEC3_H_
#define STAPPLER_GEOM_SPVEC3_H_

#include "SPSIMD.h"
#include "SPVec2.h"

namespace stappler::geom {

class Mat4;
class Quaternion;

class Vec3 {
public:
	static const Vec3 ZERO;
	static const Vec3 ONE;
	static const Vec3 UNIT_X;
	static const Vec3 UNIT_Y;
	static const Vec3 UNIT_Z;

	static void add(const Vec3& v1, const Vec3& v2, Vec3* dst) {
		dst->x = v1.x + v2.x; dst->y = v1.y + v2.y; dst->z = v1.z + v2.z;
	}
	static void subtract(const Vec3& v1, const Vec3& v2, Vec3* dst) {
		dst->x = v1.x - v2.x; dst->y = v1.y - v2.y; dst->z = v1.z - v2.z;
	}
	static void cross(const Vec3& v1, const Vec3& v2, Vec3* dst) {
		simd::crossVec3(&v1.x, &v2.x, &dst->x);
	}
	static float dot(const Vec3& v1, const Vec3& v2) {
		return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
	}

	static float angle(const Vec3& v1, const Vec3& v2);
	static void clamp(const Vec3& v, const Vec3& min, const Vec3& max, Vec3* dst);

	float x;
	float y;
	float z;

	constexpr Vec3() : x(0.0f), y(0.0f), z(0.0f) { }
	constexpr Vec3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) { }
	constexpr Vec3(const Vec2 &pt, float zz) : x(pt.x), y(pt.y), z(zz) { }

	constexpr Vec3(const Vec3& p1, const Vec3& p2) : x(p2.x - p1.x), y(p2.y - p1.y), z(p2.z - p1.z) { }
	constexpr Vec3(const Vec3& copy) = default;

	constexpr bool isZero() const { return x == 0.0f && y == 0.0f && z == 0.0f; }
	constexpr bool isOne() const { return x == 1.0f && y == 1.0f && z == 1.0f; }
	constexpr void add(const Vec3& v) { x += v.x; y += v.y; z += v.z; }
	constexpr void add(float xx, float yy, float zz) { x += xx; y += yy; z += zz; }
	constexpr void subtract(const Vec3& v) { x -= v.x; y -= v.y; z -= v.z; }
	constexpr float length() const { return std::sqrt(x * x + y * y + z * z); }
	constexpr float lengthSquared() const { return (x * x + y * y + z * z); }
	constexpr void negate() { x = -x; y = -y; z = -z; }
	constexpr void scale(float scalar) { x *= scalar; y *= scalar; z *= scalar; }
	constexpr Vec3 lerp(const Vec3& target, float alpha) const { return *this * (1.f - alpha) + target * alpha; }
	constexpr void setZero() { x = y = z = 0.0f; }

	constexpr bool fuzzyEquals(const Vec3& b, float var) const {
		return (x - var <= b.x && b.x <= x + var) && (y - var <= b.y && b.y <= y + var)
				 && (z - var <= b.z && b.z <= z + var);
	}

	constexpr Vec3 getAbs() const {
		return Vec3(std::abs(x), std::abs(y), std::abs(z));
	}

	void clamp(const Vec3& min, const Vec3& max);
	constexpr Vec3 & normalize();

	void cross(const Vec3& v) { cross(*this, v, this); }
	float dot(const Vec3& v) const { return (x * v.x + y * v.y + z * v.z); }

	float distance(const Vec3& v) const;
	float distanceSquared(const Vec3& v) const;

	Vec2 xy() const;

	Vec3 getNormalized() const;

	/**
	 * Updates this vector towards the given target using a smoothing function.
	 * The given response time determines the amount of smoothing (lag). A longer
	 * response time yields a smoother result and more lag. To force this vector to
	 * follow the target closely, provide a response time that is very small relative
	 * to the given elapsed time. */
	void smooth(const Vec3& target, float elapsedTime, float responseTime);

	constexpr const Vec3 operator+(const Vec3& v) const {
		Vec3 result(*this);
		result.add(v);
		return result;
	}

	constexpr Vec3& operator+=(const Vec3& v) {
		add(v);
		return *this;
	}

	constexpr const Vec3 operator-(const Vec3& v) const {
		Vec3 result(*this);
		result.subtract(v);
		return result;
	}

	constexpr Vec3& operator-=(const Vec3& v) {
		subtract(v);
		return *this;
	}

	constexpr const Vec3 operator-() const {
		Vec3 result(*this);
		result.negate();
		return result;
	}

	constexpr const Vec3 operator*(float s) const {
		Vec3 result(*this);
		result.scale(s);
		return result;
	}

	constexpr Vec3& operator*=(float s) {
		scale(s);
		return *this;
	}

	constexpr const Vec3 operator/(float s) const {
		return Vec3(this->x / s, this->y / s, this->z / s);
	}

	constexpr bool operator < (const Vec3& rhs) const {
		return (x < rhs.x && y < rhs.y && z < rhs.z);
	}

	constexpr bool operator >(const Vec3& rhs) const {
		return (x > rhs.x && y > rhs.y && z > rhs.z);
	}

	constexpr bool operator==(const Vec3& v) const {
		return x==v.x && y==v.y && z==v.z;
	}

	constexpr bool operator!=(const Vec3& v) const {
		return x!=v.x || y!=v.y || z!=v.z;
	}
};

constexpr inline Vec3 & Vec3::normalize() {
	float n = x * x + y * y + z * z;
	// Already normalized.
	if (n == 1.0f) {
		return *this;
	}

	n = std::sqrt(n);
	// Too close to zero.
	if (n < math::MATH_TOLERANCE) {
		return *this;
	}

	n = 1.0f / n;
	x *= n;
	y *= n;
	z *= n;

	return *this;
}

constexpr const Vec3 Vec3::ZERO(0.0f, 0.0f, 0.0f);
constexpr const Vec3 Vec3::ONE(1.0f, 1.0f, 1.0f);
constexpr const Vec3 Vec3::UNIT_X(1.0f, 0.0f, 0.0f);
constexpr const Vec3 Vec3::UNIT_Y(0.0f, 1.0f, 0.0f);
constexpr const Vec3 Vec3::UNIT_Z(0.0f, 0.0f, 1.0f);

constexpr const Vec3 operator*(float x, const Vec3& v) {
	Vec3 result(v);
	result.scale(x);
	return result;
}

inline std::basic_ostream<char> &
operator << (std::basic_ostream<char> & os, const Vec3 & vec) {
	os << "(x: " << vec.x << "; y: " << vec.y << "; z: " << vec.z << ")";
	return os;
}

inline std::bitset<3> lessThanEqual(const Vec3 &l, const Vec3 &r) {
	std::bitset<3> ret;
	ret.set(0, l.x <= r.x);
	ret.set(1, l.y <= r.y);
	ret.set(2, l.z <= r.z);
	return ret;
}

}

#endif /* STAPPLER_GEOM_SPVEC3_H_ */
