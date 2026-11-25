import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";
import { isInsideFolders, saveImage, withMember } from "./agentFiles";
import { AgentBackend, AgentToolAnswer, AgentTools } from "./agentServer";
import { collectDocumentFiles, realPathOf, runUntilFilesAreGiven, toolPathOf } from "./documentFiles";
import { previewIsLight, YuiPreview } from "./preview";
import { errorMessage, toolFilesFolder, YuiAgentAnswer, YuiTool, YuiToolFile } from "./tool";

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

    /** The file of the document the call names, when it names one. */
    document?: string;

    /** The file the image of the answer is written to, which the tool can't do as it has no disk. */
    output?: string;
}

/** What a call came to: what the tool answered, for the document and the image of the call, or why it was not asked. */
interface Ran extends YuiAgentAnswer {
    refused?: string;
    document?: string;
    output?: string;
}

const maximumReferenceBytes = 32 * 1024 * 1024;

// The tool capturing the preview looks at what is shown, and so never opens the panel
const captureTool = "yui_capture_preview";

function failure (reason: string): AgentToolAnswer {
    return { text: reason, isError: true, image: "" };
}

// Whether a file is one of the workspace where it really is: a link leading out of it is not
function isInWorkspace (file: string): boolean {
    return isInsideFolders (file, (vscode.workspace.workspaceFolders ?? []).map (folder => folder.uri.fsPath));
}

async function readReference (file: string): Promise<Uint8Array | undefined> {
    try {
        const status = await fs.promises.stat (file);
        return status.isFile () && status.size <= maximumReferenceBytes ? await fs.promises.readFile (file) : undefined;
    }
    catch {
        return undefined;
    }
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
        // An answer told on one line stays on one
        return JSON.stringify (mapped (JSON.parse (text)), null, text.includes ("\n") ? 2 : undefined);
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

            // The tool tells the files it read for the document without having been given them, as what a shader is
            // compiled into: with those there are, it answers again
            const ran = await runUntilFilesAreGiven (async (): Promise<Ran | undefined> => {
                const prepared = await this.prepare (args);

                if (typeof prepared === "string")
                    return { refused: prepared, text: "", missing: [] };

                const argumentsJson = JSON.stringify (prepared.args);
                const answer = listing.drawing.includes (name)
                    ? await this.preview.agentCall (name, argumentsJson, prepared.files, name !== captureTool)
                    : await tool.agentCall (name, argumentsJson, prepared.files);

                return answer === undefined ? undefined : { ...answer, document: prepared.document, output: prepared.output };
            });

            if (ran === undefined)
                return failure ("the YUI tool stopped, see the YUI output channel");

            if (ran.refused !== undefined)
                return failure (ran.refused);

            const parsed = JSON.parse (ran.text) as AgentToolAnswer;
            const image = typeof parsed.image === "string" ? parsed.image : "";
            let text = withRealPaths (parsed.text);

            if (ran.output !== undefined && parsed.isError !== true && image.length > 0) {
                await saveImage (ran.output, image);
                text = withMember (text, "output", ran.output);
            }

            return { text, isError: parsed.isError === true, image };
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
    private async prepare (given: Record<string, unknown>): Promise<Prepared | string> {
        // The image is written here once the tool gave it, so the tool is not told about the file
        const { output: givenOutput, ...args } = given;
        let output: string | undefined;

        if (givenOutput !== undefined) {
            if (typeof givenOutput !== "string" || path.extname (givenOutput).toLowerCase () !== ".png")
                return "'output' is the path of a .png file";

            output = this.resolve (givenOutput)?.fsPath;

            if (output === undefined)
                return `'${givenOutput}' is not a file of the workspace`;
        }

        const name = args["file"];

        if (typeof name !== "string" || name.length === 0)
            return { args, files: [], output };

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

        const givenReference = args["reference"];

        if (typeof givenReference === "string" && givenReference.length > 0) {
            const reference = this.resolve (givenReference)?.fsPath;

            if (reference === undefined)
                return `'${givenReference}' is not a file of the workspace`;

            const data = await readReference (reference);

            if (data === undefined)
                return `can't read '${reference}'`;

            collected.files.push ({ path: toolPathOf (reference), data });
            prepared["reference"] = toolPathOf (reference);
        }

        return { args: prepared, files: collected.files, document: uri.fsPath, output };
    }
}
