/**
Copyright 2013 BlackBerry Inc.
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

#include "SPVec2.h"
#include "SPGeometry.h"

namespace STAPPLER_VERSIONIZED stappler::geom {

static bool isOneDimensionSegmentOverlap(float A, float B, float C, float D, float *S, float * E) {
	const float ABmin = std::min(A, B);
	const float ABmax = std::max(A, B);
	const float CDmin = std::min(C, D);
	const float CDmax = std::max(C, D);

	if (ABmax < CDmin || CDmax < ABmin) {
		// ABmin->ABmax->CDmin->CDmax or CDmin->CDmax->ABmin->ABmax
		return false;
	} else {
		if (ABmin >= CDmin && ABmin <= CDmax) {
			// CDmin->ABmin->CDmax->ABmax or CDmin->ABmin->ABmax->CDmax
			if (S != nullptr) *S = ABmin;
			if (E != nullptr) *E = CDmax < ABmax ? CDmax : ABmax;
		} else if (ABmax >= CDmin && ABmax <= CDmax) {
			// ABmin->CDmin->ABmax->CDmax
			if (S != nullptr) *S = CDmin;
			if (E != nullptr) *E = ABmax;
		} else {
			// ABmin->CDmin->CDmax->ABmax
			if (S != nullptr) *S = CDmin;
			if (E != nullptr) *E = CDmax;
		}
		return true;
	}
}

// cross procuct of 2 vector. A->B X C->D
static float crossProduct2Vector(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D) {
	return (D.y - C.y) * (B.x - A.x) - (D.x - C.x) * (B.y - A.y);
}

Vec2::Vec2(const Size2 &s) : x(s.width), y(s.height) { }

Vec2::Vec2(const Extent2 &s) : x(s.width), y(s.height) { }

float Vec2::angle(const Vec2& v1, const Vec2& v2) {
	const float dz = v1.x * v2.y - v1.y * v2.x;
	return atan2f(fabsf(dz) + math::MATH_FLOAT_SMALL, dot(v1, v2));
}

void Vec2::clamp(const Vec2& min, const Vec2& max) {
	assert(!(min.x > max.x || min.y > max.y ));

	// Clamp the x value.
	if (x < min.x) {
		x = min.x;
	}

	if (x > max.x) {
		x = max.x;
	}

	// Clamp the y value.
	if (y < min.y) {
		y = min.y;
	}

	if (y > max.y) {
		y = max.y;
	}
}

void Vec2::clamp(const Vec2& v, const Vec2& min, const Vec2& max, Vec2* dst) {
	assert(dst);
	assert(!(min.x > max.x || min.y > max.y ));

	// Clamp the x value.
	dst->x = v.x;
	if (dst->x < min.x) {
		dst->x = min.x;
	}
	if (dst->x > max.x) {
		dst->x = max.x;
	}

	// Clamp the y value.
	dst->y = v.y;
	if (dst->y < min.y) {
		dst->y = min.y;
	}
	if (dst->y > max.y) {
		dst->y = max.y;
	}
}

Vec2 Vec2::getNormalized() const {
	Vec2 v(*this);
	v.normalize();
	return v;
}

void Vec2::rotate(const Vec2& point, float angle) {
	const double sinAngle = sin(angle);
	const double cosAngle = cos(angle);

	if (point == Vec2::ZERO) {
		float tempX = x * cosAngle - y * sinAngle;
		y = y * cosAngle + x * sinAngle;
		x = tempX;
	} else {
		float tempX = x - point.x;
		float tempY = y - point.y;

		x = tempX * cosAngle - tempY * sinAngle + point.x;
		y = tempY * cosAngle + tempX * sinAngle + point.y;
	}
}

float Vec2::getAngle(const Vec2& other) const {
	Vec2 a2 = getNormalized();
	Vec2 b2 = other.getNormalized();
	const float angle = atan2f(a2.cross(b2), a2.dot(b2));
	if( fabs(angle) < NumericLimits<float>::epsilon() ) {
		return 0.f;
	}
	return angle;
}

Vec2 Vec2::rotateByAngle(const Vec2& pivot, float angle) const {
	return pivot + (*this - pivot).rotate(Vec2::forAngle(angle));
}

bool Vec2::isLineIntersect(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D, float *S, float *T) {
	// FAIL: Line undefined
	if ( (A.x==B.x && A.y==B.y) || (C.x==D.x && C.y==D.y) ) {
		return false;
	}

	const float denom = crossProduct2Vector(A, B, C, D);

	if (denom == 0) {
		// Lines parallel or overlap
		return false;
	}

	if (S != nullptr) {
		*S = crossProduct2Vector(C, D, C, A) / denom;
	}
	if (T != nullptr) {
		*T = crossProduct2Vector(A, B, C, A) / denom;
	}

	return true;
}

bool Vec2::isLineParallel(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D) {
	// FAIL: Line undefined
	if ( (A.x==B.x && A.y==B.y) || (C.x==D.x && C.y==D.y) ) {
		return false;
	}

	if (crossProduct2Vector(A, B, C, D) == 0) {
		// line overlap
		if (crossProduct2Vector(C, D, C, A) == 0 || crossProduct2Vector(A, B, C, A) == 0) {
			return false;
		}

		return true;
	}

	return false;
}

bool Vec2::isLineOverlap(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D) {
	// FAIL: Line undefined
	if ( (A.x==B.x && A.y==B.y) || (C.x==D.x && C.y==D.y) ) {
		return false;
	}

	if (crossProduct2Vector(A, B, C, D) == 0 && (crossProduct2Vector(C, D, C, A) == 0 || crossProduct2Vector(A, B, C, A) == 0)) {
		return true;
	}

	return false;
}

bool Vec2::isSegmentOverlap(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D, Vec2* S, Vec2* E) {
	if (isLineOverlap(A, B, C, D)) {
		return isOneDimensionSegmentOverlap(A.x, B.x, C.x, D.x, &S->x, &E->x)
				&& isOneDimensionSegmentOverlap(A.y, B.y, C.y, D.y, &S->y, &E->y);
	}

	return false;
}

bool Vec2::isSegmentIntersect(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D) {
	float S, T;

	if (isLineIntersect(A, B, C, D, &S, &T )&& (S >= 0.0f && S <= 1.0f && T >= 0.0f && T <= 1.0f)) {
		return true;
	}

	return false;
}

Vec2 Vec2::getIntersectPoint(const Vec2& A, const Vec2& B, const Vec2& C, const Vec2& D) {
	float S, T;

	if (isLineIntersect(A, B, C, D, &S, &T)) {
		// Vec2 of intersection
		Vec2 P;
		P.x = A.x + S * (B.x - A.x);
		P.y = A.y + S * (B.y - A.y);
		return P;
	}

	return Vec2(nan(), nan());
}

#ifdef __LCC__

const Vec2 Vec2::ZERO(0.0f, 0.0f);
const Vec2 Vec2::ONE(1.0f, 1.0f);
const Vec2 Vec2::UNIT_X(1.0f, 0.0f);
const Vec2 Vec2::UNIT_Y(0.0f, 1.0f);

#endif

}
