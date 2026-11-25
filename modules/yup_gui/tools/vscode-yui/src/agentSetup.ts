import * as path from "path";
import * as vscode from "vscode";
import { AgentBackend, AgentServer } from "./agentServer";
import { errorMessage } from "./tool";

const defaultPort = 7391;
const portsToTry = 10;
const serverName = "yui";
const configFile = ".mcp.json";

function isPortTaken (error: unknown): boolean {
    return typeof error === "object" && error !== null && (error as { code?: unknown }).code === "EADDRINUSE";
}

/**
    Serves the tools for agents while the `yui.mcp.enabled` setting is on, and sets a workspace up
    for them: the setting, and the entry of the server in the `.mcp.json` its agents read.
*/
export class YuiAgentTools implements vscode.Disposable {
    private server: AgentServer | undefined;
    private pending: Promise<void> = Promise.resolve ();
    private settingUp = false;
    private disposed = false;

    constructor (private readonly backend: AgentBackend, private readonly version: string, private readonly log: (message: string) => void) {
    }

    dispose (): void {
        this.disposed = true;
        void this.server?.stop ();
        this.server = undefined;
    }

    /** Starts, stops or moves the server to follow the settings. */
    async apply (): Promise<void> {
        await this.enqueue (false);
    }

    // The settings are followed one change at a time, as starting and stopping the server takes a while
    private enqueue (mayChangePort: boolean): Promise<number | undefined> {
        const followed = this.pending.then (() => this.follow (mayChangePort));

        this.pending = followed.then (() => undefined, error => this.log (`the tools for agents could not follow their settings: ${errorMessage (error)}`));
        return followed.catch (() => undefined);
    }

    /** The `YUI: Set Up Agent Tools` command. */
    async setUp (): Promise<void> {
        const folder = await this.chooseFolder ();

        if (folder === undefined)
            return;

        // While this lasts the server is kept on the port found for it, whatever the settings said when it was asked for
        this.settingUp = true;

        try {
            await this.setUpIn (folder);
        }
        catch (error) {
            void vscode.window.showErrorMessage (`The YUI tools for agents could not be set up: ${errorMessage (error)}`);
        }
        finally {
            this.settingUp = false;
        }
    }

    private async setUpIn (folder: vscode.WorkspaceFolder): Promise<void> {
        const configuration = vscode.workspace.getConfiguration ("yui.mcp", folder.uri);

        await configuration.update ("enabled", true, vscode.ConfigurationTarget.Workspace);

        // The port of the settings, or one of the next ones when another window has it
        const port = await this.enqueue (true);

        if (this.server === undefined || port === undefined) {
            void vscode.window.showErrorMessage ("The YUI tools for agents could not be served, see the YUI output channel.");
            return;
        }

        if (port !== configuration.get<number> ("port"))
            await configuration.update ("port", port, vscode.ConfigurationTarget.Workspace);

        const written = await this.writeConfig (folder, this.server.url);

        if (written !== undefined)
            void vscode.window.showInformationMessage (`The YUI tools for agents are served at ${this.server.url} and listed in ${written}, where the agents started in this workspace find them.`);
    }

    private async chooseFolder (): Promise<vscode.WorkspaceFolder | undefined> {
        const folders = vscode.workspace.workspaceFolders ?? [];

        if (folders.length === 0) {
            void vscode.window.showInformationMessage ("Open the folder of a project to set the YUI tools for agents up in it.");
            return undefined;
        }

        return folders.length === 1 ? folders[0] : vscode.window.showWorkspaceFolderPick ({ placeHolder: "The folder agents are started in" });
    }

    // Returns the port served, when the server runs
    private async follow (mayChangePort: boolean): Promise<number | undefined> {
        const configuration = vscode.workspace.getConfiguration ("yui.mcp");
        const wanted = configuration.get<number> ("port") ?? defaultPort;

        if (this.disposed)
            return undefined;

        if (configuration.get<boolean> ("enabled") !== true) {
            await this.server?.stop ();
            this.server = undefined;
            return undefined;
        }

        if (this.server !== undefined && (this.server.port === wanted || mayChangePort || this.settingUp))
            return this.server.port;

        await this.server?.stop ();
        this.server = undefined;

        for (let port = wanted; port < wanted + (mayChangePort ? portsToTry : 1); ++port) {
            try {
                const started = await AgentServer.start (this.backend, port, this.version);

                // The extension may have been turned off while the server started
                if (this.disposed) {
                    await started.stop ();
                    return undefined;
                }

                this.server = started;
                this.log (`the tools for agents are served at ${started.url}`);
                return port;
            }
            catch (error) {
                this.log (`the tools for agents can't be served on port ${port}: ${errorMessage (error)}`);

                if (! isPortTaken (error))
                    break;
            }
        }

        if (! mayChangePort && ! this.settingUp)
            void this.offerSetUp (wanted);

        return undefined;
    }

    // Another window serving its own workspace has the port: the command gives this one another
    private async offerSetUp (port: number): Promise<void> {
        const again = "Set Up Again";
        const chosen = await vscode.window.showWarningMessage (`The YUI tools for agents can't be served on port ${port}, which may be taken by another window.`, again);

        if (chosen === again)
            await this.setUp ();
    }

    // Adds the server to the file agents read, keeping what it holds. Returns the file, or nothing when it is left alone
    private async writeConfig (folder: vscode.WorkspaceFolder, url: string): Promise<string | undefined> {
        const uri = vscode.Uri.joinPath (folder.uri, configFile);
        const entry = { type: "http", url };
        let text = "";

        try {
            text = Buffer.from (await vscode.workspace.fs.readFile (uri)).toString ("utf8");
        }
        catch (error) {
            // Only a file that is not there is written from nothing: one that can't be read keeps what it holds
            if (! (error instanceof vscode.FileSystemError) || error.code !== "FileNotFound") {
                void vscode.window.showErrorMessage (`${uri.fsPath} could not be read (${errorMessage (error)}). Add the server to its "mcpServers" by hand: "${serverName}": ${JSON.stringify (entry)}`);
                return undefined;
            }
        }

        let config: Record<string, unknown> = {};

        if (text.trim ().length > 0) {
            try {
                const parsed: unknown = JSON.parse (text);

                if (typeof parsed !== "object" || parsed === null || Array.isArray (parsed))
                    throw new Error ("it is not an object");

                config = parsed as Record<string, unknown>;
            }
            catch (error) {
                void vscode.window.showErrorMessage (`${path.basename (uri.fsPath)} could not be read (${errorMessage (error)}). Add the server to its "mcpServers" by hand: "${serverName}": ${JSON.stringify (entry)}`);
                return undefined;
            }
        }

        const servers = config["mcpServers"];
        config["mcpServers"] = { ...(typeof servers === "object" && servers !== null ? servers : {}), [serverName]: entry };

        // The file keeps the indentation it has
        const indent = /\n([ \t]+)"/.exec (text)?.[1] ?? "  ";

        try {
            await vscode.workspace.fs.writeFile (uri, Buffer.from (JSON.stringify (config, null, indent) + "\n", "utf8"));
        }
        catch (error) {
            void vscode.window.showErrorMessage (`${uri.fsPath} could not be written: ${errorMessage (error)}`);
            return undefined;
        }

        return uri.fsPath;
    }
}
