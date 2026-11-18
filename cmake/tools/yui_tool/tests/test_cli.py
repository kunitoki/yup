# Copyright (c) 2026 - kunitoki@gmail.com
# ISC license: https://www.isc.org/licenses/
"""Run manually: python3 test_cli.py /absolute/path/to/yup_yui_tool"""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zlib

BINARY = str(Path(sys.argv.pop(1)).resolve())

VALID = "Component { Slider { id: knob; } }\n"
SYNTAX_ERROR = "Component {\n    Slider { id: ; }\n}\n"
UNKNOWN_TYPE = "Component { Slidr { id: knob; } }\n"
UNDECLARED_PROPERTY = "Column {\n    Slider { id: a; mysterySetting: 123; }\n}\n"
STYLE_ONLY = "style {\n    .knob { trackWidth: 4; }\n}\n"
STYLE_ERROR = "style {\n    Slider::before { opacity: 1; }\n}\n"
SVG = '<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10"><rect width="10" height="10" fill="#ff0000"/></svg>\n'


class YuiToolCliTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
        return path

    def invoke(self, *args, stdin=None):
        return subprocess.run([BINARY, *args], cwd=self.root, input=stdin, capture_output=True, text=True, timeout=60)

    def diagnostics(self, *args, stdin=None, code=None):
        result = self.invoke(*args, "--json", stdin=stdin)
        if code is not None:
            self.assertEqual(result.returncode, code, result.stderr)
        return json.loads(result.stdout)["diagnostics"]

    # ==========================================================================
    # Usage

    def test_help_and_usage_errors(self):
        self.assertEqual(self.invoke("--help").returncode, 0)
        self.assertEqual(self.invoke().returncode, 2)
        self.assertEqual(self.invoke("format", "ui.yui").returncode, 2)
        self.assertEqual(self.invoke("verify").returncode, 2)
        self.assertEqual(self.invoke("verify", "--unknown", "ui.yui").returncode, 2)
        self.assertEqual(self.invoke("verify", "missing.yui").returncode, 2)
        self.assertEqual(self.invoke("lint", "--stdin").returncode, 2)
        self.assertEqual(self.invoke("schema", "extra").returncode, 2)
        self.assertEqual(self.invoke("import-svg").returncode, 2)
        self.assertEqual(self.invoke("import-svg", "missing.svg").returncode, 2)

    def test_strict_and_stdin_belong_to_lint(self):
        self.write("ui.yui", VALID)
        self.assertEqual(self.invoke("verify", "--strict", "ui.yui").returncode, 2)
        self.assertEqual(self.invoke("verify", "--stdin", "--path", "ui.yui", stdin=VALID).returncode, 2)

    # ==========================================================================
    # verify

    def test_verify_accepts_a_valid_document(self):
        self.write("ui.yui", VALID)
        result = self.invoke("verify", "ui.yui")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("ui.yui", result.stderr)
        self.assertEqual(self.diagnostics("verify", "ui.yui", code=0), [])

    def test_verify_reports_where_a_syntax_error_is(self):
        path = self.write("ui.yui", SYNTAX_ERROR)
        result = self.invoke("verify", "ui.yui")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertIn(f"{path}:2:", result.stderr)
        self.assertIn(": error: ", result.stderr)

    def test_verify_reports_an_unknown_component_type(self):
        path = self.write("ui.yui", UNKNOWN_TYPE)
        found = self.diagnostics("verify", "ui.yui", code=1)
        self.assertEqual(len(found), 1, found)
        self.assertEqual(found[0]["file"], str(path))
        self.assertEqual((found[0]["line"], found[0]["column"]), (1, 13))
        self.assertEqual(found[0]["severity"], "error")
        self.assertIn("Slidr", found[0]["message"])

    def test_verify_checks_every_file(self):
        self.write("good.yui", VALID)
        bad = self.write("bad.yui", UNKNOWN_TYPE)
        found = self.diagnostics("verify", "good.yui", "bad.yui", code=1)
        self.assertEqual([entry["file"] for entry in found], [str(bad)])

    def test_verify_ignores_warnings(self):
        self.write("ui.yui", UNDECLARED_PROPERTY)
        self.assertEqual(self.diagnostics("verify", "ui.yui", code=0), [])

    # ==========================================================================
    # Imports and files without a root component

    def test_imports_resolve_next_to_the_file(self):
        self.write("parts/caption.yui", 'component Caption : Label { text: "Imported"; }\n')
        self.write("ui.yui", 'import "parts/caption.yui";\n\nColumn { Caption { id: caption; } }\n')
        self.assertEqual(self.diagnostics("verify", "ui.yui", code=0), [])

    def test_a_missing_import_is_an_error(self):
        self.write("ui.yui", 'import "missing.yui";\n\nComponent { }\n')
        found = self.diagnostics("verify", "ui.yui", code=1)
        self.assertEqual(len(found), 1, found)
        self.assertIn("missing.yui", found[0]["message"])

    def test_an_error_in_an_import_is_reported_in_that_file(self):
        part = self.write("parts/style.yui", STYLE_ERROR)
        self.write("ui.yui", 'import "parts/style.yui";\n\nComponent { }\n')
        found = self.diagnostics("verify", "ui.yui", code=1)
        self.assertEqual(len(found), 1, found)
        self.assertEqual(found[0]["file"], str(part))
        self.assertEqual(found[0]["line"], 2)

    def test_a_file_of_declarations_is_valid_without_a_root(self):
        self.write("style.yui", STYLE_ONLY)
        self.write("empty.yui", "")
        self.assertEqual(self.diagnostics("verify", "style.yui", "empty.yui", code=0), [])

    def test_a_file_of_declarations_still_reports_its_errors(self):
        path = self.write("style.yui", STYLE_ERROR)
        found = self.diagnostics("verify", "style.yui", code=1)
        self.assertEqual(len(found), 1, found)
        self.assertEqual(found[0]["file"], str(path))
        self.assertEqual(found[0]["line"], 2)

    # ==========================================================================
    # Models

    def test_a_binding_to_an_undeclared_model_is_an_error(self):
        self.write("ui.yui", "Slider { id: gain; value <=> model.gain; }\n")
        found = self.diagnostics("verify", "ui.yui", code=1)
        self.assertEqual(len(found), 1, found)
        self.assertIn("model", found[0]["message"])

    def test_models_declare_what_a_document_binds_to(self):
        self.write("ui.yui", "Slider { id: gain; value <=> model.gain; }\n")
        self.write("models.json", json.dumps({"model": {"gain": 0.5}}))
        self.assertEqual(self.diagnostics("verify", "--models", "models.json", "ui.yui", code=0), [])
        self.assertEqual(self.diagnostics("lint", "--models", "models.json", "--stdin", "--path", "ui.yui",
                                          stdin="Slider { id: gain; value <=> model.gain; }\n", code=0), [])

    def test_a_property_missing_from_a_model_is_an_error(self):
        self.write("ui.yui", "Slider { id: gain; value <=> model.gain; }\n")
        self.write("models.json", json.dumps({"model": {"level": 0.5}}))
        found = self.diagnostics("verify", "--models", "models.json", "ui.yui", code=1)
        self.assertIn("gain", found[0]["message"])

    def test_an_unusable_models_file_is_a_usage_error(self):
        self.write("ui.yui", VALID)
        self.write("broken.json", "{ not json")
        self.write("list.json", "[1, 2]")
        self.write("flat.json", json.dumps({"model": 1}))
        for models in ("missing.json", "broken.json", "list.json", "flat.json"):
            with self.subTest(models=models):
                self.assertEqual(self.invoke("verify", "--models", models, "ui.yui").returncode, 2)
        self.assertEqual(self.invoke("verify", "ui.yui", "--models").returncode, 2)

    # ==========================================================================
    # lint

    def test_lint_reports_warnings_without_failing(self):
        path = self.write("ui.yui", UNDECLARED_PROPERTY)
        found = self.diagnostics("lint", "ui.yui", code=0)
        self.assertEqual(len(found), 1, found)
        self.assertEqual(found[0]["severity"], "warning")
        self.assertEqual(found[0]["file"], str(path))
        self.assertEqual(found[0]["line"], 2)
        self.assertIn("mysterySetting", found[0]["message"])

        result = self.invoke("lint", "ui.yui")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(": warning: ", result.stderr)

    def test_strict_lint_fails_on_warnings(self):
        self.write("ui.yui", UNDECLARED_PROPERTY)
        self.assertEqual(self.invoke("lint", "--strict", "ui.yui").returncode, 1)
        self.write("clean.yui", VALID)
        self.assertEqual(self.invoke("lint", "--strict", "clean.yui").returncode, 0)

    def test_lint_reads_unsaved_text_from_stdin(self):
        path = self.write("ui.yui", VALID)
        found = self.diagnostics("lint", "--stdin", "--path", "ui.yui", stdin=UNKNOWN_TYPE, code=1)
        self.assertEqual(len(found), 1, found)
        self.assertEqual(found[0]["file"], str(path))
        self.assertEqual((found[0]["line"], found[0]["column"]), (1, 13))

        self.write("ui.yui", UNKNOWN_TYPE)
        self.assertEqual(self.diagnostics("lint", "--stdin", "--path", "ui.yui", stdin=VALID, code=0), [])

    def test_stdin_lints_a_file_that_is_not_saved_yet(self):
        self.write("parts/caption.yui", 'component Caption : Label { text: "Imported"; }\n')
        text = 'import "parts/caption.yui";\n\nColumn { Caption { id: caption; } }\n'
        self.assertEqual(self.diagnostics("lint", "--stdin", "--path", "unsaved.yui", stdin=text, code=0), [])
        self.assertEqual(self.diagnostics("lint", "--stdin", "--path", "unsaved.yui", stdin=STYLE_ONLY, code=0), [])

    # ==========================================================================
    # schema

    def test_schema_describes_the_registered_types(self):
        result = self.invoke("schema")
        self.assertEqual(result.returncode, 0, result.stderr)
        schema = json.loads(result.stdout)
        self.assertEqual(schema["version"], 1)

        types = {entry["name"]: entry for entry in schema["types"]}
        self.assertNotIn("base", types["Component"])

        slider = types["Slider"]
        self.assertTrue(slider["instantiable"])

        base = slider
        while "base" in base:
            base = types[base["base"]]
        self.assertEqual(base["name"], "Component")

        properties = {entry["name"]: entry for entry in slider["properties"]}
        self.assertEqual(properties["value"]["type"], "number")
        self.assertFalse(properties["value"]["readOnly"])
        self.assertTrue(properties["dragging"]["state"])
        self.assertTrue(properties["dragging"]["readOnly"])
        self.assertEqual(properties["sliderType"]["type"], "enumeration")
        self.assertGreater(len(properties["sliderType"]["values"]), 1)

        signals = {entry["name"]: entry for entry in slider["signals"]}
        self.assertEqual([argument["name"] for argument in signals["onValueChanged"]["arguments"]], ["value"])

        self.assertFalse(types["DocumentWindow"]["instantiable"])

    def test_schema_lists_the_palette_roles(self):
        schema = json.loads(self.invoke("schema").stdout)
        self.assertIn("accent", schema["paletteRoles"])
        self.assertEqual(len(schema["paletteRoles"]), 8)

    def test_schema_writes_to_a_file(self):
        result = self.invoke("schema", "--output", "schema.json")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "")
        self.assertEqual(json.loads((self.root / "schema.json").read_text())["version"], 1)

    # ==========================================================================
    # import-svg

    def test_import_svg_writes_a_drawing_that_verifies(self):
        self.write("logo.svg", SVG)
        result = self.invoke("import-svg", "logo.svg", "--name", "Logo", "--output", "logo.yui")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("drawing Logo", (self.root / "logo.yui").read_text())
        self.assertEqual(self.diagnostics("verify", "logo.yui", code=0), [])

    def test_import_svg_prints_to_stdout_by_default(self):
        self.write("logo.svg", SVG)
        result = self.invoke("import-svg", "logo.svg")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("drawing Imported", result.stdout)

    def test_import_svg_declares_color_properties(self):
        self.write("logo.svg", SVG)
        result = self.invoke("import-svg", "logo.svg", "--color-properties")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("color1", result.stdout)

    def test_import_svg_rejects_what_is_not_an_svg(self):
        self.write("notes.svg", "not an svg\n")
        result = self.invoke("import-svg", "notes.svg")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        self.assertIn(": error: ", result.stderr)


    # ==========================================================================
    # render

    def render(self, *args, code=0):
        """Renders into out.png and returns its size in pixels, skipping where nothing can draw."""
        result = self.invoke("render", *args, "--output", "out.png")
        if result.returncode == 3:
            self.skipTest("no graphics device to render with")
        self.assertEqual(result.returncode, code, result.stderr)
        if code != 0:
            self.assertFalse((self.root / "out.png").exists())
            return result
        data = (self.root / "out.png").read_bytes()
        self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
        return tuple(int.from_bytes(data[offset:offset + 4], "big") for offset in (16, 20))

    def first_pixel(self):
        """Returns the top left pixel of out.png, which no filter changes as it has no neighbour."""
        data = (self.root / "out.png").read_bytes()
        channels = {2: 3, 6: 4}[data[25]]
        position, compressed = 8, b""
        while position < len(data):
            length = int.from_bytes(data[position:position + 4], "big")
            if data[position + 4:position + 8] == b"IDAT":
                compressed += data[position + 8:position + 8 + length]
            position += 12 + length
        return tuple(zlib.decompress(compressed)[1:1 + channels][:3])

    def test_render_fills_behind_the_interface_with_the_palette_of_the_root(self):
        self.write("ui.yui", "palette Signal {\n    colors: #ff0000, #00ff00;\n    background: #ff0000;\n}\n\n"
                             "style {\n    #panel { palette: Signal; }\n}\n\nComponent { id: panel; }\n")
        self.render("ui.yui", "--size", "8x8", "--scale", "1")
        self.assertEqual(self.first_pixel(), (255, 0, 0))

        self.write("plain.yui", "Component { }\n")
        self.render("plain.yui", "--size", "8x8", "--scale", "1", "--theme", "light")
        light = self.first_pixel()
        self.render("plain.yui", "--size", "8x8", "--scale", "1", "--theme", "dark")
        self.assertGreater(sum(light), sum(self.first_pixel()))

    def test_render_draws_the_root_at_the_size_and_scale_asked(self):
        self.write("ui.yui", 'Column { padding: 8; TextButton { text: "OK"; preferredHeight: 28; } }\n')
        self.assertEqual(self.render("ui.yui"), (1280, 800))
        self.assertEqual(self.render("ui.yui", "--size", "120x60"), (240, 120))
        self.assertEqual(self.render("ui.yui", "--size", "120x60", "--scale", "1", "--theme", "light"), (120, 60))

    def test_render_uses_the_size_the_root_asks_for(self):
        self.write("ui.yui", "Window { width: 320; height: 200; }\n")
        self.assertEqual(self.render("ui.yui", "--scale", "1"), (320, 200))
        self.assertEqual(self.render("ui.yui", "--scale", "1", "--default-size", "100x80"), (320, 200))
        self.assertEqual(self.render("ui.yui", "--scale", "1", "--size", "100x80"), (100, 80))

    def test_render_falls_back_to_the_default_size_given(self):
        self.write("ui.yui", "Column { }\n")
        self.assertEqual(self.render("ui.yui", "--scale", "1", "--default-size", "100x80"), (100, 80))
        self.assertEqual(self.render("ui.yui", "--scale", "1", "--default-size", "100x80", "--size", "50x40"), (50, 40))
        self.assertEqual(self.invoke("render", "ui.yui", "--output", "out.png", "--default-size", "0x10").returncode, 2)

    def test_render_previews_what_a_file_of_declarations_declares(self):
        self.write("dot.yui", "drawing Dot {\n    circle { center: bounds.center; radius: 20; fill: #ff0000; }\n}\n")
        self.write("colors.yui", "palette Ember {\n    colors: #1b1b1e, #2b2b31, #ff8800, #f4f4f6;\n}\n")
        self.assertEqual(self.render("dot.yui", "--scale", "1"), (200, 200))
        self.assertEqual(self.render("dot.yui", "--drawing", "Dot", "--as", "Slider", "--scale", "1"), (200, 200))
        self.assertEqual(self.render("colors.yui", "--scale", "1"), (360, 300))
        self.assertEqual(self.render("colors.yui", "--palette", "Ember", "--scale", "1"), (360, 300))

    def test_render_previews_a_declaration_of_a_file_with_a_root(self):
        self.write("ui.yui", 'component Caption : Label { text: "Imported"; }\n\nColumn { Caption { id: caption; } }\n')
        self.assertEqual(self.render("ui.yui", "--as", "Caption", "--scale", "1"), (200, 200))

    def test_render_reports_what_cant_be_drawn(self):
        self.write("broken.yui", UNKNOWN_TYPE)
        self.write("empty.yui", "")
        self.write("style.yui", STYLE_ONLY)
        self.write("dot.yui", "drawing Dot {\n    circle { center: bounds.center; radius: 20; fill: #ff0000; }\n}\n")

        result = self.render("broken.yui", code=1)
        self.assertIn(":1:13: error: ", result.stderr)

        self.assertIn("nothing to render", self.render("empty.yui", code=1).stderr)
        self.assertIn("nothing to render", self.render("style.yui", code=1).stderr)
        self.assertIn("Missing", self.render("dot.yui", "--drawing", "Missing", code=1).stderr)
        self.assertIn("Missing", self.render("dot.yui", "--palette", "Missing", code=1).stderr)

    def test_render_usage_errors(self):
        self.write("ui.yui", VALID)
        self.assertEqual(self.invoke("render", "ui.yui").returncode, 2)
        self.assertEqual(self.invoke("render", "--output", "out.png").returncode, 2)
        self.assertEqual(self.invoke("render", "missing.yui", "--output", "out.png").returncode, 2)
        self.assertEqual(self.invoke("render", "ui.yui", "--output", "out.png", "--size", "big").returncode, 2)
        self.assertEqual(self.invoke("render", "ui.yui", "--output", "out.png", "--scale", "0").returncode, 2)
        self.assertEqual(self.invoke("render", "ui.yui", "--output", "out.png", "--theme", "sepia").returncode, 2)
        self.assertEqual(self.invoke("render", "ui.yui", "--output", "out.png", "--drawing", "A", "--palette", "B").returncode, 2)
        self.assertEqual(self.invoke("render", "ui.yui", "--output", "out.png", "--as", "}").returncode, 2)


if __name__ == "__main__":
    unittest.main()
