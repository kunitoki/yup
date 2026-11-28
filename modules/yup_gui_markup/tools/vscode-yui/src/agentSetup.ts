import * as crypto from "crypto";
import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";
import { AgentBackend, AgentServer } from "./agentServer";
import { errorMessage } from "./tool";
import { FileInGit, gitRunner, keepOutOfCommits } from "./versionControl";

const defaultPort = 7391;
const portsToTry = 10;
const serverName = "yui";
const configFile = ".mcp.json";
const tokenSecret = "yui.mcp.token";

function isPortTaken (error: unknown): boolean {
    return typeof error === "object" && error !== null && (error as { code?: unknown }).code === "EADDRINUSE";
}

function asRecord (value: unknown): Record<string, unknown> {
    return typeof value === "object" && value !== null && ! Array.isArray (value) ? value as Record<string, unknown> : {};
}

// Makes a file one only its owner reads and writes, making it first when it is not there
async function keepPrivate (file: string): Promise<void> {
    const handle = await fs.promises.open (file, "a", 0o600);

    try {
        await handle.chmod (0o600);
    }
    finally {
        await handle.close ();
    }
}

/** A file the server is listed in with its token, and whether git keeps that file out of the commits of its repository. */
interface Listing {
    file: string;
    keptOut: boolean;
}

// Tells who looks after the file holding the token: git, or its user
function careOf (listing: Listing): string {
    return listing.keptOut ? "That file is kept out of the commits of this repository." : "Keep that file out of version control.";
}

/**
    Serves the tools for agents while the `yui.mcp.enabled` setting is on and the workspace is
    trusted, and sets a workspace up for them: the setting, and the entry of the server in the
    `.mcp.json` its agents read, with the token the server answers for. That file is first kept
    out of the commits of its repository, and is not written when git commits it.
*/
export class YuiAgentTools implements vscode.Disposable {
    private server: AgentServer | undefined;
    private pending: Promise<void> = Promise.resolve ();
    private settingUp = false;
    private disposed = false;

    /**
        @param secrets Where the token of the server is kept from one run of the editor to the next.
    */
    constructor (private readonly backend: AgentBackend, private readonly version: string, private readonly log: (message: string) => void,
                 private readonly secrets: vscode.SecretStorage) {
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
        // The settings and the files of a workspace that is not trusted are left as they are
        if (! vscode.workspace.isTrusted) {
            void vscode.window.showInformationMessage ("Trust this workspace to serve the YUI tools for agents from it.");
            return;
        }

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

        const written = await this.writeConfig (folder, this.server);

        if (written !== undefined)
            void vscode.window.showInformationMessage (`The YUI tools for agents are served at ${this.server.url} and listed in ${written.file}, where the agents started in this workspace find them with the token they are answered for. ${careOf (written)}`);
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

        // A workspace that is not trusted is not served, whoever turned the setting on
        if (configuration.get<boolean> ("enabled") !== true || ! vscode.workspace.isTrusted) {
            await this.server?.stop ();
            this.server = undefined;
            return undefined;
        }

        if (this.server !== undefined && (this.server.port === wanted || mayChangePort || this.settingUp))
            return this.server.port;

        await this.server?.stop ();
        this.server = undefined;

        const token = await this.token ();

        for (let port = wanted; port < wanted + (mayChangePort ? portsToTry : 1); ++port) {
            try {
                const started = await AgentServer.start (this.backend, port, this.version, token);

                // The extension may have been turned off while the server started
                if (this.disposed) {
                    await started.stop ();
                    return undefined;
                }

                this.server = started;
                this.log (`the tools for agents are served at ${started.url}`);

                // The command lists the server itself, once the port it found is in the settings
                if (! this.settingUp)
                    await this.refreshConfigs (started);

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

    // The token the server answers for: made once and kept with the secrets of the editor, so every window of the
    // user has the same and no other user has it
    private async token (): Promise<string> {
        const kept = await this.secrets.get (tokenSecret);

        if (kept !== undefined && kept.length > 0)
            return kept;

        const made = crypto.randomBytes (32).toString ("hex");

        await this.secrets.store (tokenSecret, made);
        return made;
    }

    // A workspace set up before the server asked for a token, or with another token, lists the server without what
    // it now asks for. Its file is brought up to date, as long as the server it lists is this very one
    private async refreshConfigs (server: AgentServer): Promise<void> {
        for (const folder of vscode.workspace.workspaceFolders ?? []) {
            if (! await this.listsWithoutToken (folder, server))
                continue;

            const written = await this.writeConfig (folder, server);

            if (written !== undefined)
                void vscode.window.showInformationMessage (`${written.file} now holds the token the YUI tools for agents are answered for: agents started before need to read it again. ${careOf (written)}`);
        }
    }

    private async listsWithoutToken (folder: vscode.WorkspaceFolder, server: AgentServer): Promise<boolean> {
        try {
            const text = Buffer.from (await vscode.workspace.fs.readFile (vscode.Uri.joinPath (folder.uri, configFile))).toString ("utf8");
            const entry = asRecord (asRecord (asRecord (JSON.parse (text))["mcpServers"])[serverName]);

            return entry["url"] === server.url && asRecord (entry["headers"])["Authorization"] !== server.headers["Authorization"];
        }
        catch {
            // A file that is not there, or that can't be read, lists nothing to bring up to date
            return false;
        }
    }

    // Asks git what it makes of the file the token is about to be written to, and to keep it out of its commits.
    // Returns nothing when git could not tell or could not be told: the file is then handled as one it commits
    private async askGit (folder: vscode.WorkspaceFolder, uri: vscode.Uri): Promise<FileInGit | undefined> {
        // Git is only asked about a folder on disk
        if (uri.scheme !== "file")
            return "outside";

        try {
            const found = await keepOutOfCommits (folder.uri.fsPath, uri.fsPath, gitRunner ());

            if (found === "excluded")
                this.log (`${uri.fsPath} is kept out of the commits of its repository by a rule of this clone, in the info/exclude of git`);

            return found;
        }
        catch (error) {
            this.log (`git could not be told to keep ${uri.fsPath} out of its commits: ${errorMessage (error)}`);
            return undefined;
        }
    }

    // The token is not written to a file git commits, or may commit: its user is given the entry of the server
    // instead, to list it where no commit takes it
    private async offerEntry (file: string, inGit: FileInGit | undefined, entry: string): Promise<void> {
        const copy = "Copy Entry";
        const why = inGit === "tracked" ? "is under version control here"
                  : inGit === "shared" ? "is kept among the files to commit by the rules of this repository"
                  : "could not be kept out of the commits of its repository, see the YUI output channel";
        const chosen = await vscode.window.showWarningMessage (`${file} ${why}, so the token the YUI tools for agents are answered for is not written to it. Copy the entry of the server and add it to your own agent configuration, outside the repository.`, copy);

        if (chosen === copy)
            await vscode.env.clipboard.writeText (entry);
    }

    // Adds the server to the file agents read, keeping what it holds. Returns the file, or nothing when it is left alone
    private async writeConfig (folder: vscode.WorkspaceFolder, server: AgentServer): Promise<Listing | undefined> {
        const uri = vscode.Uri.joinPath (folder.uri, configFile);
        const entry = { type: "http", url: server.url, headers: server.headers };
        const inGit = await this.askGit (folder, uri);

        if (inGit === undefined || inGit === "tracked" || inGit === "shared") {
            void this.offerEntry (uri.fsPath, inGit, `"${serverName}": ${JSON.stringify (entry)}`);
            return undefined;
        }

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

        // It holds the token from now on, which is kept from the other users of this machine: before it is written,
        // and after in case writing it made another file of it
        const guard = async () => {
            if (uri.scheme === "file")
                await keepPrivate (uri.fsPath).catch (error => this.log (`${uri.fsPath} could not be kept from the other users of this machine: ${errorMessage (error)}`));
        };

        try {
            await guard ();
            await vscode.workspace.fs.writeFile (uri, Buffer.from (JSON.stringify (config, null, indent) + "\n", "utf8"));
            await guard ();
        }
        catch (error) {
            void vscode.window.showErrorMessage (`${uri.fsPath} could not be written: ${errorMessage (error)}`);
            return undefined;
        }

        return { file: uri.fsPath, keptOut: inGit !== "outside" };
    }
}
