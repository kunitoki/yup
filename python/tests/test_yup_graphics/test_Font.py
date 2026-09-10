import os

import pytest

import yup

#==================================================================================================

# Candidate system fonts (TTF/OTF) across platforms; tests are skipped when none is present.
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

    pytest.skip("no usable system font file found for font tests")

#==================================================================================================

def test_default_font_is_empty():
    font = yup.Font()
    assert font.isEmpty()

#==================================================================================================

def test_load_font_from_file():
    font = _load_any_font()
    assert not font.isEmpty()

def test_font_copy():
    font = _load_any_font()
    copy = yup.Font(font)
    assert copy == font
    assert not (copy != font)

#==================================================================================================

def test_font_metrics():
    font = _load_any_font()
    assert font.getHeight() > 0.0
    assert font.getWeight() >= 0
    assert font.getAscent() != 0.0
    assert font.getDescent() != 0.0

def test_font_height_roundtrip():
    font = _load_any_font()

    font.setHeight(24.0)
    assert font.getHeight() == 24.0

    resized = font.withHeight(48.0)
    assert resized.getHeight() == 48.0
    assert font.getHeight() == 24.0  # original untouched by withHeight

#==================================================================================================

def test_font_repr():
    font = _load_any_font()
    assert "Font" in repr(font)
    assert "24" in repr(font.withHeight(24.0))

#==================================================================================================

def test_font_axis_api_does_not_throw():
    font = _load_any_font()
    numAxis = font.getNumAxis()
    assert numAxis >= 0

    for index in range(numAxis):
        description = font.getAxisDescription(index)
        assert description is not None
        assert description.tagName != ""
        assert description.minimumValue <= description.defaultValue <= description.maximumValue

        value = font.getAxisValue(index)
        font.setAxisValue(index, value)
        font.resetAxisValue(index)

    font.resetAllAxisValues()
