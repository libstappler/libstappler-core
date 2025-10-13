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

#ifndef CORE_CORE_PLATFORM_SPCOREJNI_H_
#define CORE_CORE_PLATFORM_SPCOREJNI_H_

#include "SPJniCall.h" // IWYU pragma: keep
#include "SPRef.h" // IWYU pragma: keep
#include "SPMemory.h"

#if ANDROID

struct AConfiguration;
struct ANativeActivity;
struct AAssetManager;

#define SP_JAVA_APPLICATION_CLASS "org/stappler/core/Application"

/* Use Application from org.stappler.core library as the base class of android application

define: 
	<application
		...
		android:name="org.stappler.core.Application"
		...
	>
in AndroidManifest.xml
*/

namespace STAPPLER_VERSIONIZED stappler::platform {

struct SP_PUBLIC ApplicationInfo : public memory::PoolObject {
	enum Orientation {
		Any = 0x0000,
		Portrait = 0x0001,
		Landscape = 0x0002,
		Square = 0x0003,
	};

	static Rc<SharedRef<ApplicationInfo>> getCurrent();
	static Rc<SharedRef<ApplicationInfo>> acquireNewInfo();

	using PoolObject::PoolObject;

	bool init(const jni::Ref &);

	StringView bundleName;
	StringView applicationName;
	StringView applicationVersion;
	StringView userAgent;
	StringView systemAgent;
	StringView locale;

	uint32_t pixelWidth = 0;
	uint32_t pixelHeight = 0;
	float dpWidth = 0.0f;
	float dpHeight = 0.0f;
	float density = 1.0f;

	Orientation orientation = Any;
	bool isEmulator = false;

	jni::Global jConfig = nullptr; // android.content.res.Configuration reference
};

} // namespace stappler::platform

namespace STAPPLER_VERSIONIZED stappler::jni {

class Env;
struct App;

template <detail::ClassNameSignature Value>
using L = detail::ObjectSignature<Value>;

template <typename T>
using A = detail::ArraySignature<T>;

struct ApplicationProxy : ClassProxy {
	StaticField<"s_application", L<SP_JAVA_APPLICATION_CLASS>> s_application = this;
	StaticField<"CLIPBOARD_SERVICE", jstring> CLIPBOARD_SERVICE = this;
	StaticField<"DISPLAY_SERVICE", jstring> DISPLAY_SERVICE = this;

	Method<"getAssets", L<"android.content.res.AssetManager">()> getAssets = this;
	Method<"getCodeCacheDir", L<"java.io.File">()> getCodeCacheDir = this;
	Method<"getPackageName", jstring()> getPackageName = this;
	Method<"getPackageManager", L<"android/content/pm/PackageManager">()> getPackageManager = this;
	Method<"getFilesDir", L<"java/io/File">()> getFilesDir = this;
	Method<"getCacheDir", L<"java/io/File">()> getCacheDir = this;
	Method<"getExternalFilesDir", L<"java/io/File">(jstring)> getExternalFilesDir = this;
	Method<"getExternalCacheDir", L<"java/io/File">()> getExternalCacheDir = this;

	Method<"getApplicationInfo", L<"android/content/pm/ApplicationInfo">()> getApplicationInfo =
			this;
	Method<"getSystemService", L<"java/lang/Object">(jstring)> getSystemService = this;

	Method<"getResources", L<"android/content/res/Resources">()> getResources = this;

	Method<"getString", jstring(jint)> getString = this;
	Method<"isEmulator", jboolean()> isEmulator = this;
	Method<"setNative", void(jlong)> setNative = this;

	using ClassProxy::ClassProxy;
};

struct EnvironmentProxy : ClassProxy {
	StaticMethod<"getExternalStorageDirectory", L<"java/io/File">()> getExternalStorageDirectory =
			this;
	StaticMethod<"getExternalStoragePublicDirectory", L<"java/io/File">(jstring)>
			getExternalStoragePublicDirectory = this;

	using ClassProxy::ClassProxy;
};

struct FileProxy : ClassProxy {
	Method<"getAbsolutePath", jstring()> getAbsolutePath = this;

	using ClassProxy::ClassProxy;
};

struct ClassClassProxy : ClassProxy {
	Method<"getClassLoader", L<"java.lang.ClassLoader">()> getClassLoader = this;

	Method<"getMethods", A<L<"java.lang.reflect.Method">>()> getMethods = this;
	Method<"getFields", A<L<"java.lang.reflect.Field">>()> getFields = this;
	Method<"getName", jstring()> getName = this;

	using ClassProxy::ClassProxy;
};

struct ClassMethodProxy : ClassProxy {
	Method<"getName", jstring()> getName = this;

	using ClassProxy::ClassProxy;
};

struct ClassFieldProxy : ClassProxy {
	Method<"getName", jstring()> getName = this;
	Method<"getType", jclass()> getType = this;
	Method<"getInt", jint(jobject)> getInt = this;
	Method<"getLong", jlong(jobject)> getLong = this;
	Method<"getShort", jshort(jobject)> getShort = this;
	Method<"getFloat", jfloat(jobject)> getFloat = this;
	Method<"getDouble", jdouble(jobject)> getDouble = this;
	Method<"getChar", jchar(jobject)> getChar = this;
	Method<"getByte", jbyte(jobject)> getByte = this;
	Method<"getBoolean", jboolean(jobject)> getBoolean = this;

	using ClassProxy::ClassProxy;
};

struct SystemProxy : ClassProxy {
	StaticMethod<"getProperty", jstring(jstring)> getProperty = this;

	using ClassProxy::ClassProxy;
};

struct WebSettingsProxy : ClassProxy {
	StaticMethod<"getDefaultUserAgent", jstring(L<"android/content/Context">)> getDefaultUserAgent =
			this;

	using ClassProxy::ClassProxy;
};

// With DexClassLoader we can load additional classes from application, that was not defined in Manifest
struct DexClassLoaderProxy : ClassProxy {
	Constructor<void(jstring, jstring, jstring, L<"java/lang/ClassLoader">)> constructor = this;

	Method<"loadClass", jclass(jstring, jboolean)> loadClass = this;

	using ClassProxy::ClassProxy;
};

struct PackageManagerProxy : ClassProxy {
	Method<"getApplicationInfo", L<"android/content/pm/ApplicationInfo">(jstring, jint)>
			getApplicationInfo = this;
	Method<"getPackageInfo", L<"android/content/pm/PackageInfo">(jstring, jint)> getPackageInfo =
			this;

	using ClassProxy::ClassProxy;
};

struct ApplicationInfoProxy : ClassProxy {
	Field<"versionCode", jint> versionCode = this;
	Field<"versionName", jstring> versionName = this;

	Field<"labelRes", jint> labelRes = this;
	Field<"nonLocalizedLabel", L<"java/lang/CharSequence">> nonLocalizedLabel = this;

	Field<"publicSourceDir", jstring> publicSourceDir = this;
	Field<"nativeLibraryDir", jstring> nativeLibraryDir = this;

	using ClassProxy::ClassProxy;
};

struct PackageInfoProxy : ClassProxy {
	Field<"versionCode", jint> versionCode = this;
	Field<"versionName", jstring> versionName = this;

	using ClassProxy::ClassProxy;
};

struct ResourcesProxy : ClassProxy {
	Method<"getDisplayMetrics", L<"android/util/DisplayMetrics">()> getDisplayMetrics = this;
	Method<"getConfiguration", L<"android/content/res/Configuration">()> getConfiguration = this;

	using ClassProxy::ClassProxy;
};

struct DisplayMetricsProxy : ClassProxy {
	Constructor<void()> constructor = this;

	Field<"density", jfloat> density = this;
	Field<"xdpi", jfloat> xdpi = this;
	Field<"ydpi", jfloat> ydpi = this;
	Field<"heightPixels", jint> heightPixels = this;
	Field<"widthPixels", jint> widthPixels = this;

	using ClassProxy::ClassProxy;
};

struct IntentProxy : ClassProxy {
	StaticField< "ACTION_VIEW", jstring> ACTION_VIEW = this;

	Constructor<void(jstring, L<"android/net/Uri">)> constructor = this;
	Method<"addFlags", L<"android/content/Intent">(jint)> addFlags = this;

	using ClassProxy::ClassProxy;
};

struct UriProxy : ClassProxy {
	StaticMethod<"parse", L<"android/net/Uri">(jstring)> parse = this;

	using ClassProxy::ClassProxy;
};

struct WindowLayoutParamsProxy : ClassProxy {
	StaticField<"FLAG_TRANSLUCENT_STATUS", jint> FLAG_TRANSLUCENT_STATUS = this;
	StaticField<"FLAG_TRANSLUCENT_NAVIGATION", jint> FLAG_TRANSLUCENT_NAVIGATION = this;
	StaticField<"FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS", jint> FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS = this;
	StaticField<"FLAG_FULLSCREEN", jint> FLAG_FULLSCREEN = this;
	StaticField<"FLAG_LAYOUT_INSET_DECOR", jint> FLAG_LAYOUT_INSET_DECOR = this;
	StaticField<"FLAG_LAYOUT_IN_SCREEN", jint> FLAG_LAYOUT_IN_SCREEN = this;
	StaticField<"FLAG_LAYOUT_ATTACHED_IN_DECOR", jint> FLAG_LAYOUT_ATTACHED_IN_DECOR = this;

	Field<"flags", jint> flags = this;

	using ClassProxy::ClassProxy;
};

struct WindowManagerProxy : ClassProxy {
	Method<"getDefaultDisplay", L<"android/view/Display">()> getDefaultDisplay = this;
	Method<"getCurrentWindowMetrics", L<"android/view/WindowMetrics">()> getCurrentWindowMetrics =
			this;

	using ClassProxy::ClassProxy;
};

struct WindowMetricsProxy : ClassProxy {
	Method<"getBounds", L<"android/graphics/Rect">()> getBounds = this;
	Method<"getDensity", jfloat()> getDensity = this;
	Method<"getWindowInsets", L<"android/view/WindowInsets">()> getWindowInsets = this;

	using ClassProxy::ClassProxy;
};

struct WindowProxy : ClassProxy {
	Method<"addFlags", void(jint)> addFlags = this;
	Method<"clearFlags", void(jint)> clearFlags = this;
	Method<"setFlags", void(jint, jint)> setFlags = this;
	Method<"getAttributes", L<"android/view/WindowManager$LayoutParams">()> getAttributes = this;
	Method<"getInsetsController", L<"android/view/WindowInsetsController">()> getInsetsController =
			this;

	using ClassProxy::ClassProxy;
};

struct DisplayManagerProxy : ClassProxy {
	Global service = nullptr;

	Method<"getDisplays", A<L<"android.view.Display">>()> getDisplays = this;
	Method<"getDisplayTopology", L<"android.hardware.display.DisplayTopology">()>
			getDisplayTopology = this;

	using ClassProxy::ClassProxy;
};

struct DisplayProxy : ClassProxy {
	Method<"getDisplayId", jint()> getDisplayId = this;
	Method<"getName", jstring()> getName = this;
	Method<"getMode", L<"android.view.Display$Mode">()> getMode = this;
	Method<"getDeviceProductInfo", L<"android/hardware/display/DeviceProductInfo">()>
			getDeviceProductInfo = this;
	Method<"getRotation", jint()> getRotation = this;
	Method<"getSupportedModes", A<L<"android.view.Display$Mode">>()> getSupportedModes = this;
	Method<"getSupportedRefreshRates", A<jfloat>()> getSupportedRefreshRates = this;
	Method<"getMetrics", void(L<"android/util/DisplayMetrics">)> getMetrics = this;
	Method<"getRealMetrics", void(L<"android/util/DisplayMetrics">)> getRealMetrics = this;

	using ClassProxy::ClassProxy;
};

struct DisplayModeProxy : ClassProxy {
	Method<"getAlternativeRefreshRates", A<jfloat>()> getAlternativeRefreshRates = this;
	Method<"getModeId", jint()> getModeId = this;
	Method<"getPhysicalHeight", jint()> getPhysicalHeight = this;
	Method<"getPhysicalWidth", jint()> getPhysicalWidth = this;
	Method<"getRefreshRate", jfloat()> getRefreshRate = this;

	using ClassProxy::ClassProxy;
};

struct DeviceProductInfoProxy : ClassProxy {
	Method<"getManufacturerPnpId", jstring()> getManufacturerPnpId = this;
	Method<"getName", jstring()> getName = this;
	Method<"getProductId", jstring()> getProductId = this;

	using ClassProxy::ClassProxy;
};

struct SparseArrayProxy : ClassProxy {
	Method<"size", jint()> size = this;
	Method<"keyAt", jint(jint)> keyAt = this;
	Method<"valueAt", jobject(jint)> valueAt = this;

	using ClassProxy::ClassProxy;
};

struct DisplayTopologyProxy : ClassProxy {
	Method<"getAbsoluteBounds", L<"android.util.SparseArray">()> getAbsoluteBounds = this;

	using ClassProxy::ClassProxy;
};

struct RectFProxy : ClassProxy {
	Field<"bottom", jfloat> bottom = this;
	Field<"left", jfloat> left = this;
	Field<"right", jfloat> right = this;
	Field<"top", jfloat> top = this;

	using ClassProxy::ClassProxy;
};

struct RectProxy : ClassProxy {
	Field<"bottom", jint> bottom = this;
	Field<"left", jint> left = this;
	Field<"right", jint> right = this;
	Field<"top", jint> top = this;

	using ClassProxy::ClassProxy;
};

struct ViewProxy : ClassProxy {
	StaticField<"SYSTEM_UI_FLAG_LAYOUT_STABLE", jint> SYSTEM_UI_FLAG_LAYOUT_STABLE = this;
	StaticField<"SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION", jint>
			SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = this;
	StaticField<"SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN", jint> SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = this;
	StaticField<"SYSTEM_UI_FLAG_HIDE_NAVIGATION", jint> SYSTEM_UI_FLAG_HIDE_NAVIGATION = this;
	StaticField<"SYSTEM_UI_FLAG_FULLSCREEN", jint> SYSTEM_UI_FLAG_FULLSCREEN = this;
	StaticField<"SYSTEM_UI_FLAG_IMMERSIVE_STICKY", jint> SYSTEM_UI_FLAG_IMMERSIVE_STICKY = this;
	StaticField<"SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR", jint> SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR =
			this;
	StaticField<"SYSTEM_UI_FLAG_LIGHT_STATUS_BAR", jint> SYSTEM_UI_FLAG_LIGHT_STATUS_BAR = this;

	using ClassProxy::ClassProxy;
};

struct ClipboardManagerProxy : ClassProxy {
	Global service = nullptr;

	using ClassProxy::ClassProxy;
};

class SP_PUBLIC ClassLoader {
public:
	struct NativePaths {
		LocalString apkPath = nullptr;
		LocalString nativeLibraryDir = nullptr;
	};

	bool init(App *app, JNIEnv *);
	void finalize();

	void foreachMethod(const RefClass &, const Callback<void(StringView, const Ref &)> &);
	void foreachField(const RefClass &,
			const Callback<void(StringView, StringView, const Ref &)> &);

	int getIntField(const Ref &origin, const Ref &field);

	LocalClass findClass(const Env &, StringView);
	LocalClass findClass(const RefString &);

	StringView getApkPath() const { return apkPath; }
	StringView getNativeLibraryDir() const { return nativeLibraryDir; }

protected:
	Global appClassLoader = nullptr;
	GlobalClass appClassLoaderClass = nullptr;

	Global apkClassLoader = nullptr;
	GlobalClass apkClassLoaderClass = nullptr;

	GlobalClass loaderClassClass = nullptr;

	memory::StandartInterface::StringType apkPath;
	memory::StandartInterface::StringType nativeLibraryDir;

	App *app = nullptr;
};

struct SP_PUBLIC App : public sp::Ref {
	JavaVM *vm = nullptr;
	int32_t sdkVersion = 0;
	Global jApplication = nullptr;
	Global jAssetManager = nullptr;
	AAssetManager *nAssetManager = nullptr;
	AConfiguration *config = nullptr;
	ClassLoader classLoader;

	ApplicationProxy Application = SP_JAVA_APPLICATION_CLASS;
	ClassClassProxy Class = "java/lang/Class";
	FileProxy File = "java/io/File";
	ClassMethodProxy Method = "java/lang/reflect/Method";
	ClassFieldProxy Field = "java/lang/reflect/Field";
	SystemProxy System = "java/lang/System";
	WebSettingsProxy WebSettings = "android/webkit/WebSettings";
	DexClassLoaderProxy DexClassLoader = "dalvik/system/DexClassLoader";
	PackageManagerProxy PackageManager = "android/content/pm/PackageManager";
	ApplicationInfoProxy ApplicationInfo = "android/content/pm/ApplicationInfo";
	EnvironmentProxy Environment = "android/os/Environment";
	PackageInfoProxy PackageInfo = "android/content/pm/PackageInfo";
	ResourcesProxy Resources = "android/content/res/Resources";
	DisplayMetricsProxy DisplayMetrics = "android/util/DisplayMetrics";
	IntentProxy Intent = "android/content/Intent";
	UriProxy Uri = "android/net/Uri";
	WindowLayoutParamsProxy WindowLayoutParams = "android/view/WindowManager$LayoutParams";
	WindowManagerProxy WindowManager = "android/view/WindowManager";
	WindowProxy Window = "android/view/Window";
	WindowMetricsProxy WindowMetrics = "android/view/WindowMetrics";
	DisplayManagerProxy DisplayManager = "android/hardware/display/DisplayManager";
	DisplayProxy Display = "android/view/Display";
	DisplayModeProxy DisplayMode = "android/view/Display$Mode";
	DeviceProductInfoProxy DeviceProductInfo = "android/hardware/display/DeviceProductInfo";
	SparseArrayProxy SparseArray = "android/util/SparseArray";
	DisplayTopologyProxy DisplayTopology = "android/hardware/display/DisplayTopology";
	RectFProxy RectF = "android/graphics/RectF";
	RectProxy Rect = "android/graphics/Rect";
	ViewProxy View = "android/view/View";
	ClipboardManagerProxy ClipboardManager = "android/content/ClipboardManager";

	mem_std::Map<mem_std::String, int> drawables;
	mem_std::Function<bool(ANativeActivity *, BytesView)> activityLoader;
	mem_std::Function<void(platform::ApplicationInfo *)> configurationHandler;
	mem_std::Function<void()> lowMemoryHandler;

	// updates automatically when configuration changed
	std::mutex infoMutex;
	Rc<SharedRef<platform::ApplicationInfo>> currentInfo = nullptr;

	virtual ~App();
	App(const RefClass &);

	void handleConfigurationChanged(const jni::Ref &ref);
	void handleLowMemory(const jni::Ref &ref);

	void setActivityLoader(mem_std::Function<bool(ANativeActivity *, BytesView)> &&);
	void setConfigurationHandler(mem_std::Function<void(platform::ApplicationInfo *)> &&);
	void setLowMemoryHandler(mem_std::Function<void()> &&);

	Rc<SharedRef<platform::ApplicationInfo>> makeInfo(const jni::Ref &ref);

	bool loadActivity(ANativeActivity *, BytesView);
};

class SP_PUBLIC Env {
public:
	static Env getEnv();
	static App *getApp();

	static ClassLoader *getClassLoader();

	static void loadJava(JavaVM *);
	static void finalizeJava();

	Env() : _env(nullptr) { }
	Env(JNIEnv *env) : _env(env) { }

	Env(const Env &) = default;
	Env(Env &&) = default;
	Env &operator=(const Env &) = default;
	Env &operator=(Env &&) = default;

	explicit operator bool() const { return _env != nullptr; }

	operator JNIEnv *() const { return _env; }

	JNIEnv *env() const { return _env; }

	template <typename... Args>
	Local newObject(jclass clazz, jmethodID methodID, Args &&...args) const {
		auto ret =
				Local(_env->NewObject(clazz, methodID, Forward(std::forward<Args>(args))...), _env);
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

	LocalClass findClass(const char *name) const {
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
		auto ret = LocalString(data.terminated()
						? _env->NewStringUTF(data.data())
						: _env->NewStringUTF(data.str<memory::StandartInterface>().data()),
				_env);
#if DEBUG
		checkErrors();
#endif
		return ret;
	}

	jobject newGlobalRef(jobject obj) const { return _env->NewGlobalRef(obj); }

	void deleteGlobalRef(jobject obj) const { _env->DeleteGlobalRef(obj); }

	void checkErrors() const;

protected:
	JNIEnv *_env = nullptr;
};

} // namespace stappler::jni

#endif

#endif /* CORE_CORE_PLATFORM_SPCOREJNI_H_ */
