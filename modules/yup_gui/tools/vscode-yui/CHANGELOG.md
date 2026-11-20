# Changelog

All notable changes to this extension will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.0.0] - Unreleased

### Added

- TextMate grammar (`source.yui`) for `.yui` files: `import`, `tokens`,
  `drawing`, `palette`, `style` and `component` declarations, `property`,
  `property alias` and `signal`, `let` and `repeat` scopes, shapes and filter
  primitives, selectors with classes, ids and pseudo classes, `@keyframes`,
  colors, units, built-in functions, bindings and handler arrows, comments,
  strings and numbers.
- Language configuration: comment toggling, bracket matching and auto-closing,
  and indentation rules.
- Snippets: component, drawing, repeat, style, rule, transition, keyframes,
  animation, palette, tokens, property, alias, signal, the `Row` / `Column` /
  `Grid` / `Stack` layouts, binding, handlers and import.
- Diagnostics from `yup_yui_tool lint`, on open, on save and debounced while
  typing. The document is checked as it is in its editor, with the files it
  imports, and problems in imported documents are reported on those documents.
- The `yui.lint.models` setting points at a JSON file declaring the application
  models, so bindings such as `value <=> params.cutoff;` are resolved instead of
  being reported as unknown elements.
- Completions from `yup_yui_tool schema`: instantiable types, the properties and
  signals of the enclosing type, enumeration values, style pseudo classes and
  palette roles.
- **YUI: Open Preview**, running the document live in a webview panel beside
  the editor: the interface answers the mouse and the keyboard, and follows
  what is typed, the active editor and the color theme. The toolbar of the
  panel chooses what is shown (the interface, or a drawing, a palette or a
  component the document declares, a drawing painted by a chosen component
  type), the size (automatic, the one of the panel or fixed), the theme and the
  zoom. The size and the theme are the `yui.preview.size` and
  `yui.preview.theme` settings.
- **YUI: Import SVG as Drawing**, converting an `.svg` into a `drawing` block
  through `yup_yui_tool import-svg`.
- An extension and file icon (`images/yui-icon.png`) for `.yui` documents.
- The `yui.lint.enabled` setting.
- The tool is packaged as a WebAssembly module (`tools/package-tool.sh`), so
  one package works on every platform and holds no native binary. A package
  built without it keeps highlighting and snippets, and turns the rest off.
