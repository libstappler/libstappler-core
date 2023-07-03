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

#include "STFieldIntArray.h"
#include "STSqlQuery.h"
#include "STStorageScheme.h"

namespace stappler::db {

bool FieldIntArray::transformValue(const db::Scheme &, const Value &obj, Value &val, bool isCreate) const {
	if (val.isArray()) {
		for (auto &it : val.asArray()) {
			if (!it.isInteger()) {
				return false;
			}
		}
		return true;
	}
	return false;
}

Value FieldIntArray::readFromStorage(const db::ResultCursor &iface, size_t field) const {
	if (iface.isBinaryFormat(field)) {
		auto r = stappler::BytesViewNetwork(iface.toBytes(field));
		SPUNUSED auto ndim = r.readUnsigned32();
		r.offset(4); // ignored;
		SPUNUSED auto oid = r.readUnsigned32();
		auto size = r.readUnsigned32();
		SPUNUSED auto index = r.readUnsigned32();

		if (size > 0) {
			Value ret(Value::Type::ARRAY); ret.getArray().reserve(size);
			while (!r.empty()) {
				auto size = r.readUnsigned32();
				switch (size) {
				case 1: ret.addInteger(r.readUnsigned()); break;
				case 2: ret.addInteger(r.readUnsigned16()); break;
				case 4: ret.addInteger(r.readUnsigned32()); break;
				case 8: ret.addInteger(r.readUnsigned64()); break;
				default: break;
				}
			}
			return ret;
		}
	}
	return Value();
}

bool FieldIntArray::writeToStorage(db::QueryInterface &iface, StringStream &query, const Value &val) const {
	if (val.isArray()) {
		query << "'{";
		bool init = true;
		for (auto &it : val.asArray()) {
			if (init) { init = false; } else { query << ","; }
			query << it.asInteger();
		}
		query << "}'";
		return true;
	}
	return false;
}

StringView FieldIntArray::getTypeName() const {
	return "integer[]";
}

bool FieldIntArray::isSimpleLayout() const {
	return true;
}

String FieldIntArray::getIndexName() const { return toString(name, "_gin_int"); }
String FieldIntArray::getIndexField() const { return toString("USING GIN ( \"", name, "\"  gin__int_ops)"); }

bool FieldIntArray::isComparationAllowed(db::Comparation c) const {
	switch (c) {
	case db::Comparation::Includes:
	case db::Comparation::Equal:
	case db::Comparation::IsNotNull:
	case db::Comparation::IsNull:
		return true;
		break;
	default:
		break;
	}
	return false;
}

void FieldIntArray::writeQuery(const db::Scheme &s, stappler::sql::Query<db::Binder, Interface>::WhereContinue &whi,
			stappler::sql::Operator op, const StringView &f, stappler::sql::Comparation cmp, const Value &val, const Value &) const {
	if (cmp == db::Comparation::IsNull || cmp == db::Comparation::IsNotNull) {
		whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), cmp, val);
	} else {
		if (val.isInteger()) {
			whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), "@>", db::sql::SqlQuery::RawString{toString("ARRAY[", val.asInteger(), ']')});
		} else if (val.isArray()) {
			StringStream str; str << "ARRAY[";
			bool init = false;
			for (auto &it : val.asArray()) {
				if (it.isInteger()) {
					if (init) { str << ","; } else { init = true; }
					str << it.getInteger();
				}
			}
			str << "]";
			if (init) {
				whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), "&&", db::sql::SqlQuery::RawString{str.str()});
			}
		}
	}
}

bool FieldBigIntArray::transformValue(const db::Scheme &, const Value &obj, Value &val, bool isCreate) const {
	if (val.isArray()) {
		for (auto &it : val.asArray()) {
			if (!it.isInteger()) {
				return false;
			}
		}
		return true;
	}
	return false;
}

Value FieldBigIntArray::readFromStorage(const db::ResultCursor &iface, size_t field) const {
	if (iface.isBinaryFormat(field)) {
		auto r = stappler::BytesViewNetwork(iface.toBytes(field));
		SPUNUSED auto ndim = r.readUnsigned32();
		r.offset(4); // ignored;
		SPUNUSED auto oid = r.readUnsigned32();
		auto size = r.readUnsigned32();
		SPUNUSED auto index = r.readUnsigned32();

		if (size > 0) {
			Value ret(Value::Type::ARRAY); ret.getArray().reserve(size);
			while (!r.empty()) {
				auto size = r.readUnsigned32();
				switch (size) {
				case 1: ret.addInteger(r.readUnsigned()); break;
				case 2: ret.addInteger(r.readUnsigned16()); break;
				case 4: ret.addInteger(r.readUnsigned32()); break;
				case 8: ret.addInteger(r.readUnsigned64()); break;
				default: break;
				}
			}
			return ret;
		}
	}
	return Value();
}

bool FieldBigIntArray::writeToStorage(db::QueryInterface &iface, StringStream &query, const Value &val) const {
	if (val.isArray()) {
		query << "'{";
		bool init = true;
		for (auto &it : val.asArray()) {
			if (init) { init = false; } else { query << ","; }
			query << it.asInteger();
		}
		query << "}'";
		return true;
	}
	return false;
}

StringView FieldBigIntArray::getTypeName() const { return "bigint[]"; }
bool FieldBigIntArray::isSimpleLayout() const { return true; }
String FieldBigIntArray::getIndexName() const { return toString(name, "_gin_bigint"); }
String FieldBigIntArray::getIndexField() const { return toString("USING GIN ( \"", name, "\"  array_ops)"); }

bool FieldBigIntArray::isComparationAllowed(db::Comparation c) const {
	switch (c) {
	case db::Comparation::Includes:
	case db::Comparation::Equal:
	case db::Comparation::IsNotNull:
	case db::Comparation::IsNull:
		return true;
		break;
	default:
		break;
	}
	return false;
}

void FieldBigIntArray::writeQuery(const db::Scheme &s, stappler::sql::Query<db::Binder, Interface>::WhereContinue &whi, stappler::sql::Operator op,
		const StringView &f, stappler::sql::Comparation cmp, const Value &val, const Value &) const {
	if (cmp == db::Comparation::IsNull || cmp == db::Comparation::IsNotNull) {
		whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), cmp, val);
	} else {
		if (val.isInteger()) {
			whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), "@>", db::sql::SqlQuery::RawString{toString("ARRAY[", val.asInteger(), "::bigint]")});
		} else if (val.isArray()) {
			StringStream str; str << "ARRAY[";
			bool init = false;
			for (auto &it : val.asArray()) {
				if (it.isInteger()) {
					if (init) { str << ","; } else { init = true; }
					str << it.getInteger() << "::bigint";
				}
			}
			str << "]";
			if (init) {
				whi.where(op, db::sql::SqlQuery::Field(s.getName(), f), "&&", db::sql::SqlQuery::RawString{str.str()});
			}
		}
	}
}

}
