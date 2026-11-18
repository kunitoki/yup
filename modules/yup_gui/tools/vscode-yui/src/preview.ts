import * as crypto from "crypto";
import * as path from "path";
import * as vscode from "vscode";
import { collectDocumentFiles, withoutComments } from "./documentFiles";
import { maximumSize, PreviewModule, previewPage, zoomLevels } from "./previewPage";

export interface PreviewSchema {
    instantiableTypes (): string[];
}

export interface PreviewHost {
    log (message: string): void;
    schema (): Promise<PreviewSchema | undefined>;
    modelsPath (document: vscode.TextDocument): string | undefined;
}

/** Something a document can show: its interface, or a drawing, a palette or a component it declares. */
interface PreviewTarget {
    id: string;
    label: string;
    drawing: string;
    palette: string;
    component: string;
}

const rootTarget = "root";
const plainHost = "Component";
const themes = ["auto", "light", "dark"];
const typingDelayMs = 150;

/** A size of the preview: 'auto', 'panel' or a fixed '<width>x<height>' in logical pixels. */
function isSizeOption (value: string): boolean {
    return value === "auto" || value === "panel"
        || (/^[1-9][0-9]{0,3}x[1-9][0-9]{0,3}$/.test (value) && value.split ("x").every (length => Number (length) <= maximumSize));
}

function sizeOption (): string {
    const configured = (vscode.workspace.getConfiguration ("yui.preview").get<string> ("size") ?? "").trim ().toLowerCase ();
    return isSizeOption (configured) ? configured : "auto";
}

function themeOption (): string {
    const configured = vscode.workspace.getConfiguration ("yui.preview").get<string> ("theme") ?? "";
    return themes.includes (configured) ? configured : "auto";
}

function declarationsOf (text: string, keyword: string): string[] {
    const pattern = new RegExp (`^${keyword}\\s+([A-Za-z_][A-Za-z0-9_]*)\\s*[:{]`, "gm");
    const names: string[] = [];
    let match = pattern.exec (text);

    while (match !== null) {
        names.push (match[1]);
        match = pattern.exec (text);
    }

    return names;
}

function hasRootComponent (text: string): boolean {
    return /^[A-Z][A-Za-z0-9_]*\s*\{/m.test (text);
}

/** What the text of a document can show, its interface first. */
function targetsOf (text: string): PreviewTarget[] {
    const targets: PreviewTarget[] = [];

    if (hasRootComponent (text))
        targets.push ({ id: rootTarget, label: "Interface", drawing: "", palette: "", component: "" });

    for (const name of declarationsOf (text, "drawing"))
        targets.push ({ id: `drawing:${name}`, label: `Drawing: ${name}`, drawing: name, palette: "", component: "" });

    for (const name of declarationsOf (text, "palette"))
        targets.push ({ id: `palette:${name}`, label: `Palette: ${name}`, drawing: "", palette: name, component: "" });

    for (const name of declarationsOf (text, "component"))
        targets.push ({ id: `component:${name}`, label: `Component: ${name}`, drawing: "", palette: "", component: name });

    return targets;
}

function isLightTheme (): boolean {
    const kind = vscode.window.activeColorTheme.kind;
    return kind === vscode.ColorThemeKind.Light || kind === vscode.ColorThemeKind.HighContrastLight;
}

/**
    The single preview panel, where the tool runs the interface of the active document.

    The document is shown as it is in its editor, so what is typed shows before it is saved, and the
    interface answers the mouse and the keyboard.
*/
export class YuiPreview {
    private panel: vscode.WebviewPanel | undefined;
    private document: vscode.TextDocument | undefined;
    private readonly targets = new Map<string, string> ();
    private readonly hosts = new Map<string, string> ();
    private readonly sent = new Map<string, string> ();
    private readonly disposables: vscode.Disposable[] = [];
    private typingTimer: NodeJS.Timeout | undefined;
    private zoom = "fit";
    private shown: string | undefined;
    private ready = false;
    private sequence = 0;

    constructor (private readonly host: PreviewHost, private readonly toolScript: string | undefined) {
        this.disposables.push (
            vscode.workspace.onDidSaveTextDocument (document => {
                if (document.languageId === "yui")
                    void this.render ();
            }),
            vscode.workspace.onDidChangeTextDocument (event => {
                if (event.contentChanges.length > 0 && event.document.languageId === "yui" && this.panel !== undefined)
                    this.renderSoon ();
            }),
            vscode.window.onDidChangeActiveTextEditor (editor => {
                if (editor === undefined || editor.document.languageId !== "yui")
                    return;

                if (this.document !== undefined && editor.document.uri.toString () === this.document.uri.toString ())
                    return;

                this.document = editor.document;
                void this.render ();
            }),
            vscode.workspace.onDidChangeConfiguration (event => {
                if (event.affectsConfiguration ("yui.preview") || event.affectsConfiguration ("yui.lint.models"))
                    void this.render ();
            }),
            vscode.window.onDidChangeActiveColorTheme (() => void this.render ())
        );
    }

    dispose (): void {
        for (const disposable of this.disposables)
            disposable.dispose ();

        this.disposables.length = 0;
        this.cancelTyping ();

        if (this.panel !== undefined) {
            this.panel.dispose ();
            this.panel = undefined;
        }
    }

    async open (): Promise<void> {
        const editor = vscode.window.activeTextEditor;

        if (editor === undefined || editor.document.languageId !== "yui") {
            void vscode.window.showInformationMessage ("Open a .yui document to preview it.");
            return;
        }

        this.document = editor.document;

        if (this.panel !== undefined) {
            this.panel.reveal (vscode.ViewColumn.Beside, true);
            await this.render ();
            return;
        }

        const toolFolder = this.toolScript === undefined ? undefined : vscode.Uri.file (path.dirname (this.toolScript));

        this.ready = false;
        this.sent.clear ();
        this.panel = vscode.window.createWebviewPanel ("yui.preview",
                                                       "YUI Preview",
                                                       { viewColumn: vscode.ViewColumn.Beside, preserveFocus: true },
                                                       { enableScripts: true, retainContextWhenHidden: true, localResourceRoots: toolFolder === undefined ? [] : [toolFolder] });

        this.panel.onDidDispose (() => {
            this.cancelTyping ();
            this.panel = undefined;
            this.ready = false;
        }, undefined, this.disposables);

        const webview = this.panel.webview;
        let module: PreviewModule | undefined;

        if (this.toolScript !== undefined && toolFolder !== undefined) {
            module = {
                script: webview.asWebviewUri (vscode.Uri.file (this.toolScript)).toString (),
                folder: webview.asWebviewUri (toolFolder).toString () + "/",
                source: webview.cspSource
            };
        }

        webview.onDidReceiveMessage (message => this.received (message), undefined, this.disposables);

        // The page says when it listens, which starts the first render
        webview.html = previewPage (crypto.randomBytes (16).toString ("hex"), module);
    }

    private received (message: unknown): void {
        if (typeof message !== "object" || message === null)
            return;

        const record = message as Record<string, unknown>;
        const name = record["name"];
        const value = record["value"];

        if (record["type"] === "ready") {
            // A page that was loaded again, as one moved to another window is, has none of the files
            this.sent.clear ();
            this.ready = true;
            void this.render ();
        }
        else if (record["type"] === "refresh") {
            void this.render ();
        }
        else if (record["type"] === "failed") {
            this.host.log (`the preview can't run: ${String (record["reason"])}`);
        }
        else if (record["type"] === "option" && typeof name === "string" && typeof value === "string") {
            void this.optionChosen (name, value);
        }
    }

    private renderSoon (): void {
        this.cancelTyping ();

        this.typingTimer = setTimeout (() => {
            this.typingTimer = undefined;
            void this.render ();
        }, typingDelayMs);
    }

    private cancelTyping (): void {
        if (this.typingTimer !== undefined) {
            clearTimeout (this.typingTimer);
            this.typingTimer = undefined;
        }
    }

    // What to show and its host are only ever looked up among those the document offers, never passed on as typed
    private async optionChosen (name: string, value: string): Promise<void> {
        if (name === "target" && this.document !== undefined)
            this.targets.set (this.document.uri.toString (), value);
        else if (name === "host" && this.shown !== undefined)
            this.hosts.set (this.shown, value);
        else if (name === "zoom" && zoomLevels.includes (value))
            this.zoom = value;
        else if ((name === "size" && isSizeOption (value)) || (name === "theme" && themes.includes (value)))
            return this.updateSetting (name, value);
        else
            return;

        await this.render ();
    }

    // The size and the theme are settings, so the toolbar and the settings editor always agree. Changing one renders again
    private async updateSetting (name: string, value: string): Promise<void> {
        const configuration = vscode.workspace.getConfiguration ("yui.preview");
        const inWorkspace = configuration.inspect<string> (name)?.workspaceValue !== undefined;

        try {
            await configuration.update (name, value, inWorkspace ? vscode.ConfigurationTarget.Workspace : vscode.ConfigurationTarget.Global);
        }
        catch (error) {
            this.host.log (`the setting yui.preview.${name} could not be changed: ${error instanceof Error ? error.message : String (error)}`);
        }
    }

    private async hostTypes (text: string): Promise<string[]> {
        const schema = await this.host.schema ();
        const names = new Set ((schema?.instantiableTypes () ?? []).concat (declarationsOf (text, "component")));

        names.delete (plainHost);
        return [plainHost].concat (Array.from (names).sort ());
    }

    private async render (): Promise<void> {
        const document = this.document;

        if (this.panel === undefined || document === undefined || ! this.ready)
            return;

        this.cancelTyping ();

        const sequence = ++this.sequence;
        const key = document.uri.toString ();
        const text = withoutComments (document.getText ());
        const targets = targetsOf (text);
        const target: PreviewTarget | undefined = targets.find (candidate => candidate.id === this.targets.get (key)) ?? targets[0];
        const hosts = target !== undefined && target.drawing.length > 0 ? await this.hostTypes (text) : [];

        if (sequence !== this.sequence || this.panel === undefined)
            return;

        this.panel.title = `YUI Preview: ${path.basename (document.uri.fsPath)}`;
        this.shown = target === undefined ? undefined : `${key} ${target.id}`;

        const chosenHost = this.shown === undefined ? undefined : this.hosts.get (this.shown);
        const host = chosenHost !== undefined && hosts.includes (chosenHost) ? chosenHost : plainHost;
        const size = sizeOption ();
        const theme = themeOption ();
        const zoom = this.zoom;

        this.post ({ type: "state", targets: targets.map (({ id, label }) => ({ id, label })), target: target?.id ?? "", hosts, host, size, theme, zoom });

        if (this.toolScript === undefined) {
            this.post ({ type: "note", text: "The preview needs the yup_yui_tool module, which this installation of the extension does not have." });
            return;
        }

        if (target === undefined) {
            this.post ({ type: "note", text: "Nothing to preview: the document has no root component, drawing, palette or component declaration." });
            return;
        }

        const collected = await collectDocumentFiles (document, this.host.modelsPath (document));

        if (sequence !== this.sequence)
            return;

        // The page keeps the files it was given, so only those that changed are sent again
        const changed = collected.files.filter (file => {
            const digest = crypto.createHash ("sha1").update (file.data).digest ("hex");

            if (this.sent.get (file.path) === digest)
                return false;

            this.sent.set (file.path, digest);
            return true;
        });

        // A message to a page is written as JSON, where a Buffer of Node turns itself into a list of numbers
        // before it can be sent as bytes: a plain array of bytes is sent as such
        const files = changed.map (file => ({ path: file.path, data: new Uint8Array (file.data) }));

        this.post ({
            type: "show",
            main: collected.main,
            models: collected.models,
            files,
            drawing: target.drawing,
            palette: target.palette,
            host: target.component.length > 0 ? target.component : (host !== plainHost ? host : ""),
            interface: target.id === rootTarget,
            light: theme !== "auto" ? theme === "light" : isLightTheme (),
            size,
            zoom
        });
    }

    private post (message: unknown): void {
        void this.panel?.webview.postMessage (message);
    }
}
