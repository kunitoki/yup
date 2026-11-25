import * as crypto from "crypto";
import * as fs from "fs";
import * as path from "path";
import { Worker } from "worker_threads";

/** A file given to the tool, at the path it has in the folder of the tool. */
export interface YuiToolFile {
    path: string;
    data: Uint8Array;

    /** True for a file that is no longer there, which the tool forgets. */
    removed?: boolean;
}

export interface YuiToolResult {
    code: number;
    stdout: string;
    stderr: string;

    /** The files the documents it loaded name and the tool was not given, by their paths in the folder of the tool. */
    missing: string[];
}

/** What a tool of an agent answered, in JSON, with the files the document it was run on names and the tool was not given. */
export interface YuiAgentAnswer {
    text: string;
    missing: string[];
}

/** Returns the files a host of the tool does not have as they are now, and remembers it is given them. */
export function unsentFiles (sent: Map<string, string>, files: YuiToolFile[]): YuiToolFile[] {
    return files.filter (file => {
        const digest = file.removed === true ? "removed" : crypto.createHash ("sha1").update (file.data).digest ("hex");

        if (sent.get (file.path) === digest)
            return false;

        sent.set (file.path, digest);
        return true;
    });
}

/** The folder the tool keeps the files it is given in, which the paths it prints start with. */
export const toolFilesFolder = "/yui/";

const moduleName = "yup_yui_tool";

/** Returns the script of the tool built for the web, which an extension packaged without it does not have. */
export function findToolScript (extensionPath: string): string | undefined {
    const folder = path.join (extensionPath, "tool");
    const found = [".js", ".wasm"].every (extension => fs.existsSync (path.join (folder, moduleName + extension)));

    return found ? path.join (folder, moduleName + ".js") : undefined;
}

/**
    Runs the commands of the tool, a WebAssembly module, in a worker thread.

    The first command starts the worker, which keeps the module and the files it was given for the
    next ones. A worker that stops is started again, but not one that stops twice in a row: a module
    that can't be loaded would otherwise be started for every command.
*/
export class YuiTool {
    private worker: Worker | undefined;
    private readonly waiting = new Map<number, (result: YuiToolResult | undefined) => void> ();
    private readonly sent = new Map<string, string> ();
    private nextId = 0;
    private stops = 0;

    constructor (private readonly script: string, private readonly log: (message: string) => void) {
    }

    isAvailable (): boolean {
        return this.stops < 2;
    }

    /** Runs a command on the files given, of which only those that changed are sent again. */
    run (args: string[], files: YuiToolFile[] = []): Promise<YuiToolResult | undefined> {
        return this.ask ({ args }, files);
    }

    /** Returns the tools of an agent as the tool describes them, in JSON. */
    async agentTools (): Promise<string | undefined> {
        return (await this.ask ({ agentTools: true }, []))?.stdout;
    }

    /** Runs a tool of an agent on the files given, with its arguments as JSON, and returns its answer in JSON. */
    async agentCall (name: string, argumentsJson: string, files: YuiToolFile[]): Promise<YuiAgentAnswer | undefined> {
        const result = await this.ask ({ agentTool: name, agentArguments: argumentsJson }, files);

        return result === undefined ? undefined : { text: result.stdout, missing: result.missing };
    }

    private ask (asked: object, files: YuiToolFile[]): Promise<YuiToolResult | undefined> {
        const worker = this.start ();

        if (worker === undefined)
            return Promise.resolve (undefined);

        const changed = unsentFiles (this.sent, files);

        return new Promise (resolve => {
            const id = this.nextId++;

            this.waiting.set (id, resolve);
            worker.postMessage ({ ...asked, id, files: changed });
        });
    }

    dispose (): void {
        const worker = this.worker;

        this.worker = undefined;
        this.stops = 2;
        this.finish (undefined);

        if (worker !== undefined)
            void worker.terminate ();
    }

    private start (): Worker | undefined {
        if (this.worker !== undefined || ! this.isAvailable ())
            return this.worker;

        const worker = new Worker (path.join (__dirname, "toolWorker.js"), { workerData: { script: this.script } });

        worker.on ("message", (answer: { id: number; error?: string; code: number; stdout: string; stderr: string; missing?: string[] }) => {
            const resolve = this.waiting.get (answer.id);

            this.waiting.delete (answer.id);

            if (answer.error !== undefined)
                this.log (`the YUI tool failed: ${answer.error}`);
            else
                this.stops = 0;

            resolve?.(answer.error !== undefined ? undefined : { code: answer.code, stdout: answer.stdout, stderr: answer.stderr, missing: answer.missing ?? [] });
        });

        worker.on ("error", error => this.stopped (worker, error.message));
        worker.on ("exit", code => this.stopped (worker, `it exited with code ${code}`));

        this.sent.clear ();
        this.worker = worker;
        return worker;
    }

    private stopped (worker: Worker, reason: string): void {
        if (this.worker !== worker)
            return;

        this.worker = undefined;
        ++this.stops;
        this.log (`the YUI tool stopped: ${reason}`);
        this.finish (undefined);
    }

    private finish (result: YuiToolResult | undefined): void {
        for (const resolve of this.waiting.values ())
            resolve (result);

        this.waiting.clear ();
    }
}

/** Formats an unknown thrown value for user facing messages. */
export function errorMessage (error: unknown): string {
    return error instanceof Error ? error.message : String (error);
}
