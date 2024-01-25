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

#ifndef STAPPLER_DB_STSTORAGE_H_
#define STAPPLER_DB_STSTORAGE_H_

#include "SPMemUserData.h"
#include "SPMemory.h"
#include "SPData.h"
#include "STStorageConfig.h"

namespace stappler::db {

using namespace mem_pool;

struct InputFile;

class Adapter;
class Transaction;
class Worker;

class Query;
class BackendInterface;
class Binder;
class QueryInterface;
class ResultCursor;

class Scheme;
class Field;
class Object;
class User;

struct FieldText;
struct FieldPassword;
struct FieldExtra;
struct FieldFile;
struct FieldImage;
struct FieldObject;
struct FieldArray;
struct FieldView;
struct FieldFullTextView;
struct FieldCustom;

namespace internals {

struct RequestData {
	bool exists = false;
	StringView address;
	StringView hostname;
	StringView uri;

	operator bool() { return exists; }
};

Adapter getAdapterFromContext();

void scheduleAyncDbTask(const Callback<Function<void(const Transaction &)>(pool_t *)> &setupCb);

bool isAdministrative();

String getDocuemntRoot();

const Scheme *getFileScheme();
const Scheme *getUserScheme();

InputFile *getFileFromContext(int64_t);

RequestData getRequestData();
int64_t getUserIdFromContext();

}

namespace messages {

/* Serenity cross-server messaging interface
 *
 * error(Tag, Name, Data = nullptr)
 *  - send information about error to current output interface,
 *  if debug interface is active, it will receive this message
 *
 * debug(Tag, Name, Data = nullptr)
 *  - send debug information to current output interface and
 *  connected debug interface
 *
 * broadcast(Message)
 *  - send cross-server broadcast with message
 *
 *  Broadcast format:
 *  (message) ->  { "message": true, "level": "debug"/"error", "data": Data }
 *   - used for system errors, warnings and debug, received by admin shell
 *
 *  (websocket) -> { "server": "SERVER_NAME", "url": "WEBSOCKET_URL", "data": Data }
 *   - used for communication between websockets, received by all websockets with the same server id and url
 *
 *  (option) -> { "system": true, "option": "OPTION_NAME", "OPTION_NAME": NewValue }
 *   - used for system-wide option switch, received only by system
 *
 *   NOTE: All broadcast messages coded as CBOR, so, BYTESTRING type is safe
 */

bool isDebugEnabled();
void setDebugEnabled(bool);

void _addErrorMessage(Value &&);
void _addDebugMessage(Value &&);

void broadcast(const Value &);
void broadcast(const Bytes &);

template <typename Source, typename Text>
void _addError(Source &&source, Text &&text) {
	_addErrorMessage(Value{
		std::make_pair("source", Value(std::forward<Source>(source))),
		std::make_pair("text", Value(std::forward<Text>(text)))
	});
}

template <typename Source, typename Text>
void _addError(Source &&source, Text &&text, Value &&d) {
	_addErrorMessage(Value{
		std::make_pair("source", Value(std::forward<Source>(source))),
		std::make_pair("text", Value(std::forward<Text>(text))),
		std::make_pair("data", std::move(d))
	});
}

template <typename Source, typename Text>
void _addDebug(Source &&source, Text &&text) {
	_addDebugMessage(Value{
		std::make_pair("source", Value(std::forward<Source>(source))),
		std::make_pair("text", Value(std::forward<Text>(text)))
	});
}

template <typename Source, typename Text>
void _addDebug(Source &&source, Text &&text, Value &&d) {
	_addDebugMessage(Value{
		std::make_pair("source", Value(std::forward<Source>(source))),
		std::make_pair("text", Value(std::forward<Text>(text))),
		std::make_pair("data", std::move(d))
	});
}

template <typename Source, typename Text>
void _addLocal(Source &&source, Text &&text) {
	broadcast(Value{
		std::make_pair("local", Value(true)),
		std::make_pair("message", Value(true)),
		std::make_pair("data", Value({
			std::make_pair("source", Value(std::forward<Source>(source))),
			std::make_pair("text", Value(std::forward<Text>(text)))
		})),
	});
}

template <typename Source, typename Text>
void _addLocal(Source &&source, Text &&text, Value &&d) {
	broadcast(Value{
		std::make_pair("local", Value(true)),
		std::make_pair("message", Value(true)),
		std::make_pair("data", Value({
			std::make_pair("source", Value(std::forward<Source>(source))),
			std::make_pair("text", Value(std::forward<Text>(text))),
			std::make_pair("data", std::move(d))
		})),
	});
}

template <typename ... Args>
void error(Args && ...args) {
	_addError(std::forward<Args>(args)...);
}

template <typename ... Args>
void debug(Args && ...args) {
	_addDebug(std::forward<Args>(args)...);
}

template <typename ... Args>
void local(Args && ...args) {
	_addLocal(std::forward<Args>(args)...);
}

}

// Default StorageRoot class is partially operational, requires customization for real systems
/*class StorageRoot {
public:
	virtual ~StorageRoot() { }

	virtual bool isDebugEnabled() const;
	virtual void setDebugEnabled(bool v);

	virtual void addErrorMessage(Value &&data) const;
	virtual void addDebugMessage(Value &&data) const;

	virtual void broadcast(const Value &val);
	virtual void broadcast(const Bytes &val);

	virtual Transaction acquireTransaction(const Adapter &adapter);
	virtual Adapter getAdapterFromContext();

	// Argument is a callback (function, that should not be stored), that return
	// 	 function (that can and will be stored), allocated from specified memory pool;
	// returned function should be called within async thread with usable db::Transaction to do its work
	virtual void scheduleAyncDbTask(
			const Callback<Function<void(const Transaction &)>(pool_t *)> &setupCb);

	virtual bool isAdministrative() const;
	virtual String getDocuemntRoot() const;
	virtual const Scheme *getFileScheme() const;
	virtual const Scheme *getUserScheme() const;
	virtual InputFile *getFileFromContext(int64_t) const;
	virtual internals::RequestData getRequestData() const;
	virtual int64_t getUserIdFromContext() const;

protected:
	virtual void onLocalBroadcast(const Value &) { }
	virtual void onStorageTransaction(Transaction &) { }

	mutable std::mutex _debugMutex;
	std::atomic<bool> _debug = false;
};
*/

struct InputConfig {
	enum class Require {
		None = 0,
		Data = 1,
		Files = 2,
		Body = 4,
		FilesAsData = 8,
	};

	void updateLimits(const Map<String, Field> &vec);

	Require required = Require::None;
	size_t maxRequestSize = config::getMaxRequestSize();
	size_t maxVarSize = config::getMaxVarSize();
	size_t maxFileSize = config::getMaxFileSize();

	TimeInterval updateTime = config::getInputUpdateTime();
	float updateFrequency = config::getInputUpdateFrequency();
};

SP_DEFINE_ENUM_AS_MASK(InputConfig::Require);

}

#endif /* STAPPLER_DB_STSTORAGE_H_ */
