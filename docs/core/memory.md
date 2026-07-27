# Memory

YUP favors explicit ownership and RAII over raw `new`/`delete`. `yup_core`
provides smart pointers, reference counting, raw memory buffers, leak detection,
and atomics.

## HeapBlock

`HeapBlock<T>` is a lightweight RAII wrapper around a raw heap allocation - a
resizable buffer of `T` with no per-element construction. Use it for POD buffers
where you want manual control without leaking.

```cpp
HeapBlock<float> buffer (numSamples);     // allocates numSamples floats
buffer[0] = 1.0f;
buffer.calloc (numSamples);               // reallocate, zero-initialised
// freed automatically
```

## MemoryBlock

`MemoryBlock` is a resizable block of raw bytes with convenient I/O and
conversion helpers. Use it for binary blobs, serialization buffers, and file
contents.

```cpp
MemoryBlock block (1024, /* initialiseToZero */ true);
block.append (data, numBytes);
block.setSize (2048, true);

void*  raw  = block.getData();
size_t size = block.getSize();

block.loadFromHexString ("deadbeef");
```

## Reference counting

### ReferenceCountedObject

Derive from `ReferenceCountedObject` and hold instances with
`ReferenceCountedObjectPtr<T>` (commonly aliased as `T::Ptr`). The object is
deleted when the last pointer goes away.

```cpp
class Resource : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<Resource>;
};

Resource::Ptr a = new Resource();         // ref count = 1
Resource::Ptr b = a;                      // ref count = 2
// deleted when both go out of scope
```

This is the ownership model used across YUP's GPU handles, images, and many
graph nodes.

### WeakReference

`WeakReference<T>` observes an object without keeping it alive, becoming null
when the target is destroyed. Add the `YUP_DECLARE_WEAK_REFERENCEABLE` macro to
the target class.

```cpp
WeakReference<MyObject> weak (myObject);

if (weak != nullptr)
    weak->doSomething();                  // safe: null after myObject dies
```

## Smart & scoped pointers

- **`std::unique_ptr` / `std::shared_ptr`** - prefer the standard smart pointers
  for exclusive/shared ownership.
- **`OptionalScopedPointer`** - a pointer that may or may not own its target.
- **`SharedResourcePointer<T>`** - a lazily-created singleton-style shared
  resource, reference-counted across all users.
- **`ContainerDeletePolicy`** - customization point for how containers delete
  owned objects.

## Singletons

The `YUP_DECLARE_SINGLETON` family of macros implements safe singletons with
controlled construction/destruction order.

```cpp
class Manager
{
public:
    YUP_DECLARE_SINGLETON (Manager, false)
};
```

## Leak detection

In debug builds, YUP can detect leaked and dangling objects. Add the macro to a
class and any leak at shutdown becomes an assertion pointing at the type.

```cpp
class MyType
{
    // ...
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyType)
};
```

`HeavyweightLeakedObjectDetector` offers per-instance backtraces for harder
cases.

## Atomics & byte order

- **`Atomic<T>`** - a thin wrapper over `std::atomic` with YUP-friendly helpers,
  used for lock-free flags and counters on the audio thread.
- **`ByteOrder`** - endian-swapping and little/big-endian read/write helpers for
  serialization.

```cpp
Atomic<bool> ready { false };
ready.set (true);
if (ready.get()) ...
```

```{note}
Locks (`CriticalSection`, `SpinLock`, `ReadWriteLock`, …) and threads live in the
[Multithreading](../multithreading/index.md) area. `Atomic` is documented here
because it is a memory primitive.
```

## Allocation hooks

`AllocationHooks` lets tests and tools observe or assert on heap allocations -
useful for verifying that audio-thread code paths do not allocate.

## See also

- [Containers](containers.md) - ownership models applied to collections.
- [Multithreading](../multithreading/index.md) - synchronization primitives.
- [Files & streams](files-and-streams.md) - `MemoryBlock` I/O.
