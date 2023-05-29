/**
Copyright (c) 2021-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#include "STSqliteHandle.h"
#include "STSqliteDriver.h"
#include "sqlite3.h"

namespace stappler::db::sqlite {

static std::mutex s_logMutex;

class SqliteQueryInterface : public db::QueryInterface {
public:
	using Binder = db::Binder;

	struct BindingData {
		uint32_t idx = 0;
		Bytes data;
		Type type;
	};

	size_t push(String &&val) {
		auto &it = params.emplace_back(BindingData());
		it.data.assign_strong((uint8_t *)val.data(), val.size() + 1);
		it.idx = params.size();
		it.type = Type::Text;
		return it.idx;
	}

	size_t push(const StringView &val) {
		auto &it = params.emplace_back(BindingData());
		it.data.assign_strong((uint8_t *)val.data(), val.size() + 1);
		it.data.data()[val.size()] = 0;
		it.idx = params.size();
		it.type = Type::Text;
		return it.idx;
	}

	size_t push(Bytes &&val) {
		auto &it = params.emplace_back(BindingData());
		it.data = std::move(val);
		it.idx = params.size();
		it.type = Type::Bytes;
		return it.idx;
	}

	size_t push(StringStream &query, const Value &val, bool force, bool compress = false) {
		if (!force || val.getType() == Value::Type::EMPTY) {
			switch (val.getType()) {
			case Value::Type::EMPTY: query << "NULL"; break;
			case Value::Type::BOOLEAN: query << (val.asBool() ? "TRUE" : "FALSE"); break;
			case Value::Type::INTEGER: query << val.asInteger(); break;
			case Value::Type::DOUBLE:
				if (std::isnan(val.asDouble())) {
					query << "'NaN'";
				} else if (val.asDouble() == std::numeric_limits<double>::infinity()) {
					query << "'-Infinity'";
				} else if (-val.asDouble() == std::numeric_limits<double>::infinity()) {
					query << "'Infinity'";
				} else {
					query << std::setprecision(std::numeric_limits<double>::max_digits10 + 1) << val.asDouble();
				}
				break;
			case Value::Type::CHARSTRING:
				query << "?" << push(val.getString());
				break;
			case Value::Type::BYTESTRING:
				query << "?" << push(val.asBytes());
				break;
			case Value::Type::ARRAY:
			case Value::Type::DICTIONARY:
				query << "?" << push(data::write<Interface>(val, EncodeFormat(EncodeFormat::Cbor,
						compress ? EncodeFormat::LZ4HCCompression : EncodeFormat::DefaultCompress)));
				break;
			default: break;
			}
		} else {
			query << "?" << push(data::write<Interface>(val, EncodeFormat(EncodeFormat::Cbor,
					compress ? EncodeFormat::LZ4HCCompression : EncodeFormat::DefaultCompress)));
		}
		return params.size();
	}

	virtual void bindInt(db::Binder &, StringStream &query, int64_t val) override {
		query << val;
	}
	virtual void bindUInt(db::Binder &, StringStream &query, uint64_t val) override {
		query << val;
	}
	virtual void bindDouble(db::Binder &, StringStream &query, double val) override {
		query << std::setprecision(std::numeric_limits<double>::max_digits10 + 1) << val;
	}
	virtual void bindString(db::Binder &, StringStream &query, const String &val) override {
		if (auto num = push(String(val))) {
			query << "?" << num;
		}
	}
	virtual void bindMoveString(db::Binder &, StringStream &query, String &&val) override {
		if (auto num = push(std::move(val))) {
			query << "?" << num;
		}
	}
	virtual void bindStringView(db::Binder &, StringStream &query, const StringView &val) override {
		if (auto num = push(val)) {
			query << "?" << num;
		}
	}
	virtual void bindBytes(db::Binder &, StringStream &query, const Bytes &val) override {
		if (auto num = push(Bytes(val))) {
			query << "?" << num;
		}
	}
	virtual void bindMoveBytes(db::Binder &, StringStream &query, Bytes &&val) override {
		if (auto num = push(std::move(val))) {
			query << "?" << num;
		}
	}
	virtual void bindCoderSource(db::Binder &, StringStream &query, const stappler::CoderSource &val) override {
		if (auto num = push(Bytes(val.data(), val.data() + val.size()))) {
			query << "?" << num;
		}
	}
	virtual void bindValue(db::Binder &, StringStream &query, const Value &val) override {
		push(query, val, false);
	}
	virtual void bindDataField(db::Binder &, StringStream &query, const db::Binder::DataField &f) override {
		if (f.field && f.field->getType() == db::Type::Custom) {
			if (!f.field->getSlot<db::FieldCustom>()->writeToStorage(*this, query, f.data)) {
				query << "NULL";
			}
		} else {
			push(query, f.data, f.force, f.compress);
		}
	}
	virtual void bindTypeString(db::Binder &, StringStream &query, const db::Binder::TypeString &type) override {
		if (auto num = push(type.str)) {
			query << "?" << num;
		}
	}

	virtual void bindFullText(db::Binder &, StringStream &query, const db::Binder::FullTextField &d) override {
		query << " NULL";
		/*if (!d.data || !d.data.isArray() || d.data.size() == 0) {
			query << "NULL";
		} else {
			bool first = true;
			for (auto &it : d.data.asArray()) {
				auto &data = it.getString(0);
				auto lang = stappler::search::Language(it.getInteger(1));
				auto rank = stappler::search::SearchData::Rank(it.getInteger(2));
				auto type = stappler::search::SearchData::Type(it.getInteger(3));

				if (!data.empty()) {
					if (!first) { query << " || "; } else { first = false; }
					auto dataIdx = push(data);
					if (type == stappler::search::SearchData::Parse) {
						if (rank == stappler::search::SearchData::Unknown) {
							query << " to_tsvector('" << stappler::search::getLanguageName(lang) << "', $" << dataIdx << "::text)";
						} else {
							query << " setweight(to_tsvector('" << stappler::search::getLanguageName(lang) << "', $" << dataIdx << "::text), '" << char('A' + char(rank)) << "')";
						}
					} else {
						query << " $" << dataIdx << "::tsvector";
					}
				} else {
					query << " NULL";
				}
			}
		}*/
	}

	virtual void bindFullTextRank(db::Binder &, StringStream &query, const db::Binder::FullTextRank &d) override {
		query << " NULL";
		/*int normalizationValue = 0;
		if (d.field->hasFlag(db::Flags::TsNormalize_DocLength)) {
			normalizationValue |= 2;
		} else if (d.field->hasFlag(db::Flags::TsNormalize_DocLengthLog)) {
			normalizationValue |= 1;
		} else if (d.field->hasFlag(db::Flags::TsNormalize_UniqueWordsCount)) {
			normalizationValue |= 8;
		} else if (d.field->hasFlag(db::Flags::TsNormalize_UniqueWordsCountLog)) {
			normalizationValue |= 16;
		}
		query << " ts_rank(" << d.scheme << ".\"" << d.field->getName() << "\", " << d.query << ", " << normalizationValue << ")";*/
	}

	virtual void bindFullTextData(db::Binder &, StringStream &query, const db::FullTextData &d) override {
		query << " NULL";
		/*auto idx = push(String(d.buffer));
		switch (d.type) {
		case db::FullTextData::Parse:
			query  << " websearch_to_tsquery('" << d.getLanguage() << "', $" << idx << "::text)";
			break;
		case db::FullTextData::Cast:
			query  << " to_tsquery('" << d.getLanguage() << "', $" << idx << "::text)";
			break;
		case db::FullTextData::ForceCast:
			query  << " $" << idx << "::tsquery ";
			break;
		}*/
	}

	virtual void bindIntVector(Binder &, StringStream &query, const Vector<int64_t> &vec) override {
		query << "(";
		bool start = true;
		for (auto &it : vec) {
			if (start) { start = false; } else { query << ","; }
			query << it;
		}
		query << ")";
	}

	virtual void bindDoubleVector(Binder &b, StringStream &query, const Vector<double> &vec) override {
		query << "(";
		bool start = true;
		for (auto &it : vec) {
			if (start) { start = false; } else { query << ","; }
			bindDouble(b, query, it);
		}
		query << ")";
	}

	virtual void bindStringVector(Binder &b, StringStream &query, const Vector<StringView> &vec) override {
		query << "(";
		bool start = true;
		for (auto &it : vec) {
			if (start) { start = false; } else { query << ","; }
			bindStringView(b, query, it);
		}
		query << ")";
	}

	virtual void clear() override {
		params.clear();
	}

public:
	Vector<BindingData> params;
};

Handle::Handle(const Driver *d, Driver::Handle h) : driver(d), handle(h) {
	_profile = stappler::sql::Profile::Sqlite;
	if (h.get()) {
		auto c = d->getConnection(h);
		if (c.get()) {
			conn = c;
			dbName = driver->getDbName(h);
		}
	}
}

Handle::Handle(Handle &&h) : driver(h.driver), handle(h.handle), conn(h.conn), lastError(h.lastError) {
	h.conn = Driver::Connection(nullptr);
	h.driver = nullptr;
}

Handle & Handle::operator=(Handle &&h) {
	handle = h.handle;
	conn = h.conn;
	driver = h.driver;
	lastError = h.lastError;
	h.conn = Driver::Connection(nullptr);
	h.driver = nullptr;
	return *this;
}

Handle::operator bool() const {
	return conn.get() != nullptr;
}

Driver::Handle Handle::getHandle() const {
	return handle;
}

Driver::Connection Handle::getConnection() const {
	return conn;
}

void Handle::close() {
	conn = Driver::Connection(nullptr);
}

void Handle::makeQuery(const stappler::Callback<void(sql::SqlQuery &)> &cb) {
	SqliteQueryInterface interface;
	db::sql::SqlQuery query(&interface);
	query.setProfile(_profile);
	cb(query);
}

bool Handle::selectQuery(const sql::SqlQuery &query, const stappler::Callback<bool(sql::Result &)> &cb,
		const Callback<void(const Value &)> &errCb) {
	if (!conn.get() || getTransactionStatus() == db::TransactionStatus::Rollback) {
		return false;
	}

	auto queryInterface = static_cast<SqliteQueryInterface *>(query.getInterface());

	if (messages::isDebugEnabled()) {
		if (!query.getTarget().starts_with("__")) {
			messages::local("Database-Query", query.getQuery().weak());
		}
	}

	auto queryString = query.getQuery().weak();

	sqlite3_stmt *stmt = nullptr;
	auto err = sqlite3_prepare_v3((sqlite3 *)conn.get(), queryString.data(), queryString.size(), 0, &stmt, nullptr);
	if (err != SQLITE_OK) {
		auto info = driver->getInfo(conn, err);
		if (errCb) {
			errCb(info);
		}
#if DEBUG
		s_logMutex.lock();
		std::cout << query.getQuery().weak() << "\n";
		std::cout << EncodeFormat::Pretty << info << "\n";
		info.setString(query.getQuery().str(), "query");
		s_logMutex.unlock();
#endif
		cancelTransaction();
		return false;
	}

	// bind
	for (auto &it : queryInterface->params) {
		switch (it.type) {
		case Type::Text:
			sqlite3_bind_text(stmt, it.idx, (const char *)it.data.data(), it.data.size() - 1, SQLITE_STATIC);
			break;
		case Type::Bytes:
			sqlite3_bind_blob(stmt, it.idx, it.data.data(), it.data.size(), SQLITE_STATIC);
			break;
		default:
			break;
		}
	}

	err = sqlite3_step(stmt);
	if (err != SQLITE_OK && err != SQLITE_DONE && err != SQLITE_ROW) {
		auto info = driver->getInfo(conn, err);
		if (errCb) {
			errCb(info);
		}
#if DEBUG
		s_logMutex.lock();
		std::cout << query.getQuery().weak() << "\n";
		std::cout << EncodeFormat::Pretty << info << "\n";
		info.setString(query.getQuery().str(), "query");
		s_logMutex.unlock();
#endif
		sqlite3_finalize(stmt);
		cancelTransaction();
		return false;
	}

	ResultCursor cursor(driver, conn, Driver::Result(stmt), err);
	db::sql::Result ret(&cursor);
	cb(ret);
	return true;
}

bool Handle::performSimpleQuery(const StringView &query, const Callback<void(const Value &)> &errCb) {
	if (getTransactionStatus() == db::TransactionStatus::Rollback) {
		return false;
	}

	StringView queryData = query;

	const char *outPtr = query.data();
	bool success = true;
	while (outPtr && *outPtr != 0 && success) {
		auto size = queryData.size() - (outPtr - queryData.data());
		StringView nextQuery(outPtr, size);
		nextQuery.skipChars<StringView::WhiteSpace>();
		if (nextQuery.empty()) {
			break;
		}

		sqlite3_stmt *stmt = nullptr;
		auto err = sqlite3_prepare_v3((sqlite3 *)conn.get(), nextQuery.data(), nextQuery.size(), 0, &stmt, &outPtr);
		if (err != SQLITE_OK) {
			auto len = outPtr - nextQuery.data();
			StringView performedQuery(nextQuery.data(), len);
			auto info = driver->getInfo(conn, err);
			if (errCb) {
				errCb(info);
			}
			s_logMutex.lock();
			std::cout << performedQuery << "\n";
			std::cout << info << "\n";
			s_logMutex.unlock();
			cancelTransaction();
			return false;
		}

		err = sqlite3_step(stmt);

		if (err != SQLITE_OK && err != SQLITE_DONE && err != SQLITE_ROW) {
			auto info = driver->getInfo(conn, err);
			if (errCb) {
				errCb(info);
			}
	#if DEBUG
			s_logMutex.lock();
			std::cout << nextQuery << "\n";
			std::cout << EncodeFormat::Pretty << info << "\n";
			info.setString(nextQuery, "query");
			s_logMutex.unlock();
	#endif
			sqlite3_finalize(stmt);
			cancelTransaction();
			return false;
		}

		success = ResultCursor::statusIsSuccess(err);
		sqlite3_finalize(stmt);
	}
	return success;
}

bool Handle::performSimpleSelect(const StringView &query, const stappler::Callback<void(sql::Result &)> &cb,
		const Callback<void(const Value &)> &errCb) {
	if (getTransactionStatus() == db::TransactionStatus::Rollback) {
		return false;
	}

	sqlite3_stmt *stmt = nullptr;
	auto err = sqlite3_prepare_v3((sqlite3 *)conn.get(), query.data(), query.size(), 0, &stmt, nullptr);
	if (err != SQLITE_OK) {
		auto info = driver->getInfo(conn, err);
		if (errCb) {
			errCb(info);
		}
		s_logMutex.lock();
		std::cout << query << "\n";
		std::cout << info << "\n";
		s_logMutex.unlock();
		cancelTransaction();
		return false;
	}

	err = sqlite3_step(stmt);

	ResultCursor cursor(driver, conn, Driver::Result(stmt), err);
	db::sql::Result ret(&cursor);
	cb(ret);
	return true;
}

bool Handle::isSuccess() const {
	return ResultCursor::statusIsSuccess(lastError);
}

bool Handle::beginTransaction() {
	if (transactionStatus != db::TransactionStatus::None) {
		return false;
	}

	driver->setUserId(handle, internals::getUserIdFromContext());

	switch (level) {
	case TransactionLevel::Deferred:
		if (performSimpleQuery("BEGIN DEFERRED"_weak)) {
			level = TransactionLevel::Deferred;
			transactionStatus = db::TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::Immediate:
		if (performSimpleQuery("BEGIN IMMEDIATE"_weak)) {
			level = TransactionLevel::Immediate;
			transactionStatus = db::TransactionStatus::Commit;
			return true;
		}
		break;
	case TransactionLevel::Exclusive:
		if (performSimpleQuery("BEGIN EXCLUSIVE"_weak)) {
			level = TransactionLevel::Exclusive;
			transactionStatus = db::TransactionStatus::Commit;
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

bool Handle::endTransaction() {
	switch (transactionStatus) {
	case db::TransactionStatus::Commit:
		transactionStatus = db::TransactionStatus::None;
		if (performSimpleQuery("COMMIT"_weak)) {
			finalizeBroadcast();
			return true;
		}
		break;
	case db::TransactionStatus::Rollback:
		transactionStatus = db::TransactionStatus::None;
		if (performSimpleQuery("ROLLBACK"_weak)) {
			finalizeBroadcast();
			return false;
		}
		break;
	default:
		break;
	}
	return false;
}

}
