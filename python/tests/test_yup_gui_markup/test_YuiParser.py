import yup

"""
YuiParser, YuiWriter and the DataTreeDiagnostic a failed parse reports.

Parsing only checks the syntax and builds a DataTree, so nothing here needs an application.

Not covered: the source map of a ParseResult is not bound. YuiDocument uses it to give its own
diagnostics their line and column, which test_YuiDocument.py covers.
"""

ROOT = """
    Component {
        id: panel; width: 200;
        Slider { id: knob; class: "knob big"; x: parent.width / 4; }
    }"""

COMPONENTS = """
    tokens {
        property spacing: number = 8;
        property title: string;
    }
    component LabeledKnob : Column {
        property label: string = "Gain";
        property alias value <=> knob.value;
        signal moved;
        signal changed (value: number, final: boolean);
        gap: spacing;
        Slider { id: knob; }
        Label { text: label; }
    }
    Column {
        Slider { id: gain; value <=> model.gain; onValueChanged: (v) => action ("gain", v); changed pressed: { action ("a"); action ("b"); } }
        Label { text: gain.pressed ? "pressed" : "idle"; }
    }"""

FILES = {
    "palette.yui": 'palette P { colors: "#100b00-#85cb33"; }',
    "style.yui": 'import "palette.yui"; style { Slider { palette: P; } }',
    "broken.yui": "\n style {",
}


def resolve_import(path):
    return FILES.get(path)


# ==============================================================================
# Parsing
# ==============================================================================

def test_parse_builds_a_document_tree():
    result = yup.YuiParser.parse(ROOT)

    assert result.wasOk()
    assert result.diagnostics == []
    assert str(result.document.getType()) == "yui"

    root = result.document.getChildWithName("Component")
    assert root.isValid()
    assert str(root.getProperty("width")) == "200"

    # Values are kept as they are written, for the code consuming the tree to evaluate.
    knob = root.getChildWithName("Slider")
    assert str(knob.getProperty("class")) == '"knob big"'
    assert str(knob.getProperty("x")) == "parent.width / 4"


def test_parse_accepts_an_empty_text():
    result = yup.YuiParser.parse(" // nothing here\n")

    assert result.wasOk()
    assert result.document.getNumChildren() == 0


def test_parse_returns_no_document_on_errors():
    result = yup.YuiParser.parse("drawing { rect { bounds: ; } }")

    assert not result.wasOk()
    assert not result.document.isValid()
    assert len(result.diagnostics) == 1


def test_parse_keeps_the_imports_without_a_resolver():
    result = yup.YuiParser.parse('import "style.yui"; Component { }')

    assert result.wasOk()
    assert str(result.document.getChild(0).getType()) == "import"


# ==============================================================================
# Imports
# ==============================================================================

def test_parse_merges_the_imported_files():
    result = yup.YuiParser.parse('import "style.yui"; import "palette.yui"; Component { }', resolve_import)

    assert result.wasOk(), yup.DataTreeDiagnostic.join(result.diagnostics)

    # Each file is imported once, the first time it is met, and no import element is left.
    document = result.document
    assert document.getNumChildren() == 3
    assert [str(document.getChild(index).getType()) for index in range(3)] == ["palette", "style", "Component"]


def test_parse_reports_the_imports_it_cant_resolve():
    result = yup.YuiParser.parse('import "missing.yui";', resolve_import)

    assert not result.wasOk()
    assert result.diagnostics[0].toString() == "1:1: Can't import 'missing.yui'"


def test_parse_fails_to_import_without_a_resolver():
    result = yup.YuiParser.parse('import "missing.yui";', None)

    assert not result.wasOk()
    assert result.diagnostics[0].toString() == "1:1: Can't import 'missing.yui'"


def test_parse_reports_the_errors_of_an_imported_file_with_its_path():
    result = yup.YuiParser.parse('import "broken.yui";', resolve_import)

    assert not result.wasOk()

    diagnostic = result.diagnostics[0]
    assert diagnostic.source == "broken.yui"
    assert (diagnostic.line, diagnostic.column) == (2, 9)
    assert diagnostic.toString() == "broken.yui:2:9: Expected '}'"


# ==============================================================================
# Diagnostics
# ==============================================================================

def test_diagnostic_tells_where_the_error_is():
    result = yup.YuiParser.parse("drawing Knob {\n    circle { radius: 4 }\n}")

    assert not result.wasOk()

    diagnostic = result.diagnostics[0]
    assert diagnostic.message == "Expected ';'"
    assert diagnostic.source == ""
    assert (diagnostic.line, diagnostic.column) == (2, 24)
    assert diagnostic.severity == yup.DataTreeDiagnostic.Severity.error
    assert diagnostic.isError()
    assert not diagnostic.isWarning()


def test_diagnostic_is_printed_with_its_position():
    diagnostic = yup.YuiParser.parse("drawing Knob {\n    circle { radius: 4 }\n}").diagnostics[0]

    assert diagnostic.toString() == "2:24: Expected ';'"
    assert str(diagnostic) == "2:24: Expected ';'"
    assert repr(diagnostic).startswith("<yup.DataTreeDiagnostic")
    assert "2:24: Expected ';'" in repr(diagnostic)


def test_diagnostics_are_joined():
    first = yup.YuiParser.parse("drawing {").diagnostics[0]
    second = yup.YuiParser.parse("drawing { let r 4; }").diagnostics[0]

    assert first.toString() == "1:10: Expected '}'"
    assert second.toString() == "1:17: Expected ':'"

    assert yup.DataTreeDiagnostic.join([first, second]) == "1:10: Expected '}'\n1:17: Expected ':'"
    assert yup.DataTreeDiagnostic.join([first, second], "; ") == "1:10: Expected '}'; 1:17: Expected ':'"
    assert yup.DataTreeDiagnostic.join([]) == ""


# ==============================================================================
# Writing
# ==============================================================================

def test_write_uses_the_canonical_layout():
    document = yup.YuiParser.parse("drawing Icon { drawing Leaf {} use Leaf { bounds: bounds; } }").document

    assert yup.YuiWriter.write(document) == ("drawing Icon {\n"
                                              "    drawing Leaf {}\n"
                                              "    use Leaf {\n"
                                              "        bounds: bounds;\n"
                                              "    }\n"
                                              "}\n")


def test_write_round_trips_a_document():
    parsed = yup.YuiParser.parse(COMPONENTS)
    assert parsed.wasOk(), yup.DataTreeDiagnostic.join(parsed.diagnostics)

    document = parsed.document
    written = yup.YuiWriter.write(document)

    reparsed = yup.YuiParser.parse(written)

    assert reparsed.wasOk(), yup.DataTreeDiagnostic.join(reparsed.diagnostics)
    assert reparsed.document.isEquivalentTo(document)
    assert yup.YuiWriter.write(reparsed.document) == written


def test_write_takes_a_single_element():
    document = yup.YuiParser.parse('Row { Label { text: "Hi"; } }').document
    assert document.getNumChildren() == 1

    label = document.getChild(0).getChild(0)

    assert yup.YuiWriter.write(label) == 'Label {\n    text: "Hi";\n}\n'


def test_write_of_an_empty_document_is_empty():
    assert yup.YuiWriter.write(yup.DataTree(yup.Identifier("yui"))) == ""
