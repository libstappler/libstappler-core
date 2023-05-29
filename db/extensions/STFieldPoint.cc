/**
Copyright (c) 2019-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#include "STFieldPoint.h"
#include "STStorageScheme.h"

namespace stappler::db {

bool FieldPoint::transformValue(const db::Scheme &, const Value &obj, Value &val, bool isCreate) const {
	if (val.isArray() && val.size() == 2 && val.isDouble(0) && val.isDouble(1)) {
		return true;
	}
	return false;
}

Value FieldPoint::readFromStorage(const db::ResultCursor &iface, size_t field) const {
	if (iface.isBinaryFormat(field)) {
		auto r = stappler::BytesViewNetwork(iface.toBytes(field));

		if (r.size() == 16) {
			auto x = r.readFloat64();
			auto y = r.readFloat64();

			return Value({
				Value(x),
				Value(y),
			});
		}
	}
	return Value();
}

bool FieldPoint::writeToStorage(db::QueryInterface &iface, StringStream &query, const Value &val) const {
	if (val.isArray() && val.size() == 2 && val.isDouble(0) && val.isDouble(1)) {
		query << std::setprecision(std::numeric_limits<double>::max_digits10) << "point(" << val.getDouble(0) << "," << val.getDouble(1) << ")";
		return true;
	}
	return false;
}

StringView FieldPoint::getTypeName() const {
	return "point";
}

bool FieldPoint::isSimpleLayout() const {
	return true;
}

bool FieldPoint::isComparationAllowed(stappler::sql::Comparation c) const {
	return c == db::Comparation::Includes || c == db::Comparation::Equal || c == db::Comparation::In;
}

void FieldPoint::writeQuery(const db::Scheme &s, stappler::sql::Query<db::Binder, Interface>::WhereContinue &whi, stappler::sql::Operator op,
		const StringView &f, stappler::sql::Comparation cmp, const Value &val, const Value &) const {
	if (val.isArray() && val.size() == 4) {
		if (whi.state == stappler::sql::Query<db::Binder, Interface>::State::None) {
			whi.state = stappler::sql::Query<db::Binder, Interface>::State::Some;
		} else {
			Query_writeOperator(whi.query->getStream(), op);
		}
		auto &stream = whi.query->getStream();
		stream << "(" << s.getName() << ".\"" << f << "\" <@ box '("
			<< std::setprecision(std::numeric_limits<double>::max_digits10)
			<< val.getDouble(0) << "," << val.getDouble(1) << "),(" << val.getDouble(2) << "," << val.getDouble(3) << ")')";
	}
}

String FieldPoint::getIndexName() const { return toString(name, "_gist_point"); }
String FieldPoint::getIndexField() const { return toString("USING GIST( \"", name, "\")"); }

}
