import pytest

import yup

"""
StyleSheet, and the Component side of styling: style classes and attaching a sheet to a tree.

Sheets are attached to hand-written trees here, which only records them: a stylesheet attached
with Component.setStyleSheet is matched against the tree when the window refreshes, or with
StyleContext.restyleNow, and StyleContext is not bound. The values a sheet computes are covered by
test_YuiDocument.py, since a document styles its interface as soon as it is loaded.

The file runs through the gui_app fixture, as the C++ tests of the styling install a theme.

Not covered: StyleSheet.findPalette is not bound, because ThemePalette is not.
"""

# The parser of yup_gui_markup is what makes a sheet from text
if not hasattr(yup, "YuiParser"):
    pytest.skip(allow_module_level=True)

pytestmark = pytest.mark.usefixtures("gui_app")

SHEET = """
    palette Ember { colors: "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f"; accent: #ff9a1f; }
    drawing Knob { circle { center: bounds.center; radius: 4; fill: thumbColor; } }
    style {
        .synth { palette: Ember; Slider.thumbColor: palette.accent; }
        Slider { drawing: Knob; }
    }"""


def style_classes(component):
    return [str(style_class) for style_class in component.getStyleClasses()]


# ==============================================================================
# Parsing
# ==============================================================================

def test_parse_builds_a_sheet_from_text():
    sheet = yup.YuiParser.parseStyleSheet(SHEET)

    assert isinstance(sheet, yup.StyleSheet)

    assert sheet.getPaletteNames().size() == 1
    assert sheet.getPaletteNames().contains("Ember")

    assert sheet.findDrawing("Knob").isValid()
    assert not sheet.findDrawing("Missing").isValid()


def test_parse_reads_the_properties_its_selectors_watch():
    sheet = yup.YuiParser.parseStyleSheet("""
        style {
            TextButton,
            ComboBox:open {
                cursor: hand;
            }
            Slider, #special {
                opacity: 0.4;
            }
            .override {
                opacity: 0.8;
            }
        }""")

    # A selector reading a property restyles when it changes, a declaration setting one doesn't.
    assert sheet.watchesProperty("open")
    assert not sheet.watchesProperty("opacity")


def test_parse_failure_raises_with_the_syntax_error():
    with pytest.raises(ValueError) as error:
        yup.YuiParser.parseStyleSheet("drawing {")

    assert str(error.value) == "1:10: Expected '}'"


def test_parse_failure_raises_with_the_style_error():
    with pytest.raises(ValueError) as error:
        yup.YuiParser.parseStyleSheet("style {\n  Slider::before { opacity: 1; }\n}")

    assert "Pseudo-elements are not supported in selector 'Slider::before'" in str(error.value)


# ==============================================================================
# Style classes
# ==============================================================================

def test_component_has_no_style_classes_by_default():
    component = yup.Component()

    assert component.getStyleClasses() == []
    assert not component.hasStyleClass("panel")


def test_style_classes_are_added_once_and_keep_their_order():
    component = yup.Component()
    component.addStyleClass("panel")
    component.addStyleClass("accent")
    component.addStyleClass("panel")

    assert component.hasStyleClass("panel")
    assert component.hasStyleClass(yup.Identifier("accent"))
    assert style_classes(component) == ["panel", "accent"]


def test_style_classes_are_removed():
    component = yup.Component()
    component.addStyleClass("panel")
    component.addStyleClass("accent")

    component.removeStyleClass("panel")
    component.removeStyleClass("missing")

    assert not component.hasStyleClass("panel")
    assert style_classes(component) == ["accent"]


def test_style_classes_are_replaced():
    component = yup.Component()
    component.addStyleClass("panel")

    component.setStyleClasses(["knob", yup.Identifier("big")])
    assert style_classes(component) == ["knob", "big"]

    component.setStyleClasses([])
    assert component.getStyleClasses() == []


# ==============================================================================
# Attaching a sheet
# ==============================================================================

def test_component_has_no_sheet_by_default():
    assert yup.Component().getStyleSheet() is None


def test_sheet_is_attached_and_detached():
    sheet = yup.YuiParser.parseStyleSheet(SHEET)
    root = yup.Component()

    root.setStyleSheet(sheet)
    assert root.getStyleSheet() is sheet

    root.setStyleSheet(None)
    assert root.getStyleSheet() is None


def test_sheet_is_attached_to_the_root_only():
    root = yup.Component()
    child = yup.Slider(yup.SliderType.LinearHorizontal)
    root.addChildComponent(child)

    root.setStyleSheet(yup.YuiParser.parseStyleSheet(SHEET))

    # The sheet styles everything inside the root, but is only attached to it.
    assert root.getStyleSheet() is not None
    assert child.getStyleSheet() is None


def test_component_keeps_its_sheet_alive():
    root = yup.Component()

    # The sheet is reference counted, so it outlives the Python object the call was made with.
    root.setStyleSheet(yup.YuiParser.parseStyleSheet(SHEET))

    assert root.getStyleSheet().getPaletteNames().contains("Ember")


def test_sheet_styles_several_trees():
    sheet = yup.YuiParser.parseStyleSheet(SHEET)
    first = yup.Component()
    second = yup.Component()

    first.setStyleSheet(sheet)
    second.setStyleSheet(sheet)

    assert first.getStyleSheet() is sheet
    assert second.getStyleSheet() is sheet
