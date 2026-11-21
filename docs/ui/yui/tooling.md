# Tools for `.yui` files

`yup_yui_tool` checks `.yui` files outside the application: on the command
line, while configuring a build, and, [built for the web](#the-tool-for-the-web),
behind the diagnostics, completions and live preview of the VS Code extension.
It also renders documents to images, describes their layout, imports SVG
artwork as drawings and gives all of this to [coding agents](#tools-for-agents).
The tool lives in `cmake/tools/yui_tool`, and loads documents with the same
`YuiDocument` and `YuiLoader` an application uses, so it reports what the
application would.

```sh
just yui_tool --help
```

builds it as `cmake/tools/yui_tool/build/yup_yui_tool` and runs it with the
arguments given. The tool sets itself up as an application does, default theme
included, but never opens a window, so it also runs on machines without a
display such as build servers.

## Checking files

```sh
yup_yui_tool verify ui/main.yui ui/parts/style.yui
yup_yui_tool lint ui/main.yui
yup_yui_tool lint --strict ui/main.yui
```

`verify` reports the errors: what stops a document from loading, and what it
loads without, such as a binding to a property that doesn't exist. `lint`
reports the warnings too, for example a property set on a component that
doesn't declare it. Problems are written to the standard error, one per line:

```text
/project/ui/main.yui:12:9: error: Unknown component type 'Slidr'
/project/ui/parts/style.yui:4:5: warning: Undeclared property 'glow' on Slider
```

Imports resolve relative to the file, as `YuiLoader` resolves them, and a
problem in an imported file is reported at that file. The parser stops at the
first syntax error, so a file shows one syntax error at a time, then every
other problem once it parses.

A file without a root component, holding only what other files import (styles,
palettes, drawings, components), is valid: it is checked as the declarations of
an empty interface.

| Exit code | Meaning |
|---|---|
| `0` | No error. Warnings don't fail `lint` unless `--strict` is given. |
| `1` | Errors were found (or warnings, with `--strict`). |
| `2` | The arguments are wrong, or a file can't be read. |

### Models

A document binding to the models of the application (`value <=> params.cutoff`)
can only be checked when the tool knows them, since they are set in C++ with
`YuiDocument::setModel`. Without them every such binding is an error. Declare
them in a JSON file, each model with its properties and a sample value of the
type the application gives them:

```json
{
    "params": { "cutoff": 0.65, "resonance": 0.3, "mode": 0, "name": "Init" }
}
```

```sh
yup_yui_tool verify --models ui/models.json ui/main.yui
```

A binding to a property the file doesn't list is then an error, which catches
misspelled names. Actions need no declaration, since they are looked up when
they run.

### Machine readable output

`--json` prints the problems to the standard output instead, for editors and
scripts. The exit codes stay the same:

```json
{
    "diagnostics": [
        { "file": "/project/ui/main.yui", "line": 12, "column": 9, "severity": "error", "message": "Unknown component type 'Slidr'" }
    ]
}
```

`file` is the absolute path of the file the problem is in, `line` and `column`
start at 1 and are both 0 when the position isn't known, and `severity` is
`error` or `warning`.

`lint --stdin --path ui/main.yui` reads the text of the document from the
standard input rather than from the file, which doesn't have to exist, while
its imports are still read next to it. This is how an editor checks unsaved
changes.

### What the tool can't know

- Component types registered by the application with `ComponentTypeRegistry`
  are unknown to the tool, which only has the types of `yup_gui`.
- Values are checked against the default theme: properties an application adds
  to its own theme aren't there.

## Checking while configuring

`yup_add_yui_resources` runs `verify` on the files it embeds when given
`VERIFY`, so an invalid file fails the configuration of the build instead of
shipping:

```cmake
yup_add_yui_resources (MyAppUi
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/ui"
    VERIFY
    VERIFY_MODELS ui/models.json
    FILES
        ui/main.yui
        ui/parts/style.yui)
```

The tool is built for the host into the build tree the first time, which takes
as long as building `yup_gui`. See the [CMake API](../../build-system/cmake-api.md).

## The schema

```sh
yup_yui_tool schema --output yui-schema.json
```

writes every component type a `.yui` file can name, as registered in
`ComponentTypeRegistry`, with the properties and signals it declares, the
palette roles and the names of the `colors.` constants. Editors build their completions from it:

```json
{
    "version": 1,
    "types": [
        {
            "name": "Slider",
            "base": "Component",
            "instantiable": true,
            "properties": [
                { "name": "value", "type": "number", "state": false, "styleable": false, "inherited": true, "readOnly": false },
                { "name": "dragging", "type": "boolean", "default": false, "state": true, "styleable": false, "inherited": true, "readOnly": true }
            ],
            "signals": [
                { "name": "onValueChanged", "description": "Emitted with the new value when the value changes", "arguments": [ { "name": "value", "type": "number" } ] }
            ]
        }
    ],
    "paletteRoles": ["background", "surface", "surfaceRaised", "outline", "text", "textMuted", "accent", "onAccent"],
    "namedColors": ["black", "white", "transparentBlack", "transparentWhite", "aliceblue", "..."]
}
```

- `properties` and `signals` are the ones the type declares itself. The others
  come from its `base`, which the root type doesn't have.
- `type` is `number`, `integer`, `boolean`, `string`, `color`, `point`,
  `rectangle`, `transform`, `enumeration`, `font`, `drawing`, `array` or `any`.
  An `array` has an `elementType` when its elements are typed, and an
  `enumeration` lists its `values`.
- `default` is given when it can be written as a `.yui` literal, colors as
  `#rrggbbaa`.
- `state` marks a boolean usable as a pseudo-class in a selector (`:dragging`),
  `styleable` a property a stylesheet may set, and `inherited` a value that
  also applies to the descendants of the component it is set on.
- `version` changes when a field is removed or changes meaning, not when one
  is added, so readers ignore the fields they don't know.

## Rendering a preview

```sh
yup_yui_tool render ui/main.yui --output main.png
yup_yui_tool render ui/main.yui --output main.png --size 640x760 --theme light --models ui/models.json
```

draws the interface of a document into a PNG, without opening a window. The
size of the image, in logical pixels, is the first of:

1. `--size <width>x<height>`;
2. the `width` and `height` the root component asks for;
3. `--default-size <width>x<height>`, for a caller with a size to offer when
   the document has none;
4. 640 by 400, or a smaller size when previewing a declaration.

`--scale` is the number of pixels per logical pixel, 2 by default so the image
is sharp on high density displays, and `--theme` is the mode of the default
theme, `dark` or `light`. The image is filled behind the interface with the
background of the palette its root is drawn with, the one of the theme unless
the document sets another. A document that sets its own palette looks the same
in both modes, unless the palette has a dark and a light version or the
document reads `theme.dark` (see [Palettes](styles.md#palettes)).

A container gives no size to a child that asks for none: a `Row` in a `Column`
needs a `preferredHeight` or a `flexGrow`, or it is laid out with no height
and nothing of it shows in the image.

What a file declares can be previewed on its own:

```sh
yup_yui_tool render ui/knob.yui --output knob.png --drawing Knob --as Slider
yup_yui_tool render ui/parts.yui --output dial.png --as LabeledKnob
yup_yui_tool render ui/palette.yui --output palette.png --palette Ember
```

- `--drawing <Name>` paints a component with the drawing. The component is a
  plain `Component` unless `--as <Type>` names another one: a drawing reading
  the properties of a widget, such as the `value` of a `Slider`, needs it as
  its host.
- `--as <Type>` alone shows one component of that type, which is how a
  composite declared with `component` is previewed.
- `--palette <Name>` shows a few widgets and a swatch of each role in the
  palette.

The component takes the place of the root when the file has one. A file
without a root and without these options shows its first drawing, or its first
palette when it declares no drawing.

The errors of the document are printed as for `verify`. The image is still
written when the document loads despite them.

| Exit code | Meaning |
|---|---|
| `0` | The image was written. |
| `1` | The document can't be loaded, or there is nothing to render. |
| `2` | The arguments are wrong, or a file can't be read or written. |
| `3` | There is no graphics device to render with. |

Rendering draws on the GPU, which `verify`, `lint` and `schema` never need. It
works on macOS, through Metal, with no display attached. On Windows it uses
Direct3D without a window. It isn't available on Linux yet, where OpenGL needs
a context that the tool doesn't create.

## Inspecting the layout

```sh
yup_yui_tool inspect ui/main.yui
yup_yui_tool inspect ui/main.yui --size 640x760 --models ui/models.json
```

prints the layout of a document as JSON, without drawing it, so it needs no
graphics device and works everywhere. It takes the options of `render` saying
what to show and how (`--size`, `--default-size`, `--theme`, `--models`,
`--drawing`, `--as`, `--palette`), and exits with the same codes.

```json
{
    "file": "/project/ui/main.yui",
    "width": 640,
    "height": 400,
    "root": {
        "type": "Column", "path": "", "line": 3, "column": 1, "bounds": [0, 0, 640, 400],
        "children": [
            { "type": "Row", "id": "toolbar", "path": "0", "line": 6, "column": 5, "bounds": [0, 0, 640, 0] }
        ]
    },
    "warnings": [
        { "message": "Row #toolbar has no height, so nothing of it shows: inside a Row or a Column it needs a preferred size or a flexGrow", "path": "0", "line": 6, "column": 5 }
    ]
}
```

- A component has its `type`, its `id` when it has one, the `line` and `column`
  it was written at (and the `file`, for one of an imported file), its `text`
  when it has one, and `visible: false` when it is hidden.
- `bounds` are `[x, y, width, height]` in logical pixels, from the top left of
  the interface.
- `path` names a component by its place among the `children`: `0/2` is the
  third child of the first child of the root.
- `warnings` tell what is laid out with no width or no height, or outside the
  interface. Once for a component, not again for what it holds.

Only the components the document describes are listed. Those a widget creates
for itself are left out, and what they hold is listed in their place.

The layout is the one the interface settles in: its styles are brought up to
date with its size, and their transitions to their end.

## Tools for agents

A coding agent writing `.yui` files, such as Claude Code or GitHub Copilot,
can't see what it wrote. The tool gives it what `lint`, `inspect`, `render`
and `schema` give, as tools of the
[Model Context Protocol](https://modelcontextprotocol.io/):

| Tool | Answers |
|---|---|
| `yui_check` | The problems of a file and of the files it imports, and the models it binds to that its models file lacks. |
| `yui_inspect` | The layout, as `inspect` prints it, with the problems. Also where the elements of drawings land, and what pointer events did. |
| `yui_render` | A PNG of the interface, or of a drawing, a palette or a component of the file, with the problems. All of it, or a part drawn larger, under a grid, or next to a reference it is compared with. |
| `yui_schema` | What a file can name: the component types, the properties and signals of one, the elements of drawings, what styles take, the functions of expressions. |
| `yui_capture_preview` | A PNG of the live preview of VS Code as it is now, with the values of its models, a long list told by its length. Only the [extension](#vs-code) has it. |

The tools taking a `file` also take what `render` takes, under the same names:
`drawing`, `palette`, `as`, `size`, `theme`, `models`, and `scale` for those
giving an image (1 by default, 4 at most). Without `models`, a `<name>.yui`
file uses the `<name>.models.json` next to it when there is one. Answers are
JSON on one line.

### The state shown

- `values` sets model values over those of the models file:
  `{ "params": { "cutoff": 0.9 } }`.
- `states` shows components as their `:hover` or `:pressed` rules style them,
  each named by its `id` or by the `path` `yui_inspect` gives it:
  `[{ "id": "play", "state": "hover" }]`. The focus can't be shown, as it
  needs a window.
- `events` plays pointer events, see [below](#playing-pointer-events).

`yui_inspect` and `yui_render` show the interface as it settles: its styles up
to date with its size and its states, and their transitions at their end.

### Looking closer

`yui_render` draws a part of the interface in place of all of it:

```json
{ "file": "ui/main.yui", "region": [320, 80, 160, 100], "scale": 4, "grid": 20 }
{ "file": "ui/main.yui", "element": "cutoff", "margin": 12, "scale": 8 }
```

- `region` is `[x, y, width, height]` in logical pixels of the interface.
- `element` is the `id` of a component, or its `path`, drawn with a `margin`
  around it (8 when not given). The answer tells the `region` that was drawn.
- `scale` goes up to 8 for a part. An image is at most 4096 pixels a side.
- `grid` draws lines that many logical pixels apart, each with the coordinate
  of the interface it is at, so places and sizes are read off the image, a
  part of the interface included.

The interface is laid out at its size whatever part of it is drawn.

### Comparing with a reference

```json
{ "file": "ui/main.yui", "reference": "design/main.png" }
```

`reference` names an image of what the interface is meant to look like. The
image returned then holds three panels of the size of the render: the
reference, the render, and their difference, where what differs goes from
yellow to red as it differs more. They are side by side, or one above the
other for an interface wider than tall. The answer adds:

```json
"comparison": {
    "similarity": 0.953, "differentPixels": 0.081,
    "reference": [1280, 800], "resized": false, "stretched": false,
    "panels": ["reference", "render", "difference"], "arrangement": "column", "panel": [1280, 800],
    "worst": [ { "region": [320, 80, 160, 100], "error": 0.31 } ]
}
```

- `similarity` is 1 for two images that are the same. `differentPixels` is
  the share of the pixels a person would see differ.
- `worst` are the parts differing most, as regions to draw next with
  `region` and a `scale`, where the reference is compared part for part.
- A reference of another size is brought to the size of the render
  (`resized`), and `stretched` tells one of other proportions.
- An interface with no size of its own, and no `size` given, takes the size
  of the reference.

The comparison is pixel by pixel: sharp text one pixel off scores badly, and
the difference panel tells that from a real difference better than the score.

### Saving the image

`output` names a `.png` file the image of `yui_render` or of
`yui_capture_preview` is also written to. It must be inside the folder of the
project, its folders are made and a file there is replaced. The image is still
returned.

### Playing pointer events

```json
"events": [ { "id": "play", "type": "click" },
            { "id": "cutoff", "type": "drag", "by": [0, -40] },
            { "id": "cutoff", "type": "wheel", "delta": [0, 1] } ]
```

`events` are played in order before `yui_inspect` tells the layout or
`yui_render` draws. A `type` is `click`, `doubleClick`, `drag`, `wheel` or
`hover`, on a component named by its `id` or its `path`. `at` is a point in
the component, its middle when not given. A drag goes `by` a distance or `to`
a point of the component, and a wheel turns by a `delta`.

An event goes where a pointer at that place would, so the answer tells what
it did:

```json
"events":  [ { "type": "click", "received": { "type": "ToggleButton", "id": "play", "path": "0/2" } } ],
"changed": { "params": { "playing": [false, true] } },
"actions": [ { "name": "started", "arguments": [] } ]
```

- `received` is the component that got the event. A `warning` tells when it
  is not the one named: another one is over it, or it is disabled or hidden.
- `changed` are the model values the events changed, each with the value
  before and the one after. Of a list, the elements that changed.
- `actions` are the `action ("name", ...)` the interface called, in order.
- An event changing the range of a [`repeat` of
  components](documents.md#repeating-components) has them made again before
  the next event, which can be on one of the new ones by its `path`.

The pointer stays where the last event left it, so the component under it is
drawn hovered. There are no key events, and a popup does not open, as both
need a window.

### Inside the layout

Two arguments of `yui_inspect`:

- `element` describes one component, named by its `id` or its `path`, with
  what it holds, in place of the whole interface.
- `drawings: true` adds, to each component a drawing paints, the elements of
  the drawing that have an `id`:

```json
"drawing": [ { "type": "arc", "id": "value", "line": 46, "bounds": [212, 96, 44, 44] },
             { "type": "rect", "id": "tick", "line": 51, "bounds": [200, 150, 68, 4], "rows": 12 },
             { "type": "text", "id": "caption", "line": 60, "bounds": [200, 160, 68, 16], "text": [222, 162, 24, 12] } ]
```

`bounds` are in the interface, under the transforms of the groups and the
viewports around the element, so they can be given to `yui_render` as a
`region`. An element inside a `repeat` covers all its copies and tells their
number as `rows`. A `text` also tells the area its glyphs cover.

### What a file can name

`yui_schema` answers from the tables the files are checked with, so it can't
tell what the tool does not accept:

| Arguments | Answers |
|---|---|
| none | The component types and the palette roles. |
| `type` | The properties and signals a type declares, with the `bases` declaring the others. `inherited: true` lists those too. |
| `topic: "drawing"` | The elements of a drawing: the geometry and the properties of each, what it can hold, and the paint every shape takes. |
| `topic: "styles"` | The declarations of a rule that are not properties, the pseudo-classes and the palette roles. |
| `topic: "expressions"` | The functions, the members and methods of each type, and the values of the enumerations. |

### The models a file lacks

When a file binds to models, or to properties, that its models file does not
declare, `yui_check` lists them ready to be added to that file:

```json
"missingModels": { "seq": { "tempo": 0, "running": false, "steps": [false] } },
"errorsWithMissingModels": 0
```

Each has a sample value of the kind the file reads it as: a number, `false`,
`""`, or a list of one of these. `errorsWithMissingModels` is how many errors
the file still has once they are added, 0 when it then loads clean.

### Serving the tools

There are two ways to serve them. Both are listed in the `.mcp.json` at the root
of the project, which Claude Code and GitHub Copilot read:

```json
{
    "mcpServers": {
        "yui": { "type": "stdio", "command": "/path/to/yup_yui_tool", "args": ["mcp"] }
    }
}
```

`yup_yui_tool mcp` serves them by itself, on its standard input and output,
with no editor. Paths are relative to the folder it is started in. `yui_render`
needs a graphics device, as `render` does, so it fails on Linux, where
`yui_inspect` still answers.

```json
{
    "mcpServers": {
        "yui": { "type": "http", "url": "http://127.0.0.1:7391/mcp" }
    }
}
```

The [VS Code extension](#vs-code) serves them from the editor, with no native
tool to build: it checks the documents as they are in their editors, unsaved
changes included, and draws in its preview panel on every platform.

## Importing an SVG

```sh
yup_yui_tool import-svg logo.svg --name Logo --output ui/logo.yui
yup_yui_tool import-svg logo.svg --color-properties
```

writes the SVG as a `drawing` of native elements, to the standard output
without `--output`. `--name` is the name of the drawing (`Imported` by
default), and `--color-properties` declares every color the SVG is painted with
as a `color` property, so the component styled with the drawing can change
them. What can't be painted exactly as the SVG is listed as warnings. This is
`YuiSvgImporter`, described in [Data-driven drawing](drawing.md).

## The tool for the web

```sh
just yui_tool_wasm
```

builds the tool with Emscripten, whose SDK must be activated, as
`cmake/tools/yui_tool/build_wasm/yup_yui_tool.js` and `yup_yui_tool.wasm`. It is
the same tool as one module for every platform, which a host loads and calls
into instead of starting a process:

- an editor extension runs the commands with it. The module has no disk, so the
  host gives it the files, and gets back what the native tool prints. `render`
  is left to the native tool, the only one with an image file to write;
- a page runs the application of the module, which shows an interface live in a
  canvas: it answers the mouse and the keyboard, and is told what to show.

| Entry point | Purpose |
|---|---|
| `createYuiTool (options)` | Loads the module. With `noInitialRun` the application isn't started, and the module only runs commands. |
| `yuiToolAllocate (size)`, `yuiToolWriteFile (path)` | Give a file: its bytes are copied to the memory returned, then it is named with a path relative to the folder of the files. |
| `yuiToolRun (arguments)` | Runs a command in the folder of the files. The arguments are a JSON array of strings, the result is the exit code, and the output goes to `print` and `printErr`. |
| `yuiPreviewShow (path, drawing, palette, type, models, light)` | Shows a file in the canvas, or a drawing, a palette or a component type of it, once the application called `yuiPreviewReady`. The outcome is given to `yuiPreviewShown`: the problems, and the size the interface asks for. |
| `yuiAgentTools ()` | Returns the [tools for agents](#tools-for-agents) as JSON: `tools`, each with its `name`, `description` and `inputSchema`, `drawing`, the names of those only a page can run, and `instructions`. |
| `yuiAgentCall (name, arguments)` | Runs one of them on the files given, with its arguments as a JSON object. The answer is JSON: `text`, `isError`, and `image`, a PNG in base64 that is empty when the tool gives none. |

The module is built without threads (see `YUP_ENABLE_EMSCRIPTEN_THREADS` in the
[CMake API](../../build-system/cmake-api.md)), so it runs in pages that can't be
cross-origin isolated, which the webview of an editor is.

## VS Code

The `vscode-yui` extension in `modules/yup_gui/tools/vscode-yui` adds `.yui`
support to VS Code:

- syntax highlighting and snippets;
- problems as you type, from `lint`, including those in imported files;
- completions of component types, properties, signals, enumeration values,
  palette roles, `colors.` names and pseudo-classes, from `schema`;
- `YUI: Open Preview`, a panel beside the editor running the interface, a
  drawing, a palette or a component of the file;
- the `YUI: Import SVG as Drawing` command;
- the [tools for agents](#tools-for-agents), once `YUI: Set Up Agent Tools` was
  run in a workspace.

```sh
just vscode_yui
```

builds the tool for the web, which needs the Emscripten SDK activated, packages
it with the extension and installs it. The extension holds no native binary, so
the same package works on every platform. The settings are `yui.lint.enabled`, `yui.lint.models`, the path of
the [models](#models) file of the workspace, `yui.preview.size`,
`yui.preview.theme`, `yui.mcp.enabled` and `yui.mcp.port`.

Documents are checked and shown as they are in their editors, so what is typed
counts before it is saved. The preview is live: the interface answers the mouse
and the keyboard, plays its transitions, and keeps the state of its components
while the document changes under it, as [hot reload](hot-reload.md) does in
an application, and while its theme is switched. The problems of the document
are listed under it.

The toolbar of the preview chooses what it shows and how:

- **Show**: the interface, or one of the drawings, palettes and components the
  file declares, a drawing painted by the component type picked next to it;
- **Size**: `Auto` shows an interface at the `width` and `height` its root
  sets, and otherwise at the size of the panel, `Panel` is always the size of
  the panel, both following it as it is resized, and a listed or typed size
  such as `640x400` is fixed;
- **Theme**: the default theme in its light or dark mode, or the one matching
  the editor. It changes what the document takes from the theme: its palette
  where the document sets none, the version of a
  [palette declared for both modes](styles.md#palettes), and `theme.dark`;
- **Zoom**: from 50% to 400%.

The size and the theme are the two `yui.preview` settings, which the toolbar
writes.

`YUI: Set Up Agent Tools` turns `yui.mcp.enabled` on for the workspace and
adds the server to its `.mcp.json`, keeping what the file holds. The server
only listens on this machine, at `http://127.0.0.1:<yui.mcp.port>/mcp`, only
reads the files of the workspace and writes none: a document importing a file
outside of it is checked without that file. The models of the
`yui.lint.models` setting are read wherever they are. Each window serving a
workspace needs a port of its own, which the command finds when the one set is
taken. A tool that draws opens the preview panel beside the editor when it is
closed, and leaves what it shows alone. Without a `theme`, it draws in the one
the preview shows.

There is no formatter yet: writing a document back from its tree would lose
its comments.

## Testing the tool

```sh
just yui_tool_test
```

builds the tool and runs `cmake/tools/yui_tool/tests/test_cli.py` against it.

```sh
just yui_tool_wasm_test
```

builds the tool for the web and runs `cmake/tools/yui_tool/tests/test_web.js`
with Node, which loads the module and runs the commands the way an editor does.

```sh
just vscode_yui_test
```

runs the tests of the extension that need no editor: those of the server its
tools for agents are reached through.

## See also

- [The `.yui` language](language.md) - the syntax the tool checks.
- [Complete UIs in `.yui`](documents.md) - models, actions and diagnostics.
- [Hot reloading `.yui` files](hot-reload.md) - embedding files with
  `yup_add_yui_resources`.
