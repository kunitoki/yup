// What the script of the page of the preview finds beside the page itself: the values previewPage.ts writes before
// it, the API of VS Code and the tool. It is only read by `tsc -p page`, which checks the script against it.

/** The values the page is set up with, written before the script by `previewPage ()`. */
declare const settings: {
    /** Where the tool is, or nothing in an extension packaged without it. */
    module: { script: string; folder: string } | null;

    /** What the script loading the tool is allowed by. */
    nonce: string;

    /** The largest width or height of the preview, in logical pixels. */
    maximumSize: number;

    /** How many times larger than its size 'fit' shows something small. */
    maximumFit: number;

    /** The space kept around the interface, in pixels. */
    contentPadding: number;
};

/** The API VS Code gives the pages it shows, which a page takes once. */
declare function acquireVsCodeApi (): {
    postMessage (message: unknown): void;
    getState (): any;
    setState (state: unknown): void;
};

/** Creates the tool, which the script of the tool declares once it is loaded. */
declare function createYuiTool (options: YuiToolOptions): Promise<YuiTool>;

/** What the page gives the tool it creates: its canvas, and what the tool tells it. */
interface YuiToolOptions {
    canvas: HTMLCanvasElement;
    locateFile (name: string): string;
    print (text: string): void;
    printErr (text: string): void;
    onAbort (reason: unknown): void;
    yuiPreviewShown (outcome: Outcome): void;
    yuiPreviewFramed (x: number, y: number, width: number, height: number): number;
    yuiPreviewModelChanged (model: string, property: string, value: any): void;
    yuiPreviewDrawn (problem: string): void;
    yuiPreviewReady (): void;
}

/** The tool once it is ready: what it was created with, to which it added what it exports. */
interface YuiTool extends YuiToolOptions {
    wasmMemory: { buffer: ArrayBuffer };
    ccall (name: string, returns: string | null, types: string[], values: unknown[]): any;
    _yuiToolAllocate (size: number): number;
    _yuiPreviewSetView (width: number, height: number): void;
    _yuiPreviewSetGuides (shown: number): void;
    _yuiPreviewSetCaret (line: number, column: number): void;
    _yuiPreviewSetStats (measured: number): void;
}

/** A file given to the tool, at the path it has in the folder of the tool. */
interface ToolFile {
    path: string;
    data: Uint8Array;
    removed?: boolean;
}

/** A choice of a list of the toolbar, those of a group under its name. */
interface Choice {
    id: string;
    label: string;
    group?: string;
}

/** What the toolbar offers and shows as chosen. */
interface StateMessage {
    targets: Choice[];
    target: string;
    hosts: string[];
    shownAs: string;
    size: string;
    theme: string;
    zoom: string;
}

/** What to show: the files of a document, and what of it. */
interface ShowMessage {
    main: string;
    models: string;
    modelsFile: string;
    modelsValues: any;
    files: ToolFile[];
    notes: string[];
    drawing: string;
    palette: string;
    host: string;
    use: string;
    interface: boolean;
    light: boolean;
    size: string;
    zoom: string;
}

/** A tool of an agent to run on the files given with it. */
interface AgentCallMessage {
    id: number;
    name: string;
    arguments: string;
    files: ToolFile[];
}

/** A component of the interface, as the tool tells it. */
interface Use {
    type: string;
    id?: string;
    path: string;
}

/** What the tool tells of what it was asked to show. */
interface Outcome {
    status: string;
    message: string;
    problems: { file: string; line: number; column: number; severity: string; message: string }[];
    hasInterface: boolean;
    width: number;
    height: number;
    defaultWidth: number;
    defaultHeight: number;
    inContext: boolean;
    uses: Use[];
    use: string;
    needsHost: boolean;
    models: Record<string, Record<string, any>>;
}
