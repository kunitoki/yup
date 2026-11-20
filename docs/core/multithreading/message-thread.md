# The message thread

Every YUP application has one **message thread**: the thread that runs the event
loop, delivers mouse and keyboard events, paints components and fires timers.
Most of YUP's UI classes may only be touched from it. The classes on this page
live in `yup_events` and are how other threads hand work back to it.

## Where the message loop runs

In a GUI application, `START_YUP_APPLICATION` creates the application object and
runs the message loop on the main thread; `initialise()` and `shutdown()` are
called on it:

```cpp
class MyApp : public yup::YUPApplication
{
public:
    yup::String getApplicationName() override    { return "MyApp"; }
    yup::String getApplicationVersion() override { return "1.0"; }

    void initialise (const yup::String& commandLine) override { /* create windows */ }
    void shutdown() override                                  { /* release them */ }
};

START_YUP_APPLICATION (MyApp)
```

Console programs and tests that use `yup_events` without an application object
initialize it with a `ScopedYupInitialiser_GUI` at the top of `main()`, or with
`initialiseYup_GUI()` and `shutdownYup_GUI()` when embedding YUP into another
event loop. Create and destroy them on the thread that will act as the message
thread.

## Checking the current thread

```cpp
if (yup::MessageManager::existsAndIsCurrentThread())
    updateLabel();
```

Two assertions document and enforce the rule in debug builds:

```cpp
void MyComponent::refresh()
{
    YUP_ASSERT_MESSAGE_THREAD;              // must be called on the message thread
    // ...
}

void MyModel::apply()
{
    YUP_ASSERT_MESSAGE_MANAGER_IS_LOCKED;   // message thread, or a MessageManagerLock held
    // ...
}
```

## Posting work to the message thread

`MessageManager::callAsync` queues a function to run on the message thread and
returns at once. It can be called from any thread except the audio thread, and
returns `false` if the message could not be posted:

```cpp
void Downloader::run()
{
    auto data = fetch();

    yup::MessageManager::callAsync ([weakEditor = yup::WeakReference<yup::Component> (editor),
                                     data = std::move (data)]
    {
        if (auto* e = weakEditor.get())
            static_cast<Editor*> (e)->show (data);
    });
}
```

The function may run after the object that posted it is gone. Capture data by
value, and reach components through a `WeakReference<Component>`, which becomes
null when the component is deleted.

`callFunctionOnMessageThread` is the blocking form: it waits until the callback
has run on the message thread and returns its result. Called on the message
thread itself, it runs the callback immediately:

```cpp
static void* readTitle (void* userData)
{
    auto* window = static_cast<MainWindow*> (userData);
    return new yup::String (window->getTitle());
}

std::unique_ptr<yup::String> title (static_cast<yup::String*> (
    yup::MessageManager::getInstance()->callFunctionOnMessageThread (&readTitle, mainWindow)));
```

Blocking a worker on the message thread can deadlock if the message thread is
waiting for that worker; prefer `callAsync` whenever the caller does not need an
answer.

## Locking the message thread

`MessageManagerLock` suspends the message thread so a background thread can call
UI code directly. Pass the current `Thread` (or `ThreadPoolJob`) so the attempt
gives up when the thread is asked to stop, and always check the result:

```cpp
void Worker::run()
{
    while (! threadShouldExit())
    {
        auto progress = doSomeWork();

        const yup::MessageManagerLock mml (this);

        if (! mml.lockWasGained())
            return;                         // the thread is being stopped

        progressBar.setValue (progress);
    }
}
```

The lock is re-entrant and is a no-op on the message thread. It stalls the whole
UI while held, so keep the locked section tiny; `callAsync` is usually the
better choice.

## Timers

A `Timer` calls `timerCallback()` periodically on the message thread:

```cpp
class Clock : public yup::Component, private yup::Timer
{
public:
    Clock()  { startTimerHz (30); }
    ~Clock() override { stopTimer(); }

private:
    void timerCallback() override { repaint(); }
};
```

`startTimer (ms)` sets the interval in milliseconds and `startTimerHz (hz)` as a
rate; calling either again restarts the timer with the new period, and
`stopTimer()` stops it. `isTimerRunning()`, `getTimerInterval()` and
`getTimerFrequencyHz()` report its state. Timers are approximate: a slow
callback or a busy message thread delays them, and missed ticks are not queued
up. For precise timing off the message thread, see
[`HighResolutionTimer`](threads.md#precise-periodic-callbacks).

`MultiTimer` runs several independent timers in one object, each identified by
an integer passed to `timerCallback (int timerID)`:

```cpp
startTimer (blinkTimer, 500);
startTimer (autosaveTimer, 60 * 1000);
```

For a one-shot delay, `Timer::callAfterDelay` runs a function once on the
message thread:

```cpp
yup::Timer::callAfterDelay (2000, [weakThis = yup::WeakReference<yup::Component> (this)]
{
    if (weakThis != nullptr)
        weakThis->setVisible (false);
});
```

## Coalescing updates

When a background thread changes data many times per second, posting a message
for each change floods the queue. `AsyncUpdater` merges them: every
`triggerAsyncUpdate()` before the callback runs results in a single
`handleAsyncUpdate()` on the message thread.

```cpp
class LevelView : public yup::Component, private yup::AsyncUpdater
{
public:
    void setLevel (float newLevel)          // any thread
    {
        level = newLevel;
        triggerAsyncUpdate();
    }

private:
    void handleAsyncUpdate() override       // message thread
    {
        repaint();
    }

    std::atomic<float> level { 0.0f };
};
```

`cancelPendingUpdate()` drops a pending callback, `isUpdatePending()` reports
one, and `handleUpdateNowIfNeeded()` runs it synchronously when called on the
message thread. A callback still pending when the updater is destroyed is dropped.

`LockingAsyncUpdater` does the same with a function instead of a base class, and
its `cancelPendingUpdate()` also waits for a callback already running - use it
when the callback touches an object another thread is about to delete:

```cpp
yup::LockingAsyncUpdater updater { [this] { refreshList(); } };

updater.triggerAsyncUpdate();
```

## Broadcasting changes

`ChangeBroadcaster` tells any number of `ChangeListener`s that something
changed, without saying what. Like `AsyncUpdater`, repeated
`sendChangeMessage()` calls coalesce into one callback per listener on the
message thread:

```cpp
class Document : public yup::ChangeBroadcaster
{
public:
    void setText (const yup::String& t)
    {
        text = t;
        sendChangeMessage();
    }

    yup::String text;
};

class Toolbar : public yup::Component, private yup::ChangeListener
{
public:
    explicit Toolbar (Document& d) : doc (d) { doc.addChangeListener (this); }
    ~Toolbar() override { doc.removeChangeListener (this); }

private:
    void changeListenerCallback (yup::ChangeBroadcaster*) override { updateTitle (doc.text); }

    Document& doc;
};
```

`sendSynchronousChangeMessage()` calls the listeners immediately and must be
used on the message thread. `dispatchPendingMessages()` delivers a pending
asynchronous change right away.

`ActionBroadcaster` is the string-carrying variant: `sendActionMessage (text)`
delivers each message to every `ActionListener::actionListenerCallback`
asynchronously on the message thread, without coalescing.

Many YUP setters take a `NotificationType` to choose how their listeners are
told about a change: `dontSendNotification`, `sendNotification` (the default
for the class), `sendNotificationSync` or `sendNotificationAsync`.

## From the audio thread

None of these mechanisms are realtime-safe: `callAsync`, `triggerAsyncUpdate`
and `sendChangeMessage` may allocate or lock. From the audio thread, write to an
atomic or an [`AbstractFifo`](../containers.md) and let a message-thread
`Timer` poll it, or use the `ParameterChangeQueue` that
[audio processors](../../audio/processors.md#reactive-parameter-updates) already
provide for parameter changes. See
[the audio thread](index.md#the-audio-thread).

## Related pages

- [Threads](threads.md) - background threads and `HighResolutionTimer`.
- [Thread pools](thread-pool.md) - returning job results to the UI.
- [Synchronization](synchronization.md) - locks and events between threads.
- [System & Application](../system-and-app.md) - console applications.
