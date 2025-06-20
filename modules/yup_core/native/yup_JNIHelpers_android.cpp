/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

//==============================================================================
#include "generated/yup_YupInvocationHandler_bytecode.h"
#define invocationHandleByteCode yup::javaYupInvocationHandlerBytecode

//==============================================================================
#include "generated/yup_FragmentOverlay_bytecode.h"
#define javaFragmentOverlay yup::javaFragmentOverlayBytecode

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    STATICMETHOD (newProxyInstance, "newProxyInstance", "(Ljava/lang/ClassLoader;[Ljava/lang/Class;Ljava/lang/reflect/InvocationHandler;)Ljava/lang/Object;")

DECLARE_JNI_CLASS (JavaProxy, "java/lang/reflect/Proxy")
#undef JNI_CLASS_MEMBERS

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK)                                                                  \
    METHOD (constructor, "<init>", "(J)V")                                                                                                     \
    METHOD (clear, "clear", "()V")                                                                                                             \
    CALLBACK (yup_invokeImplementer, "dispatchInvoke", "(JLjava/lang/Object;Ljava/lang/reflect/Method;[Ljava/lang/Object;)Ljava/lang/Object;") \
    CALLBACK (yup_dispatchDelete, "dispatchFinalize", "(J)V")

DECLARE_JNI_CLASS_WITH_BYTECODE (YupInvocationHandler, "org/kunitoki/yup/YupInvocationHandler", 10, invocationHandleByteCode)
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (loadClass, "loadClass", "(Ljava/lang/String;Z)Ljava/lang/Class;") \
    STATICMETHOD (getSystemClassLoader, "getSystemClassLoader", "()Ljava/lang/ClassLoader;")

DECLARE_JNI_CLASS (JavaClassLoader, "java/lang/ClassLoader")
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (constructor, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V")

DECLARE_JNI_CLASS (AndroidDexClassLoader, "dalvik/system/DexClassLoader")
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (constructor, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V")

DECLARE_JNI_CLASS_WITH_MIN_SDK (AndroidInMemoryDexClassLoader, "dalvik/system/InMemoryDexClassLoader", 26)
#undef JNI_CLASS_MEMBERS

//==============================================================================
struct SystemJavaClassComparator
{
    static int compareElements (JNIClassBase* first, JNIClassBase* second)
    {
        auto isSysClassA = isSystemClass (first);
        auto isSysClassB = isSystemClass (second);

        if ((! isSysClassA) && (! isSysClassB))
        {
            return DefaultElementComparator<bool>::compareElements (first != nullptr && first->byteCode != nullptr,
                                                                    second != nullptr && second->byteCode != nullptr);
        }

        return DefaultElementComparator<bool>::compareElements (isSystemClass (first),
                                                                isSystemClass (second));
    }

    static bool isSystemClass (JNIClassBase* cls)
    {
        if (cls == nullptr)
            return false;

        String path (cls->getClassPath());

        return path.startsWith ("java/")
            || path.startsWith ("android/")
            || path.startsWith ("dalvik/");
    }
};

//==============================================================================
JNIClassBase::JNIClassBase (const char* cp, int classMinSDK, const uint8* bc, size_t n)
    : classPath (cp)
    , byteCode (bc)
    , byteCodeSize (n)
    , minSDK (classMinSDK)
    , classRef (nullptr)
{
    SystemJavaClassComparator comparator;

    getClasses().addSorted (comparator, this);
}

JNIClassBase::~JNIClassBase()
{
    getClasses().removeFirstMatchingValue (this);
}

Array<JNIClassBase*>& JNIClassBase::getClasses()
{
    static Array<JNIClassBase*> classes;
    return classes;
}

// Get code cache directory without yet having a context object
static File getCodeCacheDirectory()
{
    int pid = getpid();
    File cmdline ("/proc/" + String (pid) + "/cmdline");

    auto bundleId = cmdline.loadFileAsString().trimStart().trimEnd();

    if (bundleId.isEmpty())
        return {};

    return File ("/data/data/" + bundleId + "/code_cache");
}

void JNIClassBase::initialise (JNIEnv* env, jobject context)
{
    auto sdkVersion = getAndroidSDKVersion();

    if (sdkVersion >= minSDK)
    {
        LocalRef<jstring> classNameAndPackage (javaString (String (classPath).replaceCharacter (L'/', L'.')));
        static Array<GlobalRef> byteCodeLoaders;

        if (! SystemJavaClassComparator::isSystemClass (this))
        {
            // We use the context's class loader, rather than the 'system' class loader, because we
            // may need to load classes from our library dependencies (such as the BillingClient
            // library), and the system class loader is not aware of those libraries.
            const LocalRef<jobject> defaultClassLoader { env->CallObjectMethod (context,
                                                                                env->GetMethodID (env->FindClass ("android/content/Context"),
                                                                                                  "getClassLoader",
                                                                                                  "()Ljava/lang/ClassLoader;")) };

            tryLoadingClassWithClassLoader (env, defaultClassLoader.get());

            if (classRef == nullptr)
            {
                for (auto& byteCodeLoader : byteCodeLoaders)
                {
                    tryLoadingClassWithClassLoader (env, byteCodeLoader.get());

                    if (classRef != nullptr)
                        break;
                }

                // fallback by trying to load the class from bytecode
                if (byteCode != nullptr)
                {
                    LocalRef<jobject> byteCodeClassLoader;

                    MemoryOutputStream uncompressedByteCode;

                    {
                        MemoryInputStream rawGZipData (byteCode, byteCodeSize, false);
                        GZIPDecompressorInputStream gzipStream (&rawGZipData, false, GZIPDecompressorInputStream::gzipFormat);
                        uncompressedByteCode.writeFromInputStream (gzipStream, -1);
                    }

                    if (sdkVersion >= 26)
                    {
                        LocalRef<jbyteArray> byteArray (env->NewByteArray ((jsize) uncompressedByteCode.getDataSize()));
                        jboolean isCopy;
                        auto* dst = env->GetByteArrayElements (byteArray.get(), &isCopy);
                        memcpy (dst, uncompressedByteCode.getData(), uncompressedByteCode.getDataSize());
                        env->ReleaseByteArrayElements (byteArray.get(), dst, 0);

                        LocalRef<jobject> byteBuffer (env->CallStaticObjectMethod (JavaByteBuffer, JavaByteBuffer.wrap, byteArray.get()));

                        byteCodeClassLoader = LocalRef<jobject> (env->NewObject (AndroidInMemoryDexClassLoader,
                                                                                 AndroidInMemoryDexClassLoader.constructor,
                                                                                 byteBuffer.get(),
                                                                                 defaultClassLoader.get()));
                    }
                    else if (uncompressedByteCode.getDataSize() >= 32)
                    {
                        auto codeCacheDir = getCodeCacheDirectory();

                        // The dex file has an embedded 20-byte long SHA-1 signature at offset 12
                        auto fileName = String::toHexString ((char*) uncompressedByteCode.getData() + 12, 20, 0) + ".dex";
                        auto dexFile = codeCacheDir.getChildFile (fileName);
                        auto optimizedDirectory = codeCacheDir.getChildFile ("optimized_cache");
                        optimizedDirectory.createDirectory();

                        if (dexFile.replaceWithData (uncompressedByteCode.getData(), uncompressedByteCode.getDataSize()))
                        {
                            byteCodeClassLoader = LocalRef<jobject> (env->NewObject (AndroidDexClassLoader,
                                                                                     AndroidDexClassLoader.constructor,
                                                                                     javaString (dexFile.getFullPathName()).get(),
                                                                                     javaString (optimizedDirectory.getFullPathName()).get(),
                                                                                     nullptr,
                                                                                     defaultClassLoader.get()));
                        }
                        else
                        {
                            // can't write to cache folder
                            jassertfalse;
                        }
                    }

                    if (byteCodeClassLoader != nullptr)
                    {
                        tryLoadingClassWithClassLoader (env, byteCodeClassLoader.get());
                        byteCodeLoaders.add (GlobalRef (byteCodeClassLoader));
                    }
                }
            }
        }

        if (classRef == nullptr)
            classRef = (jclass) env->NewGlobalRef (LocalRef<jobject> (env->FindClass (classPath)));

        jassert (classRef != nullptr);
        initialiseFields (env);
    }
}

void JNIClassBase::tryLoadingClassWithClassLoader (JNIEnv* env, jobject classLoader)
{
    LocalRef<jstring> classNameAndPackage (javaString (String (classPath).replaceCharacter (L'/', L'.')));

    // Android SDK <= 19 has a bug where the class loader might throw an exception but still return
    // a non-nullptr. So don't assign the result of this call to a jobject just yet...
    auto classObj = env->CallObjectMethod (classLoader, JavaClassLoader.loadClass, classNameAndPackage.get(), (jboolean) true);

    if (jthrowable exception = env->ExceptionOccurred())
    {
        env->ExceptionClear();
        classObj = nullptr;
    }

    // later versions of Android don't throw at all, so re-check the object
    if (classObj != nullptr)
        classRef = (jclass) env->NewGlobalRef (LocalRef<jobject> (classObj));
}

void JNIClassBase::release (JNIEnv* env)
{
    if (classRef != nullptr)
        env->DeleteGlobalRef (classRef);
}

void JNIClassBase::initialiseAllClasses (JNIEnv* env, jobject context)
{
    const Array<JNIClassBase*>& classes = getClasses();
    for (int i = classes.size(); --i >= 0;)
        classes.getUnchecked (i)->initialise (env, context);
}

void JNIClassBase::releaseAllClasses (JNIEnv* env)
{
    const Array<JNIClassBase*>& classes = getClasses();
    for (int i = classes.size(); --i >= 0;)
        classes.getUnchecked (i)->release (env);
}

jmethodID JNIClassBase::resolveMethod (JNIEnv* env, const char* methodName, const char* params)
{
    jmethodID m = env->GetMethodID (classRef, methodName, params);
    jassert (m != nullptr);
    return m;
}

jmethodID JNIClassBase::resolveStaticMethod (JNIEnv* env, const char* methodName, const char* params)
{
    jmethodID m = env->GetStaticMethodID (classRef, methodName, params);
    jassert (m != nullptr);
    return m;
}

jfieldID JNIClassBase::resolveField (JNIEnv* env, const char* fieldName, const char* signature)
{
    jfieldID f = env->GetFieldID (classRef, fieldName, signature);
    jassert (f != nullptr);
    return f;
}

jfieldID JNIClassBase::resolveStaticField (JNIEnv* env, const char* fieldName, const char* signature)
{
    jfieldID f = env->GetStaticFieldID (classRef, fieldName, signature);
    jassert (f != nullptr);
    return f;
}

void JNIClassBase::resolveCallbacks (JNIEnv* env, const Array<JNINativeMethod>& nativeCallbacks)
{
    if (nativeCallbacks.size() > 0)
        env->RegisterNatives (classRef, nativeCallbacks.begin(), (jint) nativeCallbacks.size());
}

//==============================================================================
LocalRef<jobject> CreateJavaInterface (AndroidInterfaceImplementer* implementer,
                                       const StringArray& interfaceNames,
                                       LocalRef<jobject> subclass)
{
    auto* env = getEnv();

    implementer->javaSubClass = GlobalRef (subclass);

    // you need to override at least one interface
    jassert (interfaceNames.size() > 0);

    auto classArray = LocalRef<jobject> (env->NewObjectArray (interfaceNames.size(), JavaClass, nullptr));
    LocalRef<jobject> classLoader;

    for (auto i = 0; i < interfaceNames.size(); ++i)
    {
        auto aClass = LocalRef<jobject> (env->FindClass (interfaceNames[i].toRawUTF8()));

        if (aClass != nullptr)
        {
            if (i == 0)
                classLoader = LocalRef<jobject> (env->CallObjectMethod (aClass, JavaClass.getClassLoader));

            env->SetObjectArrayElement ((jobjectArray) classArray.get(), i, aClass);
        }
        else
        {
            // interface class not found
            jassertfalse;
        }
    }

    auto invocationHandler = LocalRef<jobject> (env->NewObject (YupInvocationHandler, YupInvocationHandler.constructor, reinterpret_cast<jlong> (implementer)));

    // CreateJavaInterface() is expected to be called just once for a given implementer
    jassert (implementer->invocationHandler == nullptr);

    implementer->invocationHandler = GlobalRef (invocationHandler);

    return LocalRef<jobject> (env->CallStaticObjectMethod (JavaProxy, JavaProxy.newProxyInstance, classLoader.get(), classArray.get(), invocationHandler.get()));
}

LocalRef<jobject> CreateJavaInterface (AndroidInterfaceImplementer* implementer,
                                       const StringArray& interfaceNames)
{
    return CreateJavaInterface (implementer, interfaceNames, LocalRef<jobject> (getEnv()->NewObject (JavaObject, JavaObject.constructor)));
}

LocalRef<jobject> CreateJavaInterface (AndroidInterfaceImplementer* implementer,
                                       const String& interfaceName)
{
    return CreateJavaInterface (implementer, StringArray (interfaceName));
}

AndroidInterfaceImplementer::~AndroidInterfaceImplementer()
{
    clear();
}

void AndroidInterfaceImplementer::clear()
{
    if (invocationHandler != nullptr)
        getEnv()->CallVoidMethod (invocationHandler,
                                  YupInvocationHandler.clear);
}

jobject AndroidInterfaceImplementer::invoke (jobject /*proxy*/, jobject method, jobjectArray args)
{
    auto* env = getEnv();
    return env->CallObjectMethod (method, JavaMethod.invoke, javaSubClass.get(), args);
}

jobject yup_invokeImplementer (JNIEnv*, jobject /*object*/, jlong host, jobject proxy, jobject method, jobjectArray args)
{
    if (auto* myself = reinterpret_cast<AndroidInterfaceImplementer*> (host))
        return myself->invoke (proxy, method, args);

    return nullptr;
}

void yup_dispatchDelete (JNIEnv*, jobject /*object*/, jlong host)
{
    if (auto* myself = reinterpret_cast<AndroidInterfaceImplementer*> (host))
        delete myself;
}

//==============================================================================
jobject ActivityLifecycleCallbacks::invoke (jobject proxy, jobject method, jobjectArray args)
{
    auto* env = getEnv();

    struct Comparator
    {
        bool operator() (const char* a, const char* b) const
        {
            return CharPointer_ASCII { a }.compare (CharPointer_ASCII { b }) < 0;
        }
    };

    // clang-format off
    static const std::map<const char*, void (*) (ActivityLifecycleCallbacks&, jobject, jobject), Comparator> entries
    {
        { "onActivityConfigurationChanged", [] (auto& t, auto activity, auto) { t.onActivityConfigurationChanged (activity); } },
        { "onActivityCreated", [] (auto& t, auto activity, auto bundle) { t.onActivityCreated (activity, bundle); } },
        { "onActivityDestroyed", [] (auto& t, auto activity, auto) { t.onActivityDestroyed (activity); } },
        { "onActivityPaused", [] (auto& t, auto activity, auto) { t.onActivityPaused (activity); } },
        { "onActivityPostCreated", [] (auto& t, auto activity, auto bundle) { t.onActivityPostCreated (activity, bundle); } },
        { "onActivityPostDestroyed", [] (auto& t, auto activity, auto) { t.onActivityPostDestroyed (activity); } },
        { "onActivityPostPaused", [] (auto& t, auto activity, auto) { t.onActivityPostPaused (activity); } },
        { "onActivityPostResumed", [] (auto& t, auto activity, auto) { t.onActivityPostResumed (activity); } },
        { "onActivityPostSaveInstanceState", [] (auto& t, auto activity, auto bundle) { t.onActivityPostSaveInstanceState (activity, bundle); } },
        { "onActivityPostStarted", [] (auto& t, auto activity, auto) { t.onActivityPostStarted (activity); } },
        { "onActivityPostStopped", [] (auto& t, auto activity, auto) { t.onActivityPostStopped (activity); } },
        { "onActivityPreCreated", [] (auto& t, auto activity, auto bundle) { t.onActivityPreCreated (activity, bundle); } },
        { "onActivityPreDestroyed", [] (auto& t, auto activity, auto) { t.onActivityPreDestroyed (activity); } },
        { "onActivityPrePaused", [] (auto& t, auto activity, auto) { t.onActivityPrePaused (activity); } },
        { "onActivityPreResumed", [] (auto& t, auto activity, auto) { t.onActivityPreResumed (activity); } },
        { "onActivityPreSaveInstanceState", [] (auto& t, auto activity, auto bundle) { t.onActivityPreSaveInstanceState (activity, bundle); } },
        { "onActivityPreStarted", [] (auto& t, auto activity, auto) { t.onActivityPreStarted (activity); } },
        { "onActivityPreStopped", [] (auto& t, auto activity, auto) { t.onActivityPreStopped (activity); } },
        { "onActivityResumed", [] (auto& t, auto activity, auto) { t.onActivityResumed (activity); } },
        { "onActivitySaveInstanceState", [] (auto& t, auto activity, auto bundle) { t.onActivitySaveInstanceState (activity, bundle); } },
        { "onActivityStarted", [] (auto& t, auto activity, auto) { t.onActivityStarted (activity); } },
        { "onActivityStopped", [] (auto& t, auto activity, auto) { t.onActivityStopped (activity); } },
    };
    // clang-format on

    const auto methodName = yupString ((jstring) env->CallObjectMethod (method, JavaMethod.getName));
    const auto iter = entries.find (methodName.toRawUTF8());

    if (iter == entries.end())
        return AndroidInterfaceImplementer::invoke (proxy, method, args);

    const auto activity = env->GetArrayLength (args) > 0 ? env->GetObjectArrayElement (args, 0) : (jobject) nullptr;
    const auto bundle = env->GetArrayLength (args) > 1 ? env->GetObjectArrayElement (args, 1) : (jobject) nullptr;
    (iter->second) (*this, activity, bundle);

    return nullptr;
}

//==============================================================================
int getAndroidSDKVersion()
{
    // this is used so often that we need to cache this
    static int sdkVersion = []
    {
        // don't use any jni helpers as they might not have been initialised yet
        // when this method is used
        auto* env = getEnv();

        auto buildVersion = env->FindClass ("android/os/Build$VERSION");
        jassert (buildVersion != nullptr);

        auto sdkVersionField = env->GetStaticFieldID (buildVersion, "SDK_INT", "I");
        jassert (sdkVersionField != nullptr);

        return env->GetStaticIntField (buildVersion, sdkVersionField);
    }();

    return sdkVersion;
}

bool isPermissionDeclaredInManifest (const String& requestedPermission)
{
    auto* env = getEnv();

    LocalRef<jobject> pkgManager (env->CallObjectMethod (getAppContext().get(), AndroidContext.getPackageManager));
    LocalRef<jobject> pkgName (env->CallObjectMethod (getAppContext().get(), AndroidContext.getPackageName));
    LocalRef<jobject> pkgInfo (env->CallObjectMethod (pkgManager.get(), AndroidPackageManager.getPackageInfo, pkgName.get(), 0x00001000 /* PERMISSIONS */));

    LocalRef<jobjectArray> permissions ((jobjectArray) env->GetObjectField (pkgInfo.get(), AndroidPackageInfo.requestedPermissions));
    int n = env->GetArrayLength (permissions);

    for (int i = 0; i < n; ++i)
    {
        LocalRef<jstring> jstr ((jstring) env->GetObjectArrayElement (permissions, i));
        String permissionId (yupString (jstr));

        if (permissionId == requestedPermission)
            return true;
    }

    return false;
}

//==============================================================================
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK)                                                            \
    METHOD (construct, "<init>", "()V")                                                                                                  \
    METHOD (close, "close", "()V")                                                                                                       \
    CALLBACK (generatedCallback<&FragmentOverlay::onActivityResultCallback>, "onActivityResultNative", "(JIILandroid/content/Intent;)V") \
    CALLBACK (generatedCallback<&FragmentOverlay::onCreatedCallback>, "onCreateNative", "(JLandroid/os/Bundle;)V")                       \
    CALLBACK (generatedCallback<&FragmentOverlay::onStartCallback>, "onStartNative", "(J)V")                                             \
    CALLBACK (generatedCallback<&FragmentOverlay::onRequestPermissionsResultCallback>, "onRequestPermissionsResultNative", "(JI[Ljava/lang/String;[I)V")

DECLARE_JNI_CLASS_WITH_BYTECODE (YupFragmentOverlay, "org/kunitoki/yup/FragmentOverlay", 16, javaFragmentOverlay)
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (show, "show", "(Landroid/app/FragmentManager;Ljava/lang/String;)V")

DECLARE_JNI_CLASS (AndroidDialogFragment, "android/app/DialogFragment")
#undef JNI_CLASS_MEMBERS

//==============================================================================
FragmentOverlay::FragmentOverlay()
    : native (LocalRef<jobject> (getEnv()->NewObject (YupFragmentOverlay, YupFragmentOverlay.construct)))
{
}

FragmentOverlay::~FragmentOverlay()
{
    auto* env = getEnv();

    env->CallVoidMethod (native.get(), YupFragmentOverlay.close);
}

void FragmentOverlay::open()
{
    auto* env = getEnv();

    LocalRef<jobject> bundle (env->NewObject (AndroidBundle, AndroidBundle.constructor));
    env->CallVoidMethod (bundle.get(), AndroidBundle.putLong, javaString ("cppThis").get(), (jlong) this);
    env->CallVoidMethod (native.get(), AndroidFragment.setArguments, bundle.get());

    LocalRef<jobject> fm (env->CallObjectMethod (getMainActivity().get(), AndroidActivity.getFragmentManager));
    env->CallVoidMethod (native.get(), AndroidDialogFragment.show, fm.get(), javaString ("FragmentOverlay").get());
}

void FragmentOverlay::onCreatedCallback (JNIEnv* env, FragmentOverlay& t, jobject obj)
{
    t.onCreated (LocalRef<jobject> { env->NewLocalRef (obj) });
}

void FragmentOverlay::onStartCallback (JNIEnv*, FragmentOverlay& t)
{
    t.onStart();
}

void FragmentOverlay::onRequestPermissionsResultCallback (JNIEnv* env, FragmentOverlay& t, jint requestCode, jobjectArray jPermissions, jintArray jGrantResults)
{
    Array<int> grantResults;
    int n = (jGrantResults != nullptr ? env->GetArrayLength (jGrantResults) : 0);

    if (n > 0)
    {
        auto* data = env->GetIntArrayElements (jGrantResults, nullptr);

        for (int i = 0; i < n; ++i)
            grantResults.add (data[i]);

        env->ReleaseIntArrayElements (jGrantResults, data, 0);
    }

    t.onRequestPermissionsResult (requestCode,
                                  javaStringArrayToYup (LocalRef<jobjectArray> (jPermissions)),
                                  grantResults);
}

void FragmentOverlay::onActivityResultCallback (JNIEnv* env, FragmentOverlay& t, jint requestCode, jint resultCode, jobject data)
{
    t.onActivityResult (requestCode, resultCode, LocalRef<jobject> (env->NewLocalRef (data)));
}

jobject FragmentOverlay::getNativeHandle()
{
    return native.get();
}

//==============================================================================
void startAndroidActivityForResult (const LocalRef<jobject>& intent,
                                    int requestCode,
                                    std::function<void (int, int, LocalRef<jobject>)>&& callback)
{
    auto* launcher = new ActivityLauncher (intent, requestCode);
    launcher->callback = [launcher, c = std::move (callback)] (auto&&... args)
    {
        NullCheckedInvocation::invoke (c, args...);
        delete launcher;
    };
    launcher->open();
}

//==============================================================================
bool androidHasSystemFeature (const String& property)
{
    LocalRef<jobject> appContext (getAppContext());

    if (appContext != nullptr)
    {
        auto* env = getEnv();

        LocalRef<jobject> packageManager (env->CallObjectMethod (appContext.get(), AndroidContext.getPackageManager));

        if (packageManager != nullptr)
            return env->CallBooleanMethod (packageManager.get(),
                                           AndroidPackageManager.hasSystemFeature,
                                           javaString (property).get())
                != 0;
    }

    // unable to get app's context
    jassertfalse;
    return false;
}

String audioManagerGetProperty (const String& property)
{
    if (getAndroidSDKVersion() >= 17)
    {
        auto* env = getEnv();
        LocalRef<jobject> audioManager (env->CallObjectMethod (getAppContext().get(), AndroidContext.getSystemService, javaString ("audio").get()));

        if (audioManager != nullptr)
        {
            LocalRef<jstring> jProperty (javaString (property));

            auto methodID = env->GetMethodID (AndroidAudioManager, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");

            if (methodID != nullptr)
                return yupString (LocalRef<jstring> ((jstring) env->CallObjectMethod (audioManager.get(),
                                                                                      methodID,
                                                                                      javaString (property).get())));
        }
    }

    return {};
}

} // namespace yup
