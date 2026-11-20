# Multithreading

Threads, thread pools, locks and the message-thread model that keeps UI, audio
and background work apart.

**Modules covered:** `yup_core` (threads and synchronization) and `yup_events`
(message thread, timers and asynchronous callbacks).

## The threads of a YUP application

A typical YUP application runs on three kinds of thread:

- **The message thread** runs the event loop: input events, painting, timers
  and most UI code. Components may only be touched from it. See
  [the message thread](message-thread.md).
- **The audio thread** is started by the audio device and calls your audio
  callback at a fixed rate. It has hard deadlines, see
  [below](#the-audio-thread).
- **Background threads** - your own `Thread`s and `ThreadPool` workers - do
  slow work such as loading files, networking or analysis, and hand results back
  to the message thread.

## What to use

| You want to | Use |
| --- | --- |
| Run a long-lived loop in the background | [`Thread`](threads.md) |
| Fire and forget a single task | [`Thread::launch`](threads.md#one-off-background-tasks) |
| Run many short tasks in parallel | [`ThreadPool`](thread-pool.md) |
| Share one thread between many small periodic jobs | [`TimeSliceThread`](threads.md#sharing-one-thread-between-many-clients) |
| Get a precise periodic callback off the message thread | [`HighResolutionTimer`](threads.md#precise-periodic-callbacks) |
| Run code on the message thread from elsewhere | [`MessageManager::callAsync`](message-thread.md#posting-work-to-the-message-thread) |
| Get a periodic callback on the message thread | [`Timer`](message-thread.md#timers) |
| Merge many update requests into one callback | [`AsyncUpdater`](message-thread.md#coalescing-updates) |
| Protect shared data | [`CriticalSection`, `SpinLock`, `ReadWriteLock`](synchronization.md) |
| Wait for another thread | [`WaitableEvent`, `CountDownLatch`](synchronization.md#waking-another-thread) |
| Stop work in progress | [`CancelToken`](cancellation.md) |

## The audio thread

The audio callback must finish within its buffer period every time, or the
output glitches. Anything that can block for an unbounded time is off limits on
the audio thread:

- **No locks** that another thread may hold - no `CriticalSection`,
  `ReadWriteLock`, `MessageManagerLock` or `WaitableEvent::wait`.
- **No allocation or deallocation** - no `new`, growing an `Array` or `String`,
  or creating `std::function`s that allocate.
- **No messages** - `callAsync`, `triggerAsyncUpdate` and `sendChangeMessage`
  allocate and lock.
- **No file or network I/O**, logging, or waiting for other threads.

Exchange data with the audio thread through lock-free structures instead:

- `std::atomic` or [`Atomic`](../memory.md) for single values such as gains and
  flags.
- [`AbstractFifo`](../containers.md) for streams of samples, MIDI or commands,
  with one thread writing and the other reading.
- The `ParameterChangeQueue` of
  [audio processors](../../audio/processors.md#reactive-parameter-updates) for
  parameter changes that the UI should reflect.

On the other side, a message-thread [`Timer`](message-thread.md#timers) polls
those atomics and FIFOs to update the UI.

```cpp
class Meter : public yup::Component, private yup::Timer
{
public:
    Meter() { startTimerHz (30); }

    void pushLevel (float level) noexcept    // audio thread
    {
        peak.store (yup::jmax (level, peak.load()));
    }

private:
    void timerCallback() override            // message thread
    {
        displayed = peak.exchange (0.0f);
        repaint();
    }

    std::atomic<float> peak { 0.0f };
    float displayed = 0.0f;
};
```

## Pages

- [Threads](threads.md) - `Thread`, stopping and waking threads, priorities,
  realtime threads, `TimeSliceThread` and `HighResolutionTimer`.
- [Thread pools](thread-pool.md) - `ThreadPool` and `ThreadPoolJob`, and
  returning results to the UI.
- [Synchronization](synchronization.md) - locks, events, latches and
  inter-process primitives.
- [The message thread](message-thread.md) - `MessageManager`, timers,
  `AsyncUpdater` and change broadcasters.
- [Cancellation](cancellation.md) - `CancelToken` and `CancelTokenSource`:
  requesting cancellation, observing it, and waiting on it.

```{toctree}
:hidden:
:maxdepth: 1

threads
thread-pool
synchronization
message-thread
cancellation
```
