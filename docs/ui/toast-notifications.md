# Toast Notifications

`ToastNotification` is a cross-platform utility for displaying toast-style
notifications (system popups) from a desktop or mobile app. It wraps each
platform's native notification API behind a single interface:

| Platform | Backend |
| -------- | ------- |
| Windows  | WinToast port (toast XML + AUMI/shortcut handling) |
| macOS / iOS | `UserNotifications` (`UNUserNotificationCenter`) |
| Android  | `NotificationManager` via JNI |
| Linux / BSD | `notify-send`, falling back to `zenity` |
| Emscripten | Browser Web Notifications API |

## Basic usage

`ToastNotification` is a singleton. Set the application name, initialize the
backend, and send a notification:

```cpp
auto& notifications = *ToastNotification::getInstance();

notifications.setAppName ("MyApp");
notifications.setAppUserModelId ("MyCompany.MyApp");

if (auto result = notifications.initialize(); result.failed())
    handleError (result.getErrorMessage());

notifications.sendNotification (
    "Hello!",
    "This is a toast notification.",
    [] (const Result& result)
    {
        if (result.failed())
            handleError (result.getErrorMessage());
    });
```

`sendNotification` is a convenience for the common title + message case. For
more control, build a `ToastTemplate` and call `showToast`:

```cpp
ToastTemplate toast;
toast.setFirstLine ("Backup complete");
toast.setSecondLine ("42 files were synced.");
toast.setAttributionText ("via MyApp");
toast.addAction ("View");
toast.setExpiration (5000); // hide after 5 seconds

toast.onActivated = [] { openViewer(); };

auto result = notifications.showToast (toast);

if (result.wasOk())
    notifications.hideToast (result.getValue()); // hide it on demand
```

## ToastTemplate

`ToastTemplate` describes the content and behavior of one notification:

- **Text fields** - `setFirstLine` / `setSecondLine` / `setThirdLine`
  (`setTextField`), limited by the template `TemplateType`
  (`text01`..`text04`, `imageAndText01`..`imageAndText04`).
- **Attribution** - `setAttributionText`, a small line below the content.
- **Images** - `setImagePath` (with a `CropHint`) and `setHeroImagePath`.
- **Actions** - `addAction` adds a button; activation is reported through
  `onActivatedWithAction (index)`.
- **Audio** - `setAudioPath (AudioSystemFile)` or a raw path/URI, plus an
  `AudioOption` (`default_`, `silent`, `loop`).
- **Behavior** - `setScenario` (`default_`, `alarm`, `incomingCall`,
  `reminder`), `setDuration` (`system`, `short_`, `long_`), and
  `setExpiration` (milliseconds from now).

Event callbacks (`onActivated`, `onActivatedWithAction`, `onDismissed`,
`onFailed`) are `std::function`s stored in the template; the backend keeps its
own copy until the notification is gone. They may be invoked on a
platform-dependent thread - marshal back to your UI thread if needed.

## Permissions

On platforms with a user-facing notification permission (Apple, Android 13+,
and the browser on Emscripten), permission is independent of `initialize()`:
query it with `getPermissionState()` and ask for it with `requestPermission()`
at any time:

```cpp
ToastNotification::getPermissionState ([] (ToastNotification::PermissionState state)
{
    // notDetermined | granted | denied
});

ToastNotification::requestPermission ([] (ToastNotification::PermissionState state)
{
    if (state == ToastNotification::PermissionState::granted)
        sendIt();
});
```

`showToast()` and `sendNotification()` re-check the permission before sending
and request it automatically when it hasn't been decided yet, so the simple
one-call flow keeps working. Because the user can deny or revoke permission at
any time (including from the system settings), the synchronous return value of
`showToast()` only reports that the request was accepted - the id is valid for
`hideToast()` - while the authoritative outcome arrives through the optional
completion callback:

```cpp
auto result = notifications.showToast (toast,
                                       [] (const ResultValue<int64>& outcome)
{
    if (outcome.failed())
        handleError (outcome.getErrorMessage()); // e.g. "permission denied"
});

if (result.failed())
    handleError (result.getErrorMessage()); // immediate failure (not initialized, ...)
```

`setPermissionStateChangedCallback()` can observe permission changes (delivered
on Apple macOS 12+ / iOS 15+ only; elsewhere treat `getPermissionState()` as
the source of truth).

## Platform notes

Not every field maps onto every backend:

- **Windows** - full parity, including the App User Model ID
  (`setAppUserModelId`, `configureAUMI`) and the Start-menu shortcut policy
  (`setShortcutPolicy`).
- **Apple** - everything except expiration, which has no native equivalent; the
  backend removes the delivered notification after the timeout instead. Images
  are copied to a temporary file before creating the attachment, because
  `UNUserNotificationCenter` deletes the file at the URL it is given - the
  original image is preserved.
- **Android** - action activation callbacks are not delivered (action buttons
  fire broadcasts); notifications need `POST_NOTIFICATIONS` on Android 13+
  (`RuntimePermissions::postNotifications`).
- **Linux / BSD** - `notify-send` must be installed (falls back to `zenity`);
  `hideToast` and `clear` are no-ops, and actions are not delivered.
- **Emscripten** - permission must be granted by the user (browsers require a
  user gesture); event callbacks are not delivered.

`ToastNotification::clear()` hides all displayed notifications and is also
called when the singleton is destroyed.
