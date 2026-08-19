# Cancellation

`yup::CancelToken` and `yup::CancelTokenSource` (in `yup_core/threads`) provide
a thread-safe way to request and observe the cancellation of a long-running
operation.

## Requesting cancellation

`CancelTokenSource` is the only thing that can request cancellation, and it
does so either explicitly via `cancel()` or automatically when it is destroyed
(unless it has been moved-from). `CancelToken` is an observer-only handle that
shares the source's cancellation state; all copies of a token observe the same
cancellation:

```cpp
yup::CancelTokenSource source;
auto token = source.getToken(); // observer handle

// ... hand `token` to worker threads or sub-operations ...

source.cancel(); // visible via token.wasCancelled()
```

The source is move-only (like `std::jthread`): moving it transfers ownership,
and the moved-from source no longer cancels on destruction. When the source
goes out of scope, the token is cancelled automatically - no manual cleanup
needed:

```cpp
yup::CancelToken token;
{
    yup::CancelTokenSource source;
    token = source.getToken(); // start a long-running operation
} // source destroyed -> token cancelled
```

## Observing cancellation

The three complementary ways to observe cancellation can be mixed freely:

- **Poll** - `wasCancelled()` is a lock-free atomic read that never blocks and
  performs no allocation, so it is safe on any thread, including real-time
  threads.

- **Block** - `waitForCancellation (timeoutMs)` suspends the calling thread
  until the token is cancelled (or the timeout expires). Because the
  underlying event is manual-reset, any number of threads can wait
  concurrently and all of them wake up on cancellation:

  ```cpp
  if (token.waitForCancellation (5000))
      cleanup();
  ```

- **Callback** - `registerCallback (cb)` invokes `cb` exactly once, on the
  thread that cancels the source, in registration order. If the token is
  already cancelled, `cb` runs synchronously inside `registerCallback()`. The
  returned `Registration` handle keeps the callback registered; destroying it
  (or calling `unregister()`) removes the callback so a future cancellation
  will not invoke it:

  ```cpp
  auto registration = token.registerCallback ([] { cleanup(); });

  // later: registration.unregister();
  ```

## Non-cancellable tokens

Tokens created with the default constructor or with `CancelToken::none()` can
never be cancelled; all of them compare equal. `wasCancelled()` always returns
false for them, `waitForCancellation()` returns false immediately, and
registered callbacks are never invoked. A cancellable token can only be
obtained from a live `CancelTokenSource`.

## Thread-safety guarantees

- `CancelTokenSource::cancel()`, `CancelToken::wasCancelled()`,
  `waitForCancellation()`, `registerCallback()` and
  `Registration::unregister()` are safe to call concurrently from any number
  of threads.
- Each registered callback is invoked at most once.
- `wasCancelled()` never blocks and performs no allocation.
- Callbacks must not block and must not throw; a throwing callback is caught
  and asserted without preventing the remaining callbacks from running.
- `unregister()` guarantees the callback will not be invoked by a *future*
  cancellation. A callback already captured by an in-flight cancellation may
  still run once after `unregister()` returns.
- Cancellation triggered by a source's destructor runs on the destroying
  thread, so callbacks run in whatever context the source is destroyed in.
