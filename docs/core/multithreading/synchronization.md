# Synchronization

When two threads share data, every access needs a rule: a lock, an atomic, or a
hand-off through a queue. YUP's primitives live in `yup_core` and work the same
on every platform.

## Choosing a primitive

| You need to | Use |
| --- | --- |
| Protect shared state, held briefly, any thread but the audio thread | `CriticalSection` + `ScopedLock` |
| Protect a tiny critical section with almost no contention | `SpinLock` |
| Let many threads read while writes are rare | `ReadWriteLock` |
| Publish a single flag or counter | `std::atomic` or [`Atomic`](../memory.md) |
| Stream data from one thread to another without locking | [`AbstractFifo`](../containers.md) |
| Wake a waiting thread | `WaitableEvent`, or `Thread::wait` / `notify` |
| Wait until N tasks have finished | `CountDownLatch` |
| Stop work in progress | [`CancelToken`](cancellation.md) |
| Serialize access between processes | `InterProcessLock`, `InterProcessSemaphore` |

The audio thread is special: it must never wait for another thread, so none of
the locks or events above belong in an audio callback. See
[the audio thread](index.md#the-audio-thread).

## Critical sections

`CriticalSection` is a re-entrant mutex: the thread holding it can enter it
again without deadlocking. Always lock it through a scoped object, which
releases it on every exit path:

```cpp
class Playlist
{
public:
    void add (const yup::String& item)
    {
        const yup::ScopedLock sl (lock);
        items.add (item);
    }

    yup::StringArray getItems() const
    {
        const yup::ScopedLock sl (lock);
        return items;
    }

private:
    yup::CriticalSection lock;
    yup::StringArray items;
};
```

Copy data out while holding the lock and work on the copy afterwards, as
`getItems()` does: the shorter a lock is held, the less other threads wait.

Two more scoped helpers cover the other cases:

```cpp
{
    const yup::ScopedTryLock stl (lock);   // never blocks

    if (stl.isLocked())
        updateCache();
}

{
    const yup::ScopedLock sl (lock);
    prepare();

    {
        const yup::ScopedUnlock ul (lock); // release temporarily
        doSlowWorkWithoutTheLock();
    }

    commit();                              // locked again here
}
```

`DummyCriticalSection` has the same interface and does nothing, for classes
that take their lock type as a template parameter - such as `Array` and
`OwnedArray` - when they are only used from one thread.

## Spin locks

`SpinLock` busy-waits instead of sleeping. It is very small and very fast when
uncontended, and wasteful when it is not:

```cpp
yup::SpinLock lock;

{
    const yup::SpinLock::ScopedLockType sl (lock);
    latestValue = newValue;
}
```

Use it only for a few instructions of work. Unlike `CriticalSection`, a
`SpinLock` is not re-entrant - locking it twice from one thread deadlocks.
`RecursiveSpinLock` is the re-entrant variant. Both have `ScopedLockType`,
`ScopedUnlockType` and `ScopedTryLockType`, and `tryEnter()` to attempt the lock
without spinning.

## Read/write locks

`ReadWriteLock` lets any number of readers in at once, but a writer alone:

```cpp
yup::ReadWriteLock lock;
yup::HashMap<yup::String, Preset> presets;

Preset find (const yup::String& name)
{
    const yup::ScopedReadLock rl (lock);
    return presets[name];
}

void store (const yup::String& name, const Preset& preset)
{
    const yup::ScopedWriteLock wl (lock);
    presets.set (name, preset);
}
```

A waiting writer blocks new readers, so writers cannot be starved. The lock is
recursive, and a thread holding the write lock can also take the read lock.
`ScopedTryReadLock` and `ScopedTryWriteLock` attempt it without blocking.

It pays off only when reads are frequent and long enough to overlap; for short
accesses, a plain `CriticalSection` is cheaper.

## Waking another thread

`WaitableEvent` blocks one thread until another signals it:

```cpp
yup::WaitableEvent dataReady;

// consumer
if (dataReady.wait (1000))       // true when signalled, false after 1 s
    consume();

// producer
produce();
dataReady.signal();
```

By default the event resets automatically: a signal releases a single waiter
and is consumed. Construct it with `WaitableEvent (true)` for a manual-reset
event, which stays signalled - releasing every waiter - until `reset()` is
called. `wait()` with no argument, or a negative timeout, waits forever.

A [`Thread`](threads.md#sleeping-and-waking) has its own built-in event through
`wait()` and `notify()`, which also wakes on `stopThread()`; prefer that inside
a thread loop.

## Waiting for a batch of work

`CountDownLatch` blocks until a counter reaches zero:

```cpp
yup::CountDownLatch remaining (numTasks);

for (int i = 0; i < numTasks; ++i)
    pool.addJob ([&remaining, i] { runTask (i); remaining.countDown(); });

remaining.wait();                // returns once every task has counted down
```

`addCount (n)` raises the count when more work is discovered while the batch
runs, and `getCount()` reports how much is left. The count never drops below
zero.

## Precise waits

`WaitableTimer::waitUntil (ms)` blocks the calling thread until an absolute
time, measured in milliseconds on the
[`Time::getMillisecondCounterHiRes()`](../time.md) clock, with better precision
than `Thread::sleep` - useful for pacing a render loop or a clock thread:

```cpp
yup::WaitableTimer timer;
auto next = yup::Time::getMillisecondCounterHiRes();

while (! threadShouldExit())
{
    next += 10.0;
    timer.waitUntil (next);
    tick();
}
```

## Between processes

`InterProcessLock` is a named lock shared by every process that uses the same
name - for example to make sure only one instance writes a settings file:

```cpp
yup::InterProcessLock lock ("com.mycompany.myapp.settings");

{
    const yup::InterProcessLock::ScopedLockType sl (lock);

    if (sl.isLocked())
        saveSettings();
}
```

`enter (timeout)` and `exit()` are the manual form. Create and release the lock
on the same thread.

`InterProcessSemaphore` is a named counting semaphore, paired with
`SharedMemory` for exchanging data between processes in real time. One process
calls `create (name)`, the others `open (name)`, both returning a `Result`.
`signal()` never blocks or allocates and is safe on a realtime thread;
`wait (timeout)` blocks until the count is positive. It is desktop-only:
`create` and `open` fail on WebAssembly and Android.

## Related pages

- [Threads](threads.md) - `wait` and `notify` built into every `Thread`.
- [Containers](../containers.md) - `AbstractFifo` for lock-free hand-off.
- [Memory](../memory.md) - `Atomic`.
- [Cancellation](cancellation.md) - stopping work across threads.
