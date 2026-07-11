# Transactions

Every change to a [`DataTree`](datatree.md) is made through a **transaction**.
Transactions batch a set of edits and apply them atomically: listeners are
notified only after a successful commit, and the whole batch can be tied to an
[undo manager](#undo-and-redo).

## The transaction lifecycle

`beginTransaction()` returns a move-only RAII `Transaction`. It **commits
automatically** when it goes out of scope, unless you abort it first.

```cpp
DataTree settings ("Settings");

{
    auto tx = settings.beginTransaction();
    tx.setProperty ("theme", "dark");
    tx.setProperty ("fontSize", 14);
}   // <- commits here
```

For conditional edits, commit or abort explicitly:

```cpp
auto tx = settings.beginTransaction();
tx.setProperty ("experimental", true);

if (shouldKeep)
    tx.commit();     // apply now; transaction becomes inactive
else
    tx.abort();      // discard all batched changes
```

| Method | Description |
| ------ | ----------- |
| `commit()` | Applies all batched changes and notifies listeners. |
| `abort()` | Discards all batched changes. |
| `isActive()` | True until committed or aborted. |

## Property edits

```cpp
{
    auto tx = settings.beginTransaction();
    tx.setProperty ("theme", "dark");
    tx.removeProperty ("legacyOption");
    tx.removeAllProperties();   // clear everything
}
```

## Child edits

```cpp
DataTree parent ("Parent");
DataTree a ("Child"), b ("Child");

{
    auto tx = parent.beginTransaction();

    tx.addChild (a, 0);     // insert at index 0
    tx.addChild (b);        // append (index = -1)

    tx.moveChild (1, 0);    // move from index 1 to 0

    tx.removeChild (a);     // remove a specific child
    tx.removeChild (0);     // remove by index
    // tx.removeAllChildren();
}
```

```{note}
Batched edits within one transaction are applied in order on commit. Keeping
related edits in a single transaction produces a single, coherent set of
notifications (and a single undo step).
```

## Undo and redo

Pass an `UndoManager` to `beginTransaction()` to make a transaction undoable.
Name the step on the undo manager, not the transaction:

```cpp
UndoManager undoManager;

undoManager.beginNewTransaction ("Change language");
{
    auto tx = settings.beginTransaction (&undoManager);
    tx.setProperty ("language", "en");
    tx.setProperty ("region", "US");
}

undoManager.undo();   // revert both property changes as one step
undoManager.redo();   // re-apply them
```

Group several transactions into one undoable step by calling
`beginNewTransaction()` once before them; every transaction passed the same
`UndoManager` between named steps is coalesced.

## Schema-validated transactions

When you have a [`DataTreeSchema`](schema.md), use `beginValidatedTransaction()`
to reject invalid edits. Each operation returns a
[`Result`](../core/results-and-errors.md), and the transaction only commits if
all operations succeeded — see [Schema & validation](schema.md#validated-transactions).

```cpp
auto tx = settings.beginValidatedTransaction (schema);

auto r = tx.setProperty ("theme", "dark");   // returns yup::Result
if (r.failed())
    DBG (r.getErrorMessage());
```

## See also

- [DataTree](datatree.md) — the structure being mutated.
- [Schema & validation](schema.md) — validated transactions.
- [Core: results & error handling](../core/results-and-errors.md)
