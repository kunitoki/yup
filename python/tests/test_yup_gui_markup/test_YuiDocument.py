import gc

import pytest

import yup

"""
YuiDocument: loading an interface from .yui text, looking its components up, the diagnostics of a
load, imports, actions, undo and reloading in place.

Nothing is shown, so no native window is needed: a handler is run by changing a value, or by
calling the onClick the markup connected, which is what the C++ tests do.

The file runs through the gui_app fixture, the Python side of the theme the C++ tests install:
widgets such as TextButton lay their text out with the theme font, and the global theme only exists
while an application is initialised. That theme is the default one rather than an empty one, so
only the values a document or a test sets are asserted, never what a property defaults to.

Not covered: YuiDocument.createSchema is not bound, because DataTreeSchema is not, and neither are
the document listeners, the source map and the builder.
"""

pytestmark = pytest.mark.usefixtures("gui_app")

SIMPLE = "Component { Slider { id: knob; } }"

PANEL = """
    Component {
        id: panel; width: 200; height: 100;
        Slider { id: knob; class: "knob big"; sliderType: rotaryVerticalDrag;
                 x: parent.width / 4; width: parent.width / 2; height: 50; value: 0.25; }
        Label { id: caption; text: "Volume"; }
    }"""


def load(text):
    document = yup.YuiDocument()
    result = document.loadFromString(text)

    assert result.wasOk(), result.getErrorMessage()

    return document


# ==============================================================================
# Loading
# ==============================================================================

def test_document_is_empty_until_something_is_loaded():
    document = yup.YuiDocument()

    assert document.getRoot() is None
    assert document.findById("knob") is None
    assert document.getStyleSheet() is None
    assert document.getDiagnostics() == []
    assert not document.getTree().isValid()


def test_load_from_string_builds_the_interface():
    document = load(PANEL)

    assert document.getDiagnostics() == []
    assert document.getTree().isValid()
    assert str(document.getTree().getType()) == "yui"

    root = document.getRoot()
    assert root is not None
    assert root.getComponentID() == "panel"
    assert root.getNumChildComponents() == 2
    assert not root.isOpaque()

    knob = document.findById("knob")
    assert knob is not None
    assert knob.hasParent()
    assert knob.hasStyleClass("knob")
    assert knob.hasStyleClass("big")
    assert knob.getX() == 50.0
    assert knob.getWidth() == 100.0
    assert knob.getProperty("sliderType") == "rotaryVerticalDrag"
    assert knob.getProperty("value") == 0.25

    caption = document.findById("caption")
    assert caption is not None
    assert caption.getProperty("text") == "Volume"


def test_components_are_found_by_id():
    document = load(PANEL)

    assert document.findById("knob") is not None
    assert document.findById("knob") is document.findById("knob")
    assert document.findById("knob") is not document.findById("caption")
    assert document.findById("missing") is None


def test_components_reach_python_as_their_own_class():
    document = load(PANEL)

    knob = document.findById("knob")
    assert isinstance(knob, yup.Slider)
    assert knob.getValue() == 0.25

    caption = document.findById("caption")
    assert isinstance(caption, yup.Label)
    assert caption.getText() == "Volume"

    assert type(document.getRoot()) is yup.Component


def test_components_stay_owned_by_the_document():
    document = load(SIMPLE)

    root = document.getRoot()
    knob = document.findById("knob")
    del root, knob
    gc.collect()

    # Dropping the Python objects must not destroy the components they borrowed.
    assert document.getRoot().getNumChildComponents() == 1
    assert document.findById("knob").getTypeName() == "Slider"


def test_components_keep_their_document_alive():
    knob = load(SIMPLE).findById("knob")
    gc.collect()

    assert knob.getTypeName() == "Slider"
    assert knob.hasParent()


def test_load_takes_a_parsed_document():
    parsed = yup.YuiParser.parse(SIMPLE)
    assert parsed.wasOk()

    document = yup.YuiDocument()
    result = document.load(parsed.document)

    assert result.wasOk(), result.getErrorMessage()
    assert document.findById("knob") is not None


def test_load_rejects_a_tree_that_is_not_a_document():
    document = yup.YuiDocument()

    assert document.load(yup.DataTree(yup.Identifier("other"))).failed()
    assert document.getRoot() is None


def test_clear_destroys_the_interface():
    document = load(SIMPLE)

    document.clear()

    assert document.getRoot() is None
    assert document.getDiagnostics() == []
    assert not document.getTree().isValid()


def test_base_directory_round_trip(tmp_path):
    document = yup.YuiDocument()
    document.setBaseDirectory(yup.File(str(tmp_path)))

    assert document.getBaseDirectory() == yup.File(str(tmp_path))


# ==============================================================================
# Styling
# ==============================================================================

def test_load_attaches_the_style_sheet_and_styles_the_interface():
    document = load("""
        style {
            .knob { trackWidth: 4 + 2; }
        }

        Component {
            Slider { id: knob; class: "knob"; }
        }""")

    assert document.getDiagnostics() == []

    sheet = document.getStyleSheet()
    assert sheet is not None
    assert document.getRoot().getStyleSheet() is sheet

    # A document is styled as soon as it is loaded, so the value the rule computes is there.
    assert document.findById("knob").findProperty("trackWidth") == 6.0


# ==============================================================================
# Type infos
# ==============================================================================

def test_find_type_info_returns_the_registered_types():
    document = load(SIMPLE)

    info = document.findTypeInfo("Slider")
    assert info is not None
    assert str(info.getTypeName()) == "Slider"
    assert info is document.findById("knob").getTypeInfo()

    assert document.findTypeInfo("Slidr") is None


def test_find_type_info_returns_the_types_the_document_declares():
    document = load("""
        component Caption : Label { text: "Composite"; }

        Column { Caption { id: caption; } }""")

    info = document.findTypeInfo("Caption")
    assert info is not None
    assert str(info.getTypeName()) == "Caption"
    assert info.isSubtypeOf("Label")

    # An instance is a Label carrying the type info of its declaration.
    caption = document.findById("caption")
    assert isinstance(caption, yup.Label)
    assert caption.getTypeName() == "Caption"
    assert caption.getTypeInfo() is info
    assert caption.getText() == "Composite"


# ==============================================================================
# Diagnostics
# ==============================================================================

def test_failed_load_keeps_the_interface_and_reports_the_error():
    document = load(SIMPLE)
    root = document.getRoot()

    result = document.loadFromString("Component {\n    Slider { id: ; }\n}")

    assert result.failed()
    assert document.getRoot() is root
    assert document.findById("knob") is not None

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) == 1
    assert diagnostics[0].line == 2
    assert diagnostics[0].isError()
    assert not diagnostics[0].isWarning()
    assert diagnostics[0].severity == yup.DataTreeDiagnostic.Severity.error
    assert result.getErrorMessage() == diagnostics[0].toString()


def test_unknown_component_type_fails_the_load():
    document = load(SIMPLE)
    knob = document.findById("knob")

    result = document.loadFromString("Component { Slidr { id: knob; } }")

    assert result.failed()
    assert result.getErrorMessage() == "1:13: Unknown component type 'Slidr'"
    assert len(document.getDiagnostics()) == 1
    assert document.findById("knob") is knob

    # The tree goes back to the loaded one, so fixing the text keeps the components.
    assert document.loadFromString(SIMPLE).wasOk()
    assert document.findById("knob") is knob


def test_values_that_cant_be_applied_are_reported_without_failing():
    document = yup.YuiDocument()
    result = document.loadFromString("Component { Slider { id: knob; value: missing * 2; } }")

    assert result.wasOk()
    assert document.findById("knob") is not None

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) == 1
    assert diagnostics[0].line == 1
    assert diagnostics[0].message.startswith("Slider.value: ")


def test_diagnostics_tell_where_the_problems_are_written():
    document = load("style {\n"
                    "    .knob { cursor: finger; }\n"
                    "}\n"
                    "Component {\n"
                    "    Slider { class: \"knob\"; value: missing * 2; }\n"
                    "}")

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) == 2, yup.DataTreeDiagnostic.join(diagnostics)

    assert diagnostics[0].toString().startswith("5:29: Slider.value: ")
    assert (diagnostics[0].line, diagnostics[0].column) == (5, 29)

    assert diagnostics[1].toString() == "2:13: Unknown cursor 'finger'"
    assert diagnostics[1].message == "Unknown cursor 'finger'"
    assert diagnostics[1].source == ""
    assert (diagnostics[1].line, diagnostics[1].column) == (2, 13)
    assert str(diagnostics[1]) == diagnostics[1].toString()

    joined = yup.DataTreeDiagnostic.join(diagnostics)
    assert joined == diagnostics[0].toString() + "\n" + diagnostics[1].toString()


def test_diagnostics_are_copies_of_the_last_load():
    document = yup.YuiDocument()
    assert document.loadFromString("Component { Slidr { } }").failed()

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) >= 1
    message = diagnostics[0].message

    # A later load replaces the diagnostics of the document, not the ones already handed out.
    assert document.loadFromString(SIMPLE).wasOk()
    assert document.getDiagnostics() == []
    assert diagnostics[0].message == message


# ==============================================================================
# Imports
# ==============================================================================

def test_imports_are_resolved_by_a_callable():
    requested = []

    def resolve_import(path):
        requested.append(path)

        if path == "parts.yui":
            return 'component Caption : Label { text: "Imported"; }'

        return None

    document = yup.YuiDocument()
    result = document.loadFromString("""
        import "parts.yui";

        Column { Caption { id: caption; } }""", resolve_import)

    assert result.wasOk(), result.getErrorMessage()
    assert requested == ["parts.yui"]

    assert document.findById("caption").getProperty("text") == "Imported"
    assert document.findTypeInfo("Caption") is not None


def test_import_fails_without_a_resolver():
    document = yup.YuiDocument()
    result = document.loadFromString("import \"missing.yui\";\nComponent { }")

    assert result.failed()

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) == 1
    assert diagnostics[0].toString() == "1:1: Can't import 'missing.yui'"


def test_import_fails_when_the_resolver_finds_nothing():
    document = yup.YuiDocument()
    result = document.loadFromString("import \"missing.yui\";\nComponent { }", lambda path: None)

    assert result.failed()
    assert result.getErrorMessage() == "1:1: Can't import 'missing.yui'"


def test_import_errors_name_the_imported_file():
    document = yup.YuiDocument()
    result = document.loadFromString("import \"broken.yui\";\nComponent { }", lambda path: "\n style {")

    assert result.failed()

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) == 1
    assert diagnostics[0].source == "broken.yui"
    assert diagnostics[0].toString() == "broken.yui:2:9: Expected '}'"


# ==============================================================================
# Actions
# ==============================================================================

ACTIONS = """
    Component {
        TextButton { id: go; onClick: action ("reset", knob.value * 2, "now"); }
        Slider { id: knob; value: 0.25; onValueChanged: action ("reset"); }
    }"""


def load_with_action(received):
    document = yup.YuiDocument()
    document.setAction("reset", lambda arguments: received.append(arguments))

    result = document.loadFromString(ACTIONS)
    assert result.wasOk(), result.getErrorMessage()
    assert document.getDiagnostics() == []

    # Applying the values of the markup may already have emitted a signal.
    received.clear()

    return document


def test_action_receives_the_arguments_of_the_signal():
    received = []
    document = load_with_action(received)

    document.findById("knob").setProperty("value", 0.75)

    assert len(received) == 1
    assert isinstance(received[0], list)
    assert received[0] == [pytest.approx(0.75)]


def test_action_receives_the_values_of_the_call():
    received = []
    document = load_with_action(received)

    go = document.findById("go")
    assert isinstance(go, yup.TextButton)
    assert go.onClick is not None

    go.onClick()

    assert len(received) == 1
    assert received[0] == [pytest.approx(0.5), "now"]


def test_action_can_be_set_after_loading():
    received = []

    document = load(ACTIONS)
    document.setAction("reset", lambda arguments: received.append(arguments))

    document.findById("knob").setProperty("value", 0.5)

    assert received == [[pytest.approx(0.5)]]


def test_action_is_removed_with_none():
    received = []
    document = load_with_action(received)

    document.setAction("reset", None)
    document.findById("knob").setProperty("value", 0.75)

    assert received == []


def test_action_error_reaches_the_caller():
    def failing_action(arguments):
        raise ValueError("no reset")

    document = load(ACTIONS)
    document.setAction("reset", failing_action)

    with pytest.raises(ValueError, match="no reset"):
        document.findById("knob").setProperty("value", 0.75)


# ==============================================================================
# Reloading
# ==============================================================================

def test_reloading_updates_the_interface_in_place():
    document = load('Component { Slider { id: knob; value: 0.25; } Label { id: caption; text: "a"; } }')

    root = document.getRoot()
    knob = document.findById("knob")
    knob.setProperty("value", 0.75)

    result = document.loadFromString('Component { Slider { id: knob; value: 0.25; } Label { id: caption; text: "b"; } }')
    assert result.wasOk(), result.getErrorMessage()

    # The components are kept, with what changed since: the value isn't written again.
    assert document.getRoot() is root
    assert document.findById("knob") is knob
    assert knob.getProperty("value") == 0.75
    assert document.findById("caption").getProperty("text") == "b"


def test_reloads_are_undoable():
    undo_manager = yup.UndoManager()

    document = yup.YuiDocument()
    document.setUndoManager(undo_manager)

    assert document.loadFromString('Component { Label { id: caption; text: "a"; } }').wasOk()
    caption = document.findById("caption")

    assert document.loadFromString('Component { Label { id: caption; text: "b"; } }').wasOk()
    assert document.loadFromString("Component { Missing { } }").failed()

    assert undo_manager.undo()
    assert document.findById("caption") is caption
    assert caption.getProperty("text") == "a"

    assert undo_manager.redo()
    assert caption.getProperty("text") == "b"

    del caption

    assert undo_manager.undo()
    assert undo_manager.undo()
    assert document.getRoot() is None

    document.setUndoManager(None)


def test_document_keeps_its_undo_manager_alive():
    document = yup.YuiDocument()
    document.setUndoManager(yup.UndoManager())
    gc.collect()

    assert document.loadFromString(SIMPLE).wasOk()
    assert document.loadFromString("Column { Slider { id: knob; } }").wasOk()
    assert document.findById("knob") is not None
