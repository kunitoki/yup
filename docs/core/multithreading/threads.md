# Threads

`yup::Thread` is a named OS thread with a cooperative stop flag and a built-in
wait/notify pair. Use it for long-lived background work that owns its own loop:
a file scanner, a network poller, a render or analysis thread. For many short
tasks, use a [thread pool](thread-pool.md) instead.

## Writing a thread

Subclass `Thread`, give it a name and implement `run()`. The thread runs until
`run()` returns, so a long-lived thread loops and checks `threadShouldExit()`:

```cpp
class FolderWatcher : public yup::Thread
{
public:
    FolderWatcher()
        : Thread ("FolderWatcher")
    {
    }

    ~FolderWatcher() override
    {
        stopThread (2000);
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            scanFolder();
            wait (500);          // sleep, but wake at once if stopThread() is called
        }
    }

private:
    void scanFolder();
};

FolderWatcher watcher;
watcher.startThread();
```

The name shows up in debuggers and profilers, so make it descriptive. An
optional second constructor argument sets the stack size in bytes; the default
`Thread::osDefaultStackSize` uses the platform's.

## Stopping a thread

Stopping is cooperative: you ask, and `run()` notices and returns.

```cpp
watcher.signalThreadShouldExit();      // raise the flag and return immediately
watcher.waitForThreadToExit (2000);    // wait up to 2 s for run() to return

watcher.stopThread (2000);             // both at once
```

`stopThread` raises the flag, wakes a thread blocked in `wait()`, and waits for
it to exit. If the thread does not finish within the timeout, it may be killed
forcibly on some platforms, which can leave locks held and memory leaked - so
treat the timeout as a safety net, and check `threadShouldExit()` often enough
that it is never reached.

Always stop a thread before the object is destroyed. Stopping it in the
subclass destructor, as above, is the safest place: by the time the base
`Thread` destructor runs, your members are already gone.

Code that does not have a pointer to its thread can still check the flag:

```cpp
if (yup::Thread::currentThreadShouldExit())
    return;
```

To react to the stop request from outside the thread - for example to close a
socket the thread is blocked on - add a `Thread::Listener`; its
`exitSignalSent()` is called from the thread that called
`signalThreadShouldExit()`.

For cancelling work that spans threads, pools and callbacks, prefer a
[`CancelToken`](cancellation.md): the thread flag only reaches code that has
access to the `Thread`.

## Sleeping and waking

| Call | Use it to |
| --- | --- |
| `wait (ms)` | Block this thread until `notify()` is called, `stopThread()` is called, or the timeout passes. A negative timeout waits forever. Returns true when woken. |
| `notify()` | Wake the thread from another thread, e.g. when new work has been queued. |
| `Thread::sleep (ms)` | Block unconditionally. Nothing can wake it early - prefer `wait()` in a thread loop. |
| `Thread::yield()` | Give up the rest of the time slice. |

A typical worker waits for work and is woken by the producer:

```cpp
void run() override
{
    while (! threadShouldExit())
    {
        if (auto job = queue.pop())
            process (*job);
        else
            wait (-1);
    }
}

// from any other thread
queue.push (job);
worker.notify();
```

## One-off background tasks

`Thread::launch` starts an anonymous thread that runs a function and deletes
itself when the function returns:

```cpp
yup::Thread::launch ([file]
{
    auto contents = file.loadFileAsString();
    // ...
});
```

It returns false if the thread could not be created. There is no handle to
stop or wait for it, so the function must be self-contained and safe to outlive
whatever launched it. Use it for occasional fire-and-forget work; for anything
frequent, a [thread pool](thread-pool.md) avoids the cost of creating a thread
each time.

## Priorities

`Thread::Priority` is a portable hint, from highest to lowest: `highest`,
`high`, `normal`, `low`, `background`.

```cpp
worker.startThread (yup::Thread::Priority::low);
worker.setPriority (yup::Thread::Priority::background);

yup::Thread::launch (yup::Thread::Priority::high, [] { /* ... */ });
```

`startThread (priority)` returns false and does not run the thread if it could
not be created with that priority (Android starts it anyway and upgrades it
afterwards). The priority is ignored on Linux.

## Realtime threads

A thread that must keep pace with the audio device - a secondary audio
renderer, a GPU audio dispatcher - can be started with realtime scheduling
where the platform offers it:

```cpp
auto options = yup::Thread::RealtimeOptions()
                   .withPriority (8)
                   .withApproximateAudioProcessingTime (512, 48000.0);

renderer.startRealtimeThread (options);
```

| Option | Meaning | Used by |
| --- | --- | --- |
| `withPriority (0 - 10)` | Realtime priority, 10 is highest. | POSIX platforms |
| `withProcessingTimeMs (ms)` | Typical CPU time needed per wake-up. | macOS, iOS |
| `withMaximumProcessingTimeMs (ms)` | Worst-case CPU time needed per wake-up. | macOS, iOS |
| `withApproximateAudioProcessingTime (samples, rate)` | The maximum processing time, derived from a block size and sample rate. | macOS, iOS |
| `withPeriodMs (ms)` / `withPeriodHz (hz)` | How often the thread wakes up. | macOS, iOS |

The options cannot be changed while the thread runs, and a normal thread cannot
be promoted to realtime: stop it and start it again. Code running on a realtime
thread follows the same rules as the [audio thread](index.md#the-audio-thread).

## CPU affinity

```cpp
worker.setAffinityMask (0b0011);                       // cores 0 and 1
yup::Thread::setCurrentThreadAffinityMask (0b0100);    // the calling thread, core 2
```

Affinity is rarely worth setting by hand - the scheduler usually does better -
but it can help pin a dedicated processing thread in a benchmark.

## Identifying threads

```cpp
const auto id = yup::Thread::getCurrentThreadId();   // a ThreadID, valid for any thread
auto* thread  = yup::Thread::getCurrentThread();     // nullptr unless it is a yup::Thread

if (thread != nullptr)
    DBG (thread->getThreadName());
```

`isThreadRunning()`, `getThreadId()` and `getPriority()` query a `Thread`
object from any thread.

## Per-thread values

`ThreadLocalValue<T>` gives each thread its own copy of a value, created on
first access from that thread:

```cpp
yup::ThreadLocalValue<int> recursionDepth;

void visit (Node& node)
{
    ++recursionDepth.get();
    // ...
    --recursionDepth.get();
}
```

`T` must be a primitive or a simple struct. Access it through `get()`, `*`,
`->` or by assigning to it. A thread that is done with its value, typically a
pool thread finishing a job, can free it with `releaseCurrentThreadStorage()`;
otherwise every value lives until the `ThreadLocalValue` is destroyed.

## Sharing one thread between many clients

`TimeSliceThread` runs many small, periodic tasks on a single thread - the
usual home for things like read-ahead buffering of several audio files:

```cpp
class DiskReader : public yup::TimeSliceClient
{
public:
    int useTimeSlice() override
    {
        const bool didWork = readNextChunk();
        return didWork ? 0 : 50;   // call again at once, or in about 50 ms
    }
};

yup::TimeSliceThread ioThread ("Disk IO");
DiskReader reader;

ioThread.addTimeSliceClient (&reader);
ioThread.startThread();
// ...
ioThread.removeTimeSliceClient (&reader);
ioThread.stopThread (1000);
```

The return value of `useTimeSlice()` schedules the next call:

| Return | Meaning |
| --- | --- |
| `0` | Call again as soon as possible. |
| positive | Call again after roughly this many milliseconds. |
| negative | Remove this client; it will not be called again. |

Clients share one thread, so each call must be short and must never block.
Remove a client before destroying it: `removeTimeSliceClient` waits for a call
in progress to finish.

## Precise periodic callbacks

`Timer` (see [the message thread](message-thread.md#timers)) fires on the
message thread and is only as precise as the event loop is idle.
`HighResolutionTimer` fires on its own thread at a steady interval:

```cpp
class ClockTick : public yup::HighResolutionTimer
{
    void hiResTimerCallback() override
    {
        // runs on the timer's thread, not the message thread
        tick.fetch_add (1);
    }

    std::atomic<int> tick { 0 };
};

ClockTick clock;
clock.startTimer (1);   // every millisecond
```

`stopTimer()`, `isTimerRunning()` and `getTimerInterval()` control it. The
callback must be short and thread-safe, and anything touching the UI has to be
[posted to the message thread](message-thread.md#posting-work-to-the-message-thread).

## Related pages

- [Thread pools](thread-pool.md) - running many short jobs on a fixed set of
  threads.
- [Synchronization](synchronization.md) - locks, events and latches for sharing
  data between threads.
- [The message thread](message-thread.md) - getting results back to the UI.
- [Cancellation](cancellation.md) - stopping work across threads with tokens.
