# XML

`yup_core` includes a lightweight, self-contained XML DOM. `XmlElement` is a
mutable node in the tree (a tag with attributes, child elements, and text), and
`XmlDocument` parses text or files into an element tree. It is well suited to
configuration files, document formats, and interop with XML-based tooling.

## Parsing

The quickest way to parse is the static `XmlDocument::parse`, which returns a
`std::unique_ptr<XmlElement>` for the document's root element (or `nullptr` on
failure):

```cpp
if (auto root = XmlDocument::parse (xmlText))          // from a String
{
    // ... use root ...
}

if (auto root = XmlDocument::parse (File ("config.xml")))  // from a File
{
    // ...
}
```

The free functions `parseXML (text)` / `parseXML (file)` are equivalent
shorthands.

### Parse errors

For diagnostics, construct an `XmlDocument` and inspect
`getLastParseError()` after requesting the root:

```cpp
XmlDocument doc (xmlText);
auto root = doc.getDocumentElement();

if (root == nullptr)
    DBG ("Parse error: " << doc.getLastParseError());
```

To parse only when the root tag matches, use
`getDocumentElementIfTagMatches ("expectedTag")` (or the
`parseXMLIfTagMatches` free function). To cheaply read only the outer element
and its attributes, pass `onlyReadOuterDocumentElement = true` to
`getDocumentElement()`.

## Reading elements

An `XmlElement` has a **tag name**, a set of **attributes**, and an ordered list
of **child elements** (some of which may be text nodes).

```cpp
String tag   = root->getTagName();
bool  isCfg  = root->hasTagName ("config");
```

### Attributes

Typed getters convert the stored string and fall back to a default when the
attribute is absent:

```cpp
String version = root->getStringAttribute ("version");                 // "" if absent
String theme   = root->getStringAttribute ("theme", "light");          // with default
int    width   = root->getIntAttribute ("width", 640);
double scale   = root->getDoubleAttribute ("scale", 1.0);
bool   enabled = root->getBoolAttribute ("enabled", false);

bool has = root->hasAttribute ("version");
int  n   = root->getNumAttributes();
String name  = root->getAttributeName (0);   // by index
String value = root->getAttributeValue (0);
```

### Child elements

```cpp
int count               = root->getNumChildElements();
XmlElement* first       = root->getFirstChildElement();
XmlElement* byIndex     = root->getChildElement (2);
XmlElement* byName      = root->getChildByName ("item");
XmlElement* byAttr      = root->getChildByAttribute ("id", "main");
```

Iterate children with the range-based `getChildIterator()` (skips nothing -
including text nodes), or walk the linked list manually with
`getNextElement()`:

```cpp
for (auto* child : root->getChildIterator())
{
    if (child->isTextElement())
        continue;

    DBG (child->getTagName());
}

// Only elements of a given tag:
for (auto* item = root->getChildByName ("item");
     item != nullptr;
     item = item->getNextElementWithTagName ("item"))
{
    DBG (item->getStringAttribute ("name"));
}
```

### Text content

Text is stored as special child nodes. Use `getAllSubText()` to concatenate all
descendant text, or `getChildElementAllSubText (tag, default)` to read the text
of a named child:

```cpp
String body  = element->getAllSubText();
String title = root->getChildElementAllSubText ("title", "");
bool   isText = node->isTextElement();
```

## Building & modifying

`XmlElement` is fully mutable. Set attributes, create children, and add text:

```cpp
XmlElement config ("config");
config.setAttribute ("version", 2);          // int, double, or String overloads
config.setAttribute ("theme", "dark");

auto* item = config.createNewChildElement ("item");   // created & appended
item->setAttribute ("id", "main");
item->addTextElement ("Hello world");

// Reparent an externally-owned element (takes ownership):
config.addChildElement (new XmlElement ("extra"));

config.removeAttribute ("theme");
config.deleteAllChildElementsWithTagName ("item");
```

```{note}
`createNewChildElement` allocates, appends, and returns the child (owned by the
parent). `addChildElement` / `insertChildElement` take ownership of an element
you allocated yourself. Children are deleted with their parent, so don't
`delete` them manually.
```

## Serializing

`toString()` renders the element (with an XML header by default); `writeTo()`
streams to an `OutputStream` or writes a `File`. A `TextFormat` controls the
output:

```cpp
String xml = config.toString();                        // pretty, with header
String oneLine = config.toString (XmlElement::TextFormat().singleLine());
String noHeader = config.toString (XmlElement::TextFormat().withoutHeader());

config.writeTo (File ("config.xml"));
```

`TextFormat` fields include `addDefaultHeader`, `lineWrapLength`, and
`newLineChars` (set to `nullptr` for a single line - what `singleLine()` does).

## See also

- [Data interchange](data-interchange.md) - `var`, JSON, and the data layer.
- [Files & streams](files-and-streams.md) - parsing from and writing to streams.
- [Serialization](serialisation.md) - describe and persist your own C++ types.
