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

namespace yup
{

//==============================================================================
/**
    Describes the content and behavior of a toast notification.

    This class mirrors the concept of the toast templates exposed by the Windows
    toast notification system, but it is used as the cross-platform description
    of a notification: each platform backend maps the fields onto its native
    notification API (Windows toast XML, Apple UserNotifications, Android
    NotificationManager, Linux notify-send, or the browser Web Notifications API).

    A template is a value type: it can be copied freely and the platform backend
    will retain its own copy of the relevant fields (including the event
    callbacks) until the notification is gone.

    Not all fields are supported by every backend. As a general rule:
    - text fields, attribution text, image paths and expiration are supported
      everywhere except where the underlying API has no equivalent (for example,
      Apple's UNUserNotificationCenter has no expiration concept);
    - actions, audio options, scenarios and durations are best-effort and are
      mapped onto the closest native concept available, or ignored.

    @see ToastNotification

    @tags{Core}
*/
class YUP_API ToastTemplate
{
public:
    //==============================================================================
    /** The scenario the notification is displayed for. This can influence the
        priority and behavior of the notification on some platforms.
    */
    enum class Scenario
    {
        /** The default scenario: a normal, non-persistent notification. */
        default_,

        /** An alarm: persistent until dismissed, usually with a sound. */
        alarm,

        /** An incoming call: persistent, full screen on some platforms. */
        incomingCall,

        /** A reminder: persistent until dismissed, usually with a sound. */
        reminder
    };

    /** How long the notification should remain visible. */
    enum class Duration
    {
        /** Let the system decide. */
        system,

        /** A short display time. */
        short_,

        /** A long display time. */
        long_
    };

    /** Audio behavior of the notification. */
    enum class AudioOption
    {
        /** Use the default sound for the scenario. */
        default_,

        /** Play no sound at all. */
        silent,

        /** Loop the sound until the notification is dismissed. */
        loop
    };

    /** The layout of the notification. The numeric values match the Windows
        toast template types, so they can be forwarded to the native API.

        Note that the number of settable text fields follows the WinToastLib
        convention: text03 exposes two fields and text04 exposes three, while
        the underlying Windows templates contain one more empty text node each.
    */
    enum class TemplateType
    {
        /** A single image with one line of text. */
        imageAndText01 = 0,

        /** A single image with two lines of text. */
        imageAndText02,

        /** A single image with two lines of text. */
        imageAndText03,

        /** A single image with three lines of text. */
        imageAndText04,

        /** One line of text. */
        text01,

        /** Two lines of text. */
        text02,

        /** Two lines of text. */
        text03,

        /** Three lines of text. */
        text04
    };

    /** The system sound files that can be associated with a notification.
        This maps onto the Windows ms-winsoundevent: URIs; other backends map
        the entries onto the closest system sound they can find.
    */
    enum class AudioSystemFile
    {
        defaultSound,
        im,
        mail,
        reminder,
        sms,
        alarm,
        alarm2,
        alarm3,
        alarm4,
        alarm5,
        alarm6,
        alarm7,
        alarm8,
        alarm9,
        alarm10,
        call,
        call1,
        call2,
        call3,
        call4,
        call5,
        call6,
        call7,
        call8,
        call9,
        call10
    };

    /** How an image should be cropped when displayed. */
    enum class CropHint
    {
        /** Keep the image square. */
        square,

        /** Crop the image to a circle. */
        circle
    };

    /** The position of a text field inside the notification. */
    enum class TextField
    {
        firstLine = 0,
        secondLine,
        thirdLine
    };

    /** The reason a notification was dismissed. */
    enum class DismissalReason
    {
        /** The user explicitly dismissed the notification. */
        userCanceled = 0,

        /** The application hid the notification. */
        applicationHidden = 1,

        /** The notification timed out. */
        timedOut = 2
    };

    //==============================================================================
    /** Creates a template with the given type.

        @param type  The template type, which determines the number of available
                     text fields. The default is a two-line text template.
    */
    ToastTemplate (TemplateType type = TemplateType::text02);

    //==============================================================================
    /** Sets the text of one of the template's text fields.

        @param text      The text to display.
        @param position  The field to set. If the position is out of range for
                         the template type, the call is ignored.
    */
    void setTextField (const String& text, TextField position);

    /** Sets the first line of text. @see setTextField */
    void setFirstLine (const String& text) { setTextField (text, TextField::firstLine); }

    /** Sets the second line of text. @see setTextField */
    void setSecondLine (const String& text) { setTextField (text, TextField::secondLine); }

    /** Sets the third line of text. @see setTextField */
    void setThirdLine (const String& text) { setTextField (text, TextField::thirdLine); }

    /** Returns the text of one of the template's text fields.

        If the position is out of range for the template type, an empty string
        is returned.
    */
    String getTextField (TextField position) const;

    /** Returns the number of text fields available for the template type. */
    size_t getTextFieldsCount() const noexcept;

    //==============================================================================
    /** Sets the attribution text, a small line of text displayed below the main
        content. Not all backends support this; unsupported backends ignore it.
    */
    void setAttributionText (String newAttributionText);

    /** Returns the attribution text. */
    const String& getAttributionText() const noexcept { return attributionText; }

    //==============================================================================
    /** Sets the image to display in the notification, together with an optional
        crop hint. The file must exist and be readable by the platform backend.
    */
    void setImagePath (File newImagePath, CropHint newCropHint = CropHint::square);

    /** Sets a hero image, a large image displayed at the top of the
        notification. Only supported on backends that have a hero image concept
        (Windows and Apple); other backends ignore it.
    */
    void setHeroImagePath (File newHeroImagePath, bool inlineImage = false);

    /** Returns the image path, or an empty File if none was set. */
    File getImagePath() const noexcept { return imagePath; }

    /** Returns the hero image path, or an empty File if none was set. */
    File getHeroImagePath() const noexcept { return heroImagePath; }

    /** Returns the crop hint for the image. */
    CropHint getCropHint() const noexcept { return cropHint; }

    /** Returns true if the template type includes an image field. */
    bool hasImage() const noexcept { return type < TemplateType::text01; }

    /** Returns true if a hero image was set. */
    bool hasHeroImage() const noexcept { return hasImage() && ! heroImagePath.getFullPathName().isEmpty(); }

    /** Returns true if the hero image should be displayed inline. */
    bool isInlineHeroImage() const noexcept { return inlineHeroImage; }

    //==============================================================================
    /** Sets the sound to play using a system sound file. */
    void setAudioPath (AudioSystemFile file);

    /** Sets the sound to play using a raw audio path or URI.
        On Windows this accepts ms-winsoundevent: URIs; on other platforms the
        string is passed through to the backend, which may interpret it as a
        file path or ignore it.
    */
    void setAudioPath (String newAudioPath);

    /** Sets the audio option of the notification. */
    void setAudioOption (AudioOption newAudioOption) noexcept { audioOption = newAudioOption; }

    /** Returns the system sound file, if one was set via setAudioPath(). */
    std::optional<AudioSystemFile> getAudioSystemFile() const noexcept { return audioSystemFile; }

    /** Returns the raw audio path or URI, if any was set. */
    const String& getAudioPath() const noexcept { return audioPath; }

    /** Returns the audio option. */
    AudioOption getAudioOption() const noexcept { return audioOption; }

    //==============================================================================
    /** Sets how long the notification should be visible. Best-effort on most
        backends; ignored where the native API has no equivalent.
    */
    void setDuration (Duration newDuration) noexcept { duration = newDuration; }

    /** Returns the duration. */
    Duration getDuration() const noexcept { return duration; }

    /** Sets the expiration time, in milliseconds from now. Once expired, the
        notification is removed by the system. Not all backends support this;
        on Apple, the notification is removed by the backend after the timeout.
    */
    void setExpiration (int64 millisecondsFromNow) noexcept { expiration = millisecondsFromNow; }

    /** Returns the expiration time in milliseconds from now (0 = never). */
    int64 getExpiration() const noexcept { return expiration; }

    /** Sets the scenario of the notification. @see Scenario */
    void setScenario (Scenario newScenario) noexcept { scenario = newScenario; }

    /** Returns the scenario. */
    Scenario getScenario() const noexcept { return scenario; }

    /** Adds an action button to the notification. The button label is
        displayed by the backend; activation is reported through
        onActivatedWithAction, when the backend supports it.
    */
    void addAction (String label);

    /** Returns the number of action buttons. */
    size_t getActionsCount() const noexcept { return actions.size(); }

    /** Returns the label of the action at the given index. */
    const String& getActionLabel (size_t index) const;

    /** Returns the template type. */
    TemplateType getType() const noexcept { return type; }

    //==============================================================================
    /** Called when the user activates (clicks) the notification, without an
        action button. May be invoked on a platform-dependent thread.
    */
    std::function<void()> onActivated;

    /** Called when the user activates one of the notification's action buttons.
        The argument is the index of the action, as added with addAction().
    */
    std::function<void (int)> onActivatedWithAction;

    /** Called when the notification is dismissed, with the dismissal reason. */
    std::function<void (DismissalReason)> onDismissed;

    /** Called when the platform failed to display the notification. */
    std::function<void()> onFailed;

private:
    //==============================================================================
    std::vector<String> textFields;
    std::vector<String> actions;
    String attributionText;
    File imagePath;
    File heroImagePath;
    String audioPath;
    std::optional<AudioSystemFile> audioSystemFile;
    Scenario scenario { Scenario::default_ };
    Duration duration { Duration::system };
    AudioOption audioOption { AudioOption::default_ };
    TemplateType type { TemplateType::text02 };
    CropHint cropHint { CropHint::square };
    int64 expiration { 0 };
    bool inlineHeroImage { false };
};

//==============================================================================
/**
    A cross-platform utility to display toast notifications.

    This is a singleton that lazily initializes the platform-specific
    notification backend on first use and then lets the application display,
    hide and clear notifications. On platforms without a notification backend
    (or when the platform backend is unavailable), initialize() returns a failed
    Result and the show methods report a failure.

    Typical usage:

    @code
    auto& notifications = *ToastNotification::getInstance();

    notifications.setAppName ("MyApp");
    notifications.setAppUserModelId ("MyCompany.MyApp");

    if (auto result = notifications.initialize(); result.failed())
        return;

    ToastTemplate toast;
    toast.setFirstLine ("Hello!");
    toast.setSecondLine ("This is a toast notification.");

    auto result = notifications.showToast (toast);
    @endcode

    A convenience method is also provided for the common case of a simple
    title + message notification:

    @code
    ToastNotification::getInstance()->sendNotification (
        "Hello!",
        "This is a toast notification.",
        [] (const Result& result)
        {
            if (result.failed())
                handleError (result.getErrorMessage());
        });
    @endcode

    On platforms with a user-facing notification permission (Apple, Android 13+
    and the browser on Emscripten), the permission is independent of
    initialize(): use getPermissionState() to query it and requestPermission()
    to ask for it at any time. showToast() and sendNotification() re-check the
    permission before sending and request it automatically when it hasn't been
    decided yet, so the simple one-call flow keeps working - but because the
    user can deny or later revoke permission at any time, the authoritative
    outcome of a send is delivered asynchronously through the completion
    callback (see showToast()).

    All methods may be called from any thread. The configuration setters
    (setAppName, setAppUserModelId, setShortcutPolicy, setFallbackImage) are
    intended to be called before initialize() and are read without
    synchronization, so don't change them concurrently with showing
    notifications. The event callbacks (see ToastTemplate) and the result
    callbacks may be invoked on a platform-dependent thread, so applications
    that touch UI code inside them should marshal back to their UI thread
    first. On Emscripten the permission operations and showToast() marshal to
    the browser's main thread, so they may briefly block the caller in pthread
    builds.

    The singleton is deleted automatically when the application shuts down
    (see DeletedAtShutdown), so there is no need to call deleteInstance()
    explicitly.

    @tags{Events}
*/
class YUP_API ToastNotification : public DeletedAtShutdown
{
public:
    //==============================================================================
    /** The possible error codes reported by the backends. @see getErrorDescription */
    enum class Error
    {
        noError = 0,
        notInitialized,
        systemNotSupported,
        shellLinkNotCreated,
        invalidAppUserModelID,
        invalidParameters,
        invalidHandler,
        notDisplayed,

        /** The user denied or revoked notification permission. */
        permissionDenied,
        unknownError
    };

    /** The state of the platform's notification permission.

        Not every platform has a user-facing permission (Windows and Linux/BSD
        are always granted); on the others the user can deny the request or
        revoke permission later from the system settings, so the state can
        change at any time.
    */
    enum class PermissionState
    {
        /** The user hasn't decided yet (or the platform can't tell). */
        notDetermined = 0,

        /** Notifications are allowed. */
        granted = 1,

        /** The user denied or revoked notification permission. */
        denied = 2
    };

    /** Policy for creating or validating the Windows start-menu shortcut that
        carries the App User Model ID. Only meaningful on Windows; ignored
        elsewhere.
    */
    enum class ShortcutPolicy
    {
        /** Don't check, create, or modify a shortcut. */
        ignore = 0,

        /** Require a shortcut with a matching AUMI; don't create or modify one. */
        requireNoCreate = 1,

        /** Require a shortcut with a matching AUMI; create it if missing and
            update it if it doesn't match. This is the default.
        */
        requireCreate = 2
    };

    //==============================================================================
    /** Returns the singleton instance, creating it if necessary. */
    YUP_DECLARE_SINGLETON (ToastNotification, false)

    //==============================================================================
    /** Initializes the platform notification backend.

        This is safe to call multiple times and from multiple threads: only the
        first caller performs the platform initialization, the others block
        until it completes.

        @returns A successful Result if the backend is available and was
                 initialized, or a failed Result describing the problem
                 (e.g. the platform has no notification backend).
    */
    Result initialize();

    /** Returns true if the platform backend was initialized successfully. */
    bool isInitialized() const noexcept { return state.load (std::memory_order_relaxed) == State::initialized; }

    /** Hides all currently displayed notifications, and releases any resources
        held by the backend.
    */
    void clear();

    //==============================================================================
    /** Queries the current notification permission state, without showing any
        dialog.

        The callback is invoked (possibly asynchronously) with the current
        state. On platforms without a user-facing permission (Windows,
        Linux/BSD) it reports granted immediately.

        @see requestPermission, PermissionState
    */
    static void getPermissionState (std::function<void (PermissionState)> callback);

    /** Requests notification permission from the user, if needed.

        If the permission has already been decided, the callback fires
        immediately with the current state; otherwise the platform shows its
        permission prompt and the callback fires with the outcome.

        On Emscripten the browser requires this to be called from a user
        gesture (for example a button click) for the prompt to appear.

        @see getPermissionState, PermissionState
    */
    static void requestPermission (std::function<void (PermissionState)> callback);

    /** Registers a callback invoked whenever the notification permission state
        changes (for example when the user revokes it from the system settings).

        This is best-effort: only Apple (macOS 12+ / iOS 15+) delivers change
        events; other platforms never invoke it, so treat getPermissionState()
        as the source of truth.
    */
    void setPermissionStateChangedCallback (std::function<void (PermissionState)> callback);

    //==============================================================================
    /** Sets the application name. This is used by the platform backends to
        identify the application (e.g. the toast source on Windows or the
        notification category name on Linux). If not set, it defaults to the
        name of the current executable file.
    */
    void setAppName (String newAppName);

    /** Returns the application name. */
    const String& getAppName() const noexcept { return appName; }

    /** Sets the App User Model ID (AUMI) used on Windows to associate toasts
        with the application. If not set, the app name is used. Ignored on
        other platforms.
    */
    void setAppUserModelId (String newAppUserModelId);

    /** Returns the App User Model ID. */
    const String& getAppUserModelId() const noexcept { return appUserModelId; }

    /** Sets the shortcut policy used on Windows. @see ShortcutPolicy */
    void setShortcutPolicy (ShortcutPolicy newPolicy) noexcept { shortcutPolicy = newPolicy; }

    /** Sets a fallback image used by backends that display an icon alongside
        the notification when the template doesn't specify one (Linux only).
    */
    void setFallbackImage (std::optional<File> newFallbackImage);

    //==============================================================================
    /** Builds a Windows-style App User Model ID from its components.

        The result is "companyName.productName", plus ".subProduct" and
        ".versionInformation" when provided.

        @see setAppUserModelId
    */
    static String configureAUMI (StringRef companyName,
                                 StringRef productName,
                                 StringRef subProduct = {},
                                 StringRef versionInformation = {});

    /** Returns a human-readable description of the given error code. */
    static String getErrorDescription (Error error);

    //==============================================================================
    /** Displays a toast notification described by the given template.

        The backend keeps its own copy of the template (including the event
        callbacks) until the notification is dismissed, hidden or expired.

        The permission is re-checked before sending; if it hasn't been decided
        yet it is requested automatically, so a bare showToast() keeps working.
        Because the user can deny or revoke permission asynchronously, the
        synchronous return value only reports that the request was accepted
        (the id is valid for hideToast()); the authoritative outcome is
        delivered through completion.

        @param toast       The notification to display.
        @param completion  An optional callback invoked exactly once with the
                           final outcome of the send attempt: ok(id) once the
                           notification was accepted by the platform, or a
                           failure (for example "permission denied") if it
                           couldn't be shown. It may be invoked synchronously
                           or asynchronously, on a platform-dependent thread.
                           It is not invoked when the function returns a
                           failure before any send attempt was made (for
                           example when the backend isn't initialized).

        @returns A successful ResultValue containing the notification id, which
                 can be used with hideToast(), or a failed ResultValue
                 describing an immediate problem (backend not initialized,
                 invalid parameters, or synchronously-known permission
                 denial).
    */
    ResultValue<int64> showToast (const ToastTemplate& toast,
                                  std::function<void (const ResultValue<int64>&)> completion = {});

    /** Hides the notification with the given id, if it is still displayed.

        @param id  The notification id returned by showToast().
        @returns true if the notification was found and hidden.
    */
    bool hideToast (int64 id);

    //==============================================================================
    /** Convenience method to send a simple title + message notification.

        This initializes the backend if needed, displays a two-line text
        notification with the given title and message (the message is shown as
        the attribution text where supported), and invokes the callback with the
        outcome of the operation.

        @param title                   The notification title.
        @param message                 The notification message.
        @param resultCallback          An optional callback invoked with the
                                       outcome of the operation (including
                                       permission denial), possibly
                                       asynchronously.
        @param expirationMilliseconds  An optional expiration time in
                                       milliseconds from now.
    */
    void sendNotification (StringRef title,
                           StringRef message,
                           std::function<void (const Result&)> resultCallback = {},
                           std::optional<int> expirationMilliseconds = std::nullopt);

private:
    //==============================================================================
    ToastNotification();
    ~ToastNotification();

    //==============================================================================
    enum class State : uint32_t
    {
        created = 0,
        initializing = 1,
        initialized = 2,
        failed = 3
    };

    std::atomic<State> state { State::created };

    String appName;
    String appUserModelId;
    ShortcutPolicy shortcutPolicy { ShortcutPolicy::requireCreate };
    std::optional<File> fallbackImage;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToastNotification)
};

//==============================================================================
#ifndef DOXYGEN
namespace detail
{
/** The configuration shared between the platform backends. */
struct ToastNotificationSettings
{
    String appName;
    String appUserModelId;
    ToastNotification::ShortcutPolicy shortcutPolicy { ToastNotification::ShortcutPolicy::requireCreate };
    std::optional<File> fallbackImage;
};

/** Initializes the platform backend. Only one backend is compiled per platform. */
Result toastNotificationInitialize (const ToastNotificationSettings& settings);

/** Queries the current notification permission state. */
void toastNotificationGetPermissionState (std::function<void (ToastNotification::PermissionState)> callback);

/** Requests notification permission, invoking the callback with the outcome. */
void toastNotificationRequestPermission (std::function<void (ToastNotification::PermissionState)> callback);

/** Registers a callback invoked when the permission state changes (best-effort). */
void toastNotificationSetPermissionStateChangedCallback (std::function<void (ToastNotification::PermissionState)> callback);

/** Displays a notification described by the given template, reporting the
    final outcome through completion.
*/
ResultValue<int64> toastNotificationShow (const ToastTemplate& toast,
                                          const ToastNotificationSettings& settings,
                                          std::function<void (const ResultValue<int64>&)> completion);

/** Hides the notification with the given id, returning true if it was found. */
bool toastNotificationHide (int64 id);

/** Hides all displayed notifications and releases backend resources. */
void toastNotificationClear();
} // namespace detail
#endif

} // namespace yup
