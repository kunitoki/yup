# Multithreading

Threading primitives and the cooperative message-thread model that keeps UI and
audio work correctly separated.

**Modules covered:** threading facilities in `yup_core`.

```{warning}
**Work in progress.** This area is still being written. Reference pages for
threads, the message-thread contract, synchronization primitives, and
audio-thread safety are still to come.
```

## Topics

- **Threads** - `Thread`, thread pools, and background work.
- **Message thread** - the `MessageManager`, `MessageManagerLock`, and
  posting work back to the UI thread.
- **Synchronization** - `CriticalSection`, atomics, and lock-free patterns for
  the audio thread.
- **Async** - timers, async updaters, and deferred callbacks.

## Render thread

Each `ComponentNative` window renders on its own dedicated `Thread`. Each
frame runs the `refreshDisplay` walk (component animation/logic) followed by the
repaint of any dirty regions, both under a `MessageManagerLock` so only that
window's tree suspends the message thread; GL command submission
(`GraphicsContext::end`) and the buffer swap happen after the lock is released.
The frame loop wakes on a repaint request or near the frame deadline, so
`refreshDisplay` keeps running at the desired frame rate even when nothing is
dirty. This keeps the message thread responsive with multiple windows open and
ensures vsync waits never serialize input and timer dispatch.
