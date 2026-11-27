# YUI for Visual Studio Code

Language support, a live preview and tools for coding agents for **YUI**, the
declarative UI language of the [YUP](https://github.com/kunitoki/yup) library.

A `.yui` document describes a component tree, its drawings, its shaders, its
stylesheet and its palettes, and binds them to the models of the application.
A small one, a dial drawn for a slider and a label following it:

```yui
drawing Dial {
    property value: number = 0;

    let c: bounds.center;
    let r: min (bounds.width, bounds.height) * 0.4;

    circle { center: c; radius: r; fill: palette.surfaceRaised;
             stroke: palette.accent; strokeWidth: 2; }

    line { from: c; to: polar (c, r * 0.8, 135deg + 270deg * value);
           stroke: palette.text; strokeWidth: 3; cap: round; }
}

style {
    .knob { drawing: Dial; }
}

Column {
    padding: 10;
    gap: 10;

    Slider { id: cutoff; class: knob; sliderType: rotaryVerticalDrag; value: 0.65; flexGrow: 1; }

    Label { text: format ("cutoff %.0f%%", cutoff.value * 100);
            justification: center; preferredHeight: 28; }
}
```

Everything beyond the grammar is done by YUP itself, which the extension holds
as a WebAssembly module: documents are checked and run the way an application
loads them, there is nothing else to install, and it works the same on every
platform.

## Getting started

1. Open a `.yui` file: its problems are reported as you type.
2. Run **YUI: Open Preview** from the command palette, or click the preview
   icon in the editor title, to run the interface beside the editor.
3. When the document binds to the models of an application, describe them in a
   models file next to it, as told under [Settings](#settings).

## Features

- **Syntax highlighting** for `.yui` files: `import`, `tokens`, `drawing`,
  `shader`, `palette`, `style` and `component` declarations, `property` /
  `property alias` / `signal`, `let` and `repeat` scopes, shapes and filter
  primitives, selectors with classes, ids and pseudo classes, `@keyframes`,
  colors, units (`deg`, `ms`, `s`, `px`, `%`), built-in functions, two-way
  bindings (`<=>`) and handler arrows (`=>`)
- **Snippets** for components, drawings, shaders, stylesheets, palettes,
  tokens, property and signal declarations, transitions, keyframes, animations,
  the `Row` / `Column` / `Grid` / `Stack` layouts, a 3D scene, a model, two-way
  bindings and handlers
- **Editing conveniences**: comment toggling, bracket matching and
  auto-closing, and indentation rules
- **Diagnostics** on open, on save and while typing.
  The document is checked as it is in its editor, with the files it imports and
  those it reads, so unsaved edits count, and a problem in an imported document
  is reported on that document. Its shaders are compiled and its models and
  textures loaded, see [Shaders and 3D scenes](#shaders-and-3d-scenes)
- **Completions**: the component types, those of a 3D scene included, the
  properties and signals of the enclosing one, those a `component` of the
  document adds included, the values of an enumeration property, the elements
  and properties of a `drawing`, the drawings, shaders and palettes declared
  where one is named, the properties of a shader where it is used, the pseudo
  classes inside `style`, the roles and the colors of the palettes after
  `palette.`, the tokens after `tokens.` and the properties of a model after
  its name
- **Outline** of the declarations, the style rules and the components of a
  document, and **go to definition** on a drawing, a shader, a component, a
  palette, a color of a palette, a token or an import, in the files it imports
  too
- **Hovers** telling the type, the description and the values of a property or
  a signal, and **color swatches** with the color picker on `#rgb`, `#rgba`,
  `#rrggbb` and `#rrggbbaa`
- **Preview**: **YUI: Open Preview** runs the interface next to the editor,
  where it answers the mouse and the keyboard and follows what is typed, with a
  toolbar choosing what is shown, its size, its zoom and its theme, guides
  showing the bounds of its components and statistics of how it is drawn
- **SVG import**: **YUI: Import SVG as Drawing** converts an `.svg` into a
  `drawing` block, saves it as the `.yui` file you choose and opens it
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
| `yui.lint.models` | empty | Path to a JSON file declaring the application models, absolute or relative to the workspace folder of the linted document. When empty, a document uses `<document>.models.json` next to it. Documents are linted again when their models file is created, edited or deleted. Each folder of a workspace can set its own. |
| `yui.files.readOutsideWorkspace` | `false` | **Unsafe.** Read the files a document names wherever they are, instead of only in the folders of the workspace and of the document. See [Preview](#preview). |
| `yui.preview.size` | `auto` | The size of the preview: `auto`, `panel` or `<width>x<height>` in logical pixels, such as `640x400`. See [Preview](#preview). |
| `yui.preview.theme` | `auto` | The theme the preview is shown with: `auto` follows the VSCode color theme, `light` and `dark` set it. |
| `yui.mcp.enabled` | `false` | Serve the [tools for agents](#tools-for-agents) on this machine. **YUI: Set Up Agent Tools** turns it on for a workspace. |
| `yui.mcp.port` | `7391` | The port they are served on, at `http://127.0.0.1:<port>/mcp`. Each window serving them needs its own. |

In a workspace that is not trusted (Restricted Mode) the extension checks,
completes and previews documents as usual, but does not serve the tools for
agents, and ignores what the workspace sets for `yui.mcp.enabled`,
`yui.mcp.port`, `yui.lint.models` and `yui.files.readOutsideWorkspace`, which
is then off whoever set it.

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
[hot reload](https://yup.readthedocs.io/en/latest/ui/yui/hot-reload.html) does
in an application.

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

## Shaders and 3D scenes

A document reads more than the documents it imports: the GLSL of its `shader`
blocks, the models of its `Model3D` and the textures of its `Material3D`, all
by paths relative to it. The extension gives them to the tool as an
application finds them next to the document, for the diagnostics and for the
preview alike:

- A shader is compiled from its GLSL, as a development build of an application
  compiles it, and what the compiler says is reported where the `shader` block
  names the file. The bundle a build made of it, next to it (`fx/crt.frag.ysl`
  for `fx/crt.frag`, `.raster.ysl` and `.full.ysl` for a material), is loaded
  in its place, as an application does.
- A model is a `.glb`, or a `.gltf` with the buffers and the images it keeps in
  other files. Textures are PNG, JPEG or WebP images.
- A file is read as it is in its editor while it holds changes that are not
  saved, so typing in the source of a shader updates the problems and the
  preview. One changed outside the editor, as a model exported again, is
  followed when it is in the workspace: otherwise **Refresh** shows it.
- A file larger than 16 MB, or the files of a document beyond 64 MB together,
  are left out: a warning at the top of the document and under the preview
  tells which, and the document is checked and shown as if they were missing.
- A file is read when it is in a folder of the workspace or in the folder of
  the document, where it really is. One that is elsewhere, or that a link
  leads elsewhere to, is left out the same way: open the folder holding both
  to have it read. `yui.files.readOutsideWorkspace` lifts that, for a font or
  an image kept above the workspace: a document then reads any file it names,
  so it is unsafe with documents that are not yours, and is off by default.
  The tools for agents stay held to the workspace.

The preview draws a `Scene3D` with the raster tier of the YUP renderer, as any
OpenGL device does: forward shading, antialiased by multisampling. An
application on a desktop with Metal, Direct3D, Vulkan or WebGPU draws the same
scene with the full tier, whose temporal anti-aliasing, ambient occlusion,
soft shadows and screen-space reflections the preview does not show. A scene
the preview can't draw at all is told under it, with the problems of the
document.

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
    "yui": {
      "type": "http",
      "url": "http://127.0.0.1:7391/mcp",
      "headers": { "Authorization": "Bearer <token>" }
    }
  }
}
```

- Documents are read as they are in their editors, so unsaved changes count.
- The tools that draw run in the preview panel, which opens beside the editor
  when it is closed. What it shows is left alone.
- The server only listens on this machine, and only answers who sends its
  token: the port can be reached by every user and program of the machine.
  The token is made once, kept in the secret storage of VS Code and written
  to `.mcp.json` as the `Authorization` header. It is a secret: the file is
  made readable by its owner only.
- The token is kept out of your commits. In a git repository, a `.mcp.json`
  that is neither tracked nor ignored is first ignored for your clone only,
  by a line naming it in `.git/info/exclude`, which is never committed: the
  `.gitignore` of the project is left alone. A `.mcp.json` that is tracked,
  or that a rule of the project keeps from being ignored (`!.mcp.json`), is
  not written, as is one git could not be told to ignore: the command tells
  so and offers to copy the entry of the
  server, token included, for you to add to your own agent configuration,
  which is outside the repository: `copilot mcp add` and `claude mcp add`
  write there. Outside a git repository the file is written, and keeping it
  out of version control is left to you.
- A `.mcp.json` listing the server without its token, as one written before
  the server asked for it or on another machine, is brought up to date when
  the server starts, in the same way: one that is tracked is left as it is,
  and told about until the entry of the server is taken out of it. Agents
  started before that need to read the file again.
- The server only reads the files of the workspace. A document importing a
  file outside the workspace is checked without that file. The models of
  `yui.lint.models` are read wherever they are.
- It writes one kind of file: the `.png` a call that draws names as `output`,
  inside the workspace.
- It is only served from a trusted workspace.
- Each window serves its own workspace on its own port. When the port set is
  taken, the command finds the next free one and writes it to the workspace
  settings and to `.mcp.json`.

The arguments of the tools are described in the
[tooling page](https://yup.readthedocs.io/en/latest/ui/yui/tooling.html#tools-for-agents)
of the documentation, with `yup_yui_tool mcp`, the command line tool of YUP
serving the same tools with no editor.

## Documentation

The [YUI documentation](https://yup.readthedocs.io/en/latest/ui/yui/index.html)
tells the [language](https://yup.readthedocs.io/en/latest/ui/yui/language.html),
its [styles](https://yup.readthedocs.io/en/latest/ui/yui/styles.html),
[drawings](https://yup.readthedocs.io/en/latest/ui/yui/drawing.html),
[layout](https://yup.readthedocs.io/en/latest/ui/yui/layout.html) and
[documents](https://yup.readthedocs.io/en/latest/ui/yui/documents.html), and
the [tooling](https://yup.readthedocs.io/en/latest/ui/yui/tooling.html) this
extension is made of. Documents to start from are in
[`examples/graphics/data/yui`](https://github.com/kunitoki/yup/tree/main/examples/graphics/data/yui)
of the YUP repository.

## Source and license

The extension is part of the YUP repository, in
[`modules/yup_gui_markup/tools/vscode-yui`](https://github.com/kunitoki/yup/tree/main/modules/yup_gui_markup/tools/vscode-yui),
where `DEVELOPMENT.md` tells how to build it. Problems are reported in its
[issues](https://github.com/kunitoki/yup/issues).

ISC. Copyright (c) 2024-2026 kunitoki@gmail.com.
