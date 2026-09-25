import * as path from "path";
import { ChildProcess, spawn } from "child_process";
import * as vscode from "vscode";
import { errorMessage } from "./compiler";

/** Selected audio/MIDI devices, matching the `yup_dsp_compiler run` options. */
export interface YdspDeviceSelection {
    audioType?: string;
    audioInput?: string;
    audioOutput?: string;
    midiInput?: string;
    midiOutput?: string;
}

export interface YdspPlayerRequest {
    file: string;
    verbose: boolean;
    extraArguments: string[];
    devices: YdspDeviceSelection;
}

export type YdspPlayerStatus = "idle" | "playing" | "stopping";

export interface YdspPlayerState {
    status: YdspPlayerStatus;
    running: boolean;
    file?: string;
}

const STOP_GRACE_PERIOD_MS = 2000;
const TAIL_LENGTH = 4096;

/** Builds the `run` arguments for a request. */
export function buildPlayerArguments (request: YdspPlayerRequest): string[] {
    // Hot reload watches the entry file and everything it imports, so it works
    // for standalone sources and projects alike.
    const result = ["run", request.file, "--hotreload"];

    const { audioType, audioInput, audioOutput, midiInput, midiOutput } = request.devices;

    if (audioType !== undefined && audioType.length > 0)
        result.push ("--audio-type", audioType);

    if (audioOutput !== undefined && audioOutput.length > 0)
        result.push ("--audio-output", audioOutput);

    if (audioInput !== undefined && audioInput.length > 0)
        result.push ("--audio-input", audioInput);

    if (midiInput !== undefined && midiInput.length > 0)
        result.push ("--midi-input", midiInput);

    if (midiOutput !== undefined && midiOutput.length > 0)
        result.push ("--midi-output", midiOutput);

    if (request.verbose)
        result.push ("--verbose");

    result.push (...request.extraArguments);

    return result;
}

/** True when both requests would launch the same player command line. */
export function samePlayerRequest (a: YdspPlayerRequest, b: YdspPlayerRequest): boolean {
    return buildPlayerArguments (a).join ("\u0000") === buildPlayerArguments (b).join ("\u0000");
}

/** Owns the single `yup_dsp_compiler run` subprocess used for patch playback. */
export class YdspPlayer implements vscode.Disposable {
    constructor (private readonly executable: string,
                 private readonly output: vscode.OutputChannel) {
    }

    get status (): YdspPlayerStatus {
        if (this.child === undefined)
            return "idle";

        return this.stopping ? "stopping" : "playing";
    }

    get isRunning (): boolean {
        return this.child !== undefined;
    }

    get currentFile (): string | undefined {
        return this.request?.file;
    }

    get onDidChangeState (): vscode.Event<YdspPlayerState> {
        return this.stateEmitter.event;
    }

    /** True when the given request is already the one being played. */
    isPlayingRequest (request: YdspPlayerRequest): boolean {
        return this.isRunning && this.request !== undefined && samePlayerRequest (this.request, request);
    }

    /**
        Starts playback, replacing any running player once it has shut down. An
        identical request is ignored unless `force` is set, so repeated Run
        actions never spawn a second player.
    */
    start (request: YdspPlayerRequest, force = false): void {
        if (! force && this.isPlayingRequest (request))
            return;

        this.request = request;

        if (this.child !== undefined) {
            this.pending = request;
            this.stop ();
            return;
        }

        this.launch (request);
    }

    /** Asks the player to stop, forcing termination if it does not exit promptly. */
    stop (): void {
        const child = this.child;

        if (child === undefined)
            return;

        this.stopping = true;
        child.kill ("SIGTERM");
        this.emitState ();

        this.killTimer = setTimeout (() => {
            if (this.child === child)
                child.kill ("SIGKILL");
        }, STOP_GRACE_PERIOD_MS);
    }

    dispose (): void {
        this.pending = undefined;
        this.stopping = true;
        this.child?.kill ("SIGTERM");
    }

    private launch (request: YdspPlayerRequest): void {
        const args = buildPlayerArguments (request);
        this.output.appendLine (`> ${this.executable} ${args.join (" ")}`);

        let child: ChildProcess;

        try {
            child = spawn (this.executable, args, { cwd: path.dirname (request.file), windowsHide: true });
        } catch (error) {
            this.output.appendLine (`Cannot start the YDSP player: ${errorMessage (error)}`);
            void vscode.window.showErrorMessage (`YDSP: cannot start the player (${errorMessage (error)}).`);
            this.request = undefined;
            this.emitState ();
            return;
        }

        this.child = child;
        this.stopping = false;
        this.tail = "";

        child.stdout?.setEncoding ("utf8");
        child.stdout?.on ("data", (chunk: string) => this.output.append (chunk));

        child.stderr?.setEncoding ("utf8");
        child.stderr?.on ("data", (chunk: string) => {
            this.output.append (chunk);
            this.tail = (this.tail + chunk).slice (-TAIL_LENGTH);
        });

        child.on ("error", (error) => {
            // A failed spawn emits 'error' and 'close', never 'exit'.
            this.output.appendLine (`Player error: ${errorMessage (error)}`);
            this.tail = errorMessage (error);
            this.handleTermination (child, null, null);
        });

        child.on ("exit", (code, signal) => this.handleTermination (child, code, signal));

        this.emitState ();
    }

    private handleTermination (child: ChildProcess, code: number | null, signal: NodeJS.Signals | null): void {
        if (this.child !== child)
            return;

        this.child = undefined;

        if (this.killTimer !== undefined) {
            clearTimeout (this.killTimer);
            this.killTimer = undefined;
        }

        const stopped = this.stopping;
        this.stopping = false;

        if (! stopped && code !== 0) {
            const detail = this.tail.trim ().split ("\n").pop () ?? "";
            const ending = code !== null ? `code ${code}` : signal !== null ? `signal ${signal}` : "before opening a device";
            this.output.appendLine (`Player exited (${ending}).`);
            this.output.show (true);
            void vscode.window.showErrorMessage (`YDSP: playback stopped${detail.length > 0 ? ` - ${detail}` : ""}.`);
        }

        const pending = this.pending;
        this.pending = undefined;

        if (pending !== undefined) {
            this.launch (pending);
            return;
        }

        this.request = undefined;
        this.emitState ();
    }

    private emitState (): void {
        this.stateEmitter.fire ({ status: this.status, running: this.child !== undefined, file: this.request?.file });
    }

    private readonly stateEmitter = new vscode.EventEmitter<YdspPlayerState> ();
    private child: ChildProcess | undefined;
    private request: YdspPlayerRequest | undefined;
    private pending: YdspPlayerRequest | undefined;
    private killTimer: NodeJS.Timeout | undefined;
    private stopping = false;
    private tail = "";
}
