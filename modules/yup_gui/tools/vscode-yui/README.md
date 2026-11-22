# YUI for Visual Studio Code

Syntax highlighting, snippets, editing support, live diagnostics, preview and
SVG import for **YUI** - the declarative UI language of the
[YUP](https://github.com/kunitoki/yup) library.

A `.yui` document describes a component tree, its drawings, its stylesheet and
its palettes, and binds them to the models of the application. Everything the
extension knows beyond the grammar comes from `yup_yui_tool`: diagnostics from
`lint`, completions, outline, definitions and hovers from `complete`,
`outline`, `definition` and `hover`, drawings from `import-svg`, and the
preview from the tool running the interface itself. The extension only passes
them on, so any editor asking the tool gets the same.

The tool is packaged as a WebAssembly module, so one package works on every
platform and holds no native binary. Syntax highlighting and snippets stay
available in a package built without it.

## Features

- **Syntax highlighting** for `.yui` files: `import`, `tokens`, `drawing`,
  `palette`, `style` and `component` declarations, `property` / `property
  alias` / `signal`, `let` and `repeat` scopes, shapes and filter primitives,
  selectors with classes, ids and pseudo classes, `@keyframes`, colors, units
  (`deg`, `ms`, `s`, `px`, `%`), built-in functions, two-way bindings (`<=>`)
  and handler arrows (`=>`)
- **Snippets** for components, drawings, stylesheets, palettes, tokens,
  property and signal declarations, transitions, keyframes, animations, the
  `Row` / `Column` / `Grid` / `Stack` layouts, two-way bindings and handlers
- **Editing conveniences**: comment toggling, bracket matching and
  auto-closing, and indentation rules
- **Diagnostics** from `yup_yui_tool lint`, on open, on save and while typing.
  The document is checked as it is in its editor, with the files it imports, so
  unsaved edits count, and a problem in an imported document is reported on
  that document
- **Completions**: the component types, the properties and signals of the
  enclosing one, those a `component` of the document adds included, the values
  of an enumeration property, the elements and properties of a `drawing`, the
  drawings and palettes declared where one is named, the pseudo classes inside
  `style`, the roles and the colors of the palettes after `palette.`, the
  tokens after `tokens.` and the properties of a model after its name
- **Outline** of the declarations, the style rules and the components of a
  document, and **go to definition** on a drawing, a component, a palette, a
  color of a palette, a token or an import, in the files it imports too
- **Hovers** telling the type, the description and the values of a property or
  a signal, and **color swatches** with the color picker on `#rgb`, `#rgba`,
  `#rrggbb` and `#rrggbbaa`
- **Preview**: **YUI: Open Preview** runs the interface next to the editor,
  where it answers the mouse and the keyboard and follows what is typed, with a
  toolbar choosing what is shown, its size, its zoom and its theme, guides
  showing the bounds of its components and statistics of how it is drawn
- **SVG import**: **YUI: Import SVG as Drawing** converts an `.svg` into a
  `drawing` block and opens it in a new editor
- **Tools for agents**: **YUI: Set Up Agent Tools** gives the coding agents of
  a workspace, such as Claude Code and GitHub Copilot, the problems, the layout
  and an image of the `.yui` documents they write. See
  [Tools for agents](#tools-for-agents)

Formatting is deliberately not provided: the `.yui` writer does not preserve
comments, so a formatter would drop them.

## Settings

| Setting | Default | Description |
|---|---|---|
| `yui.lint.enabled` | `true` | Report problems while editing. |
| `yui.lint.models` | empty | Path to a JSON file declaring the application models, absolute or relative to the workspace folder of the linted document. When empty, a document uses `<document>.models.json` next to it. Documents are linted again when their models file is created, edited or deleted. |
| `yui.preview.size` | `auto` | The size of the preview: `auto`, `panel` or `<width>x<height>` in logical pixels, such as `640x400`. See [Preview](#preview). |
| `yui.preview.theme` | `auto` | The theme the preview is shown with: `auto` follows the VSCode color theme, `light` and `dark` set it. |
| `yui.mcp.enabled` | `false` | Serve the [tools for agents](#tools-for-agents) on this machine. **YUI: Set Up Agent Tools** turns it on for a workspace. |
| `yui.mcp.port` | `7391` | The port they are served on, at `http://127.0.0.1:<port>/mcp`. Each window serving them needs its own. |

A document that binds to a model (`value <=> params.cutoff;`) reads something
that only exists inside the application, so without a models file those
bindings are reported as `unknown element 'params'`. The file declares each
model with its properties and sample values:

```json
{
  "params": {
    "cutoff": 0.65,
    "resonance": 0.3,
    "mode": 0
  }
}
```

## Preview

**YUI: Open Preview** (the preview icon in the editor title, or the command
palette) opens one panel beside the editor, where the tool runs the document
whose editor is active. The interface is live: it answers the mouse and, once
clicked, the keyboard, plays its transitions and animations, and follows the
document as it is typed, keeping the state of its components the way
[hot reload](../../../../docs/ui/yui/hot-reload.md) does in an application.

The toolbar of the panel controls the preview:

| Control | Choices |
|---|---|
| **Show** | The interface of the document, or any drawing, palette or component it or the files it imports declare. |
| **as** | A drawing and a component are shown where the interface uses them: on the first component the drawing paints, or the first one of the type, framed with its paint margin. **as** picks another of them, or shows a drawing on its own, painting one component of a type: `Component`, or `Slider` for a drawing reading the value of a slider. |
| **Size** | `Auto`, `Panel`, a listed size or `Custom`. The two fields next to it show the size of the interface in logical pixels, and typing in them fixes it. |
| **Zoom** | `Fit` scales what is shown to the panel, up to 8 times larger or as small as it takes. A percentage shows it at that scale and scrolls. Either way it is drawn at the size it is seen at, so it stays sharp. |
| **Theme** | The sun sets the light theme and the moon the dark one. With neither pressed the preview follows the VSCode color theme: click the pressed one to go back to that. It changes what the document takes from the theme: its palette where the document sets none, and the version of a palette declared for both modes. The interface keeps its state. |
| **Models** | Opens the values of the models the interface binds to, to change them and see it follow, go back to those of the models file or write them to it. |
| **Guides** | Shows the bounds of components, the area each paints when it has a paint margin, dotted, and its distances to the bounds of what holds it. |
| **Statistics** | Tells how the interface is drawn, once a second: the frames a second, the time painting one takes, how much of the panel the last one painted again, the pixels and the GPU it is drawn with, and the components taking the longest to paint. |
| **Refresh** | Shows the document again, for a change the panel can't see, such as one of the models file. |

With the `Auto` size:

- an interface whose root sets a `width` and a `height` is shown at that size,
  which `Fit` then scales to the panel: this is how an interface designed at
  one size is previewed;
- any other interface takes the size of the panel;
- a drawing or a component shown where it is used takes the size it has there;
- a palette, or a drawing on its own, keeps the size the tool gives it: the one
  of the `viewBox` of the drawing when it sets one with plain numbers.

`Panel` is always the size of the panel, whatever the root asks for or is
shown. Either way the interface follows the panel as it is resized, so
dragging its edge shows how the layout responds. A fixed size ignores the
panel.

**Size** and **Theme** are the `yui.preview.size` and `yui.preview.theme`
settings: the toolbar writes them to the user settings, or to those of the
workspace when it already sets them, and follows them when they are edited.
What is shown is remembered per document and the zoom for the panel, both until
the window closes.

A drawing is best shown where it is used: there it has the palette, the style
rules, the models and the kind of component it was made for. On its own it only
has the first palette of the document, and one reading the properties of a
widget fails until **as** names that widget, which the panel says.

The times of **Statistics** are measured while it is on, which costs a little
for each component painted, and nothing is measured while it is off. They are
what painting asks of the processor: what the GPU then does is not in them, and
shows in the frames a second.

With **Guides** on, the component the caret of the editor is in is outlined in
the preview. A click in the preview picks the component under it instead of
reaching the interface, and a click with the shift key adds one or takes it
away. Clicking never moves the caret, and moving the caret shows its component
again.

The interface is drawn at the pixel density of the display, with the models of
`yui.lint.models`. The problems of the document are listed under it, and a
document that stops loading keeps the interface it had until it loads again.

The preview draws with WebGL 2, and the panel says so in a window that has
none.

## Tools for agents

A coding agent can't see what the `.yui` file it wrote looks like. **YUI: Set Up
Agent Tools** serves it tools of the
[Model Context Protocol](https://modelcontextprotocol.io/) from the editor:

| Tool | Answers |
|---|---|
| `yui_check` | The problems of a document and of the files it imports. |
| `yui_inspect` | Its layout as text: each component with its type, id, source line and bounds, and warnings for what has no size or lies outside the interface. |
| `yui_render` | An image of the interface, or of a drawing, a palette or a component of the file, at a size and in a theme, with model values and `:hover` or `:pressed` states set. Also a region or an element of it drawn larger, a grid of coordinates or guides over it, and a comparison with a reference image. |
| `yui_schema` | The component types, or the properties and signals of one. |
| `yui_capture_preview` | An image of the preview panel as it is now, with the values of its models. |

The command turns `yui.mcp.enabled` on for the workspace and adds the server to
the `.mcp.json` at its root, which Claude Code and GitHub Copilot read:

```json
{
  "mcpServers": {
    "yui": { "type": "http", "url": "http://127.0.0.1:7391/mcp" }
  }
}
```

- Documents are read as they are in their editors, so unsaved changes count.
- The tools that draw run in the preview panel, which opens beside the editor
  when it is closed. What it shows is left alone.
- The server only listens on this machine and only reads the files of the
  workspace. A document importing a file outside the workspace is checked
  without that file. The models of `yui.lint.models` are read wherever they
  are.
- It writes one kind of file: the `.png` a call that draws names as `output`,
  inside the workspace.
- Each window serves its own workspace on its own port. When the port set is
  taken, the command finds the next free one and writes it to the workspace
  settings and to `.mcp.json`.

The arguments of the tools are described in the
[tooling page](../../../../docs/ui/yui/tooling.md#tools-for-agents) of the
documentation, with `yup_yui_tool mcp`, which serves the same tools with no
editor.

## Installing

### From source (development)

1. Package the tool once, from the repository root: `just yui_tool_wasm`, then
   `sh modules/yup_gui/tools/vscode-yui/tools/package-tool.sh`.
2. Open this folder (`modules/yup_gui/tools/vscode-yui`) in VSCode.
3. Press <kbd>F5</kbd> - a new Extension Development Host window opens with the
   extension loaded.

### As a VSIX

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

## Language reference

The full language specification lives in the YUP repository at
[`docs/ui/yui/language.md`](../../../../docs/ui/yui/language.md), with the
[styles](../../../../docs/ui/yui/styles.md),
[drawing](../../../../docs/ui/yui/drawing.md),
[layout](../../../../docs/ui/yui/layout.md) and
[documents](../../../../docs/ui/yui/documents.md) pages next to it. Real
documents to test highlighting against live in
[`examples/graphics/data/yui/`](../../../../examples/graphics/data/yui/).

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

## License

ISC - see [LICENSE](LICENSE). Copyright (c) 2024-2026 kunitoki@gmail.com.
