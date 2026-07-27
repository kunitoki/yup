# DataTree

`DataTree` is a hierarchical data structure where each node has a **type**
(an [`Identifier`](../core/strings-and-text.md#identifier)) and holds both
**properties** (name → [`var`](../core/data-interchange.md#var) pairs) and
ordered **child nodes**. It is reference-counted and cheap to copy - copies share
the same underlying data, so passing a `DataTree` around gives every holder a
view of the same tree.

```{important}
`DataTree` is **read-only through its public accessors**. Every mutation goes
through a [transaction](transactions.md), which guarantees atomicity and fires
change notifications. There are no public setters on `DataTree` itself.
```

## Creating a tree

```cpp
#include <yup_data_model/yup_data_model.h>
using namespace yup;

DataTree settings ("AppSettings");   // node type = "AppSettings"

DataTree invalid;                     // default-constructed = invalid
```

A default-constructed `DataTree` is **invalid** - a safe placeholder returned by
lookups that find nothing. Always check `isValid()` (or the `explicit operator
bool`) before using a result:

```cpp
DataTree child = settings.getChildWithName ("ServerConfig");
if (child.isValid())
    use (child);
```

## Properties

Read properties with `getProperty` (with an optional default). The value type
follows the stored `var`:

```cpp
String version = settings.getProperty ("version", "unknown");
bool   debug   = settings.getProperty ("debug", false);
int    maxConn = settings.getProperty ("maxConnections", 50);

bool has  = settings.hasProperty ("version");
int  count = settings.getNumProperties();
Identifier name = settings.getPropertyName (0);   // by index
```

Writing properties requires a transaction - see [Transactions](transactions.md):

```cpp
{
    auto tx = settings.beginTransaction();
    tx.setProperty ("version", "1.2.3");
    tx.setProperty ("debug", true);
}
```

## Child nodes

Children are ordered and accessed by index or by type name:

```cpp
int n              = settings.getNumChildren();
DataTree first     = settings.getChild (0);
DataTree server    = settings.getChildWithName ("ServerConfig");
Identifier type    = server.getType();
```

Add, move, and remove children through a transaction:

```cpp
DataTree server ("ServerConfig");
DataTree ui ("UIConfig");

{
    auto tx = settings.beginTransaction();
    tx.addChild (server);       // append
    tx.addChild (ui, 0);        // insert at index 0
}
```

## Iterating

`DataTree` supports range-based iteration over its direct children:

```cpp
for (const auto& child : settings)
{
    DBG (child.getType().toString()
         << " (" << child.getNumProperties() << " properties)");
}
```

## Searching

Predicate helpers search children or the whole subtree without manual recursion:

```cpp
// Direct children matching a predicate
std::vector<DataTree> configs;
settings.findChildren (configs, [] (const DataTree& child)
{
    return child.getType().toString().endsWith ("Config");
});

// First descendant (any depth) matching a predicate
DataTree debugNode = settings.findDescendant ([] (const DataTree& node)
{
    return node.getProperty ("debug", false);
});

// All descendants matching a predicate
std::vector<DataTree> allSettings;
settings.findDescendants (allSettings, [] (const DataTree& node)
{
    return node.getType().toString().endsWith ("Settings");
});
```

For richer, composable queries - filtering by property, navigation axes,
ordering, extraction - use the [DataTreeQuery](query.md) engine.

## Change notifications

Register a `DataTree::Listener` to react to mutations. Callbacks fire **after** a
transaction commits:

```cpp
class MyListener : public DataTree::Listener
{
public:
    void propertyChanged (DataTree& tree, const Identifier& property) override {}
    void childAdded      (DataTree& parent, DataTree& child) override {}
    void childRemoved    (DataTree& parent, DataTree& child, int formerIndex) override {}
    void childMoved      (DataTree& parent, DataTree& child, int oldIndex, int newIndex) override {}
    void treeRedirected  (DataTree& tree) override {}
};

MyListener listener;
settings.addListener (&listener);
// ...
settings.removeListener (&listener);
```

| Callback | Fires when |
| -------- | ---------- |
| `propertyChanged` | A property is set, changed, or removed. |
| `childAdded` | A child is inserted. |
| `childRemoved` | A child is removed (with its former index). |
| `childMoved` | A child changes position. |
| `treeRedirected` | The node is repointed at different underlying data. |

```{tip}
For typed, cached, auto-refreshing access to a single property, prefer a
[`CachedValue`](cached-value.md) over manual listeners.
```

## See also

- [Transactions](transactions.md) - how to mutate a tree.
- [Querying](query.md) · [Schema & validation](schema.md)
- [Core: `var` & data interchange](../core/data-interchange.md)
