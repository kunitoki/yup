import pytest

import yup

"""
TreeView, TreeViewItem and DataTreeViewItem: the tree a Python caller builds out of its own item
subclasses, the rows and selection the view derives from it, and the callbacks it gets back.

Items are handed to C++ by value: addSubItem and setRootItem take ownership, and the Python object
stays alive with the C++ item, so its overrides keep being called and the variables that still
point at it stay usable. refreshItemComponent and DataTreeViewItem.createSubItem are not bound, for
the same ownership reason as ListBoxModel.refreshRowComponent.

The view is never given a size, so it has no row components and no native window is needed: the
rows and the selection exist without them.
"""

pytestmark = pytest.mark.usefixtures("juce_app")


class Item(yup.TreeViewItem):
    def __init__(self, name, lazy=0):
        super().__init__()
        self.name = name
        self.lazy = lazy
        self.log = []

    def getItemText(self):
        return self.name

    def mightContainSubItems(self):
        return self.lazy > 0 or self.getNumSubItems() > 0

    def itemOpennessChanged(self, isNowOpen):
        self.log.append(("open", isNowOpen))

        if isNowOpen and self.lazy > 0 and self.getNumSubItems() == 0:
            for index in range(self.lazy):
                self.addSubItem(Item(f"{self.name}{index}"))

    def itemSelectionChanged(self, isNowSelected):
        self.log.append(("selected", isNowSelected))


def build():
    root = Item("root")
    a = root.addSubItem(Item("a"))
    a.addSubItem(Item("a0"))
    root.addSubItem(Item("b"))

    return root


def new_tree(root=None):
    tree = yup.TreeView()
    tree.setExpandAnimationTime(0.0)
    tree.setRootItem(root if root is not None else build())

    return tree


def row_names(tree):
    return [tree.getItemOnRow(row).getItemText() for row in range(tree.getNumRowsInTree())]


# ==============================================================================
# TreeViewItem
# ==============================================================================

def test_sub_items_are_owned_by_their_parent():
    root = build()

    assert root.getNumSubItems() == 2
    assert root.getSubItem(0).getItemText() == "a"
    assert root.getSubItem(0).getParentItem().getItemText() == "root"
    assert root.getSubItem(0).getSubItem(0).getDepth() == 2
    assert root.getSubItem(5) is None


def test_added_items_keep_their_python_state():
    root = Item("root")
    added = root.addSubItem(Item("child", lazy=3))

    assert isinstance(added, Item)
    assert added.lazy == 3


def test_adding_none_is_rejected():
    with pytest.raises(ValueError):
        Item("root").addSubItem(None)


def test_removed_items_come_back_to_python():
    root = build()

    removed = root.removeSubItem(1)

    assert removed.getItemText() == "b"
    assert removed.getParentItem() is None
    assert root.getNumSubItems() == 1
    assert root.removeSubItem(7) is None


def test_moving_and_clearing_sub_items():
    root = build()

    root.moveSubItem(0, 1)
    assert [root.getSubItem(i).getItemText() for i in range(2)] == ["b", "a"]

    root.clearSubItems()
    assert root.getNumSubItems() == 0


def test_opening_calls_the_override():
    item = Item("lazy", lazy=2)

    item.setOpen(True)

    assert item.isOpen()
    assert item.getNumSubItems() == 2
    assert item.log == [("open", True)]


# ==============================================================================
# TreeView
# ==============================================================================

def test_the_root_row_comes_first():
    tree = new_tree()

    assert row_names(tree) == ["root"]
    assert tree.getRootItem().getItemText() == "root"
    assert tree.getRootItem().getOwnerView() is not None


def test_opening_items_adds_their_rows():
    tree = new_tree()
    root = tree.getRootItem()

    root.setOpen(True)
    assert row_names(tree) == ["root", "a", "b"]

    root.getSubItem(0).setOpen(True)
    assert row_names(tree) == ["root", "a", "a0", "b"]
    assert tree.getRowOf(root.getSubItem(1)) == 3


def test_lazy_python_items_fill_in_on_open():
    root = Item("r", lazy=2)
    tree = new_tree(root)

    root.setOpen(True)

    assert row_names(tree) == ["r", "r0", "r1"]


def test_a_hidden_root_shows_its_sub_items():
    tree = new_tree()

    tree.setRootItemVisible(False)

    assert not tree.isRootItemVisible()
    assert row_names(tree) == ["a", "b"]


def test_changes_to_python_items_update_the_rows():
    tree = new_tree()
    root = tree.getRootItem()
    root.setOpen(True)

    root.addSubItem(Item("c"), 0)
    assert row_names(tree) == ["root", "c", "a", "b"]

    root.removeSubItem(0)
    assert row_names(tree) == ["root", "a", "b"]


def test_selection_is_reported_by_item():
    tree = new_tree()
    root = tree.getRootItem()
    root.setOpen(True)

    calls = []
    tree.onSelectionChanged = lambda: calls.append(True)

    b = root.getSubItem(1)
    b.setSelected(True)

    assert b.isSelected()
    assert tree.getNumSelectedItems() == 1
    assert [item.getItemText() for item in tree.getSelectedItems()] == ["b"]
    assert ("selected", True) in b.log
    assert calls == [True]

    tree.clearSelectedItems()
    assert tree.getNumSelectedItems() == 0


def test_collapsing_moves_the_selection_to_the_item():
    tree = new_tree()
    root = tree.getRootItem()
    root.setOpen(True)
    a = root.getSubItem(0)
    a.setOpen(True)

    a.getSubItem(0).setSelected(True)
    a.setOpen(False)

    assert a.isSelected()
    assert row_names(tree) == ["root", "a", "b"]


def test_the_openness_state_round_trips():
    tree = new_tree()
    root = tree.getRootItem()
    root.setOpen(True)
    root.getSubItem(0).setOpen(True)

    state = tree.getOpennessState(False)

    root.setOpenRecursively(False)
    assert row_names(tree) == ["root"]

    tree.restoreOpennessState(state, False)
    assert row_names(tree) == ["root", "a", "a0", "b"]


def test_callbacks_can_be_set_and_cleared():
    tree = new_tree()

    tree.onItemClicked = lambda item: None
    tree.onItemDoubleClicked = lambda item: None
    tree.onReturnKeyPressed = lambda item: None
    tree.onDeleteKeyPressed = lambda items: None

    tree.onItemClicked = None
    tree.onDeleteKeyPressed = None


def test_layout_settings_round_trip():
    tree = new_tree()

    tree.setIndentSize(12.0)
    tree.setDefaultItemHeight(30.0)
    tree.setIndentGuidesVisible(False)
    tree.setOpenCloseButtonsVisible(False)
    tree.setSelectionMode(yup.ListBox.SelectionMode.multiple)

    assert tree.getIndentSize() == 12.0
    assert tree.getDefaultItemHeight() == 30.0
    assert not tree.areIndentGuidesVisible()
    assert not tree.areOpenCloseButtonsVisible()
    assert tree.getSelectionMode() == yup.ListBox.SelectionMode.multiple


def test_nothing_is_dragged_without_a_drag():
    assert yup.TreeView.getDraggedItems(yup.DragAndDropSourceDetails()) == []


def test_style_ids_are_exposed():
    assert str(yup.TreeView.Style.indentGuideColorId) == "treeViewIndentGuide"
    assert str(yup.TreeView.Style.disclosureColorId) == "treeViewDisclosure"
    assert str(yup.TreeView.Style.itemTextColorId) == "treeViewItemText"
    assert str(yup.TreeView.Style.itemTextSelectedColorId) == "treeViewItemTextSelected"
    assert str(yup.TreeView.Style.dropIndicatorColorId) == "treeViewDropIndicator"


# ==============================================================================
# DataTreeViewItem
# ==============================================================================

def make_document():
    document = yup.DataTree(yup.Identifier("Root"))

    transaction = document.beginTransaction()
    transaction.addChild(yup.DataTree(yup.Identifier("First")))
    transaction.addChild(yup.DataTree(yup.Identifier("Second")))
    transaction.commit()

    return document


def test_data_tree_items_mirror_the_nodes():
    document = make_document()
    tree = yup.TreeView()
    tree.setRootItemVisible(False)
    tree.setRootItem(yup.DataTreeViewItem(document))

    assert row_names(tree) == ["First", "Second"]

    transaction = document.beginTransaction()
    transaction.addChild(yup.DataTree(yup.Identifier("Third")))
    transaction.commit()

    assert row_names(tree) == ["First", "Second", "Third"]


def test_data_tree_items_keep_their_undo_manager():
    manager = yup.UndoManager()
    item = yup.DataTreeViewItem(make_document(), manager)

    assert item.getUndoManager() is not None
    assert str(item.getDataTree().getType()) == "Root"
    assert item.mightContainSubItems()
