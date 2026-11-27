// @ts-check

// The script of the page of the preview panel. previewPage.ts writes it in the page as it is here, after `settings`,
// the values it sets the page up with. What else it finds there is declared in globals.d.ts, for it to be checked.

(function () {
    const vscode = acquireVsCodeApi ();

    /** @param {string} id */
    const element = id => document.getElementById (id);
    const target = /** @type {HTMLSelectElement} */ (element ("target"));
    const shownAs = /** @type {HTMLSelectElement} */ (element ("shownAs"));
    const size = /** @type {HTMLSelectElement} */ (element ("size"));
    const width = /** @type {HTMLInputElement} */ (element ("width"));
    const height = /** @type {HTMLInputElement} */ (element ("height"));
    const zoom = /** @type {HTMLSelectElement} */ (element ("zoom"));

    // Each theme has its button, and neither is pressed while the theme follows the one of the editor
    const themes = { light: element ("themeLight"), dark: element ("themeDark") };
    const viewport = element ("viewport");
    const stage = element ("stage");
    const canvas = /** @type {HTMLCanvasElement} */ (element ("canvas"));
    const note = element ("note");
    const problems = element ("problems");
    const module = settings.module;

    /**
        @param {string} name
        @param {string} value
    */
    const choose = (name, value) => vscode.postMessage ({ type: "option", name: name, value: value });

    /**
        Shows a button as pressed or not, for those that turn something on.

        @param {HTMLElement} button
        @param {boolean} on
    */
    function press (button, on) {
        button.classList.toggle ("on", on);
        button.setAttribute ("aria-pressed", String (on));
    }

    /**
        Fills a list with its choices, those of a group under its name.

        @param {HTMLSelectElement} select
        @param {Choice[]} items
        @param {string} selected
    */
    function fill (select, items, selected) {
        const signature = JSON.stringify (items);

        if (select.dataset.items !== signature) {
            select.dataset.items = signature;
            select.replaceChildren ();

            /** @type {HTMLOptGroupElement} */
            let group;

            for (const item of items) {
                const option = document.createElement ("option");
                option.value = item.id;
                option.textContent = item.label;

                if (item.group !== undefined && (group === undefined || group.label !== item.group)) {
                    group = document.createElement ("optgroup");
                    group.label = item.group;
                    select.appendChild (group);
                }

                (item.group !== undefined ? group : select).appendChild (option);
            }
        }

        select.value = selected;
    }

    function typedSize () {
        const columns = Math.round (Number (width.value));
        const rows = Math.round (Number (height.value));

        return columns >= 1 && columns <= settings.maximumSize && rows >= 1 && rows <= settings.maximumSize ? columns + "x" + rows : undefined;
    }

    /**
        @param {string | number} columns
        @param {string | number} rows
    */
    function showSize (columns, rows) {
        if (document.activeElement !== width)
            width.value = String (columns);

        if (document.activeElement !== height)
            height.value = String (rows);
    }

    /**
        @param {HTMLElement} node
        @param {string} text
    */
    function showText (node, text) {
        node.textContent = text;
        node.hidden = text.length === 0;
    }

    // The types a drawing can be shown alone as, and what was chosen for what is shown: a use of it or one of them
    /** @type {string[]} */
    let hosts = [];
    let chosenAs = "";

    /** @type {Map<string, Use[]>} */
    const knownUses = new Map ();

    /** @param {StateMessage} state */
    function applyState (state) {
        fill (target, state.targets, state.target);
        target.disabled = state.targets.length === 0;

        hosts = state.hosts;
        chosenAs = state.shownAs;

        size.value = state.size;

        // A fixed size that is none of the listed ones
        if (size.selectedIndex < 0)
            size.value = "custom";

        if (/^[0-9]+x[0-9]+$/.test (state.size))
            showSize (state.size.split ("x")[0], state.size.split ("x")[1]);

        for (const [name, button] of Object.entries (themes))
            press (button, state.theme === name);

        zoom.value = state.zoom;
    }

    /**
        What is shown can be seen where the interface uses it, or alone as a type of component. The places it is
        used at are only known while one of them is shown, so they are kept for when it stands alone.

        @param {ShowMessage} message
        @param {Outcome} outcome
    */
    function showChoicesOfAs (message, outcome) {
        const key = message.main + " " + target.value;
        const alone = chosenAs.startsWith ("host:");

        if (outcome.inContext)
            knownUses.set (key, outcome.uses);

        const uses = outcome.inContext ? outcome.uses : (alone ? knownUses.get (key) || [] : []);
        const items = uses.map (use => ({ id: "use:" + use.path, label: use.type + (use.id ? " #" + use.id : (use.path ? " " + use.path : "")), group: "In the interface" }));

        if (alone && uses.length === 0)
            items.push ({ id: "use:", label: "Where it is used", group: "In the interface" });

        for (const host of hosts)
            items.push ({ id: "host:" + host, label: host, group: "On its own, as" });

        fill (shownAs, items, outcome.inContext ? "use:" + outcome.use : (alone ? chosenAs : "host:" + (hosts[0] || "")));
        element ("shownAsLabel").hidden = items.length < 2;
    }

    // What is listed under the interface: the problems of the document shown, then why the tool could not draw a
    // scene of it, which it only knows once it drew
    /** @type {string[]} */
    let told = [];
    let undrawn = "";

    function listProblems () {
        const scene = undrawn.length > 0 ? ["error: a Scene3D can't be drawn by the preview, which draws with WebGL 2: " + undrawn] : [];
        showText (problems, told.concat (scene).join ("\n"));
    }

    /**
        Shows a note in place of the interface.

        @param {string} text
    */
    function showNote (text) {
        shown = undefined;
        element ("shownAsLabel").hidden = true;
        stage.classList.add ("idle");
        showText (note, text);
        told = [];
        undrawn = "";
        listProblems ();
    }

    // ---------------------------------------------------------------- the tool

    // The space left for the interface, in logical pixels, and the pixels the display had for one of them then
    const panel = { width: 1, height: 1, ratio: 0 };

    // The part of the interface that is shown, in its logical pixels, and how many times larger it is seen
    let frame = { x: 0, y: 0, width: 1, height: 1 };
    let scale = 1;

    // The tool draws as many pixels for one of the page as the display is said to have. Saying it has more of
    // them is how the preview zooms: what is shown is drawn at the size it is seen at, not stretched to it
    const displayRatio = (() => {
        for (let holder = window; holder !== null; holder = Object.getPrototypeOf (holder)) {
            const descriptor = Object.getOwnPropertyDescriptor (holder, "devicePixelRatio");

            if (descriptor !== undefined)
                return descriptor.get !== undefined ? () => descriptor.get.call (window) : () => descriptor.value;
        }

        return () => 1;
    }) ();

    Object.defineProperty (window, "devicePixelRatio", { configurable: true, get: () => displayRatio () * scale });

    /** @type {YuiTool} */
    let tool;
    let failed = false;
    let failure = "";

    /** @type {ShowMessage} */
    let waiting;

    /** @type {AgentCallMessage[]} */
    const agentCalls = [];

    // The files given before the tool started, in the order they came: what shows them and the calls of
    // agents both give some, and the last one given of a file is the one that counts
    /** @type {ToolFile[]} */
    const earlyFiles = [];

    /** @type {{ message: ShowMessage, report: Outcome }} */
    let shown;

    /** @type {Outcome} */
    let report;
    let collecting = false;

    /** @type {string[]} */
    let errors = [];

    /** @param {unknown} reason */
    function fail (reason) {
        if (failed)
            return;

        failed = true;
        failure = "the preview can't run here: " + reason;
        tool = undefined;
        waiting = undefined;
        showNote ("The preview can't run here: " + reason + ".");
        vscode.postMessage ({ type: "failed", reason: String (reason) });

        for (const call of agentCalls.splice (0))
            agentCall (call);
    }

    // The files the tool read for what it just did without having been given them
    function missingFiles () {
        return JSON.parse (tool.ccall ("yuiToolMissingFiles", "string", [], []));
    }

    /**
        Gives the tool the files of its host, at the paths it names them with.

        @param {ToolFile[]} files
    */
    function writeFiles (files) {
        for (const file of files) {
            if (file.removed === true) {
                tool.ccall ("yuiToolRemoveFile", null, ["string"], [file.path]);
                continue;
            }

            const pointer = tool._yuiToolAllocate (file.data.length);
            new Uint8Array (tool.wasmMemory.buffer, pointer, file.data.length).set (file.data);
            tool.ccall ("yuiToolWriteFile", null, ["string"], [file.path]);
        }
    }

    /**
        Runs a tool of an agent, which the page is asked for when the tool draws. Those asked before the tool
        started wait for it.

        @param {AgentCallMessage} message
    */
    function agentCall (message) {
        if (failed) {
            vscode.postMessage ({ type: "agentResult", id: message.id, error: failure });
            return;
        }

        if (tool === undefined) {
            earlyFiles.push (...message.files);
            agentCalls.push (Object.assign ({}, message, { files: [] }));
            return;
        }

        try {
            writeFiles (message.files);

            const result = tool.ccall ("yuiAgentCall", "string", ["string", "string"], [message.name, message.arguments]);
            vscode.postMessage ({ type: "agentResult", id: message.id, result: result, missing: missingFiles () });
        }
        catch (error) {
            vscode.postMessage ({ type: "agentResult", id: message.id, error: String (error) });
        }
    }

    function start () {
        if (document.createElement ("canvas").getContext ("webgl2") === null) {
            fail ("WebGL 2 is not available in this window");
            return;
        }

        /** @type {YuiToolOptions} */
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
            yuiPreviewFramed: (x, y, width, height) => {
                frame = { x: x, y: y, width: width, height: height };
                scale = scaleOf (width, height);
                return scale;
            },
            yuiPreviewModelChanged: (model, property, value) => modelChanged (model, property, value),
            yuiPreviewDrawn: problem => {
                undrawn = problem;
                listProblems ();
            },
            yuiPreviewReady: () => {
                tool = /** @type {YuiTool} */ (options);

                try {
                    writeFiles (earlyFiles.splice (0));
                    tool._yuiPreviewSetGuides (guidesOn ? 1 : 0);
                    tool._yuiPreviewSetCaret (caret.line, caret.column);
                    tool._yuiPreviewSetStats (statsOn ? 1 : 0);
                }
                catch (error) {
                    fail (error);
                    return;
                }

                if (waiting !== undefined)
                    show (waiting);

                for (const call of agentCalls.splice (0))
                    agentCall (call);
            }
        };

        const script = document.createElement ("script");
        script.nonce = settings.nonce;
        script.src = module.script;
        script.addEventListener ("load", () => createYuiTool (options).catch (fail));
        script.addEventListener ("error", () => fail ("the tool could not be loaded"));
        document.body.appendChild (script);
    }

    /**
        How many times larger than its size a part of the interface is seen: what fits it in the panel, or the zoom.

        @param {number} width
        @param {number} height
    */
    function scaleOf (width, height) {
        const zoom = shown.message.zoom;
        const wanted = zoom === "fit" ? Math.min (settings.maximumFit, panel.width / width, panel.height / height) : Number (zoom) / 100;

        // A canvas has no more than so many pixels a side, and what nearly fits is shown as it is
        const most = settings.maximumSize / (Math.max (width, height) * displayRatio ());
        return Math.abs (wanted - 1) < 0.01 ? Math.min (1, most) : Math.max (0.05, Math.min (wanted, most));
    }

    // Gives the interface its size, and the canvas the size the part of it that is shown is seen at
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
        else if (message.size === "panel" || message.interface || outcome.inContext)
            [columns, rows] = fitted;

        // The tool tells the part shown before it sizes its window, which is when it reads how sharp to draw
        tool._yuiPreviewSetView (columns, rows);

        const width = Math.max (1, Math.round (frame.width * scale));
        const height = Math.max (1, Math.round (frame.height * scale));

        canvas.style.width = width + "px";
        canvas.style.height = height + "px";
        stage.style.width = width + "px";
        stage.style.height = height + "px";
        showSize (columns, rows);
    }

    /** @param {ShowMessage} message */
    function show (message) {
        if (failed)
            return;

        if (tool === undefined) {
            earlyFiles.push (...message.files);
            waiting = Object.assign ({}, message, { files: [] });
            showNote ("Starting the preview...");
            return;
        }

        waiting = undefined;

        /** @type {string[]} */
        let missing = [];

        try {
            writeFiles (message.files);

            report = undefined;
            errors = [];
            collecting = true;
            tool.ccall ("yuiPreviewShow", null, ["string", "string", "string", "string", "string", "string", "number"],
                        [message.main, message.drawing, message.palette, message.host, message.use, message.models, message.light ? 1 : 0]);
            missing = missingFiles ();
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
        /** @param {string} path */
        const named = path => path.replace (/^([/]yui[/])?[0-9]+[/]/, "");
        const lines = report.problems.map (problem => named (problem.file) + (problem.line > 0 ? ":" + problem.line + ":" + problem.column : "") + ": " + problem.severity + ": " + problem.message);

        if (report.needsHost)
            lines.push ("hint: this drawing reads the properties of a widget, choose the type it is made for next to 'as'");

        // A document that stops loading keeps the interface it had, with its problems listed under it, after what
        // its host could not give the tool
        shown = report.hasInterface ? { message: message, report: report } : undefined;
        stage.classList.toggle ("idle", ! report.hasInterface);
        showText (note, report.status === "nothing" ? "Nothing to preview: " + report.message + "." : "");
        told = (message.notes || []).map (text => "warning: " + text).concat (lines, errors.map (named));
        undrawn = "";
        listProblems ();
        showChoicesOfAs (message, report);
        layout ();

        // The host gives the files the tool read without having them, when it has them, and shows the document again
        if (missing.length > 0)
            vscode.postMessage ({ type: "missing", main: message.main, files: missing });

        liveValues = report.models || {};
        fileValues = message.modelsValues !== null && typeof message.modelsValues === "object" ? message.modelsValues : null;
        modelsFile.textContent = message.modelsFile || "";
        modelsFile.title = message.modelsFile || "";
        buildModels ();
    }

    // ------------------------------------------------------------ guides

    // The tool draws the guides over the interface, and picks the components clicked while they show
    const guidesToggle = element ("guidesToggle");
    let guidesOn = false;
    let caret = { line: 0, column: 0 };

    /** @param {boolean} on */
    function showGuides (on) {
        guidesOn = on;
        press (guidesToggle, on);
        vscode.setState (Object.assign ({}, vscode.getState () || {}, { guidesOn: on }));

        if (tool !== undefined)
            tool._yuiPreviewSetGuides (on ? 1 : 0);
    }

    /**
        The component the caret of the editor is in is the one shown until others are clicked.

        @param {{ line: number, column: number }} message
    */
    function caretMoved (message) {
        caret = { line: message.line, column: message.column };

        if (tool !== undefined)
            tool._yuiPreviewSetCaret (caret.line, caret.column);
    }

    guidesToggle.addEventListener ("click", () => showGuides (! guidesOn));

    // ------------------------------------------------------------ statistics

    // The tool measures how the interface is painted while this is on, and tells it in a few lines
    const statsToggle = element ("statsToggle");
    const stats = element ("stats");
    let statsOn = false;

    /** @type {number} */
    let statsTimer;

    function readStats () {
        if (tool !== undefined && ! failed)
            stats.textContent = tool.ccall ("yuiPreviewStats", "string", [], []);
    }

    /** @param {boolean} on */
    function showStats (on) {
        statsOn = on;
        press (statsToggle, on);
        stats.hidden = ! on;
        vscode.setState (Object.assign ({}, vscode.getState () || {}, { statsOn: on }));
        clearInterval (statsTimer);

        if (tool !== undefined)
            tool._yuiPreviewSetStats (on ? 1 : 0);

        // Once a second, which is as often as the frames a second are counted
        if (on) {
            readStats ();
            statsTimer = setInterval (readStats, 1000);
        }
    }

    statsToggle.addEventListener ("click", () => showStats (! statsOn));

    // ------------------------------------------------------------ models

    // The values the interface sees, the ones of the file and the controls showing them, by model and property
    /** @type {Record<string, Record<string, any>>} */
    let liveValues = {};

    /** @type {Record<string, any>} */
    let fileValues = null;

    /** @type {string} */
    let modelsSignature;

    /** @type {Map<string, { row: HTMLElement, update: (value: any) => void }>} */
    const controls = new Map ();
    const modelsPanel = element ("models");
    const modelsBody = element ("modelsBody");
    const modelsFile = element ("modelsFile");
    const modelsToggle = element ("modelsToggle");

    /** @param {any} value */
    const kindOf = value => Array.isArray (value) ? "array" : (value === null ? "json" : (["number", "boolean", "string"].includes (typeof value) ? typeof value : "json"));

    /**
        @param {string} model
        @param {string} property
    */
    const keyOf = (model, property) => model + "\u0000" + property;

    /** @param {string} name */
    const isMeta = name => name.startsWith ("$");

    /** @param {number} value */
    const rounded = value => Math.round (value * 1e6) / 1e6;

    /**
        Arrays of plain values stay on one line, so long series remain readable.

        @param {any} value
        @param {string} indent
        @returns {string}
    */
    function formatJson (value, indent) {
        const inner = indent + "    ";

        if (Array.isArray (value)) {
            if (value.every (item => item === null || typeof item !== "object"))
                return "[" + value.map (item => formatJson (item, inner)).join (", ") + "]";

            return value.length === 0 ? "[]" : "[\n" + value.map (item => inner + formatJson (item, inner)).join (",\n") + "\n" + indent + "]";
        }

        if (value !== null && typeof value === "object") {
            const entries = Object.entries (value);
            return entries.length === 0 ? "{}" : "{\n" + entries.map (([name, item]) => inner + JSON.stringify (name) + ": " + formatJson (item, inner)).join (",\n") + "\n" + indent + "}";
        }

        return typeof value === "number" ? String (rounded (value)) : JSON.stringify (value);
    }

    /**
        @param {string} model
        @param {string} property
        @param {any} value
    */
    function send (model, property, value) {
        if (tool === undefined)
            return;

        liveValues[model][property] = value;
        tool.ccall ("yuiPreviewSetModelValue", "number", ["string", "string", "string"], [model, property, JSON.stringify (value)]);
        markModified (model, property);
    }

    /**
        @param {string} model
        @param {string} property
    */
    function markModified (model, property) {
        const control = controls.get (keyOf (model, property));
        const original = fileValues !== null && fileValues[model] !== undefined ? fileValues[model][property] : undefined;

        if (control !== undefined)
            control.row.classList.toggle ("modified", formatJson (liveValues[model][property], "") !== formatJson (original, ""));
    }

    /**
        @param {string} model
        @param {string} property
        @param {any} value
    */
    function modelChanged (model, property, value) {
        if (liveValues[model] === undefined)
            return;

        liveValues[model][property] = value;
        controls.get (keyOf (model, property))?.update (value);
        markModified (model, property);
    }

    /**
        The range of a number, from "$ranges" in the file ([minimum, maximum, step]) or else guessed from its value.

        @param {string} model
        @param {string} property
        @param {number} value
    */
    function rangeOf (model, property, value) {
        const declared = fileValues !== null && fileValues["$ranges"] && fileValues["$ranges"][model] ? fileValues["$ranges"][model][property] : undefined;

        if (Array.isArray (declared) && declared.length >= 2)
            return { minimum: declared[0], maximum: declared[1], step: declared[2] > 0 ? declared[2] : "any" };

        const whole = Number.isInteger (value) && Math.abs (value) > 1;
        return { minimum: Math.min (0, value * 2), maximum: Math.max (1, value * 2), step: whole ? 1 : "any" };
    }

    /**
        @param {string} model
        @param {string} property
        @param {number} value
        @param {HTMLElement} control
        @returns {(value: any) => void}
    */
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

    /**
        @param {string} model
        @param {string} property
        @param {string} kind
        @param {HTMLElement} control
        @returns {(value: any) => void}
    */
    function textControl (model, property, kind, control) {
        const field = document.createElement (kind === "string" ? "input" : "textarea");

        if (kind === "string")
            /** @type {HTMLInputElement} */ (field).type = "text";

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

    /**
        @param {string} model
        @param {string} property
        @param {HTMLElement} control
        @returns {(value: any) => void}
    */
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

    /** @param {boolean} open */
    function showModels (open) {
        modelsPanel.hidden = ! open;
        press (modelsToggle, open);
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
        /** @type {Record<string, any>} */
        const saved = {};

        for (const [name, value] of Object.entries (fileValues)) {
            if (isMeta (name))
                saved[name] = value;
        }

        Object.assign (saved, JSON.parse (tool.ccall ("yuiPreviewModels", "string", [], [])));
        vscode.postMessage ({ type: "saveModels", text: formatJson (saved, "") + "\n" });
    });

    // ------------------------------------------------------------ messages

    window.addEventListener ("message", event => {
        const message = event.data;

        if (message.type === "state")
            applyState (message);
        else if (message.type === "show")
            show (message);
        else if (message.type === "agentCall")
            agentCall (message);
        else if (message.type === "caret")
            caretMoved (message);
        else if (message.type === "note" && ! failed)
            showNote (message.text);
    });

    target.addEventListener ("change", () => choose ("target", target.value));
    shownAs.addEventListener ("change", () => choose ("shownAs", shownAs.value));
    size.addEventListener ("change", () => choose ("size", size.value === "custom" ? typedSize () || "640x400" : size.value));
    zoom.addEventListener ("change", () => choose ("zoom", zoom.value));

    for (const [name, button] of Object.entries (themes))
        button.addEventListener ("click", () => choose ("theme", button.classList.contains ("on") ? "auto" : name));
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

    // The interface answers the right button itself, so the cut, copy and paste menu of the editor stays out of it
    canvas.addEventListener ("contextmenu", event => event.preventDefault ());

    // The space left for the interface, which the scroll bars of the content never change
    function resized () {
        const space = {
            width: Math.max (1, viewport.clientWidth - 2 * settings.contentPadding),
            height: Math.max (1, viewport.clientHeight - 2 * settings.contentPadding),
            ratio: displayRatio ()
        };

        // A panel changing size is told twice, by the window and by its viewport: the interface is drawn once
        if (space.width === panel.width && space.height === panel.height && space.ratio === panel.ratio)
            return;

        Object.assign (panel, space);
        layout ();
    }

    new ResizeObserver (resized).observe (viewport);

    // A window moved to another display, or zoomed, has other pixels for one of the page
    window.addEventListener ("resize", resized);

    showModels ((vscode.getState () || {}).modelsOpen === true);
    showGuides ((vscode.getState () || {}).guidesOn === true);
    showStats ((vscode.getState () || {}).statsOn === true);

    if (module !== null)
        start ();

    vscode.postMessage ({ type: "ready" });
}) ();
