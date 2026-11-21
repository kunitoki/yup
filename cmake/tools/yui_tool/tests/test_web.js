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

test ("an agent is told why a tool can't answer", () => {
    write ("agent/plain.yui", "Column {}\n");

    assert.equal (agent ("yui_check", { file: "agent/missing.yui" }).isError, true);
    assert.equal (agent ("yui_bogus").isError, true);
    assert.match (agent ("yui_render", { file: "agent/plain.yui" }).text, /no graphics device/);
    assert.match (agent ("yui_capture_preview").text, /no live preview/);
});
