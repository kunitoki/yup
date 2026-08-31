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
    The Emscripten backend uses the browser's Web Notifications API. Browser
    restrictions apply: requesting permission requires a user gesture on most
    browsers, notifications are only shown while the page is visible unless a
    service worker is installed, and activation/action callbacks cannot be
    delivered back to native code (the JS API reports them as events that
    require a service worker), so onActivated / onActivatedWithAction /
    onDismissed / onFailed are not invoked on this platform.
*/

#if YUP_WASM

namespace yup
{
namespace detail
{

namespace
{

//==============================================================================
struct WasmToastState
{
    yup::Atomic<int32> nextId { 1 };
    bool initialized { false };
};

WasmToastState& getToastState()
{
    static WasmToastState state;
    return state;
}

bool isNotificationApiAvailable()
{
    // clang-format off
    return EM_ASM_INT ({ return typeof Notification !== "undefined"; }) != 0;
    // clang-format on
}

ToastNotification::PermissionState getPermissionStateSync()
{
    // clang-format off
    const auto permission = MAIN_THREAD_EM_ASM_INT (
    {
        if (typeof Notification === "undefined")
            return 0;
        if (Notification.permission === "granted")
            return 2;
        if (Notification.permission === "denied")
            return 1;
        return 0;
    });
    // clang-format on

    switch (permission)
    {
        case 1:
            return ToastNotification::PermissionState::denied;
        case 2:
            return ToastNotification::PermissionState::granted;
        default:
            return ToastNotification::PermissionState::notDetermined;
    }
}

//==============================================================================
struct PermissionPollState
{
    std::vector<std::function<void (ToastNotification::PermissionState)>> callbacks;
    int remainingTicks { 0 };
    bool polling { false };
};

PermissionPollState& getPermissionPollState()
{
    static PermissionPollState state;
    return state;
}

static void permissionPollTick (void*)
{
    auto& poll = getPermissionPollState();

    const auto permission = getPermissionStateSync();

    if (permission == ToastNotification::PermissionState::notDetermined && poll.remainingTicks-- > 0)
    {
        emscripten_async_call (&permissionPollTick, nullptr, 200);
        return;
    }

    poll.polling = false;

    auto callbacks = std::move (poll.callbacks);
    poll.callbacks.clear();

    for (auto& callback : callbacks)
    {
        if (callback)
            callback (permission);
    }
}

void requestPermissionInternal (std::function<void (ToastNotification::PermissionState)> callback)
{
    const auto permission = getPermissionStateSync();

    if (permission != ToastNotification::PermissionState::notDetermined)
    {
        if (callback)
            callback (permission);

        return;
    }

    auto& poll = getPermissionPollState();
    poll.callbacks.push_back (std::move (callback));

    if (poll.polling)
        return; // an ongoing poll will deliver to all queued callbacks

    poll.polling = true;
    poll.remainingTicks = 40; // ~8 seconds at 200 ms per tick

    // clang-format off
    MAIN_THREAD_EM_ASM (
    {
        if (typeof Notification !== "undefined" && Notification.permission === "default")
            Notification.requestPermission();
    });
    // clang-format on

    emscripten_async_call (&permissionPollTick, nullptr, 200);
}

} // namespace

//==============================================================================
Result toastNotificationInitialize (const ToastNotificationSettings&)
{
    auto& state = getToastState();

    if (state.initialized)
        return Result::ok();

    if (! isNotificationApiAvailable())
        return Result::fail (ToastNotification::getErrorDescription (ToastNotification::Error::systemNotSupported));

    // clang-format off
    MAIN_THREAD_EM_ASM (
    {
        if (Notification.permission === "default")
            Notification.requestPermission();
    });
    // clang-format on

    state.initialized = true;
    return Result::ok();
}

//==============================================================================
ResultValue<int64> toastNotificationShow (const ToastTemplate& toast, const ToastNotificationSettings& settings, std::function<void (const ResultValue<int64>&)> completion)
{
    auto& state = getToastState();

    if (! state.initialized)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notInitialized));

    const int64 id = (state.nextId += 1) - 1;

    const auto showNow = [toast, settings, id]() -> ResultValue<int64>
    {
        const String title = toast.getTextField (ToastTemplate::TextField::firstLine);
        const String body = toast.getTextField (ToastTemplate::TextField::secondLine);

        File icon = toast.getImagePath();
        if (! icon.existsAsFile() && settings.fallbackImage.has_value())
            icon = *settings.fallbackImage;

        const String iconPath = icon.existsAsFile() ? icon.getFullPathName() : String();
        const int64 expiration = toast.getExpiration();

        // clang-format off
        const auto created = MAIN_THREAD_EM_ASM_INT (
        {
            var id = $0;
            var title = UTF8ToString ($1);
            var body = UTF8ToString ($2);
            var icon = UTF8ToString ($3);
            var expirationMs = $4;

            if (typeof Notification === "undefined" || Notification.permission !== "granted")
                return 0;

            var options = ({ body: body, tag: "yup-" + id });
            if (icon.length > 0)
                options.icon = icon;

            var notification = new Notification (title, options);

            if (typeof window.YupToastNotifications === "undefined")
                window.YupToastNotifications = {};

            window.YupToastNotifications[id] = notification;

            notification.onclick = function ()
            {
                window.focus();
                notification.close();
                delete window.YupToastNotifications[id];
            };

            if (expirationMs > 0)
            {
                setTimeout (function ()
                {
                    notification.close();
                    delete window.YupToastNotifications[id];
                }, expirationMs);
            }

            return 1;
        }, id, title.toRawUTF8(), body.toRawUTF8(), iconPath.toRawUTF8(), static_cast<double> (expiration));
        // clang-format on

        if (created != 0)
            return makeResultValueOk (id);

        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notDisplayed));
    };

    const auto complete = [completion] (const ResultValue<int64>& result)
    {
        if (completion)
            completion (result);
    };

    switch (getPermissionStateSync())
    {
        case ToastNotification::PermissionState::granted:
        {
            const auto result = showNow();
            complete (result);
            return result;
        }

        case ToastNotification::PermissionState::denied:
        {
            const ResultValue<int64> result (makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::permissionDenied)));
            complete (result);
            return result;
        }

        case ToastNotification::PermissionState::notDetermined:
        {
            requestPermissionInternal ([showNow, complete] (ToastNotification::PermissionState permission)
            {
                if (permission == ToastNotification::PermissionState::granted)
                    complete (showNow());
                else
                    complete (makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::permissionDenied)));
            });

            return makeResultValueOk (id);
        }
    }

    return makeResultValueOk (id);
}

//==============================================================================
bool toastNotificationHide (int64 id)
{
    // clang-format off
    const auto found = MAIN_THREAD_EM_ASM_INT (
    {
        var id = $0;

        if (typeof window !== "undefined" && typeof window.YupToastNotifications !== "undefined" && window.YupToastNotifications[id] !== undefined)
        {
            window.YupToastNotifications[id].close();
            delete window.YupToastNotifications[id];
            return 1;
        }

        return 0;
    }, id);
    // clang-format on

    return found != 0;
}

//==============================================================================
void toastNotificationClear()
{
    // clang-format off
    MAIN_THREAD_EM_ASM (
    {
        if (typeof window === "undefined" || typeof window.YupToastNotifications === "undefined")
            return;

        for (var key in window.YupToastNotifications)
        {
            window.YupToastNotifications[key].close();
            delete window.YupToastNotifications[key];
        }
    });
    // clang-format on
}

//==============================================================================
void toastNotificationGetPermissionState (std::function<void (ToastNotification::PermissionState)> callback)
{
    if (callback)
        callback (getPermissionStateSync());
}

//==============================================================================
void toastNotificationRequestPermission (std::function<void (ToastNotification::PermissionState)> callback)
{
    requestPermissionInternal (std::move (callback));
}

//==============================================================================
void toastNotificationSetPermissionStateChangedCallback (std::function<void (ToastNotification::PermissionState)>)
{
    // The browser reports permission changes to JS only, and there is no
    // JS->C++ bridge in this module; use getPermissionState() instead.
}

} // namespace detail
} // namespace yup

#endif // YUP_WASM
