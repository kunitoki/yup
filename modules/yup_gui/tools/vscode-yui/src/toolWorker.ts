import { parentPort, workerData } from "worker_threads";

interface Request {
    id: number;
    args: string[];
    files: { path: string; data: Uint8Array }[];
}

interface ToolModule {
    wasmMemory: { buffer: ArrayBuffer };
    _yuiToolAllocate (size: number): number;
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

        const code = tool.ccall ("yuiToolRun", "number", ["string"], [JSON.stringify (request.args)]);

        parentPort?.postMessage ({ id: request.id, code, stdout: output.join ("\n"), stderr: errors.join ("\n") });
    }).catch ((error: unknown) => {
        // The module is left in a state nobody knows, so the worker ends and another one starts
        parentPort?.postMessage ({ id: request.id, error: error instanceof Error ? error.message : String (error) });
        setImmediate (() => process.exit (1));
    });
});
