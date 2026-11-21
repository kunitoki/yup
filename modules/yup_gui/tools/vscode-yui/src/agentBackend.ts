import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";
import { AgentBackend, AgentToolAnswer, AgentTools } from "./agentServer";
import { collectDocumentFiles, realPathOf } from "./documentFiles";
import { previewIsLight, YuiPreview } from "./preview";
import { errorMessage, toolFilesFolder, YuiTool, YuiToolFile } from "./tool";

export interface AgentBackendHost {
    yuiTool (): YuiTool | undefined;
    modelsPath (document: vscode.TextDocument): string | undefined;
    log (message: string): void;
}

/** What the tool says of its tools: their definitions, and the names of those that draw. */
interface Listing extends AgentTools {
    drawing: string[];
}

/** A call ready for the tool: its arguments with the paths the tool knows the files by, and those files. */
interface Prepared {
    args: Record<string, unknown>;
    files: YuiToolFile[];
}

// The tool capturing the preview looks at what is shown, and so never opens the panel
const captureTool = "yui_capture_preview";

function failure (reason: string): AgentToolAnswer {
    return { text: reason, isError: true, image: "" };
}

// Whether a file is one of the workspace where it really is: a link leading out of it is not
function isInWorkspace (file: string): boolean {
    const real = (target: string) => {
        try {
            return fs.realpathSync (target);
        }
        catch {
            return path.resolve (target);
        }
    };

    const found = real (file);

    return (vscode.workspace.workspaceFolders ?? []).some (folder => {
        const relative = path.relative (real (folder.uri.fsPath), found);
        return relative.length > 0 && ! relative.startsWith ("..") && ! path.isAbsolute (relative);
    });
}

// The paths the tool gives are those of its own files, which mean nothing to who reads its answer
function withRealPaths (text: string): string {
    const isToolPath = (value: string) => value.startsWith (toolFilesFolder);

    const mapped = (value: unknown): unknown => {
        if (typeof value === "string")
            return isToolPath (value) ? realPathOf (value) ?? value : value;

        if (Array.isArray (value))
            return value.map (mapped);

        if (typeof value === "object" && value !== null)
            return Object.fromEntries (Object.entries (value).map (([name, member]) => [name, mapped (member)]));

        return value;
    };

    try {
        return JSON.stringify (mapped (JSON.parse (text)), null, 2);
    }
    catch {
        // What is not JSON is a sentence, where a path ends at a space, a quote or the colon before a line number
        return text.replace (new RegExp (`${toolFilesFolder}[0-9]+/[^\\s:'"]*`, "g"), found => realPathOf (found) ?? found);
    }
}

/**
    Runs the tools of an agent for the server: on the documents as they are in their editors, in the
    worker of the tool, or in the page of the preview for those that draw.
*/
export class YuiAgentBackend implements AgentBackend {
    private listing: Promise<Listing | undefined> | undefined;

    constructor (private readonly host: AgentBackendHost, private readonly preview: YuiPreview) {
    }

    async describe (): Promise<AgentTools> {
        const listing = await this.list ();
        return listing ?? { tools: [], instructions: "" };
    }

    async call (name: string, args: Record<string, unknown>): Promise<AgentToolAnswer> {
        try {
            const tool = this.host.yuiTool ();
            const listing = await this.list ();

            if (tool === undefined || listing === undefined)
                return failure ("the yup_yui_tool module of this installation of the extension is missing or has no tools for agents, see the YUI output channel");

            const prepared = await this.prepare (args);

            if (typeof prepared === "string")
                return failure (prepared);

            const argumentsJson = JSON.stringify (prepared.args);
            const answer = listing.drawing.includes (name)
                ? await this.preview.agentCall (name, argumentsJson, prepared.files, name !== captureTool)
                : await tool.agentCall (name, argumentsJson, prepared.files);

            if (answer === undefined)
                return failure ("the YUI tool stopped, see the YUI output channel");

            const parsed = JSON.parse (answer) as AgentToolAnswer;
            return { text: withRealPaths (parsed.text), isError: parsed.isError === true, image: typeof parsed.image === "string" ? parsed.image : "" };
        }
        catch (error) {
            this.host.log (`the agent tool ${name} failed: ${errorMessage (error)}`);
            return failure (errorMessage (error));
        }
    }

    private async list (): Promise<Listing | undefined> {
        if (this.listing === undefined)
            this.listing = this.fetchListing ();

        const listing = await this.listing;

        // A tool that could not answer is asked again next time
        if (listing === undefined)
            this.listing = undefined;

        return listing;
    }

    private async fetchListing (): Promise<Listing | undefined> {
        const text = await this.host.yuiTool ()?.agentTools ();

        if (text === undefined)
            return undefined;

        try {
            return JSON.parse (text) as Listing;
        }
        catch (error) {
            this.host.log (`the tools for agents could not be read: ${errorMessage (error)}`);
            return undefined;
        }
    }

    // Finds the file a call names in the workspace, absolute or relative to one of its folders
    private resolve (name: string): vscode.Uri | undefined {
        const folders = vscode.workspace.workspaceFolders ?? [];
        const candidates = path.isAbsolute (name) ? [name] : folders.map (folder => path.resolve (folder.uri.fsPath, name));

        // Only the files of the workspace are given to an agent
        const allowed = candidates.filter (isInWorkspace);
        const found = allowed.find (candidate => fs.existsSync (candidate)) ?? allowed[0];

        return found === undefined ? undefined : vscode.Uri.file (found);
    }

    // Gives the tool the document a call names as it is in its editor, with the files it needs, and the call
    // the paths the tool knows them by. A call naming no file is passed on as it is
    private async prepare (args: Record<string, unknown>): Promise<Prepared | string> {
        const name = args["file"];

        if (typeof name !== "string" || name.length === 0)
            return { args, files: [] };

        const uri = this.resolve (name);

        if (uri === undefined)
            return `'${name}' is not a file of the workspace`;

        let document: vscode.TextDocument;

        try {
            document = await vscode.workspace.openTextDocument (uri);
        }
        catch {
            return `can't read '${uri.fsPath}'`;
        }

        const givenModels = args["models"];
        let modelsPath = this.host.modelsPath (document);

        if (typeof givenModels === "string" && givenModels.length > 0) {
            modelsPath = this.resolve (givenModels)?.fsPath;

            if (modelsPath === undefined)
                return `'${givenModels}' is not a file of the workspace`;
        }

        // The files the document names are held to the workspace too. The models of the settings are where the user said
        const collected = await collectDocumentFiles (document, modelsPath, isInWorkspace);

        if (typeof givenModels === "string" && givenModels.length > 0 && collected.models.length === 0)
            return `can't read '${modelsPath}'`;

        const prepared: Record<string, unknown> = { ...args, file: collected.main };

        if (collected.models.length > 0)
            prepared["models"] = collected.models;
        else
            delete prepared["models"];

        // Without a theme, what is drawn is in the one the preview shows
        if (prepared["theme"] === undefined)
            prepared["theme"] = previewIsLight () ? "light" : "dark";

        return { args: prepared, files: collected.files };
    }
}
