/**
Copyright (c) 2019-2022 Roman Katuntsev <sbkarr@stappler.org>
Copyright (c) 2023-2024 Stappler LLC <admin@stappler.dev>

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

#ifndef STAPPLER_DB_PQ_SPDBPQDRIVER_H_
#define STAPPLER_DB_PQ_SPDBPQDRIVER_H_

#include "SPSqlDriver.h"

namespace STAPPLER_VERSIONIZED stappler::db::pq {

struct DriverSym;

class SP_PUBLIC Driver : public sql::Driver {
public:
	enum class Status {
		Empty = 0,
		CommandOk,
		TuplesOk,
		CopyOut,
		CopyIn,
		BadResponse,
		NonfatalError,
		FatalError,
		CopyBoth,
		SingleTuple,
	};

	enum class TransactionStatus {
		Idle,
		Active,
		InTrans,
		InError,
		Unknown
	};

	static Driver *open(pool_t *, ApplicationInterface *, StringView path = StringView(), const void *external = nullptr);

	virtual ~Driver();

	virtual bool init(Handle handle, const Vector<StringView> &) override;

	virtual void performWithStorage(Handle handle, const Callback<void(const db::Adapter &)> &cb) const override;
	virtual BackendInterface *acquireInterface(Handle handle, pool_t *) const override;

	virtual Handle connect(const Map<StringView, StringView> &) const override;
	virtual void finish(Handle) const override;

	virtual Connection getConnection(Handle h) const override;

	virtual bool isValid(Handle) const override;
	virtual bool isValid(Connection) const override;
	virtual bool isIdle(Connection) const override;

	virtual Time getConnectionTime(Handle) const override;

	virtual int listenForNotifications(Handle) const override;
	virtual bool consumeNotifications(Handle, const Callback<void(StringView)> &) const override;

	virtual bool isNotificationsSupported() const override { return true; }

	TransactionStatus getTransactionStatus(Connection) const;

	Status getStatus(Result res) const;

	bool isBinaryFormat(Result res, size_t field) const;
	bool isNull(Result res, size_t row, size_t field) const;
	char *getValue(Result res, size_t row, size_t field) const;
	size_t getLength(Result res, size_t row, size_t field) const;

	char *getName(Result res, size_t field) const;
	unsigned int getType(Result res, size_t field) const;

	size_t getNTuples(Result res) const;
	size_t getNFields(Result res) const;
	size_t getCmdTuples(Result res) const;

	char *getStatusMessage(Status) const;
	char *getResultErrorMessage(Result res) const;

	void clearResult(Result res) const;

	Result exec(Connection conn, const char *query) const;
	Result exec(Connection conn, const char *command, int nParams, const char *const *paramValues,
			const int *paramLengths, const int *paramFormats, int resultFormat) const;

	explicit operator bool () const { return _handle != nullptr; }

	BackendInterface::StorageType getTypeById(uint32_t) const;
	StringView getTypeNameById(uint32_t) const;

protected:
	Driver(pool_t *pool, ApplicationInterface *, StringView, const void *external);

	Handle doConnect(const char * const *keywords, const char * const *values, int expand_dbname) const;

	bool _init = false;

	Vector<Pair<uint32_t, BackendInterface::StorageType>> _storageTypes;
	Vector<Pair<uint32_t, String>> _customTypes;

	DriverSym *_handle = nullptr;
	const void *_external = nullptr;
};

class SP_PUBLIC ResultCursor final : public db::ResultCursor {
public:
	inline static constexpr bool pgsql_is_success(Driver::Status x) {
		return (x == Driver::Status::Empty) || (x == Driver::Status::CommandOk) || (x == Driver::Status::TuplesOk) || (x == Driver::Status::SingleTuple);
	}

	ResultCursor(const Driver *d, Driver::Result res);

	virtual ~ResultCursor();
	virtual bool isBinaryFormat(size_t field) const override;
	virtual bool isNull(size_t field) const override;
	virtual StringView toString(size_t field) const override;
	virtual BytesView toBytes(size_t field) const override;
	virtual int64_t toInteger(size_t field) const override;
	virtual double toDouble(size_t field) const override;
	virtual bool toBool(size_t field) const override;
	virtual Value toTypedData(size_t field) const override;
	virtual Value toCustomData(size_t field, const FieldCustom *) const override;
	virtual int64_t toId() const override;
	virtual StringView getFieldName(size_t field) const override;
	virtual Value getInfo() const override;
	virtual void clear() override;
	Driver::Status getError() const;

	virtual bool isSuccess() const override;
	virtual bool isEmpty() const override;
	virtual bool isEnded() const override;
	virtual size_t getFieldsCount() const override;
	virtual size_t getAffectedRows() const override;
	virtual size_t getRowsHint() const override;

	virtual bool next() override;
	virtual void reset() override;

public:
	const Driver *driver = nullptr;
	Driver::Result result = Driver::Result(nullptr);
	size_t nrows = 0;
	size_t currentRow = 0;
	Driver::Status err = Driver::Status::Empty;
};

}

#endif /* STAPPLER_DB_PQ_SPDBPQDRIVER_H_ */
