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

#include "STStorage.h"
#include "STStorageAdapter.h"
#include "STStorageTransaction.h"
#include "SPFilesystem.h"

namespace stappler::db {

bool StorageRoot::isDebugEnabled() const {
	return _debug;
}

void StorageRoot::setDebugEnabled(bool v) {
	_debug = v;
}

void StorageRoot::addErrorMessage(Value &&data) const {
	_debugMutex.lock();
	std::cout << "[Error]: " << stappler::data::EncodeFormat::Pretty << data << "\n";
	_debugMutex.unlock();
}

void StorageRoot::addDebugMessage(Value &&data) const {
	_debugMutex.lock();
	std::cout << "[Debug]: " << stappler::data::EncodeFormat::Pretty << data << "\n";
	_debugMutex.unlock();
}

void StorageRoot::broadcast(const Value &val) {
	if (val.getBool("local")) {
		onLocalBroadcast(val);
	} else {
		if (auto a = db::Adapter::FromContext()) {
			a.broadcast(val);
		}
	}
}

void StorageRoot::broadcast(const Bytes &val) {
	if (auto a = db::Adapter::FromContext()) {
		a.broadcast(val);
	}
}

Transaction StorageRoot::acquireTransaction(const Adapter &adapter) {
	if (auto pool = stappler::memory::pool::acquire()) {
		if (auto d = stappler::memory::pool::get<Transaction::Data>(pool, adapter.getTransactionKey())) {
			auto ret = Transaction(d);
			ret.retain();
			return ret;
		} else {
			d = new (pool) Transaction::Data{adapter};
			d->role = AccessRoleId::System;
			stappler::memory::pool::store(pool, d, adapter.getTransactionKey());
			auto ret = Transaction(d);
			ret.retain();

			onStorageTransaction(ret);

			return ret;
		}
	}
	return Transaction(nullptr);
}

Adapter StorageRoot::getAdapterFromContext() {
	if (auto p = pool::acquire()) {
		BackendInterface *h = nullptr;
		stappler::memory::pool::userdata_get((void **)&h, config::getStorageInterfaceKey(), p);
		if (h) {
			return Adapter(h);
		}
	}
	return Adapter(nullptr);
}

void StorageRoot::scheduleAyncDbTask(const stappler::Callback<Function<void(const Transaction &)>(stappler::memory::pool_t *)> &setupCb) {
	/*if (auto serv = stellator::server()) {
		stellator::Task::perform(serv, [&] (stellator::Task &task) {
			auto cb = setupCb(task.pool());
			task.addExecuteFn([cb = std::move(cb)] (const stellator::Task &task) -> bool {
				task.performWithStorage([&] (const Transaction &t) {
					t.performAsSystem([&] () -> bool {
						cb(t);
						return true;
					});
				});
				return true;
			});
		});
	}*/
}

bool StorageRoot::isAdministrative() const {
	return true;
}

String StorageRoot::getDocuemntRoot() const {
	return filesystem::writablePath<Interface>();
}

const Scheme *StorageRoot::getFileScheme() const {
	// return stellator::server().getFileScheme();
	return nullptr;
}

const Scheme *StorageRoot::getUserScheme() const {
	// return stellator::server().getUserScheme();
	return nullptr;
}

InputFile *StorageRoot::getFileFromContext(int64_t) const {
	return nullptr;
}

internals::RequestData StorageRoot::getRequestData() const {
	return internals::RequestData();
}

int64_t StorageRoot::getUserIdFromContext() const {
	return 0;
}

static StorageRoot defaultRoot;
static StorageRoot *s_root = &defaultRoot;

void setStorageRoot(StorageRoot *root) {
	SPASSERT(s_root == &defaultRoot, "Root redefinition is forbidden");
	s_root = root;
}

namespace messages {

bool isDebugEnabled() {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->isDebugEnabled();
}

void setDebugEnabled(bool v) {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->setDebugEnabled(v);
}

void _addErrorMessage(Value &&data) {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->addErrorMessage(std::move(data));
}

void _addDebugMessage(Value &&data) {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->addDebugMessage(std::move(data));
}

void broadcast(const Value &val) {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->broadcast(val);
}

void broadcast(const Bytes &val) {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->broadcast(val);
}

}

Transaction Transaction::acquire(const Adapter &adapter) {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->acquireTransaction(adapter);
}

namespace internals {

Adapter getAdapterFromContext() {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->getAdapterFromContext();
}

void scheduleAyncDbTask(const stappler::Callback<Function<void(const Transaction &)>(stappler::memory::pool_t *)> &setupCb) {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->scheduleAyncDbTask(setupCb);
}

bool isAdministrative() {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->isAdministrative();
}

String getDocuemntRoot() {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->getDocuemntRoot();
}

const Scheme *getFileScheme() {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->getFileScheme();
}

const Scheme *getUserScheme() {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->getUserScheme();
}

InputFile *getFileFromContext(int64_t id) {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->getFileFromContext(id);
}

RequestData getRequestData() {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->getRequestData();
}

int64_t getUserIdFromContext() {
	SPASSERT(s_root, "Root should be defined before any stellator storage ops");
	return s_root->getUserIdFromContext();
}

}

}
