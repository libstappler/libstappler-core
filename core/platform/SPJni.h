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

#ifndef CORE_CORE_PLATFORM_SPCOREJNI_H_
#define CORE_CORE_PLATFORM_SPCOREJNI_H_

#include "SPCore.h"

#if ANDROID

#include <jni.h>

namespace STAPPLER_VERSIONIZED stappler::jni {

class Local;
class LocalClass;
class LocalString;
class Global;
class GlobalClass;
class GlobalString;
class Ref;
class RefClass;
class RefString;

template <typename ReturnType>
struct CallMethod;

template <typename ReturnType>
struct CallStaticMethod;

template <typename ReturnType>
struct GetField;

template <typename ReturnType>
struct GetStaticField;

template <typename Origin>
class SP_PUBLIC ObjectInterface {
public:
	template <typename Type>
	auto getField(jfieldID id) const -> typename GetField<Type>::Result;

	template <typename Type, typename ... Args>
	auto callMethod(jmethodID methodID, Args && ... args) const -> typename CallMethod<Type>::Result;

	LocalClass getClass() const;

	LocalString getClassName() const;

private:
    JNIEnv *getObjectEnv() const;
    jobject getObject() const;
};

template <typename Origin>
class SP_PUBLIC ClassInterface {
public:
	jmethodID getMethodID(const char *name, const char *sig) const;
	jmethodID getStaticMethodID(const char *name, const char *sig) const;

	jfieldID getFieldID(const char *name, const char *sig) const;

	jfieldID getStaticFieldID(const char *name, const char *sig) const;

	template <typename Type>
	auto getStaticField(jfieldID id) const -> typename GetStaticField<Type>::Result;

	template <typename Type>
	auto getStaticField(const char *name, const char *sig = nullptr) const -> typename GetStaticField<Type>::Result;

	template <typename Type, typename ... Args>
	auto callStaticMethod(jmethodID methodID, Args && ... args) const -> typename CallStaticMethod<Type>::Result;

    jint registerNatives(const JNINativeMethod* methods, jint nMethods);
    jint unregisterNatives();

private:
    JNIEnv *getClassEnv() const;
    jclass getInterfaceClass() const;
};

template <typename Origin>
class SP_PUBLIC StringInterface {
public:
	WideStringView getWideString();
	StringView getString();

	void reset();

private:
    JNIEnv *getStringEnv() const;
    jstring getInterfaceString() const;

protected:
	jboolean _isCopy = 0;
	jboolean _utfIsCopy = 0;
	const jchar *_chars = nullptr;
	const char *_utfChar = nullptr;
};

class SP_PUBLIC Local : public ObjectInterface<Local> {
public:
	~Local();
	Local(jobject obj, JNIEnv *env);

	Local(const Local &) = delete;
	Local & operator=(const Local &) = delete;

	Local(Local &&other);
	Local & operator=(Local &&);

	Local(std::nullptr_t);
	Local & operator=(std::nullptr_t);

	Local() : Local(nullptr) { }

	jobject get() const { return _obj; }
	operator jobject() const { return get(); }
	explicit operator bool() const { return _obj != nullptr; }

	JNIEnv *getEnv() const { return _env; }

	Global getGlobal() const;

protected:
	jobject _obj = nullptr;
	JNIEnv *_env = nullptr;
};

class SP_PUBLIC LocalString : public Local, public StringInterface<LocalString> {
public:
	~LocalString();
	LocalString(jstring obj, JNIEnv *env);

	LocalString(const LocalString &) = delete;
	LocalString & operator=(const LocalString &) = delete;

	LocalString(LocalString &&other);
	LocalString & operator=(LocalString &&);

	LocalString(std::nullptr_t);
	LocalString & operator=(std::nullptr_t);

	LocalString() : LocalString(nullptr) { }

	jstring get() const { return static_cast<jstring>(_obj); }
	operator jstring() const { return get(); }

	GlobalString getGlobal() const;
};

class SP_PUBLIC LocalClass : public Local, public ClassInterface<LocalClass> {
public:
	LocalClass(jclass obj, JNIEnv *env);

	LocalClass(const LocalClass &) = delete;
	LocalClass & operator=(const LocalClass &) = delete;

	LocalClass(LocalClass &&other);
	LocalClass & operator=(LocalClass &&);

	LocalClass(std::nullptr_t);
	LocalClass & operator=(std::nullptr_t);

	LocalClass() : LocalClass(nullptr) { }

	jclass get() const { return static_cast<jclass>(_obj); }
	operator jclass() const { return get(); }

	GlobalClass getGlobal() const;
};

class SP_PUBLIC Global : public ObjectInterface<Global> {
public:
	~Global();
	Global(const Local &obj);
	Global(const Ref &obj);

	Global(const Global &);
	Global & operator=(const Global &);

	Global(Global &&other);
	Global & operator=(Global &&);

	Global(std::nullptr_t);
	Global & operator=(std::nullptr_t);

	jobject get() const { return _obj; }
	operator jobject() const { return get(); }
	explicit operator bool() const { return _obj != nullptr; }

	JNIEnv *getEnv() const;

protected:
	jobject _obj = nullptr;
};

class SP_PUBLIC GlobalString : public Global {
public:
	GlobalString(const LocalString &obj);
	GlobalString(const RefString &obj);

	GlobalString(const GlobalString &);
	GlobalString & operator=(const GlobalString &);

	GlobalString(GlobalString &&other);
	GlobalString & operator=(GlobalString &&);

	GlobalString(std::nullptr_t);
	GlobalString & operator=(std::nullptr_t);

	jstring get() const { return static_cast<jstring>(_obj); }
	operator jstring() const { return get(); }
};

class SP_PUBLIC GlobalClass : public Global, public ClassInterface<GlobalClass> {
public:
	GlobalClass(const LocalClass &obj);
	GlobalClass(const RefClass &obj);

	GlobalClass(const GlobalClass &);
	GlobalClass & operator=(const GlobalClass &);

	GlobalClass(GlobalClass &&other);
	GlobalClass & operator=(GlobalClass &&);

	GlobalClass(std::nullptr_t);
	GlobalClass & operator=(std::nullptr_t);

	jclass get() const { return static_cast<jclass>(_obj); }
	operator jclass() const { return get(); }
};

class SP_PUBLIC Ref : public ObjectInterface<Ref> {
public:
	Ref(jobject obj, JNIEnv *env) : _obj(obj), _env(env) { }
	Ref(const Local &obj) : _obj(obj.get()), _env(obj.getEnv()) { }
	Ref(const Global &obj, JNIEnv *env) : _obj(obj.get()), _env(env) { }
	Ref(std::nullptr_t) : _obj(nullptr), _env(nullptr) { }

	jobject get() const { return _obj; }
	operator jobject() const { return get(); }
	explicit operator bool() const { return _obj != nullptr; }

	JNIEnv *getEnv() const { return _env; }

	Global getGlobal() const;

protected:
	jobject _obj = nullptr;
	JNIEnv *_env = nullptr;
};

class SP_PUBLIC RefString : public Ref, public StringInterface<RefString> {
public:
	~RefString();
	RefString(jstring obj, JNIEnv *env) : Ref(obj, env) { }
	RefString(const LocalString &obj) : Ref(obj.get(), obj.getEnv()) { }
	RefString(const GlobalString &obj, JNIEnv *env) : Ref(obj.get(), env) { }
	RefString(std::nullptr_t) : Ref(nullptr) { }

	jstring get() const { return static_cast<jstring>(_obj); }
	operator jstring() const { return get(); }

	GlobalString getGlobal() const;
};

class SP_PUBLIC RefClass : public Ref, public ClassInterface<RefClass> {
public:
	RefClass(jclass obj, JNIEnv *env) : Ref(obj, env) { }
	RefClass(const LocalClass &obj) : Ref(obj.get(), obj.getEnv()) { }
	RefClass(const GlobalClass &obj, JNIEnv *env) : Ref(obj.get(), env) { }
	RefClass(std::nullptr_t) : Ref(nullptr) { }

	jclass get() const { return static_cast<jclass>(_obj); }
	operator jclass() const { return get(); }

	GlobalClass getGlobal() const;
};

SP_PUBLIC inline auto Forward(const Local &l) { return l.get(); }
SP_PUBLIC inline auto Forward(const LocalString &l) { return l.get(); }
SP_PUBLIC inline auto Forward(const LocalClass &l) { return l.get(); }
SP_PUBLIC inline auto Forward(const Global &l) { return l.get(); }
SP_PUBLIC inline auto Forward(const GlobalString &l) { return l.get(); }
SP_PUBLIC inline auto Forward(const GlobalClass &l) { return l.get(); }
SP_PUBLIC inline auto Forward(const Ref &l) { return l.get(); }
SP_PUBLIC inline auto Forward(const RefString &l) { return l.get(); }
SP_PUBLIC inline auto Forward(const RefClass &l) { return l.get(); }
SP_PUBLIC inline auto Forward(jboolean value) { return value; }
SP_PUBLIC inline auto Forward(jbyte value) { return value; }
SP_PUBLIC inline auto Forward(jchar value) { return value; }
SP_PUBLIC inline auto Forward(jshort value) { return value; }
SP_PUBLIC inline auto Forward(jint value) { return value; }
SP_PUBLIC inline auto Forward(jlong value) { return value; }
SP_PUBLIC inline auto Forward(jfloat value) { return value; }
SP_PUBLIC inline auto Forward(jdouble value) { return value; }
SP_PUBLIC inline auto Forward(std::nullptr_t value) { return value; }

class SP_PUBLIC Env {
public:
	static Env getEnv();
	static int32_t getSdkVersion();

	static void loadJava(JavaVM *vm, int32_t sdk);
	static void finalizeJava();

	Env() : _env(nullptr) { }
	Env(JNIEnv *env) : _env(env) { }

	Env(const Env &) = default;
	Env(Env &&) = default;
	Env & operator=(const Env &) = default;
	Env & operator=(Env &&) = default;

	explicit operator bool() const { return _env != nullptr; }

	operator JNIEnv *() const { return _env; }

	template <typename ... Args>
	Local newObject(jclass clazz, jmethodID methodID, Args && ... args) const {
		auto ret = Local(_env->NewObject(clazz, methodID, Forward(std::forward<Args>(args))...), _env);
#if DEBUG
		checkErrors();
#endif
		return ret;
    }

	LocalClass getClass(jobject obj) const {
		auto ret = LocalClass(_env->GetObjectClass(obj), _env);
#if DEBUG
		checkErrors();
#endif
		return ret;
	}

	LocalClass findClass(const char* name) const {
		auto ret = LocalClass(_env->FindClass(name), _env);
#if DEBUG
		checkErrors();
#endif
		return ret;
	}

    LocalString newString(WideStringView data) const {
		auto ret = LocalString(_env->NewString((jchar *)data.data(), data.size()), _env);
#if DEBUG
		checkErrors();
#endif
		return ret;
    }

    LocalString newString(StringView data) const {
		auto ret = LocalString(data.terminated() ? _env->NewStringUTF(data.data())
				: _env->NewStringUTF(data.str<memory::StandartInterface>().data()), _env);
#if DEBUG
		checkErrors();
#endif
		return ret;
    }

    jobject newGlobalRef(jobject obj) const {
    	return _env->NewGlobalRef(obj);
    }

    void deleteGlobalRef(jobject obj) const {
    	_env->DeleteGlobalRef(obj);
    }

    void checkErrors() const;

protected:
	JNIEnv *_env = nullptr;
};

#define SP_JNI_EACH_BASIC_TYPE(Name) \
	Name(jboolean, Boolean, "Z") \
	Name(jbyte, Byte, "B") \
	Name(jchar, Char, "C") \
	Name(jshort, Short, "S") \
	Name(jint, Int, "I") \
	Name(jlong, Long, "L") \
	Name(jfloat, Float, "F") \
	Name(jdouble, Double, "D")


#define SP_JNI_MAKE_CALL_METHOD(Type, Name, Sig) \
	template <> \
	struct CallMethod<Type> { \
		using Result = Type; \
		using Intermediate = Type; \
		static constexpr auto Ptr = &_JNIEnv::Call##Name##Method; \
		static Result wrap(Type t, JNIEnv *env) { return t; } \
	};

#define SP_JNI_MAKE_CALL_METHOD_WRAP(Type, Wrapped) \
	template <> \
	struct CallMethod<Type> { \
		using Result = Wrapped; \
		using Intermediate = Type; \
		static constexpr auto Ptr = &_JNIEnv::CallObjectMethod; \
		static Result wrap(Type t, JNIEnv *env) { return Wrapped(t, env); } \
	};

template <>
struct CallMethod<void> {
	using Result = void;
	using Intermediate = void;
	static constexpr auto Ptr = &_JNIEnv::CallVoidMethod;

	template <typename T>
	static void wrap(T &&t, JNIEnv *env) { }
};

SP_JNI_EACH_BASIC_TYPE(SP_JNI_MAKE_CALL_METHOD)
SP_JNI_MAKE_CALL_METHOD_WRAP(jobject, ::sp::jni::Local)
SP_JNI_MAKE_CALL_METHOD_WRAP(jclass, ::sp::jni::LocalClass)
SP_JNI_MAKE_CALL_METHOD_WRAP(jstring, ::sp::jni::LocalString)


#define SP_JNI_MAKE_CALL_STATIC_METHOD(Type, Name, Sig) \
	template <> \
	struct CallStaticMethod<Type> { \
		using Result = Type; \
		using Intermediate = Type; \
		static constexpr auto Ptr = &_JNIEnv::CallStatic##Name##Method; \
		static Result wrap(Type t, JNIEnv *env) { return t; } \
	};

#define SP_JNI_MAKE_CALL_STATIC_METHOD_WRAP(Type, Wrapped) \
	template <> \
	struct CallStaticMethod<Type> { \
		using Result = Wrapped; \
		using Intermediate = Type; \
		static constexpr auto Ptr = &_JNIEnv::CallStaticObjectMethod; \
		static Result wrap(Type t, JNIEnv *env) { return Wrapped(t, env); } \
	};

template <>
struct CallStaticMethod<void> {
	using Result = void;
	using Intermediate = void;
	static constexpr auto Ptr = &_JNIEnv::CallVoidMethod;

	template <typename T>
	static void wrap(T &&t, JNIEnv *env) { }
};

SP_JNI_EACH_BASIC_TYPE(SP_JNI_MAKE_CALL_STATIC_METHOD)
SP_JNI_MAKE_CALL_STATIC_METHOD_WRAP(jobject, ::sp::jni::Local)
SP_JNI_MAKE_CALL_STATIC_METHOD_WRAP(jclass, ::sp::jni::LocalClass)
SP_JNI_MAKE_CALL_STATIC_METHOD_WRAP(jstring, ::sp::jni::LocalString)


#define SP_JNI_MAKE_GET_FIELD(Type, Name, Sig) \
	template <> \
	struct GetField<Type> { \
		using Result = Type; \
		static constexpr auto Ptr = &_JNIEnv::Get##Name##Field; \
		static Result wrap(Type t, JNIEnv *env) { return t; } \
	};

#define SP_JNI_MAKE_GET_FIELD_WRAP(Type, Wrapped) \
	template <> \
	struct GetField<Type> { \
		using Result = Wrapped; \
		using Intermediate = Type; \
		static constexpr auto Ptr = &_JNIEnv::GetObjectField; \
		static Result wrap(Type t, JNIEnv *env) { return Wrapped(t, env); } \
	};

SP_JNI_EACH_BASIC_TYPE(SP_JNI_MAKE_GET_FIELD)
SP_JNI_MAKE_GET_FIELD_WRAP(jobject, ::sp::jni::Local)
SP_JNI_MAKE_GET_FIELD_WRAP(jclass, ::sp::jni::LocalClass)
SP_JNI_MAKE_GET_FIELD_WRAP(jstring, ::sp::jni::LocalString)


#define SP_JNI_MAKE_GET_STATIC_FIELD(Type, Name, _Sig) \
	template <> \
	struct GetStaticField<Type> { \
		using Result = Type; \
		using Intermediate = Type; \
		static constexpr auto Sig = _Sig; \
		static constexpr auto Ptr = &_JNIEnv::GetStatic##Name##Field; \
		static Result wrap(Type t, JNIEnv *env) { return t; } \
	};

#define SP_JNI_MAKE_GET_STATIC_FIELD_WRAP(Type, Wrapped, _Sig) \
	template <> \
	struct GetStaticField<Type> { \
		using Result = Wrapped; \
		using Intermediate = Type; \
		static constexpr auto Sig = _Sig; \
		static constexpr auto Ptr = &_JNIEnv::GetStaticObjectField; \
		static Result wrap(Type t, JNIEnv *env) { return Wrapped(t, env); } \
	};

SP_JNI_EACH_BASIC_TYPE(SP_JNI_MAKE_GET_STATIC_FIELD)
SP_JNI_MAKE_GET_STATIC_FIELD_WRAP(jobject, ::sp::jni::Local, "Ljava/lang/Object;")
SP_JNI_MAKE_GET_STATIC_FIELD_WRAP(jstring, ::sp::jni::LocalString, "Ljava/lang/String;")
SP_JNI_MAKE_GET_STATIC_FIELD_WRAP(jclass, ::sp::jni::LocalClass, "Ljava/lang/Class;")


template <typename Origin>
template <typename Type>
auto ObjectInterface<Origin>::getField(jfieldID id) const -> typename GetField<Type>::Result {
	auto ret = GetField<Type>::wrap((typename GetStaticField<Type>::Intermediate)
			(getObjectEnv()->*GetField<Type>::Ptr)(getObject(), id), getObjectEnv());
#if DEBUG
		Env(getObjectEnv()).checkErrors();
#endif
	return ret;
}

template <typename Origin>
template <typename Type, typename ... Args>
auto ObjectInterface<Origin>::callMethod(jmethodID methodID, Args && ... args) const -> typename CallMethod<Type>::Result {
	if constexpr (std::is_same_v<Type, void>) {
		(typename CallMethod<Type>::Intermediate)(getObjectEnv()->*CallMethod<Type>::Ptr)(getObject(), methodID,
			Forward(std::forward<Args>(args))...);
	} else {
		auto ret = CallMethod<Type>::wrap((typename CallMethod<Type>::Intermediate)
				(getObjectEnv()->*CallMethod<Type>::Ptr)(getObject(), methodID,
				Forward(std::forward<Args>(args))...), getObjectEnv());
#if DEBUG
		Env(getObjectEnv()).checkErrors();
#endif
		return ret;
	}
}

template <typename Origin>
LocalClass ObjectInterface<Origin>::getClass() const {
	return LocalClass(getObjectEnv()->GetObjectClass(getObject()), getObjectEnv());
}

template <typename Origin>
LocalString ObjectInterface<Origin>::getClassName() const {
	static jmethodID getName = nullptr;

	auto obj = Ref(getObject(), getObjectEnv());
	auto cl = obj.getClass();

	if (!getName) {
		auto classClass = cl.getClass();
		getName = classClass.getMethodID("getName", "()Ljava/lang/String;");
	}
	return cl.callMethod<jstring>(getName);
}

template <typename Origin>
JNIEnv *ObjectInterface<Origin>::getObjectEnv() const {
	return static_cast<const Origin *>(this)->getEnv();
}

template <typename Origin>
jobject ObjectInterface<Origin>::getObject() const {
	return static_cast<const Origin *>(this)->get();
}

template <typename Origin>
jmethodID ClassInterface<Origin>::getMethodID(const char* name, const char* sig) const {
	auto ret = getClassEnv()->GetMethodID(getInterfaceClass(), name, sig);
#if DEBUG
		Env(getClassEnv()).checkErrors();
#endif
	return ret;
}

template <typename Origin>
jmethodID ClassInterface<Origin>::getStaticMethodID(const char* name, const char* sig) const {
	auto ret = getClassEnv()->GetStaticMethodID(getInterfaceClass(), name, sig);
#if DEBUG
		Env(getClassEnv()).checkErrors();
#endif
	return ret;
}

template <typename Origin>
jfieldID ClassInterface<Origin>::getFieldID(const char* name, const char* sig) const {
	auto ret = getClassEnv()->GetFieldID(getInterfaceClass(), name, sig);
#if DEBUG
		Env(getClassEnv()).checkErrors();
#endif
	return ret;
}

template <typename Origin>
jfieldID ClassInterface<Origin>::getStaticFieldID(const char* name, const char* sig) const {
	auto ret = getClassEnv()->GetStaticFieldID(getInterfaceClass(), name, sig);
#if DEBUG
		Env(getClassEnv()).checkErrors();
#endif
	return ret;
}

template <typename Origin>
template <typename Type>
auto ClassInterface<Origin>::getStaticField(jfieldID id) const -> typename GetStaticField<Type>::Result {
	auto ret = GetStaticField<Type>::wrap((typename GetStaticField<Type>::Intermediate)
			(getClassEnv()->*GetStaticField<Type>::Ptr)(getInterfaceClass(), id), getClassEnv());
#if DEBUG
		Env(getClassEnv()).checkErrors();
#endif
	return ret;
}

template <typename Origin>
template <typename Type>
auto ClassInterface<Origin>::getStaticField(const char *name, const char *sig) const -> typename GetStaticField<Type>::Result {
	auto ret = GetStaticField<Type>::wrap(
		(typename GetStaticField<Type>::Intermediate)
			(getClassEnv()->*GetStaticField<Type>::Ptr)
				(getInterfaceClass(), getStaticFieldID(name, sig ? sig : GetStaticField<Type>::Sig)),
		getClassEnv());
#if DEBUG
		Env(getClassEnv()).checkErrors();
#endif
	return ret;
}

template <typename Origin>
template <typename Type, typename ... Args>
auto ClassInterface<Origin>::callStaticMethod(jmethodID methodID, Args && ... args) const -> typename CallStaticMethod<Type>::Result {
	if constexpr (std::is_same_v<Type, void>) {
		(typename CallStaticMethod<Type>::Intermediate)(getClassEnv()->*CallStaticMethod<Type>::Ptr)(getInterfaceClass(), methodID,
				Forward(std::forward<Args>(args))...);
	} else {
		auto ret = CallStaticMethod<Type>::wrap((typename CallStaticMethod<Type>::Intermediate)
				(getClassEnv()->*CallStaticMethod<Type>::Ptr)(getInterfaceClass(), methodID,
				Forward(std::forward<Args>(args))...), getClassEnv());
#if DEBUG
		Env(getClassEnv()).checkErrors();
#endif
		return ret;
	}
}

template <typename Origin>
jint ClassInterface<Origin>::registerNatives(const JNINativeMethod* methods, jint nMethods) {
	return getClassEnv()->RegisterNatives(getInterfaceClass(), methods, nMethods);
}

template <typename Origin>
jint ClassInterface<Origin>::unregisterNatives() {
	return getClassEnv()->UnregisterNatives(getInterfaceClass());
}

template <typename Origin>
JNIEnv *ClassInterface<Origin>::getClassEnv() const {
	return static_cast<const Origin *>(this)->getEnv();
}

template <typename Origin>
jclass ClassInterface<Origin>::getInterfaceClass() const {
	return static_cast<const Origin *>(this)->get();
}

template <typename Origin>
WideStringView StringInterface<Origin>::getWideString() {
	if (!_chars) {
		_chars = getStringEnv()->GetStringChars(getInterfaceString(), &_isCopy);
	}

	return WideStringView((const char16_t *)_chars, getStringEnv()->GetStringLength(getInterfaceString()));
}

template <typename Origin>
StringView StringInterface<Origin>::getString() {
	if (!_utfChar) {
		_utfChar = getStringEnv()->GetStringUTFChars(getInterfaceString(), &_utfIsCopy);
	}

	return StringView(_utfChar, getStringEnv()->GetStringUTFLength(getInterfaceString()));
}

template <typename Origin>
void StringInterface<Origin>::reset() {
	if (_chars) {
		getStringEnv()->ReleaseStringChars(getInterfaceString(), _chars);
		_chars = nullptr;
	}

	if (_utfChar) {
		getStringEnv()->ReleaseStringUTFChars(getInterfaceString(), _utfChar);
		_utfChar = nullptr;
	}
}

template <typename Origin>
JNIEnv *StringInterface<Origin>::getStringEnv() const {
	return static_cast<const Origin *>(this)->getEnv();
}

template <typename Origin>
jstring StringInterface<Origin>::getInterfaceString() const {
	return static_cast<const Origin *>(this)->get();
}

}

#endif

#endif /* CORE_CORE_PLATFORM_SPCOREJNI_H_ */
