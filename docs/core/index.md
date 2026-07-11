# Core

`yup_core` is the foundation every other YUP module builds on. It provides
strings and text, containers, memory management, files and streams, math,
error handling, time, data interchange (`var`/JSON/XML), system information, and
networking - the portable C++ backbone of the framework.

**Modules covered:** `yup_core`, `yup_simd`.

```{note}
Threading lives in `yup_core` too (`Thread`, `CriticalSection`, `ThreadPool`,
lock-free FIFOs, …), but it is documented in its own
[Multithreading](../multithreading/index.md) area to keep the audio/UI thread
guidance together.
```

## In this area

- [Strings & text](strings-and-text.md) - `String`, `StringArray`,
  `StringPairArray`, `Identifier`, `StringRef`, character pointers, and Base64.
- [Containers](containers.md) - `Array`, `OwnedArray`, `ReferenceCountedArray`,
  `HashMap`, `SortedSet`, `Span`, `AbstractFifo`, and `ListenerList`.
- [Memory](memory.md) - `HeapBlock`, `MemoryBlock`, `ReferenceCountedObject`,
  `WeakReference`, smart pointers, `Singleton`, leak detection, and `Atomic`.
- [Files & streams](files-and-streams.md) - `File`, directory iteration,
  `InputStream`/`OutputStream`, memory streams, ZIP, and GZIP.
- [Results & error handling](results-and-errors.md) - `Result` and
  `ResultValue<T>` for fallible operations.
- [Maths](maths.md) - `Range`, `NormalisableRange`, `Random`, `BigInteger`,
  `Expression`, and statistics helpers.
- [Time](time.md) - `Time`, `RelativeTime`, and `PerformanceCounter`.
- [Data interchange](data-interchange.md) - `var`, `DynamicObject`, and JSON.
- [XML](xml.md) - the `XmlElement` / `XmlDocument` DOM and parser.
- [Serialization](serialisation.md) - the `SerialisationTraits` framework,
  `BinaryArchive`, and `var`/JSON conversion.
- [System & application](system-and-app.md) - `SystemStats`, `Uuid`,
  `ConsoleApplication`, runtime permissions, and platform macros.
- [Networking](networking.md) - `URL`, `WebInputStream`, `Socket`, `IPAddress`,
  and `NamedPipe`.

## Conventions

- **American English** throughout: `Color`, `center`, `initialize`.
- **Fluent, non-mutating variants**: many types offer a mutating method plus a
  `-ed`/`with*` counterpart that returns a new value (e.g.
  `String::trim()`/`trimmed`, `Range::withStart`).
- **`Result` / `ResultValue<T>`** for operations that can fail; assertions
  (`jassert`) for programming errors.
- **RAII everywhere**: prefer the smart pointers and scope guards in
  [Memory](memory.md) over manual `new`/`delete`.

```{toctree}
:hidden:
:maxdepth: 1

strings-and-text
containers
memory
files-and-streams
results-and-errors
maths
time
data-interchange
xml
serialisation
system-and-app
networking
```
