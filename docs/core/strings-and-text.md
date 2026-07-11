# Strings & Text

YUP's text stack is built around `String`, a Unicode-aware, copy-on-write string
type, plus a family of supporting types for collections, identifiers, and
encodings. All text is stored as UTF-8 internally and converts on demand.

## String

`String` is the workhorse text type. It is reference-counted and cheap to copy,
holds Unicode text, and offers a large, fluent query/transform API.

```cpp
String s = "Hello, YUP";

bool empty   = s.isEmpty();
int  len     = s.length();
bool has     = s.contains ("YUP");
bool starts  = s.startsWith ("Hello");
bool ends    = s.endsWith ("YUP");
int  idx     = s.indexOf ("YUP");        // -1 if not found

String sub   = s.substring (7);           // "YUP"
String slice = s.substring (0, 5);        // "Hello"
String lower = s.toLowerCase();
String upper = s.toUpperCase();
String clean = s.trim();                  // remove leading/trailing whitespace
String rep   = s.replace ("YUP", "world");
```

### Numeric conversions

```cpp
int    i = String ("42").getIntValue();
double d = String ("3.14").getDoubleValue();
float  f = String ("2.5").getFloatValue();

String fromNum = String (42);             // "42"
String fixed   = String (3.14159, 2);     // "3.14" (2 decimal places)
```

### Building strings

`String` supports `operator<<` and `+` for concatenation:

```cpp
String out;
out << "count=" << 10 << ", ratio=" << 0.5;

String path = String ("dir/") + fileName;
```

### Encodings & interop

Convert to and from raw encodings via character pointers and helpers:

```cpp
String fromC = String::fromUTF8 (utf8Bytes, numBytes);
auto   utf8  = s.toRawUTF8();             // const char* (UTF-8)
std::string std = s.toStdString();
```

The `CharPointer_UTF8`, `CharPointer_UTF16`, and `CharPointer_UTF32` types give
you iterator-style access to the underlying code units when you need explicit
encoding control.

## StringRef

`StringRef` is a lightweight, non-owning reference to a string, used for function
parameters to avoid copies. Most APIs that only read a string accept `StringRef`,
so you can pass a `String`, a string literal, or a `const char*` transparently:

```cpp
void logMessage (StringRef text);   // accepts "literal", String, const char*
```

## StringArray

An ordered collection of strings with convenient join/split/search helpers.

```cpp
StringArray lines;
lines.add ("one");
lines.addArray ({ "two", "three" });

auto joined = lines.joinIntoString (", ");        // "one, two, three"
auto parts  = StringArray::fromTokens ("a,b,c", ",", "");
bool has    = lines.contains ("two");
lines.removeDuplicates (false);
lines.sort (false);
```

## StringPairArray

An ordered map of string keys to string values - handy for headers, metadata,
and simple property sets.

```cpp
StringPairArray headers;
headers.set ("Content-Type", "application/json");

String value = headers.getValue ("Content-Type", /* default */ "text/plain");
```

## Identifier

`Identifier` is an interned, immutable string optimized for fast comparison
(pointer equality). Use it for property names, node types, and other repeated
keys - for example in the [DataTree](../data/datatree.md) model.

```cpp
static const Identifier propertyName ("width");

if (someId == propertyName)   // O(1) comparison
    ...
```

## Base64 & text utilities

- **`Base64`** - encode/decode binary data to and from Base64 text.
- **`TextDiff`** - compute the difference between two strings.
- **`LocalisedStrings`** - translation/localization lookup.
- **`NewLine`** - the platform newline token for stream output.

```cpp
String encoded;
Base64::convertToBase64 (memoryOutputStream, data, numBytes);
```

## See also

- [Containers](containers.md) - `StringArray` builds on `Array`.
- [Data interchange](data-interchange.md) - `var` stringifies through `String`.
- [Files & streams](files-and-streams.md) - reading and writing text.
