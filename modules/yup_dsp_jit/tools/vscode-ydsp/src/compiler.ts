import * as path from "path";
import { execFile } from "child_process";
import * as vscode from "vscode";

/** Returns the compiler binary used for both the language server and playback. */
export function resolveCompilerPath (context: vscode.ExtensionContext): string {
    const configured = vscode.workspace.getConfiguration ("ydsp.server").get<string> ("path");

    if (configured !== undefined && configured.trim ().length > 0)
        return configured;

    const name = process.platform === "win32" ? "yup_dsp_compiler.exe" : "yup_dsp_compiler";
    return path.join (context.extensionPath, "server", `${process.platform}-${process.arch}`, name);
}

export interface YdspCompilerResult {
    code: number;
    stdout: string;
    stderr: string;
}

/** Runs the compiler once and resolves with its streams and exit code. */
export function runCompiler (executable: string, args: string[], cwd: string | undefined): Promise<YdspCompilerResult> {
    return new Promise<YdspCompilerResult> ((resolve, reject) => {
        execFile (executable, args, { cwd, windowsHide: true, maxBuffer: 8 * 1024 * 1024 }, (error, stdout, stderr) => {
            if (error === null) {
                resolve ({ code: 0, stdout, stderr });
                return;
            }

            // Non numeric codes describe spawn failures such as ENOENT or EACCES.
            const code = typeof error.code === "number" ? error.code : undefined;

            if (code === undefined) {
                reject (error);
                return;
            }

            resolve ({ code, stdout, stderr });
        });
    });
}

/** Formats an unknown thrown value for user facing messages. */
export function errorMessage (error: unknown): string {
    return error instanceof Error ? error.message : String (error);
}
