import * as path from "path";
import * as vscode from "vscode";
import { LanguageClient, LanguageClientOptions, ServerOptions, Trace } from "vscode-languageclient/node";

let client: LanguageClient | undefined;

export function activate (context: vscode.ExtensionContext): void {
    const output = vscode.window.createOutputChannel("YDSP");
    context.subscriptions.push(output);
    output.appendLine("Activating YDSP language support");
    const configuredPath = vscode.workspace.getConfiguration("ydsp.server").get<string>("path");
    const executable = configuredPath || path.join (context.extensionPath, "server", `${process.platform}-${process.arch}`, process.platform === "win32" ? "yup_dsp_compiler.exe" : "yup_dsp_compiler");
    const serverOptions: ServerOptions = { command: executable, args: ["--lsp"] };
    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: "file", language: "ydsp" }],
        synchronize: { fileEvents: vscode.workspace.createFileSystemWatcher("**/*.ydsp") }
    };

    client = new LanguageClient("ydsp", "YDSP Language Server", serverOptions, clientOptions);
    const trace = vscode.workspace.getConfiguration("ydsp.server").get<string>("trace", "off");
    client.setTrace(trace === "verbose" ? Trace.Verbose : trace === "messages" ? Trace.Messages : Trace.Off);
    client.start().then(() => output.appendLine(`YDSP language server started: ${executable}`)).catch(() => {
        output.appendLine(`YDSP language server could not start: ${executable}`);
        void vscode.window.showErrorMessage(`YDSP language server could not start: ${executable}`);
    });
    context.subscriptions.push({ dispose: () => client?.stop() });
}

export function deactivate (): Thenable<void> | undefined {
    return client?.stop();
}
