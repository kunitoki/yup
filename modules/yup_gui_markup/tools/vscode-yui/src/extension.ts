import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";
import { YuiAgentBackend } from "./agentBackend";
import { YuiAgentTools } from "./agentSetup";
import { SharedDiagnostics } from "./diagnostics";
import { collectDocumentFiles, isNamedBy, realPathOf, runUntilFilesAreGiven, setReadableFolders, setUnsavedTexts, toolPathOf } from "./documentFiles";
import { characterIn, registerLanguageFeatures } from "./language";
import { YuiPreview } from "./preview";
import { errorMessage, findToolScript, YuiTool, YuiToolAsking, YuiToolFile, YuiToolResult } from "./tool";

const lintDebounceMs = 300;

function asArray (value: unknown): unknown[] {
    return Array.isArray (value) ? value : [];
}

function asRecord (value: unknown): Record<string, unknown> {
    if (typeof value !== "object" || value === null || Array.isArray (value))
        return {};

    return value as Record<string, unknown>;
}

function asString (value: unknown): string | undefined {
    return typeof value === "string" && value.length > 0 ? value : undefined;
}

function defaultDrawingName (file: string): string {
    const parts = path.basename (file, path.extname (file)).split (/[^A-Za-z0-9]+/).filter (part => part.length > 0);
    const name = parts.map (part => part.charAt (0).toUpperCase () + part.substring (1)).join ("");

    if (name.length === 0 || ! /^[A-Za-z]/.test (name))
        return `Drawing${name}`;

    return name;
}

/** Owns the tool and the diagnostics for the session, and asks the tool what the editor features show. */
class YuiSession {
    private readonly tool: YuiTool | undefined;
    private readonly outlines = new Map<string, { version: number; outline: Promise<Record<string, unknown> | undefined> }> ();
    private readonly timers = new Map<string, NodeJS.Timeout> ();
    private readonly sequences = new Map<string, number> ();
    private readonly reportedModels = new Set<string> ();

    // A file imported by two open documents has its problems told by the check of each, and shows them all once
    private readonly problems = new SharedDiagnostics<vscode.Diagnostic> (item =>
        `${item.severity} ${item.range.start.line}:${item.range.start.character} ${item.range.end.line}:${item.range.end.character} ${item.message}`);

    constructor (private readonly context: vscode.ExtensionContext,
                 private readonly output: vscode.OutputChannel,
                 private readonly diagnostics: vscode.DiagnosticCollection) {
        const script = this.toolScript ();

        this.tool = script === undefined ? undefined : new YuiTool (script, message => this.log (message), () => {
            void vscode.window.showWarningMessage ("The YUI tool stopped twice in a row: diagnostics, completions and the tools for agents are off until it runs again. See the YUI output channel.");
        });

        void vscode.commands.executeCommand ("setContext", "yui.hasTool", this.tool !== undefined);

        if (this.tool === undefined)
            this.log ("this installation of the extension has no yup_yui_tool module: diagnostics, completions, the preview and SVG import are off");
    }

    /** The script of the tool, which the preview runs too. An extension packaged without the tool has none. */
    toolScript (): string | undefined {
        return findToolScript (this.context.extensionPath);
    }

    /** Whether there is a tool to run, without which what it gives is quietly off. */
    hasTool (): boolean {
        return this.tool !== undefined && this.tool.isAvailable ();
    }

    /** The tool, for who runs it on documents of its own. */
    yuiTool (): YuiTool | undefined {
        return this.hasTool () ? this.tool : undefined;
    }

    dispose (): void {
        for (const timer of this.timers.values ())
            clearTimeout (timer);

        this.timers.clear ();
        this.tool?.dispose ();
    }

    log (message: string): void {
        this.output.appendLine (message);
    }

    /** Runs a command of the tool on the files given, which are named as the tool knows them. */
    async run (args: string[], files: YuiToolFile[] = [], asking: YuiToolAsking = {}): Promise<YuiToolResult | undefined> {
        return this.tool?.run (args, files, asking);
    }

    /**
        Asks the tool about a document as it is in its editor, with the files it names.

        @param command      A command of the tool answering in JSON: outline, complete, definition or hover.
        @param args         Its arguments after the document.
        @param cancellation Tells when the editor stopped waiting for the answer. What is asked with one is asked by
                            the editor for itself: asked again, what of it still waits to run is not run.
        @returns What it answered, or nothing when there is no tool, it could not answer or the answer is not waited for.
    */
    async ask (document: vscode.TextDocument, command: string, args: string[] = [], cancellation?: vscode.CancellationToken): Promise<Record<string, unknown> | undefined> {
        if (! isYuiDocument (document) || ! this.hasTool ())
            return undefined;

        const collected = await collectDocumentFiles (document, this.modelsPath (document));

        // Reading the files took a while, in which the editor may have asked for something else
        if (cancellation?.isCancellationRequested === true)
            return undefined;

        const models = command === "complete" && collected.models.length > 0 ? ["--models", collected.models] : [];
        const asking: YuiToolAsking = cancellation === undefined ? {} : { supersedes: `${command} ${document.uri.toString ()}`, cancellation };
        const result = await this.run ([command, collected.main, ...args, ...models], collected.files, asking);

        if (result === undefined)
            return undefined;

        if (result.code !== 0) {
            this.log (`${command} failed with exit code ${result.code}: ${result.stderr.trim ()}`);
            return undefined;
        }

        try {
            return asRecord (JSON.parse (result.stdout));
        }
        catch (error) {
            this.log (`${command} returned invalid JSON: ${errorMessage (error)}`);
            return undefined;
        }
    }

    /** What a document is made of, as the tool tells it: asked once for each state of the document. */
    outline (document: vscode.TextDocument): Promise<Record<string, unknown> | undefined> {
        const key = document.uri.toString ();
        const kept = this.outlines.get (key);

        if (kept !== undefined && kept.version === document.version)
            return kept.outline;

        const outline = this.ask (document, "outline");

        this.outlines.set (key, { version: document.version, outline });
        return outline;
    }

    configurationChanged (event: vscode.ConfigurationChangeEvent): void {
        if (event.affectsConfiguration ("yui.lint.models"))
            this.reportedModels.clear ();

        if (event.affectsConfiguration ("yui.lint.enabled") || event.affectsConfiguration ("yui.lint.models"))
            this.lintOpenDocuments ();
    }

    lintOpenDocuments (): void {
        for (const document of vscode.workspace.textDocuments) {
            if (isYuiDocument (document))
                void this.lint (document);
        }
    }

    /** Lints again the open documents whose models file, or a file they name, was created, changed or deleted. */
    fileChanged (file: string): void {
        for (const document of vscode.workspace.textDocuments) {
            if (isYuiDocument (document) && (isNamedBy (document.uri.fsPath, file) || this.wantedModelsPath (document).file === file)) {
                // What the tool told the document is made of counted the file as it was
                this.outlines.delete (document.uri.toString ());
                this.lintSoon (document);
            }
        }
    }

    lintSoon (document: vscode.TextDocument): void {
        const key = document.uri.toString ();
        const pending = this.timers.get (key);

        if (pending !== undefined)
            clearTimeout (pending);

        this.timers.set (key, setTimeout (() => {
            this.timers.delete (key);
            void this.lint (document);
        }, lintDebounceMs));
    }

    async lint (document: vscode.TextDocument): Promise<void> {
        if (! isYuiDocument (document))
            return;

        // A document is checked because it, or what it reads, is new or changed: a tool that stopped may not stop on it
        this.tool?.retry ();

        const enabled = vscode.workspace.getConfiguration ("yui.lint").get<boolean> ("enabled");

        if (enabled === false || ! this.hasTool ()) {
            this.clear (document);
            return;
        }

        const key = document.uri.toString ();
        const sequence = (this.sequences.get (key) ?? 0) + 1;
        this.sequences.set (key, sequence);

        // The tool is given the document as it is in its editor, with the files it names. It tells those it read
        // without having been given them, as what a shader is compiled into: with those there are, it checks again
        const result = await runUntilFilesAreGiven (async () => {
            const collected = await collectDocumentFiles (document, this.modelsPath (document));

            // A check that is not the last one asked for does not run: it would take the place of that one
            if (this.sequences.get (key) !== sequence)
                return undefined;

            const args = ["lint", "--json"];

            if (collected.models.length > 0)
                args.push ("--models", collected.models);

            args.push (collected.main);

            const ran = await this.run (args, collected.files, { supersedes: `lint ${key}` });

            return ran === undefined ? undefined : { ...ran, document: document.uri.fsPath, sought: collected.sought, problems: collected.problems };
        });

        if (this.sequences.get (key) !== sequence)
            return;

        if (result === undefined) {
            this.clear (document);
            return;
        }

        if (result.code !== 0 && result.code !== 1) {
            this.log (`lint failed with exit code ${result.code}: ${result.stderr.trim ()}`);
            this.clear (document);
            return;
        }

        let parsed: unknown;

        try {
            parsed = JSON.parse (result.stdout);
        }
        catch (error) {
            this.log (`lint returned invalid JSON: ${errorMessage (error)}`);
            this.clear (document);
            return;
        }

        const grouped = this.collect (document, parsed);
        const top = vscode.Uri.file (document.uri.fsPath).toString ();

        // What the document names that could not be given to the tool is told at its top
        for (const problem of result.problems) {
            const diagnostic = new vscode.Diagnostic (new vscode.Range (0, 0, 0, 1), problem, vscode.DiagnosticSeverity.Warning);
            diagnostic.source = "yui";

            grouped.set (top, [...(grouped.get (top) ?? []), diagnostic]);
        }

        this.apply (document, grouped);
    }

    clear (document: vscode.TextDocument): void {
        this.publish (this.problems.clear (document.uri.toString ()));
    }

    forget (document: vscode.TextDocument): void {
        const key = document.uri.toString ();
        const pending = this.timers.get (key);

        if (pending !== undefined) {
            clearTimeout (pending);
            this.timers.delete (key);
        }

        this.clear (document);
        this.sequences.delete (key);
        this.outlines.delete (key);
    }

    private collect (document: vscode.TextDocument, parsed: unknown): Map<string, vscode.Diagnostic[]> {
        const grouped = new Map<string, vscode.Diagnostic[]> ();

        for (const entry of asArray (asRecord (parsed)["diagnostics"])) {
            const record = asRecord (entry);
            const message = asString (record["message"]) ?? "Unknown problem";
            const file = realPathOf (asString (record["file"]) ?? "");
            const uri = file === undefined ? document.uri : vscode.Uri.file (file);
            const edited = file === undefined ? document : vscode.workspace.textDocuments.find (open => open.uri.scheme === "file" && open.uri.fsPath === file);
            const line = Math.max (1, numberOf (record["line"]));
            const column = Math.max (1, numberOf (record["column"]));

            // The problem is told at one character, which an editor may write with two units
            const diagnostic = new vscode.Diagnostic (new vscode.Range (line - 1, characterIn (edited, line, column), line - 1, characterIn (edited, line, column + 1)),
                                                      message,
                                                      record["severity"] === "warning" ? vscode.DiagnosticSeverity.Warning : vscode.DiagnosticSeverity.Error);
            diagnostic.source = "yui";

            const key = uri.toString ();
            const existing = grouped.get (key);

            if (existing === undefined)
                grouped.set (key, [diagnostic]);
            else
                existing.push (diagnostic);
        }

        return grouped;
    }

    private apply (document: vscode.TextDocument, grouped: Map<string, vscode.Diagnostic[]>): void {
        this.publish (this.problems.set (document.uri.toString (), grouped));
    }

    // Shows the problems of files again, as the checks of all the documents telling some now have them
    private publish (uris: string[]): void {
        for (const uri of uris) {
            const items = this.problems.of (uri);

            if (items.length > 0)
                this.diagnostics.set (vscode.Uri.parse (uri), items);
            else
                this.diagnostics.delete (vscode.Uri.parse (uri));
        }
    }

    // The models file a document asks for, whether it exists or not
    private wantedModelsPath (document: vscode.TextDocument): { file: string; configured: boolean } {
        const configured = vscode.workspace.getConfiguration ("yui.lint", document.uri).get<string> ("models")?.trim () ?? "";

        // Without a setting, a document uses the models next to it, named after it
        if (configured.length === 0)
            return { file: path.join (path.dirname (document.uri.fsPath), path.basename (document.uri.fsPath, ".yui") + ".models.json"), configured: false };

        if (path.isAbsolute (configured))
            return { file: configured, configured: true };

        const folder = vscode.workspace.getWorkspaceFolder (document.uri);
        const base = folder === undefined ? path.dirname (document.uri.fsPath) : folder.uri.fsPath;
        return { file: path.resolve (base, configured), configured: true };
    }

    modelsPath (document: vscode.TextDocument): string | undefined {
        if (document.uri.scheme !== "file")
            return undefined;

        const { file: resolved, configured } = this.wantedModelsPath (document);

        if (fs.existsSync (resolved))
            return resolved;

        if (configured && ! this.reportedModels.has (resolved)) {
            this.reportedModels.add (resolved);
            this.log (`models file not found, continuing without it: ${resolved}`);
        }

        return undefined;
    }
}

function numberOf (value: unknown): number {
    return typeof value === "number" && Number.isFinite (value) ? value : 0;
}

function isYuiDocument (document: vscode.TextDocument): boolean {
    return document.languageId === "yui" && document.uri.scheme === "file";
}

// Where a drawing is offered to be saved: next to the document being edited, else in the workspace, else next to its image
function importFolder (image: vscode.Uri): vscode.Uri {
    const edited = vscode.window.activeTextEditor?.document.uri;

    if (edited !== undefined && edited.scheme === "file")
        return vscode.Uri.joinPath (edited, "..");

    return vscode.workspace.workspaceFolders?.[0]?.uri ?? vscode.Uri.joinPath (image, "..");
}

async function importSvg (session: YuiSession): Promise<void> {
    if (session.toolScript () === undefined) {
        void vscode.window.showInformationMessage ("Importing an SVG needs the yup_yui_tool module, which this installation of the extension does not have.");
        return;
    }

    const selection = await vscode.window.showOpenDialog ({
        canSelectMany: false,
        openLabel: "Import",
        filters: { "SVG images": ["svg"] }
    });

    if (selection === undefined || selection.length === 0)
        return;

    const file = selection[0];
    const name = await vscode.window.showInputBox ({
        prompt: "Name of the generated drawing",
        value: defaultDrawingName (file.fsPath),
        validateInput: value => /^[A-Za-z][A-Za-z0-9_]*$/.test (value)
            ? undefined
            : "The name must start with a letter and contain only letters, digits and underscores."
    });

    if (name === undefined)
        return;

    // The drawing is saved before it is opened: a document that is no file is not checked, completed or shown
    const saved = await vscode.window.showSaveDialog ({
        defaultUri: vscode.Uri.joinPath (importFolder (file), path.basename (file.fsPath, path.extname (file.fsPath)) + ".yui"),
        saveLabel: "Import",
        filters: { "YUI documents": ["yui"] }
    });

    if (saved === undefined)
        return;

    const image = toolPathOf (file.fsPath);
    const result = await session.run (["import-svg", image, "--name", name], [{ path: image, data: await fs.promises.readFile (file.fsPath) }]);

    if (result === undefined) {
        void vscode.window.showErrorMessage (`Importing ${path.basename (file.fsPath)} failed, see the YUI output channel.`);
        return;
    }

    if (result.stderr.trim ().length > 0)
        session.log (result.stderr.trim ());

    if (result.code !== 0) {
        void vscode.window.showErrorMessage (`Importing ${path.basename (file.fsPath)} failed, see the YUI output channel.`);
        return;
    }

    try {
        await vscode.workspace.fs.writeFile (saved, Buffer.from (result.stdout.endsWith ("\n") ? result.stdout : result.stdout + "\n", "utf8"));
    }
    catch (error) {
        void vscode.window.showErrorMessage (`${saved.fsPath} could not be written: ${errorMessage (error)}`);
        return;
    }

    await vscode.window.showTextDocument (await vscode.workspace.openTextDocument (saved));
}

export function activate (context: vscode.ExtensionContext): void {
    const output = vscode.window.createOutputChannel ("YUI");
    const diagnostics = vscode.languages.createDiagnosticCollection ("yui");
    const session = new YuiSession (context, output, diagnostics);
    const preview = new YuiPreview (session, session.toolScript (), vscode.Uri.joinPath (context.extensionUri, "images", "yui-icon.png"));
    const agentTools = new YuiAgentTools (new YuiAgentBackend (session, preview), String (context.extension.packageJSON.version), message => session.log (message), context.secrets);

    context.subscriptions.push (output, diagnostics, session, preview, agentTools);

    // A file a document names counts as it is in its editor while that holds changes of it that are not saved
    setUnsavedTexts (file => vscode.workspace.textDocuments.find (document => document.uri.scheme === "file" && document.uri.fsPath === file && document.isDirty)?.getText ());

    // A document reads the files it names in the workspace and in its own folder, and no other
    setReadableFolders (() => (vscode.workspace.workspaceFolders ?? []).map (folder => folder.uri.fsPath));

    // The models of a document and the files it names change outside the editor too: a model exported again, a shader built
    const filesWatcher = vscode.workspace.createFileSystemWatcher ("**/*");
    const fileChanged = (uri: vscode.Uri) => {
        session.fileChanged (uri.fsPath);
        preview.fileChanged (uri.fsPath);
    };

    context.subscriptions.push (
        filesWatcher,
        filesWatcher.onDidCreate (fileChanged),
        filesWatcher.onDidChange (fileChanged),
        filesWatcher.onDidDelete (fileChanged),
        vscode.workspace.onDidOpenTextDocument (document => void session.lint (document)),
        vscode.workspace.onDidSaveTextDocument (document => void session.lint (document)),
        vscode.workspace.onDidChangeTextDocument (event => {
            if (event.contentChanges.length === 0)
                return;

            if (isYuiDocument (event.document))
                session.lintSoon (event.document);
            else if (event.document.uri.scheme === "file")
                session.fileChanged (event.document.uri.fsPath);
        }),
        vscode.workspace.onDidCloseTextDocument (document => session.forget (document)),
        vscode.workspace.onDidChangeConfiguration (event => {
            session.configurationChanged (event);

            if (event.affectsConfiguration ("yui.mcp"))
                void agentTools.apply ();
        }),
        vscode.workspace.onDidGrantWorkspaceTrust (() => {
            // The settings a workspace may not set until it is trusted count from now on
            session.lintOpenDocuments ();
            void agentTools.apply ();
        }),
        ...registerLanguageFeatures (session),
        vscode.commands.registerCommand ("yui.preview", () => preview.open ()),
        vscode.commands.registerCommand ("yui.importSvg", () => importSvg (session)),
        vscode.commands.registerCommand ("yui.setUpAgentTools", () => agentTools.setUp ())
    );

    session.lintOpenDocuments ();
    void agentTools.apply ();
}

export function deactivate (): void {
}
