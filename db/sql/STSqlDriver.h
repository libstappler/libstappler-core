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

#ifndef STAPPLER_DB_SQL_STSQLDRIVER_H_
#define STAPPLER_DB_SQL_STSQLDRIVER_H_

#include "STStorageBackendInterface.h"
#include "STStorageField.h"

namespace stappler::db::sql {

class Driver : public AllocBase {
public:
	using Handle = stappler::ValueWrapper<void *, class HandleClass>;
	using Result = stappler::ValueWrapper<void *, class ResultClass>;
	using Connection = stappler::ValueWrapper<void *, class ConnectionClass>;

	static Driver *open(pool_t *, StringView path = StringView(), const void *external = nullptr);

	virtual ~Driver();

	StringView getDriverName() const { return _driverPath; }

	virtual bool init(Handle handle, const Vector<StringView> &) = 0;

	virtual void performWithStorage(Handle handle, const Callback<void(const db::Adapter &)> &cb) const = 0;
	virtual BackendInterface *acquireInterface(Handle handle, pool_t *) const = 0;

	virtual Handle connect(const Map<StringView, StringView> &) const = 0;
	virtual void finish(Handle) const = 0;

	virtual Connection getConnection(Handle h) const = 0;

	virtual bool isValid(Handle) const = 0;
	virtual bool isValid(Connection) const = 0;
	virtual bool isIdle(Connection) const = 0;

	virtual Time getConnectionTime(Handle) const = 0;

	virtual int listenForNotifications(Handle) const { return -1; }
	virtual bool consumeNotifications(Handle, const Callback<void(StringView)> &) const { return true; }

	virtual bool isNotificationsSupported() const { return false; }

	void setDbCtrl(Function<void(bool)> &&);

protected:
	StringView _driverPath;
	Function<void(bool)> _dbCtrl = nullptr;
};

}

#endif /* STAPPLER_DB_SQL_STSQLDRIVER_H_ */
