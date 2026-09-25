import * as path from "path";
import * as vscode from "vscode";
import { LanguageClient, LanguageClientOptions, ServerOptions, Trace } from "vscode-languageclient/node";
import { errorMessage, resolveCompilerPath } from "./compiler";
import { listYdspDevices, YdspAudioDevice, YdspDevices, YdspMidiDevice } from "./devices";
import { YdspChoice, YdspChoiceKey, YdspPanelProvider, YdspPanelState, WebviewMessage } from "./panel";
import { YdspDeviceSelection, YdspPlayer, YdspPlayerRequest, YdspPlayerState } from "./player";

let client: LanguageClient | undefined;

const PATCH_EXTENSIONS = [".ydsp", ".ydsp-project"];
const DEVICE_STATE_KEY = "ydsp.player.devices";
const ACTIVE_CONTEXT_KEY = "ydsp.playerActive";
const EXCLUDED_FILES = "**/{node_modules,.git,build}/**";
const DEVICE_CACHE_MS = 30000;
const PATCH_CACHE_MS = 5000;
const FOLLOW_DEBOUNCE_MS = 300;
const REFRESH_DEBOUNCE_MS = 250;
const NO_DEVICE_VALUE = "";

interface PatchQuickPick extends vscode.QuickPickItem {
    uri: vscode.Uri;
}

interface ActionQuickPick extends vscode.QuickPickItem {
    action: "run" | "stop" | "restart" | "devices" | "panel";
}

function isPatchFile (fsPath: string): boolean {
    return PATCH_EXTENSIONS.some (extension => fsPath.endsWith (extension));
}

function unique (values: string[]): string[] {
    return [...new Set (values)];
}

function emptyDeviceChoices (): YdspPanelState["devices"] {
    return { audioTypes: [], audioOutputs: [], audioInputs: [], midiInputs: [], midiOutputs: [] };
}

function describeAudio (devices: YdspAudioDevice[], direction: "input" | "output"): YdspChoice[] {
    const seen = new Set<string> ();
    const result: YdspChoice[] = [];

    for (const device of devices.filter (device => device.direction === direction)) {
        const key = `${device.type}|${device.name}`;

        if (seen.has (key))
            continue;

        seen.add (key);
        result.push ({ label: device.name, value: device.name, detail: device.type });
    }

    return result;
}

function describeMidi (devices: YdspMidiDevice[], direction: "input" | "output"): YdspChoice[] {
    const seen = new Set<string> ();
    const result: YdspChoice[] = [{ label: "Disabled", value: NO_DEVICE_VALUE }];

    for (const device of devices.filter (device => device.direction === direction)) {
        // The identifier is a CoreMIDI object id chain ("-1665950543 580703631"):
        // the player matches identifiers before names, so it stays the selected
        // value, but it is unreadable and only the name is listed.
        if (device.name.length === 0 || seen.has (device.name))
            continue;

        seen.add (device.name);

        result.push ({ label: device.name, value: device.id.length > 0 ? device.id : device.name });
    }

    return result;
}

export function activate (context: vscode.ExtensionContext): void {
    const output = vscode.window.createOutputChannel ("YDSP");
    context.subscriptions.push (output);
    output.appendLine ("Activating YDSP language support");

    const executable = resolveCompilerPath (context);

    const serverOptions: ServerOptions = { command: executable, args: ["--lsp"] };
    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: "file", language: "ydsp" }, { scheme: "file", pattern: "**/*.ydsp-project" }],
        synchronize: { fileEvents: vscode.workspace.createFileSystemWatcher ("**/*.{ydsp,ydsp-project}") }
    };

    client = new LanguageClient ("ydsp", "YDSP Language Server", serverOptions, clientOptions);
    const trace = vscode.workspace.getConfiguration ("ydsp.server").get<string> ("trace", "off");
    client.setTrace (trace === "verbose" ? Trace.Verbose : trace === "messages" ? Trace.Messages : Trace.Off);
    client.start ().then (() => output.appendLine (`YDSP language server started: ${executable}`)).catch (() => {
        output.appendLine (`YDSP language server could not start: ${executable}`);
        void vscode.window.showErrorMessage (`YDSP language server could not start: ${executable}`);
    });
    context.subscriptions.push ({ dispose: () => client?.stop () });

    const playback = vscode.window.createOutputChannel ("YDSP Playback");
    const player = new YdspPlayer (executable, playback);
    context.subscriptions.push (playback, player);

    const status = vscode.window.createStatusBarItem (vscode.StatusBarAlignment.Left, 100);
    status.command = "ydsp.playerMenu";
    context.subscriptions.push (status);

    void vscode.commands.executeCommand ("setContext", ACTIVE_CONTEXT_KEY, false);

    let deviceCache: { devices: YdspDevices; time: number } | undefined;
    let patchCache: { files: vscode.Uri[]; time: number } | undefined;
    let pendingTestNote: number | undefined;
    let followTimer: NodeJS.Timeout | undefined;
    let refreshTimer: NodeJS.Timeout | undefined;
    let refreshSequence = 0;

    const readDevices = (): YdspDeviceSelection => context.workspaceState.get<YdspDeviceSelection> (DEVICE_STATE_KEY, {});
    const writeDevices = (selection: YdspDeviceSelection): Thenable<void> => context.workspaceState.update (DEVICE_STATE_KEY, selection);

    const getDevices = async (force = false): Promise<YdspDevices> => {
        const now = Date.now ();

        if (! force && deviceCache !== undefined && now - deviceCache.time < DEVICE_CACHE_MS)
            return deviceCache.devices;

        const devices = await listYdspDevices (executable);
        deviceCache = { devices, time: now };
        return devices;
    };

    const findPatches = async (force = false): Promise<vscode.Uri[]> => {
        const now = Date.now ();

        if (! force && patchCache !== undefined && now - patchCache.time < PATCH_CACHE_MS)
            return patchCache.files;

        const projects = await vscode.workspace.findFiles ("**/*.ydsp-project", EXCLUDED_FILES);
        const sources = await vscode.workspace.findFiles ("**/*.ydsp", EXCLUDED_FILES);
        const files = [...projects, ...sources];

        // findFiles order is not guaranteed, so order the list the way the panel
        // shows it (natural, numeric-aware) and keep it stable across refreshes.
        files.sort ((left, right) => vscode.workspace.asRelativePath (left).localeCompare (vscode.workspace.asRelativePath (right), undefined, { numeric: true, sensitivity: "base" }));

        patchCache = { files, time: now };
        return files;
    };

    const describeDevices = (devices: YdspDevices): YdspPanelState["devices"] => ({
        audioTypes: [{ label: "Default (system)", value: NO_DEVICE_VALUE }, ...unique (devices.audio.map (device => device.type)).map (type => ({ label: type, value: type }))],
        audioOutputs: [{ label: "Default device", value: NO_DEVICE_VALUE }, ...describeAudio (devices.audio, "output")],
        audioInputs: [{ label: "Default device", value: NO_DEVICE_VALUE }, { label: "No input (feeds silence)", value: "none" }, ...describeAudio (devices.audio, "input")],
        midiInputs: describeMidi (devices.midi, "input"),
        midiOutputs: describeMidi (devices.midi, "output")
    });

    const dirtyPatches = (): string[] =>
        vscode.workspace.textDocuments
            .filter (document => document.isDirty && isPatchFile (document.uri.fsPath))
            .map (document => vscode.workspace.asRelativePath (document.uri));

    const makeRequest = (file: string): YdspPlayerRequest => {
        const configuration = vscode.workspace.getConfiguration ("ydsp.player");
        const extra = [...configuration.get<string[]> ("arguments", [])];
        const sampleRate = configuration.get<number> ("sampleRate", 0);
        const blockSize = configuration.get<number> ("blockSize", 0);
        const note = pendingTestNote;
        pendingTestNote = undefined;

        if (sampleRate > 0)
            extra.push ("--sample-rate", String (sampleRate));

        if (blockSize > 0)
            extra.push ("--block-size", String (blockSize));

        if (note !== undefined)
            extra.push ("--test-note", String (note));

        return {
            file,
            verbose: configuration.get<boolean> ("verbose", false),
            extraArguments: extra,
            devices: readDevices ()
        };
    };

    const collectState = async (): Promise<YdspPanelState> => {
        const configuration = vscode.workspace.getConfiguration ("ydsp.player");
        const playing = player.currentFile;
        let devices = emptyDeviceChoices ();

        try {
            devices = describeDevices (await getDevices ());
        } catch (error) {
            devices = { ...emptyDeviceChoices (), error: errorMessage (error) };
        }

        return {
            status: player.status,
            playing,
            playingLabel: playing !== undefined ? vscode.workspace.asRelativePath (playing) : undefined,
            patches: (await findPatches ()).map (uri => ({ path: uri.fsPath, label: vscode.workspace.asRelativePath (uri), playing: uri.fsPath === playing })),
            unsaved: dirtyPatches (),
            devices,
            selection: readDevices (),
            sampleRate: configuration.get<number> ("sampleRate", 0),
            blockSize: configuration.get<number> ("blockSize", 0),
            verbose: configuration.get<boolean> ("verbose", false),
            follow: configuration.get<boolean> ("followActiveEditor", false)
        };
    };

    const refresh = async (): Promise<void> => {
        const sequence = ++refreshSequence;
        const state = await collectState ();

        // Ignore results that a newer refresh has already superseded.
        if (sequence !== refreshSequence)
            return;

        panel.post ({ type: "state", state });
    };

    const scheduleRefresh = (): void => {
        if (refreshTimer !== undefined)
            clearTimeout (refreshTimer);

        refreshTimer = setTimeout (() => {
            refreshTimer = undefined;
            void refresh ();
        }, REFRESH_DEBOUNCE_MS);
    };

    const setSelection = (selection: YdspDeviceSelection, key: keyof YdspDeviceSelection, value: string): void => {
        selection[key] = value.length === 0 ? undefined : value;
    };

    const resolvePlaybackFile = async (resource?: vscode.Uri): Promise<string | undefined> => {
        if (resource !== undefined && resource.scheme === "file" && isPatchFile (resource.fsPath))
            return resource.fsPath;

        const active = vscode.window.activeTextEditor?.document.uri;

        if (active !== undefined && active.scheme === "file" && isPatchFile (active.fsPath))
            return active.fsPath;

        const files = await findPatches (true);

        if (files.length === 0) {
            void vscode.window.showErrorMessage ("YDSP: no .ydsp or .ydsp-project file was found in this workspace.");
            return undefined;
        }

        if (files.length === 1)
            return files[0].fsPath;

        const picked = await vscode.window.showQuickPick<PatchQuickPick> (
            files.map (uri => ({ label: vscode.workspace.asRelativePath (uri), uri })),
            { title: "YDSP: select a patch to play" });

        return picked?.uri.fsPath;
    };

    // The single funnel for playback: every entry point routes through here, so
    // at most one `run` subprocess exists per window.
    const play = async (target?: string): Promise<void> => {
        const file = target ?? await resolvePlaybackFile ();

        if (file === undefined)
            return;

        if (vscode.env.remoteName !== undefined)
            playback.appendLine (`Remote workspace (${vscode.env.remoteName}): audio plays on the machine hosting the extension.`);

        player.start (makeRequest (file));
    };

    const restartPlayback = (): void => {
        const file = player.currentFile;

        if (file === undefined) {
            void play ();
            return;
        }

        player.start (makeRequest (file), true);
    };

    const stopPlayback = (): void => player.stop ();

    const applyDeviceChange = async (): Promise<void> => {
        await refresh ();

        if (player.isRunning)
            restartPlayback ();
    };

    const devicePickers: Array<{ key: keyof YdspDeviceSelection; title: string; choices: YdspChoiceKey }> = [
        { key: "audioType", title: "YDSP: audio backend", choices: "audioTypes" },
        { key: "audioOutput", title: "YDSP: audio output", choices: "audioOutputs" },
        { key: "audioInput", title: "YDSP: audio input", choices: "audioInputs" },
        { key: "midiInput", title: "YDSP: MIDI input", choices: "midiInputs" },
        { key: "midiOutput", title: "YDSP: MIDI output", choices: "midiOutputs" }
    ];

    const selectDevices = async (): Promise<void> => {
        let described: YdspPanelState["devices"];

        try {
            described = describeDevices (await getDevices (true));
        } catch (error) {
            void vscode.window.showErrorMessage (`YDSP: cannot list audio/MIDI devices (${errorMessage (error)}).`);
            return;
        }

        const selection = { ...readDevices () };

        for (const picker of devicePickers) {
            const current = selection[picker.key] ?? NO_DEVICE_VALUE;
            const items = described[picker.choices].map (choice => ({
                label: choice.value === current ? `$(check) ${choice.label}` : choice.label,
                description: choice.detail,
                value: choice.value
            }));

            const picked = await vscode.window.showQuickPick (items, { title: picker.title, placeHolder: "Escape keeps the current selection" });

            if (picked === undefined)
                break;

            setSelection (selection, picker.key, picked.value);
        }

        await writeDevices (selection);
        await applyDeviceChange ();
    };

    const handleMessage = async (message: WebviewMessage): Promise<void> => {
        switch (message.type) {
            case "ready":
                await refresh ();
                return;

            case "run":
                await play (message.path);
                return;

            case "stop":
                stopPlayback ();
                return;

            case "restart":
                restartPlayback ();
                return;

            case "refreshDevices":
                deviceCache = undefined;
                await refresh ();
                return;

            case "selection": {
                const selection = { ...readDevices () };
                setSelection (selection, message.key, message.value);
                await writeDevices (selection);
                await applyDeviceChange ();
                return;
            }

            case "setting":
                await vscode.workspace.getConfiguration ("ydsp.player").update (message.key, message.value, vscode.ConfigurationTarget.Workspace);
                await refresh ();
                return;

            case "testNote":
                pendingTestNote = message.value;

                if (player.isRunning)
                    restartPlayback ();
                else
                    await play ();

                return;
        }
    };

    const panel = new YdspPanelProvider (context.extensionUri, message => { void handleMessage (message); });
    context.subscriptions.push (vscode.window.registerWebviewViewProvider (YdspPanelProvider.viewId, panel, { webviewOptions: { retainContextWhenHidden: true } }));

    const updateStatus = (state: YdspPlayerState): void => {
        void vscode.commands.executeCommand ("setContext", ACTIVE_CONTEXT_KEY, state.running);

        if (state.file === undefined) {
            status.hide ();
            return;
        }

        status.text = state.status === "stopping" ? "$(debug-stop) YDSP: stopping" : `$(play) YDSP: ${path.basename (state.file)}`;
        status.tooltip = `Playing ${state.file}\nClick for playback actions.`;
        status.show ();
    };

    context.subscriptions.push (player.onDidChangeState (state => {
        updateStatus (state);
        scheduleRefresh ();
    }));

    // Pinned playback: switching editors only retargets the player when the
    // user opts in, and never spawns a second subprocess.
    const followActiveEditor = (): void => {
        if (! vscode.workspace.getConfiguration ("ydsp.player").get<boolean> ("followActiveEditor", false))
            return;

        if (followTimer !== undefined)
            clearTimeout (followTimer);

        followTimer = setTimeout (() => {
            followTimer = undefined;

            const editor = vscode.window.activeTextEditor;

            if (editor === undefined || ! isPatchFile (editor.document.uri.fsPath))
                return;

            void play (editor.document.uri.fsPath);
        }, FOLLOW_DEBOUNCE_MS);
    };

    context.subscriptions.push (
        vscode.window.onDidChangeActiveTextEditor (() => followActiveEditor ()),
        vscode.workspace.onDidChangeTextDocument (() => scheduleRefresh ()),
        vscode.workspace.onDidSaveTextDocument (() => scheduleRefresh ()),
        vscode.workspace.onDidOpenTextDocument (() => scheduleRefresh ()),
        vscode.workspace.onDidCloseTextDocument (() => scheduleRefresh ()),
        vscode.workspace.onDidCreateFiles (() => { patchCache = undefined; scheduleRefresh (); }),
        vscode.workspace.onDidDeleteFiles (() => { patchCache = undefined; scheduleRefresh (); }),
        vscode.workspace.onDidRenameFiles (() => { patchCache = undefined; scheduleRefresh (); }),
        vscode.workspace.onDidChangeConfiguration (event => {
            if (! event.affectsConfiguration ("ydsp.player"))
                return;

            void refresh ();
            followActiveEditor ();
        })
    );

    const playerMenu = async (): Promise<void> => {
        const actions: ActionQuickPick[] = [
            { label: "$(play) Run patch", action: "run" },
            { label: "$(settings-gear) Select audio/MIDI devices", action: "devices" },
            { label: "$(layout-sidebar-left) Open the YDSP panel", action: "panel" }
        ];

        if (player.isRunning)
            actions.unshift ({ label: "$(debug-stop) Stop playback", action: "stop" }, { label: "$(debug-restart) Restart playback", action: "restart" });

        const picked = await vscode.window.showQuickPick (actions, { title: "YDSP playback" });

        if (picked === undefined)
            return;

        const commands: Record<ActionQuickPick["action"], string> = {
            run: "ydsp.run",
            stop: "ydsp.stop",
            restart: "ydsp.restart",
            devices: "ydsp.selectDevices",
            panel: "ydsp.openPanel"
        };

        await vscode.commands.executeCommand (commands[picked.action]);
    };

    context.subscriptions.push (
        vscode.commands.registerCommand ("ydsp.run", (resource?: vscode.Uri) => play (resource !== undefined && resource.scheme === "file" ? resource.fsPath : undefined)),
        vscode.commands.registerCommand ("ydsp.stop", () => stopPlayback ()),
        vscode.commands.registerCommand ("ydsp.restart", () => restartPlayback ()),
        vscode.commands.registerCommand ("ydsp.selectDevices", () => selectDevices ()),
        vscode.commands.registerCommand ("ydsp.playerMenu", () => playerMenu ()),
        vscode.commands.registerCommand ("ydsp.openPanel", () => vscode.commands.executeCommand (`${YdspPanelProvider.viewId}.focus`))
    );

    void refresh ();
}

export function deactivate (): Thenable<void> | undefined {
    return client?.stop ();
}
