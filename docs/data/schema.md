# Schema & Validation

`DataTreeSchema` describes the allowed structure of a [`DataTree`](datatree.md)
using **JSON Schema**-style rules: node types, their properties (types,
defaults, ranges, enums, patterns), and child constraints. A schema lets you
create nodes with defaults pre-applied, validate whole trees, and run
[validated transactions](#validated-transactions) that reject invalid edits.

## Defining a schema

Schemas are declared in JSON and loaded into a reference-counted
`DataTreeSchema::Ptr`:

```cpp
String schemaJson = R"({
    "nodeTypes": {
        "AppSettings": {
            "description": "Application configuration root",
            "properties": {
                "version": {
                    "type": "string",
                    "required": true,
                    "default": "1.2.3",
                    "pattern": "^\\d+\\.\\d+\\.\\d+$"
                },
                "theme": {
                    "type": "string",
                    "default": "light",
                    "enum": ["light", "dark", "auto"]
                },
                "fontSize": {
                    "type": "number",
                    "default": 12,
                    "minimum": 8,
                    "maximum": 72
                }
            },
            "children": {
                "allowedTypes": ["ServerConfig", "UIConfig"],
                "maxCount": 10
            }
        },
        "ServerConfig": {
            "properties": {
                "port":     { "type": "number", "default": 8080, "minimum": 1, "maximum": 65535 },
                "hostname": { "type": "string", "default": "localhost" }
            },
            "children": { "maxCount": 0 }
        }
    }
})";

DataTreeSchema::Ptr schema = DataTreeSchema::fromJsonSchemaString (schemaJson);
if (schema == nullptr)
{
    DBG ("Failed to load schema");
    return;
}
```

`fromJsonSchema (const var&)` is also available when you already have parsed
JSON.

## Creating nodes with defaults

```cpp
// A node with all defaults applied.
DataTree settings = schema->createNode ("AppSettings");
// → version="1.2.3", theme="light", fontSize=12

// A valid child of a given parent type.
DataTree server = schema->createChildNode ("AppSettings", "ServerConfig");
// → port=8080, hostname="localhost"
```

## Inspecting the schema

```cpp
auto theme = schema->getPropertyInfo ("AppSettings", "theme");
DBG (theme.type);                     // "string"
DBG (theme.defaultValue.toString());  // "light"
DBG (theme.enumValues.size());        // 3

auto children = schema->getChildConstraints ("AppSettings");
DBG (children.maxCount);               // 10
DBG (children.allowedTypes.size());    // 2
```

## Validated transactions

`beginValidatedTransaction (schema)` returns a transaction whose operations each
return a [`Result`](../core/results-and-errors.md). Invalid edits are rejected,
and the transaction commits only if every operation succeeded.

```cpp
DataTree settings = schema->createNode ("AppSettings");
auto tx = settings.beginValidatedTransaction (schema);

auto ok1 = tx.setProperty ("theme", "dark");     // valid enum
jassert (ok1.wasOk());

auto ok2 = tx.setProperty ("fontSize", 16);      // within range
jassert (ok2.wasOk());

auto bad1 = tx.setProperty ("theme", "invalid"); // not in enum
jassert (bad1.failed());
DBG (bad1.getErrorMessage());

auto bad2 = tx.setProperty ("fontSize", 100);    // out of range
jassert (bad2.failed());

// Create and add a valid child (defaults applied), returning a ResultValue.
auto childResult = tx.createAndAddChild ("ServerConfig");
if (childResult.wasOk())
    DataTree server = childResult.getValue();
```

| Operation | Returns |
| --------- | ------- |
| `setProperty (name, value)` | `Result` |
| `removeProperty (name)` | `Result` |
| `createAndAddChild (type, index = -1)` | `ResultValue<DataTree>` |

See [Transactions](transactions.md) for the non-validated transaction API.

## Validating an existing tree

Validate a whole subtree at once - useful after loading data from disk or the
network:

```cpp
auto result = schema->validate (settings);
if (result.failed())
    DBG ("Validation failed: " << result.getErrorMessage());
else
    DBG ("Tree is valid");
```

Finer-grained checks are available via `validatePropertyValue (nodeType, name,
value)` and `validateChildAddition (parentType, childType)`.

## See also

- [DataTree](datatree.md) · [Transactions](transactions.md)
- [Core: serialization](../core/serialisation.md) - persist validated trees.
