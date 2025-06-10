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

/*
    Note that a lot of methods that you'd expect to find in this file actually
    live in yup_posix_SharedCode.h!
*/

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    FIELD (activityInfo, "activityInfo", "Landroid/content/pm/ActivityInfo;")

DECLARE_JNI_CLASS (AndroidResolveInfo, "android/content/pm/ResolveInfo")
#undef JNI_CLASS_MEMBERS

//==============================================================================
JavaVM* androidJNIJavaVM = nullptr;
jobject androidApkContext = nullptr;

//==============================================================================
JNIEnv* getEnv() noexcept
{
    if (androidJNIJavaVM != nullptr)
    {
        JNIEnv* env;
        androidJNIJavaVM->AttachCurrentThread (&env, nullptr);

        return env;
    }

    // You did not call Thread::initialiseYUP which must be called at least once in your apk
    // before using any YUP APIs.
    jassertfalse;
    return nullptr;
}

static void JNICALL yup_JavainitialiseYUP (JNIEnv* env, jobject /*jclass*/, jobject context)
{
    JNIClassBase::initialiseAllClasses (env, context);
    Thread::initialiseYUP (env, context);
}

extern "C" jint JNIEXPORT yup_JNI_OnLoad (JavaVM* vm, void*)
{
    // Huh? JNI_OnLoad was called two times!
    jassert (androidJNIJavaVM == nullptr);

    androidJNIJavaVM = vm;

    auto* env = getEnv();

    // register the initialisation function
    auto yupJavaClass = env->FindClass ("com/kunitoki/yup/Java");

    if (yupJavaClass != nullptr)
    {
        JNINativeMethod method { "initialiseYUP", "(Landroid/content/Context;)V", reinterpret_cast<void*> (yup_JavainitialiseYUP) };

        auto status = env->RegisterNatives (yupJavaClass, &method, 1);
        jassert (status == 0);
    }
    else
    {
        // com.kunitoki.yup.Java class not found. Apparently this project is a library. That's ok, the user will have to
        // call Thread::initialiseYUP manually
        env->ExceptionClear();
    }

    return JNI_VERSION_1_2;
}

//==============================================================================
class YupActivityWatcher final : public ActivityLifecycleCallbacks
{
public:
    YupActivityWatcher()
    {
        LocalRef<jobject> appContext (getAppContext());

        if (appContext != nullptr)
        {
            auto* env = getEnv();

            myself = GlobalRef (CreateJavaInterface (this, "android/app/Application$ActivityLifecycleCallbacks"));
            env->CallVoidMethod (appContext.get(), AndroidApplication.registerActivityLifecycleCallbacks, myself.get());
        }

        checkActivityIsMain (androidApkContext);
    }

    ~YupActivityWatcher() override
    {
        LocalRef<jobject> appContext (getAppContext());

        if (appContext != nullptr && myself != nullptr)
        {
            auto* env = getEnv();

            env->CallVoidMethod (appContext.get(), AndroidApplication.unregisterActivityLifecycleCallbacks, myself.get());
            clear();
            myself.clear();
        }
    }

    void onActivityStarted (jobject activity) override
    {
        auto* env = getEnv();

        checkActivityIsMain (activity);

        ScopedLock lock (currentActivityLock);

        if (currentActivity != nullptr)
        {
            // see Clarification June 2001 in JNI reference for why this is
            // necessary
            LocalRef<jobject> localStorage (env->NewLocalRef (currentActivity));

            if (env->IsSameObject (localStorage.get(), activity) != 0)
                return;

            env->DeleteWeakGlobalRef (currentActivity);
            currentActivity = nullptr;
        }

        if (activity != nullptr)
            currentActivity = env->NewWeakGlobalRef (activity);
    }

    void onActivityStopped (jobject activity) override
    {
        auto* env = getEnv();

        ScopedLock lock (currentActivityLock);

        if (currentActivity != nullptr)
        {
            // important that the comparison happens in this order
            // to avoid race condition where the weak reference becomes null
            // just after the first check
            if (env->IsSameObject (currentActivity, activity) != 0
                || env->IsSameObject (currentActivity, nullptr) != 0)
            {
                env->DeleteWeakGlobalRef (currentActivity);
                currentActivity = nullptr;
            }
        }
    }

    LocalRef<jobject> getCurrent()
    {
        ScopedLock lock (currentActivityLock);
        return LocalRef<jobject> (getEnv()->NewLocalRef (currentActivity));
    }

    LocalRef<jobject> getMain()
    {
        ScopedLock lock (currentActivityLock);
        return LocalRef<jobject> (getEnv()->NewLocalRef (mainActivity));
    }

    static YupActivityWatcher& getInstance()
    {
        static YupActivityWatcher activityWatcher;
        return activityWatcher;
    }

private:
    void checkActivityIsMain (jobject context)
    {
        auto* env = getEnv();

        ScopedLock lock (currentActivityLock);

        if (mainActivity != nullptr)
        {
            if (env->IsSameObject (mainActivity, nullptr) != 0)
            {
                env->DeleteWeakGlobalRef (mainActivity);
                mainActivity = nullptr;
            }
        }

        if (mainActivity == nullptr)
        {
            LocalRef<jobject> appContext (getAppContext());
            auto mainActivityPath = getMainActivityClassPath();

            if (mainActivityPath.isNotEmpty())
            {
                auto clasz = env->GetObjectClass (context);
                auto activityPath = yupString (LocalRef<jstring> ((jstring) env->CallObjectMethod (clasz, JavaClass.getName)));

                // This may be problematic for apps which use several activities with the same type. We just
                // assume that the very first activity of this type is the main one
                if (activityPath == mainActivityPath)
                    mainActivity = env->NewWeakGlobalRef (context);
            }
        }
    }

    static String getMainActivityClassPath()
    {
        static String mainActivityClassPath;

        if (mainActivityClassPath.isEmpty())
        {
            LocalRef<jobject> appContext (getAppContext());

            if (appContext != nullptr)
            {
                auto* env = getEnv();

                LocalRef<jobject> pkgManager (env->CallObjectMethod (appContext.get(), AndroidContext.getPackageManager));
                LocalRef<jstring> pkgName ((jstring) env->CallObjectMethod (appContext.get(), AndroidContext.getPackageName));

                LocalRef<jobject> intent (env->NewObject (AndroidIntent, AndroidIntent.constructWithString, javaString ("android.intent.action.MAIN").get()));

                intent = LocalRef<jobject> (env->CallObjectMethod (intent.get(),
                                                                   AndroidIntent.setPackage,
                                                                   pkgName.get()));

                LocalRef<jobject> resolveInfo (env->CallObjectMethod (pkgManager.get(), AndroidPackageManager.resolveActivity, intent.get(), 0));

                if (resolveInfo != nullptr)
                {
                    LocalRef<jobject> activityInfo (env->GetObjectField (resolveInfo.get(), AndroidResolveInfo.activityInfo));
                    LocalRef<jstring> jName ((jstring) env->GetObjectField (activityInfo.get(), AndroidPackageItemInfo.name));
                    LocalRef<jstring> jPackage ((jstring) env->GetObjectField (activityInfo.get(), AndroidPackageItemInfo.packageName));

                    mainActivityClassPath = yupString (jName);
                }
            }
        }

        return mainActivityClassPath;
    }

    GlobalRef myself;
    CriticalSection currentActivityLock;
    jweak currentActivity = nullptr;
    jweak mainActivity = nullptr;
};

//==============================================================================
#if YUP_MODULE_AVAILABLE_yup_events && YUP_ANDROID
void yup_yupEventsAndroidStartApp();
#endif

void Thread::initialiseYUP (void* jniEnv, void* context)
{
    static CriticalSection cs;
    ScopedLock lock (cs);

    // jniEnv and context should not be null!
    jassert (jniEnv != nullptr && context != nullptr);

    auto* env = static_cast<JNIEnv*> (jniEnv);

    if (androidJNIJavaVM == nullptr)
    {
        JavaVM* javaVM = nullptr;

        auto status = env->GetJavaVM (&javaVM);
        jassert (status == 0 && javaVM != nullptr);

        androidJNIJavaVM = javaVM;
    }

    static bool firstCall = true;

    if (firstCall)
    {
        firstCall = false;

        // if we ever support unloading then this should probably be a weak reference
        androidApkContext = env->NewGlobalRef (static_cast<jobject> (context));
        YupActivityWatcher::getInstance();

#if YUP_MODULE_AVAILABLE_yup_events && YUP_ANDROID
        yup_yupEventsAndroidStartApp();
#endif
    }
}

//==============================================================================
LocalRef<jobject> getAppContext() noexcept
{
    auto* env = getEnv();
    auto context = androidApkContext;

    // You did not call Thread::initialiseYUP which must be called at least once in your apk
    // before using any YUP APIs.
    jassert (env != nullptr && context != nullptr);

    if (context == nullptr)
        return LocalRef<jobject>();

    if (env->IsInstanceOf (context, AndroidApplication) != 0)
        return LocalRef<jobject> (env->NewLocalRef (context));

    LocalRef<jobject> applicationContext (env->CallObjectMethod (context, AndroidContext.getApplicationContext));

    if (applicationContext == nullptr)
        return LocalRef<jobject> (env->NewLocalRef (context));

    return applicationContext;
}

LocalRef<jobject> getCurrentActivity() noexcept
{
    return YupActivityWatcher::getInstance().getCurrent();
}

LocalRef<jobject> getMainActivity() noexcept
{
    return YupActivityWatcher::getInstance().getMain();
}

//==============================================================================
using RealtimeThreadFactory = pthread_t (*) (void* (*entry) (void*), void* userPtr);
// This is defined in the yup_audio_devices module, with different definitions depending on
// whether OpenSL/Oboe are enabled.
RealtimeThreadFactory getAndroidRealtimeThreadFactory();

#if ! YUP_MODULE_AVAILABLE_yup_audio_devices
RealtimeThreadFactory getAndroidRealtimeThreadFactory()
{
    return nullptr;
}
#endif

extern JavaVM* androidJNIJavaVM;

static auto setPriorityOfThisThread (Thread::Priority p)
{
    return setpriority (PRIO_PROCESS,
                        (id_t) gettid(),
                        ThreadPriorities::getNativePriority (p))
        == 0;
}

bool Thread::createNativeThread (Priority)
{
    const auto threadEntryProc = [] (void* userData) -> void*
    {
        auto* myself = static_cast<Thread*> (userData);

        setPriorityOfThisThread (myself->priority);

        yup_threadEntryPoint (myself);

        if (androidJNIJavaVM != nullptr)
        {
            void* env = nullptr;
            androidJNIJavaVM->GetEnv (&env, JNI_VERSION_1_2);

            // only detach if we have actually been attached
            if (env != nullptr)
                androidJNIJavaVM->DetachCurrentThread();
        }

        return nullptr;
    };

    if (isRealtime())
    {
        if (const auto factory = getAndroidRealtimeThreadFactory())
        {
            threadHandle = (void*) factory (threadEntryProc, this);
            threadId = (ThreadID) threadHandle.load();
            return threadId != nullptr;
        }
        else
        {
            jassertfalse;
        }
    }

    PosixThreadAttribute attr { threadStackSize };
    threadId = threadHandle = makeThreadHandle (attr, this, threadEntryProc);

    return threadId != nullptr;
}

void Thread::killThread()
{
    if (threadHandle != nullptr)
        jassertfalse; // pthread_cancel not available!
}

Thread::Priority Thread::getPriority() const
{
    jassert (Thread::getCurrentThreadId() == getThreadId());

    const auto native = getpriority (PRIO_PROCESS, (id_t) gettid());
    return ThreadPriorities::getYupPriority (native);
}

bool Thread::setPriority (Priority priorityIn)
{
    jassert (Thread::getCurrentThreadId() == getThreadId());

    if (isRealtime())
        return false;

    const auto priorityToUse = priority = priorityIn;
    return setPriorityOfThisThread (priorityToUse) == 0;
}

//==============================================================================
YUP_API void YUP_CALLTYPE Process::setPriority (ProcessPriority) {}

YUP_API bool YUP_CALLTYPE yup_isRunningUnderDebugger() noexcept
{
    StringArray lines;
    File ("/proc/self/status").readLines (lines);

    for (int i = lines.size(); --i >= 0;) // (NB - it's important that this runs in reverse order)
        if (lines[i].upToFirstOccurrenceOf (":", false, false).trim().equalsIgnoreCase ("TracerPid"))
            return (lines[i].fromFirstOccurrenceOf (":", false, false).trim().getIntValue() > 0);

    return false;
}

YUP_API void YUP_CALLTYPE Process::raisePrivilege() {}

YUP_API void YUP_CALLTYPE Process::lowerPrivilege() {}

} // namespace yup
