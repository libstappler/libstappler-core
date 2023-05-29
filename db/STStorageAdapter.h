/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#ifndef STAPPLER_DB_STSTORAGEADAPTER_H_
#define STAPPLER_DB_STSTORAGEADAPTER_H_

#include "STStorageBackendInterface.h"

namespace stappler::db {

class Adapter final : public AllocBase {
public:
	static Adapter FromContext();

	Adapter(BackendInterface *);

	Adapter(const Adapter &);
	Adapter& operator=(const Adapter &);

	operator bool () const { return _interface != nullptr; }

	bool operator == (const Adapter &a) const { return _interface == a._interface; }

	BackendInterface *interface() const;

	String getTransactionKey() const;

public: // key-value storage
	bool set(const stappler::CoderSource &, const Value &, stappler::TimeInterval = config::getKeyValueStorageTime()) const;
	Value get(const stappler::CoderSource &) const;
	bool clear(const stappler::CoderSource &) const;

public:
	bool init(const BackendInterface::Config &cfg, const Map<StringView, const Scheme *> &) const;

	void makeSessionsCleanup() const;

	User * authorizeUser(const Auth &, const StringView &name, const StringView &password) const;

	void broadcast(const Bytes &) const;
	void broadcast(const Value &val) const;
	void broadcast(StringView url, Value &&val, bool exclusive) const;

	bool performWithTransaction(const Callback<bool(const db::Transaction &)> &cb) const;

	Vector<int64_t> getReferenceParents(const Scheme &, uint64_t oid, const Scheme *, const Field *) const;

	StringView getDatabaseName() const { return _interface->getDatabaseName(); }

protected:
	friend class Transaction;

	int64_t getDeltaValue(const Scheme &); // scheme-based delta
	int64_t getDeltaValue(const Scheme &, const FieldView &, uint64_t); // view-based delta

	Vector<int64_t> performQueryListForIds(const QueryList &, size_t count = stappler::maxOf<size_t>()) const;
	Value performQueryList(const QueryList &, size_t count = stappler::maxOf<size_t>(), bool forUpdate = false) const;

	bool foreach(Worker &, const Query &, const Callback<bool(Value &)> &) const;
	Value select(Worker &, const Query &) const;

	Value create(Worker &, Value &) const;
	Value save(Worker &, uint64_t oid, const Value &obj, const Vector<String> &fields) const;
	Value patch(Worker &, uint64_t oid, const Value &patch) const;

	bool remove(Worker &, uint64_t oid) const;

	size_t count(Worker &, const Query &) const;

	void scheduleAutoField(const Scheme &, const Field &, uint64_t id);

protected:
	Value field(Action, Worker &, uint64_t oid, const Field &, Value && = Value()) const;
	Value field(Action, Worker &, const Value &, const Field &, Value && = Value()) const;

	bool addToView(const FieldView &, const Scheme *, uint64_t oid, const Value &) const;
	bool removeFromView(const FieldView &, const Scheme *, uint64_t oid) const;

	bool beginTransaction() const;
	bool endTransaction() const;

	void cancelTransaction() const;
	bool isInTransaction() const;
	TransactionStatus getTransactionStatus() const;

	void runAutoFields(const Transaction &t, const Vector<uint64_t> &vec, const Scheme &, const Field &);

protected:
	BackendInterface *_interface;
};

}

#endif /* STAPPLER_DB_STSTORAGEADAPTER_H_ */
