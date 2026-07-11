# CachedValue

`CachedValue<T>` gives fast, **typed** access to a single [`DataTree`](datatree.md)
property. It reads through a local cache (so repeated reads are cheap) and
automatically refreshes when the underlying property changes — it is itself a
`DataTree` listener. Writing through it starts a transaction for you.

## Basic usage

Bind a `CachedValue` to a tree, a property name, and an optional default:

```cpp
class AppComponent
{
public:
    explicit AppComponent (const DataTree& tree)
        : settings (tree)
        , theme    (tree, "theme", "light")   // (tree, property, default)
        , fontSize (tree, "fontSize", 12)
        , enabled  (tree, "enabled", true)
    {
    }

    void demo()
    {
        String t = theme.get();     // fast cached read
        theme.set ("dark");         // starts a transaction + notifies listeners
        int sz = fontSize;          // implicit conversion (operator T)
    }

private:
    DataTree settings;
    CachedValue<String> theme;
    CachedValue<int>    fontSize;
    CachedValue<bool>   enabled;
};
```

| Member | Description |
| ------ | ----------- |
| `get()` / `operator T()` | Returns the cached value (or the default). |
| `set (value)` | Writes the property via a transaction (type-converted). |
| `getDefault()` / `setDefault (value)` | The fallback used when the property is absent. |
| `isUsingDefault()` | True when the property doesn't exist and the default is in effect. |
| `refresh()` | Forces a re-read from the tree. |

```{note}
`set()` converts `T` to `var` via `VariantConverter<T>` and applies the change in
its own transaction, so it triggers the normal
[change notifications](datatree.md#change-notifications). If conversion fails it
is silently ignored.
```

## Reactive updates

Because a `CachedValue` listens to its tree, external edits are reflected
automatically — no manual refresh needed:

```cpp
AppComponent component (settingsTree);

// An external change, elsewhere in the app:
{
    auto tx = settingsTree.beginTransaction();
    tx.setProperty ("theme", "dark");
}

String current = component.theme.get();   // "dark"
```

## Thread-safe access

`AtomicCachedValue<T>` has the same interface but performs atomic reads/writes,
making it safe to read from one thread while another mutates the tree — useful
for values shared between the UI and audio threads.

```cpp
class ConnectionState
{
public:
    explicit ConnectionState (const DataTree& tree)
        : count  (tree, "connections", 0)
        , status (tree, "status", "disconnected")
    {
    }

    void increment()          { count.set (count.get() + 1); }  // atomic
    String getStatus() const  { return status.get(); }          // atomic read

private:
    AtomicCachedValue<int>    count;
    AtomicCachedValue<String> status;
};
```

```{tip}
Use `CachedValue` for single-thread UI/model code and `AtomicCachedValue` when a
property is touched from more than one thread. For collections of tree-backed
objects, combine either with a [DataTreeObjectList](object-list.md).
```

## See also

- [DataTree](datatree.md) — properties and change notifications.
- [Object lists](object-list.md) — objects that expose `CachedValue` members.
- [Core: multithreading](../multithreading/index.md) — thread-safety context.
