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

#==================================================================================================

def _variable_font_file() -> yup.File:
    from utilities import get_test_data_file

    file = get_test_data_file("fonts/Linefont-VariableFont_wdth,wght.ttf")
    if not file.existsAsFile():
        pytest.skip("tests/data/fonts/Linefont-VariableFont_wdth,wght.ttf is not available")

    return file

def test_variable_font_axis_descriptions():
    font = yup.Font.loadFontFromFile(_variable_font_file())
    assert font.getNumAxis() == 2

    axes = [font.getAxisDescription(index) for index in range(font.getNumAxis())]
    assert all(repr(axis).startswith("Font.Axis('") for axis in axes)
    assert sorted(axis.tagName for axis in axes) == ["wdth", "wght"]
    assert font.getAxisDescription(99) is None

def test_variable_font_axis_values_in_bulk():
    font = yup.Font.loadFontFromFile(_variable_font_file())
    wght = next(font.getAxisDescription(index) for index in range(font.getNumAxis())
                if font.getAxisDescription(index).tagName == "wght")

    font.setAxisValues([yup.Font.AxisOption("wght", wght.maximumValue)])
    assert abs(font.getAxisValue("wght") - wght.maximumValue) < 1e-3

    varied = font.withAxisValues([yup.Font.AxisOption("wght", wght.minimumValue)])
    assert abs(varied.getAxisValue("wght") - wght.minimumValue) < 1e-3
    assert abs(font.getAxisValue("wght") - wght.maximumValue) < 1e-3

def test_font_features():
    font = yup.Font.loadFontFromFile(_variable_font_file())

    featured = font.withFeatures([yup.Font.Feature("liga", 0), yup.Font.Feature(0x6b65726e, 1)])
    assert not featured.isEmpty()

    with pytest.raises(ValueError):
        yup.Font.Feature("abc", 1)

def test_font_loading_from_data_and_failures():
    file = _variable_font_file()
    missing = file.getSiblingFile("missing-font.ttf").getFullPathName()

    with open(file.getFullPathName(), "rb") as stream:
        assert yup.Font.loadFontFromData(stream.read()).getNumAxis() == 2

    with pytest.raises(ValueError):
        yup.Font.loadFontFromData(b"definitely not a font file")

    with pytest.raises(ValueError):
        yup.Font.loadFontFromFile(yup.File(missing))

    assert yup.Font.loadFontFromFirstAvailableFile([missing, file.getFullPathName()]).getNumAxis() == 2

    with pytest.raises(ValueError):
        yup.Font.loadFontFromFirstAvailableFile([missing])

@pytest.mark.parametrize("loader", ["loadSerifSystemTextFont", "loadMonospaceSystemTextFont", "loadColorEmojiSystemFont"])
def test_system_font_loaders(loader):
    # System fonts depend on the machine: either a font comes back or ValueError is raised.
    try:
        font = getattr(yup.Font, loader)()
    except ValueError:
        pytest.skip(f"{loader} found no font on this machine")

    assert not font.isEmpty()
