import pageScript from "../page/preview.page.js";

/** The zoom levels of the preview. 'fit' scales what is shown to the panel, up or down. */
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

// How many times larger than its size 'fit' shows something small, such as the knob a drawing paints
const maximumFit = 8;

// The icons of the toolbar, each drawn in a square of 16 with lines of the color of the text
const icons = {
    light: `<circle cx="8" cy="8" r="3"/><path d="M8 1.5v1.5M8 13v1.5M1.5 8H3M13 8h1.5M3.4 3.4l1.1 1.1M11.5 11.5l1.1 1.1M3.4 12.6l1.1-1.1M11.5 4.5l1.1-1.1"/>`,
    dark: `<path d="M14 8.5A6 6 0 1 1 7.5 2a4.7 4.7 0 0 0 6.5 6.5z"/>`,
    models: `<path d="M2 4h5M10 4h4M2 8h2M7 8h7M2 12h6M11 12h3"/><circle cx="8.5" cy="4" r="1.5"/><circle cx="5.5" cy="8" r="1.5"/><circle cx="9.5" cy="12" r="1.5"/>`,
    guides: `<rect x="5" y="5" width="6" height="6"/><path d="M1.5 8H5M11 8h3.5M8 1.5V5M8 11v3.5"/>`,
    stats: `<path d="M1.5 8h3l2-5 3 10 2-5h3"/>`,
    refresh: `<path d="M15.3 2.7v4h-4"/><path d="M13.7 10a6 6 0 1 1-1.4-6.2l3 2.9"/>`
};

function icon (shapes: string): string {
    return `<svg viewBox="0 0 16 16" aria-hidden="true">${shapes}</svg>`;
}

/**
    The page of the preview panel: a toolbar and, under it, the interface with its problems.
    Between them, when asked for, a few lines tell how the interface is drawn.

    The page is loaded once and runs the tool, which shows the interface in a canvas. It says when it
    is ready and what is chosen in the toolbar, and is told what the toolbar offers and what to show:
    the files of a document, and what of it.
*/
export function previewPage (nonce: string, module: PreviewModule | undefined): string {
    const sizes = sizePresets.map (size => `<option value="${size}">${size.replace ("x", " x ")}</option>`).join ("");
    const zooms = zoomLevels.map (zoom => `<option value="${zoom}">${zoom === "fit" ? "Fit" : `${zoom}%`}</option>`).join ("");
    const scripts = module === undefined ? `'nonce-${nonce}'` : `'nonce-${nonce}' 'wasm-unsafe-eval' ${module.source}`;
    const requests = module === undefined ? "" : ` connect-src ${module.source};`;

    // What the script of the page is set up with, written before it: where the tool is, if it is there, what the
    // script loading it is allowed by, and what the page and its script both count with
    const settings = { module: module === undefined ? null : { script: module.script, folder: module.folder }, nonce, maximumSize, maximumFit, contentPadding };

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
    #toolbar .group {
        display: inline-flex;
        gap: 2px;
    }
    button.icon {
        display: inline-flex;
        align-items: center;
        justify-content: center;
        width: 26px;
        padding: 0;
    }
    button.icon svg {
        width: 16px;
        height: 16px;
        fill: none;
        stroke: currentColor;
        stroke-width: 1.3;
        stroke-linecap: round;
        stroke-linejoin: round;
    }
    /* Always as high, and cut where the panel ends: what it tells must not move the interface it tells about */
    #stats {
        flex: none;
        height: 4.2em;
        margin: 0;
        padding: 4px ${contentPadding}px;
        overflow: hidden;
        border-bottom: 1px solid var(--vscode-editorWidget-border, transparent);
        font-family: var(--vscode-editor-font-family);
        font-size: 11px;
        line-height: 1.4;
        opacity: 0.85;
        text-overflow: ellipsis;
        white-space: pre;
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
    <label id="shownAsLabel" title="The component of the interface it is shown as, or the type of the one it is shown alone as" hidden><span>as</span><select id="shownAs"></select></label>
    <label title="Auto is the size the root asks for or else the one of the panel, Panel always follows the panel"><span>Size</span><select id="size"><option value="auto">Auto</option><option value="panel">Panel</option>${sizes}<option value="custom">Custom</option></select></label>
    <div class="fields" title="The size in logical pixels, type one to fix it">
        <input id="width" type="number" min="1" max="${maximumSize}" aria-label="Width">
        <span>x</span>
        <input id="height" type="number" min="1" max="${maximumSize}" aria-label="Height">
    </div>
    <label title="Fit scales what is shown to the panel, a percentage shows it at that scale"><span>Zoom</span><select id="zoom">${zooms}</select></label>
    <div class="group" role="group" aria-label="Theme">
        <button id="themeLight" class="icon" aria-label="Light theme" title="Light theme. Click it again to follow the color theme of the editor">${icon (icons.light)}</button>
        <button id="themeDark" class="icon" aria-label="Dark theme" title="Dark theme. Click it again to follow the color theme of the editor">${icon (icons.dark)}</button>
    </div>
    <button id="modelsToggle" class="icon" aria-label="Models" title="Models: show the values of the models the interface binds to, to fake them">${icon (icons.models)}</button>
    <button id="guidesToggle" class="icon" aria-label="Guides" title="Guides: show the bounds of the components clicked here, or of the one the caret of the editor is in, with their paint margin and their distances to what holds them. While they show, a click picks a component, with the shift key for several">${icon (icons.guides)}</button>
    <button id="statsToggle" class="icon" aria-label="Statistics" title="Statistics: tell how the interface is drawn, the frames a second, the time painting takes and the components taking the longest. Measuring costs a little while it is on">${icon (icons.stats)}</button>
    <button id="refresh" class="icon" aria-label="Refresh" title="Refresh: show the document again">${icon (icons.refresh)}</button>
</div>
<pre id="stats" hidden></pre>
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
const settings = ${JSON.stringify (settings)};
${pageScript}
</script>
</body>
</html>`;
}
