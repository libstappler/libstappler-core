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

#ifndef CORE_GEOM_GLSL_SPGLSL_H_
#define CORE_GEOM_GLSL_SPGLSL_H_

#if SP_GLSL

#define color4 vec4
#define SP_GLSL_IN in
#define SP_GLSL_INLINE

#else

#include "SPVec2.h"
#include "SPVec3.h"
#include "SPVec4.h"
#include "SPMat4.h"
#include "SPColor.h"

#define SP_GLSL_IN
#define SP_GLSL_INLINE inline

namespace stappler::glsl {

using vec2 = geom::Vec2;
using vec3 = geom::Vec3;
using vec4 = geom::Vec4;
using mat4 = geom::Mat4;
using uint = uint32_t;
using color4 = geom::Color4F;

template <typename T>
inline T abs(const T &v) {
	return geom::_abs(v);
}

template <typename T>
inline bool all(const T &v) {
	return v.all();
}

template <typename T>
inline bool any(const T &v) {
	return v.any();
}

using math::clamp;

template <typename T>
inline T ceil(const T &v) {
	return geom::_ceil(v);
}

template <typename T>
inline T cross(const T &v1, const T &v2) {
	T ret;
	T::cross(v1, v2, &ret);
	return ret;
}

template <typename T>
inline float distance(const T &v1, const T &v2) {
	return v1.distance(v2);
}

template <typename T>
inline float dot(const T &v1, const T &v2) {
	return T::dot(v1, v2);
}

template <typename T>
inline auto equal(const T &v1, const T &v2) {
	return geom::_equal(v1, v2);
}

template <typename T>
inline auto floor(const T &v1) {
	return geom::_floor(v1);
}

template <typename T>
inline auto fract(const T &v1) {
	return geom::_fract(v1);
}

template <typename T>
inline auto greaterThan(const T &v1, const T &v2) {
	return geom::_greaterThan(v1, v2);
}

template <typename T>
inline auto greaterThanEqual(const T &v1, const T &v2) {
	return geom::_greaterThanEqual(v1, v2);
}

template <typename T>
inline auto inversesqrt(const T &v) {
	return geom::_inversesqrt(v);
}

template <typename T>
inline auto isinf(const T &v) {
	return geom::_isinf(v);
}

template <typename T>
inline auto isnan(const T &v) {
	return geom::_isnan(v);
}

template <typename T>
inline float length(const T &value) {
	return value.length();
}

inline float length(float v) {
	return v;
}

template <typename T>
inline auto lessThan(const T &v1, const T &v2) {
	return geom::_lessThan(v1, v2);
}

template <typename T>
inline auto lessThanEqual(const T &v1, const T &v2) {
	return geom::_lessThanEqual(v1, v2);
}

template <typename T>
inline T max(const T &v1, const T &v2) {
	return geom::_max(v1, v2);
}

template <typename T>
inline T min(const T &v1, const T &v2) {
	return geom::_min(v1, v2);
}

template <typename T>
inline T normalize(const T &value) {
	return value.getNormalized();
}

template <typename T>
inline T sign(const T &value) {
	return geom::_sign(value);
}

template <typename T>
inline T step(const T &v1, const T &v2) {
	return geom::_step(v1, v2);
}

template <typename T>
inline T step(const float &v1, const T &v2) {
	return geom::_step(geom::fill<T>(v1), v2);
}

template <typename T>
inline T trunc(const T &v) {
	return geom::_trunc(v);
}

template <typename T>
inline bool none(const T &v) {
	return v.none();
}

template <typename T>
inline auto notEqual(const T &v1, const T &v2) {
	return geom::_notEqual(v1, v2);
}

template <typename T>
inline auto round(const T &v1) {
	return geom::_round(v1);
}

}

#endif

#endif /* CORE_GEOM_GLSL_SPGLSL_H_ */
