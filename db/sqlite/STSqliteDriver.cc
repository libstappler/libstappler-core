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

#include "STSqliteDriver.h"
#include "STSqliteHandle.h"
#include "sqlite3.h"

namespace stappler::db::sqlite {

struct DriverHandle {
	sqlite3 *conn;
	const Driver *driver;
	void *padding;
	pool_t *pool;
	StringView name;
	sqlite3_stmt *oidQuery = nullptr;
	int64_t userId = 0;
};

Driver *Driver::open(StringView path) {
	if (sqlite3_initialize() == SQLITE_OK) {
		return new Driver(path);
	} else {
		std::cout << "[sqlite::Driver] sqlite3_initialize failed\n";
	}
	return nullptr;
}

Driver::~Driver() {
	sqlite3_shutdown();
}

bool Driver::init(Handle handle, const Vector<StringView> &) {
	return true;
}

void Driver::performWithStorage(Handle handle, const Callback<void(const db::Adapter &)> &cb) const {
	auto targetPool = pool::acquire();

	db::sqlite::Handle h(this, handle);
	db::Adapter storage(&h);
	pool::userdata_set((void *)&h, config::getStorageInterfaceKey(), nullptr, targetPool);

	cb(storage);

	auto stack = stappler::memory::pool::get<db::Transaction::Stack>(targetPool, config::getTransactionStackKey());
	if (stack) {
		for (auto &it : stack->stack) {
			if (it->adapter == storage) {
				it->adapter = db::Adapter(nullptr);
				messages::error("Root", "Incomplete transaction found");
			}
		}
	}
	pool::userdata_set((void *)nullptr, storage.getTransactionKey().data(), nullptr, targetPool);
	pool::userdata_set((void *)nullptr, config::getStorageInterfaceKey(), nullptr, targetPool);
}

BackendInterface *Driver::acquireInterface(Handle handle, pool_t *pool) const {
	BackendInterface *ret = nullptr;
	pool::push(pool);
	ret = new (pool) db::sqlite::Handle(this, handle);
	pool::pop();
	return ret;
}

static StringView Driver_exec(pool_t *p, sqlite3 *db, StringView query) {
	sqlite3_stmt *stmt = nullptr;
	if (sqlite3_prepare_v3(db, query.data(), query.size(), 0, &stmt, nullptr) != SQLITE_OK) {
		return StringView();
	}

	auto err = sqlite3_step(stmt);
	if (err != SQLITE_ROW) {
		sqlite3_finalize(stmt);
		return StringView();
	}

	if (p) {
		StringView result((const char *)sqlite3_column_text(stmt, 0), size_t(sqlite3_column_bytes(stmt, 0)));
		result = result.pdup(p);
		sqlite3_finalize(stmt);
		return result;
	}

	sqlite3_finalize(stmt);
	return StringView();
}

static void stellator_next_oid_xFunc(sqlite3_context *ctx, int nargs, sqlite3_value **args) {
	sqlite3_int64 ret = 0;
	DriverHandle *data = (DriverHandle *)sqlite3_user_data(ctx);
	auto err = sqlite3_step(data->oidQuery);
	if (err == SQLITE_ROW) {
		ret = sqlite3_column_int64(data->oidQuery, 0);
	}
	if (!ret) {
		ret = Time::now().toMicros();
	}
	sqlite3_reset(data->oidQuery);
	sqlite3_result_int64(ctx, ret);
}

static void stellator_now_xFunc(sqlite3_context *ctx, int nargs, sqlite3_value **args) {
	sqlite3_result_int64(ctx, Time::now().toMicros());
}

static void stellator_user_xFunc(sqlite3_context *ctx, int nargs, sqlite3_value **args) {
	DriverHandle *data = (DriverHandle *)sqlite3_user_data(ctx);
	sqlite3_result_int64(ctx, data->userId);
}

Driver::Handle Driver::connect(const Map<StringView, StringView> &params) const {
	auto p = pool::create(pool::acquire());
	Driver::Handle rec;
	pool::push(p);

	int flags = 0;
	StringView mode;
	StringView dbname("");
	StringView journal;

	for (auto &it : params) {
		if (it.first == "dbname") {
			dbname = it.second;
		} else if (it.first == "mode") {
			mode = it.second;
		} else if (it.first == "cache") {
			if (it.second == "shared") {
				flags |= SQLITE_OPEN_SHAREDCACHE;
			} else if (it.second == "private") {
				flags |= SQLITE_OPEN_PRIVATECACHE;
			}
		} else if (it.first == "threading") {
			if (it.second == "serialized") {
				flags |= SQLITE_OPEN_FULLMUTEX;
			} else if (it.second == "multi" || it.second == "multithread" || it.second == "multithreaded") {
				flags |= SQLITE_OPEN_NOMUTEX;
			}
		} else if (it.first == "journal") {
			if (it.second == "delete" || it.second == "truncate" || it.second == "persist"
					|| it.second == "memory" || it.second == "wal" || it.second == "off") {
				journal = it.second;
			}
		} else if (it.first != "driver" && it.first == "nmin" && it.first == "nkeep"
				&& it.first == "nmax" && it.first == "exptime" && it.first == "persistent") {
			std::cout << "[sqlite::Driver] unknown connection parameter: " << it.first << "=" << it.second << "\n";
		}
	}

	if (!mode.empty()) {
		if (mode == "ro") {
			flags |= SQLITE_OPEN_READONLY;
		} else if (mode == "rw") {
			flags |= SQLITE_OPEN_READWRITE;
		} else if (mode == "rwc") {
			flags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
		} else if (mode == "memory") {
			flags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_MEMORY;
		} else {
			std::cout << "[sqlite::Driver] unknown mode parameter: " << mode << "\n";
		}
	} else {
		flags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	}

	do {
		sqlite3 *db = nullptr;
		if (!dbname.starts_with("/")) {
			dbname = StringView(filesystem::writablePath<Interface>(dbname)).pdup();
			stappler::filesystem::mkdir_recursive(stappler::filepath::root(dbname), true);
		}
		if (sqlite3_open_v2(dbname.data(), &db, flags, nullptr) == SQLITE_OK) {
			sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DDL, 0, nullptr);
			sqlite3_db_config(db, SQLITE_DBCONFIG_DQS_DML, 0, nullptr);
			sqlite3_db_config(db, SQLITE_DBCONFIG_ENABLE_FKEY, 1, nullptr);

			if (!journal.empty()) {
				auto m = stappler::string::toupper<Interface>(journal);
				auto mode = stappler::string::toupper<Interface>(Driver_exec(p, db, "PRAGMA journal_mode;"));
				if (mode.empty()) {
					sqlite3_close(db);
					break;
				}

				if (mode != m) {
					auto query = toString("PRAGMA journal_mode = ", m);
					auto cmode = stappler::string::toupper<Interface>(Driver_exec(p, db, query));
					if (mode.empty() || cmode != m) {
						std::cout << "[sqlite::Driver] fail to enable journal_mode '" << m << "'\n";
						sqlite3_close(db);
						break;
					}
				}
			}

			Driver_exec(nullptr, db, "CREATE TABLE IF NOT EXISTS \"__objects\" ( \"__oid\" BIGINT NOT NULL DEFAULT 0, \"control\" INT NOT NULL PRIMARY KEY DEFAULT 0 ) WITHOUT ROWID;");
			Driver_exec(nullptr, db, "INSERT OR IGNORE INTO \"__objects\" (\"__oid\") VALUES (0);");

			auto h = (DriverHandle *)pool::palloc(p, sizeof(DriverHandle));
			h->pool = p;
			h->driver = this;
			h->conn = db;
			h->name = dbname.pdup(p);

			StringView str("UPDATE OR IGNORE \"__objects\" SET \"__oid\" = \"__oid\" + 1 WHERE \"control\" = 0 RETURNING \"__oid\";");

			sqlite3_stmt *stmt = nullptr;
			auto err = sqlite3_prepare_v3(db, str.data(), str.size(), SQLITE_PREPARE_PERSISTENT, &stmt, nullptr);
			if (err == SQLITE_OK) {
				h->oidQuery = stmt;
			}

			sqlite3_create_function_v2(db, "stellator_next_oid", 0, SQLITE_UTF8, (void *)h,
					stellator_next_oid_xFunc, nullptr, nullptr, nullptr);
			sqlite3_create_function_v2(db, "stellator_now", 0, SQLITE_UTF8, (void *)h,
					stellator_now_xFunc, nullptr, nullptr, nullptr);
			sqlite3_create_function_v2(db, "stellator_user", 0, SQLITE_UTF8, (void *)h,
					stellator_user_xFunc, nullptr, nullptr, nullptr);

			pool::cleanup_register(p, [query = h->oidQuery, ret = h->conn] {
				if (query) {
					sqlite3_finalize(query);
				}
				sqlite3_close(ret);
			});

			rec = Handle(h);
		}
	} while (0);

	pool::pop();
	if (!rec.get()) {
		pool::destroy(p);
	}
	return rec;
}

void Driver::finish(Handle h) const {
	auto db = (DriverHandle *)h.get();
	if (db && db->pool) {
		pool::destroy(db->pool);
	}
}

Driver::Connection Driver::getConnection(Handle h) const {
	auto db = (DriverHandle *)h.get();
	return Driver::Connection(db->conn);
}

bool Driver::isValid(Handle) const {
	return true;
}

bool Driver::isValid(Connection) const {
	return true;
}

bool Driver::isIdle(Connection) const {
	return true;
}

StringView Driver::getDbName(Handle h) const {
	auto db = (DriverHandle *)h.get();
	return db->name;
}

Value Driver::getInfo(Connection conn, int err) const {
	return Value({
		stappler::pair("error", Value(err)),
		stappler::pair("status", Value(sqlite3_errstr(int(err)))),
		stappler::pair("desc", Value(sqlite3_errmsg((sqlite3 *)conn.get()))),
	});
}

void Driver::setUserId(Handle h, int64_t userId) const {
	auto db = (DriverHandle *)h.get();
	db->userId = userId;
}

Driver::Driver(StringView mem) { _driverPath = mem.pdup(); }

bool ResultCursor::statusIsSuccess(int x) {
	return (x == SQLITE_DONE) || (x == SQLITE_ROW) || (x == SQLITE_OK);
}

ResultCursor::ResultCursor(const Driver *d, Driver::Connection conn, Driver::Result res, int status)
: driver(d), conn(conn), result(res), err(status) { }

ResultCursor::~ResultCursor() {
	clear();
}

bool ResultCursor::isBinaryFormat(size_t field) const {
	return true;
}

BackendInterface::StorageType ResultCursor::getType(size_t field) const {
	auto t = sqlite3_column_type((sqlite3_stmt *)result.get(), field);
	switch (t) {
	case SQLITE_INTEGER: return BackendInterface::StorageType::Int8; break;
	case SQLITE_FLOAT: return BackendInterface::StorageType::Float8; break;
	case SQLITE_TEXT: return BackendInterface::StorageType::Text; break;
	case SQLITE_BLOB: return BackendInterface::StorageType::Bytes; break;
	case SQLITE_NULL: return BackendInterface::StorageType::Unknown; break;
	default: return BackendInterface::StorageType::Unknown; break;
	}
	return BackendInterface::StorageType::Unknown;
}

bool ResultCursor::isNull(size_t field) const {
	return sqlite3_column_type((sqlite3_stmt *)result.get(), field) == SQLITE_NULL;
}

StringView ResultCursor::toString(size_t field) const {
	switch (sqlite3_column_type((sqlite3_stmt *)result.get(), field)) {
	case SQLITE_INTEGER:
		return StringView(toString(sqlite3_column_int64((sqlite3_stmt *)result.get(), field))).pdup();
		break;
	case SQLITE_FLOAT:
		return StringView(toString(sqlite3_column_double((sqlite3_stmt *)result.get(), field))).pdup();
		break;
	case SQLITE_TEXT:
		return StringView((const char *)sqlite3_column_text((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field));
		break;
	case SQLITE_BLOB:
		return StringView((const char *)sqlite3_column_blob((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field));
		break;
	case SQLITE_NULL:
		return StringView("(null)");
		break;
	}

	return StringView();
}

BytesView ResultCursor::toBytes(size_t field) const {
	switch (sqlite3_column_type((sqlite3_stmt *)result.get(), field)) {
	case SQLITE_INTEGER: {
		int64_t value = sqlite3_column_int64((sqlite3_stmt *)result.get(), field);
		return BytesView((const uint8_t *)&value, sizeof(int64_t)).pdup();
		break;
	}
	case SQLITE_FLOAT: {
		double value = sqlite3_column_double((sqlite3_stmt *)result.get(), field);
		return BytesView((const uint8_t *)&value, sizeof(int64_t)).pdup();
		break;
	}
	case SQLITE_TEXT:
		return BytesView((const uint8_t *)sqlite3_column_text((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field));
		break;
	case SQLITE_BLOB:
		return BytesView((const uint8_t *)sqlite3_column_blob((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field));
		break;
	case SQLITE_NULL:
		return BytesView();
		break;
	}

	return BytesView();
}

int64_t ResultCursor::toInteger(size_t field) const {
	switch (sqlite3_column_type((sqlite3_stmt *)result.get(), field)) {
	case SQLITE_INTEGER:
		return sqlite3_column_int64((sqlite3_stmt *)result.get(), field);
		break;
	case SQLITE_FLOAT:
		return int64_t(sqlite3_column_double((sqlite3_stmt *)result.get(), field));
		break;
	case SQLITE_TEXT:
		return StringView((const char *)sqlite3_column_text((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field)).readInteger(10).get(0);
		break;
	case SQLITE_BLOB:
		return int64_t(BytesView((const uint8_t *)sqlite3_column_blob((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field)).readUnsigned64());
		break;
	case SQLITE_NULL:
		break;
	}

	return 0;
}

double ResultCursor::toDouble(size_t field) const {
	switch (sqlite3_column_type((sqlite3_stmt *)result.get(), field)) {
	case SQLITE_INTEGER:
		return double(sqlite3_column_int64((sqlite3_stmt *)result.get(), field));
		break;
	case SQLITE_FLOAT:
		return sqlite3_column_double((sqlite3_stmt *)result.get(), field);
		break;
	case SQLITE_TEXT:
		return StringView((const char *)sqlite3_column_text((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field)).readDouble().get(0);
		break;
	case SQLITE_BLOB:
		return BytesView((const uint8_t *)sqlite3_column_blob((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field)).readFloat64();
		break;
	case SQLITE_NULL:
		break;
	}

	return 0.0;
}
bool ResultCursor::toBool(size_t field) const {
	switch (sqlite3_column_type((sqlite3_stmt *)result.get(), field)) {
	case SQLITE_INTEGER:
		return sqlite3_column_int64((sqlite3_stmt *)result.get(), field) != 0;
		break;
	case SQLITE_FLOAT:
		return sqlite3_column_double((sqlite3_stmt *)result.get(), field) != 0.0;
		break;
	case SQLITE_TEXT: {
		StringView data((const char *)sqlite3_column_text((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field));
		if (data == "1" || data == "true" || data == "TRUE") {
			return true;
		}
		break;
	}
	case SQLITE_BLOB: {
		BytesView data((const uint8_t *)sqlite3_column_blob((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field));
		if (data.empty()) {
			return false;
		} else {
			return true;
		}
		break;
	}
	case SQLITE_NULL:
		break;
	}

	return false;
}
Value ResultCursor::toTypedData(size_t field) const {
	switch (sqlite3_column_type((sqlite3_stmt *)result.get(), field)) {
	case SQLITE_INTEGER:
		return Value(int64_t(sqlite3_column_int64((sqlite3_stmt *)result.get(), field)));
		break;
	case SQLITE_FLOAT:
		return Value(sqlite3_column_double((sqlite3_stmt *)result.get(), field));
		break;
	case SQLITE_TEXT:
		return Value(StringView((const char *)sqlite3_column_text((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field)));
		break;
	case SQLITE_BLOB:
		return Value(BytesView((const uint8_t *)sqlite3_column_blob((sqlite3_stmt *)result.get(), field),
				sqlite3_column_bytes((sqlite3_stmt *)result.get(), field)));
		break;
	case SQLITE_NULL:
		break;
	}

	return Value();
}
int64_t ResultCursor::toId() const {
	return toInteger(0);
}
StringView ResultCursor::getFieldName(size_t field) const {
	if (auto ptr = sqlite3_column_name((sqlite3_stmt *)result.get(), field)) {
		return StringView(ptr);
	}
	return StringView();
}
bool ResultCursor::isSuccess() const {
	return result.get() && statusIsSuccess(err);
}
bool ResultCursor::isEmpty() const {
	return err != SQLITE_ROW;
}
bool ResultCursor::isEnded() const {
	return err == SQLITE_DONE;
}
size_t ResultCursor::getFieldsCount() const {
	return sqlite3_column_count((sqlite3_stmt *)result.get());
}
size_t ResultCursor::getAffectedRows() const {
	return sqlite3_changes((sqlite3 *)conn.get());
}
size_t ResultCursor::getRowsHint() const {
	return 0;
}
Value ResultCursor::getInfo() const {
	return Value({
		stappler::pair("error", Value(err)),
		stappler::pair("status", Value(sqlite3_errstr(int(err)))),
		stappler::pair("desc", Value(sqlite3_errmsg((sqlite3 *)conn.get()))),
	});
}

bool ResultCursor::next() {
	if (err == SQLITE_ROW) {
		err = sqlite3_step((sqlite3_stmt *)result.get());
		return err == SQLITE_ROW;
	}
	return false;
}

void ResultCursor::reset() {
	if (result.get()) {
		sqlite3_reset((sqlite3_stmt *)result.get());
		err = sqlite3_step((sqlite3_stmt *)result.get());
		result = Driver::Result(nullptr);
	}
}

void ResultCursor::clear() {
	if (result.get()) {
		sqlite3_finalize((sqlite3_stmt *)result.get());
		result = Driver::Result(nullptr);
	}
}

int ResultCursor::getError() const {
	return err;
}

}
