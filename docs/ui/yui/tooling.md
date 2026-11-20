# Tools for `.yui` files

`yup_yui_tool` checks `.yui` files outside the application: on the command
line, while configuring a build, and, [built for the web](#the-tool-for-the-web),
behind the diagnostics, completions and live preview of the VS Code extension.
It also renders documents to images and imports SVG artwork as drawings. The
tool lives in `cmake/tools/yui_tool`, and loads documents with the same
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
`ComponentTypeRegistry`, with the properties and signals it declares, and the
palette roles. Editors build their completions from it:

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
    "paletteRoles": ["background", "surface", "surfaceRaised", "outline", "text", "textMuted", "accent", "onAccent"]
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

The module is built without threads (see `YUP_ENABLE_EMSCRIPTEN_THREADS` in the
[CMake API](../../build-system/cmake-api.md)), so it runs in pages that can't be
cross-origin isolated, which the webview of an editor is.

## VS Code

The `vscode-yui` extension in `modules/yup_gui/tools/vscode-yui` adds `.yui`
support to VS Code:

- syntax highlighting and snippets;
- problems as you type, from `lint`, including those in imported files;
- completions of component types, properties, signals, enumeration values,
  palette roles and pseudo-classes, from `schema`;
- `YUI: Open Preview`, a panel beside the editor running the interface, a
  drawing, a palette or a component of the file;
- the `YUI: Import SVG as Drawing` command.

```sh
just vscode_yui
```

builds the tool for the web, which needs the Emscripten SDK activated, packages
it with the extension and installs it. The extension holds no native binary, so
the same package works on every platform. The settings are `yui.lint.enabled`, `yui.lint.models`, the path of
the [models](#models) file of the workspace, `yui.preview.size` and
`yui.preview.theme`.

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

## See also

- [The `.yui` language](language.md) - the syntax the tool checks.
- [Complete UIs in `.yui`](documents.md) - models, actions and diagnostics.
- [Hot reloading `.yui` files](hot-reload.md) - embedding files with
  `yup_add_yui_resources`.
