import { parentPort, workerData } from "worker_threads";

/** What the tool is asked: a command, a tool of an agent with its arguments as JSON, or the list of those tools. */
type Asked = { args: string[] } | { agentTool: string; agentArguments: string } | { agentTools: true };

type Request = Asked & {
    id: number;
    files: { path: string; data: Uint8Array }[];
};

interface ToolModule {
    wasmMemory: { buffer: ArrayBuffer };
    _yuiToolAllocate (size: number): number;
    _yuiAgentCall?: unknown;
    ccall (name: string, returns: string | null, types: string[], values: unknown[]): unknown;
}

let output: string[] = [];
let errors: string[] = [];

// The application of the module would open a window: here it is left alone, and only runs commands
const create = require ((workerData as { script: string }).script) as (options: object) => Promise<ToolModule>;
const ready = create ({ noInitialRun: true, print: (line: string) => output.push (line), printErr: (line: string) => errors.push (line) });

parentPort?.on ("message", (request: Request) => {
    ready.then (tool => {
        for (const file of request.files) {
            const pointer = tool._yuiToolAllocate (file.data.length);

            new Uint8Array (tool.wasmMemory.buffer, pointer, file.data.length).set (file.data);
            tool.ccall ("yuiToolWriteFile", null, ["string"], [file.path]);
        }

        output = [];
        errors = [];

        // A module built before the tools of an agent existed still runs the commands
        if (! ("args" in request) && tool._yuiAgentCall === undefined) {
            parentPort?.postMessage ({ id: request.id, error: "the yup_yui_tool module of this installation has no tools for agents: build it again" });
            return;
        }

        // The tools of an agent answer with JSON, which stands for what a command prints
        let code: unknown = 0;
        let answer: unknown;

        if ("agentTool" in request)
            answer = tool.ccall ("yuiAgentCall", "string", ["string", "string"], [request.agentTool, request.agentArguments]);
        else if ("agentTools" in request)
            answer = tool.ccall ("yuiAgentTools", "string", [], []);
        else
            code = tool.ccall ("yuiToolRun", "number", ["string"], [JSON.stringify (request.args)]);

        parentPort?.postMessage ({ id: request.id, code, stdout: typeof answer === "string" ? answer : output.join ("\n"), stderr: errors.join ("\n") });
    }).catch ((error: unknown) => {
        // The module is left in a state nobody knows, so the worker ends and another one starts
        parentPort?.postMessage ({ id: request.id, error: error instanceof Error ? error.message : String (error) });
        setImmediate (() => process.exit (1));
    });
});
