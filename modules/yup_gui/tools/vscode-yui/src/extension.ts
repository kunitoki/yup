import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";
import { YuiAgentBackend } from "./agentBackend";
import { YuiAgentTools } from "./agentSetup";
import { collectDocumentFiles, realPathOf, toolPathOf } from "./documentFiles";
import { YuiPreview } from "./preview";
import { errorMessage, findToolScript, YuiTool, YuiToolFile, YuiToolResult } from "./tool";

const lintDebounceMs = 300;

const pseudoClasses = ["hover", "pressed", "active", "focus", "focus-within", "disabled", "enabled"];

interface YuiProperty {
    name: string;
    type: string;
    values: string[];
    state: boolean;
    description: string | undefined;
}

interface YuiSignal {
    name: string;
    signature: string;
    description: string | undefined;
}

interface YuiType {
    name: string;
    base: string | undefined;
    instantiable: boolean;
    properties: YuiProperty[];
    signals: YuiSignal[];
}

/** The type, palette and named color information reported by 'yup_yui_tool schema'. */
class YuiSchema {
    constructor (private readonly types: Map<string, YuiType>, readonly paletteRoles: string[], readonly namedColors: string[]) {
    }

    hasType (name: string): boolean {
        return this.types.has (name);
    }

    instantiableTypes (): string[] {
        const names: string[] = [];

        for (const type of this.types.values ()) {
            if (type.instantiable)
                names.push (type.name);
        }

        return names;
    }

    properties (name: string): YuiProperty[] {
        const result: YuiProperty[] = [];
        const seen = new Set<string> ();

        for (const type of this.chain (name)) {
            for (const property of type.properties) {
                if (seen.has (property.name))
                    continue;

                seen.add (property.name);
                result.push (property);
            }
        }

        return result;
    }

    signals (name: string): YuiSignal[] {
        const result: YuiSignal[] = [];
        const seen = new Set<string> ();

        for (const type of this.chain (name)) {
            for (const signal of type.signals) {
                if (seen.has (signal.name))
                    continue;

                seen.add (signal.name);
                result.push (signal);
            }
        }

        return result;
    }

    property (typeName: string, propertyName: string): YuiProperty | undefined {
        for (const type of this.chain (typeName)) {
            for (const property of type.properties) {
                if (property.name === propertyName)
                    return property;
            }
        }

        return undefined;
    }

    stateProperties (): string[] {
        const names = new Set<string> ();

        for (const type of this.types.values ()) {
            for (const property of type.properties) {
                if (property.state)
                    names.add (property.name);
            }
        }

        return Array.from (names);
    }

    private chain (name: string): YuiType[] {
        const result: YuiType[] = [];
        const visited = new Set<string> ();
        let current = this.types.get (name);

        while (current !== undefined && ! visited.has (current.name)) {
            visited.add (current.name);
            result.push (current);
            current = current.base === undefined ? undefined : this.types.get (current.base);
        }

        return result;
    }
}

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

function asStringArray (value: unknown): string[] {
    return asArray (value).filter ((item): item is string => typeof item === "string");
}

function parseSchema (document: unknown): YuiSchema {
    const root = asRecord (document);
    const types = new Map<string, YuiType> ();

    for (const entry of asArray (root["types"])) {
        const record = asRecord (entry);
        const name = asString (record["name"]);

        if (name === undefined)
            continue;

        const properties: YuiProperty[] = [];

        for (const propertyEntry of asArray (record["properties"])) {
            const property = asRecord (propertyEntry);
            const propertyName = asString (property["name"]);

            if (propertyName === undefined)
                continue;

            properties.push ({
                name: propertyName,
                type: asString (property["type"]) ?? "any",
                values: asStringArray (property["values"]),
                state: property["state"] === true,
                description: asString (property["description"])
            });
        }

        const signals: YuiSignal[] = [];

        for (const signalEntry of asArray (record["signals"])) {
            const signal = asRecord (signalEntry);
            const signalName = asString (signal["name"]);

            if (signalName === undefined)
                continue;

            const parameters = asArray (signal["arguments"]).map (argument => {
                const parameter = asRecord (argument);
                const argumentName = asString (parameter["name"]) ?? "value";
                const argumentType = asString (parameter["type"]);

                return argumentType === undefined ? argumentName : `${argumentName}: ${argumentType}`;
            });

            signals.push ({
                name: signalName,
                signature: `${signalName} (${parameters.join (", ")})`,
                description: asString (signal["description"])
            });
        }

        types.set (name, {
            name,
            base: asString (record["base"]),
            instantiable: record["instantiable"] !== false,
            properties,
            signals
        });
    }

    return new YuiSchema (types, asStringArray (root["paletteRoles"]), asStringArray (root["namedColors"]));
}

interface YuiBlock {
    header: string;
    typeName: string | undefined;
}

function stripComments (text: string): string {
    return text.replace (/\/\*[\s\S]*?\*\//g, " ").replace (/\/\/[^\n]*/g, " ");
}

/** Finds the innermost block containing the offset, scanning the text without a parser. */
function findEnclosingBlock (text: string, offset: number): YuiBlock | undefined {
    const stack: { brace: number; headerStart: number }[] = [];
    let boundary = 0;
    let index = 0;

    while (index < offset) {
        const character = text.charAt (index);

        if (character === "/" && text.charAt (index + 1) === "/") {
            const end = text.indexOf ("\n", index);
            index = end < 0 ? offset : end + 1;
            continue;
        }

        if (character === "/" && text.charAt (index + 1) === "*") {
            const end = text.indexOf ("*/", index + 2);
            index = end < 0 ? offset : end + 2;
            continue;
        }

        if (character === "\"") {
            index += 1;

            while (index < offset && text.charAt (index) !== "\"") {
                if (text.charAt (index) === "\\")
                    index += 1;

                index += 1;
            }

            index += 1;
            continue;
        }

        if (character === "{") {
            stack.push ({ brace: index, headerStart: boundary });
            boundary = index + 1;
        }
        else if (character === "}" || character === ";") {
            if (character === "}")
                stack.pop ();

            boundary = index + 1;
        }

        index += 1;
    }

    const innermost = stack.pop ();

    if (innermost === undefined)
        return undefined;

    const header = stripComments (text.substring (innermost.headerStart, innermost.brace)).trim ();
    return { header, typeName: typeNameOfHeader (header) };
}

/** Returns the schema type a block header refers to, following 'component Name : Base' to the base. */
function typeNameOfHeader (header: string): string | undefined {
    const derived = /^component\s+[A-Za-z_][A-Za-z0-9_]*\s*:\s*([A-Z][A-Za-z0-9_]*)/.exec (header);

    if (derived !== null)
        return derived[1];

    const first = /[A-Z][A-Za-z0-9_]*/.exec (header);
    return first === null ? undefined : first[0];
}

function defaultDrawingName (file: string): string {
    const parts = path.basename (file, path.extname (file)).split (/[^A-Za-z0-9]+/).filter (part => part.length > 0);
    const name = parts.map (part => part.charAt (0).toUpperCase () + part.substring (1)).join ("");

    if (name.length === 0 || ! /^[A-Za-z]/.test (name))
        return `Drawing${name}`;

    return name;
}

/** Owns the tool, the diagnostics and the cached schema for the session. */
class YuiSession {
    private readonly tool: YuiTool | undefined;
    private schemaPromise: Promise<YuiSchema | undefined> | undefined;
    private readonly timers = new Map<string, NodeJS.Timeout> ();
    private readonly sequences = new Map<string, number> ();
    private readonly lintedUris = new Map<string, string[]> ();
    private readonly reportedModels = new Set<string> ();

    constructor (private readonly context: vscode.ExtensionContext,
                 private readonly output: vscode.OutputChannel,
                 private readonly diagnostics: vscode.DiagnosticCollection) {
        const script = this.toolScript ();

        this.tool = script === undefined ? undefined : new YuiTool (script, message => this.log (message));
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
    async run (args: string[], files: YuiToolFile[] = []): Promise<YuiToolResult | undefined> {
        return this.tool?.run (args, files);
    }

    async schema (): Promise<YuiSchema | undefined> {
        if (this.schemaPromise === undefined)
            this.schemaPromise = this.fetchSchema ();

        const schema = await this.schemaPromise;

        if (schema === undefined)
            this.schemaPromise = undefined;

        return schema;
    }

    private async fetchSchema (): Promise<YuiSchema | undefined> {
        const result = await this.run (["schema"]);

        if (result === undefined)
            return undefined;

        if (result.code !== 0) {
            this.log (`schema failed with exit code ${result.code}: ${result.stderr.trim ()}`);
            return undefined;
        }

        try {
            return parseSchema (JSON.parse (result.stdout));
        }
        catch (error) {
            this.log (`schema returned invalid JSON: ${errorMessage (error)}`);
            return undefined;
        }
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

    /** Lints again the open documents using a models file that was created, changed or deleted. */
    modelsChanged (file: string): void {
        for (const document of vscode.workspace.textDocuments) {
            if (isYuiDocument (document) && this.wantedModelsPath (document).file === file)
                this.lintSoon (document);
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

        const enabled = vscode.workspace.getConfiguration ("yui.lint").get<boolean> ("enabled");

        if (enabled === false || ! this.hasTool ()) {
            this.clear (document);
            return;
        }

        const key = document.uri.toString ();
        const sequence = (this.sequences.get (key) ?? 0) + 1;
        this.sequences.set (key, sequence);

        // The tool is given the document as it is in its editor, with the files it names
        const collected = await collectDocumentFiles (document, this.modelsPath (document));
        const args = ["lint", "--json"];

        if (collected.models.length > 0)
            args.push ("--models", collected.models);

        args.push (collected.main);

        const result = await this.run (args, collected.files);

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

        this.apply (document, this.collect (document, parsed));
    }

    clear (document: vscode.TextDocument): void {
        const key = document.uri.toString ();
        const owned = this.lintedUris.get (key);

        if (owned !== undefined) {
            for (const uri of owned)
                this.diagnostics.delete (vscode.Uri.parse (uri));
        }

        this.lintedUris.delete (key);
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
    }

    private collect (document: vscode.TextDocument, parsed: unknown): Map<string, vscode.Diagnostic[]> {
        const grouped = new Map<string, vscode.Diagnostic[]> ();

        for (const entry of asArray (asRecord (parsed)["diagnostics"])) {
            const record = asRecord (entry);
            const message = asString (record["message"]) ?? "Unknown problem";
            const file = realPathOf (asString (record["file"]) ?? "");
            const uri = file === undefined ? document.uri : vscode.Uri.file (file);
            const line = Math.max (0, numberOf (record["line"]) - 1);
            const column = Math.max (0, numberOf (record["column"]) - 1);

            const diagnostic = new vscode.Diagnostic (new vscode.Range (line, column, line, column + 1),
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
        const key = document.uri.toString ();
        const owned = this.lintedUris.get (key);

        if (owned !== undefined) {
            for (const uri of owned) {
                if (! grouped.has (uri))
                    this.diagnostics.delete (vscode.Uri.parse (uri));
            }
        }

        for (const [uri, items] of grouped)
            this.diagnostics.set (vscode.Uri.parse (uri), items);

        this.lintedUris.set (key, Array.from (grouped.keys ()));
    }

    // The models file a document asks for, whether it exists or not
    private wantedModelsPath (document: vscode.TextDocument): { file: string; configured: boolean } {
        const configured = vscode.workspace.getConfiguration ("yui.lint").get<string> ("models")?.trim () ?? "";

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

class YuiCompletionProvider implements vscode.CompletionItemProvider {
    constructor (private readonly session: YuiSession) {
    }

    async provideCompletionItems (document: vscode.TextDocument, position: vscode.Position): Promise<vscode.CompletionItem[]> {
        try {
            return await this.complete (document, position);
        }
        catch (error) {
            this.session.log (`completion failed: ${errorMessage (error)}`);
            return [];
        }
    }

    private async complete (document: vscode.TextDocument, position: vscode.Position): Promise<vscode.CompletionItem[]> {
        const schema = await this.session.schema ();

        if (schema === undefined)
            return [];

        const line = document.getText (new vscode.Range (position.line, 0, position.line, position.character));

        if (/palette\.[A-Za-z_]*$/.test (line)) {
            const roles = schema.paletteRoles.concat (["dark"]);
            return roles.map (role => new vscode.CompletionItem (role, vscode.CompletionItemKind.EnumMember));
        }

        if (/(?:^|[^A-Za-z0-9_.])colors\.[A-Za-z_]*$/.test (line))
            return schema.namedColors.map (name => new vscode.CompletionItem (name, vscode.CompletionItemKind.Color));

        const block = findEnclosingBlock (document.getText (), document.offsetAt (position));

        if (block !== undefined && block.header === "style" && ! line.includes (";") && /:[A-Za-z-]*$/.test (line)) {
            const names = pseudoClasses.concat (schema.stateProperties ());
            return names.map (name => new vscode.CompletionItem (name, vscode.CompletionItemKind.EnumMember));
        }

        const typeName = block === undefined ? undefined : block.typeName;
        const assignment = /(?:^|[{;])\s*([A-Za-z_][A-Za-z0-9_]*)\s*:\s*([A-Za-z0-9_]*)$/.exec (line);

        if (assignment !== null) {
            if (typeName === undefined)
                return [];

            const property = schema.property (typeName, assignment[1]);

            if (property === undefined || property.type !== "enumeration")
                return [];

            return property.values.map (value => new vscode.CompletionItem (value, vscode.CompletionItemKind.EnumMember));
        }

        const word = /^\s*([A-Za-z_][A-Za-z0-9_]*)?$/.exec (line);

        if (word === null)
            return [];

        const prefix = word[1] ?? "";
        const items: vscode.CompletionItem[] = [];

        if (prefix.length === 0 || /^[A-Z]/.test (prefix)) {
            for (const name of schema.instantiableTypes ())
                items.push (new vscode.CompletionItem (name, vscode.CompletionItemKind.Class));
        }

        if (prefix.length === 0 || /^[a-z_]/.test (prefix)) {
            if (typeName === undefined || ! schema.hasType (typeName))
                return items;

            for (const property of schema.properties (typeName)) {
                const item = new vscode.CompletionItem (property.name, vscode.CompletionItemKind.Property);
                item.detail = property.type;

                if (property.description !== undefined)
                    item.documentation = property.description;

                items.push (item);
            }

            for (const signal of schema.signals (typeName)) {
                const item = new vscode.CompletionItem (signal.name, vscode.CompletionItemKind.Event);
                item.detail = signal.signature;

                if (signal.description !== undefined)
                    item.documentation = signal.description;

                items.push (item);
            }
        }

        return items;
    }
}

async function importSvg (session: YuiSession): Promise<void> {
    if (! session.hasTool ()) {
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

    const document = await vscode.workspace.openTextDocument ({ language: "yui", content: result.stdout });
    await vscode.window.showTextDocument (document);
}

export function activate (context: vscode.ExtensionContext): void {
    const output = vscode.window.createOutputChannel ("YUI");
    const diagnostics = vscode.languages.createDiagnosticCollection ("yui");
    const session = new YuiSession (context, output, diagnostics);
    const preview = new YuiPreview (session, session.toolScript ());
    const agentTools = new YuiAgentTools (new YuiAgentBackend (session, preview), String (context.extension.packageJSON.version), message => session.log (message));

    context.subscriptions.push (output, diagnostics, session, preview, agentTools);

    const modelsWatcher = vscode.workspace.createFileSystemWatcher ("**/*.json");
    const modelsChanged = (uri: vscode.Uri) => session.modelsChanged (uri.fsPath);

    context.subscriptions.push (
        modelsWatcher,
        modelsWatcher.onDidCreate (modelsChanged),
        modelsWatcher.onDidChange (modelsChanged),
        modelsWatcher.onDidDelete (modelsChanged),
        vscode.workspace.onDidOpenTextDocument (document => void session.lint (document)),
        vscode.workspace.onDidSaveTextDocument (document => void session.lint (document)),
        vscode.workspace.onDidChangeTextDocument (event => {
            if (event.contentChanges.length === 0)
                return;

            if (isYuiDocument (event.document))
                session.lintSoon (event.document);
            else if (event.document.uri.scheme === "file")
                session.modelsChanged (event.document.uri.fsPath);
        }),
        vscode.workspace.onDidCloseTextDocument (document => session.forget (document)),
        vscode.workspace.onDidChangeConfiguration (event => {
            session.configurationChanged (event);

            if (event.affectsConfiguration ("yui.mcp"))
                void agentTools.apply ();
        }),
        vscode.languages.registerCompletionItemProvider ({ language: "yui" }, new YuiCompletionProvider (session), ".", ":"),
        vscode.commands.registerCommand ("yui.preview", () => preview.open ()),
        vscode.commands.registerCommand ("yui.importSvg", () => importSvg (session)),
        vscode.commands.registerCommand ("yui.setUpAgentTools", () => agentTools.setUp ())
    );

    session.lintOpenDocuments ();
    void agentTools.apply ();
}

export function deactivate (): void {
}
