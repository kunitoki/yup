// Tests of yup_yui_tool built for the web, run by `just yui_tool_wasm_test`:
//
//     node --test tests/test_web.js
//
// The module is loaded as an editor extension loads it: without starting its application, giving it
// the files in memory and running the commands on them.

const assert = require ("node:assert/strict");
const fs = require ("node:fs");
const path = require ("node:path");
const { before, test } = require ("node:test");

const createYuiTool = require (process.env.YUI_TOOL_WASM ?? path.join (__dirname, "../build_wasm/yup_yui_tool.js"));
const repository = path.join (__dirname, "../../../..");

let tool;
let output = [];
let errors = [];

function write (name, data) {
    const bytes = Buffer.from (data);
    const pointer = tool._yuiToolAllocate (bytes.length);

    new Uint8Array (tool.wasmMemory.buffer, pointer, bytes.length).set (bytes);
    tool.ccall ("yuiToolWriteFile", null, ["string"], [name]);
}

function run (...args) {
    output = [];
    errors = [];

    const code = tool.ccall ("yuiToolRun", "number", ["string"], [JSON.stringify (args)]);
    return { code, stdout: output.join ("\n"), stderr: errors.join ("\n") };
}

function lint (...args) {
    const result = run ("lint", "--json", ...args);
    return { code: result.code, diagnostics: JSON.parse (result.stdout).diagnostics };
}

function agent (name, args = {}) {
    return JSON.parse (tool.ccall ("yuiAgentCall", "string", ["string", "string"], [name, JSON.stringify (args)]));
}

before (async () => {
    tool = await createYuiTool ({ noInitialRun: true, print: line => output.push (line), printErr: line => errors.push (line) });
});

test ("a valid document has no problems", () => {
    write ("valid/main.yui", "Column {\n    gap: 4;\n\n    Label { text: \"Hello\"; }\n}\n");

    assert.deepEqual (lint ("valid/main.yui"), { code: 0, diagnostics: [] });
});

test ("a problem is reported at its place in the file given", () => {
    write ("warning/main.yui", "Column {\n    gapp: 4;\n}\n");

    const { code, diagnostics } = lint ("warning/main.yui");

    assert.equal (code, 0);
    assert.deepEqual (diagnostics, [{ file: "/yui/warning/main.yui", line: 2, column: 5, severity: "warning", message: "Column.gapp: undeclared property 'gapp'" }]);
});

test ("a syntax error fails the command", () => {
    write ("syntax/main.yui", "Column {\n    gap 4;\n}\n");

    const { code, diagnostics } = lint ("syntax/main.yui");

    assert.equal (code, 1);
    assert.equal (diagnostics.length, 1);
    assert.equal (diagnostics[0].severity, "error");
    assert.equal (diagnostics[0].line, 2);
});

test ("a file given again replaces the one before", () => {
    write ("replaced/main.yui", "Column {\n    gap 4;\n}\n");
    assert.equal (lint ("replaced/main.yui").code, 1);

    write ("replaced/main.yui", "Column {\n    gap: 4;\n}\n");
    assert.deepEqual (lint ("replaced/main.yui"), { code: 0, diagnostics: [] });
});

test ("a file emptied replaces the one before", () => {
    write ("emptied/main.yui", "Column {\n    gap 4;\n}\n");
    assert.equal (lint ("emptied/main.yui").code, 1);

    write ("emptied/main.yui", "");
    assert.deepEqual (lint ("emptied/main.yui"), { code: 0, diagnostics: [] });
});

test ("a file removed is no longer there to import", () => {
    write ("removed/main.yui", "import \"part.yui\";\n\nColumn {\n    Part {}\n}\n");
    write ("removed/part.yui", "component Part : Label {\n    text: \"Part\";\n}\n");
    assert.equal (lint ("removed/main.yui").code, 0);

    tool.ccall ("yuiToolRemoveFile", null, ["string"], ["removed/part.yui"]);
    assert.equal (lint ("removed/main.yui").code, 1);
});

test ("an empty file is a document with no problems", () => {
    write ("empty/main.yui", "");

    assert.deepEqual (lint ("empty/main.yui"), { code: 0, diagnostics: [] });
});

test ("an import is read from the files given, and its problems are reported on it", () => {
    write ("imports/main.yui", "import \"parts/title.yui\";\n\nColumn {\n    Title {}\n}\n");
    write ("imports/parts/title.yui", "component Title : Label {\n    text: \"Title\";\n    oops: 1;\n}\n");

    const { code, diagnostics } = lint ("imports/main.yui");

    assert.equal (code, 0);
    assert.deepEqual (diagnostics.map (diagnostic => [diagnostic.file, diagnostic.line]), [["/yui/imports/parts/title.yui", 3]]);
});

test ("the models a document binds to are declared by a file", () => {
    write ("models/main.yui", "Slider {\n    value <=> params.cutoff;\n}\n");
    write ("models/models.json", "{ \"params\": { \"cutoff\": 0.5 } }");

    assert.equal (lint ("models/main.yui").code, 1);
    assert.deepEqual (lint ("--models", "models/models.json", "models/main.yui"), { code: 0, diagnostics: [] });
});

test ("the schema describes the component types, the palette roles and the named colors", () => {
    const result = run ("schema");
    const schema = JSON.parse (result.stdout);

    assert.equal (result.code, 0);
    assert.equal (schema.version, 1);
    assert.ok (schema.types.some (type => type.name === "Slider" && type.instantiable));
    assert.ok (schema.paletteRoles.includes ("accent"));
    assert.ok (schema.namedColors.includes ("transparentBlack"));
});

test ("an SVG is imported as a drawing", () => {
    write ("svg/logo.svg", fs.readFileSync (path.join (repository, "logos/yup_color_icon.svg")));

    const result = run ("import-svg", "svg/logo.svg", "--name", "Logo");

    assert.equal (result.code, 0);
    assert.match (result.stdout, /^drawing Logo\b/m);
});

test ("what is not a command is refused with the usage", () => {
    const result = run ("bogus");

    assert.equal (result.code, 2);
    assert.match (result.stderr, /yup_yui_tool lint/);
});

test ("rendering to an image is left to the native tool", () => {
    write ("render/main.yui", "Column {}\n");

    assert.equal (run ("render", "render/main.yui", "--output", "main.png").code, 3);
});

test ("the tools of an agent are listed, with those only a page can run", () => {
    const listed = JSON.parse (tool.ccall ("yuiAgentTools", "string", [], []));

    assert.deepEqual (listed.tools.map (entry => entry.name), ["yui_check", "yui_inspect", "yui_render", "yui_schema", "yui_capture_preview"]);
    assert.deepEqual (listed.drawing, ["yui_render", "yui_capture_preview"]);
    assert.deepEqual (listed.tools[0].inputSchema.required, ["file"]);
    assert.ok (listed.instructions.length > 0);
});

test ("an agent checks and inspects the files given", () => {
    write ("agent/main.yui", "Column {\n    Row { id: bar;\n        Label { text: \"Lost\"; }\n    }\n}\n");

    const checked = agent ("yui_check", { file: "agent/main.yui" });

    assert.equal (checked.isError, false);
    assert.equal (checked.image, "");
    assert.deepEqual (JSON.parse (checked.text), { file: "/yui/agent/main.yui", errors: 0, warnings: 0, problems: [] });

    const inspected = agent ("yui_inspect", { file: "agent/main.yui", size: "200x100" });
    const layout = JSON.parse (inspected.text);

    assert.equal (inspected.isError, false);
    assert.equal (layout.root.children[0].id, "bar");
    assert.deepEqual (layout.warnings.map (warning => warning.path), ["0"]);
});

test ("an agent plays pointer events and reads what they changed", () => {
    write ("events/main.yui", "Column { width: 200; height: 120;\n"
                              + "    ToggleButton { id: power; preferredHeight: 30; checked <=> model.power; }\n"
                              + "    Slider { id: gain; preferredHeight: 30; value <=> model.gain; }\n"
                              + "    TextButton { id: save; preferredHeight: 30; onClick: action (\"save\", model.gain); }\n"
                              + "    Row { preferredHeight: 30; repeat i in 0 .. (model.power ? 3 : 1) { Label { text: format (\"%d\", i); } } }\n}\n");
    write ("events/main.models.json", JSON.stringify ({ model: { power: false, gain: 0.25 } }));

    const played = agent ("yui_inspect", { file: "events/main.yui", events: [{ id: "power", type: "click" },
                                                                             { id: "gain", type: "drag", by: [60, 0] },
                                                                             { id: "save", type: "click" }] });
    const told = JSON.parse (played.text);

    assert.equal (played.isError, false, played.text);
    assert.deepEqual (told.events.map (event => event.received.id), ["power", "gain", "save"]);
    assert.deepEqual (told.changed.model.power, [false, true]);
    assert.ok (told.changed.model.gain[1] > 0.25);
    assert.equal (told.actions[0].name, "save");

    // The components of a repeat follow the range the events changed, with no message loop to wait for
    assert.equal (told.root.children[3].children.length, 3);
});

test ("an agent is told what a file can name, the models it lacks and where its drawings land", () => {
    write ("lacking/main.yui", "drawing Mark {\n    rect { id: box; bounds: rect (2, 4, 20, 10); fill: #ff0000; }\n}\n\n"
                               + "style {\n    #mark { drawing: Mark; }\n}\n\n"
                               + "Component { width: 100; height: 50;\n"
                               + "    Component { id: mark; x: 10; y: 10; width: 40; height: 20; enabled: seq.on; opacity: seq.steps[2]; }\n}\n");

    const checked = JSON.parse (agent ("yui_check", { file: "lacking/main.yui" }).text);

    assert.deepEqual (checked.missingModels, { seq: { on: false, steps: [0] } });
    assert.equal (checked.errorsWithMissingModels, 0);

    write ("lacking/main.models.json", JSON.stringify (checked.missingModels));

    const mark = JSON.parse (agent ("yui_inspect", { file: "lacking/main.yui", element: "mark", drawings: true }).text).root;

    assert.deepEqual (mark.drawing.map (element => [element.id, element.bounds, element.line]), [["box", [12, 14, 20, 10], 2]]);

    for (const topic of ["drawing", "styles", "expressions"])
        assert.equal (agent ("yui_schema", { topic }).isError, false);

    assert.ok (JSON.parse (agent ("yui_schema", { topic: "drawing" }).text).elements.some (element => element.type === "rect"));
    assert.deepEqual (JSON.parse (agent ("yui_schema", { type: "Slider" }).text).bases, ["Component"]);
});

test ("an agent is shown a drawing where the interface uses it", () => {
    write ("used/main.yui", "drawing Dial {\n    circle { center: bounds.center; radius: 2 + 4 * normalized; fill: #ffffff; }\n}\n\n"
                            + "style {\n    .dial { drawing: Dial; }\n}\n\n"
                            + "Component { width: 100; height: 80;\n"
                            + "    Slider { id: first; class: dial; x: 10; y: 20; width: 30; height: 30; paintMargin: 4; }\n}\n");

    const used = JSON.parse (agent ("yui_inspect", { file: "used/main.yui", drawing: "Dial" }).text);

    assert.deepEqual (used.use, { type: "Slider", id: "first", path: "0" });
    assert.deepEqual ([used.root.bounds, used.root.paintBounds, used.problems], [[10, 20, 30, 30], [6, 16, 38, 38], []]);
    assert.equal (run ("--version").code, 0);
});

test ("an editor is told what a file is made of and what can be written in it, while it does not load", () => {
    write ("typed/main.yui", "import \"parts.yui\";\n\ndrawing Knob { property ring: color = #abc; }\n\nColumn {\n    Slider { wid\n");
    write ("typed/parts.yui", "component Dial : Slider { }\n");

    const outline = JSON.parse (run ("outline", "typed/main.yui").stdout);

    assert.deepEqual (outline.symbols.map (symbol => symbol.name), ["Knob", "Column"]);
    assert.deepEqual (outline.targets.map (target => target.id), ["root", "drawing:Knob", "component:Dial"]);
    assert.deepEqual (outline.imports[0].file, "/yui/typed/parts.yui");
    assert.equal (outline.colors.length, 1);

    const labels = JSON.parse (run ("complete", "typed/main.yui", "--at", "6:17").stdout).items.map (item => item.label);

    assert.ok (labels.includes ("width") && labels.includes ("Dial"));
    assert.deepEqual (JSON.parse (run ("definition", "typed/main.yui", "--at", "1:10").stdout), { file: "/yui/typed/parts.yui", line: 1, column: 1 });
});

test ("an agent is told why a tool can't answer", () => {
    write ("agent/plain.yui", "Column {}\n");

    assert.equal (agent ("yui_check", { file: "agent/missing.yui" }).isError, true);
    assert.equal (agent ("yui_bogus").isError, true);
    assert.match (agent ("yui_render", { file: "agent/plain.yui" }).text, /no graphics device/);
    assert.match (agent ("yui_render", { file: "agent/plain.yui", output: "agent/plain.png" }).text, /nowhere to write/);
    assert.equal (agent ("yui_render", { file: "agent/plain.yui", reference: "agent/missing.png" }).isError, true);
    assert.match (agent ("yui_capture_preview").text, /no live preview/);
});
