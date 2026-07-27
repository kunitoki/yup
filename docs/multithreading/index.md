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
