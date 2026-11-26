# Developing the YUI extension

How to run, package and publish the extension from the YUP repository. What it
does for who installs it is told by the [README](README.md).

## From source

1. Package the tool once, from the repository root: `just yui_tool_wasm`, then
   `sh modules/yup_gui_markup/tools/vscode-yui/tools/package-tool.sh`.
2. Open this folder (`modules/yup_gui_markup/tools/vscode-yui`) in VSCode.
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
cd modules/yup_gui_markup/tools/vscode-yui
sh tools/package-tool.sh
npm install
npm run compile
npx --no-install vsce package -o vscode-yui.vsix
code --install-extension vscode-yui.vsix
```

`vsce` is a development dependency pinned to an exact version, so the package
is made by the version `package-lock.json` holds and not by the latest one of
the day. Raise it with `npm install --save-dev --save-exact @vscode/vsce@latest`.

`YUI_TOOL` overrides the folder the packaging script takes the module from.
The generated `tool/` directory is release output and is ignored by Git.

## The tool

The module links what a development build of an application loads a document
with: the 3D renderer, glTF models, PNG, JPEG and WebP images, and the compiler
of the shaders written in GLSL. That compiler (glslang and SPIRV-Cross) is
about a third of the module. `YUP_YUI_TOOL_WEB_SHADER_COMPILER` leaves it out:

```sh
cd cmake/tools/yui_tool
emcmake cmake -G Ninja -S . -B build_wasm -DCMAKE_BUILD_TYPE=Release -DYUP_YUI_TOOL_WEB_SHADER_COMPILER=OFF
cmake --build build_wasm
```

The extension then only loads the shaders a build compiled into `.ysl`
bundles, and reports a shader written in GLSL as lacking its bundle.

With the compiler, the module holds the material shader template of
`yup_renderer` and the library it includes (`modules/yup_renderer/shaders`),
embedded in a folder of its own, `/yup_renderer/shaders`, which the tool tells
the renderer to read them from to compile a material: the module holds nothing
of the path it was built at.

## Tests

```sh
just vscode_yui_test
```

runs the tests that need no editor, with Node: the server of the tools for
agents and its token, what those tools read and write, the file holding the
token being kept out of the commits, the files a document is given to the
tool with, the problems of a file several documents import, the columns of
the tool as an editor counts them, the grammar and the snippets. They are
listed in the `test` script of `package.json` and in `esbuild.js`, which
bundles them. What needs the tool is tested with it, by
`just yui_tool_wasm_test`.

The `YUI Tooling` workflow (`.github/workflows/yui_tooling.yml`) runs them, and
the tests of the tool, on every push that changes the tooling or the modules
it tells about.

## Publishing

The `VSCode Extension` workflow (`.github/workflows/deploy_vscode.yml`), run by
hand, calls that workflow to build, test and package the extension, then
publishes it to the Visual Studio Marketplace. Its header tells what to set up
once. Raise `version` in `package.json` first, as the Marketplace refuses a
version it already has.

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
├── src/diagnostics.ts            # the problems of a file, by the document whose check told them
├── src/language.ts               # completions, outline, definitions, hovers and colors, as the tool answers them
├── src/positions.ts              # the columns of the tool as an editor counts them, and back
├── src/agentServer.ts            # the server of the tools for agents, and the token it answers for
├── src/agentBackend.ts           # runs those tools on the documents of the editor
├── src/agentFiles.ts             # what they read and write of the workspace
├── src/agentSetup.ts             # their settings and their token, and the command setting a workspace up
├── src/versionControl.ts         # what git makes of the file holding the token, and the rule keeping it out of the commits
├── src/documentFiles.ts          # a document with the files it names and reads, as the tool is given them
├── src/tool.ts                   # runs the commands of the tool in a worker thread, and starts it again when it stops
├── src/toolWorker.ts             # the worker: loads the module and runs a command
├── src/preview.ts                # the preview panel: what to show
├── src/previewPage.ts            # the page of the panel: toolbar and canvas
├── syntaxes/yui.tmLanguage.json  # TextMate grammar (source.yui)
├── snippets/yui.code-snippets    # code snippets
├── tools/package-tool.sh         # copies the tool built for the web into tool/
├── test/agentServer.test.ts      # tests of the server, run by `npm test`
├── test/agentFiles.test.ts       # tests of what the tools read and write
├── test/versionControl.test.ts   # tests of the file holding the token being kept out of the commits
├── test/documentFiles.test.ts    # tests of the files a document is given to the tool with
├── test/diagnostics.test.ts      # tests of the problems several documents tell of a file
├── test/positions.test.ts        # tests of the columns of the tool and of an editor
├── test/syntax.test.ts           # tests of the grammar and of the snippets
└── test/sample.yui               # a document to try the grammar on
```
