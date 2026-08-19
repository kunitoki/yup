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
// The per-process state of the toast backend. The JS-side notification handles
// are kept in a Map keyed by the notification id, so they can be closed on
// demand.
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
    return EM_ASM_INT ({ return typeof Notification != = "undefined";
    }) != 0;
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

    // Permission must be requested from a user gesture on most browsers, so a
    // request issued here may be ignored. It is harmless to try.
    MAIN_THREAD_EM_ASM ({
        if (Notification.permission === "default")
            Notification.requestPermission();
    });

    state.initialized = true;
    return Result::ok();
}

//==============================================================================
ResultValue<int64> toastNotificationShow (const ToastTemplate& toast, const ToastNotificationSettings& settings)
{
    auto& state = getToastState();

    if (! state.initialized)
        return makeResultValueFail (ToastNotification::getErrorDescription (ToastNotification::Error::notInitialized));

    const int32 id = (state.nextId += 1) - 1;

    const String title = toast.getTextField (ToastTemplate::TextField::firstLine);
    const String body = toast.getTextField (ToastTemplate::TextField::secondLine);

    File icon = toast.getImagePath();
    if (! icon.existsAsFile() && settings.fallbackImage.has_value())
        icon = *settings.fallbackImage;

    const String iconPath = icon.existsAsFile() ? icon.getFullPathName() : String();
    const int64 expiration = toast.getExpiration();

    MAIN_THREAD_EM_ASM ({
        var id = $0;
        var title = UTF8ToString ($1);
        var body = UTF8ToString ($2);
        var icon = UTF8ToString ($3);
        var expirationMs = $4;

        if (typeof Notification === "undefined" || Notification.permission !== "granted")
            return;

        var options = { body: body, tag: "yup-" + id };
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
    }, id, title.toRawUTF8(), body.toRawUTF8(), iconPath.toRawUTF8(), static_cast<double> (expiration));

    return makeResultValueOk (static_cast<int64> (id));
}

//==============================================================================
bool toastNotificationHide (int64 id)
{
    MAIN_THREAD_EM_ASM ({
        var id = $0;

        if (typeof window.YupToastNotifications !== "undefined" && window.YupToastNotifications[id] !== undefined)
        {
            window.YupToastNotifications[id].close();
            delete window.YupToastNotifications[id];
        }
    }, id);

    return true;
}

//==============================================================================
void toastNotificationClear()
{
    MAIN_THREAD_EM_ASM ({
        if (typeof window.YupToastNotifications === "undefined")
            return;

        for (var key in window.YupToastNotifications)
        {
            window.YupToastNotifications[key].close();
            delete window.YupToastNotifications[key];
        }
    });
}

} // namespace detail
} // namespace yup

#endif // YUP_WASM
