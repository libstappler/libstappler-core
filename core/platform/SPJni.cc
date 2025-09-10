/**
 Copyright (c) 2025 Stappler LLC <admin@stappler.dev>

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

#include "SPJni.h"
#include "SPStringView.h"

#if ANDROID

namespace STAPPLER_VERSIONIZED stappler::jni {

static JavaVM *s_vm = nullptr;
static int32_t s_sdk = 0;

struct JavaThread {
	JavaVM *vm = nullptr;
	JNIEnv *env = nullptr;
	bool attached = false;

	~JavaThread() {
		if (attached) {
			vm->DetachCurrentThread();
		}
	}

	bool init(JavaVM *v, JNIEnv *e, bool a) {
		vm = v;
		env = e;
		attached = a;
		return true;
	}
};

static thread_local JavaThread tl_thread;

Local::~Local() {
	if (_obj) {
		_env->DeleteLocalRef(_obj);
	}
}

Local::Local(jobject obj, JNIEnv *env) : _obj(obj), _env(env) { }

Local::Local(Local &&other) {
	_obj = other._obj;
	_env = other._env;
	other._obj = nullptr;
	other._env = nullptr;
}

Local &Local::operator=(Local &&other) {
	if (&other == this) {
		return *this;
	}

	if (_obj) {
		_env->DeleteLocalRef(_obj);
		_obj = nullptr;
	}

	_obj = other._obj;
	_env = other._env;
	other._obj = nullptr;
	other._env = nullptr;
	return *this;
}

Local::Local(std::nullptr_t) : _obj(nullptr), _env(nullptr) { }

Local &Local::operator=(std::nullptr_t) {
	if (_obj) {
		_env->DeleteLocalRef(_obj);
		_obj = nullptr;
	}

	_obj = nullptr;
	_env = nullptr;
	return *this;
}

Global Local::getGlobal() const { return Global(*this); }

LocalClass::LocalClass(jclass obj, JNIEnv *env) : Local(obj, env) { }

LocalClass::LocalClass(LocalClass &&other) : Local(nullptr) {
	_obj = other._obj;
	_env = other._env;
	other._obj = nullptr;
	other._env = nullptr;
}

LocalClass &LocalClass::operator=(LocalClass &&other) {
	if (&other == this) {
		return *this;
	}

	if (_obj) {
		_env->DeleteLocalRef(_obj);
		_obj = nullptr;
	}

	_obj = other._obj;
	_env = other._env;
	other._obj = nullptr;
	other._env = nullptr;
	return *this;
}

LocalClass::LocalClass(std::nullptr_t) : Local(nullptr) { }

LocalClass &LocalClass::operator=(std::nullptr_t) {
	if (_obj) {
		_env->DeleteLocalRef(_obj);
		_obj = nullptr;
	}

	_obj = nullptr;
	_env = nullptr;
	return *this;
}


GlobalClass LocalClass::getGlobal() const { return GlobalClass(*this); }

Global::~Global() {
	if (_obj) {
		Env::getEnv().deleteGlobalRef(_obj);
		_obj = nullptr;
	}
}

Global::Global(const Local &obj) {
	if (obj) {
		_obj = obj.getEnv()->NewGlobalRef(obj);
	}
}

Global::Global(const Ref &obj) {
	if (obj) {
		_obj = obj.getEnv()->NewGlobalRef(obj);
	}
}

Global::Global(const Global &other) {
	if (other._obj) {
		_obj = Env::getEnv().newGlobalRef(other._obj);
	}
}

Global &Global::operator=(const Global &other) {
	if (&other == this) {
		return *this;
	}

	auto env = Env::getEnv();

	if (_obj) {
		env.deleteGlobalRef(_obj);
		_obj = nullptr;
	}

	if (other._obj) {
		_obj = env.newGlobalRef(other._obj);
	}
	return *this;
}

Global::Global(Global &&other) {
	_obj = other._obj;
	other._obj = nullptr;
}

Global &Global::operator=(Global &&other) {
	if (&other == this) {
		return *this;
	}

	if (_obj) {
		Env::getEnv().deleteGlobalRef(_obj);
		_obj = nullptr;
	}

	_obj = other._obj;
	other._obj = nullptr;
	return *this;
}

Global::Global(std::nullptr_t) : _obj(nullptr) { }

Global &Global::operator=(std::nullptr_t) {
	if (_obj) {
		Env::getEnv().deleteGlobalRef(_obj);
		_obj = nullptr;
	}

	_obj = nullptr;
	return *this;
}

JNIEnv *Global::getEnv() const { return Env::getEnv(); }

LocalString::~LocalString() { reset(); }

LocalString::LocalString(jstring obj, JNIEnv *env) : Local(obj, env) { }

LocalString::LocalString(LocalString &&other) : Local(move(other)) { }

LocalString &LocalString::operator=(LocalString &&other) {
	if (&other == this) {
		return *this;
	}

	reset();
	Local::operator=(move(other));
	return *this;
}

LocalString::LocalString(std::nullptr_t) : Local(nullptr) { }

LocalString &LocalString::operator=(std::nullptr_t) {
	reset();

	Local::operator=(nullptr);
	return *this;
}

GlobalString LocalString::getGlobal() const { return GlobalString(*this); }

GlobalString::GlobalString(const LocalString &obj) : Global(nullptr) {
	if (obj) {
		_obj = obj.getEnv()->NewGlobalRef(obj);
	}
}

GlobalString::GlobalString(const RefString &obj) : Global(nullptr) {
	if (obj) {
		_obj = obj.getEnv()->NewGlobalRef(obj);
	}
}

GlobalString::GlobalString(const GlobalString &other) : Global(other) { }

GlobalString &GlobalString::operator=(const GlobalString &other) {
	if (&other == this) {
		return *this;
	}

	Global::operator=(other);
	return *this;
}

GlobalString::GlobalString(GlobalString &&other) : Global(move(other)) { }

GlobalString &GlobalString::operator=(GlobalString &&other) {
	if (&other == this) {
		return *this;
	}

	Global::operator=(move(other));
	return *this;
}

GlobalString::GlobalString(std::nullptr_t) : Global(nullptr) { }

GlobalString &GlobalString::operator=(std::nullptr_t) {
	Global::operator=(nullptr);
	return *this;
}

GlobalClass::GlobalClass(const LocalClass &obj) : Global(nullptr) {
	if (obj) {
		_obj = obj.getEnv()->NewGlobalRef(obj);
	}
}

GlobalClass::GlobalClass(const RefClass &obj) : Global(nullptr) {
	if (obj) {
		_obj = obj.getEnv()->NewGlobalRef(obj);
	}
}

GlobalClass::GlobalClass(const GlobalClass &other) : Global(other) { }

GlobalClass &GlobalClass::operator=(const GlobalClass &other) {
	if (&other == this) {
		return *this;
	}

	Global::operator=(other);
	return *this;
}

GlobalClass::GlobalClass(GlobalClass &&other) : Global(move(other)) { }

GlobalClass &GlobalClass::operator=(GlobalClass &&other) {
	if (&other == this) {
		return *this;
	}

	Global::operator=(move(other));
	return *this;
}

GlobalClass::GlobalClass(std::nullptr_t) : Global(nullptr) { }

GlobalClass &GlobalClass::operator=(std::nullptr_t) {
	Global::operator=(nullptr);
	return *this;
}

Global Ref::getGlobal() const { return Global(*this); }

RefString::~RefString() { reset(); }

GlobalString RefString::getGlobal() const { return GlobalString(*this); }

GlobalClass RefClass::getGlobal() const { return GlobalClass(*this); }

static JNIEnv *getVmEnv() {
	void *ret = nullptr;
	if (s_vm) {
		s_vm->GetEnv(&ret, JNI_VERSION_1_6);
	} else {
		log::source().error("JNI", "JavaVM not found");
	}
	return reinterpret_cast<JNIEnv *>(ret);
}

Env Env::getEnv() {
	if (!tl_thread.env) {
		auto env = getVmEnv();
		if (env) {
			tl_thread.init(s_vm, env, false);
		} else {
			s_vm->AttachCurrentThread(&env, nullptr);
			if (env) {
				tl_thread.init(s_vm, env, true);
			}
		}
	}
	return tl_thread.env;
}

int32_t Env::getSdkVersion() { return s_sdk; }

void Env::loadJava(JavaVM *vm, int32_t sdk) {
	s_vm = vm;
	s_sdk = sdk;

	platform::i18n::load(vm, sdk);
}

void Env::finalizeJava() {
	s_vm = nullptr;

	platform::i18n::finalize();
}

void Env::checkErrors() const {
	if (_env->ExceptionCheck()) {
		// Read exception msg
		auto e = Local(_env->ExceptionOccurred(), _env);
		_env->ExceptionClear(); // clears the exception; e seems to remain valid

		auto clazz = e.getClass();
		auto classClass = clazz.getClass();
		jmethodID getName = classClass.getMethodID("getName", "()Ljava/lang/String;");
		jmethodID getMessage = clazz.getMethodID("getMessage", "()Ljava/lang/String;");

		auto message = e.callMethod<jstring>(getMessage);
		auto exName = clazz.callMethod<jstring>(getName);

		log::source().error("JNI", "[", exName.getString(), "] ", message.getString());
	}
}

} // namespace stappler::jni

#endif
