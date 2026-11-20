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
| `abort()` | Discards all batched changes. Listeners are [told it was aborted](datatree.md#following-a-transaction). |
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

### Several children at once

On a node with many children, adding, removing or moving them one call at a time
gets slow, as every call shifts the children after it. These edits do the same
work in one go:

```cpp
auto tx = parent.beginTransaction();

tx.addChildren (newRows, 0);        // insert them together at index 0 (-1 appends)
tx.removeChildren (10, 100);        // remove 100 children, starting at index 10
tx.removeChildren (selectedRows);   // remove these children
tx.reorderChildren (sortedRows);    // all the children, in their new order
```

| Method | Equals |
| ------ | ------ |
| `addChildren (children, index)` | `addChild()` for each tree in order, the first one at `index`. The trees `addChild()` ignores are skipped, and so is a tree listed twice. |
| `removeChildren (start, count)` | `removeChild (index)` from the last index of the range down to the first. |
| `removeChildren (children)` | `removeChild()` on each listed child, from the last one in the node to the first, whatever the order they are listed in. |
| `reorderChildren (newOrder)` | `moveChild()` bringing each child to its new index, from the first to the last. Ignored unless `newOrder` holds exactly the children of the node, each once. |

Listeners get the callbacks of the calls on the right, in the same order, and undo
works the same. The one difference is that the node is already in its final state
when the callbacks arrive.

```{note}
`addChildren()` inserts together the children that have no parent yet. Children
taken from another node are moved one at a time, as `addChild()` does.
```

```{note}
Batched edits within one transaction are applied in order on commit. Keeping
related edits in a single transaction produces a single, coherent set of
notifications (and a single undo step).
```

## Editing several nodes together

`DataTree::BatchTransaction` commits the edits of several nodes as one
transaction:

```cpp
DataTree::BatchTransaction batch (&undoManager);

for (const auto& row : selectedRows)
    batch.edit (row).setProperty ("selected", false);

// commits here, or call batch.commit() / batch.abort()
```

`edit()` returns the pending transaction of a node, the same one each time. The
result is what committing those transactions one after the other would give, in
the order the nodes were first edited, with two differences: the listeners of all
the edited nodes are told about [one transaction](datatree.md#following-a-transaction),
and the batch is a single undoable action. It is not faster than one transaction
per node.

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
all operations succeeded - see [Schema & validation](schema.md#validated-transactions).

```cpp
auto tx = settings.beginValidatedTransaction (schema);

auto r = tx.setProperty ("theme", "dark");   // returns yup::Result
if (r.failed())
    YUP_DBG (r.getErrorMessage());
```

## See also

- [DataTree](datatree.md) - the structure being mutated.
- [Schema & validation](schema.md) - validated transactions.
- [Core: results & error handling](../core/results-and-errors.md)
