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

#include "SPVectorPath.h"
#include "SPData.h"

namespace stappler::vg {

#define SP_PATH_LOG(...)
//#define SP_PATH_LOG(...) stappler::log::format(log::Debug, "Path Debug", __VA_ARGS__)

#define SP_PATH_LOG_TEXT(...)
//#define SP_PATH_LOG_TEXT(...) stappler::log::text(log::Debug, "Path Debug", __VA_ARGS__)


// to prevent math errors on relative values we use double for SVG reader
// Path itself uses single-word float for performance
class SVGPathReader {
public:
#if MODULE_STAPPLER_FILESYSTEM
	static bool readFile(VectorPath *p, const StringView &str) {
		if (!str.empty()) {
			auto content = filesystem::readTextFile<Interface>(str);
			StringView r(content);
			r.skipUntilString("<path ");
			if (!r.is("<path ")) {
				return false;
			}

			r.skipString("<path ");
			StringView pathContent = r.readUntil<StringView::Chars<'>'>>();
			pathContent.skipUntilString("d=\"");
			if (r.is("d=\"")) {
				r.skipString("d=\"");
				return readPath(p, pathContent.readUntil<StringView::Chars<'"'>>());
			}
		}
		return false;
	}
#endif

	static bool readPath(VectorPath *p, const StringView &r) {
		if (r.size() > 0) {
			SVGPathReader reader(p, r);
			return reader.parse();
		}
		return false;
	}

protected:
	bool parse() {
		while (!reader.empty()) {
			if (!readCmdGroup()) {
				return false;
			}
		}

		return true;
	}

	bool readCmdGroup() {
		readWhitespace();
		while (!reader.empty()) {
			if (!readCmd()) {
				return false;
			}
		}
		return true;
	}

	bool readCmd() {
		if (!readMoveTo()) {
			return false;
		}
		readWhitespace();

		bool readNext = true;
		while(readNext) {
			readNext = readDrawTo();
			if (readNext) {
				readWhitespace();
			}
		}

		return true;
	}

	bool readMoveTo() {
		if (reader >= 1) {
			readWhitespace();
			bool relative = true;
			if (reader.is('M')) {
				relative = false; ++ reader;
			} else if (reader.is('m')) {
				relative = true; ++ reader;
			} else {
				return false;
			}

			readWhitespace();
			return readMoveToArgs(relative);
		}
		return false;
	}

	bool readDrawTo() {
		if (reader >= 1) {
			auto c = reader[0];
			++ reader;

			readWhitespace();
			if (c == 'M' || c == 'm') {
				return readMoveToArgs(c == 'm');
			} else if (c == 'Z' || c == 'z') {
				SP_PATH_LOG("Z");
				if (_pathStarted) {
					_x = _sx;
					_y = _sy;
					_pathStarted = false;
				}
				path->closePath();
				return true;
			} else if (c == 'L' || c == 'l') {
				return readLineToArgs(c == 'l');
			} else if (c == 'H' || c == 'h') {
				return readHorizontalLineTo(c == 'h');
			} else if (c == 'V' || c == 'v') {
				return readVerticalLineTo(c == 'v');
			} else if (c == 'C' || c == 'c') {
				return readCubicBezier(c == 'c');
			} else if (c == 'S' || c == 's') {
				return readCubicBezierShort(c == 's');
			} else if (c == 'Q' || c == 'q') {
				return readQuadraticBezier(c == 'q');
			} else if (c == 'T' || c == 't') {
				return readQuadraticBezierShort(c == 't');
			} else if (c == 'A' || c == 'a') {
				return readEllipticalArc(c == 'a');
			}
		}
		return false;
	}

	bool readLineToArgs(bool relative) {
		double x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readCoordPair(x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) { x = _x + x; y = _y + y; }

				SP_PATH_LOG("L %f %f (%f %f)", x, y, x - _x, y - _y);
				_x = x; _y = y; _b = false;
				path->lineTo(x, y);
			}
		}
		return ret;
	}

	bool readHorizontalLineTo(bool relative) {
		double x;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readNumber(x);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) { x = _x + x; }

				SP_PATH_LOG("H %f (%f)", x, x - _x);
				_x = x; _b = false;
				path->lineTo(x, _y);
			}
		}
		return ret;
	}

	bool readVerticalLineTo(bool relative) {
		double y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readNumber(y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) { y = _y + y; }

				SP_PATH_LOG("V %f (%f)", y, y - _y);
				_y = y; _b = false;
				path->lineTo(_x, y);
			}
		}
		return ret;
	}

	bool readCubicBezier(bool relative) {
		double x1, y1, x2, y2, x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readCurveToArg(x1, y1, x2, y2, x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) {
					x1 = _x + x1; y1 = _y + y1;
					x2 = _x + x2; y2 = _y + y2;
					x = _x + x; y = _y + y;
				}
				_x = x; _y = y; _bx = x2, _by = y2; _b = true;
				SP_PATH_LOG("C %f %f %f %f %f %f", x1, y1, x2, y2, x, y);
				path->cubicTo(x1, y1, x2, y2, x, y);
			}
		}
		return ret;
	}

	bool readCubicBezierShort(bool relative) {
		double x1, y1, x2, y2, x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readSmoothCurveToArg(x2, y2, x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }

				getNewBezierParams(x1, y1);
				if (relative) {
					x2 = _x + x2; y2 = _y + y2;
					x = _x + x; y = _y + y;
				}
				_x = x; _y = y; _bx = x2, _by = y2; _b = true;
				SP_PATH_LOG("S (%f %f) %f %f %f %f", x1, y1, x2, y2, x, y);
				path->cubicTo(x1, y1, x2, y2, x, y);
			}
		}
		return ret;
	}

	bool readQuadraticBezier(bool relative) {
		double x1, y1, x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readQuadraticCurveToArg(x1, y1, x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) {
					x1 = _x + x1; y1 = _y + y1;
					x = _x + x; y = _y + y;
				}
				_x = x; _y = y; _bx = x1, _by = y1; _b = true;
				path->quadTo(x1, y1, x, y);
			}
		}
		return ret;
	}

	bool readQuadraticBezierShort(bool relative) {
		double x1, y1, x, y;
		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readSmoothQuadraticCurveToArg(x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				getNewBezierParams(x1, y1);
				if (relative) {
					x = _x + x; y = _y + y;
				}
				_x = x; _y = y; _bx = x1, _by = y1; _b = true;
				path->quadTo(x1, y1, x, y);
			}
		}
		return ret;
	}

	bool readEllipticalArc(bool relative) {
		double rx, ry, xAxisRotation, x, y;
		bool largeArc, sweep;

		bool readNext = true, first = true, ret = false;
		while (readNext) {
			readCommaWhitespace();
			readNext = readEllipticalArcArg(rx, ry, xAxisRotation, largeArc, sweep, x, y);
			if (first && !readNext) {
				return false;
			} else if (readNext) {
				if (first) { ret = true; first = false; }
				if (relative) {
					x = _x + x; y = _y + y;
				}

				if (rx == 0 || ry == 0) {
					_x = x; _y = y; _b = false;
					path->lineTo(x, y);
				} else {
					_x = x; _y = y; _b = false;
					path->arcTo(rx, ry, xAxisRotation, largeArc, sweep, x, y);
				}
			}
		}
		return ret;
	}

	bool readMoveToArgs(bool relative) {
		double x = 0.0f, y = 0.0f;
		if (!readCoordPair(x, y)) {
			return false;
		}

		if (relative) {
			x = _x + x;
			y = _y + y;
		}

		_b = false;
		_x = x;
		_y = y;
		//if (!_pathStarted) {
			_sx = _x;
			_sy = _y;
			_pathStarted = true;
		//}

		SP_PATH_LOG("M %f %f", _x, _y);
		path->moveTo(x, y);
		readCommaWhitespace();
		readLineToArgs(relative);

		return true;
	}

	bool readCurveToArg(double &x1, double &y1, double &x2, double &y2, double &x, double &y) {
		if (!readCoordPair(x1, y1)) {
			return false;
		}
		readCommaWhitespace();
		if (!readCoordPair(x2, y2)) {
			return false;
		}
		readCommaWhitespace();
		if (!readCoordPair(x, y)) {
			return false;
		}
		return true;
	}

	bool readSmoothCurveToArg(double &x2, double &y2, double &x, double &y) {
		return readQuadraticCurveToArg(x2, y2, x, y);
	}

	bool readQuadraticCurveToArg(double &x1, double &y1, double &x, double &y) {
		if (!readCoordPair(x1, y1)) {
			return false;
		}
		readCommaWhitespace();
		if (!readCoordPair(x, y)) {
			return false;
		}
		return true;
	}

	bool readEllipticalArcArg(double &_rx, double &_ry, double &_xAxisRotation,
			bool &_largeArc, bool &_sweep, double &_dx, double &_dy) {
		double rx, ry, xAxisRotation, x, y;
		bool largeArc, sweep;

		if (!readCoordPair(rx, ry)) {
			return false;
		}
		readCommaWhitespace();
		if (!readNumber(xAxisRotation)) {
			return false;
		}

		if (!readCommaWhitespace()) {
			return false;
		}

		if (!readFlag(largeArc)) {
			return false;
		}

		readCommaWhitespace();
		if (!readFlag(sweep)) {
			return false;
		}

		readCommaWhitespace();
		if (!readCoordPair(x, y)) {
			return false;
		}

		_rx = rx;
		_ry = ry;
		_xAxisRotation = xAxisRotation;
		_largeArc = largeArc;
		_sweep = sweep;
		_dx = x;
		_dy = y;

		return true;
	}

	bool readSmoothQuadraticCurveToArg(double &x, double &y) {
		return readCoordPair(x, y);
	}

	bool readCoordPair(double &x, double &y) {
		double value1 = 0.0f, value2 = 0.0f;
		if (!readNumber(value1)) {
			return false;
		}
		readCommaWhitespace();
		if (!readNumber(value2)) {
			return false;
		}

		x = value1;
		y = value2;
		return true;
	}

	bool readWhitespace() { return reader.readChars<StringView::WhiteSpace>().size() != 0; }

	bool readCommaWhitespace() {
		if (reader >= 1) {
			bool ws = readWhitespace();
			if (reader.is(',')) {
				++ reader;
			} else {
				return ws;
			}
			readWhitespace();
			return true;
		}
		return false;
	}

	bool readNumber(double &val) {
		if (!reader.empty()) {
			if (!reader.readDouble().grab(val)) {
				return false;
			}
			return true;
		}
		return false;
	}

	bool readFlag(bool &flag) {
		if (reader >= 1) {
			if (reader.is('0') || reader.is('1')) {
				flag = (reader.is('1'));
				++ reader;
				return true;
			}
		}
		return false;
	}

	void getNewBezierParams(double &bx, double &by) {
		if (_b) {
			bx = _x * 2 - _bx; by = _y * 2 - _by;
		} else {
			bx = _x; by = _y;
		}
	}

	SVGPathReader(VectorPath *p, const StringView &r)
	: path(p), reader(r) { }

	double _x = 0.0f, _y = 0.0f;

	bool _b = false;
	double _bx = 0.0f, _by = 0.0f;

	double _sx = 0.0f, _sy = 0.0f;
	bool _pathStarted = false;
	VectorPath *path = nullptr;
	StringView reader;
};

VectorPath::VectorPath() { }
VectorPath::VectorPath(size_t count) {
	_points.reserve(count * 3);
	_commands.reserve(count);
}

VectorPath::VectorPath(const VectorPath &path) : _points(path._points), _commands(path._commands), _params(path._params) { }

VectorPath &VectorPath::operator=(const VectorPath &path) {
	_points = path._points;
	_commands = path._commands;
	_params = path._params;
	return *this;
}

VectorPath::VectorPath(VectorPath &&path) : _points(move(path._points)), _commands(move(path._commands)), _params(move(path._params)) { }

VectorPath &VectorPath::operator=(VectorPath &&path) {
	_points = move(path._points);
	_commands = move(path._commands);
	_params = move(path._params);
	return *this;
}

bool VectorPath::init() {
	return true;
}

bool VectorPath::init(const StringView &str) {
	_commands.clear();
	_points.clear();

	if (!SVGPathReader::readPath(this, str)) {
		return false;
	}
	return true;
}

#if MODULE_STAPPLER_FILESYSTEM
bool VectorPath::init(FilePath &&str) {
	_commands.clear();
	_points.clear();

	if (!SVGPathReader::readFile(this, str.get())) {
		return false;
	}
	return true;
}
#endif

bool VectorPath::init(BytesView data) {
	_commands.clear();
	_points.clear();

	SP_PATH_LOG_TEXT("VectorPath::init");

	BytesViewNetwork reader(data);

	auto v = data::cbor::_readInt(reader);
	if (v != 1) {
		return false; // unsupported version
	}

	addPath(data);
	return true;
}

VectorPath & VectorPath::addPath(const VectorPath &path) {
	auto &cmds = path.getCommands();
	auto &points = path.getPoints();

	_commands.reserve(_commands.size() + cmds.size());
	for (auto &it : cmds) { _commands.emplace_back(it); }

	_points.reserve(_points.size() + points.size());
	for (auto &it : points) { _points.emplace_back(it); }

	return *this;
}

VectorPath & VectorPath::addPath(StringView str) {
	SVGPathReader::readPath(this, str);
	return *this;
}

VectorPath & VectorPath::addPath(BytesView data) {
	float x1, y1, x2, y2, x3, y3;
	uint32_t tmp;

	BytesViewNetwork reader(data);

	auto v = data::cbor::_readInt(reader);
	if (v != 1) {
		return *this; // unsupported version
	}

	auto ncommands = data::cbor::_readInt(reader);
	auto npoints = data::cbor::_readInt(reader);
	_commands.reserve(ncommands);
	_points.reserve(npoints);
	for (; ncommands != 0; --ncommands) {
		auto cmd = data::cbor::_readInt(reader);
		switch (uint8_t(cmd)) {
		case toInt(Command::MoveTo):
			x1 = data::cbor::_readNumber(reader);
			y1 = data::cbor::_readNumber(reader);
			SP_PATH_LOG_TEXT("L ", x1, " ", y1);
			moveTo(x1, y1);
			break;
		case toInt(Command::LineTo):
			x1 = data::cbor::_readNumber(reader);
			y1 = data::cbor::_readNumber(reader);
			SP_PATH_LOG_TEXT("L ", x1, " ", y1);
			lineTo(x1, y1);
			break;
		case toInt(Command::QuadTo):
			x1 = data::cbor::_readNumber(reader);
			y1 = data::cbor::_readNumber(reader);
			x2 = data::cbor::_readNumber(reader);
			y2 = data::cbor::_readNumber(reader);
			SP_PATH_LOG_TEXT("Q ", x1, " ", y1, " ", x2, " ", y2);
			quadTo(x1, y1, x2, y2);
			break;
		case toInt(Command::CubicTo):
			x1 = data::cbor::_readNumber(reader);
			y1 = data::cbor::_readNumber(reader);
			x2 = data::cbor::_readNumber(reader);
			y2 = data::cbor::_readNumber(reader);
			x3 = data::cbor::_readNumber(reader);
			y3 = data::cbor::_readNumber(reader);
			SP_PATH_LOG_TEXT("C ", x1, " ", y1, " ", x2, " ", y2, " ", x3, " ", y3);
			cubicTo(x1, y1, x2, y2, x3, y3);
			break;
		case toInt(Command::ArcTo):
			x1 = data::cbor::_readNumber(reader);
			y1 = data::cbor::_readNumber(reader);
			x2 = data::cbor::_readNumber(reader);
			y2 = data::cbor::_readNumber(reader);
			x3 = data::cbor::_readNumber(reader);
			tmp = data::cbor::_readInt(reader);
			SP_PATH_LOG_TEXT("A ", x1, " ", y1, " ", x2, " ", y2, " ", x3, " ", tmp, " ", ((tmp & 2) != 0), " ", ((tmp & 1) != 0));
			arcTo(x1, y1, x3, (tmp & 2) != 0, (tmp & 1) != 0, x2, y2);
			break;
		case toInt(Command::ClosePath):
			closePath();
			SP_PATH_LOG_TEXT("Z");
			break;
		default: break;
		}
	}
	return *this;
}

size_t VectorPath::count() const {
	return _commands.size();
}

VectorPath & VectorPath::moveTo(float x, float y) {
	_commands.emplace_back(Command::MoveTo);
	_points.emplace_back(x, y);
	return *this;
}

VectorPath & VectorPath::lineTo(float x, float y) {
	_commands.emplace_back((_commands.empty() || _commands.back() == Command::ClosePath) ? Command::MoveTo : Command::LineTo);
	_points.emplace_back(x, y);
	return *this;
}

VectorPath & VectorPath::quadTo(float x1, float y1, float x2, float y2) {
	_commands.emplace_back(Command::QuadTo);
	_points.emplace_back(x1, y1);
	_points.emplace_back(x2, y2);
	return *this;
}

VectorPath & VectorPath::cubicTo(float x1, float y1, float x2, float y2, float x3, float y3) {
	_commands.emplace_back(Command::CubicTo);
	_points.emplace_back(x1, y1);
	_points.emplace_back(x2, y2);
	_points.emplace_back(x3, y3);
	return *this;
}

VectorPath & VectorPath::arcTo(float rx, float ry, float angle, bool largeFlag, bool sweepFlag, float x, float y) {
	_commands.emplace_back(Command::ArcTo);
	_points.emplace_back(rx, ry);
	_points.emplace_back(x, y);
	_points.emplace_back(angle, largeFlag, sweepFlag);
	return *this;
}
VectorPath & VectorPath::closePath() {
	_commands.emplace_back(Command::ClosePath);
	return *this;
}

VectorPath & VectorPath::addRect(const Rect& rect) {
	return addRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}
VectorPath & VectorPath::addRect(const Rect& rect, float rx, float ry) {
	return addRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height, rx, ry);
}
VectorPath & VectorPath::addRect(float x, float y, float width, float height) {
	moveTo(x, y);
	lineTo(x + width, y);
	lineTo(x + width, y + height);
	lineTo(x, y + height);
	closePath();
	return *this;
}
VectorPath & VectorPath::addOval(const Rect& oval) {
	addEllipse(oval.getMidX(), oval.getMidY(), oval.size.width / 2.0f, oval.size.height / 2.0f);
	return *this;
}
VectorPath & VectorPath::addCircle(float x, float y, float radius) {
	moveTo(x + radius, y);
	arcTo(radius, radius, 0, false, false, x, y - radius);
	arcTo(radius, radius, 0, false, false, x - radius, y);
	arcTo(radius, radius, 0, false, false, x, y + radius);
	arcTo(radius, radius, 0, false, false, x + radius, y);
	closePath();
	return *this;
}

VectorPath & VectorPath::addEllipse(float x, float y, float rx, float ry) {
	moveTo(x + rx, y);
	arcTo(rx, ry, 0, false, false, x, y - ry);
	arcTo(rx, ry, 0, false, false, x - rx, y);
	arcTo(rx, ry, 0, false, false, x, y + ry);
	arcTo(rx, ry, 0, false, false, x + rx, y);
	closePath();
	return *this;
}

VectorPath & VectorPath::addArc(const Rect& oval, float startAngle, float sweepAngle) {
	const auto rx = oval.size.width / 2;
	const auto ry = oval.size.height / 2;

	const auto x = rx * cosf(startAngle);
	const auto y = ry * sinf(startAngle);

	const auto sx = rx * cosf(startAngle + sweepAngle);
	const auto sy = ry * sinf(startAngle + sweepAngle);

	moveTo(oval.origin.x + rx + x, oval.origin.y + ry + y);
	arcTo(rx, ry, 0.0f, (sweepAngle > numbers::pi)?true:false, true, oval.origin.x + rx + sx, oval.origin.y + ry + sy);
	return *this;
}

VectorPath & VectorPath::addRect(float x, float y, float width, float height, float rx, float ry) {
	if (isnan(rx)) {
		rx = 0.0f;
	}
	if (isnan(ry)) {
		ry = 0.0f;
	}

	if (rx == 0.0f && ry == 0.0f) {
		return addRect(x, y, width, height);
	} else if (rx == 0.0f) {
		rx = ry;
	} else if (ry == 0.0f) {
		ry = rx;
	}

	rx = std::min(width / 2.0f, rx);
	ry = std::min(height / 2.0f, ry);

	moveTo(x + width - rx, y);
	arcTo(rx, ry, 0, false, true, x + width, y + ry);
	lineTo(x + width, y + height - ry);
	arcTo(rx, ry, 0, false, true, x + width - rx, y + height);
	lineTo(x + rx, y + height);
	arcTo(rx, ry, 0, false, true, x, y + height - ry);
	lineTo(x, y + ry);
	arcTo(rx, ry, 0, false, true, x + rx, y);
	closePath();

	return *this;
}

VectorPath & VectorPath::setFillColor(const Color4B &color) {
	_params.fillColor = color;
	return *this;
}
VectorPath & VectorPath::setFillColor(const Color3B &color, bool preserveOpacity) {
	_params.fillColor = Color4B(color, preserveOpacity?_params.fillColor.a:255);
	return *this;
}
VectorPath & VectorPath::setFillColor(const Color &color, bool preserveOpacity) {
	_params.fillColor = Color4B(color, preserveOpacity?_params.fillColor.a:255);
	return *this;
}
const Color4B &VectorPath::getFillColor() const {
	return _params.fillColor;
}

VectorPath & VectorPath::setStrokeColor(const Color4B &color) {
	_params.strokeColor = color;
	return *this;
}
VectorPath & VectorPath::setStrokeColor(const Color3B &color, bool preserveOpacity) {
	_params.strokeColor = Color4B(color, preserveOpacity?_params.fillColor.a:255);
	return *this;
}
VectorPath & VectorPath::setStrokeColor(const Color &color, bool preserveOpacity) {
	_params.strokeColor = Color4B(color, preserveOpacity?_params.fillColor.a:255);
	return *this;
}
const Color4B &VectorPath::getStrokeColor() const {
	return _params.strokeColor;
}

VectorPath & VectorPath::setFillOpacity(uint8_t value) {
	_params.fillColor.a = value;
	return *this;
}
uint8_t VectorPath::getFillOpacity() const {
	return _params.fillColor.a;
}

VectorPath & VectorPath::setStrokeOpacity(uint8_t value) {
	_params.strokeColor.a = value;
	return *this;
}
uint8_t VectorPath::getStrokeOpacity() const {
	return _params.strokeColor.a;
}

VectorPath & VectorPath::setStrokeWidth(float width) {
	_params.strokeWidth = width;
	return *this;
}

float VectorPath::getStrokeWidth() const {
	return _params.strokeWidth;
}

VectorPath &VectorPath::setWindingRule(Winding value) {
	_params.winding = value;
	return *this;
}
vg::Winding VectorPath::getWindingRule() const {
	return _params.winding;
}

VectorPath &VectorPath::setLineCup(LineCup value) {
	_params.lineCup = value;
	return *this;
}
vg::LineCup VectorPath::getLineCup() const {
	return _params.lineCup;
}

VectorPath &VectorPath::setLineJoin(LineJoin value) {
	_params.lineJoin = value;
	return *this;
}
vg::LineJoin VectorPath::getLineJoin() const {
	return _params.lineJoin;
}

VectorPath &VectorPath::setMiterLimit(float value) {
	_params.miterLimit = value;
	return *this;
}
float VectorPath::getMiterLimit() const {
	return _params.miterLimit;
}

VectorPath & VectorPath::setStyle(DrawStyle s) {
	_params.style = s;
	return *this;
}

vg::DrawStyle VectorPath::getStyle() const {
	return _params.style;
}

VectorPath &VectorPath::setAntialiased(bool val) {
	_params.isAntialiased = val;
	return *this;
}
bool VectorPath::isAntialiased() const {
	return _params.isAntialiased;
}

VectorPath & VectorPath::setTransform(const Mat4 &t) {
	_params.transform = t;
	return *this;
}
VectorPath & VectorPath::applyTransform(const Mat4 &t) {
	_params.transform *= t;
	return *this;
}
const Mat4 &VectorPath::getTransform() const {
	return _params.transform;
}

VectorPath & VectorPath::clear() {
	if (!empty()) {
		_commands.clear();
		_points.clear();
	}
	return *this;
}

VectorPath & VectorPath::setParams(const Params &p) {
	_params = p;
	return *this;
}

VectorPath::Params VectorPath::getParams() const {
	return _params;
}

bool VectorPath::empty() const {
	return _commands.empty();
}

void VectorPath::reserve(size_t s, size_t factor) {
	_commands.reserve(s);
	_points.reserve(s * factor);
}

const Interface::VectorType<VectorPath::Command> &VectorPath::getCommands() const {
	return _commands;
}

const Interface::VectorType<VectorPath::CommandData> &VectorPath::getPoints() const {
	return _points;
}


class PathBinaryEncoder {
public: // utility
	PathBinaryEncoder(Interface::BytesType *b) : buffer(b) { }

	void emplace(uint8_t c) {
		buffer->emplace_back(c);
	}

	void emplace(const uint8_t *buf, size_t size) {
		size_t tmpSize = buffer->size();
		buffer->resize(tmpSize + size);
		memcpy(buffer->data() + tmpSize, buf, size);
	}

private:
	Interface::BytesType *buffer;
};


Interface::BytesType VectorPath::encode() const {
	Interface::BytesType ret;
	ret.reserve(_commands.size() * sizeof(Command) + _points.size() * sizeof(CommandData) + 2 * (sizeof(size_t) + 1));
	PathBinaryEncoder enc(&ret);

	data::cbor::_writeInt(enc, 1); // version
	data::cbor::_writeInt(enc, _commands.size());
	data::cbor::_writeInt(enc, _points.size());
	auto d = _points.data();
	for (auto &it : _commands) {
		data::cbor::_writeInt(enc, toInt(it));
		switch (it) {
		case Command::MoveTo:
		case Command::LineTo:
			data::cbor::_writeNumber(enc, d[0].p.x);
			data::cbor::_writeNumber(enc, d[0].p.y);
			SP_PATH_LOG_TEXT("L ", d[0].p.x, " ", d[0].p.y);
			++ d;
			break;
		case Command::QuadTo:
			data::cbor::_writeNumber(enc, d[0].p.x);
			data::cbor::_writeNumber(enc, d[0].p.y);
			data::cbor::_writeNumber(enc, d[1].p.x);
			data::cbor::_writeNumber(enc, d[1].p.y);
			SP_PATH_LOG_TEXT("Q ", d[0].p.x, " ", d[0].p.y, " ", d[1].p.x, " ", d[1].p.y);
			d += 2;
			break;
		case Command::CubicTo:
			data::cbor::_writeNumber(enc, d[0].p.x);
			data::cbor::_writeNumber(enc, d[0].p.y);
			data::cbor::_writeNumber(enc, d[1].p.x);
			data::cbor::_writeNumber(enc, d[1].p.y);
			data::cbor::_writeNumber(enc, d[2].p.x);
			data::cbor::_writeNumber(enc, d[2].p.y);
			SP_PATH_LOG_TEXT("C ", d[0].p.x, " ", d[0].p.y, " ", d[1].p.x, " ", d[1].p.y, " ", d[2].p.x, " ", d[2].p.y);
			d += 3;
			break;
		case Command::ArcTo:
			data::cbor::_writeNumber(enc, d[0].p.x);
			data::cbor::_writeNumber(enc, d[0].p.y);
			data::cbor::_writeNumber(enc, d[1].p.x);
			data::cbor::_writeNumber(enc, d[1].p.y);
			data::cbor::_writeNumber(enc, d[2].f.v);
			data::cbor::_writeInt(enc, (uint32_t(d[2].f.a ? 1 : 0) << 1) | uint32_t(d[2].f.b ? 1 : 0));
			SP_PATH_LOG_TEXT("A ", d[0].p.x, " ", d[0].p.y, " ", d[1].p.x, " ", d[1].p.y, " " << d[2].f.v, " ",
					((uint32_t(d[2].f.a ? 1 : 0) << 1) | uint32_t(d[2].f.b ? 1 : 0)), " ", d[2].f.a, " ", d[2].f.b);
			d += 3;
			break;
		case Command::ClosePath:
			SP_PATH_LOG_TEXT("Z");
			break;
		default: break;
		}
	}

	return ret;
}

Interface::StringType VectorPath::toString(bool newline) const {
	Interface::StringStreamType stream;

	auto streamL = [&] (StringView str) {
		stream << str;
	};

	Callback<void(StringView)> streamCb(streamL);

	//stream << std::setprecision(std::numeric_limits<double>::max_digits10);
	auto d = _points.data();
	for (auto &it : _commands) {
		switch (it) {
		case Command::MoveTo:
			if (newline && d != _points.data()) {
				streamCb << "\n";
			}
			streamCb << "M " << d[0].p.x << "," << d[0].p.y << " ";
			++ d;
			break;
		case Command::LineTo:
			streamCb << "L " << d[0].p.x << "," << d[0].p.y << " ";
			++ d;
			break;
		case Command::QuadTo:
			streamCb << "Q " << d[0].p.x << "," << d[0].p.y << " "
					<< d[1].p.x << "," << d[1].p.y << " ";
			d += 2;
			break;
		case Command::CubicTo:
			streamCb << "C " << d[0].p.x << "," << d[0].p.y << " "
					<< d[1].p.x << "," << d[1].p.y << " "
					<< d[2].p.x << "," << d[2].p.y << " ";
			d += 3;
			break;
		case Command::ArcTo:
			streamCb << "A " << d[0].p.x << "," << d[0].p.y << " "
					<< d[2].f.v << " " << int32_t(d[2].f.a) << " " << int32_t(d[2].f.b) << " "
					<< d[1].p.x << "," << d[1].p.y << " ";
			d += 3;
			break;
		case Command::ClosePath:
			streamCb << "Z ";
			break;
		default: break;
		}
	}

	if (newline) {
		streamCb << "\n";
	}

	return stream.str();
}

size_t VectorPath::commandsCount() const {
	return _commands.size();
}
size_t VectorPath::dataCount() const {
	return _points.size();
}

}
