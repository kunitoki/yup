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

/** Tells when who asked stopped waiting for the answer, as the token an editor gives with what it asks does. */
export interface YuiToolCancellation {
    readonly isCancellationRequested: boolean;
    onCancellationRequested (listener: () => void): { dispose (): void };
}

/** How a command is asked for, beyond what it runs on. */
export interface YuiToolAsking {
    /** Names what is asked again and again, as the problems of a document: asked once more, what still waits to run under that name is not run. */
    supersedes?: string;

    /** Tells when the answer is not waited for anymore: a command still waiting to run is then not run. */
    cancellation?: YuiToolCancellation;
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

// How long a command is given to answer before the worker is taken to hang, which is what the page of the preview is given
const answerMs = 30000;

// How long a worker that stopped twice in a row waits before it is started again, twice as long for each stop after that
const firstRetryMs = 2000;
const maximumRetryMs = 60000;

// How long a tool that stopped a number of times in a row waits before it is started again
function retryDelayMs (stops: number): number {
    return stops < 2 ? 0 : Math.min (maximumRetryMs, firstRetryMs * 2 ** (stops - 2));
}

/** A command asked for, until the worker answered it or it is known that it won't. */
interface Waiting {
    id: number;
    asked: object;
    files: YuiToolFile[];
    supersedes: string | undefined;
    listening?: { dispose (): void };
    resolve (result: YuiToolResult | undefined): void;
}

/**
    Runs the commands of the tool, a WebAssembly module, in a worker thread.

    The first command starts the worker, which keeps the module and the files it was given for the
    next ones. It is given one command at a time, so those still waiting can be dropped: when what
    they answer is asked again, or is not waited for anymore.

    A worker that stops, or that does not answer a command in time, is started again for the next
    one. One that stops again before it answered anything waits first, longer each time: a module
    that can't be loaded would otherwise be started for every command.
*/
export class YuiTool {
    private worker: Worker | undefined;
    private readonly waiting: Waiting[] = [];
    private running: Waiting | undefined;
    private watchdog: NodeJS.Timeout | undefined;
    private readonly sent = new Map<string, string> ();
    private nextId = 0;
    private stops = 0;
    private retryAt = 0;
    private answered = false;
    private told = false;
    private disposed = false;

    /**
        @param failing Called once, the first time the tool stopped twice in a row and so waits before it starts again.
    */
    constructor (private readonly script: string, private readonly log: (message: string) => void, private readonly failing: () => void = () => undefined) {
    }

    /** Whether the tool can be asked now: not while it waits to start again. */
    isAvailable (): boolean {
        return ! this.disposed && (this.worker !== undefined || Date.now () >= this.retryAt);
    }

    /**
        Lets a tool that waits to start again start with what it is asked next, because what it runs
        on changed and may not stop it anymore. A tool that never answered is left to wait: what
        stops it is not what it is given.
    */
    retry (): void {
        if (this.answered)
            this.retryAt = 0;
    }

    /** Runs a command on the files given, of which only those that changed are sent again. */
    run (args: string[], files: YuiToolFile[] = [], asking: YuiToolAsking = {}): Promise<YuiToolResult | undefined> {
        return this.ask ({ args }, files, asking);
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

    private ask (asked: object, files: YuiToolFile[], asking: YuiToolAsking = {}): Promise<YuiToolResult | undefined> {
        if (! this.isAvailable () || asking.cancellation?.isCancellationRequested === true)
            return Promise.resolve (undefined);

        return new Promise (resolve => {
            const waiting: Waiting = { id: this.nextId++, asked, files, supersedes: asking.supersedes, resolve };

            if (waiting.supersedes !== undefined) {
                for (const earlier of this.waiting.filter (other => other.supersedes === waiting.supersedes))
                    this.drop (earlier);
            }

            waiting.listening = asking.cancellation?.onCancellationRequested (() => this.drop (waiting));
            this.waiting.push (waiting);
            this.next ();
        });
    }

    dispose (): void {
        const worker = this.worker;
        const running = this.running;

        this.disposed = true;
        this.worker = undefined;
        this.running = undefined;
        this.stopWatchdog ();

        if (running !== undefined)
            this.settle (running, undefined);

        this.finish ();

        if (worker !== undefined)
            void worker.terminate ();
    }

    // Gives the worker the command that waited the longest, once it answered the one before
    private next (): void {
        if (this.running !== undefined || this.waiting.length === 0)
            return;

        const worker = this.start ();

        if (worker === undefined) {
            this.finish ();
            return;
        }

        const running = this.waiting.shift () as Waiting;

        this.running = running;
        this.watchdog = setTimeout (() => this.stopped (worker, `it did not answer in ${answerMs / 1000} seconds`), answerMs);

        // The files are those the worker lacks now, which is not what it lacked when the command was asked for
        worker.postMessage ({ ...running.asked, id: running.id, files: unsentFiles (this.sent, running.files) });
    }

    private start (): Worker | undefined {
        if (this.worker !== undefined || ! this.isAvailable ())
            return this.worker;

        const worker = new Worker (path.join (__dirname, "toolWorker.js"), { workerData: { script: this.script } });

        worker.on ("message", (answer: { id: number; error?: string; stopping?: boolean; code: number; stdout: string; stderr: string; missing?: string[] }) => {
            const running = this.running;

            if (this.worker !== worker || running === undefined || running.id !== answer.id)
                return;

            // A worker that tells it ends is not given the commands that wait: they are for the next one
            if (answer.error !== undefined && answer.stopping === true) {
                this.stopped (worker, answer.error);
                return;
            }

            this.running = undefined;
            this.stopWatchdog ();

            if (answer.error !== undefined) {
                this.log (`the YUI tool failed: ${answer.error}`);
            }
            else {
                this.stops = 0;
                this.answered = true;
            }

            this.settle (running, answer.error !== undefined ? undefined : { code: answer.code, stdout: answer.stdout, stderr: answer.stderr, missing: answer.missing ?? [] });
            this.next ();
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

        const running = this.running;
        const delay = retryDelayMs (++this.stops);

        this.worker = undefined;
        this.running = undefined;
        this.retryAt = Date.now () + delay;
        this.stopWatchdog ();
        this.log (`the YUI tool stopped: ${reason}` + (delay > 0 ? `. It is not started again for ${delay / 1000} seconds` : ""));

        // One that hangs, or that told it ends, is still there
        void worker.terminate ();

        if (running !== undefined)
            this.settle (running, undefined);

        if (delay > 0 && ! this.told) {
            this.told = true;
            this.failing ();
        }

        // What it was not given yet is for the next one, when one may start
        this.next ();
    }

    private stopWatchdog (): void {
        if (this.watchdog !== undefined) {
            clearTimeout (this.watchdog);
            this.watchdog = undefined;
        }
    }

    // Takes a command that still waits to run out of those the worker is given
    private drop (waiting: Waiting): void {
        const index = this.waiting.indexOf (waiting);

        if (index >= 0) {
            this.waiting.splice (index, 1);
            this.settle (waiting, undefined);
        }
    }

    private settle (waiting: Waiting, result: YuiToolResult | undefined): void {
        waiting.listening?.dispose ();
        waiting.resolve (result);
    }

    private finish (): void {
        for (const waiting of this.waiting.splice (0))
            this.settle (waiting, undefined);
    }
}

/** Formats an unknown thrown value for user facing messages. */
export function errorMessage (error: unknown): string {
    return error instanceof Error ? error.message : String (error);
}
