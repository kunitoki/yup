# Data Interchange

`yup_core` provides a dynamically-typed value (`var`), object model
(`DynamicObject`), and a JSON parser/serializer - the tools for configuration,
document formats, and interop. XML has its own [dedicated page](xml.md).

## var

`var` is a dynamically-typed value that can hold void, bool, int, int64, double,
string, array, object (a `DynamicObject`), or a method. It underpins JSON, the
property system, and scripting.

```cpp
var i = 42;
var d = 3.14;
var s = "text";
var b = true;

bool isNum = d.isDouble();
int  n     = (int) i;
String str = s.toString();

// Array
var array;
array.append (1);
array.append ("two");
int size = array.size();

// Object
auto obj = new DynamicObject();
obj->setProperty ("name", "YUP");
obj->setProperty ("count", 3);
var object (obj);
var name = object["name"];        // "YUP"
```

Type predicates: `isVoid`, `isBool`, `isInt`, `isInt64`, `isDouble`, `isString`,
`isArray`, `isObject`, `isMethod`.

## DynamicObject

`DynamicObject` is a reference-counted object with named `var` properties and
optional callable methods. It is what a `var` holds when `isObject()` is true and
what JSON objects deserialize into.

```cpp
DynamicObject::Ptr obj = new DynamicObject();
obj->setProperty ("width", 640);
obj->setProperty ("height", 480);

if (obj->hasProperty ("width"))
    var w = obj->getProperty ("width");

const NamedValueSet& props = obj->getProperties();
```

## JSON

The `JSON` class converts between JSON text and `var`.

```cpp
// Parse (returns var(); use parse(text, result) for error detail)
var parsed = JSON::parse (jsonText);

// Parse with error reporting
var result;
Result r = JSON::parse (jsonText, result);
if (r.failed())
    DBG (r.getErrorMessage());

// Stringify
String text = JSON::toString (parsed, /* allOnOneLine */ false);
```

Related helpers:

- **`JSONUtils`** - convenience queries and manipulation over parsed `var`
  structures.
- **`JSONSerialisation`** - bridges JSON with the
  [serialization](#serialization) framework.

```{note}
`JSON::parse` accepts only a valid JSON object or array at the top level.
```

## XML

`yup_core` ships a small, self-contained XML DOM: `XmlElement` (a mutable node)
and `XmlDocument` (a parser for text and files).

```cpp
if (auto root = XmlDocument::parse (xmlText))       // std::unique_ptr<XmlElement>
{
    String version = root->getStringAttribute ("version");

    for (auto* child : root->getChildIterator())
        process (child->getTagName());
}
```

See the dedicated [XML](xml.md) page for parsing, attributes, child navigation,
text content, building, and serialization.

## Serialization

The `var`/JSON/XML types above are the *data* layer. To map your own C++ types
to and from archives (binary or `var`/JSON) with a single per-type description -
`SerialisationTraits`, `BinaryArchive`, and `ToVar`/`FromVar` - see the dedicated
[Serialization](serialisation.md) page.

```{seealso}
The [DataTree](../data/index.md) model in `yup_data_model` builds on
these primitives for observable, serializable document trees.
```

## See also

- [XML](xml.md) - the full `XmlElement` / `XmlDocument` reference.
- [Serialization](serialisation.md) - describe and persist your own types.
- [Containers](containers.md) - `var`, `NamedValueSet`, `DynamicObject`.
- [Files & streams](files-and-streams.md) - parse from and write to streams.
- [Data area](../data/index.md) - higher-level document/state models.
