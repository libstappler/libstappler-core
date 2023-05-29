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

#include "SPTessLine.h"

#include "SPSIMD.h"
#include "SPVec4.h"

namespace stappler::geom {

constexpr size_t getMaxRecursionDepth() { return 16; }

// based on:
// http://www.antigrain.com/research/adaptive_bezier/index.html
// https://www.khronos.org/registry/OpenVG/specs/openvg_1_0_1.pdf
// http://www.diva-portal.org/smash/get/diva2:565821/FULLTEXT01.pdf
// https://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes

struct EllipseData {
	float cx;
	float cy;
	float rx;
	float ry;
	float r_sq;
	float cos_phi;
	float sin_phi;
};

SP_ATTR_OPTIMIZE_INLINE_FN static inline simd::f32x4 getNormalizedVec(const float v0[2], const float v1[2], const float v2[2]) {
	simd::f32x4 targetVec = simd::sub(
		simd::load(v0[0], v0[1], v2[0], v2[1]),
		simd::load(v1[0], v1[1], v1[0], v1[1])); // x0, y0, x1, y1

	simd::f32x4 squaredVec = simd::mul(targetVec, targetVec); // x0 * x0, y0 * y0, x1 * x1, y1 * y1

	return simd::mul(targetVec,
		simd::rsqrt( simd::add(squaredVec,
			simde_mm_shuffle_ps(squaredVec, squaredVec, SIMDE_MM_SHUFFLE(2, 3, 0, 1))) ) // sse_rsqrt: n0, n0, n1, n1
		); // nx0, ny0, nx1, ny1
}

SP_ATTR_OPTIMIZE_INLINE_FN static inline simd::f32x4 getBisectVec(const simd::f32x4 &normVec) {
	simd::f32x4 tmpNHalf = simd::load(-0.5f);
	simd::f32x4 normShuffleVec = simde_mm_shuffle_ps(normVec, normVec, SIMDE_MM_SHUFFLE(0, 1, 3, 2)); // nx1, ny1, ny0, nx0

	simd::f32x4 dotVec = simd::mul(normVec, normShuffleVec); // nx0 * nx1, ny0 * ny1, nx1 * ny0, ny1 * nx0

	simd::f32x4 crossVec = simd::sub(
		simde_mm_movehl_ps(dotVec, dotVec),
		simde_mm_shuffle_ps(dotVec, dotVec, SIMDE_MM_SHUFFLE(2, 3, 2, 3)));

	if (simde_mm_comieq_ss(crossVec, simde_mm_setzero_ps())) [[unlikely]] {

		// normal is -ny1, nx1, length is 1

		// return
		// 0.0f
		// 1.0f
		// -ny1
		// nx1

		return simd::mul(
			simd::load(0.0f, 1.0f, -1.0f, 1.0f),
			simde_mm_shuffle_ps(simd::load(1.0f), normVec, SIMDE_MM_SHUFFLE(2, 3, 1, 0))
		);
	} else {
		// -0.5      + -0.5
		// nx0 * nx1 + ny0 * ny1
		// nx0       + nx1
		// ny0       + ny1
		simd::f32x4 normTarget = simd::add(
			simde_mm_movelh_ps(simde_mm_unpacklo_ps(tmpNHalf, dotVec), normVec),   // -0.5, nx0 * nx1, nx0, ny0,
			simde_mm_movelh_ps(simde_mm_move_ss(dotVec, tmpNHalf), normShuffleVec) // -0.5, ny0 * ny1, nx1, ny1,
		); // -1.0 , dot , tx (nx0 + nx1) , ty (ny0 + ny1)

		// -0.5  * -1.0
		// -0.5 * dotValue
		// tx   * tx
		// ty   * ty

		simd::f32x4 squaredVec = simd::mul(simde_mm_movehl_ps(normTarget, tmpNHalf), normTarget); // 0.5, -0.5 * dot, tx * tx, ty * ty

		// combined normalizing, length calculation, ccw test
		// ccw = nx1 * ny0 - ny1 * nx0
		// len = 1.0 * rsqrt( 0.5   + (-0.5 * dot)
		// ntx = tx *  rsqrt( tx*tx +  ty*ty )
		// nty = ty *  rsqrt( tx*tx +  ty*ty )

		return simde_mm_move_ss(
			simd::mul(simde_mm_movehl_ps(normTarget, simd::load(1.0f)),
				simd::rsqrt(
					simd::add(
						squaredVec,
						simde_mm_shuffle_ps(squaredVec, squaredVec, SIMDE_MM_SHUFFLE(2, 3, 0, 1))
					)
				)
			),
			crossVec
		);
	}
}

// combined normalizing, length calculation, ccw test for 3 vertexes
// ccw = nx1 * ny0 - ny1 * nx0
// len = 1.0 * rsqrt( 0.5   + (-0.5 * dot)
// ntx = tx *  rsqrt( tx*tx +  ty*ty )
// nty = ty *  rsqrt( tx*tx +  ty*ty )

SP_ATTR_OPTIMIZE_FN static void getVertexNormalLine(const float v0[2], const float v1[], const float v2[], float result[4]) {
	simd::f32x4 normVec = getNormalizedVec(v0, v1, v2);
	simd::f32x4 bisectVec = getBisectVec(normVec);
	simd::store(result, bisectVec);
}

static inline float draw_approx_err_sq(float e) {
	e = (1.0f / e);
	return e * e;
}

static inline float draw_dist_sq(float x1, float y1, float x2, float y2) {
	const float dx = x2 - x1, dy = y2 - y1;
	return dx * dx + dy * dy;
}

static inline float draw_angle(float v1_x, float v1_y, float v2_x, float v2_y) {
	return atan2f(v1_x * v2_y - v1_y * v2_x, v1_x * v2_x + v1_y * v2_y);
}

static void drawQuadBezierRecursive(LineDrawer &drawer, float x0, float y0, float x1, float y1,
		float x2, float y2, size_t depth) {
	if (depth >= getMaxRecursionDepth()) {
		return;
	}

	const float x01_mid = (x0 + x1) / 2, y01_mid = (y0 + y1) / 2; // between 0 and 1
	const float x12_mid = (x1 + x2) / 2, y12_mid = (y1 + y2) / 2; // between 1 and 2
	const float x_mid = (x01_mid + x12_mid) / 2, y_mid = (y01_mid + y12_mid) / 2; // midpoint on curve

	const float dx = x2 - x0, dy = y2 - y0;
	const float d = fabsf(((x1 - x2) * dy - (y1 - y2) * dx)) * 2.0f; //* 0.5f;

	if (d > std::numeric_limits<float>::epsilon()) { // Regular case
		const float d_sq = (d * d) / (dx * dx + dy * dy);
		if (d_sq <= drawer.distanceError) {
			if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
				drawer.push((x1 + x_mid) / 2, (y1 + y_mid) / 2);
				return;
			} else {
				// Curvature condition  (we need it for offset curve)
				const float da = fabsf(atan2f(y2 - y1, x2 - x1) - atan2f(y1 - y0, x1 - x0));
				if (std::min(da, float(2 * M_PI - da)) < drawer.angularError) {
					drawer.push((x1 + x_mid) / 2, (y1 + y_mid) / 2);
					return;
				}
			}
		}
	} else {
		float sd;
		const float da = dx * dx + dy * dy;
		if (da == 0) {
			sd = draw_dist_sq(x0, y0, x1, y1);
		} else {
			sd = ((x1 - x0) * dx + (y1 - y0) * dy) / da;
			if (sd > 0 && sd < 1) {
				return; // degraded case
			}
			if (sd <= 0) {
				sd = draw_dist_sq(x1, y1, x0, y0);
			} else if(sd >= 1) {
				sd = draw_dist_sq(x1, y1, x2, y2);
			} else {
				sd = draw_dist_sq(x1, y1, x0 + sd * dx, y0 + sd * dy);
			}
		}
		if (sd < drawer.distanceError) {
			drawer.push(x1, y1);
			return;
		}
	}

	drawQuadBezierRecursive(drawer, x0, y0, x01_mid, y01_mid, x_mid, y_mid, depth + 1);
	drawQuadBezierRecursive(drawer, x_mid, y_mid, x12_mid, y12_mid, x2, y2, depth + 1);
}

static void drawCubicBezierRecursive(LineDrawer &drawer, float x0, float y0, float x1, float y1,
		float x2, float y2, float x3, float y3, size_t depth) {
	if (depth >= getMaxRecursionDepth()) {
		return;
	}

	const float x01_mid = (x0 + x1) / 2, y01_mid = (y0 + y1) / 2; // between 0 and 1
	const float x12_mid = (x1 + x2) / 2, y12_mid = (y1 + y2) / 2;// between 1 and 2
	const float x23_mid = (x2 + x3) / 2, y23_mid = (y2 + y3) / 2;// between 2 and 3

	const float x012_mid = (x01_mid + x12_mid) / 2, y012_mid = (y01_mid + y12_mid) / 2;// bisect midpoint in 012
	const float x123_mid = (x12_mid + x23_mid) / 2, y123_mid = (y12_mid + y23_mid) / 2;// bisect midpoint in 123

	const float x_mid = (x012_mid + x123_mid) / 2, y_mid = (y012_mid + y123_mid) / 2;// midpoint on curve

	const float dx = x3 - x0, dy = y3 - y0;
	const float d1 = fabsf(((x1 - x3) * dy - (y1 - y3) * dx)) * 2.0f; // * 0.5f;// distance factor from 0-3 to 1
	const float d2 = fabsf(((x2 - x3) * dy - (y2 - y3) * dx)) * 2.0f; // * 0.5f;// distance factor from 0-3 to 2

	const bool significantPoint1 = d1 > std::numeric_limits<float>::epsilon();
	const bool significantPoint2 = d2 > std::numeric_limits<float>::epsilon();

	if (significantPoint1 && significantPoint1) {
		const float d_sq = ((d1 + d2) * (d1 + d2)) / (dx * dx + dy * dy);
		if (d_sq <= drawer.distanceError) {
			if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
				drawer.push(x12_mid, y12_mid);
				return;
			}

			const float tmp = atan2(y2 - y1, x2 - x1);
			const float da1 = fabs(tmp - atan2(y1 - y0, x1 - x0));
			const float da2 = fabs(atan2(y3 - y2, x3 - x2) - tmp);
			const float da = std::min(da1, float(2 * M_PI - da1)) + std::min(da2, float(2 * M_PI - da2));
			if (da < drawer.angularError) {
				drawer.push(x12_mid, y12_mid);
				return;
			}
		}
	} else if (significantPoint1) {
		const float d_sq = (d1 * d1) / (dx * dx + dy * dy);
		if (d_sq <= drawer.distanceError) {
			if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
				drawer.push(x12_mid, y12_mid);
				return;
			} else {
				const float da = fabsf(atan2f(y2 - y1, x2 - x1) - atan2f(y1 - y0, x1 - x0));
				if (std::min(da, float(2 * M_PI - da)) < drawer.angularError) {
					drawer.push(x1, y1);
					drawer.push(x2, y2);
					return;
				}
			}
		}
	} else if (significantPoint2) {
		const float d_sq = (d2 * d2) / (dx * dx + dy * dy);
		if (d_sq <= drawer.distanceError) {
			if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
				drawer.push(x12_mid, y12_mid);
				return;
			} else {
				const float da = fabsf(atan2f(y3 - y2, x3 - x2) - atan2f(y2 - y1, x2 - x1));
				if (std::min(da, float(2 * M_PI - da)) < drawer.angularError) {
					drawer.push(x1, y1);
					drawer.push(x2, y2);
					return;
				}
			}
		}
	} else {
		float sd1, sd2;
		const float k = dx * dx + dy * dy;
		if (k == 0) {
			sd1 = draw_dist_sq(x0, y0, x1, y1);
			sd2 = draw_dist_sq(x3, y3, x2, y2);
		} else {
			sd1 = ((x1 - x0) * dx + (y1 - y0) * dy) / k;
			sd2 = ((x2 - x0) * dx + (y2 - y0) * dy) / k;
			if (sd1 > 0 && sd1 < 1 && sd2 > 0 && sd2 < 1) {
				return;
			}

			if (sd1 <= 0) {
				sd1 = draw_dist_sq(x1, y1, x0, y0);
			} else if (sd1 >= 1) {
				sd1 = draw_dist_sq(x1, y1, x3, y3);
			} else {
				sd1 = draw_dist_sq(x1, y1, x0 + d1 * dx, y0 + d1 * dy);
			}

			if (sd2 <= 0) {
				sd2 = draw_dist_sq(x2, y2, x0, y0);
			} else if (sd2 >= 1) {
				sd2 = draw_dist_sq(x2, y2, x3, y3);
			} else {
				sd2 = draw_dist_sq(x2, y2, x0 + d2 * dx, y0 + d2 * dy);
			}
		}
		if (sd1 > sd2) {
			if (sd1 < drawer.distanceError) {
				drawer.push(x1, y1);
				return;
			}
		} else {
			if (sd2 < drawer.distanceError) {
				drawer.push(x2, y2);
				return;
			}
		}
	}

	drawCubicBezierRecursive(drawer, x0, y0, x01_mid, y01_mid, x012_mid, y012_mid, x_mid, y_mid, depth + 1);
	drawCubicBezierRecursive(drawer, x_mid, y_mid, x123_mid, y123_mid, x23_mid, y23_mid, x3, y3, depth + 1);
}

static void drawArcRecursive(LineDrawer &drawer, const EllipseData &e, float startAngle, float sweepAngle,
		float x0, float y0, float x1, float y1, size_t depth) {
	if (depth >= getMaxRecursionDepth()) {
		return;
	}

	const float x01_mid = (x0 + x1) / 2, y01_mid = (y0 + y1) / 2;

	const float n_sweep = sweepAngle / 2.0f;

	const float sx_ = e.rx * cosf(startAngle + n_sweep), sy_ = e.ry * sinf(startAngle + n_sweep);
	const float sx = e.cx - (sx_ * e.cos_phi - sy_ * e.sin_phi);
	const float sy = e.cy + (sx_ * e.sin_phi + sy_ * e.cos_phi);

	const float d = draw_dist_sq(x01_mid, y01_mid, sx, sy);

	if (d < drawer.distanceError) {
		if (drawer.angularError < std::numeric_limits<float>::epsilon()) {
			drawer.push(sx, sy);
			return;
		} else {
			const float y0_x0 = y0 / x0;
			const float y1_x1 = y1 / x1;
			const float da = fabs(atanf( e.r_sq * (y1_x1 - y0_x0) / (e.r_sq * e.r_sq * y0_x0 * y1_x1) ));
			if (da < drawer.angularError) {
				drawer.push(sx, sy);
				return;
			}
		}
	}

	drawArcRecursive(drawer, e, startAngle, n_sweep, x0, y0, sx, sy, depth + 1);
	drawer.push(sx, sy);
	drawArcRecursive(drawer, e, startAngle + n_sweep, n_sweep, sx, sy, x1, y1, depth + 1);
}

static void drawArcBegin(LineDrawer &drawer, float x0, float y0, float rx, float ry, float phi,
		bool largeArc, bool sweep, float x1, float y1) {
	rx = fabsf(rx); ry = fabsf(ry);

	const float sin_phi = sinf(phi), cos_phi = cosf(phi);

	const float x01_dst = (x0 - x1) / 2, y01_dst = (y0 - y1) / 2;
	const float x1_ = cos_phi * x01_dst + sin_phi * y01_dst;
	const float y1_ = - sin_phi * x01_dst + cos_phi * y01_dst;

	const float lambda = (x1_ * x1_) / (rx * rx) + (y1_ * y1_) / (ry * ry);
	if (lambda > 1.0f) {
		rx = sqrtf(lambda) * rx; ry = sqrtf(lambda) * ry;
	}

	const float rx_y1_ = (rx * rx * y1_ * y1_);
	const float ry_x1_ = (ry * ry * x1_ * x1_);
	const float c_st = sqrtf(((rx * rx * ry * ry) - rx_y1_ - ry_x1_) / (rx_y1_ + ry_x1_));

	const float cx_ = (largeArc != sweep ? 1.0f : -1.0f) * c_st * rx * y1_ / ry;
	const float cy_ = (largeArc != sweep ? -1.0f : 1.0f) * c_st * ry * x1_ / rx;

	const float cx = cx_ * cos_phi - cy_ * sin_phi + (x0 + x1) / 2;
	const float cy = cx_ * sin_phi + cy_ * cos_phi + (y0 + y1) / 2;

	float startAngle = draw_angle(1.0f, 0.0f, - (x1_ - cx_) / rx, (y1_ - cy_) / ry);
	float sweepAngle = draw_angle((x1_ - cx_) / rx, (y1_ - cy_) / ry, (-x1_ - cx_) / rx, (-y1_ - cy_) / ry);

	sweepAngle = (largeArc)
			? std::max(fabsf(sweepAngle), float(M_PI * 2 - fabsf(sweepAngle)))
			: std::min(fabsf(sweepAngle), float(M_PI * 2 - fabsf(sweepAngle)));

	if (rx > std::numeric_limits<float>::epsilon() && ry > std::numeric_limits<float>::epsilon()) {
		const float r_avg = (rx + ry) / 2.0f;
		const float err = (r_avg - sqrtf(drawer.distanceError)) / r_avg;
		if (err > M_SQRT1_2 * 0.5f - std::numeric_limits<float>::epsilon()) {
			const float pts = ceilf(sweepAngle / (acos(err) * 2.0f)) + 1;
			EllipseData d{ cx, cy, rx, ry, (rx * rx) / (ry * ry), cos_phi, sin_phi };

			sweepAngle = (sweep ? -1.0f : 1.0f) * sweepAngle;

			const float segmentAngle = sweepAngle / pts;

			uint32_t npts = uint32_t(pts);
			for (uint32_t i = 0; i < uint32_t(pts); ++ i) {
				const float sx_ = d.rx * cosf(startAngle + segmentAngle), sy_ = d.ry * sinf(startAngle + segmentAngle);
				const float sx = d.cx - (sx_ * d.cos_phi - sy_ * d.sin_phi);
				const float sy = d.cy + (sx_ * d.sin_phi + sy_ * d.cos_phi);

				drawArcRecursive(drawer, d, startAngle, segmentAngle, x0, y0, sx, sy, 0);
				startAngle += segmentAngle;

				if (npts - 1 == i) {
					drawer.push(x1, y1);
					x0 = x1; y0 = y1;
				} else {
					drawer.push(sx, sy);
					x0 = sx; y0 = sy;
				}
			}

			return;
		} else {
			EllipseData d{ cx, cy, rx, ry, (rx * rx) / (ry * ry), cos_phi, sin_phi };
			drawArcRecursive(drawer, d, startAngle, (sweep ? -1.0f : 1.0f) * sweepAngle, x0, y0, x1, y1, 0);
			drawer.push(x1, y1);
		}
	}
}

LineDrawer::LineDrawer(float e, Rc<Tesselator> &&fill, Rc<Tesselator> &&stroke,
		float w, LineJoin lineJoin, LineCup lineCup)
: lineJoin(lineJoin), lineCup(lineCup), strokeWidth(w / 2.0f), fill(move(fill)), stroke(move(stroke)) {
	if (fill) {
		style |= DrawStyle::Fill;
	}
	if (stroke) {
		style |= DrawStyle::Stroke;
	}

	if ((style & DrawStyle::Stroke) != DrawStyle::None) {
		if (w > 1.0f) {
			distanceError = draw_approx_err_sq(e * log2f(w));
		} else {
			distanceError = draw_approx_err_sq(e);
		}
		angularError = 0.5f;
	} else {
		distanceError = draw_approx_err_sq(e);
		angularError = 0.0f;
	}

	buffer[0].next = &buffer[1]; buffer[0].prev = &buffer[2];
	buffer[1].next = &buffer[2]; buffer[1].prev = &buffer[0];
	buffer[2].next = &buffer[0]; buffer[2].prev = &buffer[1];
	target = &buffer[0];
}

void LineDrawer::drawBegin(float x, float y) {
	if (count > 0) {
		drawClose(false);
	}

	if (fill) {
		fillCursor = fill->beginContour();
	}

	if (stroke) {
		strokeCursor = stroke->beginContour();
	}

	push(x, y);
}
void LineDrawer::drawLine(float x, float y) {
	push(x, y);
}
void LineDrawer::drawQuadBezier(float x1, float y1, float x2, float y2) {
	drawQuadBezierRecursive(*this, target->point.x, target->point.y, x1, y1, x2, y2, 0);
	push(x2, y2);
}
void LineDrawer::drawCubicBezier(float x1, float y1, float x2, float y2, float x3, float y3) {
	drawCubicBezierRecursive(*this, target->point.x, target->point.y, x1, y1, x2, y2, x3, y3, 0);
	push(x3, y3);
}
void LineDrawer::drawArc(float rx, float ry, float phi, bool largeArc, bool sweep, float x1, float y1) {
	drawArcBegin(*this, target->point.x, target->point.y, rx, ry, phi, largeArc, sweep, x1, y1);
}
void LineDrawer::drawClose(bool closed) {
	if (fill) {
		fill->pushVertex(fillCursor, target->point);
		fill->closeContour(fillCursor);
		closed = true;
	}

	if (stroke) {
		if (closed && count > 2) {
			pushStroke(target->prev->point, target->point, origin[0]);
			pushStroke(target->point, origin[0], origin[1]);

			stroke->closeStrokeContour(strokeCursor);
		} else {
			auto norm = target->point - target->prev->point;
			norm.normalize();
			auto perp = norm.getRPerp();
			perp.negate();

			stroke->pushStrokeVertex(strokeCursor, target->point, perp * strokeWidth);
		}
	}

	count = 0;
}

void LineDrawer::push(float x, float y) {
	if (count < 2) {
		origin[count] = Vec2(x, y);
	}

	if (fill) {
		if (count > 0) {
			fill->pushVertex(fillCursor, target->point);
		}
	}
	if (stroke) {
		if (count > 1) {
			pushStroke(target->prev->point, target->point, Vec2(x, y));
		}
	}

	target = target->next;
	target->point = Vec2(x, y);
	++ count;
}

void LineDrawer::pushStroke(const Vec2 &v0, const Vec2 &v1, const Vec2 &v2) {
	Vec4 result;
	getVertexNormalLine(&v0.x, &v1.x, &v2.x, &result.x);

	float mod = copysign(result.y * strokeWidth, result.x);
	if (!strokeCursor.edge) {
		auto norm = v1 - v0;
		norm.normalize();
		auto perp = norm.getRPerp();
		perp.negate();

		stroke->pushStrokeVertex(strokeCursor, v0, perp * strokeWidth);
	}

	if (std::abs(result.y) < _miterLimit) {
		stroke->pushStrokeVertex(strokeCursor, v1, Vec2(result.z * mod, result.w * mod));
	} else {
		auto l0 = v1.getDistanceSq(v0);
		auto l2 = v1.getDistanceSq(v2);

		float qSquared;
		if (l0 > l2) {
			qSquared = l2 / (result.y * result.y - 1);
		} else {
			qSquared = l0 / (result.y * result.y - 1);
		}

		float inverseMiterLimitSq = result.y * result.y * qSquared;
		float offsetLengthSq = mod * mod;

		if (offsetLengthSq > inverseMiterLimitSq) {
			mod = copysign(sqrt(inverseMiterLimitSq), result.x);
		}

		if (mod > 0.0f) {
			do {
				auto norm = v1 - v0;
				norm.normalize();
				auto perp = norm.getRPerp();
				stroke->pushStrokeBottom(strokeCursor, v1 + perp * strokeWidth);
			} while (0);

			do {
				auto norm = v2 - v1;
				norm.normalize();
				auto perp = norm.getRPerp();
				stroke->pushStrokeBottom(strokeCursor, v1 + perp * strokeWidth);
			} while (0);

			stroke->pushStrokeTop(strokeCursor, v1 + Vec2(result.z * mod, result.w * mod));
		} else {
			stroke->pushStrokeBottom(strokeCursor, v1 - Vec2(result.z * mod, result.w * mod));

			do {
				auto norm = v1 - v0;
				norm.normalize();
				auto perp = norm.getRPerp();
				stroke->pushStrokeTop(strokeCursor, v1 - perp * strokeWidth);
			} while (0);

			do {
				auto norm = v2 - v1;
				norm.normalize();
				auto perp = norm.getRPerp();
				stroke->pushStrokeTop(strokeCursor, v1 - perp * strokeWidth);
			} while (0);

		}
	}

}

}
