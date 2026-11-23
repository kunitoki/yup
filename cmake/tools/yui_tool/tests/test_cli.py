# Copyright (c) 2026 - kunitoki@gmail.com
# ISC license: https://www.isc.org/licenses/
"""Run manually: python3 test_cli.py /absolute/path/to/yup_yui_tool"""

import base64
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
NO_HEIGHT = 'Column {\n    Row { id: bar;\n        Label { text: "Lost"; }\n    }\n}\n'
HOVER = ("palette Signal {\n    colors: #ff0000, #00ff00;\n    background: #ff0000;\n}\n\n"
         "style {\n    #panel:hover { palette: Signal; }\n}\n\nComponent { id: panel; }\n")
HALVES = ("drawing Fill {\n    property tone: color = #ff0000;\n    rect { bounds: bounds; fill: tone; }\n}\n\n"
          "style {\n    #left { drawing: Fill; }\n    #right { drawing: Fill; tone: TONE; }\n}\n\n"
          "Component { width: 40; height: 20;\n"
          "    Component { id: left; x: 0; y: 0; width: 20; height: 20; }\n"
          "    Component { id: right; x: 20; y: 0; width: 20; height: 20; }\n}\n")
INTERACTIVE = ('Column { width: 200; height: 120;\n'
               '    ToggleButton { id: power; preferredHeight: 30; checked <=> model.power; }\n'
               '    Slider { id: gain; preferredHeight: 30; value <=> model.gain; }\n'
               '    TextButton { id: save; preferredHeight: 30; text: "Save"; onClick: action ("save", model.gain, "now"); }\n'
               '}\n')
REPEATED = ('Column { width: 200; height: 120;\n'
            '    ToggleButton { id: wide; preferredHeight: 30; checked <=> seq.wide; }\n'
            '    Row { id: steps; preferredHeight: 30;\n'
            '        repeat i in 0 .. (seq.wide ? 4 : 2) { ToggleButton { checked <=> seq.steps[i]; onClick: action ("step", i); } }\n'
            '    }\n'
            '}\n')
COVERED = ('Component { width: 100; height: 100;\n'
           '    TextButton { id: under; x: 0; y: 0; width: 100; height: 100; onClick: action ("never"); }\n'
           '    Component { id: cover; x: 0; y: 0; width: 100; height: 100; }\n'
           '    TextButton { id: off; x: 0; y: 0; width: 100; height: 100; enabled: false; onClick: action ("never"); }\n'
           '}\n')
DRAWN = ('drawing Meter {\n'
         '    rect { id: track; bounds: rect (2, 4, 20, 10); fill: #333333; }\n'
         '    group { transform: transform.translation (10, 20);\n'
         '        circle { id: dot; center: point (5, 5); radius: 3; fill: #00ff00; }\n    }\n'
         '    repeat i in 0 .. 3 { rect { id: tick; bounds: rect (i * 10, 40, 4, 4); fill: #ffffff; } }\n'
         '    text { id: caption; bounds: rect (0, 0, 60, 20); text: "Hi"; fill: #ffffff; justification: topLeft; }\n'
         '    rect { bounds: bounds; fill: none; stroke: #ffffff; }\n'
         '}\n\n'
         'style {\n    #meter { drawing: Meter; }\n}\n\n'
         'Component { width: 100; height: 80;\n    Component { id: meter; x: 20; y: 10; width: 60; height: 50; }\n}\n')
USED = ('palette Signal {\n    colors: #ff0000, #00ff00;\n    background: #ff0000;\n}\n\n'
        'drawing Dial {\n    rect { bounds: bounds; fill: #0000ff; }\n'
        '    circle { center: bounds.center; radius: 2 + 4 * normalized; fill: #ffffff; }\n}\n\n'
        'drawing Spare {\n    viewBox: rect (0, 0, 48, 24);\n    rect { bounds: bounds; fill: palette.background; }\n}\n\n'
        'component Caption : Label { text: "Imported"; }\n'
        'component Unused : Label { text: "Unused"; }\n\n'
        'style {\n    .dial { drawing: Dial; }\n}\n\n'
        'Component { width: 100; height: 80;\n'
        '    Slider { id: first; class: dial; x: 10; y: 20; width: 30; height: 30; paintMargin: 4; }\n'
        '    Slider { id: second; class: dial; x: 60; y: 20; width: 20; height: 20; }\n'
        '    Caption { id: caption; x: 10; y: 60; width: 60; height: 16; }\n'
        '}\n')
# A document as it is while it is typed: its last component is not closed, so it does not load
TYPED = ('import "parts.yui";\n\n'
         'tokens { gap: 8; }\n\n'
         'palette Ember { mode: dark; colors: #1b1b1e, #ff8800; glow: #ffcc00; }\n\n'
         'drawing Knob {\n'
         '    property ring: color = palette.glow;\n'
         '    drawing Cap { circle { center: bounds.center; radius: 4; } }\n'
         '    use Cap { bounds: bounds; }\n'
         '    circle {\n        ra\n    }\n'
         '}\n\n'
         'component Dial : Slider {\n    property caption: string = "Dial";\n    signal turned (amount);\n}\n\n'
         'style {\n    #add, .knob { drawing: Knob; ring: #abc; }\n    .bar { drawing: Bar; }\n}\n\n'
         'Column {\n    id: panel;\n'
         '    Dial { id: cutoff; sliderType: rotaryVerticalDrag; value <=> params.cutoff; }\n'
         '    Dial {\n        id: other;\n        cap\n    }\n'
         '    Slider { wid\n')
PARTS = 'drawing Bar { rect { bounds: bounds; fill: #ff0000; } }\ncomponent Toolbar : Row { }\n'
NEEDLE = "drawing Needle {\n    circle { center: bounds.center; radius: 4 * normalized; fill: #ff0000; }\n}\n"
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
        self.assertEqual(self.invoke("--version").returncode, 0)
        self.assertTrue(self.invoke("--version").stdout.startswith("yup_yui_tool "))
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

    def test_schema_lists_the_named_colors(self):
        schema = json.loads(self.invoke("schema").stdout)
        self.assertIn("transparentBlack", schema["namedColors"])
        self.assertIn("aliceblue", schema["namedColors"])

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

    def test_render_previews_a_declaration_where_the_interface_uses_it(self):
        self.write("ui.yui", USED)

        # The first component the drawing paints, with its paint margin, and the first one of the type
        self.assertEqual(self.render("ui.yui", "--drawing", "Dial", "--scale", "1"), (38, 38))
        self.assertEqual(self.render("ui.yui", "--as", "Caption", "--scale", "1"), (60, 16))

    def test_render_previews_a_declaration_the_interface_does_not_use_alone(self):
        self.write("ui.yui", USED)

        # A drawing is given the size of its view box and the first palette of the file
        self.assertEqual(self.render("ui.yui", "--drawing", "Spare", "--scale", "1"), (48, 24))
        self.assertEqual(self.first_pixel(), (255, 0, 0))

        self.assertEqual(self.render("ui.yui", "--as", "Unused", "--scale", "1"), (200, 200))
        self.assertEqual(self.render("ui.yui", "--drawing", "Dial", "--as", "Slider", "--scale", "1"), (200, 200))

    def test_render_draws_an_element_or_a_region(self):
        self.write("ui.yui", USED)
        self.assertEqual(self.render("ui.yui", "--element", "second", "--margin", "0", "--scale", "1"), (20, 20))
        self.assertEqual(self.first_pixel(), (0, 0, 255))
        self.assertEqual(self.render("ui.yui", "--element", "second", "--scale", "1"), (36, 36))
        self.assertEqual(self.render("ui.yui", "--drawing", "Dial", "--element", "1", "--margin", "0", "--scale", "1"), (20, 20))
        self.assertEqual(self.render("ui.yui", "--region", "90,70,50,40", "--scale", "1"), (10, 10))

    def test_render_draws_guides_over_the_components_named(self):
        self.write("ui.yui", USED)
        self.assertEqual(self.render("ui.yui", "--scale", "1"), (100, 80))
        plain = (self.root / "out.png").read_bytes()

        self.assertEqual(self.render("ui.yui", "--guides", "first,1", "--scale", "1"), (100, 80))
        self.assertNotEqual((self.root / "out.png").read_bytes(), plain)

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

        self.write("used.yui", USED)
        self.assertIn("nobody", self.render("used.yui", "--element", "nobody", code=1).stderr)
        self.assertIn("nobody", self.render("used.yui", "--guides", "first,nobody", code=1).stderr)
        self.assertIn("nothing of the interface", self.render("used.yui", "--region", "500,500,10,10", code=1).stderr)

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
        self.assertEqual(self.invoke("render", "ui.yui", "--output", "out.png", "--region", "1,2,3").returncode, 2)
        self.assertEqual(self.invoke("render", "ui.yui", "--output", "out.png", "--margin", "wide").returncode, 2)
        self.assertEqual(self.invoke("render", "ui.yui", "--output", "out.png", "--element", "knob", "--region", "0,0,4,4").returncode, 2)

    # ==========================================================================
    # inspect

    def inspect(self, *args, code=0):
        """Returns the layout inspect prints, or what it ran into when it is expected to fail."""
        result = self.invoke("inspect", *args)
        self.assertEqual(result.returncode, code, result.stderr)
        return json.loads(result.stdout) if code == 0 else result

    def test_inspect_describes_the_components_with_their_place(self):
        path = self.write("ui.yui", 'Column {\n    padding: 10;\n    Label { id: title; text: "Hello"; preferredHeight: 20; }\n'
                                    '    Slider { flexGrow: 1; }\n}\n')
        layout = self.inspect("ui.yui", "--size", "200x100")
        self.assertEqual(layout["file"], str(path))
        self.assertEqual((layout["width"], layout["height"]), (200, 100))
        self.assertEqual(layout["warnings"], [])

        root = layout["root"]
        self.assertEqual((root["type"], root["path"], root["line"], root["bounds"]), ("Column", "", 1, [0, 0, 200, 100]))
        self.assertNotIn("id", root)

        title, slider = root["children"]
        self.assertEqual((title["type"], title["id"], title["path"], title["line"], title["text"]), ("Label", "title", "0", 3, "Hello"))
        self.assertEqual((slider["type"], slider["path"], slider["line"]), ("Slider", "1", 4))
        self.assertEqual(title["bounds"][3], 20)
        self.assertGreaterEqual(slider["bounds"][1], title["bounds"][1] + 20)
        self.assertGreater(slider["bounds"][3], 20)

    def test_inspect_warns_about_what_has_no_size(self):
        self.write("ui.yui", NO_HEIGHT)
        layout = self.inspect("ui.yui", "--size", "200x100")
        self.assertEqual(len(layout["warnings"]), 1, layout["warnings"])
        self.assertIn("Row #bar has no ", layout["warnings"][0]["message"])
        self.assertEqual((layout["warnings"][0]["path"], layout["warnings"][0]["line"]), ("0", 2))

    def test_inspect_lays_out_what_a_file_declares(self):
        self.write("dot.yui", "drawing Dot {\n    circle { center: bounds.center; radius: 20; fill: #ff0000; }\n}\n")
        layout = self.inspect("dot.yui", "--drawing", "Dot", "--as", "Slider")
        self.assertEqual((layout["root"]["type"], layout["width"], layout["height"]), ("Slider", 200, 200))

    def test_inspect_describes_a_declaration_where_the_interface_uses_it(self):
        self.write("ui.yui", USED)

        first, second, caption = self.inspect("ui.yui")["root"]["children"]
        self.assertEqual(first["paintBounds"], [6, 16, 38, 38])
        self.assertNotIn("paintBounds", second)

        used = self.inspect("ui.yui", "--drawing", "Dial")
        self.assertEqual((used["root"]["type"], used["root"]["id"], used["root"]["path"], used["root"]["bounds"]), ("Slider", "first", "0", [10, 20, 30, 30]))
        self.assertEqual((used["width"], used["height"]), (100, 80))
        self.assertEqual(self.inspect("ui.yui", "--as", "Caption")["root"]["id"], "caption")

        # A component is described alone when named, another use of the drawing as well
        self.assertEqual(self.inspect("ui.yui", "--element", "second")["root"]["bounds"], [60, 20, 20, 20])
        self.assertEqual(self.inspect("ui.yui", "--drawing", "Dial", "--element", "1")["root"]["id"], "second")
        self.assertIn("nobody", self.inspect("ui.yui", "--element", "nobody", code=1).stderr)

        # What the interface does not use stands alone in place of it
        self.assertEqual((self.inspect("ui.yui", "--drawing", "Spare")["width"], self.inspect("ui.yui", "--as", "Unused")["width"]), (48, 200))

    def test_inspect_tells_when_a_drawing_needs_the_widget_it_is_made_for(self):
        self.write("needle.yui", NEEDLE)

        alone = self.invoke("inspect", "needle.yui")
        self.assertEqual(alone.returncode, 0, alone.stderr)
        self.assertIn("Unknown symbol 'normalized'", alone.stderr)
        self.assertIn("hint: ", alone.stderr)

        hosted = self.invoke("inspect", "needle.yui", "--as", "Slider")
        self.assertEqual(hosted.returncode, 0, hosted.stderr)
        self.assertNotIn("hint: ", hosted.stderr)

    def test_inspect_reports_what_cant_be_laid_out(self):
        self.write("broken.yui", UNKNOWN_TYPE)
        self.write("style.yui", STYLE_ONLY)
        result = self.inspect("broken.yui", code=1)
        self.assertIn(":1:13: error: ", result.stderr)
        self.assertEqual(result.stdout, "")
        self.assertIn("nothing to inspect", self.inspect("style.yui", code=1).stderr)

    def test_inspect_usage_errors(self):
        self.write("ui.yui", VALID)
        self.assertEqual(self.invoke("inspect").returncode, 2)
        self.assertEqual(self.invoke("inspect", "missing.yui").returncode, 2)
        self.assertEqual(self.invoke("inspect", "ui.yui", "--size", "big").returncode, 2)
        self.assertEqual(self.invoke("inspect", "ui.yui", "--scale", "2").returncode, 2)
        self.assertEqual(self.invoke("inspect", "ui.yui", "--drawing", "A", "--palette", "B").returncode, 2)

    # ==========================================================================
    # outline, complete, definition, hover

    def ask(self, command, *args, code=0):
        """Returns what a command of an editor answers."""
        result = self.invoke(command, *args)
        self.assertEqual(result.returncode, code, result.stderr)
        return json.loads(result.stdout) if code == 0 else result

    @staticmethod
    def place(text, marker, inside=0):
        """Returns the place right after a marker of a text, or that many characters before its end, as <line>:<column>."""
        index = text.index(marker) + len(marker) - inside
        return f"{text.count(chr(10), 0, index) + 1}:{index - text.rfind(chr(10), 0, index)}"

    def labels(self, marker, inside=0):
        found = self.ask("complete", "ui.yui", "--at", self.place(TYPED, marker, inside))
        return {item["label"]: item for item in found["items"]}

    def test_outline_tells_what_a_file_that_does_not_load_is_made_of(self):
        self.write("ui.yui", TYPED)
        parts = self.write("parts.yui", PARTS)
        self.assertEqual(self.invoke("verify", "ui.yui").returncode, 1)

        outline = self.ask("outline", "ui.yui")
        symbols = outline["symbols"]
        self.assertEqual([(symbol["name"], symbol["kind"]) for symbol in symbols],
                         [("tokens", "tokens"), ("Ember", "palette"), ("Knob", "drawing"), ("Dial", "component"), ("style", "style"), ("Column #panel", "element")])
        self.assertEqual([child["name"] for child in symbols[2]["children"]], ["ring"])
        self.assertEqual([child["name"] for child in symbols[4]["children"]], ["#add, .knob", ".bar"])
        self.assertEqual([child["name"] for child in symbols[5]["children"]], ["Dial #cutoff", "Dial #other", "Slider"])
        self.assertEqual((symbols[2]["range"][:2], symbols[2]["selection"]), ([7, 1], [7, 1, 7, 13]))

        self.assertEqual(outline["imports"], [{"path": "parts.yui", "range": [1, 9, 1, 18], "file": str(parts)}])

        # The colors are those written as values: the id of a selector is not one
        lines = TYPED.split("\n")
        written = [lines[color["range"][0] - 1][color["range"][1] - 1:color["range"][3] - 1] for color in outline["colors"]]
        self.assertEqual(written, ["#1b1b1e", "#ff8800", "#ffcc00", "#abc"])
        self.assertEqual([round(part * 255) for part in outline["colors"][3]["color"]], [0xaa, 0xbb, 0xcc, 255])

        # What it can show is its interface, what it declares and what the files it imports declare
        self.assertEqual([target["id"] for target in outline["targets"]],
                         ["root", "drawing:Knob", "palette:Ember", "component:Dial", "drawing:Bar", "component:Toolbar"])
        self.assertEqual(outline["targets"][4], {"id": "drawing:Bar", "label": "Drawing: Bar (parts.yui)", "drawing": "Bar", "palette": "", "component": ""})
        self.assertEqual(outline["hosts"][0], "Component")
        self.assertTrue({"Slider", "Dial", "Toolbar"} <= set(outline["hosts"]))

    def test_complete_tells_what_can_be_written_at_a_place(self):
        self.write("ui.yui", TYPED)
        self.write("parts.yui", PARTS)
        self.write("ui.models.json", '{ "params": { "cutoff": 0.5, "name": "Init" } }')

        # Where a statement starts: the properties and signals of the component, and the types it can hold
        statements = self.labels("Slider { wid")
        self.assertEqual((statements["width"]["kind"], statements["onValueChanged"]["kind"], statements["Dial"]["kind"]), ("property", "signal", "type"))
        self.assertIn("Toolbar", statements)

        # A component declared adds its own to those of the type it is made from
        declared = self.labels("        cap")
        self.assertEqual((declared["caption"]["detail"], declared["turned"]["kind"]), ("string", "signal"))
        self.assertIn("value", declared)

        # In a drawing: what an element holds and takes, and the drawings it can use
        shape = self.labels("        ra")
        self.assertTrue({"radius", "center", "fill"} <= set(shape))
        self.assertNotIn("width", shape)
        self.assertTrue({"Cap", "Knob", "Bar"} <= set(self.labels("    use ")))

        # After a dot: the colors of the palettes, and the properties of a model
        self.assertTrue({"accent", "glow"} <= set(self.labels("= palette.")))
        self.assertEqual(sorted(self.labels("<=> params.")), ["cutoff", "name"])

        # The value of a property: the drawings declared, or what an enumeration takes
        self.assertTrue({"Knob", "Bar", "none"} <= set(self.labels(".knob { drawing: ")))
        self.assertIn("rotaryVerticalDrag", self.labels("sliderType: "))

        # At the top of a file: what it is made of
        self.assertTrue({"import", "style", "Slider"} <= set(self.labels('import "parts.yui";\n')))

    def test_definition_tells_where_what_is_named_is_declared(self):
        ui = self.write("ui.yui", TYPED)
        parts = self.write("parts.yui", PARTS)

        def definition(marker, inside=1):
            return self.ask("definition", "ui.yui", "--at", self.place(TYPED, marker, inside))

        self.assertEqual(definition(".knob { drawing: Knob"), {"file": str(ui), "line": 7, "column": 9})
        self.assertEqual(definition(".bar { drawing: Bar"), {"file": str(parts), "line": 1, "column": 9})
        self.assertEqual(definition('import "parts'), {"file": str(parts), "line": 1, "column": 1})
        self.assertEqual(definition("    use Cap"), {"file": str(ui), "line": 9, "column": 13})
        self.assertEqual(definition("= palette.glow"), {"file": str(ui), "line": 5, "column": 55})
        self.assertEqual(definition("    Dial { id: cutoff", inside=14), {"file": str(ui), "line": 16, "column": 11})
        self.assertEqual(definition("id: panel"), {})

    def test_hover_tells_what_a_property_is(self):
        self.write("ui.yui", TYPED)
        self.write("parts.yui", PARTS)

        told = self.ask("hover", "ui.yui", "--at", self.place(TYPED, "id: cutoff; sliderType", 1))
        self.assertEqual(told["code"], "sliderType: enumeration")
        self.assertIn("rotaryVerticalDrag", told["values"])
        self.assertEqual(told["range"], [28, 24, 28, 34])

        # Only what a statement starts with is one
        self.assertEqual(self.ask("hover", "ui.yui", "--at", self.place(TYPED, "id: cutoff", 1)), {})

    def test_editor_commands_usage_errors(self):
        self.write("ui.yui", VALID)
        self.assertEqual(self.invoke("outline").returncode, 2)
        self.assertEqual(self.invoke("outline", "missing.yui").returncode, 2)
        self.assertEqual(self.invoke("complete", "ui.yui").returncode, 2)
        self.assertEqual(self.invoke("complete", "ui.yui", "--at", "3").returncode, 2)
        self.assertEqual(self.invoke("definition", "ui.yui", "--at", "0:1").returncode, 2)
        self.assertEqual(self.invoke("hover", "ui.yui", "--unknown").returncode, 2)
        # A place past the end of the file is its end
        self.assertIn("import", {item["label"] for item in self.ask("complete", "ui.yui", "--at", "99:99")["items"]})

    # ==========================================================================
    # mcp

    def mcp(self, *requests):
        """Sends requests to the server, each a method with its params, and returns the result of each."""
        messages = [{"jsonrpc": "2.0", "id": 0, "method": "initialize",
                     "params": {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "test", "version": "1"}}},
                    {"jsonrpc": "2.0", "method": "notifications/initialized"}]
        messages += [{"jsonrpc": "2.0", "id": index + 1, "method": method, "params": params}
                     for index, (method, params) in enumerate(requests)]

        result = self.invoke("mcp", stdin="".join(json.dumps(message) + "\n" for message in messages))
        self.assertEqual(result.returncode, 0, result.stderr)

        # The output holds the answers and nothing else, one per line
        answers = {answer["id"]: answer for answer in map(json.loads, result.stdout.splitlines())}
        self.assertEqual(answers[0]["result"]["serverInfo"]["name"], "yup_yui_tool")
        return [answers[index + 1]["result"] for index in range(len(requests))]

    def call(self, name, **arguments):
        """Calls a tool of the server: returns whether it failed, its text, and its image when it gives one."""
        result, = self.mcp(("tools/call", {"name": name, "arguments": arguments}))
        image = next((base64.b64decode(block["data"]) for block in result["content"] if block["type"] == "image"), None)
        return result.get("isError", False), result["content"][0]["text"], image

    def test_mcp_lists_the_tools(self):
        listed, = self.mcp(("tools/list", {}))
        tools = {tool["name"]: tool for tool in listed["tools"]}
        self.assertEqual(sorted(tools), ["yui_check", "yui_inspect", "yui_render", "yui_schema"])
        self.assertEqual(tools["yui_check"]["inputSchema"]["required"], ["file"])
        self.assertIn("states", tools["yui_render"]["inputSchema"]["properties"])
        self.assertEqual(self.invoke("mcp", "extra").returncode, 2)

    def test_mcp_checks_a_file(self):
        path = self.write("ui.yui", UNKNOWN_TYPE)
        failed, text, image = self.call("yui_check", file="ui.yui")
        found = json.loads(text)
        self.assertFalse(failed)
        self.assertIsNone(image)
        self.assertEqual((found["file"], found["errors"], found["warnings"]), (str(path), 1, 0))
        self.assertEqual((found["problems"][0]["line"], found["problems"][0]["column"]), (1, 13))

    def test_mcp_uses_the_models_next_to_a_file(self):
        self.write("ui.yui", "Slider { id: gain; value <=> model.gain; }\n")
        self.assertEqual(json.loads(self.call("yui_check", file="ui.yui")[1])["errors"], 1)

        self.write("ui.models.json", json.dumps({"model": {"gain": 0.5}}))
        self.assertEqual(json.loads(self.call("yui_check", file="ui.yui")[1])["errors"], 0)
        self.assertFalse(self.call("yui_inspect", file="ui.yui", values={"model": {"gain": 0.9}})[0])
        self.assertTrue(self.call("yui_inspect", file="ui.yui", values={"model": {"level": 0.9}})[0])
        self.assertTrue(self.call("yui_check", file="ui.yui", models="missing.json")[0])

    def test_mcp_lists_the_models_a_file_lacks(self):
        self.write("ui.yui", 'component Knob : Slider {\n    property label: string = "x";\n    value <=> params.inner;\n}\n\n'
                             'Column {\n'
                             '    Slider { id: gain; value <=> params.cutoff; enabled: params.power && other.flag; }\n'
                             '    Label { text: "Level " + seq.name; opacity: seq.level * 0.5; }\n'
                             '    ToggleButton { checked <=> seq.on; onClick: action ("go", params.cutoff); }\n'
                             '    Knob { label: params.title; }\n'
                             '    Label { text: gain.valu; visible: seq.steps[2]; }\n'
                             '}\n')

        # With no models at all, every binding is told with a sample value of the type it feeds
        found = json.loads(self.call("yui_check", file="ui.yui")[1])
        self.assertEqual(found["missingModels"], {"params": {"cutoff": 0, "power": False, "inner": 0, "title": ""},
                                                  "other": {"flag": False},
                                                  "seq": {"name": "", "level": 0, "on": False, "steps": [False]}})

        # What is left once they are added is not theirs to fix: here a property a component does not have
        self.assertEqual(found["errorsWithMissingModels"], 1)

        # What the models file declares is left out, and its values are kept
        self.write("ui.models.json", json.dumps({"params": {"cutoff": 0.5, "power": True}, "seq": {"name": "A", "level": 1, "on": True, "steps": [True]}}))
        found = json.loads(self.call("yui_check", file="ui.yui")[1])
        self.assertEqual(found["missingModels"], {"params": {"inner": 0, "title": ""}, "other": {"flag": False}})

        # A file lacking nothing says nothing of it
        self.write("plain.yui", VALID)
        self.assertNotIn("missingModels", json.loads(self.call("yui_check", file="plain.yui")[1]))

    def test_mcp_inspects_the_layout(self):
        self.write("ui.yui", NO_HEIGHT)
        failed, text, image = self.call("yui_inspect", file="ui.yui", size="200x100")
        layout = json.loads(text)
        self.assertFalse(failed)
        self.assertIsNone(image)
        self.assertEqual(layout["root"]["children"][0]["id"], "bar")
        self.assertEqual([warning["path"] for warning in layout["warnings"]], ["0"])
        self.assertEqual(layout["problems"], [])

    def test_mcp_inspects_the_elements_of_drawings(self):
        self.write("ui.yui", DRAWN)

        meter = json.loads(self.call("yui_inspect", file="ui.yui")[1])["root"]["children"][0]
        self.assertNotIn("drawing", meter)

        # The elements with an id, where they land in the interface
        meter = json.loads(self.call("yui_inspect", file="ui.yui", drawings=True)[1])["root"]["children"][0]
        elements = {element["id"]: element for element in meter["drawing"]}
        self.assertEqual(sorted(elements), ["caption", "dot", "tick", "track"])
        self.assertEqual((elements["track"]["type"], elements["track"]["bounds"], elements["track"]["line"]), ("rect", [22, 14, 20, 10], 2))
        self.assertEqual((elements["dot"]["bounds"], elements["dot"]["line"]), ([32, 32, 6, 6], 4))
        self.assertNotIn("rows", elements["dot"])

        # A component is described alone when asked, at the path it has in the interface
        alone = json.loads(self.call("yui_inspect", file="ui.yui", element="meter", drawings=True)[1])
        self.assertEqual((alone["root"]["id"], alone["root"]["path"], alone["root"]["bounds"]), ("meter", "0", [20, 10, 60, 50]))
        self.assertEqual(len(alone["root"]["drawing"]), 4)
        self.assertTrue(self.call("yui_inspect", file="ui.yui", element="nobody")[0])

        # What is repeated covers all its copies, and a text tells where its glyphs are
        self.assertEqual((elements["tick"]["bounds"], elements["tick"]["rows"]), ([20, 50, 24, 4], 3))
        self.assertEqual(elements["caption"]["bounds"], [20, 10, 60, 20])
        self.assertEqual(elements["caption"]["text"][:2], [20, 10])
        self.assertTrue(0 < elements["caption"]["text"][2] < 60)
        self.assertNotIn("text", elements["track"])

    def test_mcp_shows_a_declaration_where_the_interface_uses_it(self):
        self.write("ui.yui", USED)
        self.write("needle.yui", NEEDLE)

        used = json.loads(self.call("yui_inspect", file="ui.yui", drawing="Dial")[1])
        self.assertEqual(used["use"], {"type": "Slider", "id": "first", "path": "0"})
        self.assertEqual((used["root"]["id"], used["problems"]), ("first", []))
        self.assertEqual(json.loads(self.call("yui_inspect", file="ui.yui", drawing="Dial", element="second")[1])["root"]["id"], "second")
        self.assertNotIn("use", json.loads(self.call("yui_inspect", file="ui.yui", drawing="Spare")[1]))

        # A drawing made for a widget says so when it is shown on a plain component
        self.assertIn("hint", json.loads(self.call("yui_inspect", file="needle.yui")[1]))
        self.assertNotIn("hint", json.loads(self.call("yui_inspect", file="needle.yui", **{"as": "Slider"})[1]))

        failed, text, _ = self.call("yui_render", file="ui.yui", drawing="Dial")
        if failed and "no graphics device" in text:
            self.skipTest("no graphics device to render with")
        self.assertFalse(failed, text)

        drawn = json.loads(text)
        self.assertEqual((drawn["width"], drawn["height"], drawn["region"], drawn["use"]["id"]), (38, 38, [6, 16, 38, 38], "first"))

        # Guides are drawn over the components named
        plain = self.call("yui_render", file="ui.yui")[2]
        failed, _, guided = self.call("yui_render", file="ui.yui", guides=["first", "1"])
        self.assertFalse(failed)
        self.assertNotEqual(guided, plain)
        self.assertTrue(self.call("yui_render", file="ui.yui", guides=["nobody"])[0])
        self.assertTrue(self.call("yui_render", file="ui.yui", guides="first")[0])

    def test_mcp_tells_why_a_tool_cant_answer(self):
        self.write("broken.yui", UNKNOWN_TYPE)
        self.write("ui.yui", VALID)

        failed, text, _ = self.call("yui_inspect", file="broken.yui")
        self.assertTrue(failed)
        self.assertIn(":1:13: error: ", text)

        self.assertTrue(self.call("yui_inspect", file="missing.yui")[0])
        self.assertTrue(self.call("yui_inspect", file="ui.yui", size="big")[0])
        self.assertTrue(self.call("yui_inspect", file="ui.yui", theme="sepia")[0])
        self.assertTrue(self.call("yui_inspect", file="ui.yui", states=[{"id": "nobody", "state": "hover"}])[0])
        self.assertTrue(self.call("yui_inspect", file="ui.yui", states=[{"id": "knob", "state": "focus"}])[0])
        self.assertTrue(self.call("yui_inspect", file="ui.yui", states=[{"path": "7", "state": "hover"}])[0])
        self.assertFalse(self.call("yui_inspect", file="ui.yui", states=[{"id": "knob", "state": "hover"}, {"path": "0", "state": "pressed"}])[0])
        self.assertTrue(self.call("yui_capture_preview")[0])
        self.assertTrue(self.call("yui_bogus")[0])

    def test_mcp_describes_what_a_file_can_name(self):
        described = json.loads(self.call("yui_schema")[1])
        self.assertIn("Slider", [entry["name"] for entry in described["types"]])
        self.assertIn("accent", described["paletteRoles"])

        # A type is described by what it declares itself, with the bases declaring the rest
        slider = json.loads(self.call("yui_schema", type="Slider")[1])
        properties = {entry["name"]: entry for entry in slider["properties"]}
        self.assertIn("value", properties)
        self.assertNotIn("visible", properties)
        self.assertEqual(slider["bases"], ["Component"])

        slider = json.loads(self.call("yui_schema", type="Slider", inherited=True)[1])
        properties = {entry["name"]: entry for entry in slider["properties"]}
        self.assertNotIn("from", properties["value"])
        self.assertEqual(properties["visible"]["from"], "Component")
        self.assertTrue(self.call("yui_schema", type="Slidr")[0])

    def test_mcp_describes_drawings_styles_and_expressions(self):
        self.assertEqual(json.loads(self.call("yui_schema")[1])["topics"], ["components", "drawing", "styles", "expressions"])
        self.assertIn("types", json.loads(self.call("yui_schema", topic="components")[1]))

        drawing = json.loads(self.call("yui_schema", topic="drawing")[1])
        elements = {(entry["type"], entry.get("geometry", [""])[0]): entry for entry in drawing["elements"]}
        self.assertEqual(elements[("rect", "bounds")]["role"], "shape")
        self.assertIn("cornerRadius", elements[("rect", "bounds")]["properties"])
        self.assertIn(("text", "anchor"), elements)
        self.assertIn("rect", elements[("group", "")]["holds"])
        self.assertNotIn("holds", elements[("rect", "bounds")])
        self.assertIn("deviation", elements[("blur", "")]["properties"])
        self.assertIn("blur", elements[("filter", "")]["holds"])
        self.assertIn("fill", drawing["shapePaint"])

        styles = json.loads(self.call("yui_schema", topic="styles")[1])
        self.assertIn("transition", styles["declarations"])
        self.assertIn("hover", styles["pseudoClasses"])
        self.assertIn("accent", styles["paletteRoles"])

        expressions = json.loads(self.call("yui_schema", topic="expressions")[1])
        self.assertTrue(any(entry.startswith("clamp (") for entry in expressions["functions"]))
        self.assertTrue(any(entry.startswith("withAlpha (") for entry in expressions["members"]["color"]))
        self.assertFalse(any(entry.startswith(("operator", "@", "_", ".")) for entry in expressions["functions"]))
        self.assertTrue(expressions["enumerations"])

        # Each topic names the page explaining what it lists
        self.assertTrue(all(topic["docs"].endswith(".md") for topic in (drawing, styles, expressions)))
        self.assertTrue(self.call("yui_schema", topic="colors")[0])
        self.assertTrue(self.call("yui_schema", topic="drawing", type="Slider")[0])

    def test_mcp_renders_an_image_in_the_states_asked(self):
        self.write("ui.yui", HOVER)
        failed, text, image = self.call("yui_render", file="ui.yui", size="8x8")
        if failed and "no graphics device" in text:
            self.skipTest("no graphics device to render with")
        self.assertFalse(failed, text)
        self.assertEqual((json.loads(text)["width"], json.loads(text)["height"]), (8, 8))

        (self.root / "out.png").write_bytes(image)
        self.assertNotEqual(self.first_pixel(), (255, 0, 0))

        (self.root / "out.png").write_bytes(self.call("yui_render", file="ui.yui", size="8x8", states=[{"id": "panel", "state": "hover"}])[2])
        self.assertEqual(self.first_pixel(), (255, 0, 0))

        scaled = self.call("yui_render", file="ui.yui", size="8x8", scale=2)[2]
        self.assertEqual(tuple(int.from_bytes(scaled[offset:offset + 4], "big") for offset in (16, 20)), (16, 16))
        self.assertTrue(self.call("yui_render", file="ui.yui", scale=9)[0])

    def test_mcp_plays_pointer_events_and_tells_what_they_did(self):
        self.write("ui.yui", INTERACTIVE)
        self.write("ui.models.json", json.dumps({"model": {"power": False, "gain": 0.25}}))

        def play(*events):
            failed, text, _ = self.call("yui_inspect", file="ui.yui", events=list(events))
            self.assertFalse(failed, text)
            return json.loads(text)

        # A click goes where a pointer would, and what it changed in the models is told with the value before
        told = play({"id": "power", "type": "click"})
        self.assertEqual(told["changed"], {"model": {"power": [False, True]}})
        self.assertEqual((told["events"][0]["received"]["id"], told["events"][0]["received"]["type"]), ("power", "ToggleButton"))
        self.assertNotIn("warning", told["events"][0])
        self.assertEqual(told["actions"], [])

        told = play({"id": "gain", "type": "drag", "by": [60, 0]})
        self.assertGreater(told["changed"]["model"]["gain"][1], 0.25)
        self.assertEqual(told["changed"]["model"]["gain"][0], 0.25)

        told = play({"id": "gain", "type": "drag", "at": [10, 15], "to": [190, 15]})
        self.assertGreater(told["changed"]["model"]["gain"][1], 0.8)

        self.assertIn("gain", play({"id": "gain", "type": "wheel", "delta": [0, 2]})["changed"]["model"])
        self.assertEqual(play({"id": "gain", "type": "hover"})["changed"], {})

        # The actions the interface calls are told in order, with their arguments, after the events before them
        told = play({"path": "0", "type": "click"}, {"id": "save", "type": "click"}, {"id": "save", "type": "doubleClick"})
        self.assertEqual(told["changed"], {"model": {"power": [False, True]}})
        self.assertEqual([action["name"] for action in told["actions"]], ["save", "save", "save"])
        self.assertEqual(told["actions"][0]["arguments"], [0.25, "now"])
        self.assertEqual(len(told["events"]), 3)

        for events in ("click", [{"id": "nobody", "type": "click"}], [{"id": "gain", "type": "shake"}], [{"id": "gain", "type": "drag"}],
                       [{"type": "click"}], [{"id": "gain", "type": "wheel"}], [{"id": "gain", "type": "click", "at": [1]}]):
            self.assertTrue(self.call("yui_inspect", file="ui.yui", events=events)[0], events)

    def test_mcp_follows_a_repeat_of_components(self):
        self.write("ui.yui", REPEATED)

        # What the document binds to is found through the repeat, each element of a list as a list
        self.assertEqual(json.loads(self.call("yui_check", file="ui.yui")[1])["missingModels"], {"seq": {"wide": False, "steps": [False]}})

        self.write("ui.models.json", json.dumps({"seq": {"wide": False, "steps": [False, False, False, False]}}))

        def steps(told):
            return told["root"]["children"][1].get("children", [])

        failed, text, _ = self.call("yui_inspect", file="ui.yui")
        self.assertFalse(failed, text)
        self.assertEqual(len(steps(json.loads(text))), 2)

        # An event changing the range has the components made again, and the next event reaches the new ones
        failed, text, _ = self.call("yui_inspect", file="ui.yui", events=[{"id": "wide", "type": "click"}, {"path": "1/3", "type": "click"}])
        self.assertFalse(failed, text)
        told = json.loads(text)

        self.assertEqual(len(steps(told)), 4)
        self.assertEqual(told["changed"], {"seq": {"wide": [False, True], "steps": {"3": [False, True]}}})
        self.assertEqual([(action["name"], action["arguments"]) for action in told["actions"]], [("step", [3])])

    def test_mcp_warns_when_an_event_does_not_reach_its_component(self):
        self.write("ui.yui", COVERED)

        failed, text, _ = self.call("yui_inspect", file="ui.yui", events=[{"id": "under", "type": "click"}, {"id": "off", "type": "click"}])
        self.assertFalse(failed, text)
        told = json.loads(text)

        self.assertEqual(told["events"][0]["received"]["id"], "off")
        self.assertIn("off", told["events"][0]["warning"])
        self.assertIn("disabled", told["events"][1]["warning"])

        # A button that is covered or disabled is not clicked
        self.assertEqual(told["actions"], [])

    def test_mcp_renders_what_the_pointer_left(self):
        self.write("ui.yui", HOVER)
        self.draw(file="ui.yui", size="8x8", events=[{"id": "panel", "type": "hover"}])
        self.assertEqual(self.first_pixel(), (255, 0, 0))

    def draw(self, **arguments):
        """Calls yui_render: returns what it tells and its image, skipping where nothing can draw."""
        failed, text, image = self.call("yui_render", **arguments)
        if failed and "no graphics device" in text:
            self.skipTest("no graphics device to render with")
        self.assertFalse(failed, text)
        (self.root / "out.png").write_bytes(image)
        return json.loads(text), image

    def test_mcp_renders_a_region_or_an_element(self):
        self.write("ui.yui", HALVES.replace("TONE", "#0000ff"))

        told, _ = self.draw(file="ui.yui", region=[20, 0, 20, 20])
        self.assertEqual((told["width"], told["height"], told["region"]), (20, 20, [20, 0, 20, 20]))
        self.assertEqual(self.first_pixel(), (0, 0, 255))

        told, _ = self.draw(file="ui.yui", element="left", margin=0)
        self.assertEqual((told["width"], told["height"], told["region"]), (20, 20, [0, 0, 20, 20]))
        self.assertEqual(self.first_pixel(), (255, 0, 0))

        # The margin around an element stops at the edges of the interface, and a path names one as an id does
        self.assertEqual(self.draw(file="ui.yui", element="1")[0]["region"], [12, 0, 28, 20])

        told, _ = self.draw(file="ui.yui", region=[30, 5, 100, 100], scale=8)
        self.assertEqual((told["width"], told["height"], told["region"]), (80, 120, [30, 5, 10, 15]))

        self.assertTrue(self.call("yui_render", file="ui.yui", scale=8)[0])
        self.assertTrue(self.call("yui_render", file="ui.yui", region=[40, 0, 10, 10])[0])
        self.assertTrue(self.call("yui_render", file="ui.yui", region=[0, 0, 10])[0])
        self.assertTrue(self.call("yui_render", file="ui.yui", region=[0, 0, 10, 10], element="left")[0])
        self.assertTrue(self.call("yui_render", file="ui.yui", element="nobody")[0])
        self.assertTrue(self.call("yui_render", file="ui.yui", size="4096x10", scale=2)[0])

    def test_mcp_renders_a_grid_of_coordinates(self):
        self.write("ui.yui", HALVES.replace("TONE", "#0000ff"))

        _, plain = self.draw(file="ui.yui", scale=4)
        told, ruled = self.draw(file="ui.yui", scale=4, grid=10)
        self.assertEqual((told["width"], told["height"], told["grid"]), (160, 80, 10))
        self.assertNotEqual(plain, ruled)

        self.assertTrue(self.call("yui_render", file="ui.yui", grid=1)[0])
        self.assertTrue(self.call("yui_render", file="ui.yui", grid=-10)[0])

    def test_mcp_saves_the_image_where_asked(self):
        self.write("ui.yui", HALVES.replace("TONE", "#0000ff"))

        told, image = self.draw(file="ui.yui", output="shots/first.png")
        self.assertEqual(told["output"], str(self.root / "shots" / "first.png"))
        self.assertEqual((self.root / "shots" / "first.png").read_bytes(), image)

        self.assertTrue(self.call("yui_render", file="ui.yui", output="../outside.png")[0])
        self.assertTrue(self.call("yui_render", file="ui.yui", output="shots/first.jpg")[0])
        self.assertFalse((self.root / "shots" / "first.jpg").exists())

    def test_mcp_compares_a_render_with_a_reference(self):
        self.write("ui.yui", HALVES.replace("TONE", "#0000ff"))
        self.write("other.yui", HALVES.replace("TONE", "#00ff00"))
        self.draw(file="ui.yui", output="reference.png")

        # Three panels, one above the other for an interface wider than tall
        told, _ = self.draw(file="ui.yui", reference="reference.png")
        compared = told["comparison"]
        self.assertEqual((told["width"], told["height"]), (40, 76))
        self.assertEqual((compared["similarity"], compared["differentPixels"], compared["worst"]), (1, 0, []))
        self.assertEqual((compared["reference"], compared["resized"], compared["panel"]), ([40, 20], False, [40, 20]))
        self.assertEqual((compared["panels"], compared["arrangement"]), (["reference", "render", "difference"], "column"))

        # What differs is told by how much and where, as regions to draw next
        compared = self.draw(file="other.yui", reference="reference.png")[0]["comparison"]
        self.assertEqual(compared["differentPixels"], 0.5)
        self.assertLess(compared["similarity"], 0.9)
        self.assertTrue(compared["worst"])
        self.assertTrue(all(cell["region"][0] >= 20 and cell["error"] > 0 for cell in compared["worst"]))

        # A region is compared with the same region of the reference
        compared = self.draw(file="other.yui", reference="reference.png", region=[0, 0, 20, 20])[0]["comparison"]
        self.assertEqual((compared["similarity"], compared["arrangement"]), (1, "row"))

        # A reference of another size is brought to the size of the render
        self.draw(file="ui.yui", scale=2, output="double.png")
        compared = self.draw(file="ui.yui", reference="double.png")[0]["comparison"]
        self.assertEqual((compared["reference"], compared["resized"]), ([80, 40], True))
        self.assertGreater(compared["similarity"], 0.95)

        # An interface with no size of its own takes the one of the reference
        self.write("bare.yui", "Component { }\n")
        self.assertEqual(self.draw(file="bare.yui", reference="reference.png")[0]["comparison"]["panel"], [40, 20])

        self.write("notes.png", "not an image")
        self.assertTrue(self.call("yui_render", file="ui.yui", reference="missing.png")[0])
        self.assertTrue(self.call("yui_render", file="ui.yui", reference="notes.png")[0])

    def test_mcp_reads_and_writes_images_inside_its_folder_only(self):
        self.write("ui.yui", HALVES.replace("TONE", "#0000ff"))
        self.draw(file="ui.yui", output="reference.png")

        elsewhere = tempfile.TemporaryDirectory()
        self.addCleanup(elsewhere.cleanup)
        outside = Path(elsewhere.name).resolve()
        (outside / "reference.png").write_bytes((self.root / "reference.png").read_bytes())
        (self.root / "link").symlink_to(outside, target_is_directory=True)

        for reference in (str(outside / "reference.png"), "link/reference.png"):
            failed, text, _ = self.call("yui_render", file="ui.yui", reference=reference)
            self.assertTrue(failed and "'reference'" in text, text)

        # A link is not followed out of the folder
        failed, text, _ = self.call("yui_render", file="ui.yui", output="link/shot.png")
        self.assertTrue(failed and "'output'" in text, text)
        self.assertFalse((outside / "shot.png").exists())

    def test_mcp_refuses_what_is_not_a_number(self):
        self.write("ui.yui", HALVES.replace("TONE", "#0000ff"))

        for name, value in (("scale", "nan"), ("grid", "nan"), ("region", ["nan", 0, 10, 10])):
            failed, text, _ = self.call("yui_render", file="ui.yui", **{name: value})
            self.assertTrue(failed and f"'{name}'" in text, text)


if __name__ == "__main__":
    unittest.main()
