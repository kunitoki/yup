import pytest

import yup

"""
TreeView selection and callbacks, through the gui_app fixture, so they also run under the embedded
interpreter (test_TreeView.py needs juce_app, which only exists in the standalone bindings).

The view is never given a size, so it has no row components and no native window is needed. The
callbacks are only assigned and cleared here: invoking them needs clicks or key presses, and
neither is bound.
"""

pytestmark = pytest.mark.usefixtures("gui_app")


class Item(yup.TreeViewItem):
    def __init__(self, name):
        super().__init__()
        self.name = name

    def getItemText(self):
        return self.name

    def mightContainSubItems(self):
        return self.getNumSubItems() > 0


def new_tree():
    root = Item("root")
    root.addSubItem(Item("a"))
    root.addSubItem(Item("b"))

    tree = yup.TreeView()
    tree.setExpandAnimationTime(0.0)
    tree.setRootItem(root)
    root.setOpen(True)

    return tree


# ==============================================================================

def test_adding_none_is_rejected():
    with pytest.raises((TypeError, ValueError)):
        Item("root").addSubItem(None)


def test_added_items_are_returned():
    root = Item("root")
    child = root.addSubItem(Item("child"))

    assert child.getItemText() == "child"
    assert root.getNumSubItems() == 1


def test_selected_items_are_reported():
    tree = new_tree()
    assert tree.getSelectedItems() == []

    tree.getRootItem().getSubItem(1).setSelected(True)

    assert [item.getItemText() for item in tree.getSelectedItems()] == ["b"]


def test_nothing_is_dragged_without_a_drag():
    assert yup.TreeView.getDraggedItems(yup.DragAndDropSourceDetails()) == []


def test_item_callbacks_can_be_set_and_cleared():
    tree = new_tree()

    for name in ("onItemClicked", "onItemDoubleClicked", "onItemEntered", "onItemExited", "onReturnKeyPressed"):
        setattr(tree, name, lambda item: None)
        setattr(tree, name, None)

    tree.onDeleteKeyPressed = lambda items: None
    tree.onDeleteKeyPressed = None
