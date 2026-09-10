import os

import pytest

import yup

#==================================================================================================

_FONT_CANDIDATES = [
    "/System/Library/Fonts/Supplemental/Arial.ttf",
    "/System/Library/Fonts/Supplemental/Times New Roman.ttf",
    "/System/Library/Fonts/Helvetica.ttc",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/dejavu/DejaVuSans.ttf",
]


def _load_any_font() -> yup.Font:
    for path in _FONT_CANDIDATES:
        if not os.path.exists(path):
            continue

        try:
            font = yup.Font.loadFontFromFile(yup.File(path))
        except ValueError:
            continue

        if not font.isEmpty():
            return font

    pytest.skip("no usable system font file found for styled text tests")

#==================================================================================================

def test_construct_is_empty():
    text = yup.StyledText()
    assert text.isEmpty()

#==================================================================================================

def test_enums():
    assert yup.StyledText.HorizontalAlign.left.name == "left"
    assert yup.StyledText.HorizontalAlign.center.name == "center"
    assert yup.StyledText.VerticalAlign.middle.name == "middle"
    assert yup.StyledText.TextOverflow.ellipsis.name == "ellipsis"
    assert yup.StyledText.TextWrap.noWrap.name == "noWrap"
    assert yup.StyledText.TextOrigin.baseline.name == "baseline"

#==================================================================================================

def test_update_with_text_and_settings():
    font = _load_any_font()
    text = yup.StyledText()

    modifier = text.startUpdate()
    modifier.setHorizontalAlign(yup.StyledText.HorizontalAlign.center)
    modifier.setVerticalAlign(yup.StyledText.VerticalAlign.middle)
    modifier.setOverflow(yup.StyledText.TextOverflow.ellipsis)
    modifier.setWrap(yup.StyledText.TextWrap.noWrap)
    modifier.setParagraphSpacing(4.0)
    modifier.setMaxSize(yup.Size[float](200.0, 50.0))
    modifier.appendText("Hello styled world", font)
    modifier.appendText(" in color", yup.Colors.red, font)
    del modifier  # commit the update

    assert not text.isEmpty()
    assert text.getHorizontalAlign() == yup.StyledText.HorizontalAlign.center
    assert text.getVerticalAlign() == yup.StyledText.VerticalAlign.middle
    assert text.getOverflow() == yup.StyledText.TextOverflow.ellipsis
    assert text.getWrap() == yup.StyledText.TextWrap.noWrap
    assert text.getParagraphSpacing() == 4.0
    assert text.getMaxSize() == yup.Size[float](200.0, 50.0)

#==================================================================================================

def test_computed_bounds_after_update():
    font = _load_any_font()
    text = yup.StyledText()

    modifier = text.startUpdate()
    modifier.appendText("hello", font)
    del modifier

    bounds = text.getComputedTextBounds()
    assert bounds.getWidth() > 0.0
    assert bounds.getHeight() > 0.0

#==================================================================================================

def test_caret_and_selection_helpers():
    font = _load_any_font()
    text = yup.StyledText()

    modifier = text.startUpdate()
    modifier.appendText("hello world", font)
    del modifier

    assert text.isValidCharacterIndex(0)
    assert not text.isValidCharacterIndex(100000)

    caret = text.getCaretBounds(1)
    assert caret.getWidth() >= 0.0

#==================================================================================================

def test_align_from_justification():
    justified = yup.Justification(yup.Justification.Flags.center)
    assert yup.StyledText.horizontalAlignFromJustification(justified) == yup.StyledText.HorizontalAlign.center
    assert yup.StyledText.verticalAlignFromJustification(justified) == yup.StyledText.VerticalAlign.middle
