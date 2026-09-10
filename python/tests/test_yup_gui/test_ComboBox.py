import pytest

import yup

"""
ComboBox is a Component over an item list: the items and their ids, which one is selected, and
the notification that fires when that changes. The drop-down itself needs a live popup, so
nothing here opens it - the item model, the selection and the notification contract are what is
covered, which is the part a Python caller drives.

A real click that changes the selection is delivered by the platform, exactly like it is for
the C++ widgets; calling setSelectedItemIndex is the same path without a window.

The file runs inside yup.TestApplication, the same fixture test_ApplicationTheme.py takes:
updateDisplayText() lays the item text out with the theme font, and the global theme only
exists while an application is initialised. Without it the lookup dereferences a null pointer.
"""

pytestmark = pytest.mark.usefixtures("juce_app")


def new_combo():
    """A sized ComboBox: updateDisplayText sizes the font from the component's height."""
    combo = yup.ComboBox()
    combo.setSize(200.0, 30.0)

    return combo


class RecordingComboBox(yup.ComboBox):
    def __init__(self):
        super().__init__()
        self.setSize(200.0, 30.0)
        self.changes = 0

    def selectedItemChanged(self):
        self.changes += 1


def filled_combo():
    combo = new_combo()
    combo.addItem("One", 1)
    combo.addItem("Two", 2)

    return combo


# ==============================================================================
# Empty state
# ==============================================================================

def test_defaults():
    combo = new_combo()

    assert combo.getNumItems() == 0
    assert combo.getSelectedItemIndex() == -1
    assert combo.getSelectedId() == 0
    assert combo.getText() == ""
    assert combo.getTextWhenNothingSelected() == ""
    assert combo.isTextEditable() is False
    assert combo.isPopupShown() is False


# ==============================================================================
# Items
# ==============================================================================

def test_items_round_trip():
    combo = filled_combo()

    assert combo.getNumItems() == 2
    assert combo.getItemText(0) == "One"
    assert combo.getItemText(1) == "Two"
    assert combo.getItemId(0) == 1
    assert combo.getItemId(1) == 2


def test_add_item_list_numbers_the_items_from_the_first_id():
    combo = new_combo()
    combo.addItemList(yup.StringArray(["A", "B", "C"]), 10)

    assert combo.getNumItems() == 3
    assert [combo.getItemId(index) for index in range(3)] == [10, 11, 12]
    assert [combo.getItemText(index) for index in range(3)] == ["A", "B", "C"]


def test_out_of_range_lookups_return_empty_values():
    combo = filled_combo()

    assert combo.getItemText(5) == ""
    assert combo.getItemText(-1) == ""
    assert combo.getItemId(5) == 0


def test_item_text_can_be_changed():
    combo = filled_combo()
    combo.changeItemText(1, "Second")

    assert combo.getItemText(1) == "Second"


def test_clear_empties_the_items_and_the_selection():
    combo = filled_combo()
    combo.setSelectedItemIndex(0)
    combo.clear()

    assert combo.getNumItems() == 0
    assert combo.getSelectedItemIndex() == -1
    assert combo.getSelectedId() == 0


def test_a_separator_is_an_item_but_is_not_selectable():
    combo = new_combo()
    combo.addItem("One", 1)
    combo.addSeparator()
    combo.addItem("Two", 2)

    assert combo.getNumItems() == 3

    combo.setSelectedItemIndex(1)

    assert combo.getSelectedItemIndex() == -1


# ==============================================================================
# Selection
# ==============================================================================

def test_selecting_by_index_updates_the_id_and_the_text():
    combo = filled_combo()
    combo.setSelectedItemIndex(1)

    assert combo.getSelectedItemIndex() == 1
    assert combo.getSelectedId() == 2
    assert combo.getText() == "Two"


def test_selecting_by_id_updates_the_index_and_the_text():
    combo = filled_combo()
    combo.setSelectedId(1)

    assert combo.getSelectedItemIndex() == 0
    assert combo.getSelectedId() == 1
    assert combo.getText() == "One"


def test_an_unknown_id_cannot_be_selected():
    combo = filled_combo()
    combo.setSelectedId(99)

    assert combo.getSelectedItemIndex() == -1
    assert combo.getSelectedId() == 0


def test_the_placeholder_is_shown_while_nothing_is_selected():
    combo = filled_combo()
    combo.setTextWhenNothingSelected("Pick one")

    assert combo.getTextWhenNothingSelected() == "Pick one"
    assert combo.getText() == "Pick one"

    combo.setSelectedItemIndex(0)

    assert combo.getText() == "One"


def test_editable_text_can_be_toggled():
    combo = new_combo()
    combo.setEditableText(True)

    assert combo.isTextEditable() is True


# ==============================================================================
# Change notification
# ==============================================================================

def test_a_selection_change_notifies_the_override_and_the_callback():
    calls = []
    combo = RecordingComboBox()
    combo.onSelectedItemChanged = lambda: calls.append("callback")
    combo.addItem("One", 1)
    combo.addItem("Two", 2)

    combo.setSelectedItemIndex(1)

    assert combo.changes == 1
    assert calls == ["callback"]


def test_an_unchanged_selection_does_not_notify():
    calls = []
    combo = RecordingComboBox()
    combo.onSelectedItemChanged = lambda: calls.append("callback")
    combo.addItem("One", 1)

    combo.setSelectedItemIndex(0)
    combo.setSelectedItemIndex(0)

    assert combo.changes == 1
    assert calls == ["callback"]


def test_an_override_receives_the_change_without_the_callback():
    combo = RecordingComboBox()
    combo.addItem("One", 1)

    combo.setSelectedItemIndex(0)

    assert combo.changes == 1


# ==============================================================================
# Style identifiers
# ==============================================================================

def test_style_ids_are_exposed():
    assert str(yup.ComboBox.Style.backgroundColorId) == "comboBoxBackground"
    assert str(yup.ComboBox.Style.textColorId) == "comboBoxText"
    assert str(yup.ComboBox.Style.borderColorId) == "comboBoxBorder"
    assert str(yup.ComboBox.Style.arrowColorId) == "comboBoxArrow"
    assert str(yup.ComboBox.Style.focusedBorderColorId) == "comboBoxFocusedBorder"


def test_style_is_not_constructible():
    try:
        yup.ComboBox.Style()
    except TypeError:
        pass
    else:
        raise AssertionError("ComboBox.Style should not be constructible")
