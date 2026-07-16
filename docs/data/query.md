# Querying (DataTreeQuery)

`DataTreeQuery` is a query engine for [`DataTree`](datatree.md) hierarchies -
think of it as SQL/XPath for your tree. It offers two interchangeable styles:

- a **fluent API** (method chaining, full IDE autocompletion), and
- an **XPath-like string syntax** (concise, configuration-friendly).

Both are **lazy**: a query is a lightweight chain of operations that runs only
when you request results, enabling early termination and minimal work.

## A sample tree

The examples below assume this structure:

```cpp
DataTree appRoot ("Application");
{
    auto tx = appRoot.beginTransaction();

    DataTree window ("Window");
    {
        auto w = window.beginTransaction();
        w.setProperty ("title", "My Application");

        DataTree left ("Panel");
        {
            auto p = left.beginTransaction();
            p.setProperty ("name", "LeftPanel");
            p.setProperty ("width", 200);
            p.setProperty ("docked", true);

            DataTree save ("Button");
            { auto b = save.beginTransaction();
              b.setProperty ("text", "Save"); b.setProperty ("enabled", true); }

            DataTree load ("Button");
            { auto b = load.beginTransaction();
              b.setProperty ("text", "Load"); b.setProperty ("enabled", false); }

            p.addChild (save);
            p.addChild (load);
        }
        w.addChild (left);
    }
    tx.addChild (window);
}
```

## Fluent API

Start with `DataTreeQuery::from (root)`, chain selection/filtering/navigation
operations, and finish with a terminal that extracts results.

### Selection & terminals

```cpp
// All buttons anywhere in the tree
auto buttons = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .nodes();                       // std::vector<DataTree>

// The first enabled button
auto firstEnabled = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .where ([] (const DataTree& n) { return n.getProperty ("enabled", false); })
    .first()
    .node();                        // single DataTree (check isValid())

if (firstEnabled.isValid())
    DBG (firstEnabled.getProperty ("text").toString());
```

Common terminals: `nodes()`, `node()`, `strings()`, `properties()`, `count()`,
`any()`, `all()`, `execute()`.

```{note}
Node terminals return **invalid** `DataTree` values (not exceptions) when nothing
matches. Always check `isValid()` on a single `node()` result.
```

### Navigation

```cpp
.descendants()            // all descendants, any type
.descendants ("Panel")    // all descendants of a type
.children()               // immediate children
.children ("Button")      // immediate children of a type
.parent()                 // parents of the current selection
.siblings()               // nodes sharing the same parent
.ofType ("Button")        // filter current selection by type
.distinct()               // de-duplicate (useful after parent())
```

```cpp
// Parent windows of all buttons, de-duplicated
auto owners = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .parent()
    .distinct()
    .nodes();
```

### Property filtering

```cpp
.hasProperty ("name")                    // property exists
.propertyEquals ("name", "LeftPanel")    // equals a value
.propertyNotEquals ("docked", true)      // differs from a value
.propertyWhere<int> ("width", [] (int w) { return w > 180; })   // typed predicate
.where ([] (const DataTree& n) { ... })  // arbitrary predicate
```

`propertyWhere<T>()` gives type-safe access to a property value, converting from
`var` automatically.

### Extraction & transformation

```cpp
// Extract one property from many nodes
auto texts = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .property ("text")
    .strings();

// Extract several properties
auto windowProps = DataTreeQuery::from (appRoot)
    .descendants ("Window")
    .properties ({ "title", "width", "height" })
    .properties();               // std::vector<var>

// Compute derived values
auto info = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .select ([] (const DataTree& b)
    {
        return b.getProperty ("text").toString()
             + (b.getProperty ("enabled", false) ? " (on)" : " (off)");
    })
    .strings();
```

### Ordering & pagination

```cpp
.orderByProperty ("text")                       // sort by a property
.orderBy ([] (const DataTree& n) { return n.getProperty ("width", 0); })
.skip (2).take (3)                              // pagination
.at ({ 0, 2 })                                  // pick specific positions
.first()  .last()                               // ends
```

### Aggregation & conditionals

```cpp
// Group by a computed key -> map<var, std::vector<DataTree>>
auto byState = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .groupBy ([] (const DataTree& b)
    {
        return b.getProperty ("enabled", false) ? var ("on") : var ("off");
    });

// Existence / universal checks (short-circuit)
bool anyDisabled = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .any ([] (const DataTree& b) { return ! b.getProperty ("enabled", true); });

bool allDocked = DataTreeQuery::from (appRoot)
    .descendants ("Panel")
    .all ([] (const DataTree& p) { return p.getProperty ("docked", false); });

// First match, or count
auto saveBtn = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .firstWhere ([] (const DataTree& b) { return b.getProperty ("text").toString() == "Save"; });

int enabledCount = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .where ([] (const DataTree& b) { return b.getProperty ("enabled", false); })
    .count();
```

## XPath-like syntax

`DataTreeQuery::xpath (root, "...")` runs a string query and returns a
`QueryResult`:

```cpp
auto buttons     = DataTreeQuery::xpath (appRoot, "//Button").nodes();
auto enabled     = DataTreeQuery::xpath (appRoot, "//Button[@enabled='true']").nodes();
auto named       = DataTreeQuery::xpath (appRoot, "//Panel[@name]").nodes();
auto texts       = DataTreeQuery::xpath (appRoot, "//Button/@text").strings();
auto firstButton = DataTreeQuery::xpath (appRoot, "//Button[1]").node();   // 1-indexed
auto lastPanel   = DataTreeQuery::xpath (appRoot, "//Panel[last()]").node();
auto wide        = DataTreeQuery::xpath (appRoot, "//Panel[@width > 180]").nodes();
```

### Syntax reference

| Expression | Meaning |
| ---------- | ------- |
| `//Type` | All descendants of `Type` |
| `/Type` | Direct children of `Type` |
| `*` | Any node type |
| `.` / `..` | Current / parent node |
| `/following-sibling` · `/preceding-sibling` | Sibling axes |
| `[@prop]` | Has property |
| `[@prop='value']` · `[@prop!='value']` | Property equals / not-equals |
| `[@prop > 100]` | Numeric comparison (`>`, `<`, `>=`, `<=`) |
| `[1]` · `[first()]` · `[last()]` | Position (1-indexed) |
| `[position() > 2]` | Position predicate |
| `[@a='x' and @b='y']` · `or` · `not(...)` | Logical operators |
| `text()` · `count()` | Functions |

```{note}
XPath positions are **1-indexed** (XPath convention); the fluent API uses
**0-based** indices (`at`, `skip`) following C++ convention.
```

## Combining both styles

`.xpath(...)` can be applied mid-chain to the current selection, so you can mix
approaches:

```cpp
auto result = DataTreeQuery::from (appRoot)
    .descendants ("Window")
    .xpath (".//Button[@enabled='true']")   // XPath on the current selection
    .orderByProperty ("text")
    .take (5)
    .nodes();
```

## Lazy evaluation & performance

A query is not executed until a terminal is called; use the cheapest terminal
for the question you're asking:

```cpp
// Prefer any() over nodes().empty(), and count() over nodes().size().
bool hasEnabled = DataTreeQuery::from (appRoot)
    .descendants ("Button")
    .where ([] (const DataTree& b) { return b.getProperty ("enabled", false); })
    .any();     // stops at the first match

// Cache a result when you need it multiple ways.
auto result = DataTreeQuery::from (appRoot).descendants ("Button").execute();
auto all    = result.nodes();
auto count  = result.size();
bool empty  = result.empty();
```

`any()` short-circuits at the first match; `all()` at the first failure.

## Error handling

The engine is forgiving - malformed XPath and empty selections yield empty
results rather than throwing, so defensive checks are simple:

```cpp
auto r = DataTreeQuery::xpath (appRoot, "//Invalid[Syntax");
if (r.empty())
    DBG ("no results (possibly a syntax error)");
```

Guard property extraction with `hasProperty` (or `where`) when data may be
missing.

## See also

- [DataTree](datatree.md) - the structure being queried.
- [Schema & validation](schema.md) - enforce structure so queries can assume it.
