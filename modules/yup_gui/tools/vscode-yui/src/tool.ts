import * as crypto from "crypto";
import * as fs from "fs";
import * as path from "path";
import { Worker } from "worker_threads";

/** A file given to the tool, at the path it has in the folder of the tool. */
export interface YuiToolFile {
    path: string;
    data: Uint8Array;
}

export interface YuiToolResult {
    code: number;
    stdout: string;
    stderr: string;
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
        const worker = this.start ();

        if (worker === undefined)
            return Promise.resolve (undefined);

        const changed = files.filter (file => {
            const digest = crypto.createHash ("sha1").update (file.data).digest ("hex");

            if (this.sent.get (file.path) === digest)
                return false;

            this.sent.set (file.path, digest);
            return true;
        });

        return new Promise (resolve => {
            const id = this.nextId++;

            this.waiting.set (id, resolve);
            worker.postMessage ({ id, args, files: changed });
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

        worker.on ("message", (answer: { id: number; error?: string; code: number; stdout: string; stderr: string }) => {
            const resolve = this.waiting.get (answer.id);

            this.waiting.delete (answer.id);

            if (answer.error !== undefined)
                this.log (`the YUI tool failed: ${answer.error}`);
            else
                this.stops = 0;

            resolve?.(answer.error !== undefined ? undefined : { code: answer.code, stdout: answer.stdout, stderr: answer.stderr });
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
