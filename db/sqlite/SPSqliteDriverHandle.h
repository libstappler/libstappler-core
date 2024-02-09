/**
 Copyright (c) 2024 Stappler LLC <admin@stappler.dev>

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

#ifndef CORE_DB_SQLITE_SPSQLITEDRIVERHANDLE_H_
#define CORE_DB_SQLITE_SPSQLITEDRIVERHANDLE_H_

#include "SPSqliteDriver.h"
#include "sqlite3.h"

namespace STAPPLER_VERSIONIZED stappler::db::sqlite {

struct DriverHandle {
	sqlite3 *conn;
	const Driver *driver;
	void *padding;
	pool_t *pool;
	StringView name;
	sqlite3_stmt *oidQuery = nullptr;
	sqlite3_stmt *wordsQuery = nullptr;
	int64_t userId = 0;
	Time ctime;
	std::mutex mutex;
};

struct TextQueryData : AllocBase {
	const FullTextQuery *query;
	Vector<uint64_t> pos;
	Vector<uint64_t> neg;
};

static StringView Driver_exec(pool_t *p, sqlite3 *db, StringView query);

}

#endif /* CORE_DB_SQLITE_SPSQLITEDRIVERHANDLE_H_ */
