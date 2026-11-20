/** The zoom levels of the preview. 'fit' shows it at its size, shrunk to fit the panel when larger. */
export const zoomLevels = ["fit", "50", "100", "200", "400"];

/** Where the page finds the tool, the module running the interface. */
export interface PreviewModule {
    /** The address of the script of the module. */
    script: string;

    /** The address of the folder of the module, ending with a slash. */
    folder: string;

    /** What the page is allowed to load scripts and data from. */
    source: string;
}

/** The largest width or height of the preview in logical pixels, which a canvas can still be given. */
export const maximumSize = 8192;

const sizePresets = ["320x240", "480x320", "640x400", "800x600", "1024x768", "1280x720"];
const contentPadding = 12;

/**
    The page of the preview panel: a toolbar and, under it, the interface with its problems.

    The page is loaded once and runs the tool, which shows the interface in a canvas. It says when it
    is ready and what is chosen in the toolbar, and is told what the toolbar offers and what to show:
    the files of a document, and what of it.
*/
export function previewPage (nonce: string, module: PreviewModule | undefined): string {
    const sizes = sizePresets.map (size => `<option value="${size}">${size.replace ("x", " x ")}</option>`).join ("");
    const zooms = zoomLevels.map (zoom => `<option value="${zoom}">${zoom === "fit" ? "Fit" : `${zoom}%`}</option>`).join ("");
    const scripts = module === undefined ? `'nonce-${nonce}'` : `'nonce-${nonce}' 'wasm-unsafe-eval' ${module.source}`;
    const requests = module === undefined ? "" : ` connect-src ${module.source};`;
    const tool = module === undefined ? "null" : JSON.stringify ({ script: module.script, folder: module.folder });

    return `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src 'unsafe-inline'; script-src ${scripts};${requests}">
<style>
    html, body {
        height: 100%;
    }
    body {
        display: flex;
        flex-direction: column;
        margin: 0;
        padding: 0;
        background: var(--vscode-editor-background);
        color: var(--vscode-editor-foreground);
        font-family: var(--vscode-font-family);
        font-size: var(--vscode-font-size);
    }
    [hidden] {
        display: none !important;
    }
    #toolbar {
        flex: none;
        display: flex;
        flex-wrap: wrap;
        align-items: center;
        gap: 6px 12px;
        padding: 6px ${contentPadding}px;
        background: var(--vscode-editorWidget-background);
        border-bottom: 1px solid var(--vscode-editorWidget-border, transparent);
    }
    #toolbar label, #toolbar .fields {
        display: inline-flex;
        align-items: center;
        gap: 5px;
    }
    #toolbar span {
        opacity: 0.7;
    }
    select, input, button {
        box-sizing: border-box;
        height: 24px;
        padding: 0 4px;
        border: 1px solid var(--vscode-dropdown-border, transparent);
        border-radius: 2px;
        background: var(--vscode-dropdown-background);
        color: var(--vscode-dropdown-foreground);
        font: inherit;
    }
    select {
        max-width: 220px;
    }
    input {
        width: 56px;
        border-color: var(--vscode-input-border, transparent);
        background: var(--vscode-input-background);
        color: var(--vscode-input-foreground);
    }
    input::-webkit-outer-spin-button, input::-webkit-inner-spin-button {
        -webkit-appearance: none;
        margin: 0;
    }
    button {
        padding: 0 10px;
        border-color: transparent;
        background: var(--vscode-button-secondaryBackground);
        color: var(--vscode-button-secondaryForeground);
        cursor: pointer;
    }
    button:hover {
        background: var(--vscode-button-secondaryHoverBackground);
    }
    select:focus-visible, input:focus-visible, button:focus-visible {
        outline: 1px solid var(--vscode-focusBorder);
        outline-offset: -1px;
    }
    #toolbar button.on {
        background: var(--vscode-button-background);
        color: var(--vscode-button-foreground);
    }
    #main {
        flex: 1;
        display: flex;
        min-height: 0;
    }
    #viewport {
        flex: 1;
        position: relative;
        min-width: 0;
    }
    #models {
        flex: none;
        display: flex;
        flex-direction: column;
        width: 280px;
        border-left: 1px solid var(--vscode-editorWidget-border, transparent);
        background: var(--vscode-editorWidget-background);
    }
    #modelsHeader {
        display: flex;
        align-items: center;
        gap: 6px;
        padding: 6px 8px;
        border-bottom: 1px solid var(--vscode-editorWidget-border, transparent);
    }
    #modelsHeader span {
        flex: 1;
        overflow: hidden;
        opacity: 0.7;
        text-overflow: ellipsis;
        white-space: nowrap;
    }
    #modelsBody {
        flex: 1;
        overflow: auto;
        padding: 0 8px 8px;
    }
    #modelsBody h3 {
        margin: 10px 0 4px;
        font-size: 11px;
        font-weight: 600;
        text-transform: uppercase;
        opacity: 0.7;
    }
    #modelsBody p {
        opacity: 0.8;
    }
    .property {
        display: flex;
        flex-direction: column;
        gap: 2px;
        margin-bottom: 6px;
    }
    .property.modified > label::after {
        content: " *";
        color: var(--vscode-editorWarning-foreground);
    }
    .property .control {
        display: flex;
        align-items: center;
        gap: 6px;
    }
    .property input[type=range] {
        flex: 1;
        min-width: 0;
        height: auto;
        padding: 0;
        border: none;
        background: transparent;
    }
    .property input[type=number] {
        width: 72px;
    }
    .property input[type=text], .property textarea {
        flex: 1;
        width: 100%;
    }
    .property input[type=checkbox] {
        width: auto;
        height: auto;
    }
    .property textarea {
        box-sizing: border-box;
        min-height: 40px;
        resize: vertical;
        border: 1px solid var(--vscode-input-border, transparent);
        background: var(--vscode-input-background);
        color: var(--vscode-input-foreground);
        font-family: var(--vscode-editor-font-family);
        font-size: 11px;
    }
    #content {
        position: absolute;
        inset: 0;
        box-sizing: border-box;
        padding: ${contentPadding}px;
        overflow: auto;
    }
    #stage {
        margin: 0 auto;
        overflow: hidden;
    }
    /* Out of sight but still laid out: the tool reads the size of the canvas it is given */
    #stage.idle {
        position: absolute;
        visibility: hidden;
        pointer-events: none;
    }
    #canvas {
        display: block;
        transform-origin: 0 0;
        outline: none;
    }
    #note {
        margin: 0;
        opacity: 0.8;
    }
    /* Kept in sight at the bottom of the panel, over an interface that fills it */
    #problems {
        position: sticky;
        bottom: 0;
        box-sizing: border-box;
        max-height: 40%;
        margin: 0;
        padding: 8px;
        overflow: auto;
        border: 1px solid var(--vscode-editorWidget-border, transparent);
        border-radius: 4px;
        background: var(--vscode-editorWidget-background);
        color: var(--vscode-errorForeground);
        white-space: pre-wrap;
    }
    #stage:not(.idle) ~ #problems {
        margin-top: 12px;
    }
</style>
</head>
<body>
<div id="toolbar">
    <label title="What of the document is shown"><span>Show</span><select id="target"></select></label>
    <label id="hostLabel" title="The component painting the drawing" hidden><span>as</span><select id="host"></select></label>
    <label title="Auto is the size the root asks for or else the one of the panel, Panel always follows the panel"><span>Size</span><select id="size"><option value="auto">Auto</option><option value="panel">Panel</option>${sizes}<option value="custom">Custom</option></select></label>
    <div class="fields" title="The size in logical pixels, type one to fix it">
        <input id="width" type="number" min="1" max="${maximumSize}" aria-label="Width">
        <span>x</span>
        <input id="height" type="number" min="1" max="${maximumSize}" aria-label="Height">
    </div>
    <label title="Auto follows the color theme of the editor"><span>Theme</span><select id="theme"><option value="auto">Auto</option><option value="light">Light</option><option value="dark">Dark</option></select></label>
    <label title="Fit shrinks the interface to fit the panel when it is larger"><span>Zoom</span><select id="zoom">${zooms}</select></label>
    <button id="refresh" title="Show the document again">Refresh</button>
    <button id="modelsToggle" title="Show the values of the models the interface binds to, to fake them">Models</button>
</div>
<div id="main">
    <div id="viewport">
        <div id="content">
            <div id="stage" class="idle"><canvas id="canvas" tabindex="-1"></canvas></div>
            <p id="note" hidden></p>
            <pre id="problems" hidden></pre>
        </div>
    </div>
    <div id="models" hidden>
        <div id="modelsHeader">
            <span id="modelsFile"></span>
            <button id="modelsReset" title="Go back to the values of the models file">Reset</button>
            <button id="modelsSave" title="Write these values to the models file">Save</button>
        </div>
        <div id="modelsBody"></div>
    </div>
</div>
<script nonce="${nonce}">
(function () {
    const vscode = acquireVsCodeApi ();
    const element = id => document.getElementById (id);
    const target = element ("target");
    const host = element ("host");
    const size = element ("size");
    const width = element ("width");
    const height = element ("height");
    const theme = element ("theme");
    const zoom = element ("zoom");
    const viewport = element ("viewport");
    const stage = element ("stage");
    const canvas = element ("canvas");
    const note = element ("note");
    const problems = element ("problems");
    const module = ${tool};

    const choose = (name, value) => vscode.postMessage ({ type: "option", name: name, value: value });

    function fill (select, items, selected) {
        const signature = JSON.stringify (items);

        if (select.dataset.items !== signature) {
            select.dataset.items = signature;
            select.replaceChildren ();

            for (const item of items) {
                const option = document.createElement ("option");
                option.value = item.id;
                option.textContent = item.label;
                select.appendChild (option);
            }
        }

        select.value = selected;
    }

    function typedSize () {
        const columns = Math.round (Number (width.value));
        const rows = Math.round (Number (height.value));

        return columns >= 1 && columns <= ${maximumSize} && rows >= 1 && rows <= ${maximumSize} ? columns + "x" + rows : undefined;
    }

    function showSize (columns, rows) {
        if (document.activeElement !== width)
            width.value = String (columns);

        if (document.activeElement !== height)
            height.value = String (rows);
    }

    function showText (node, text) {
        node.textContent = text;
        node.hidden = text.length === 0;
    }

    function applyState (state) {
        fill (target, state.targets, state.target);
        target.disabled = state.targets.length === 0;

        fill (host, state.hosts.map (name => ({ id: name, label: name })), state.host);
        element ("hostLabel").hidden = state.hosts.length === 0;

        size.value = state.size;

        // A fixed size that is none of the listed ones
        if (size.selectedIndex < 0)
            size.value = "custom";

        if (/^[0-9]+x[0-9]+$/.test (state.size))
            showSize (state.size.split ("x")[0], state.size.split ("x")[1]);

        theme.value = state.theme;
        zoom.value = state.zoom;
    }

    // Shows a note in place of the interface
    function showNote (text) {
        shown = undefined;
        stage.classList.add ("idle");
        showText (note, text);
        showText (problems, "");
    }

    // ---------------------------------------------------------------- the tool

    // The space left for the interface, in logical pixels
    const panel = { width: 1, height: 1 };

    let tool;
    let failed = false;
    let waiting;
    let shown;
    let report;
    let collecting = false;
    let errors = [];

    function fail (reason) {
        if (failed)
            return;

        failed = true;
        tool = undefined;
        waiting = undefined;
        showNote ("The preview can't run here: " + reason + ".");
        vscode.postMessage ({ type: "failed", reason: String (reason) });
    }

    function start () {
        if (document.createElement ("canvas").getContext ("webgl2") === null) {
            fail ("WebGL 2 is not available in this window");
            return;
        }

        const options = {
            canvas: canvas,
            locateFile: name => module.folder + name,
            print: () => {},
            printErr: text => {
                if (collecting)
                    errors.push (String (text));
                else
                    console.warn (text);
            },
            onAbort: reason => fail (reason),
            yuiPreviewShown: outcome => { report = outcome; },
            yuiPreviewModelChanged: (model, property, value) => modelChanged (model, property, value),
            yuiPreviewReady: () => {
                tool = options;

                if (waiting !== undefined)
                    show (waiting);
            }
        };

        const script = document.createElement ("script");
        script.nonce = "${nonce}";
        script.src = module.script;
        script.addEventListener ("load", () => createYuiTool (options).catch (fail));
        script.addEventListener ("error", () => fail ("the tool could not be loaded"));
        document.body.appendChild (script);
    }

    // Gives the window of the tool its size, and scales the canvas it draws in
    function layout () {
        if (shown === undefined)
            return;

        const message = shown.message;
        const outcome = shown.report;
        const factor = message.zoom === "fit" ? 1 : Number (message.zoom) / 100;
        const fitted = [Math.max (1, Math.floor (panel.width / factor)), Math.max (1, Math.floor (panel.height / factor))];
        let columns = outcome.defaultWidth;
        let rows = outcome.defaultHeight;

        if (/^[0-9]+x[0-9]+$/.test (message.size))
            [columns, rows] = message.size.split ("x").map (Number);
        else if (message.size === "auto" && outcome.width >= 1 && outcome.height >= 1)
            [columns, rows] = [Math.round (outcome.width), Math.round (outcome.height)];
        else if (message.size === "panel" || message.interface)
            [columns, rows] = fitted;

        const scale = message.zoom === "fit" ? Math.min (1, panel.width / columns, panel.height / rows) : factor;

        tool._yupSetPrimaryWindowSize (columns, rows);
        canvas.style.width = columns + "px";
        canvas.style.height = rows + "px";
        canvas.style.transform = scale === 1 ? "" : "scale(" + scale + ")";
        stage.style.width = Math.floor (columns * scale) + "px";
        stage.style.height = Math.floor (rows * scale) + "px";
        showSize (columns, rows);
    }

    function show (message) {
        if (failed)
            return;

        if (tool === undefined) {
            // The files of the message waiting behind this one are still to be written
            if (waiting !== undefined)
                message = Object.assign ({}, message, { files: waiting.files.concat (message.files) });

            waiting = message;
            showNote ("Starting the preview...");
            return;
        }

        waiting = undefined;

        try {
            for (const file of message.files) {
                const pointer = tool._yuiToolAllocate (file.data.length);
                new Uint8Array (tool.wasmMemory.buffer, pointer, file.data.length).set (file.data);
                tool.ccall ("yuiToolWriteFile", null, ["string"], [file.path]);
            }

            report = undefined;
            errors = [];
            collecting = true;
            tool.ccall ("yuiPreviewShow", null, ["string", "string", "string", "string", "string", "number"],
                        [message.main, message.drawing, message.palette, message.host, message.models, message.light ? 1 : 0]);
        }
        catch (error) {
            fail (error);
            return;
        }
        finally {
            collecting = false;
        }

        if (report === undefined) {
            fail ("the tool did not answer");
            return;
        }

        // The tool names each folder of documents by a number, which means nothing to who reads the problems
        const named = path => path.replace (/^([/]yui[/])?[0-9]+[/]/, "");
        const lines = report.problems.map (problem => named (problem.file) + (problem.line > 0 ? ":" + problem.line + ":" + problem.column : "") + ": " + problem.severity + ": " + problem.message);

        // A document that stops loading keeps the interface it had, with its problems listed under it
        shown = report.hasInterface ? { message: message, report: report } : undefined;
        stage.classList.toggle ("idle", ! report.hasInterface);
        showText (note, report.status === "nothing" ? "Nothing to preview: " + report.message + "." : "");
        showText (problems, lines.concat (errors.map (named)).join ("\\n"));
        layout ();

        liveValues = report.models || {};
        fileValues = message.modelsValues !== null && typeof message.modelsValues === "object" ? message.modelsValues : null;
        modelsFile.textContent = message.modelsFile || "";
        modelsFile.title = message.modelsFile || "";
        buildModels ();
    }

    // ------------------------------------------------------------ models

    // The values the interface sees, the ones of the file and the controls showing them, by model and property
    let liveValues = {};
    let fileValues = null;
    let modelsSignature;
    const controls = new Map ();
    const modelsPanel = element ("models");
    const modelsBody = element ("modelsBody");
    const modelsFile = element ("modelsFile");
    const modelsToggle = element ("modelsToggle");

    const kindOf = value => Array.isArray (value) ? "array" : (value === null ? "json" : (["number", "boolean", "string"].includes (typeof value) ? typeof value : "json"));
    const keyOf = (model, property) => model + "\\u0000" + property;
    const isMeta = name => name.startsWith ("$");
    const rounded = value => Math.round (value * 1e6) / 1e6;

    // Arrays of plain values stay on one line, so long series remain readable
    function formatJson (value, indent) {
        const inner = indent + "    ";

        if (Array.isArray (value)) {
            if (value.every (item => item === null || typeof item !== "object"))
                return "[" + value.map (item => formatJson (item, inner)).join (", ") + "]";

            return value.length === 0 ? "[]" : "[\\n" + value.map (item => inner + formatJson (item, inner)).join (",\\n") + "\\n" + indent + "]";
        }

        if (value !== null && typeof value === "object") {
            const entries = Object.entries (value);
            return entries.length === 0 ? "{}" : "{\\n" + entries.map (([name, item]) => inner + JSON.stringify (name) + ": " + formatJson (item, inner)).join (",\\n") + "\\n" + indent + "}";
        }

        return typeof value === "number" ? String (rounded (value)) : JSON.stringify (value);
    }

    function send (model, property, value) {
        if (tool === undefined)
            return;

        liveValues[model][property] = value;
        tool.ccall ("yuiPreviewSetModelValue", "number", ["string", "string", "string"], [model, property, JSON.stringify (value)]);
        markModified (model, property);
    }

    function markModified (model, property) {
        const control = controls.get (keyOf (model, property));
        const original = fileValues !== null && fileValues[model] !== undefined ? fileValues[model][property] : undefined;

        if (control !== undefined)
            control.row.classList.toggle ("modified", formatJson (liveValues[model][property], "") !== formatJson (original, ""));
    }

    function modelChanged (model, property, value) {
        if (liveValues[model] === undefined)
            return;

        liveValues[model][property] = value;
        controls.get (keyOf (model, property))?.update (value);
        markModified (model, property);
    }

    // The range of a number, from "$ranges" in the file ([minimum, maximum, step]) or else guessed from its value
    function rangeOf (model, property, value) {
        const declared = fileValues !== null && fileValues["$ranges"] && fileValues["$ranges"][model] ? fileValues["$ranges"][model][property] : undefined;

        if (Array.isArray (declared) && declared.length >= 2)
            return { minimum: declared[0], maximum: declared[1], step: declared[2] > 0 ? declared[2] : "any" };

        const whole = Number.isInteger (value) && Math.abs (value) > 1;
        return { minimum: Math.min (0, value * 2), maximum: Math.max (1, value * 2), step: whole ? 1 : "any" };
    }

    function numberControl (model, property, value, control) {
        const range = rangeOf (model, property, value);
        const slider = document.createElement ("input");
        const field = document.createElement ("input");

        slider.type = "range";
        field.type = "number";

        for (const input of [slider, field]) {
            input.min = String (range.minimum);
            input.max = String (range.maximum);
            input.step = range.step === "any" ? String ((range.maximum - range.minimum) / 1000) : String (range.step);
        }

        field.step = String (range.step);
        slider.addEventListener ("input", () => {
            field.value = String (rounded (Number (slider.value)));
            send (model, property, Number (slider.value));
        });
        field.addEventListener ("change", () => {
            const typed = Number (field.value);

            if (field.value.trim ().length > 0 && Number.isFinite (typed)) {
                slider.value = String (typed);
                send (model, property, typed);
            }
        });

        control.append (slider, field);
        return value => {
            slider.value = String (value);

            if (document.activeElement !== field)
                field.value = String (rounded (value));
        };
    }

    function textControl (model, property, kind, control) {
        const field = document.createElement (kind === "string" ? "input" : "textarea");

        if (kind === "string")
            field.type = "text";

        field.spellcheck = false;
        field.addEventListener ("change", () => {
            if (kind === "string") {
                send (model, property, field.value);
                return;
            }

            try {
                send (model, property, JSON.parse (field.value));
                field.style.outline = "";
            }
            catch {
                field.style.outline = "1px solid var(--vscode-inputValidation-errorBorder)";
            }
        });

        control.append (field);
        return value => {
            if (document.activeElement !== field)
                field.value = kind === "string" ? String (value) : formatJson (value, "");
        };
    }

    function booleanControl (model, property, control) {
        const box = document.createElement ("input");
        box.type = "checkbox";
        box.addEventListener ("change", () => send (model, property, box.checked));

        control.append (box);
        return value => { box.checked = value === true; };
    }

    function buildModels () {
        const signature = JSON.stringify (Object.entries (liveValues).map (([model, properties]) =>
            [model, Object.entries (properties).map (([property, value]) => [property, kindOf (value)])])) + JSON.stringify (fileValues && fileValues["$ranges"]);

        if (signature === modelsSignature) {
            for (const [model, properties] of Object.entries (liveValues)) {
                for (const [property, value] of Object.entries (properties)) {
                    controls.get (keyOf (model, property))?.update (value);
                    markModified (model, property);
                }
            }

            return;
        }

        modelsSignature = signature;
        controls.clear ();
        modelsBody.replaceChildren ();

        if (Object.keys (liveValues).length === 0) {
            const hint = document.createElement ("p");
            hint.textContent = "No models. Name a models file with the setting yui.lint.models, or put one next to the document named <document>.models.json.";
            modelsBody.append (hint);
            return;
        }

        for (const [model, properties] of Object.entries (liveValues)) {
            const heading = document.createElement ("h3");
            heading.textContent = model;
            modelsBody.append (heading);

            for (const [property, value] of Object.entries (properties)) {
                const row = document.createElement ("div");
                const label = document.createElement ("label");
                const control = document.createElement ("div");
                const kind = kindOf (value);

                row.className = "property";
                label.textContent = property;
                control.className = "control";
                row.append (label, control);
                modelsBody.append (row);

                const update = kind === "number" ? numberControl (model, property, value, control)
                    : kind === "boolean" ? booleanControl (model, property, control)
                    : textControl (model, property, kind, control);

                controls.set (keyOf (model, property), { row: row, update: update });
                update (value);
                markModified (model, property);
            }
        }
    }

    function showModels (open) {
        modelsPanel.hidden = ! open;
        modelsToggle.classList.toggle ("on", open);
        vscode.setState (Object.assign ({}, vscode.getState () || {}, { modelsOpen: open }));
        resized ();
    }

    modelsToggle.addEventListener ("click", () => showModels (modelsPanel.hidden));

    element ("modelsReset").addEventListener ("click", () => {
        if (fileValues === null)
            return;

        for (const [model, properties] of Object.entries (fileValues)) {
            if (isMeta (model) || liveValues[model] === undefined || properties === null || typeof properties !== "object")
                continue;

            for (const [property, value] of Object.entries (properties)) {
                send (model, property, value);
                controls.get (keyOf (model, property))?.update (value);
                markModified (model, property);
            }
        }
    });

    element ("modelsSave").addEventListener ("click", () => {
        if (fileValues === null || tool === undefined)
            return;

        // What the tool does not read, as the ranges, is kept as the file has it
        const saved = {};

        for (const [name, value] of Object.entries (fileValues)) {
            if (isMeta (name))
                saved[name] = value;
        }

        Object.assign (saved, JSON.parse (tool.ccall ("yuiPreviewModels", "string", [], [])));
        vscode.postMessage ({ type: "saveModels", text: formatJson (saved, "") + "\\n" });
    });

    // ------------------------------------------------------------ messages

    window.addEventListener ("message", event => {
        const message = event.data;

        if (message.type === "state")
            applyState (message);
        else if (message.type === "show")
            show (message);
        else if (message.type === "note" && ! failed)
            showNote (message.text);
    });

    target.addEventListener ("change", () => choose ("target", target.value));
    host.addEventListener ("change", () => choose ("host", host.value));
    size.addEventListener ("change", () => choose ("size", size.value === "custom" ? typedSize () || "640x400" : size.value));
    theme.addEventListener ("change", () => choose ("theme", theme.value));
    zoom.addEventListener ("change", () => choose ("zoom", zoom.value));
    element ("refresh").addEventListener ("click", () => vscode.postMessage ({ type: "refresh" }));

    for (const field of [width, height]) {
        field.addEventListener ("change", () => {
            const typed = typedSize ();

            if (typed !== undefined)
                choose ("size", typed);
        });
    }

    // The keys go to the interface once it is clicked, and to the toolbar otherwise
    canvas.addEventListener ("pointerdown", () => canvas.focus ());

    // The space left for the interface, which the scroll bars of the content never change
    function resized () {
        panel.width = Math.max (1, viewport.clientWidth - ${2 * contentPadding});
        panel.height = Math.max (1, viewport.clientHeight - ${2 * contentPadding});
        layout ();
    }

    new ResizeObserver (resized).observe (viewport);
    showModels ((vscode.getState () || {}).modelsOpen === true);

    if (module !== null)
        start ();

    vscode.postMessage ({ type: "ready" });
}) ();
</script>
</body>
</html>`;
}
