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

#ifndef STAPPLER_DB_SQLITE_STSQLITEHANDLE_H_
#define STAPPLER_DB_SQLITE_STSQLITEHANDLE_H_

#include "STSqlHandle.h"
#include "STSqliteDriver.h"
#include "STStorageScheme.h"
#include "SPSql.h"

namespace stappler::db::sqlite {

enum class TransactionLevel {
	Deferred,
	Immediate,
	Exclusive,
};

class Handle : public db::sql::SqlHandle {
public:
	Handle(const Driver *, Driver::Handle);

	Handle(const Handle &) = delete;
	Handle &operator=(const Handle &) = delete;

	Handle(Handle &&);
	Handle &operator=(Handle &&);

	operator bool () const;

	const Driver *getDriver() const { return driver; }
	Driver::Handle getHandle() const;
	Driver::Connection getConnection() const;

	virtual void makeQuery(const Callback<void(sql::SqlQuery &)> &cb) override;

	virtual bool selectQuery(const db::sql::SqlQuery &, const Callback<bool(sql::Result &)> &cb,
			const Callback<void(const Value &)> &err = nullptr) override;
	virtual bool performSimpleQuery(const StringView &,
			const Callback<void(const Value &)> &err = nullptr) override;
	virtual bool performSimpleSelect(const StringView &, const Callback<void(sql::Result &)> &cb,
			const Callback<void(const Value &)> &err = nullptr) override;

	virtual bool isSuccess() const override;

	void close();

public: // adapter interface
	virtual bool init(const BackendInterface::Config &cfg, const Map<StringView, const Scheme *> &) override;

protected:
	virtual bool beginTransaction() override;
	virtual bool endTransaction() override;

	using ViewIdVec = Vector<Pair<const Scheme::ViewScheme *, int64_t>>;

	const Driver *driver = nullptr;
	Driver::Handle handle = Driver::Handle(nullptr);
	Driver::Connection conn = Driver::Connection(nullptr);
	int lastError = 0;
	TransactionLevel level = TransactionLevel::Deferred;
	stappler::sql::Profile _profile = stappler::sql::Profile::Postgres;
};

}

#endif /* STAPPLER_DB_SQLITE_STSQLITEHANDLE_H_ */
