# Containers

`yup_core` provides a set of value-semantic and reference-semantic containers
tuned for the framework's needs - predictable allocation, thread-aware variants,
and lock-free structures for the audio thread.

## Array

`Array<T>` is a dynamic, contiguous, value-type array (like `std::vector` with a
YUP-idiomatic API). It copies its elements and is the default choice for POD and
small value types.

```cpp
Array<int> values;
values.add (1);
values.add (2);
values.addArray ({ 3, 4, 5 });

int  first = values.getFirst();
int  x     = values[2];               // bounds-safe; returns default if out of range
int  idx   = values.indexOf (4);
bool has   = values.contains (3);

values.remove (0);                    // remove by index
values.removeFirstMatchingValue (4);  // remove by value
values.sort();
```

## OwnedArray

`OwnedArray<T>` owns heap-allocated objects via pointers and deletes them on
destruction or removal. Use it for polymorphic objects and non-copyable types.

```cpp
OwnedArray<Component> children;
children.add (new Button());          // takes ownership
children.add (std::make_unique<Slider>().release());

Component* c = children[0];
children.remove (0);                  // deletes the object
```

## ReferenceCountedArray

`ReferenceCountedArray<T>` holds `ReferenceCountedObjectPtr<T>` elements, keeping
each object alive while referenced. See
[`ReferenceCountedObject`](memory.md#referencecountedobject).

```cpp
ReferenceCountedArray<MyResource> resources;
resources.add (existingPtr);          // retains
```

## HashMap

`HashMap<KeyType, ValueType>` is an associative hash container.

```cpp
HashMap<String, int> counts;
counts.set ("apples", 3);

if (counts.contains ("apples"))
    int n = counts["apples"];

for (HashMap<String, int>::Iterator it (counts); it.next();)
    DBG (it.getKey() << " -> " << it.getValue());
```

## Sets

- **`SortedSet<T>`** - a sorted, unique-element collection with fast lookup.
- **`SparseSet<T>`** - an efficient set of integer ranges (e.g. selected rows).

## Span

`Span<T>` is a non-owning view over contiguous elements - a pointer + length
pair (like `std::span`). Use it to pass array slices without copying or coupling
to a specific container.

```cpp
void process (Span<const float> samples);

Array<float> buffer;
process (buffer);                     // implicitly viewed as a Span
```

## Value containers

- **`var`** - a dynamically-typed value (string, number, bool, array, object).
  See [Data interchange](data-interchange.md).
- **`NamedValueSet`** - an ordered set of `Identifier` → `var` pairs, the backing
  store for object properties.
- **`PropertySet`** - a string-keyed property store with type conversions and an
  optional fallback set.
- **`DynamicObject`** - a `var`-based object with named properties and methods.

## Lock-free & real-time containers

For the audio thread, prefer these allocation-free, wait-free structures:

- **`AbstractFifo`** - a lock-free single-producer/single-consumer index manager
  for a ring buffer. You own the storage; `AbstractFifo` hands out safe read/write
  regions.
- **`SingleThreadedAbstractFifo`** - the same pattern without atomics, for
  single-thread use.
- **`FixedSizeFunction`** - a `std::function`-like callable with inline storage
  and **no heap allocation**, safe to assign and call on the audio thread.

```cpp
AbstractFifo fifo (bufferSize);

// Producer
int start1, size1, start2, size2;
fifo.prepareToWrite (numItems, start1, size1, start2, size2);
// ... write into [start1, size1) and [start2, size2) ...
fifo.finishedWrite (size1 + size2);
```

```{seealso}
See the [Multithreading](../multithreading/index.md) area for the threading and
synchronization types that pair with these containers.
```

## Utilities

- **`ListenerList<T>`** - a type-safe list of listeners with safe iteration
  during callbacks; the basis of YUP's observer patterns.
- **`Enumerate`** - range-based `for` with an index (like Python's `enumerate`).
- **`ScopedValueSetter`** - temporarily set a variable and restore it on scope
  exit.
- **`ElementComparator`** - custom comparison objects for sorting.

## Choosing a container

| Need | Use |
| ---- | --- |
| Value types, contiguous | `Array<T>` |
| Owned polymorphic objects | `OwnedArray<T>` |
| Shared, reference-counted objects | `ReferenceCountedArray<T>` |
| Key/value lookup | `HashMap<K, V>` |
| Unique sorted elements | `SortedSet<T>` |
| Non-owning slice | `Span<T>` |
| Audio-thread queue | `AbstractFifo` |
| Dynamic/typed value | `var` |

## See also

- [Memory](memory.md) - ownership models behind the containers.
- [Strings & text](strings-and-text.md) - `StringArray`, `StringPairArray`.
- [Multithreading](../multithreading/index.md) - lock-free usage patterns.
