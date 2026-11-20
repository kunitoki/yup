import gc

import pytest

import yup

"""
YuiLoader: loading a document from a file with its imports, from a string standing for the main
file and from embedded resources, overriding files with texts held in memory, and reloading.

The files are written to pytest's tmp_path. Reloading when a watched file changes is not exercised,
since it needs the file system watcher and a running message loop: only the switch is.

The file runs through the gui_app fixture, for the reason test_YuiDocument.py gives.
"""

pytestmark = pytest.mark.usefixtures("gui_app")

MAIN = """
    import "parts/style.yui";

    Component {
        Slider { id: knob; class: "knob"; }
    }"""

MAIN_WITH_TITLE = """
    import "parts/style.yui";

    Component {
        Slider { id: knob; class: "knob"; }
        Label { id: title; }
    }"""

RESOURCES = {
    "ui.yui": b"""
        import "style.yui";

        Component {
            Slider { id: knob; class: "knob"; }
        }""",
    "style.yui": b"style { .knob { trackWidth: 3; } }",
}


def write(folder, path, text):
    file = folder.joinpath(*path.split("/"))
    file.parent.mkdir(parents=True, exist_ok=True)
    file.write_bytes(text.encode("utf-8"))


def write_style(folder, width):
    write(folder, "parts/style.yui", "style { .knob { trackWidth: %d; } }" % width)


def file_in(folder, path):
    return yup.File(str(folder.joinpath(*path.split("/"))))


def track_width(document):
    knob = document.findById("knob")

    return knob.findProperty("trackWidth") if knob is not None else -1.0


@pytest.fixture
def folder(tmp_path):
    write_style(tmp_path, 7)
    write(tmp_path, "ui.yui", MAIN)

    return tmp_path


# ==============================================================================
# Files
# ==============================================================================

def test_load_file_resolves_the_imports_relative_to_the_file(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    result = loader.loadFile(file_in(folder, "ui.yui"))

    assert result.wasOk(), result.getErrorMessage()
    assert loader.getFile() == file_in(folder, "ui.yui")
    assert document.getBaseDirectory() == yup.File(str(folder))
    assert track_width(document) == 7.0


def test_load_file_fails_when_the_file_cant_be_read(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    result = loader.loadFile(file_in(folder, "missing.yui"))

    assert result.failed()
    assert "missing.yui" in result.getErrorMessage()
    assert document.getRoot() is None


def test_loaded_text_is_returned_for_each_path(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()
    assert loader.getLoadedText("ui.yui") == file_in(folder, "ui.yui").loadFileAsString()
    assert loader.getLoadedText("parts/style.yui") == file_in(folder, "parts/style.yui").loadFileAsString()
    assert loader.getLoadedText("knob.yui") is None


# ==============================================================================
# Reloading
# ==============================================================================

def test_on_load_is_not_set_by_default():
    loader = yup.YuiLoader(yup.YuiDocument())

    assert loader.onLoad is None


def test_reload_updates_the_interface_in_place_and_reports_each_load(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    loads = []
    loader.onLoad = lambda result: loads.append(result.wasOk())

    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()
    knob = document.findById("knob")

    write_style(folder, 9)
    result = loader.reload()

    assert result.wasOk(), result.getErrorMessage()
    assert document.findById("knob") is knob
    assert track_width(document) == 9.0
    assert loads == [True, True]


def test_reload_keeps_the_interface_when_it_fails(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()
    root = document.getRoot()

    loads = []
    loader.onLoad = lambda result: loads.append(result.wasOk())

    write(folder, "parts/style.yui", "style { .knob { ")

    assert loader.reload().failed()
    assert loads == [False]
    assert document.getRoot() is root
    assert track_width(document) == 7.0

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) >= 1
    assert diagnostics[0].source == "parts/style.yui"


def test_on_load_can_be_cleared(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    loads = []
    loader.onLoad = lambda result: loads.append(result.wasOk())
    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()

    loader.onLoad = None
    assert loader.reload().wasOk()

    assert loads == [True]


# ==============================================================================
# Strings
# ==============================================================================

def test_reload_from_string_imports_from_the_file(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()

    result = loader.reloadFromString(MAIN_WITH_TITLE)

    assert result.wasOk(), result.getErrorMessage()
    assert document.findById("title") is not None
    assert track_width(document) == 7.0

    # The string stands for the file until another source is loaded.
    write_style(folder, 5)
    assert loader.reload().wasOk()
    assert document.findById("title") is not None
    assert track_width(document) == 5.0

    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()
    assert document.findById("title") is None


def test_reload_from_string_keeps_the_interface_when_the_text_is_invalid(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()
    root = document.getRoot()

    loads = []
    loader.onLoad = lambda result: loads.append(result.wasOk())

    result = loader.reloadFromString("Component {\n    Slider { id: ; }\n}")

    assert result.failed()
    assert loads == [False]
    assert document.getRoot() is root
    assert document.findById("knob") is not None
    assert track_width(document) == 7.0

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) == 1
    assert diagnostics[0].line == 2
    assert result.getErrorMessage() == diagnostics[0].toString()

    # The invalid text stays the main text until it is cleared, then the file is read again.
    assert loader.reload().failed()

    loader.clearOverrides()
    assert loader.reload().wasOk()
    assert document.findById("knob") is not None
    assert loads == [False, False, True]


def test_reload_from_string_needs_no_source():
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    assert loader.reload().failed()

    text = "Component { Slider { id: knob; } }"
    assert loader.reloadFromString(text).wasOk()
    assert document.findById("knob") is not None
    assert loader.getLoadedText("") == text

    # There is nothing to import from, and the interface loaded is kept.
    assert loader.reloadFromString('import "style.yui"; Component {}').failed()
    assert document.findById("knob") is not None


# ==============================================================================
# Overrides
# ==============================================================================

def test_overrides_replace_files_until_another_source_is_loaded(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()

    # Paths are relative to the folder of the main file, which is its own name.
    loader.setOverride("parts/style.yui", "style { .knob { trackWidth: 2; } }")
    loader.setOverride("ui.yui", MAIN_WITH_TITLE)

    assert loader.reload().wasOk()
    assert document.findById("title") is not None
    assert track_width(document) == 2.0

    # An overridden file isn't read from disk.
    write_style(folder, 5)
    assert loader.reload().wasOk()
    assert track_width(document) == 2.0

    loader.clearOverrides()
    assert loader.reload().wasOk()
    assert document.findById("title") is None
    assert track_width(document) == 5.0

    loader.setOverride("parts/style.yui", "style { .knob { trackWidth: 2; } }")
    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()
    assert track_width(document) == 5.0


def test_override_is_the_loaded_text_once_a_load_used_it(folder):
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    assert loader.loadFile(file_in(folder, "ui.yui")).wasOk()

    loader.setOverride("parts/style.yui", "style {}")
    assert loader.getLoadedText("parts/style.yui") == file_in(folder, "parts/style.yui").loadFileAsString()

    assert loader.reload().wasOk()
    assert loader.getLoadedText("parts/style.yui") == "style {}"


# ==============================================================================
# Resources
# ==============================================================================

def test_load_resource_finds_the_file_and_its_imports_with_a_callable():
    requested = []

    def find_resource(path):
        requested.append(path)

        return RESOURCES.get(path)

    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    result = loader.loadResource("ui.yui", find_resource)

    assert result.wasOk(), result.getErrorMessage()
    assert requested == ["ui.yui", "style.yui"]
    assert loader.getFile() == yup.File()
    assert track_width(document) == 3.0
    assert loader.getLoadedText("style.yui") == "style { .knob { trackWidth: 3; } }"
    assert loader.getLoadedText("parts/style.yui") is None

    assert loader.loadResource("missing.yui", find_resource).failed()
    assert track_width(document) == 3.0


def test_load_resource_keeps_its_finder_for_the_reloads():
    document = yup.YuiDocument()
    loader = yup.YuiLoader(document)

    # The finder is only referenced by the loader, which calls it again on each reload.
    assert loader.loadResource("ui.yui", lambda path: RESOURCES.get(path)).wasOk()
    gc.collect()

    assert loader.reload().wasOk()
    assert track_width(document) == 3.0

    loader.setOverride("style.yui", "style { .knob { trackWidth: 12; } }")
    assert loader.reload().wasOk()
    assert track_width(document) == 12.0


# ==============================================================================
# Lifetime and watching
# ==============================================================================

def test_loader_keeps_its_document_alive():
    loader = yup.YuiLoader(yup.YuiDocument())
    gc.collect()

    assert loader.reloadFromString("Component { Slider { id: knob; } }").wasOk()
    assert loader.getLoadedText("") == "Component { Slider { id: knob; } }"


def test_loader_is_not_watching_by_default():
    loader = yup.YuiLoader(yup.YuiDocument())

    assert loader.isWatching() is False

    loader.setWatching(False)
    assert loader.isWatching() is False


def test_watching_can_be_switched_off():
    loader = yup.YuiLoader(yup.YuiDocument())

    # Whether watching starts depends on the platform, stopping it always leaves it off.
    loader.setWatching(True)
    loader.setWatching(False)

    assert loader.isWatching() is False
