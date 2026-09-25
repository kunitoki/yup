import * as vscode from "vscode";
import { YdspDeviceSelection, YdspPlayerStatus } from "./player";

export interface YdspChoice {
    label: string;
    value: string;
    detail?: string;
}

export interface YdspPatchItem {
    path: string;
    label: string;
    playing: boolean;
}

export interface YdspPanelDevices {
    audioTypes: YdspChoice[];
    audioOutputs: YdspChoice[];
    audioInputs: YdspChoice[];
    midiInputs: YdspChoice[];
    midiOutputs: YdspChoice[];
    error?: string;
}

/** The device lists that the panel renders as selects. */
export type YdspChoiceKey = "audioTypes" | "audioOutputs" | "audioInputs" | "midiInputs" | "midiOutputs";

export interface YdspPanelState {
    status: YdspPlayerStatus;
    playing?: string;
    playingLabel?: string;
    patches: YdspPatchItem[];
    unsaved: string[];
    devices: YdspPanelDevices;
    selection: YdspDeviceSelection;
    sampleRate: number;
    blockSize: number;
    verbose: boolean;
    follow: boolean;
}

export type YdspSettingKey = "sampleRate" | "blockSize" | "verbose" | "followActiveEditor";

/** Messages sent from the sidebar webview to the extension. */
export type WebviewMessage =
    | { type: "ready" }
    | { type: "run"; path?: string }
    | { type: "stop" }
    | { type: "restart" }
    | { type: "refreshDevices" }
    | { type: "selection"; key: keyof YdspDeviceSelection; value: string }
    | { type: "setting"; key: YdspSettingKey; value: number | boolean }
    | { type: "testNote"; value: number };

/** Messages sent from the extension to the sidebar webview. */
export interface HostMessage {
    type: "state";
    state: YdspPanelState;
}

function isWebviewMessage (message: unknown): message is WebviewMessage {
    return typeof message === "object"
        && message !== null
        && typeof (message as { type?: unknown }).type === "string";
}

/** Owns the YDSP sidebar webview and its message plumbing. */
export class YdspPanelProvider implements vscode.WebviewViewProvider {
    static readonly viewId = "ydsp.panel";

    constructor (private readonly extensionUri: vscode.Uri,
                 private readonly handle: (message: WebviewMessage) => void) {
    }

    post (message: HostMessage): void {
        void this.view?.webview.postMessage (message);
    }

    resolveWebviewView (view: vscode.WebviewView): void {
        this.view = view;
        view.webview.options = { enableScripts: true, localResourceRoots: [this.extensionUri] };
        view.webview.html = renderPanel (view.webview);
        view.webview.onDidReceiveMessage ((message: unknown) => {
            if (isWebviewMessage (message))
                this.handle (message);
        });
        view.onDidChangeVisibility (() => {
            if (view.visible)
                this.handle ({ type: "ready" });
        });
        view.onDidDispose (() => { this.view = undefined; });
    }

    private view: vscode.WebviewView | undefined;
}

function createNonce (): string {
    const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    let nonce = "";

    for (let index = 0; index < 32; ++index)
        nonce += alphabet.charAt (Math.floor (Math.random () * alphabet.length));

    return nonce;
}

function renderPanel (webview: vscode.Webview): string {
    const nonce = createNonce ();

    return `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src ${webview.cspSource} 'nonce-${nonce}'; script-src 'nonce-${nonce}';">
<style nonce="${nonce}">
* {
    box-sizing: border-box;
}
body {
    font-family: var(--vscode-font-family);
    font-size: var(--vscode-font-size);
    color: var(--vscode-foreground);
    padding: 10px 12px 18px;
    margin: 0;
}
[hidden] {
    display: none !important;
}
.status {
    display: flex;
    align-items: center;
    gap: 6px;
    min-height: 16px;
    font-size: 11px;
    color: var(--vscode-descriptionForeground);
}
.status-dot {
    flex: none;
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--vscode-descriptionForeground);
    opacity: 0.5;
}
.status.playing {
    color: var(--vscode-foreground);
}
.status.playing .status-dot {
    background: var(--vscode-charts-green, #89d185);
    opacity: 1;
}
.status.stopping .status-dot {
    background: var(--vscode-charts-orange, #e2c08d);
    opacity: 1;
}
.tabs {
    display: flex;
    gap: 4px;
    padding: 3px;
    margin: 10px 0 14px;
    border-radius: 8px;
    border: 1px solid var(--vscode-widget-border, rgba(128, 128, 128, 0.2));
    background: var(--vscode-editorWidget-background, rgba(128, 128, 128, 0.12));
}
.tab {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    padding: 6px 8px;
    border: none;
    border-radius: 6px;
    background: none;
    color: var(--vscode-foreground);
    font: inherit;
    opacity: 0.7;
    cursor: pointer;
}
.tab:hover {
    background: var(--vscode-list-hoverBackground);
    opacity: 1;
}
.tab.active {
    background: var(--vscode-button-background);
    color: var(--vscode-button-foreground);
    font-weight: 600;
    opacity: 1;
}
.icon {
    flex: none;
    width: 14px;
    height: 14px;
}
.transport {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 6px;
    margin-bottom: 14px;
}
.button {
    border: none;
    border-radius: 5px;
    padding: 5px 12px;
    font: inherit;
    cursor: pointer;
    background: var(--vscode-button-secondaryBackground, rgba(128, 128, 128, 0.2));
    color: var(--vscode-button-secondaryForeground, var(--vscode-foreground));
}
.button:hover {
    background: var(--vscode-button-secondaryHoverBackground, rgba(128, 128, 128, 0.32));
}
.button.primary {
    background: var(--vscode-button-background);
    color: var(--vscode-button-foreground);
}
.button.primary:hover {
    background: var(--vscode-button-hoverBackground);
}
.button.small {
    padding: 3px 10px;
    font-size: 11px;
}
.button:disabled {
    opacity: 0.4;
    cursor: default;
}
.card {
    margin-bottom: 12px;
    padding: 10px 12px 12px;
    border: 1px solid var(--vscode-widget-border, rgba(128, 128, 128, 0.2));
    border-radius: 8px;
    background: var(--vscode-editorWidget-background, transparent);
}
.card-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    margin-bottom: 8px;
}
.card-title {
    font-size: 11px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: var(--vscode-descriptionForeground);
}
.list {
    list-style: none;
    margin: 0;
    padding: 0;
}
.patch {
    display: flex;
    align-items: center;
    gap: 8px;
    width: 100%;
    padding: 5px 8px;
    border: none;
    border-radius: 5px;
    background: none;
    color: inherit;
    font: inherit;
    text-align: left;
    cursor: pointer;
}
.patch:hover {
    background: var(--vscode-list-hoverBackground);
}
.patch .dot {
    flex: none;
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--vscode-descriptionForeground);
    opacity: 0.4;
}
.patch .label {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.patch.playing {
    background: var(--vscode-list-activeSelectionBackground);
    color: var(--vscode-list-activeSelectionForeground);
    font-weight: 600;
}
.patch.playing .dot {
    background: var(--vscode-charts-green, #89d185);
    opacity: 1;
}
.warning {
    margin: 6px 0 0;
    font-size: 11px;
    color: var(--vscode-editorWarning-foreground, var(--vscode-foreground));
}
.hint {
    margin: 8px 0 0;
    font-size: 11px;
    color: var(--vscode-descriptionForeground);
}
label {
    display: block;
    margin: 8px 0 0;
    font-size: 11px;
    color: var(--vscode-descriptionForeground);
}
label.check {
    display: flex;
    align-items: center;
    gap: 6px;
    color: var(--vscode-foreground);
    font-size: inherit;
}
.row {
    display: flex;
    align-items: center;
    gap: 6px;
}
select, input[type="number"] {
    width: 100%;
    margin-top: 3px;
    padding: 3px 6px;
    border: 1px solid var(--vscode-input-border, transparent);
    border-radius: 5px;
    background: var(--vscode-input-background);
    color: var(--vscode-input-foreground);
    font: inherit;
}
select:focus, input[type="number"]:focus {
    outline: 1px solid var(--vscode-focusBorder);
    outline-offset: -1px;
}
input[type="number"] {
    margin-top: 0;
    flex: 1;
}
label.check input {
    margin: 0;
}
</style>
</head>
<body>
<div id="status" class="status">
    <span class="status-dot"></span><span id="statusText">Idle</span>
</div>
<div id="unsaved" class="warning hidden"></div>

<div class="tabs" role="tablist">
    <button type="button" class="tab active" id="tab-performance" data-tab="performance" role="tab" aria-selected="true" aria-controls="view-performance">
        <svg class="icon" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true" focusable="false">
            <rect x="2" y="6" width="20" height="12" rx="2.5" opacity="0.35"/>
            <rect x="4.2" y="8.6" width="3" height="2.6" rx="0.8"/>
            <rect x="8.4" y="8.6" width="3" height="2.6" rx="0.8"/>
            <rect x="12.6" y="8.6" width="3" height="2.6" rx="0.8"/>
            <rect x="16.8" y="8.6" width="3" height="2.6" rx="0.8"/>
            <rect x="4.2" y="12.8" width="4.5" height="2.6" rx="0.8"/>
            <rect x="9.9" y="12.8" width="4.5" height="2.6" rx="0.8"/>
            <rect x="15.6" y="12.8" width="4.2" height="2.6" rx="0.8"/>
        </svg>
        <span>Performance</span>
    </button>
    <button type="button" class="tab" id="tab-settings" data-tab="settings" role="tab" aria-selected="false" aria-controls="view-settings">
        <svg class="icon" viewBox="0 0 24 24" fill="currentColor" aria-hidden="true" focusable="false">
            <rect x="10.8" y="1.3" width="2.4" height="4.6" rx="1.1"/>
            <rect x="10.8" y="1.3" width="2.4" height="4.6" rx="1.1" transform="rotate(45 12 12)"/>
            <rect x="10.8" y="1.3" width="2.4" height="4.6" rx="1.1" transform="rotate(90 12 12)"/>
            <rect x="10.8" y="1.3" width="2.4" height="4.6" rx="1.1" transform="rotate(135 12 12)"/>
            <rect x="10.8" y="1.3" width="2.4" height="4.6" rx="1.1" transform="rotate(180 12 12)"/>
            <rect x="10.8" y="1.3" width="2.4" height="4.6" rx="1.1" transform="rotate(225 12 12)"/>
            <rect x="10.8" y="1.3" width="2.4" height="4.6" rx="1.1" transform="rotate(270 12 12)"/>
            <rect x="10.8" y="1.3" width="2.4" height="4.6" rx="1.1" transform="rotate(315 12 12)"/>
            <path fill-rule="evenodd" d="M12 5.5a6.5 6.5 0 1 0 0 13 6.5 6.5 0 0 0 0-13Zm0 3.7a2.8 2.8 0 1 0 0 5.6 2.8 2.8 0 0 0 0-5.6Z"/>
        </svg>
        <span>Settings</span>
    </button>
</div>

<section id="view-performance" role="tabpanel" aria-labelledby="tab-performance">
    <div class="transport">
        <button type="button" id="run" class="button primary">Run</button>
        <button type="button" id="stop" class="button">Stop</button>
        <button type="button" id="restart" class="button">Restart</button>
    </div>

    <div class="card">
        <div class="card-header"><span class="card-title">Patches</span></div>
        <ul id="patches" class="list"></ul>
        <p id="noPatches" class="hint hidden">No .ydsp or .ydsp-project file in this workspace.</p>
    </div>
</section>

<section id="view-settings" role="tabpanel" aria-labelledby="tab-settings" hidden>
    <div class="card">
        <div class="card-header">
            <span class="card-title">Devices</span>
            <button type="button" id="refresh" class="button small">Refresh</button>
        </div>
        <div id="devicesError" class="warning hidden"></div>
        <label>Audio backend<select id="audioType"></select></label>
        <label>Audio output<select id="audioOutput"></select></label>
        <label>Audio input<select id="audioInput"></select></label>
        <label>MIDI input<select id="midiInput"></select></label>
        <label>MIDI output<select id="midiOutput"></select></label>
    </div>

    <div class="card">
        <div class="card-header"><span class="card-title">Options</span></div>
        <label>Sample rate<input id="sampleRate" type="number" min="0" step="1"></label>
        <label>Block size<input id="blockSize" type="number" min="0" step="1"></label>
        <label>Test note<span class="row"><input id="testNote" type="number" min="0" max="127" value="60"><button type="button" id="sendTestNote" class="button small">Send</button></span></label>
        <label class="check"><input id="verbose" type="checkbox">Verbose diagnostics</label>
        <label class="check"><input id="followActiveEditor" type="checkbox">Follow the active patch</label>
        <p class="hint">Devices and options apply on the next run. Changing them while a patch plays restarts the player. Saves are picked up by hot reload; unsaved edits are not heard.</p>
    </div>
</section>

<script nonce="${nonce}">
const api = acquireVsCodeApi ();
const deviceKeys = ["audioType", "audioOutput", "audioInput", "midiInput", "midiOutput"];
let latest = null;

function post (message) {
    api.postMessage (message);
}

function byId (id) {
    return document.getElementById (id);
}

function showTab (tab) {
    const restored = tab === "settings" ? "settings" : "performance";

    api.setState ({ tab: restored });

    for (const button of document.querySelectorAll (".tab")) {
        const active = button.dataset.tab === restored;
        button.classList.toggle ("active", active);
        button.setAttribute ("aria-selected", active ? "true" : "false");
    }

    byId ("view-performance").hidden = restored !== "performance";
    byId ("view-settings").hidden = restored !== "settings";
}

function fillSelect (id, choices, current) {
    const select = byId (id);

    if (document.activeElement === select) {
        return;
    }

    const effective = current === undefined || current === null ? "" : current;
    select.textContent = "";
    let matched = false;

    for (const choice of choices) {
        const option = document.createElement ("option");
        option.value = choice.value;
        option.textContent = choice.detail ? choice.label + " \u2014 " + choice.detail : choice.label;
        select.appendChild (option);

        if (choice.value === effective) {
            option.selected = true;
            matched = true;
        }
    }

    if (! matched && effective !== "") {
        const option = document.createElement ("option");
        option.value = effective;
        option.textContent = effective + " (not available)";
        option.selected = true;
        select.appendChild (option);
    }
}

function renderPatches (patches) {
    const list = byId ("patches");
    list.textContent = "";

    for (const patch of patches) {
        const item = document.createElement ("li");
        const button = document.createElement ("button");
        button.type = "button";
        button.className = patch.playing ? "patch playing" : "patch";

        if (patch.playing) {
            button.setAttribute ("aria-current", "true");
        }

        const dot = document.createElement ("span");
        dot.className = "dot";

        const label = document.createElement ("span");
        label.className = "label";
        label.textContent = patch.label;

        button.appendChild (dot);
        button.appendChild (label);
        button.addEventListener ("click", function () { post ({ type: "run", path: patch.path }); });

        item.appendChild (button);
        list.appendChild (item);
    }

    byId ("noPatches").classList.toggle ("hidden", patches.length > 0);
}

function setNumber (id, value) {
    const input = byId (id);

    if (document.activeElement !== input) {
        input.value = String (value);
    }
}

function render () {
    const state = latest;

    if (state === null) {
        return;
    }

    const status = byId ("status");
    status.className = "status " + state.status;

    if (state.status === "playing") {
        byId ("statusText").textContent = "Playing " + (state.playingLabel || state.playing || "");
    } else if (state.status === "stopping") {
        byId ("statusText").textContent = "Stopping...";
    } else {
        byId ("statusText").textContent = "Idle";
    }

    byId ("stop").disabled = state.status === "idle";
    byId ("restart").disabled = state.status === "idle";

    const unsaved = byId ("unsaved");

    if (state.unsaved.length > 0) {
        unsaved.textContent = "Unsaved: " + state.unsaved.join (", ") + ". Save to hear the changes.";
        unsaved.classList.remove ("hidden");
    } else {
        unsaved.classList.add ("hidden");
    }

    renderPatches (state.patches);

    const error = byId ("devicesError");

    if (state.devices.error) {
        error.textContent = "Cannot list devices: " + state.devices.error;
        error.classList.remove ("hidden");
    } else {
        error.classList.add ("hidden");
    }

    fillSelect ("audioType", state.devices.audioTypes, state.selection.audioType);
    fillSelect ("audioOutput", state.devices.audioOutputs, state.selection.audioOutput);
    fillSelect ("audioInput", state.devices.audioInputs, state.selection.audioInput);
    fillSelect ("midiInput", state.devices.midiInputs, state.selection.midiInput);
    fillSelect ("midiOutput", state.devices.midiOutputs, state.selection.midiOutput);

    setNumber ("sampleRate", state.sampleRate);
    setNumber ("blockSize", state.blockSize);
    byId ("verbose").checked = state.verbose;
    byId ("followActiveEditor").checked = state.follow;
}

for (const button of document.querySelectorAll (".tab")) {
    button.addEventListener ("click", function () { showTab (button.dataset.tab); });
}

byId ("run").addEventListener ("click", function () { post ({ type: "run" }); });
byId ("stop").addEventListener ("click", function () { post ({ type: "stop" }); });
byId ("restart").addEventListener ("click", function () { post ({ type: "restart" }); });
byId ("refresh").addEventListener ("click", function () { post ({ type: "refreshDevices" }); });

for (const key of deviceKeys) {
    byId (key).addEventListener ("change", function (event) {
        post ({ type: "selection", key: key, value: event.target.value });
    });
}

for (const key of ["sampleRate", "blockSize"]) {
    byId (key).addEventListener ("change", function (event) {
        const value = parseInt (event.target.value, 10);
        post ({ type: "setting", key: key, value: isNaN (value) || value < 0 ? 0 : value });
    });
}

byId ("verbose").addEventListener ("change", function (event) {
    post ({ type: "setting", key: "verbose", value: event.target.checked });
});

byId ("followActiveEditor").addEventListener ("change", function (event) {
    post ({ type: "setting", key: "followActiveEditor", value: event.target.checked });
});

byId ("sendTestNote").addEventListener ("click", function () {
    const value = parseInt (byId ("testNote").value, 10);

    if (! isNaN (value) && value >= 0 && value <= 127) {
        post ({ type: "testNote", value: value });
    }
});

window.addEventListener ("message", function (event) {
    const message = event.data;

    if (message && message.type === "state") {
        latest = message.state;
        render ();
    }
});

const restored = api.getState ();
showTab (restored && restored.tab ? restored.tab : "performance");

post ({ type: "ready" });
</script>
</body>
</html>
`;
}
