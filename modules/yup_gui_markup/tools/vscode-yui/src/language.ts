import * as vscode from "vscode";
import { realPathOf } from "./documentFiles";

/** Who asks the tool what the editor features show of a document, as it is in its editor. */
export interface LanguageHost {
    log (message: string): void;

    /** Asks a command of the tool answering in JSON about a document, with its arguments after the document. */
    ask (document: vscode.TextDocument, command: string, args?: string[]): Promise<Record<string, unknown> | undefined>;

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

// The tool tells a place by its line and its column from 1, and a range by where it starts and where it ends
function rangeOf (value: unknown): vscode.Range | undefined {
    const places = Array.isArray (value) && value.length === 4 && value.every (place => typeof place === "number") ? value as number[] : undefined;

    return places === undefined ? undefined : new vscode.Range (places[0] - 1, places[1] - 1, places[2] - 1, places[3] - 1);
}

function placeOf (position: vscode.Position): string[] {
    return ["--at", `${position.line + 1}:${position.character + 1}`];
}

function symbolOf (symbol: Record<string, unknown>): vscode.DocumentSymbol | undefined {
    const range = rangeOf (symbol["range"]);
    const name = text (symbol["name"]);

    if (range === undefined || name.length === 0)
        return undefined;

    const described = new vscode.DocumentSymbol (name, text (symbol["detail"]), symbolKinds[text (symbol["kind"])] ?? vscode.SymbolKind.Object,
                                                 range, rangeOf (symbol["selection"]) ?? range);

    described.children = symbolsOf (symbol["children"]);
    return described;
}

function symbolsOf (symbols: unknown): vscode.DocumentSymbol[] {
    return records (symbols).map (symbolOf).filter ((symbol): symbol is vscode.DocumentSymbol => symbol !== undefined);
}

/**
    Registers what an editor shows of a `.yui` document beyond its problems: completions, outline, definitions,
    hovers and colors. All of it is what the tool answers, so it is the same for any editor asking it.
*/
export function registerLanguageFeatures (host: LanguageHost): vscode.Disposable[] {
    const selector: vscode.DocumentSelector = { language: "yui" };

    const completions: vscode.CompletionItemProvider = {
        async provideCompletionItems (document, position) {
            const found = await host.ask (document, "complete", placeOf (position));

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
                const range = rangeOf (imported["range"]);
                return range === undefined ? undefined : new vscode.DocumentSymbol (text (imported["path"]) || "import", "import", vscode.SymbolKind.File, range, range);
            });

            return imports.filter ((symbol): symbol is vscode.DocumentSymbol => symbol !== undefined).concat (symbolsOf (outline?.["symbols"]));
        }
    };

    const definitions: vscode.DefinitionProvider = {
        async provideDefinition (document, position) {
            const found = await host.ask (document, "definition", placeOf (position));
            const file = realPathOf (text (found?.["file"]));
            const line = found?.["line"];
            const column = found?.["column"];

            if (file === undefined || typeof line !== "number" || typeof column !== "number")
                return undefined;

            return new vscode.Location (vscode.Uri.file (file), new vscode.Position (line - 1, column - 1));
        }
    };

    const hovers: vscode.HoverProvider = {
        async provideHover (document, position) {
            const found = await host.ask (document, "hover", placeOf (position));
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

            return new vscode.Hover (told, rangeOf (found["range"]));
        }
    };

    const colors: vscode.DocumentColorProvider = {
        async provideDocumentColors (document) {
            const outline = await host.outline (document);

            return records (outline?.["colors"]).flatMap (color => {
                const range = rangeOf (color["range"]);
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
