# Changelog

All notable changes to this extension will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [2.0.1] - Unreleased

### Added

- TextMate grammar (`source.yui`) for `.yui` files: `import`, `tokens`,
  `drawing`, `shader`, `palette`, `style` and `component` declarations,
  `property`, `property alias` and `signal`, `let` and `repeat` scopes, shapes
  and filter primitives, selectors with classes, ids and pseudo classes,
  `@keyframes`, colors, units, built-in functions, bindings and handler arrows,
  comments, strings and numbers.
- Language configuration: comment toggling, bracket matching and auto-closing,
  and indentation rules.
- Snippets: component, drawing, shader, repeat, style, rule, transition,
  keyframes, animation, palette, tokens, property, alias, signal, the `Row` /
  `Column` / `Grid` / `Stack` layouts, a 3D scene, a model, binding, handlers
  and import.
- Diagnostics from `yup_yui_tool lint`, on open, on save and debounced while
  typing. The document is checked as it is in its editor, with the files it
  imports, and problems in imported documents are reported on those documents,
  where a file several open documents import shows what all of them tell.
- The `yui.lint.models` setting points at a JSON file declaring the application
  models, so bindings such as `value <=> params.cutoff;` are resolved instead of
  being reported as unknown elements.
- Completions, outline, go to definition, hovers and color swatches, all
  answered by `yup_yui_tool` (`complete`, `outline`, `definition`, `hover`) for
  the document as it is typed: component types, the properties and signals of
  the enclosing type, enumeration values, the elements and properties of
  drawings, the drawings, palettes, palette colors and tokens declared, model
  properties and style pseudo classes.
- **YUI: Open Preview**, running the document live in a webview panel beside
  the editor: the interface answers the mouse and the keyboard, and follows
  what is typed, the active editor and the color theme. The toolbar of the
  panel chooses what is shown (the interface, or a drawing, a palette or a
  component the document or the files it imports declare), the size (automatic,
  the one of the panel or fixed), the theme and the zoom. A drawing and a
  component are shown where the interface uses them, or a drawing on its own
  painting a chosen component type. `Fit` scales what is shown to the panel, up
  or down, and every zoom is drawn sharp. `Guides` shows the bounds, the paint
  margin and the distances of the components clicked, or of the one the caret
  is in. `Statistics` tells the frames a second, the time painting takes and
  the components taking the longest to paint. The size and the theme are the `yui.preview.size` and
  `yui.preview.theme` settings.
- **YUI: Import SVG as Drawing**, converting an `.svg` into a `drawing` block
  through `yup_yui_tool import-svg` and saving it as a `.yui` file.
- An extension and file icon (`images/yui-icon.png`) for `.yui` documents.
- The `yui.lint.enabled` setting.
- The tool is packaged as a WebAssembly module (`tools/package-tool.sh`), so
  one package works on every platform and holds no native binary. A package
  built without it keeps highlighting and snippets, and turns the rest off.
  A tool that stops, or that does not answer in 30 seconds, is started again:
  at once the first time, then after a wait growing up to a minute, or when a
  document is checked again.
- **YUI: Set Up Agent Tools**, serving coding agents such as Claude Code and
  GitHub Copilot the problems (`yui_check`), the layout (`yui_inspect`), an
  image (`yui_render`), the component types (`yui_schema`) and the live preview
  (`yui_capture_preview`) of the `.yui` documents of a workspace, over the Model
  Context Protocol. It is off until the command is run, which sets
  `yui.mcp.enabled` and `yui.mcp.port` for the workspace and lists the server in
  its `.mcp.json`. The server only listens on this machine and only answers who
  sends its token, kept in the secret storage of the editor and written to
  `.mcp.json`, which git is first told to keep out of the commits of the clone
  (in its `info/exclude`). A `.mcp.json` under version control is left as it
  is, and the entry of the server is offered to copy instead. A call naming a
  `reference` image reads it, and one naming an `output` writes the image
  there, both inside the workspace only.
- Workspace trust: in Restricted Mode the tools for agents are not served, and
  the `yui.mcp` and `yui.lint.models` settings of the workspace are ignored.
- `yui.files.readOutsideWorkspace`, off by default and unsafe, lets a document
  read the files it names outside the workspace and its own folder, such as a
  font kept above them, instead of leaving them out with a warning. It is
  ignored in Restricted Mode, and the tools for agents stay held to the
  workspace.
- The extension is bundled with esbuild, and needs VS Code 1.90 or later.
- Shaders and 3D scenes: the `shader` blocks of a document are compiled from
  their GLSL and the errors of the compiler reported at the block, the
  `Scene3D`, `Node3D`, `Camera3D`, `Light3D`, `Mesh3D`, `Model3D` and
  `Material3D` types are checked, completed and shown in the preview, and
  shaders are listed in the outline, completed where they are named and reached
  by go to definition. The files a document reads next to it (shader sources
  or the `.ysl` bundles a build made of them, `.glb` and `.gltf` models with
  their buffers and images, PNG, JPEG and WebP textures) are given to the tool
  for the diagnostics, the preview and the tools for agents: as they are in
  their editors while not saved, and again when they change on disk in the
  workspace. A file larger than 16 MB, or outside the workspace and the folder
  of the document, is left out with a warning. The preview draws a scene with
  the raster tier of the renderer, and tells one it can't draw.
