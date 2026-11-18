# YUI for Visual Studio Code

Syntax highlighting, snippets, editing support, live diagnostics, preview and
SVG import for **YUI** - the declarative UI language of the
[YUP](https://github.com/kunitoki/yup) library.

A `.yui` document describes a component tree, its drawings, its stylesheet and
its palettes, and binds them to the models of the application. Everything the
extension knows beyond the grammar comes from `yup_yui_tool`: diagnostics from
`lint`, completions from `schema`, drawings from `import-svg`, and the preview
from the tool running the interface itself.

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
- **Completions** from `yup_yui_tool schema`: the instantiable types at the
  start of a line, the properties and signals of the enclosing type, the values
  of an enumeration property, the pseudo classes inside `style`, and the roles
  after `palette.`
- **Preview**: **YUI: Open Preview** runs the interface next to the editor,
  where it answers the mouse and the keyboard and follows what is typed, with a
  toolbar choosing what is shown, its size, its theme and its zoom
- **SVG import**: **YUI: Import SVG as Drawing** converts an `.svg` into a
  `drawing` block and opens it in a new editor

Formatting is deliberately not provided: the `.yui` writer does not preserve
comments, so a formatter would drop them.

## Settings

| Setting | Default | Description |
|---|---|---|
| `yui.lint.enabled` | `true` | Report problems while editing. |
| `yui.lint.models` | empty | Path to a JSON file declaring the application models, absolute or relative to the workspace folder of the linted document. |
| `yui.preview.size` | `auto` | The size of the preview: `auto`, `panel` or `<width>x<height>` in logical pixels, such as `640x400`. See [Preview](#preview). |
| `yui.preview.theme` | `auto` | The theme the preview is shown with: `auto` follows the VSCode color theme, `light` and `dark` set it. |

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
[hot reload](../../../../docs/ui/yui-hot-reload.md) does in an application.

The toolbar of the panel controls the preview:

| Control | Choices |
|---|---|
| **Show** | The interface of the document, or any drawing, palette or component it declares. A drawing adds **as**, the component type painting it: `Component` by default, `Slider` for a drawing reading the value of a slider. |
| **Size** | `Auto`, `Panel`, a listed size or `Custom`. The two fields next to it show the size of the interface in logical pixels, and typing in them fixes it. |
| **Theme** | `Auto` follows the VSCode color theme, `Light` and `Dark` set it. |
| **Zoom** | `Fit` shows the interface at its size, shrunk to fit the panel when it is larger. A percentage shows it at that scale and scrolls. |
| **Refresh** | Shows the document again, for a change the panel can't see, such as one of the models file. |

With the `Auto` size:

- an interface whose root sets a `width` and a `height` is shown at that size;
- any other interface takes the size of the panel;
- a drawing, a palette or a component keeps the size the tool gives it.

`Panel` is always the size of the panel, whatever the root asks for or is
shown. Either way the interface follows the panel as it is resized, so
dragging its edge shows how the layout responds. A fixed size ignores the
panel.

**Size** and **Theme** are the `yui.preview.size` and `yui.preview.theme`
settings: the toolbar writes them to the user settings, or to those of the
workspace when it already sets them, and follows them when they are edited.
What is shown is remembered per document and the zoom for the panel, both until
the window closes.

The interface is drawn at the pixel density of the display, with the models of
`yui.lint.models`. The problems of the document are listed under it, and a
document that stops loading keeps the interface it had until it loads again.

The preview draws with WebGL 2, and the panel says so in a window that has
none.

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
[`docs/ui/yui-language.md`](../../../../docs/ui/yui-language.md), with the
[styles](../../../../docs/ui/yui-styles.md),
[drawing](../../../../docs/ui/yui-drawing.md),
[layout](../../../../docs/ui/yui-layout.md) and
[documents](../../../../docs/ui/yui-documents.md) pages next to it. Real
documents to test highlighting against live in
[`examples/graphics/data/yui/`](../../../../examples/graphics/data/yui/).

## Structure

```
vscode-yui/
├── package.json                  # extension manifest
├── language-configuration.json   # comments, brackets, indentation
├── images/yui-icon.png           # extension and `.yui` file icon
├── src/extension.ts              # activation: diagnostics, completions, commands
├── src/documentFiles.ts          # a document with the files it names, as the tool is given them
├── src/tool.ts                   # runs the commands of the tool in a worker thread
├── src/toolWorker.ts             # the worker: loads the module and runs a command
├── src/preview.ts                # the preview panel: what to show
├── src/previewPage.ts            # the page of the panel: toolbar and canvas
├── syntaxes/yui.tmLanguage.json  # TextMate grammar (source.yui)
├── snippets/yui.code-snippets    # code snippets
├── tools/package-tool.sh         # copies the tool built for the web into tool/
└── test/sample.yui               # a document to try the grammar on
```

## License

ISC - see [LICENSE](LICENSE). Copyright (c) 2024-2026 kunitoki@gmail.com.
