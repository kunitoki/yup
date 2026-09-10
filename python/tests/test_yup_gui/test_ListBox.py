import pytest

import yup

"""
ListBox, its ListBoxModel and the ListBoxItem rows it creates: the item/size/selection state a
Python caller drives, and the model callbacks it receives back.

The rows only exist for a laid-out component, so nothing here builds a real list of components:
a ListBox with no size has no visible rows, which is what keeps these tests free of a native
window. Two things are therefore out of reach and covered elsewhere or deliberately not bound:

- ListBoxModel.paintListBoxItem needs a Graphics, so it is only reachable through a real paint.
- ListBoxModel.refreshComponentForRow is not bound at all: it hands the ListBox ownership of the
  returned component, which pybind11 cannot take away from a Python-owned instance.

The file runs inside yup.TestApplication, the same fixture test_ApplicationTheme.py takes.
ListBoxItem lays its text and icon out through calculateLayout(), which reads the theme font and
metrics; the global theme only exists while an application is initialised, and without it those
lookups dereference a null pointer. Each item under test is given a size for the same reason a
laid-out row has one - calculateLayout() lays out inside the item's bounds.
"""

pytestmark = pytest.mark.usefixtures("juce_app")


class Model(yup.ListBoxModel):
    def __init__(self, rows=3):
        super().__init__()
        self.rows = rows
        self.log = []

    def getNumRows(self):
        return self.rows

    def getRowHeight(self, rowIndex):
        return 20 + rowIndex

    def getRowWidth(self, rowIndex):
        return 30 + rowIndex

    def getRowText(self, rowIndex):
        return f"Row {rowIndex}"

    def selectedRowsChanged(self, selectedRows):
        self.log.append(("selected", list(selectedRows)))

    def rowClicked(self, rowIndex, event):
        self.log.append(("clicked", rowIndex))

    def rowDoubleClicked(self, rowIndex, event):
        self.log.append(("doubleClicked", rowIndex))

    def returnKeyPressed(self, lastSelectedRow):
        self.log.append(("return", lastSelectedRow))

    def deleteKeyPressed(self, selectedRows):
        self.log.append(("delete", list(selectedRows)))

    def getDragSourceDescription(self, selectedRows):
        self.log.append(("drag", list(selectedRows)))

        return "rows"


def new_item():
    item = yup.ListBoxItem()
    item.setSize(200.0, 24.0)

    return item


# ==============================================================================
# ListBoxModel
# ==============================================================================

def test_the_row_count_comes_from_the_override():
    assert Model().getNumRows() == 3
    assert Model(7).getNumRows() == 7


def test_the_base_row_count_is_pure_virtual():
    try:
        yup.ListBoxModel().getNumRows()
    except RuntimeError:
        pass
    else:
        raise AssertionError("ListBoxModel.getNumRows should raise when it is not overridden")


def test_the_override_supplies_the_row_metrics_and_text():
    model = Model()

    assert model.getRowHeight(1) == 21
    assert model.getRowWidth(1) == 31
    assert model.getRowText(2) == "Row 2"


def test_the_selection_callback_receives_the_rows():
    model = Model()
    model.selectedRowsChanged(yup.Array[int]([0, 2]))

    assert model.log == [("selected", [0, 2])]


def test_the_remaining_callbacks_receive_what_they_are_given():
    model = Model()
    event = yup.MouseEvent()

    model.rowClicked(2, event)
    model.rowDoubleClicked(1, event)
    model.returnKeyPressed(2)
    model.deleteKeyPressed(yup.Array[int]([1]))
    description = model.getDragSourceDescription(yup.Array[int]([0]))

    assert description == "rows"
    assert [entry[0] for entry in model.log] == ["clicked", "doubleClicked", "return", "delete", "drag"]
    assert model.log[0][1] == 2
    assert model.log[1][1] == 1


def test_refresh_component_for_row_is_not_exposed():
    # It transfers ownership of the returned component to the ListBox, which is not something
    # that can be expressed safely from Python, so the hook is left out of the bindings.
    assert not hasattr(Model(), "refreshComponentForRow")


# ==============================================================================
# ListBox / ListBoxItem defaults
# ==============================================================================

def test_list_box_defaults():
    box = yup.ListBox()

    assert box.getModel() is None
    assert box.getSelectionMode() == yup.ListBox.SelectionMode.single
    assert box.getOrientation() == yup.ListBox.Orientation.vertical
    assert box.getRowHeight() == 24
    assert box.getRowWidth() == 100
    assert box.isVariableHeightEnabled() is False
    assert box.isVariableWidthEnabled() is False
    assert box.getMinimumContentSize() == 0
    assert box.getNumSelectedRows() == 0
    assert box.getSelectedRow() == -1
    assert box.getSelectedRows().isEmpty() is True


def test_list_box_item_defaults():
    item = yup.ListBoxItem()

    assert item.getText() == ""
    assert item.getIconPosition() == yup.ListBoxItem.IconPosition.left
    assert item.isSelected() is False
    assert item.isHovered() is False


def test_list_box_item_state_round_trips():
    item = new_item()
    item.setText("Hello")
    item.setSelected(True)
    item.setHovered(True)

    assert item.getText() == "Hello"
    assert item.isSelected() is True
    assert item.isHovered() is True


def test_list_box_item_icon_positions_round_trip():
    item = new_item()

    for position in (
        yup.ListBoxItem.IconPosition.left,
        yup.ListBoxItem.IconPosition.right,
        yup.ListBoxItem.IconPosition.above,
        yup.ListBoxItem.IconPosition.below,
    ):
        item.setIconPosition(position)
        assert item.getIconPosition() == position


def test_list_box_item_rendering_bounds_are_empty_without_a_size():
    # Only construction and the two getters run here, so nothing has laid the item out yet.
    item = yup.ListBoxItem()

    assert item.getTextBoundsForRendering().isEmpty() is True
    assert item.getIconBoundsForRendering().isEmpty() is True


# ==============================================================================
# ListBox with a model
# ==============================================================================

def test_the_model_round_trips():
    box = yup.ListBox()
    model = Model()

    box.setModel(model)

    assert box.getModel() is model

    box.setModel(None)

    assert box.getModel() is None


def test_selection_round_trips_through_the_model():
    model = Model(3)
    box = yup.ListBox()
    box.setModel(model)

    box.selectRow(1, False)

    assert box.getSelectedRow() == 1
    assert box.getNumSelectedRows() == 1
    assert box.isRowSelected(1) is True
    assert box.isRowSelected(0) is False
    assert list(box.getSelectedRows()) == [1]
    assert model.log == [("selected", [1])]


def test_selecting_a_row_the_model_does_not_have_does_nothing():
    box = yup.ListBox()
    box.setModel(Model(2))

    box.selectRow(5, False)

    assert box.getNumSelectedRows() == 0


def test_selecting_without_a_model_does_nothing():
    box = yup.ListBox()

    box.selectRow(0, False)

    assert box.getNumSelectedRows() == 0


def test_multiple_selection_accumulates_and_clears():
    model = Model(4)
    box = yup.ListBox()
    box.setSelectionMode(yup.ListBox.SelectionMode.multiple)
    box.setModel(model)

    box.selectRow(0, False)
    box.selectRow(2, False)

    assert list(box.getSelectedRows()) == [0, 2]

    # Only selectRow carries the scrollToShowRow flag; the deselect calls take a notification.
    box.deselectRow(0, yup.NotificationType.dontSendNotification)

    assert list(box.getSelectedRows()) == [2]

    box.deselectAllRows(yup.NotificationType.dontSendNotification)

    assert box.getNumSelectedRows() == 0
    assert box.getSelectedRow() == -1


def test_the_selection_callback_fires_when_the_selection_changes():
    calls = []
    box = yup.ListBox()
    box.setModel(Model(2))
    box.onSelectionChanged = lambda: calls.append("changed")

    box.selectRow(1, False)

    assert calls == ["changed"]


def test_row_sizes_and_flags_round_trip():
    box = yup.ListBox()

    box.setRowHeight(32)
    box.setRowWidth(150)
    box.setVariableHeightEnabled(True)
    box.setVariableWidthEnabled(True)
    box.setMinimumContentSize(64)

    assert box.getRowHeight() == 32
    assert box.getRowWidth() == 150
    assert box.isVariableHeightEnabled() is True
    assert box.isVariableWidthEnabled() is True
    assert box.getMinimumContentSize() == 64


def test_orientation_and_selection_mode_round_trip():
    box = yup.ListBox()

    box.setOrientation(yup.ListBox.Orientation.horizontal)
    box.setSelectionMode(yup.ListBox.SelectionMode.none)

    assert box.getOrientation() == yup.ListBox.Orientation.horizontal
    assert box.getSelectionMode() == yup.ListBox.SelectionMode.none


def test_the_scroll_bars_are_components_of_the_list_box():
    box = yup.ListBox()

    assert isinstance(box.getVerticalScrollBar(), yup.ScrollBar)
    assert isinstance(box.getHorizontalScrollBar(), yup.ScrollBar)

    box.setVerticalScrollBarVisibility(yup.ScrollBar.VisibilityMode.alwaysVisible)
    box.setHorizontalScrollBarVisibility(yup.ScrollBar.VisibilityMode.alwaysHidden)

    assert box.getVerticalScrollBar().getVisibilityMode() == yup.ScrollBar.VisibilityMode.alwaysVisible
    assert box.getHorizontalScrollBar().getVisibilityMode() == yup.ScrollBar.VisibilityMode.alwaysHidden


# ==============================================================================
# Style identifiers
# ==============================================================================

def test_style_ids_are_exposed():
    assert str(yup.ListBox.Style.backgroundColorId) == "listBoxBackground"
    assert str(yup.ListBox.Style.outlineColorId) == "listBoxOutline"
    assert str(yup.ListBox.Style.rowBackgroundColorId) == "rowBackground"
    assert str(yup.ListBox.Style.selectedRowBackgroundColorId) == "selectedRowBackground"
    assert str(yup.ListBox.Style.hoveredRowBackgroundColorId) == "hoveredRowBackground"

    assert str(yup.ListBoxItem.Style.textColorId) == "listBoxItemText"
    assert str(yup.ListBoxItem.Style.textColorSelectedId) == "listBoxItemTextSelected"
    assert str(yup.ListBoxItem.Style.backgroundColorId) == "listBoxItemBackground"
    assert str(yup.ListBoxItem.Style.backgroundColorSelectedId) == "listBoxItemBackgroundSelected"
    assert str(yup.ListBoxItem.Style.backgroundColorHoveredId) == "listBoxItemBackgroundHovered"
