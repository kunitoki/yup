# Tree View

`TreeView` (module `yup_gui`) shows a hierarchy of items as an indented,
scrollable list. You describe the hierarchy with `TreeViewItem` subclasses; the
view shows the items whose parents are open, one row each. The rows live in an
internal `ListBox`, so scrolling, recycling, selection and keyboard navigation
behave like in a [list box](list-box.md), and a tree of thousands of items
scrolls as fast as a list.

```cpp
class NoteItem : public yup::TreeViewItem
{
public:
    explicit NoteItem (yup::String text) : text (std::move (text)) {}

    yup::String getItemText() const override { return text; }

private:
    yup::String text;
};

auto root = std::make_unique<NoteItem> ("Notes");
auto& ideas = root->addSubItem (std::make_unique<NoteItem> ("Ideas"));
ideas.addSubItem (std::make_unique<NoteItem> ("A tree view"));

yup::TreeView tree;
tree.setRootItem (std::move (root));
tree.setRootItemVisible (false);
addAndMakeVisible (tree);
```

## Items

An item owns its sub-items (`addSubItem`, `removeSubItem`, `moveSubItem`,
`clearSubItems`) and the tree owns its root. Every change made through an item
updates the tree on its own: rows are inserted or removed only where the change
is visible, and the selection and the scroll position stay where they were.
When what an item shows changes, call `itemChanged()`.

| Override | Purpose |
| --- | --- |
| `getItemText()`, `getItemIcon()` | What the built-in row shows. Long text is truncated with an ellipsis. |
| `mightContainSubItems()` | Whether the item has a disclosure button. Defaults to having sub-items. |
| `getItemHeight()` | The row height; 0 uses `TreeView::getDefaultItemHeight()`. |
| `refreshItemComponent (component)` | Custom row content, see below. |
| `itemOpennessChanged (isNowOpen)` | Called when the item opens or closes: create lazy sub-items here. |
| `itemClicked`, `itemDoubleClicked`, `itemSelectionChanged` | Interaction. A double-click toggles the item by default. |
| `getUniqueName()` | Identifies the item among its siblings when saving the openness state. |
| `getDragSourceDescription()`, `isInterestedInDragSource()`, `itemDropped()` | Drag and drop, see below. |

`setOpen`, `setOpenRecursively`, `setSelected` and `isSelected` drive the state
from code. Only visible items can be selected. Closing an item that holds the
selection moves the selection to the item.

## Opening and closing

The user opens and closes items with the disclosure buttons, with a
double-click, and with the keyboard:

| Key | Action |
| --- | --- |
| Right | Opens the item, or moves to its first sub-item when it is already open. |
| Left | Closes the item, or moves to its parent when it is already closed. |
| Alt + Right / Alt + Left | Opens or closes the whole branch. Alt-click on a disclosure button does the same. |
| Up, Down, Page Up, Page Down, Home, End, Space, Return, Delete | As in a `ListBox`. |

Opening and closing animate for 0.15 seconds: the rows of the sub-items grow
from nothing or shrink away. `setExpandAnimationTime (0.0)` turns the animation
off. It is skipped anyway when the item is out of view or when more than two
screens of rows would move.

## Lazy loading

Return `true` from `mightContainSubItems()` and create the sub-items the first
time the item opens. This is how a file browser avoids reading the whole disk:

```cpp
class FileItem : public yup::TreeViewItem
{
public:
    explicit FileItem (yup::File file) : file (std::move (file)) {}

    yup::String getItemText() const override { return file.getFileName(); }
    yup::String getUniqueName() const override { return file.getFullPathName(); }
    bool mightContainSubItems() const override { return file.isDirectory(); }

    void itemOpennessChanged (bool isNowOpen) override
    {
        if (! isNowOpen || getNumSubItems() > 0)
            return;

        for (const auto& child : file.findChildFiles (yup::File::findFilesAndDirectories, false))
            addSubItem (std::make_unique<FileItem> (child));
    }

private:
    yup::File file;
};
```

A hidden root is always open, so it gets its `itemOpennessChanged (true)` as
soon as it is set.

## Custom content

Override `refreshItemComponent()` to show your own component after the
indentation and the disclosure button. Rows are recycled, so the component
passed in may have shown another item: `reuseOrCreate<T>()` reuses it when it
already is a `T` and creates one otherwise.

```cpp
void refreshItemComponent (std::unique_ptr<yup::Component>& component) override
{
    auto& badge = reuseOrCreate<BadgeComponent> (component);
    badge.setText (getItemText());
    badge.setCount (unreadCount);
}
```

The content should call `setWantsMouseEvents (false, true)` on itself, so that
pressing it still selects the row and only its interactive children take the
press. The default implementation resets the component, which shows the
built-in icon and text.

## Drag and drop

Items are dragged out when their `getDragSourceDescription()` returns something:
the selected items travel together, and the drag carries their descriptions
(an array when there are several). Nothing is draggable by default.

The tree is also a drop target. As the pointer moves, the row under it decides
where the content would land: the top quarter inserts before the row, the
bottom quarter after it (or as the first sub-item of an open item), and the
middle into the item when it `mightContainSubItems()`. The item that would
receive the content answers `isInterestedInDragSource()`, and only then does
the tree show an insertion line or an outline. Hovering over a closed item opens
it after 0.6 seconds, and hovering near the top or bottom edge scrolls.

`TreeView::getDraggedItems()` returns the items being dragged when the drag
started from a tree, which makes reordering a few lines:

```cpp
bool isInterestedInDragSource (const yup::DragAndDropSourceDetails& details) const override
{
    return ! yup::TreeView::getDraggedItems (details).empty();
}

void itemDropped (const yup::DragAndDropSourceDetails& details, int insertIndex) override
{
    for (auto* item : yup::TreeView::getDraggedItems (details))
    {
        auto* oldParent = item->getParentItem();
        const auto oldIndex = item->getIndexInParent();

        if (oldParent == this)
        {
            const auto newIndex = yup::jmin (getNumSubItems() - 1, insertIndex > oldIndex ? insertIndex - 1 : insertIndex);
            moveSubItem (oldIndex, newIndex);
            insertIndex = newIndex + 1;
        }
        else
        {
            addSubItem (oldParent->removeSubItem (oldIndex), insertIndex++);
        }
    }
}
```

The tree never lets content be dropped into one of the dragged items or their
descendants.

## DataTree

`DataTreeViewItem` shows a `DataTree` node and follows it. Its sub-items are
created when it first opens, and from then on children added, removed and moved
in the `DataTree` - including by undo and redo - show up in the tree, and
property changes refresh the row. Override `getItemText()` to show something
other than the node type, and `createSubItem()` to use your own item type for
the children:

```cpp
class NodeItem : public yup::DataTreeViewItem
{
public:
    using DataTreeViewItem::DataTreeViewItem;

    yup::String getItemText() const override { return getDataTree().getProperty ("name").toString(); }

    std::unique_ptr<yup::TreeViewItem> createSubItem (const yup::DataTree& child) override
    {
        return std::make_unique<NodeItem> (child, getUndoManager());
    }
};

tree.setRootItem (std::make_unique<NodeItem> (document, undoManager));
```

Siblings of the same type would share the default unique name, so give them a
distinct `getItemText()` or `getUniqueName()` before saving the openness state.

Dragging and dropping `DataTreeViewItem`s moves their nodes. The move is
recorded in the `UndoManager` given to the item, as a single step even when the
nodes change parent, and the moved items are selected afterwards.

## Saving the openness state

`getOpennessState()` returns a `DataTree` recording which items are open and
selected, by `getUniqueName()`, optionally with the scroll position.
`restoreOpennessState()` applies it again, opening lazy items along the way so
a deep hierarchy comes back whole:

```cpp
const auto state = tree.getOpennessState (true);
// ... later, or after rebuilding the items
tree.restoreOpennessState (state, true);
```

## Styling

The rows use the `ListBox::Style` colors for the background, the selection and
the hover. The current row has no outline unless you set
`ListBox::Style::currentRowOutlineColorId` on the tree. `TreeView::Style` adds
`indentGuideColorId`, `disclosureColorId`, `itemTextColorId`,
`itemTextSelectedColorId` and `dropIndicatorColorId`. Colors set on the
`TreeView` reach its rows. Themes paint the rows through the `ComponentStyle`
registered for `TreeViewRow`, which exposes the row's item, depth, open
fraction and layout. See [component styling](component-styling.md).

## Python

`TreeViewItem`, `DataTreeViewItem`, `TreeView` and `TreeViewRow` are available
in the `yup` Python module. Python subclasses of `TreeViewItem` are handed over
to `addSubItem` and `setRootItem` and stay alive with the tree.
`refreshItemComponent` and `DataTreeViewItem.createSubItem` are not bound, and
the item callbacks (`onItemClicked`, `onItemDoubleClicked`,
`onReturnKeyPressed`, `onDeleteKeyPressed`) can be set but not read back.
