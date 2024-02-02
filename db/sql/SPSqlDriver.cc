/**
Copyright (c) 2021-2022 Roman Katuntsev <sbkarr@stappler.org>
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

#include "SPSqlDriver.h"
#include "SPPqDriver.h"
#include "SPSqliteDriver.h"

namespace STAPPLER_VERSIONIZED stappler::db::sql {

Driver *Driver::open(pool_t *pool, ApplicationInterface *app, StringView path, const void *external) {
	Driver *ret = nullptr;
	pool::push(pool);
	if (path == "pgsql") {
		ret = pq::Driver::open(pool, app, StringView(), external);
	} else if (path.starts_with("pgsql:")) {
		path += "pgsql:"_len;
		ret = pq::Driver::open(pool, app, path, external);
	} else if (path == "sqlite" || path == "sqlite3") {
		ret = sqlite::Driver::open(pool, app, path);
	}

	registerCleanupDestructor(ret, pool);

	pool::pop();
	return ret;
}

Driver::~Driver() { }

void Driver::setDbCtrl(Function<void(bool)> &&fn) {
	_dbCtrl = std::move(fn);
}

const CustomFieldInfo *Driver::getCustomFieldInfo(StringView key) const {
	auto it = _customFields.find(key);
	if (it != _customFields.end()) {
		return &it->second;
	}
	return nullptr;
}

Driver::Driver(pool_t *p, ApplicationInterface *app)
: _pool(p), _application(app) {

}

}
