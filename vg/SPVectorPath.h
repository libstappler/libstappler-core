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

#ifndef STAPPLER_VG_SPVECTORPATH_H_
#define STAPPLER_VG_SPVECTORPATH_H_

#include "SPRef.h"
#include "SPColor.h"
#include "SPGeometry.h"
#include "SPMat4.h"
#include "SPTessLine.h"

#if MODULE_STAPPLER_FILESYSTEM
#include "SPFilesystem.h"
#endif

namespace stappler::vg {

using namespace geom;

using Interface = memory::StandartInterface;

struct PathXRef {
	Interface::StringType id;
	Interface::StringType cacheId;
	Mat4 mat;
};

class VectorPath : public RefBase<Interface> {
public:
	using DrawStyle = geom::DrawStyle;
	using Winding = geom::Winding;
	using LineCup = geom::LineCup;
	using LineJoin = geom::LineJoin;

	struct Params {
		Mat4 transform;
		Color4B fillColor = Color4B(255, 255, 255, 255);
		Color4B strokeColor = Color4B(255, 255, 255, 255);
		DrawStyle style = DrawStyle::Fill;
		float strokeWidth = 1.0f;

		Winding winding = Winding::NonZero;
		LineCup lineCup = LineCup::Butt;
		LineJoin lineJoin = LineJoin::Miter;
		float miterLimit = 4.0f;
		bool isAntialiased = true;
	};

	union CommandData {
		struct {
			float x;
			float y;
		} p;
		struct {
			float v;
			bool a;
			bool b;
		} f;

		CommandData(float x, float y) { p.x = x; p.y = y; }
		CommandData(float r, bool a, bool b) { f.v = r; f.a = a; f.b = b; }
	};

	enum class Command : uint8_t { // use hint to decode data from `_points` vector
		MoveTo, // (x, y)
		LineTo, // (x, y)
		QuadTo, // (x1, y1) (x2, y2)
		CubicTo, // (x1, y1) (x2, y2) (x3, y3)
		ArcTo, // (rx, ry), (x, y), (rotation, largeFlag, sweepFlag)
		ClosePath, // nothing
	};

	VectorPath();
	VectorPath(size_t);

	VectorPath(const VectorPath &);
	VectorPath &operator=(const VectorPath &);

	VectorPath(VectorPath &&);
	VectorPath &operator=(VectorPath &&);

	bool init();
	bool init(const StringView &);
#if MODULE_STAPPLER_FILESYSTEM
	bool init(FilePath &&);
#endif
	bool init(BytesView);

	VectorPath & addPath(const VectorPath &);
	VectorPath & addPath(BytesView);
	VectorPath & addPath(StringView);

	size_t count() const;

	VectorPath & moveTo(float x, float y);
	VectorPath & moveTo(const Vec2 &point) {
		return this->moveTo(point.x, point.y);
	}

	VectorPath & lineTo(float x, float y);
	VectorPath & lineTo(const Vec2 &point) {
		return this->lineTo(point.x, point.y);
	}

	VectorPath & quadTo(float x1, float y1, float x2, float y2);
	VectorPath & quadTo(const Vec2& p1, const Vec2& p2) {
		return this->quadTo(p1.x, p1.y, p2.x, p2.y);
	}

	VectorPath & cubicTo(float x1, float y1, float x2, float y2, float x3, float y3);
	VectorPath & cubicTo(const Vec2& p1, const Vec2& p2, const Vec2& p3) {
		return this->cubicTo(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y);
	}

	// use _to_rad user suffix to convert from degrees to radians
	VectorPath & arcTo(float rx, float ry, float rotation, bool largeFlag, bool sweepFlag, float x, float y);
	VectorPath & arcTo(const Vec2 & r, float rotation, bool largeFlag, bool sweepFlag, const Vec2 &target) {
		return this->arcTo(r.x, r.y, rotation, largeFlag, sweepFlag, target.x, target.y);
	}

	VectorPath & closePath();

	VectorPath & addRect(const Rect& rect);
	VectorPath & addRect(const Rect& rect, float rx, float ry);
	VectorPath & addRect(float x, float y, float width, float height);
	VectorPath & addOval(const Rect& oval);
	VectorPath & addCircle(float x, float y, float radius);
	VectorPath & addEllipse(float x, float y, float rx, float ry);
	VectorPath & addArc(const Rect& oval, float startAngleInRadians, float sweepAngleInRadians);
	VectorPath & addRect(float x, float y, float width, float height, float rx, float ry);

	VectorPath & setFillColor(const Color4B &color);
	VectorPath & setFillColor(const Color3B &color, bool preserveOpacity = false);
	VectorPath & setFillColor(const Color &color, bool preserveOpacity = false);
	const Color4B &getFillColor() const;

	VectorPath & setStrokeColor(const Color4B &color);
	VectorPath & setStrokeColor(const Color3B &color, bool preserveOpacity = false);
	VectorPath & setStrokeColor(const Color &color, bool preserveOpacity = false);
	const Color4B &getStrokeColor() const;

	VectorPath & setFillOpacity(uint8_t value);
	uint8_t getFillOpacity() const;

	VectorPath & setStrokeOpacity(uint8_t value);
	uint8_t getStrokeOpacity() const;

	VectorPath & setStrokeWidth(float width);
	float getStrokeWidth() const;

	VectorPath &setWindingRule(Winding);
	Winding getWindingRule() const;

	VectorPath &setLineCup(LineCup);
	LineCup getLineCup() const;

	VectorPath &setLineJoin(LineJoin);
	LineJoin getLineJoin() const;

	VectorPath &setMiterLimit(float);
	float getMiterLimit() const;

	VectorPath & setStyle(DrawStyle s);
	DrawStyle getStyle() const;

	VectorPath &setAntialiased(bool);
	bool isAntialiased() const;

	// transform should be applied in reverse order
	VectorPath & setTransform(const Mat4 &);
	VectorPath & applyTransform(const Mat4 &);
	const Mat4 &getTransform() const;

	VectorPath & clear();

	VectorPath & setParams(const Params &);
	Params getParams() const;

	bool empty() const;

	// factor - how many points reserve for single command
	void reserve(size_t, size_t factor = 3);

	const Interface::VectorType<Command> &getCommands() const;
	const Interface::VectorType<CommandData> &getPoints() const;

	operator bool() const { return !empty(); }

	Interface::BytesType encode() const;
	Interface::StringType toString(bool newline = false) const;

	size_t commandsCount() const;
	size_t dataCount() const;

protected:
	friend class Image;

	Interface::VectorType<CommandData> _points;
	Interface::VectorType<Command> _commands;
	Params _params;
};

}

#endif /* STAPPLER_VG_SPVECTORPATH_H_ */
