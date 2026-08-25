/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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
*/

/*
    The Android backend posts notifications through the system
    NotificationManager. Notifications are built with Notification.Builder, use
    the application icon as the small icon, open the launcher activity when
    tapped, and rely on a default NotificationChannel on API 26 and above.
    Action buttons fire broadcasts that the application may handle with a
    BroadcastReceiver; activation callbacks cannot be delivered back to native
    code without one, so onActivatedWithAction is not invoked on Android.
*/

#if YUP_ANDROID

namespace yup
{
namespace detail
{

namespace
{

//==============================================================================
// Notification-related JNI classes, declared here to keep them close to their
// only user. The JNI class registry does not deduplicate class paths, so these
// declarations coexist with the ones in yup_JNIHelpers_android.h.
#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK)                               \
    METHOD (notify, "notify", "(ILandroid/app/Notification;)V")                                             \
    METHOD (cancel, "cancel", "(I)V")                                                                       \
    METHOD (cancelAll, "cancelAll", "()V")                                                                  \
    METHOD (createNotificationChannel, "createNotificationChannel", "(Landroid/app/NotificationChannel;)V") \
    METHOD (getNotificationChannel, "getNotificationChannel", "(Ljava/lang/String;)Landroid/app/NotificationChannel;")

DECLARE_JNI_CLASS_WITH_MIN_SDK (AndroidNotificationManager, "android/app/NotificationManager", 16)
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (constructor, "<init>", "(Ljava/lang/String;Ljava/lang/CharSequence;I)V")

DECLARE_JNI_CLASS_WITH_MIN_SDK (AndroidNotificationChannel, "android/app/NotificationChannel", 26)
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK)                                                   \
    METHOD (constructor, "<init>", "(Landroid/content/Context;)V")                                                              \
    METHOD (setSmallIcon, "setSmallIcon", "(I)Landroid/app/Notification$Builder;")                                              \
    METHOD (setContentTitle, "setContentTitle", "(Ljava/lang/CharSequence;)Landroid/app/Notification$Builder;")                 \
    METHOD (setContentText, "setContentText", "(Ljava/lang/CharSequence;)Landroid/app/Notification$Builder;")                   \
    METHOD (setContentIntent, "setContentIntent", "(Landroid/app/PendingIntent;)Landroid/app/Notification$Builder;")            \
    METHOD (setAutoCancel, "setAutoCancel", "(Z)Landroid/app/Notification$Builder;")                                            \
    METHOD (setChannelId, "setChannelId", "(Ljava/lang/String;)Landroid/app/Notification$Builder;")                             \
    METHOD (setTimeoutAfter, "setTimeoutAfter", "(J)Landroid/app/Notification$Builder;")                                        \
    METHOD (addAction, "addAction", "(ILjava/lang/CharSequence;Landroid/app/PendingIntent;)Landroid/app/Notification$Builder;") \
    METHOD (setStyle, "setStyle", "(Landroid/app/Notification$Style;)Landroid/app/Notification$Builder;")                       \
    METHOD (build, "build", "()Landroid/app/Notification;")

DECLARE_JNI_CLASS_WITH_MIN_SDK (AndroidNotificationBuilder, "android/app/Notification$Builder", 16)
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    FIELD (icon, "icon", "I")

DECLARE_JNI_CLASS (AndroidApplicationInfo, "android/content/pm/ApplicationInfo")
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (constructor, "<init>", "()V")                                     \
    METHOD (bigText, "bigText", "(Ljava/lang/CharSequence;)Landroid/app/Notification$BigTextStyle;")

DECLARE_JNI_CLASS (AndroidBigTextStyle, "android/app/Notification$BigTextStyle")
#undef JNI_CLASS_MEMBERS

#define JNI_CLASS_MEMBERS(METHOD, STATICMETHOD, FIELD, STATICFIELD, CALLBACK) \
    METHOD (constructor, "<init>", "()V")                                     \
    METHOD (bigPicture, "bigPicture", "(Landroid/graphics/Bitmap;)Landroid/app/Notification$BigPictureStyle;")

DECLARE_JNI_CLASS (AndroidBigPictureStyle, "android/app/Notification$BigPictureStyle")
#undef JNI_CLASS_MEMBERS

//==============================================================================
// The per-process state of the toast backend.
struct AndroidToastState
{
    yup::Atomic<int32> nextId { 1 };
    bool initialized { false };
};

AndroidToastState& getToastState()
{
    static AndroidToastState state;
    return state;
}

//==============================================================================
constexpr int notificationChannelImportance (const ToastTemplate& toast)
{
    if (toast.getAudioOption() == ToastTemplate::AudioOption::silent)
        return 2; // IMPORTANCE_LOW

    switch (toast.getScenario())
    {
        case ToastTemplate::Scenario::alarm:
        case ToastTemplate::Scenario::incomingCall:
        case ToastTemplate::Scenario::reminder:
            return 4; // IMPORTANCE_HIGH

        case ToastTemplate::Scenario::default_:
            break;
    }

    return 3; // IMPORTANCE_DEFAULT
}

LocalRef<jstring> getChannelId()
{
    return javaString ("yup_notifications");
}

void ensureNotificationChannel (JNIEnv* env, jobject notificationManager, const ToastNotificationSettings& settings, const ToastTemplate& toast)
{
    if (getAndroidSDKVersion() < 26)
        return;

    const LocalRef<jstring> channelId (getChannelId());

    LocalRef<jobject> existingChannel (env->CallObjectMethod (notificationManager, AndroidNotificationManager.getNotificationChannel, channelId.get()));

    if (existingChannel)
        return;

    const LocalRef<jstring> channelName (javaString (settings.appName.isEmpty() ? String ("Notifications") : settings.appName));

    LocalRef<jobject> channel (env->NewObject (AndroidNotificationChannel, AndroidNotificationChannel.constructor, channelId.get(), channelName.get(), notificationChannelImportance (toast)));

    if (channel)
        env->CallVoidMethod (notificationManager, AndroidNotificationManager.createNotificationChannel, channel.get());
}

LocalRef<jobject> createBitmapFromFile (JNIEnv* env, const File& file)
{
    FileInputStream stream (file);
    if (! stream.openedOk())
        return {};

    const auto size = static_cast<size_t> (stream.getTotalLength());
    if (size == 0)
        return {};

    HeapBlock<char> data (size);
    if (stream.read (data, size) != static_cast<ssize_t> (size))
        return {};

    LocalRef<jbyteArray> byteArray (env->NewByteArray (static_cast<jsize> (size)));
    env->SetByteArrayRegion (byteArray.get(), 0, static_cast<jsize> (size), reinterpret_cast<const jbyte*> (data.get()));

    return LocalRef<jobject> (env->CallStaticObjectMethod (AndroidBitmapFactory, AndroidBitmapFactory.decodeByteArray, byteArray.get(), 0, static_cast<jsize> (size)));
}

LocalRef<jobject> createLaunchIntent (JNIEnv* env)
{
    LocalRef<jobject> intent (env->NewObject (AndroidIntent, AndroidIntent.constructor));
    if (! intent)
        return {};

    const LocalRef<jstring> packageName (static_cast<jstring> (env->CallObjectMethod (getAppContext().get(), AndroidContext.getPackageName)));

    env->CallObjectMethod (intent.get(), AndroidIntent.setAction, javaString ("android.intent.action.MAIN").get());
    env->CallObjectMethod (intent.get(), AndroidIntent.addCategory, javaString ("android.intent.category.LAUNCHER").get());
    env->CallObjectMethod (intent.get(), AndroidIntent.setPackage, packageName.get());

    return intent;
}

LocalRef<jobject> createPendingIntent (JNIEnv* env, jobject intent, int requestCode)
{
    const jint flags = getAndroidSDKVersion() >= 23 ? 0x04000000 | 0x08000000 // FLAG_IMMUTABLE | FLAG_UPDATE_CURRENT
                                                    : 0x08000000;             // FLAG_UPDATE_CURRENT

    return LocalRef<jobject> (env->CallStaticObjectMethod (AndroidPendingIntent, AndroidPendingIntent.getActivity, getAppContext().get(), requestCode, intent, flags));
}

} // namespace

//==============================================================================
Result toastNotificationInitialize (const ToastNotificationSettings&)
{
    auto& state = getToastState();

    if (state.initialized)
        return Result::ok();

    // On Android 13+ the app needs the POST_NOTIFICATIONS runtime permission.
    if (getAndroidSDKVersion() >= 33 && ! RuntimePermissions::isGranted (RuntimePermissions::postNotifications))
    {
        RuntimePermissions::request (RuntimePermissions::postNotifications, [] (bool) {});
    }

    state.initialized = true;
    return Result::ok();
}

//==============================================================================
//==============================================================================
ResultValue<int64> notifyNowImpl (const ToastTemplate& toast, const ToastNotificationSettings& settings, int64 id)
{
    auto* env = getEnv();

    const LocalRef<jobject> appContext (getAppContext());

    LocalRef<jobject> notificationManager (env->CallObjectMethod (appContext.get(), AndroidContext.getSystemService, javaString ("notification").get()));

    if (! notificationManager)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::systemNotSupported));

    ensureNotificationChannel (env, notificationManager.get(), settings, toast);

    // Use the application icon as the notification's small icon.
    const LocalRef<jobject> applicationInfo (env->CallObjectMethod (appContext.get(), AndroidContext.getApplicationInfo));
    const int smallIcon = applicationInfo ? env->GetIntField (applicationInfo.get(), AndroidApplicationInfo.icon) : 0;

    const String title = toast.getTextField (ToastTemplate::TextField::firstLine);
    const String body = toast.getTextField (ToastTemplate::TextField::secondLine);

    const LocalRef<jobject> builder (env->NewObject (AndroidNotificationBuilder, AndroidNotificationBuilder.constructor, appContext.get()));
    if (! builder)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::invalidParameters));

    env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.setSmallIcon, smallIcon);
    env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.setContentTitle, javaString (title).get());
    env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.setContentText, javaString (body.isEmpty() ? toast.getAttributionText() : body).get());
    env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.setAutoCancel, JNI_TRUE);

    if (getAndroidSDKVersion() >= 26)
    {
        const LocalRef<jstring> channelId (getChannelId());
        env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.setChannelId, channelId.get());

        if (toast.getExpiration() > 0)
            env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.setTimeoutAfter, static_cast<jlong> (toast.getExpiration()));
    }

    if (toast.hasImage() && toast.getImagePath().existsAsFile())
    {
        const LocalRef<jobject> bitmap (createBitmapFromFile (env, toast.getImagePath()));

        if (bitmap)
        {
            const LocalRef<jobject> style (env->NewObject (AndroidBigPictureStyle, AndroidBigPictureStyle.constructor));
            env->CallObjectMethod (style.get(), AndroidBigPictureStyle.bigPicture, bitmap.get());
            env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.setStyle, style.get());
        }
    }
    else if (! body.isEmpty())
    {
        const LocalRef<jobject> style (env->NewObject (AndroidBigTextStyle, AndroidBigTextStyle.constructor));
        env->CallObjectMethod (style.get(), AndroidBigTextStyle.bigText, javaString (body).get());
        env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.setStyle, style.get());
    }

    const auto notificationId = static_cast<int32> (id);

    // Tapping the notification opens the app's launcher activity.
    if (const LocalRef<jobject> launchIntent { createLaunchIntent (env) })
    {
        const LocalRef<jobject> contentIntent (createPendingIntent (env, launchIntent.get(), notificationId));
        if (contentIntent)
            env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.setContentIntent, contentIntent.get());
    }

    // Action buttons fire broadcasts with "yup_action_<index>" actions. The app
    // can receive them with a BroadcastReceiver; activation callbacks are not
    // delivered back to native code.
    for (size_t i = 0; i < toast.getActionsCount(); ++i)
    {
        const LocalRef<jobject> actionIntent (env->NewObject (AndroidIntent, AndroidIntent.constructor));
        env->CallObjectMethod (actionIntent.get(), AndroidIntent.setAction, javaString ("yup_action_" + String (static_cast<int> (i))).get());

        if (const LocalRef<jobject> actionPendingIntent { createPendingIntent (env, actionIntent.get(), 100 + static_cast<int> (i)) })
            env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.addAction, 0, javaString (toast.getActionLabel (i)).get(), actionPendingIntent.get());
    }

    const LocalRef<jobject> notification (env->CallObjectMethod (builder.get(), AndroidNotificationBuilder.build));
    if (! notification)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notDisplayed));

    env->CallVoidMethod (notificationManager.get(), AndroidNotificationManager.notify, notificationId, notification.get());

    return makeResultValueOk (id);
}

//==============================================================================
ResultValue<int64> notifyNow (const ToastTemplate& toast, const ToastNotificationSettings& settings, int64 id, std::function<void (const ResultValue<int64>&)> completion)
{
    const auto result = notifyNowImpl (toast, settings, id);

    if (completion)
        completion (result);

    return result;
}

//==============================================================================
ResultValue<int64> toastNotificationShow (const ToastTemplate& toast, const ToastNotificationSettings& settings, std::function<void (const ResultValue<int64>&)> completion)
{
    auto& state = getToastState();

    if (! state.initialized)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notInitialized));

    // Reserve the notification id up front so the synchronous return is always
    // valid for hideToast(); the authoritative outcome arrives via completion.
    const int64 id = (state.nextId += 1) - 1;

    if (getAndroidSDKVersion() < 33 || RuntimePermissions::isGranted (RuntimePermissions::postNotifications))
        return notifyNow (toast, settings, id, std::move (completion));

    // The permission hasn't been granted yet: request it, then show (or fail).
    RuntimePermissions::request (RuntimePermissions::postNotifications, [toast, settings, id, completion = std::move (completion)] (bool granted) mutable
    {
        if (granted)
        {
            notifyNow (toast, settings, id, std::move (completion));
        }
        else if (completion)
        {
            completion (makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::permissionDenied)));
        }
    });

    return makeResultValueOk (id);
}

//==============================================================================
bool toastNotificationHide (int64 id)
{
    auto* env = getEnv();

    const LocalRef<jobject> appContext (getAppContext());

    LocalRef<jobject> notificationManager (env->CallObjectMethod (appContext.get(), AndroidContext.getSystemService, javaString ("notification").get()));

    if (! notificationManager)
        return false;

    env->CallVoidMethod (notificationManager.get(), AndroidNotificationManager.cancel, static_cast<int> (id));
    return true;
}

//==============================================================================
void toastNotificationClear()
{
    auto* env = getEnv();

    const LocalRef<jobject> appContext (getAppContext());

    LocalRef<jobject> notificationManager (env->CallObjectMethod (appContext.get(), AndroidContext.getSystemService, javaString ("notification").get()));

    if (! notificationManager)
        return;

    env->CallVoidMethod (notificationManager.get(), AndroidNotificationManager.cancelAll);
}

//==============================================================================
void toastNotificationGetPermissionState (std::function<void (ToastNotification::PermissionState)> callback)
{
    if (! callback)
        return;

    if (getAndroidSDKVersion() < 33)
    {
        callback (ToastNotification::PermissionState::granted);
        return;
    }

    // Android can't distinguish "never asked" from "denied", so a non-granted
    // state is reported as notDetermined (the request can be re-issued).
    callback (RuntimePermissions::isGranted (RuntimePermissions::postNotifications)
                  ? ToastNotification::PermissionState::granted
                  : ToastNotification::PermissionState::notDetermined);
}

//==============================================================================
void toastNotificationRequestPermission (std::function<void (ToastNotification::PermissionState)> callback)
{
    if (getAndroidSDKVersion() < 33)
    {
        if (callback)
            callback (ToastNotification::PermissionState::granted);

        return;
    }

    RuntimePermissions::request (RuntimePermissions::postNotifications, [callback = std::move (callback)] (bool granted) mutable
    {
        if (callback)
        {
            callback (granted ? ToastNotification::PermissionState::granted
                              : ToastNotification::PermissionState::denied);
        }
    });
}

//==============================================================================
void toastNotificationSetPermissionStateChangedCallback (std::function<void (ToastNotification::PermissionState)>)
{
    // Android delivers no permission-change events; use getPermissionState().
}

} // namespace detail
} // namespace yup

#endif // YUP_ANDROID
