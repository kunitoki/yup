# Serialization

YUP's serialization framework maps C++ types to and from archives with a single
description per type. Describe a type once and it can be written to a compact
**binary archive**, converted to/from a `var` (and therefore JSON), or embedded
inside a larger serializable object - no hand-written read/write code per field.

```{note}
The API uses the British spelling `Serialisation` (inherited from the JUCE
foundations); the surrounding prose uses American English. Types live in the
`yup_core` `serialisation/` group.
```

## Describing a type

A type becomes serializable when it declares two things:

1. a `static constexpr` **`marshallingVersion`** (an `int`, or `std::nullopt` to
   opt out of versioning), and
2. either a single **`serialise`** function, or a **`load`**/**`save`** pair for
   types that read and write differently.

The description can be **internal** (members of the type) or **external** (a
specialization of `SerialisationTraits<T>`, useful for third-party types you
cannot modify).

### Internal description

```cpp
struct Settings
{
    int    width  = 640;
    int    height = 480;
    String title  = "Untitled";

    static constexpr int marshallingVersion = 1;

    template <typename Archive>
    void serialise (Archive& archive)
    {
        archive (named ("width",  width),
                 named ("height", height),
                 named ("title",  title));
    }
};
```

```{important}
The archive's `operator()` is called with the members to serialize. Use
[`named(...)`](#named-values) so field names survive in named formats (JSON). The
**same** `serialise` function is used for both reading and writing - the archive
knows the direction.
```

### External description

When you cannot add members to a type, specialize `SerialisationTraits<T>`:

```cpp
template <>
struct yup::SerialisationTraits<ThirdPartyType>
{
    static constexpr int marshallingVersion = 0;

    template <typename Archive, typename T>
    static void serialise (Archive& archive, T& item)
    {
        archive (named ("x", item.x), named ("y", item.y));
    }
};
```

### Separate load/save

If loading and saving differ (e.g. computed fields, migration), provide a pair
instead of `serialise`:

```cpp
static constexpr int marshallingVersion = 2;

template <typename Archive>
void save (Archive& archive) const { archive (named ("value", value)); }

template <typename Archive>
void load (Archive& archive)
{
    archive (named ("value", value));

    if (archive.getVersion() < 2)   // migrate old data
        value = migrate (value);
}
```

`archive.getVersion()` returns an `std::optional<int>` - the detected version of
the object being read (`nullopt` when no version info is present).

## Helpers

### Named values

`named (name, value)` wraps a reference with a field name. In named formats
(JSON/`var`) the name becomes the object key; binary archives strip names. Use
named pairs consistently - mixing named and unnamed items in one archive is an
error.

```cpp
archive (named ("first", a), named ("second", b));
```

### Variable-size containers

If you write serialization for your own dynamically-sized type, archive a
`serialisationSize (n)` **before** the elements so the reader can resize the
container first:

```cpp
template <typename Archive, typename T>
static void load (Archive& archive, T& container)
{
    auto size = container.size();
    archive (serialisationSize (size));
    container.resize (size);

    for (auto& element : container)
        archive (element);
}
```

## Built-in support

Primitive types - arithmetic types, enums, `String`, and `var` - are serialized
directly. The framework also ships `SerialisationTraits` specializations for
common standard and YUP containers, so they work out of the box as members:

`std::vector`, `yup::Array`, `StringArray`, `std::pair`, `std::optional`,
`std::string`, `std::map`, `std::set`, C arrays (`T[N]`), and `std::array`.

## BinaryArchive

`BinaryOutputArchive` and `BinaryInputArchive` serialize to and from a binary
stream. Primitives are written in little-endian byte order; strings are written
as a length-prefixed UTF-8 blob; `named` names are omitted; `serialisationSize`
is written as an `int64`.

### Saving

```cpp
Settings settings;

MemoryOutputStream out;
BinaryOutputArchive archive (out);
archive (settings);              // recurses through serialise()

MemoryBlock bytes = out.getMemoryBlock();
```

### Loading

```cpp
MemoryInputStream in (bytes, /* keepCopy */ false);
BinaryInputArchive archive (in);

Settings settings;
archive (settings);              // fills settings from the stream
```

The input and output archives read/write in the same order and format, so a
single `serialise` function keeps them in sync automatically. Both archives'
`operator()` returns `bool` (all items handled) and accepts multiple arguments:

```cpp
archive (a, b, c);               // equivalent to three separate calls
```

```{note}
`BinaryArchive` versioning is handled at the file-format level, not per item -
`getVersion()` on the binary archives returns `nullopt`. When you need embedded
per-type versions, use the `var`/JSON path below, which records a `__version__`
property.
```

## var & JSON conversion

The same type description powers conversion to and from `var` (and therefore
[JSON](data-interchange.md#json)), via `ToVar` and `FromVar`.

```cpp
// Type -> var (-> JSON)
std::optional<var> v = ToVar::convert (settings);
if (v.has_value())
    String json = JSON::toString (*v);

// var (from JSON) -> type
var parsed = JSON::parse (jsonText);
std::optional<Settings> restored = FromVar::convert<Settings> (parsed);
```

`ToVar::convert` accepts a `ToVarOptions` to control versioning:

```cpp
auto v = ToVar::convert (settings,
                         ToVarOptions {}
                             .withVersionIncluded (false)      // omit __version__
                             .withExplicitVersion (0));         // serialize as v0
```

When a type has a non-null `marshallingVersion` and versioning is included, the
generated object carries a `__version__` property that `FromVar` reads back to
populate `archive.getVersion()`.

### VariantConverter

`VariantConverter<T>` is the customization point other YUP systems (e.g.
property/`ValueTree`-style storage) use to convert values to and from `var`. If
you have already described a type with `SerialisationTraits`, reuse it without
duplicating logic by deriving from `StrictVariantConverter<T>`:

```cpp
template <>
struct yup::VariantConverter<MyType> : yup::StrictVariantConverter<MyType> {};
```

## Choosing an archive

| Archive | Format | Versioning | Best for |
| ------- | ------ | ---------- | -------- |
| `BinaryOutputArchive` / `BinaryInputArchive` | Compact little-endian binary | File-level | State files, caches, fast/compact persistence. |
| `ToVar` / `FromVar` (+ JSON) | Human-readable `var`/JSON | Per-type `__version__` | Config, interchange, debugging, forward/backward compatibility. |

## See also

- [Data interchange](data-interchange.md) - `var`, `DynamicObject`, JSON, XML.
- [Files & streams](files-and-streams.md) - the streams archives read/write.
- [Data area](../data/index.md) - higher-level observable document models.
