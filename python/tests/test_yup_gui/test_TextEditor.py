import pytest

import yup

"""
TextEditor is a component that edits text and implements TextInputTarget, so these tests cover
the editing model: the text, the caret, the selection and the change notification, plus the
text-input half the bindings expose directly on the class.

Not covered: copy/cut/paste go through the system clipboard and the on-screen keyboard path
needs a focused, on-desktop component, so the clipboard methods are left out. Painting and the
caret blink need a rendering context and a running timer.

The file runs inside yup.TestApplication, the same fixture test_ApplicationTheme.py takes:
laying text out asks ApplicationTheme for the theme font, and the global theme only exists
while an application is initialised. Without it the lookup dereferences a null pointer.
"""

pytestmark = pytest.mark.usefixtures("juce_app")


def new_editor():
    """A sized editor: the styled text is laid out into the component's bounds."""
    editor = yup.TextEditor()
    editor.setSize(240.0, 120.0)

    return editor


class EditorWithRect(yup.TextEditor):
    """Overrides the TextInputTarget hook, which the trampoline routes back into Python."""

    def getTextInputRect(self):
        return yup.Rectangle[float](1.0, 2.0, 3.0, 4.0)


# ==============================================================================
# Defaults
# ==============================================================================

def test_defaults():
    editor = new_editor()

    assert editor.getText() == ""
    assert editor.isMultiLine() is False
    assert editor.isReadOnly() is False
    assert editor.getCaretPosition() == 0
    assert editor.isCaretVisible() is True
    assert editor.hasSelection() is False
    assert editor.getSelectedText() == ""
    assert editor.getFont() is None
    assert editor.getFontSize() is None
    assert editor.getScrollOffset() == yup.Point[float](0.0, 0.0)
    assert editor.isTextInputActive() is False


# ==============================================================================
# Setting the text
# ==============================================================================

def test_set_text_replaces_the_text_and_resets_the_caret():
    editor = new_editor()
    editor.setText("hello")

    assert editor.getText() == "hello"
    assert editor.getCaretPosition() == 0
    assert editor.hasSelection() is False


def test_set_text_strips_carriage_returns():
    editor = new_editor()
    editor.setText("a\r\nb")

    assert editor.getText() == "a\nb"


def test_insert_text_goes_in_at_the_caret():
    editor = new_editor()
    editor.insertText("abc")

    assert editor.getText() == "abc"
    assert editor.getCaretPosition() == 3

    editor.insertText("!")

    assert editor.getText() == "abc!"
    assert editor.getCaretPosition() == 4


def test_newlines_become_spaces_when_not_multiline():
    editor = new_editor()
    editor.insertText("a\nb")

    assert editor.getText() == "a b"


def test_multiline_keeps_the_newlines():
    editor = new_editor()
    editor.setMultiLine(True)
    editor.insertText("a\nb")

    assert editor.isMultiLine() is True
    assert editor.getText() == "a\nb"


def test_a_read_only_editor_ignores_inserted_text():
    editor = new_editor()
    editor.setReadOnly(True)
    editor.insertText("abc")

    assert editor.isReadOnly() is True
    assert editor.getText() == ""


# ==============================================================================
# Change notification
# ==============================================================================

def test_the_change_callback_fires_when_the_text_changes():
    calls = []
    editor = new_editor()
    editor.onTextChange = lambda: calls.append("changed")

    editor.setText("hello")

    assert calls == ["changed"]


def test_setting_the_same_text_does_not_notify():
    calls = []
    editor = new_editor()
    editor.setText("hello")
    editor.onTextChange = lambda: calls.append("changed")

    editor.setText("hello")

    assert calls == []


def test_inserting_text_notifies():
    calls = []
    editor = new_editor()
    editor.onTextChange = lambda: calls.append("changed")

    editor.insertText("hello")

    assert calls == ["changed"]


# ==============================================================================
# Caret and selection
# ==============================================================================

def test_the_caret_is_clamped_to_the_text():
    editor = new_editor()
    editor.setText("abc")

    editor.setCaretPosition(99)
    assert editor.getCaretPosition() == 3

    editor.setCaretPosition(-5)
    assert editor.getCaretPosition() == 0


def test_the_selection_round_trips():
    editor = new_editor()
    editor.setText("hello")
    editor.setSelection(yup.Range[int](1, 3))

    assert editor.hasSelection() is True
    assert editor.getSelectedText() == "el"
    assert editor.getSelection().getStart() == 1
    assert editor.getSelection().getEnd() == 3
    assert editor.getCaretPosition() == 3


def test_select_all_selects_the_whole_text():
    editor = new_editor()
    editor.setText("abc")
    editor.selectAll()

    assert editor.getSelectedText() == "abc"


def test_the_selection_is_clamped_to_the_text():
    editor = new_editor()
    editor.setText("abc")
    editor.setSelection(yup.Range[int](0, 99))

    assert editor.getSelection().getEnd() == 3


def test_deleting_the_selection_removes_only_that_range():
    editor = new_editor()
    editor.setText("hello")
    editor.setSelection(yup.Range[int](1, 3))
    editor.deleteSelectedText()

    assert editor.getText() == "hlo"
    assert editor.hasSelection() is False


def test_deleting_without_a_selection_does_nothing():
    editor = new_editor()
    editor.setText("hello")
    editor.deleteSelectedText()

    assert editor.getText() == "hello"


# ==============================================================================
# Font
# ==============================================================================

def test_the_font_size_round_trips_and_can_be_reset():
    editor = new_editor()
    editor.setFontSize(18.0)

    assert editor.getFontSize() == 18.0

    editor.resetFontSize()

    assert editor.getFontSize() is None


# ==============================================================================
# TextInputTarget
# ==============================================================================

def test_a_subclass_override_supplies_the_text_input_rect():
    assert EditorWithRect().getTextInputRect() == yup.Rectangle[float](1.0, 2.0, 3.0, 4.0)


def test_the_editor_reports_a_text_input_rect():
    editor = new_editor()

    assert isinstance(editor.getTextInputRect(), yup.Rectangle[float])


def test_text_input_bookkeeping_is_inert_without_a_native_component():
    # The editor is not on the desktop, so the calls that would talk to the on-screen keyboard
    # have nowhere to go; only the flag the interface keeps is expected to move.
    editor = new_editor()
    editor.requestTextInput()

    assert editor.isTextInputActive() is True

    editor.updateTextInputRect()
    editor.relinquishTextInput()

    assert editor.isTextInputActive() is False


# ==============================================================================
# Style identifiers
# ==============================================================================

def test_style_ids_are_exposed():
    assert str(yup.TextEditor.Style.backgroundColorId) == "textEditorBackground"
    assert str(yup.TextEditor.Style.textColorId) == "textEditorText"
    assert str(yup.TextEditor.Style.caretColorId) == "textEditorCaret"
    assert str(yup.TextEditor.Style.selectionColorId) == "textEditorSelection"
    assert str(yup.TextEditor.Style.outlineColorId) == "textEditorOutline"
    assert str(yup.TextEditor.Style.focusedOutlineColorId) == "textEditorFocusedOutline"


def test_style_is_not_constructible():
    try:
        yup.TextEditor.Style()
    except TypeError:
        pass
    else:
        raise AssertionError("TextEditor.Style should not be constructible")
