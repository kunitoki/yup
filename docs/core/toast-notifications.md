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

## Platform notes

Not every field maps onto every backend:

- **Windows** - full parity, including the App User Model ID
  (`setAppUserModelId`, `configureAUMI`) and the Start-menu shortcut policy
  (`setShortcutPolicy`).
- **Apple** - everything except expiration, which has no native equivalent; the
  backend removes the delivered notification after the timeout instead.
- **Android** - action activation callbacks are not delivered (action buttons
  fire broadcasts); notifications need `POST_NOTIFICATIONS` on Android 13+
  (`RuntimePermissions::postNotifications`).
- **Linux / BSD** - `notify-send` must be installed (falls back to `zenity`);
  `hideToast` and `clear` are no-ops, and actions are not delivered.
- **Emscripten** - permission must be granted by the user (browsers require a
  user gesture); event callbacks are not delivered.

`ToastNotification::clear()` hides all displayed notifications and is also
called when the singleton is destroyed.
