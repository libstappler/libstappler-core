/**
 Copyright (c) 2025 Stappler LLC <admin@stappler.dev>
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

#include "SPJni.h"

#if ANDROID

#include "SPLog.h"

#include <android/configuration.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/native_activity.h>

namespace STAPPLER_VERSIONIZED stappler::jni {

static Rc<App> s_app;

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

LocalClass::LocalClass(LocalClass &&other) : Local(sp::move(other)) { }

LocalClass &LocalClass::operator=(LocalClass &&other) {
	if (&other == this) {
		return *this;
	}
	Local::operator=(move(other));
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

Ref Global::ref(JNIEnv *env) const { return Ref(*this, env ? env : Env::getEnv().env()); }

LocalString::~LocalString() { reset(); }

LocalString::LocalString(jstring obj, JNIEnv *env) : Local(obj, env) { }

LocalString::LocalString(LocalString &&other) : Local(move(other)) { swap(other); }

LocalString &LocalString::operator=(LocalString &&other) {
	if (&other == this) {
		return *this;
	}

	reset();

	Local::operator=(move(other));
	swap(other);
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

RefString GlobalString::ref(JNIEnv *env) const {
	return RefString(*this, env ? env : Env::getEnv().env());
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

RefClass GlobalClass::ref(JNIEnv *env) const {
	return RefClass(*this, env ? env : Env::getEnv().env());
}

Global Ref::getGlobal() const { return Global(*this); }

RefString::~RefString() { reset(); }

GlobalString RefString::getGlobal() const { return GlobalString(*this); }

GlobalClass RefClass::getGlobal() const { return GlobalClass(*this); }

App::~App() {
	auto env = Env::getEnv();
	auto jAppRef = jApplication.ref(env);

	Application.setNative(jAppRef, 0);

	if (config) {
		AConfiguration_delete(config);
		config = nullptr;
	}

	classLoader.finalize();
}

App::App(const RefClass &cl) : Application(cl) {
	jApplication = Application.s_application(cl);

	jAssetManager = Application.getAssets(jApplication.ref(cl.getEnv()));
	if (jAssetManager) {
		nAssetManager = AAssetManager_fromJava(cl.getEnv(), jAssetManager);
	}

	auto jAppRef = jApplication.ref(cl.getEnv());

	ClipboardManager.service =
			Application.getSystemService(jAppRef, Application.CLIPBOARD_SERVICE(cl));

	DisplayManager.service = Application.getSystemService(jAppRef, Application.DISPLAY_SERVICE(cl));


	if (auto resObj = Application.getResources(jAppRef)) {
		if (auto jConf = Resources.getConfiguration(resObj)) {
			handleConfigurationChanged(jConf);
		}
	}

	classLoader.init(this, cl.getEnv());

	auto packageName = Application.getPackageName(jAppRef);
	auto drawableClassName = mem_std::toString(packageName.getString(), ".R$drawable");
	auto drawablesClass = classLoader.findClass(cl.getEnv(), drawableClassName);
	if (drawablesClass) {
		classLoader.foreachField(cl, [&](StringView type, StringView name, const jni::Ref &obj) {
			if (type == "int") {
				drawables.emplace(name.str<memory::StandartInterface>(),
						classLoader.getIntField(cl, obj));
			}
		});
	}

	Application.setNative(jAppRef, reinterpret_cast<jlong>(this));
}

void App::handleConfigurationChanged(const jni::Ref &ref) {
	std::unique_lock lock(infoMutex);

	if (config) {
		AConfiguration_delete(config);
		config = nullptr;
	}

	config = AConfiguration_new();
	AConfiguration_fromAssetManager(config, nAssetManager);

	sdkVersion = AConfiguration_getSdkVersion(config);

	currentInfo = makeInfo(ref);

	if (configurationHandler) {
		configurationHandler(currentInfo.get());
	}
}

void App::handleLowMemory(const jni::Ref &ref) {
	if (lowMemoryHandler) {
		lowMemoryHandler();
	}
}

static StringView getApplicationName(App *proxy, const jni::Ref &ctx, memory::pool_t *pool) {
	auto jAppInfo = proxy->Application.getApplicationInfo(ctx);
	if (!jAppInfo) {
		return StringView();
	}

	auto labelRes = proxy->ApplicationInfo.labelRes(jAppInfo);
	if (labelRes == 0) {
		auto jNonLocalizedLabel = proxy->ApplicationInfo.nonLocalizedLabel(jAppInfo);
		return proxy->CharSequence.toString(jNonLocalizedLabel).getString().pdup(pool);
	} else {
		auto jAppName = proxy->Application.getString(ctx, jint(labelRes));
		return jAppName.getString().pdup(pool);
	}
}

static StringView getApplicationVersion(App *proxy, const jni::Ref &ctx,
		const jni::RefString &jPackageName, memory::pool_t *pool) {
	auto jpm = proxy->Application.getPackageManager(ctx);
	if (!jpm) {
		return StringView();
	}

	auto jinfo = proxy->PackageManager.getPackageInfo(jpm, jPackageName, 0);
	if (!jinfo) {
		return StringView();
	}

	auto jversion = proxy->PackageInfo.versionName(jinfo);
	if (!jversion) {
		return StringView();
	}

	return jversion.getString().pdup(pool);
}

static StringView getSystemAgent(App *proxy, const jni::Env &env, memory::pool_t *pool) {
	return proxy->System.getProperty(proxy->System.getClass().ref(env), env.newString("http.agent"))
			.getString()
			.pdup(pool);
}

static StringView getUserAgent(App *proxy, const jni::Ref &ctx, memory::pool_t *pool) {
	if (proxy->WebSettings && proxy->WebSettings.getDefaultUserAgent) {
		return proxy->WebSettings
				.getDefaultUserAgent(proxy->WebSettings.getClass().ref(ctx.getEnv()), ctx)
				.getString()
				.pdup(pool);
	}
	return StringView();
}

void App::setActivityLoader(mem_std::Function<bool(ANativeActivity *, BytesView)> &&cb) {
	activityLoader = sp::move(cb);
}

void App::setConfigurationHandler(mem_std::Function<void(platform::ApplicationInfo *)> &&cb) {
	configurationHandler = sp::move(cb);
}

void App::setLowMemoryHandler(mem_std::Function<void()> &&cb) { lowMemoryHandler = sp::move(cb); }

Rc<SharedRef<platform::ApplicationInfo>> App::makeInfo(const jni::Ref &ref) {
	auto info = Rc<SharedRef<platform::ApplicationInfo>>::create(SharedRefMode::Allocator, ref);

	auto env = Env::getEnv();
	auto ctx = jni::Ref(jApplication, env);

	auto jPackageName = Application.getPackageName(ctx);
	if (jPackageName) {
		info->bundleName = jPackageName.getString().pdup(info->getPool());
		info->applicationName = getApplicationName(this, ctx, info->getPool());
		info->applicationVersion = getApplicationVersion(this, ctx, jPackageName, info->getPool());
		info->systemAgent = getSystemAgent(this, env, info->getPool());
		info->userAgent = getUserAgent(this, ctx, info->getPool());
	}

	// Use DP size as fallback
	int32_t widthPixels = AConfiguration_getScreenWidthDp(config);
	int32_t heightPixels = AConfiguration_getScreenHeightDp(config);
	float displayDensity = nan();

	if (auto resObj = Application.getResources(ctx)) {
		if (auto dmObj = Resources.getDisplayMetrics(resObj)) {
			displayDensity = DisplayMetrics.density(dmObj);
			heightPixels = DisplayMetrics.heightPixels(dmObj);
			widthPixels = DisplayMetrics.widthPixels(dmObj);
		}
	}

	std::array<char, 6> language;
	memcpy(language.data(), "en-us", 6);
	AConfiguration_getLanguage(config, language.data());
	AConfiguration_getCountry(config, language.data() + 3);

	string::apply_tolower_c(language);
	info->locale = StringView(language).pdup(info->getPool());

	if (isnan(displayDensity)) {
		auto densityValue = AConfiguration_getDensity(config);
		switch (densityValue) {
		case ACONFIGURATION_DENSITY_LOW: displayDensity = 0.75f; break;
		case ACONFIGURATION_DENSITY_MEDIUM: displayDensity = 1.0f; break;
		case ACONFIGURATION_DENSITY_TV: displayDensity = 1.5f; break;
		case ACONFIGURATION_DENSITY_HIGH: displayDensity = 1.5f; break;
		case 280: displayDensity = 2.0f; break;
		case ACONFIGURATION_DENSITY_XHIGH: displayDensity = 2.0f; break;
		case 360: displayDensity = 3.0f; break;
		case 400: displayDensity = 3.0f; break;
		case 420: displayDensity = 3.0f; break;
		case ACONFIGURATION_DENSITY_XXHIGH: displayDensity = 3.0f; break;
		case 560: displayDensity = 4.0f; break;
		case ACONFIGURATION_DENSITY_XXXHIGH: displayDensity = 4.0f; break;
		default: displayDensity = 1.0f; break;
		}

		widthPixels = widthPixels * displayDensity;
		heightPixels = heightPixels * displayDensity;
	}

	info->density = displayDensity;

	int32_t orientation = AConfiguration_getOrientation(config);

	switch (orientation) {
	case ACONFIGURATION_ORIENTATION_ANY:
	case ACONFIGURATION_ORIENTATION_SQUARE:
		info->pixelWidth = widthPixels;
		info->pixelHeight = heightPixels;
		break;
	case ACONFIGURATION_ORIENTATION_PORT:
		info->pixelWidth = std::min(widthPixels, heightPixels);
		info->pixelHeight = std::max(widthPixels, heightPixels);
		break;
	case ACONFIGURATION_ORIENTATION_LAND:
		info->pixelWidth = std::max(widthPixels, heightPixels);
		info->pixelHeight = std::min(widthPixels, heightPixels);
		break;
	}

	info->dpWidth = float(info->pixelWidth) / displayDensity;
	info->dpWidth = float(info->pixelHeight) / displayDensity;
	info->orientation = platform::ApplicationInfo::Orientation(orientation);

	info->isEmulator = Application.isEmulator(ctx);

	return info;
}

bool App::loadActivity(ANativeActivity *a, BytesView data) {
	if (!activityLoader) {
		return false;
	}
	return activityLoader(a, data);
}

static JNIEnv *getVmEnv(JavaVM *vm) {
	void *ret = nullptr;
	if (vm) {
		vm->GetEnv(&ret, JNI_VERSION_1_6);
	} else {
		log::source().error("JNI", "JavaVM not found");
	}
	return reinterpret_cast<JNIEnv *>(ret);
}

Env Env::getEnv() {
	if (!tl_thread.env) {
		if (s_app && s_app->vm) {
			auto env = getVmEnv(s_app->vm);
			if (env) {
				tl_thread.init(s_app->vm, env, false);
			} else if (s_app && s_app->vm) {
				s_app->vm->AttachCurrentThread(&env, nullptr);
				if (env) {
					tl_thread.init(s_app->vm, env, true);
				}
			}
		}
	}
	return tl_thread.env;
}

App *Env::getApp() { return s_app; }

ClassLoader *Env::getClassLoader() {
	return &s_app->classLoader; // &s_classLoader;
}

static void Application_handleConfigurationChanged(JNIEnv *env, jobject thiz, jlong native,
		jobject config) {
	if (native) {
		auto app = reinterpret_cast<App *>(native);
		app->handleConfigurationChanged(jni::Ref(config, env));
	}
}

static void Application_handleLowMemory(JNIEnv *env, jobject thiz, jlong native) {
	if (native) {
		auto app = reinterpret_cast<App *>(native);
		app->handleLowMemory(jni::Ref(thiz, env));
	}
}

static JNINativeMethod s_AppNativeMethods[] = {
	{"handleConfigurationChanged", "(JLandroid/content/res/Configuration;)V",
		reinterpret_cast<void *>(&Application_handleConfigurationChanged)},
	{"handleLowMemory", "(J)V", reinterpret_cast<void *>(&Application_handleLowMemory)},
};

void Env::loadJava(JavaVM *vm) {
	SPASSERT(!s_app, "VM already defined");

	auto env = getVmEnv(vm);
	tl_thread.init(vm, env, false);

	auto applicationClass = LocalClass(env->FindClass("org/stappler/core/Application"), env);

	if (applicationClass) {
		s_app = Rc<App>::create(RefClass(applicationClass));
		s_app->vm = vm;
	}

	SPASSERT(s_app,
			"Fail to load AppProxy; org/stappler/appsupport/Application class was not defined "
			"properly?");

	applicationClass.registerNatives(s_AppNativeMethods);
}

void Env::finalizeJava() { s_app = nullptr; }

void Env::checkErrors() const { detail::checkErrors(_env); }

bool ClassLoader::init(App *a, JNIEnv *env) {
	app = a;
	auto thiz = Ref(app->jApplication, env);

	auto currentClassLoader = app->Class.getClassLoader(thiz.getClass());

	auto codeCacheDir = app->Application.getCodeCacheDir(thiz);
	auto codeCachePath = app->File.getAbsolutePath(codeCacheDir);

	auto packageName = app->Application.getPackageName(thiz);
	auto packageManager = app->Application.getPackageManager(thiz);

	LocalString publicSourceDir;
	LocalString nativeLibraryDir;

	if (packageName && packageManager) {
		auto applicationInfo =
				app->PackageManager.getApplicationInfo(packageManager, packageName, 0);
		if (applicationInfo) {
			publicSourceDir = app->ApplicationInfo.publicSourceDir(applicationInfo);
			nativeLibraryDir = app->ApplicationInfo.nativeLibraryDir(applicationInfo);
		}
	}

	if (!codeCachePath || !publicSourceDir) {
		return false;
	}

	if (currentClassLoader) {
		appClassLoader = currentClassLoader;
		appClassLoaderClass = currentClassLoader.getClass();

		auto className = currentClassLoader.getClassName();

		log::source().info("JNI", "App: ClassLoader: ", className.getString());

		auto dexClassLoader =
				app->DexClassLoader.constructor(app->DexClassLoader.getClass().ref(env),
						publicSourceDir, codeCachePath, nativeLibraryDir, currentClassLoader);
		if (dexClassLoader) {
			apkClassLoader = dexClassLoader;
			apkClassLoaderClass = dexClassLoader.getClass();
		}
	}

	this->apkPath = publicSourceDir.getString().str<memory::StandartInterface>();
	this->nativeLibraryDir = nativeLibraryDir.getString().str<memory::StandartInterface>();

	return true;
}

void ClassLoader::finalize() {
	appClassLoader = nullptr;
	appClassLoaderClass = nullptr;
	apkClassLoader = nullptr;
	apkClassLoaderClass = nullptr;
}

void ClassLoader::foreachMethod(const jni::RefClass &cl,
		const Callback<void(StringView, const jni::Ref &)> &cb) {
	auto methods = app->Class.getMethods(cl);
	for (auto it : methods) { cb(app->Method.getName(it).getString(), it); }
}

void ClassLoader::foreachField(const jni::RefClass &cl,
		const Callback<void(StringView, StringView, const jni::Ref &)> &cb) {

	auto fields = app->Class.getFields(cl);

	for (auto it : fields) {
		cb(app->Field.getType(it).getName().getString(), app->Field.getName(it).getString(), it);
	}
}

int ClassLoader::getIntField(const jni::Ref &origin, const jni::Ref &field) {
	return app->Field.getInt(field, origin);
}

jni::LocalClass ClassLoader::findClass(const jni::Env &env, StringView data) {
	return findClass(env.newString(data));
}

jni::LocalClass ClassLoader::findClass(const jni::RefString &str) {
	return app->DexClassLoader.loadClass(apkClassLoader.ref(str.getEnv()), str, jboolean(1));
}

ClassProxy::ClassProxy(const char *name) {
	auto env = Env::getEnv();
	if (env) {
		_class = env.findClass(name);
	}
}

} // namespace stappler::jni

namespace STAPPLER_VERSIONIZED stappler::jni::detail {

auto TypeInfo<jobject>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jstring>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jclass>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jobjectArray>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jbooleanArray>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jbyteArray>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jcharArray>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jshortArray>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jintArray>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jlongArray>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jfloatArray>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

auto TypeInfo<jdoubleArray>::wrap(Type t, JNIEnv *env) -> Result { return Result(t, env); }

SP_PUBLIC void checkErrors(JNIEnv *env) {
	if (env->ExceptionCheck()) {
		// Read exception msg
		auto e = Local(env->ExceptionOccurred(), env);
		env->ExceptionClear(); // clears the exception; e seems to remain valid

		auto clazz = e.getClass();
		auto classClass = clazz.getClass();
		jmethodID getName = classClass.getMethodID("getName", "()Ljava/lang/String;");
		jmethodID getMessage = clazz.getMethodID("getMessage", "()Ljava/lang/String;");

		auto message = e.callMethod<jstring>(getMessage);
		auto exName = clazz.callMethod<jstring>(getName);

		log::source().error("JNI", "[", exName.getString(), "] ", message.getString());
	}
}

} // namespace stappler::jni::detail

namespace STAPPLER_VERSIONIZED stappler::platform {

Rc<SharedRef<ApplicationInfo>> ApplicationInfo::getCurrent() {
	std::unique_lock lock(jni::s_app->infoMutex);
	return jni::s_app->currentInfo;
}

Rc<SharedRef<ApplicationInfo>> ApplicationInfo::acquireNewInfo() {
	auto env = jni::Env::getEnv();
	auto app = jni::Env::getApp();
	auto jApp = app->jApplication.ref(env);

	if (auto resObj = app->Application.getResources(jApp)) {
		if (auto jConf = app->Resources.getConfiguration(resObj)) {

			return jni::s_app->makeInfo(jConf);
		}
	}
	return nullptr;
}

bool ApplicationInfo::init(const jni::Ref &ref) {
	jConfig = ref;
	return true;
}

} // namespace stappler::platform

#endif
