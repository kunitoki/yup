# Multithreading

Threading primitives and the cooperative message-thread model that keeps UI and
audio work correctly separated.

**Modules covered:** threading facilities in `yup_core`.

## Topics

- **Threads** - `Thread`, thread pools, and background work.
- **Message thread** - the `MessageManager`, `MessageManagerLock`, and
  posting work back to the UI thread.
- **Synchronization** - `CriticalSection`, atomics, and lock-free patterns for
  the audio thread.
- **Async** - timers, async updaters, and deferred callbacks.

```{note}
This area is being fleshed out. Guidance on audio-thread safety and the
message-thread contract will be added here.
```
