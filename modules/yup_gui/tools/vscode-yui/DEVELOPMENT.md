# Developing the YUI extension

How to run, package and publish the extension from the YUP repository. What it
does for who installs it is told by the [README](README.md).

## From source

1. Package the tool once, from the repository root: `just yui_tool_wasm`, then
   `sh modules/yup_gui/tools/vscode-yui/tools/package-tool.sh`.
2. Open this folder (`modules/yup_gui/tools/vscode-yui`) in VSCode.
3. Press <kbd>F5</kbd> - a new Extension Development Host window opens with the
   extension loaded.

Syntax highlighting and snippets stay available in a package built without the
tool.

## As a VSIX

From the repository root, one command builds the tool for the web and the
extension, and installs it into your local VSCode. It requires `just`, the
`code` CLI on your `PATH` and the Emscripten SDK activated:

```sh
just vscode_yui
```

Or manually:

```sh
just yui_tool_wasm
cd modules/yup_gui/tools/vscode-yui
sh tools/package-tool.sh
npm install
npm run compile
npx --yes @vscode/vsce package -o vscode-yui.vsix
code --install-extension vscode-yui.vsix
```

`YUI_TOOL` overrides the folder the packaging script takes the module from.
The generated `tool/` directory is release output and is ignored by Git.

## Publishing

The `VSCode Extension` workflow (`.github/workflows/deploy_vscode.yml`), run by
hand, builds, tests and publishes the extension to the Visual Studio
Marketplace. Its header tells what to set up once. Raise `version` in
`package.json` first, as the Marketplace refuses a version it already has.

The README is the page of the extension on the Marketplace, where a link
relative to this folder leads nowhere: keep its links absolute.

## Structure

```
vscode-yui/
├── package.json                  # extension manifest
├── language-configuration.json   # comments, brackets, indentation
├── images/yui-icon.png           # extension and `.yui` file icon
├── esbuild.js                    # bundles the extension and its tests
├── src/extension.ts              # activation: diagnostics, commands, what the tool is asked
├── src/language.ts               # completions, outline, definitions, hovers and colors, as the tool answers them
├── src/agentServer.ts            # the server of the tools for agents
├── src/agentBackend.ts           # runs those tools on the documents of the editor
├── src/agentFiles.ts             # what they read and write of the workspace
├── src/agentSetup.ts             # their settings, and the command setting a workspace up
├── src/documentFiles.ts          # a document with the files it names, as the tool is given them
├── src/tool.ts                   # runs the commands of the tool in a worker thread
├── src/toolWorker.ts             # the worker: loads the module and runs a command
├── src/preview.ts                # the preview panel: what to show
├── src/previewPage.ts            # the page of the panel: toolbar and canvas
├── syntaxes/yui.tmLanguage.json  # TextMate grammar (source.yui)
├── snippets/yui.code-snippets    # code snippets
├── tools/package-tool.sh         # copies the tool built for the web into tool/
├── test/agentServer.test.ts      # tests of the server, run by `npm test`
├── test/agentFiles.test.ts       # tests of what the tools read and write
└── test/sample.yui               # a document to try the grammar on
```
