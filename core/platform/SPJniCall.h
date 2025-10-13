/**
 Copyright (c) 2025 Stappler Team <admin@stappler.org>

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

#ifndef CORE_CORE_PLATFORM_SPCOREJNICALL_H_
#define CORE_CORE_PLATFORM_SPCOREJNICALL_H_

#include "SPJniObject.h" // IWYU pragma: keep

#if ANDROID

namespace STAPPLER_VERSIONIZED stappler::jni {

class SP_PUBLIC ClassProxy {
public:
	ClassProxy(const LocalClass &ref) : _class(ref) { }
	ClassProxy(const RefClass &ref) : _class(ref) { }
	ClassProxy(const char *);

	const jni::GlobalClass &getClass() const { return _class; }

	explicit operator bool() const { return _class.getObject() != nullptr; }

protected:
	GlobalClass _class = nullptr;
};

template <detail::ClassNameSignature Value, typename Type, typename... Args>
class SP_PUBLIC Method<Value, Type(Args...)> {
public:
	static constexpr const char *Name = Value.data.data();

	using ResultType = typename detail::SignatureWrapper<Type>::Result;
	using Sig = Type(Args...);

	Method(std::nullptr_t) { }
	Method(jmethodID id) : _method(id) { }

	Method(ClassProxy *proxy) {
		if (!proxy || !proxy->getClass()) {
			slog().warn("JNI", "ClassProxy for method ", Name, " : ", MethodSignature<Sig>::Name,
					"is invalid");
			return;
		}
		_method = proxy->getClass().getMethodID(Name, MethodSignature<Sig>::Buffer.data());
		if (!_method) {
			slog().warn("JNI", "Method ", Name, " : ", MethodSignature<Sig>::Name,
					" not found in class ", proxy->getClass().getName().getString());
		}
	}

	operator bool() const { return _method != nullptr; }

	ResultType operator()(JNIEnv *, jobject,
			typename detail::SignatureWrapper<Args>::Type &&...args) const;
	ResultType operator()(const Ref &,
			typename detail::SignatureWrapper<Args>::Type &&...args) const;

protected:
	jmethodID _method = nullptr;
};

template <detail::ClassNameSignature Value, typename Type, typename... Args>
class SP_PUBLIC StaticMethod<Value, Type(Args...)> {
public:
	static constexpr const char *Name = Value.data.data();

	using ResultType =
			typename detail::TypeInfo<typename detail::SignatureWrapper<Type>::Type>::Result;
	using Sig = Type(Args...);

	StaticMethod(std::nullptr_t) { }
	StaticMethod(jmethodID id) : _method(id) { }

	StaticMethod(ClassProxy *proxy) {
		if (!proxy || !proxy->getClass()) {
			slog().warn("JNI", "ClassProxy for static method ", Name, " : ",
					MethodSignature<Sig>::Name, "is invalid");
			return;
		}
		_method = proxy->getClass().getStaticMethodID(Name, MethodSignature<Sig>::Buffer.data());
		if (!_method) {
			slog().warn("JNI", "Static method ", Name, " : ", MethodSignature<Sig>::Name,
					" not found in class ", proxy->getClass().getName().getString());
		}
	}

	operator bool() const { return _method != nullptr; }

	ResultType operator()(JNIEnv *, jclass,
			typename detail::SignatureWrapper<Args>::Type &&...args) const;
	ResultType operator()(const RefClass &,
			typename detail::SignatureWrapper<Args>::Type &&...args) const;

protected:
	jmethodID _method = nullptr;
};

template <typename Type, typename... Args>
class SP_PUBLIC Constructor<Type(Args...)> {
public:
	static constexpr const char *Name = "<init>";

	using ResultType =
			typename detail::TypeInfo<typename detail::SignatureWrapper<Type>::Type>::Result;
	using Sig = Type(Args...);

	static_assert(std::is_same_v<ResultType, void>, "Result type should not be defined");

	Constructor(std::nullptr_t) { }
	Constructor(jmethodID id) : _method(id) { }

	Constructor(ClassProxy *proxy) {
		if (!proxy || !proxy->getClass()) {
			slog().warn("JNI", "ClassProxy for constructor ", Name, " : ",
					MethodSignature<Sig>::Name, "is invalid");
			return;
		}
		_method = proxy->getClass().getMethodID("<init>", MethodSignature<Sig>::Buffer.data());
		if (!_method) {
			slog().warn("JNI", "Constructor ", MethodSignature<Sig>::Name, " not found in class ",
					proxy->getClass().getName().getString());
		}
	}

	operator bool() const { return _method != nullptr; }

	Local operator()(JNIEnv *, jclass,
			typename detail::SignatureWrapper<Args>::Type &&...args) const;
	Local operator()(const RefClass &,
			typename detail::SignatureWrapper<Args>::Type &&...args) const;

protected:
	jmethodID _method = nullptr;
};

template <detail::ClassNameSignature Value, typename Type>
class SP_PUBLIC Field {
public:
	static constexpr const char *Name = Value.data.data();

	using ResultType = typename detail::SignatureWrapper<Type>::Result;

	Field(std::nullptr_t) { }
	Field(jfieldID id) : _field(id) { }

	Field(ClassProxy *proxy) {
		if (!proxy || !proxy->getClass()) {
			slog().warn("JNI", "ClassProxy for field ", Name, "is invalid");
			return;
		}
		_field = proxy->getClass().getFieldID(Name, FieldSignature<Type>::Buffer.data());
		if (!_field) {
			slog().warn("JNI", "Field (", FieldSignature<Type>::Name, ") ", Name,
					" not found in class ", proxy->getClass().getName().getString());
		}
	}

	operator bool() const { return _field != nullptr; }

	ResultType operator()(JNIEnv *, jobject) const;
	ResultType operator()(const Ref &) const;

protected:
	jfieldID _field = nullptr;
};

} // namespace stappler::jni

namespace STAPPLER_VERSIONIZED stappler::jni::detail {

template <detail::ClassNameSignature Value, typename Type>
class SP_PUBLIC StaticFieldBasic {
public:
	static constexpr const char *Name = Value.data.data();

	using ResultType = typename detail::SignatureWrapper<Type>::Result;

	StaticFieldBasic(std::nullptr_t) { }
	StaticFieldBasic(jfieldID id) : _field(id) { }

	StaticFieldBasic(ClassProxy *proxy) {
		if (!proxy || !proxy->getClass()) {
			slog().warn("JNI", "ClassProxy for static field ", Name, "is invalid");
			return;
		}
		_field = proxy->getClass().getStaticFieldID(Name, FieldSignature<Type>::Buffer.data());
		if (!_field) {
			slog().warn("JNI", "Static field (", FieldSignature<Type>::Name, ") ", Name,
					" not found in class ", proxy->getClass().getName().getString());
		} else {
			_value =
					proxy->getClass().getStaticField<typename detail::SignatureWrapper<Type>::Type>(
							_field);
		}
	}

	operator bool() const { return _field != nullptr; }

	ResultType operator()(JNIEnv *, jclass) const { return _value; }
	ResultType operator()(const RefClass &) const { return _value; }
	ResultType operator()() const { return _value; }

protected:
	jfieldID _field = nullptr;
	ResultType _value = 0;
};

template <detail::ClassNameSignature Value, typename Type>
class SP_PUBLIC StaticFieldObject {
public:
	static constexpr const char *Name = Value.data.data();

	using ResultType = typename detail::SignatureWrapper<Type>::Result;

	StaticFieldObject(std::nullptr_t) { }
	StaticFieldObject(jfieldID id) : _field(id) { }

	StaticFieldObject(ClassProxy *proxy) {
		if (!proxy || !proxy->getClass()) {
			slog().warn("JNI", "ClassProxy for static field ", Name, "is invalid");
			return;
		}
		_field = proxy->getClass().getStaticFieldID(Name, FieldSignature<Type>::Buffer.data());
		if (!_field) {
			slog().warn("JNI", "Static field (", FieldSignature<Type>::Name, ") ", Name,
					" not found in class ", proxy->getClass().getName().getString());
		}
	}

	operator bool() const { return _field != nullptr; }

	ResultType operator()(JNIEnv *, jclass) const;
	ResultType operator()(const RefClass &) const;

protected:
	jfieldID _field = nullptr;
};

template <ClassNameSignature Value, typename Type>
auto StaticFieldObject<Value, Type>::operator()(JNIEnv *env, jclass obj) const -> ResultType {
	return operator()(RefClass(obj, env));
}

template <ClassNameSignature Value, typename Type>
auto StaticFieldObject<Value, Type>::operator()(const RefClass &obj) const -> ResultType {
	return obj.getStaticField<typename detail::SignatureWrapper<Type>::Type>(_field);
}

} // namespace stappler::jni::detail


namespace STAPPLER_VERSIONIZED stappler::jni {

template <detail::ClassNameSignature Value, typename Type>
class SP_PUBLIC StaticField : public detail::StaticFieldObject<Value, Type> {
public:
	using detail::StaticFieldObject<Value, Type>::StaticFieldObject;
};

template <detail::ClassNameSignature Value>
class SP_PUBLIC StaticField<Value, jboolean> : public detail::StaticFieldBasic<Value, jboolean> {
public:
	using detail::StaticFieldBasic<Value, jboolean>::StaticFieldBasic;
};

template <detail::ClassNameSignature Value>
class SP_PUBLIC StaticField<Value, jbyte> : public detail::StaticFieldBasic<Value, jbyte> {
public:
	using detail::StaticFieldBasic<Value, jbyte>::StaticFieldBasic;
};

template <detail::ClassNameSignature Value>
class SP_PUBLIC StaticField<Value, jchar> : public detail::StaticFieldBasic<Value, jchar> {
public:
	using detail::StaticFieldBasic<Value, jchar>::StaticFieldBasic;
};

template <detail::ClassNameSignature Value>
class SP_PUBLIC StaticField<Value, jshort> : public detail::StaticFieldBasic<Value, jshort> {
public:
	using detail::StaticFieldBasic<Value, jshort>::StaticFieldBasic;
};

template <detail::ClassNameSignature Value>
class SP_PUBLIC StaticField<Value, jint> : public detail::StaticFieldBasic<Value, jint> {
public:
	using detail::StaticFieldBasic<Value, jint>::StaticFieldBasic;
};

template <detail::ClassNameSignature Value>
class SP_PUBLIC StaticField<Value, jlong> : public detail::StaticFieldBasic<Value, jlong> {
public:
	using detail::StaticFieldBasic<Value, jlong>::StaticFieldBasic;
};

template <detail::ClassNameSignature Value>
class SP_PUBLIC StaticField<Value, jfloat> : public detail::StaticFieldBasic<Value, jfloat> {
public:
	using detail::StaticFieldBasic<Value, jfloat>::StaticFieldBasic;
};

template <detail::ClassNameSignature Value>
class SP_PUBLIC StaticField<Value, jdouble> : public detail::StaticFieldBasic<Value, jdouble> {
public:
	using detail::StaticFieldBasic<Value, jdouble>::StaticFieldBasic;
};

template <detail::ClassNameSignature Value, typename Result, typename... Args>
auto Method<Value, Result(Args...)>::operator()(JNIEnv *env, jobject obj,
		typename detail::SignatureWrapper<Args>::Type &&...args) const -> ResultType {
	return operator()(Ref(obj, env),
			std::forward<typename detail::SignatureWrapper<Args>::Type>(args)...);
}

template <detail::ClassNameSignature Value, typename Result, typename... Args>
auto Method<Value, Result(Args...)>::operator()(const Ref &obj,
		typename detail::SignatureWrapper<Args>::Type &&...args) const -> ResultType {
	if constexpr (std::is_same_v<void, Result>) {
		return obj.callMethod<void>(_method,
				std::forward<typename detail::SignatureWrapper<Args>::Type>(args)...);
	} else {
		return obj.callMethod<typename detail::SignatureWrapper<Result>::Type>(_method,
				std::forward<typename detail::SignatureWrapper<Args>::Type>(args)...);
	}
}

template <detail::ClassNameSignature Value, typename Result, typename... Args>
auto StaticMethod<Value, Result(Args...)>::operator()(JNIEnv *env, jclass clazz,
		typename detail::SignatureWrapper<Args>::Type &&...args) const -> ResultType {
	return operator()(RefClass(clazz, env),
			std::forward<typename detail::SignatureWrapper<Args>::Type>(args)...);
}

template <detail::ClassNameSignature Value, typename Result, typename... Args>
auto StaticMethod<Value, Result(Args...)>::operator()(const RefClass &clazz,
		typename detail::SignatureWrapper<Args>::Type &&...args) const -> ResultType {
	if constexpr (std::is_same_v<void, Result>) {
		return clazz.callStaticMethod<void>(_method,
				std::forward<typename detail::SignatureWrapper<Args>::Type>(args)...);
	} else {
		return clazz.callStaticMethod<typename detail::SignatureWrapper<Result>::Type>(_method,
				std::forward<typename detail::SignatureWrapper<Args>::Type>(args)...);
	}
}

template <typename Result, typename... Args>
auto Constructor<Result(Args...)>::operator()(JNIEnv *env, jclass clazz,
		typename detail::SignatureWrapper<Args>::Type &&...args) const -> Local {
	return operator()(RefClass(clazz, env),
			std::forward<typename detail::SignatureWrapper<Args>::Type>(args)...);
}

template <typename Result, typename... Args>
auto Constructor<Result(Args...)>::operator()(const RefClass &clazz,
		typename detail::SignatureWrapper<Args>::Type &&...args) const -> Local {
	return clazz.callConstructor(_method,
			std::forward<typename detail::SignatureWrapper<Args>::Type>(args)...);
}

template <detail::ClassNameSignature Value, typename Type>
auto Field<Value, Type>::operator()(JNIEnv *env, jobject obj) const -> ResultType {
	return operator()(Ref(obj, env));
}

template <detail::ClassNameSignature Value, typename Type>
auto Field<Value, Type>::operator()(const Ref &obj) const -> ResultType {
	return obj.getField<typename detail::SignatureWrapper<Type>::Type>(_field);
}

} // namespace stappler::jni

#endif

#endif // CORE_CORE_PLATFORM_SPCOREJNICALL_H_
