# Data

Structured, observable, transactional data models. The `yup_data_model` module
is built around **DataTree** - a hierarchical, schema-validatable, undoable data
structure ideal for application state, documents, and configuration.

**Module covered:** `yup_data_model`.

## In this area

- [DataTree](datatree.md) - the core hierarchical node type: properties, child
  nodes, navigation, iteration, and change listeners.
- [Transactions](transactions.md) - the atomic mutation model, undo/redo, and
  child management.
- [Querying (DataTreeQuery)](query.md) - a fluent + XPath-like query engine for
  finding, filtering, and extracting data from a tree.
- [Schema & validation](schema.md) - `DataTreeSchema` for JSON-Schema-based
  structure, defaults, and validated transactions.
- [CachedValue](cached-value.md) - fast, reactive typed access to properties
  (plus the thread-safe `AtomicCachedValue`).
- [Object lists](object-list.md) - `DataTreeObjectList` for keeping C++ objects
  in sync with tree children.

## At a glance

```cpp
#include <yup_data_model/yup_data_model.h>
using namespace yup;

DataTree settings ("AppSettings");

// All mutation happens inside a transaction.
{
    auto tx = settings.beginTransaction();
    tx.setProperty ("version", "1.2.3");
    tx.setProperty ("debug", true);
} // commits automatically on scope exit

String version = settings.getProperty ("version", "unknown");
bool   debug   = settings.getProperty ("debug", false);
```

```{note}
`DataTree` is the successor to the classic ValueTree pattern, adding
transactional mutation, schema validation, and a query engine. It builds on the
[`var`, JSON, and serialization](../core/data-interchange.md) primitives from
`yup_core`.
```

```{toctree}
:hidden:
:maxdepth: 1

datatree
transactions
query
schema
cached-value
object-list
```
