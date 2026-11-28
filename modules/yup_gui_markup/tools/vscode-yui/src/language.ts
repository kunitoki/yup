import * as vscode from "vscode";
import { realPathOf } from "./documentFiles";
import { characterOf, toolColumnOf } from "./positions";

/** Who asks the tool what the editor features show of a document, as it is in its editor. */
export interface LanguageHost {
    log (message: string): void;

    /**
        Asks a command of the tool answering in JSON about a document, with its arguments after the document. What
        is asked with the token of the editor is not run once the editor stopped waiting, or asked for it again.
    */
    ask (document: vscode.TextDocument, command: string, args?: string[], cancellation?: vscode.CancellationToken): Promise<Record<string, unknown> | undefined>;

    /** What a document is made of: its symbols, its imports and its colors. */
    outline (document: vscode.TextDocument): Promise<Record<string, unknown> | undefined>;
}

// What the tool calls the kind of what can be written, and of a symbol, as the editor shows them
const completionKinds: Record<string, vscode.CompletionItemKind> = {
    type: vscode.CompletionItemKind.Class,
    property: vscode.CompletionItemKind.Property,
    signal: vscode.CompletionItemKind.Event,
    keyword: vscode.CompletionItemKind.Keyword,
    element: vscode.CompletionItemKind.Struct,
    value: vscode.CompletionItemKind.EnumMember,
    color: vscode.CompletionItemKind.Color,
    constant: vscode.CompletionItemKind.Constant,
    field: vscode.CompletionItemKind.Field,
    drawing: vscode.CompletionItemKind.Function,
    shader: vscode.CompletionItemKind.Interface,
    palette: vscode.CompletionItemKind.Enum
};

const symbolKinds: Record<string, vscode.SymbolKind> = {
    drawing: vscode.SymbolKind.Function,
    shader: vscode.SymbolKind.Interface,
    palette: vscode.SymbolKind.Enum,
    component: vscode.SymbolKind.Class,
    tokens: vscode.SymbolKind.Constant,
    style: vscode.SymbolKind.Namespace,
    rule: vscode.SymbolKind.Struct,
    element: vscode.SymbolKind.Object,
    property: vscode.SymbolKind.Property
};

function records (value: unknown): Record<string, unknown>[] {
    return Array.isArray (value) ? value.filter ((item): item is Record<string, unknown> => typeof item === "object" && item !== null) : [];
}

function text (value: unknown): string {
    return typeof value === "string" ? value : "";
}

/**
    Returns where an editor has a column of the tool on a line of a document, both counted from 1 as the tool
    tells them. The tool counts characters and an editor the units they are written with (see characterOf).

    @param document The document as an editor holds it. Without it the column is taken as it is.
*/
export function characterIn (document: vscode.TextDocument | undefined, line: number, column: number): number {
    return characterOf (document !== undefined && line >= 1 && line <= document.lineCount ? document.lineAt (line - 1).text : "", column);
}

// The tool tells a range of a document by the lines and the columns it starts and ends at
function rangeOf (document: vscode.TextDocument, value: unknown): vscode.Range | undefined {
    const places = Array.isArray (value) && value.length === 4 && value.every (place => typeof place === "number") ? value as number[] : undefined;

    return places === undefined ? undefined : new vscode.Range (places[0] - 1, characterIn (document, places[0], places[1]),
                                                                places[2] - 1, characterIn (document, places[2], places[3]));
}

function placeOf (document: vscode.TextDocument, position: vscode.Position): string[] {
    return ["--at", `${position.line + 1}:${toolColumnOf (document.lineAt (position.line).text, position.character)}`];
}

function symbolOf (document: vscode.TextDocument, symbol: Record<string, unknown>): vscode.DocumentSymbol | undefined {
    const range = rangeOf (document, symbol["range"]);
    const name = text (symbol["name"]);

    if (range === undefined || name.length === 0)
        return undefined;

    const described = new vscode.DocumentSymbol (name, text (symbol["detail"]), symbolKinds[text (symbol["kind"])] ?? vscode.SymbolKind.Object,
                                                 range, rangeOf (document, symbol["selection"]) ?? range);

    described.children = symbolsOf (document, symbol["children"]);
    return described;
}

function symbolsOf (document: vscode.TextDocument, symbols: unknown): vscode.DocumentSymbol[] {
    return records (symbols).map (symbol => symbolOf (document, symbol)).filter ((symbol): symbol is vscode.DocumentSymbol => symbol !== undefined);
}

/**
    Registers what an editor shows of a `.yui` document beyond its problems: completions, outline, definitions,
    hovers and colors. All of it is what the tool answers, so it is the same for any editor asking it.
*/
export function registerLanguageFeatures (host: LanguageHost): vscode.Disposable[] {
    const selector: vscode.DocumentSelector = { language: "yui" };

    const completions: vscode.CompletionItemProvider = {
        async provideCompletionItems (document, position, token) {
            const found = await host.ask (document, "complete", placeOf (document, position), token);

            return records (found?.["items"]).map (item => {
                const completion = new vscode.CompletionItem (text (item["label"]), completionKinds[text (item["kind"])] ?? vscode.CompletionItemKind.Text);

                completion.detail = text (item["detail"]) || undefined;
                completion.documentation = text (item["documentation"]) || undefined;
                return completion;
            });
        }
    };

    const symbols: vscode.DocumentSymbolProvider = {
        async provideDocumentSymbols (document) {
            const outline = await host.outline (document);

            const imports = records (outline?.["imports"]).map (imported => {
                const range = rangeOf (document, imported["range"]);
                return range === undefined ? undefined : new vscode.DocumentSymbol (text (imported["path"]) || "import", "import", vscode.SymbolKind.File, range, range);
            });

            return imports.filter ((symbol): symbol is vscode.DocumentSymbol => symbol !== undefined).concat (symbolsOf (document, outline?.["symbols"]));
        }
    };

    const definitions: vscode.DefinitionProvider = {
        async provideDefinition (document, position, token) {
            const found = await host.ask (document, "definition", placeOf (document, position), token);
            const file = realPathOf (text (found?.["file"]));
            const line = found?.["line"];
            const column = found?.["column"];

            if (file === undefined || typeof line !== "number" || typeof column !== "number")
                return undefined;

            // What is defined in another file is placed by its text when an editor holds it
            const defining = vscode.workspace.textDocuments.find (open => open.uri.scheme === "file" && open.uri.fsPath === file);

            return new vscode.Location (vscode.Uri.file (file), new vscode.Position (line - 1, characterIn (defining, line, column)));
        }
    };

    const hovers: vscode.HoverProvider = {
        async provideHover (document, position, token) {
            const found = await host.ask (document, "hover", placeOf (document, position), token);
            const code = text (found?.["code"]);

            if (found === undefined || code.length === 0)
                return undefined;

            const told = new vscode.MarkdownString ();
            const listed: unknown = found["values"];
            const values = Array.isArray (listed) ? listed.map (value => `\`${String (value)}\``) : [];

            told.appendCodeblock (code, "yui");
            told.appendText (text (found["text"]));

            if (values.length > 0)
                told.appendMarkdown (`\n\n${values.join (", ")}`);

            return new vscode.Hover (told, rangeOf (document, found["range"]));
        }
    };

    const colors: vscode.DocumentColorProvider = {
        async provideDocumentColors (document) {
            const outline = await host.outline (document);

            return records (outline?.["colors"]).flatMap (color => {
                const range = rangeOf (document, color["range"]);
                const written: unknown = color["color"];
                const parts = Array.isArray (written) ? written.map (Number) : [];

                return range === undefined || parts.length !== 4 ? [] : [new vscode.ColorInformation (range, new vscode.Color (parts[0], parts[1], parts[2], parts[3]))];
            });
        },

        // A color is written back as `#rrggbb`, or as `#rrggbbaa` when it is not opaque
        provideColorPresentations (color) {
            const digits = (value: number) => Math.round (Math.min (1, Math.max (0, value)) * 255).toString (16).padStart (2, "0");

            return [new vscode.ColorPresentation ("#" + digits (color.red) + digits (color.green) + digits (color.blue) + (color.alpha < 1 ? digits (color.alpha) : ""))];
        }
    };

    return [
        vscode.languages.registerCompletionItemProvider (selector, completions, ".", ":"),
        vscode.languages.registerDocumentSymbolProvider (selector, symbols),
        vscode.languages.registerDefinitionProvider (selector, definitions),
        vscode.languages.registerHoverProvider (selector, hovers),
        vscode.languages.registerColorProvider (selector, colors)
    ];
}
