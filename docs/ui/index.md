# UI

The GUI layer: components, windowing, event handling, layout, and widgets that
paint through the [graphics](../graphics/index.md) stack.

**Modules covered:** `yup_gui`, `yup_events`, `yup_audio_gui`.

```{warning}
**Work in progress.** This area is still being written. Concept guides for
windowing, widgets, and theming are still to come.
```

## Topics

- **Components** — the `Component` tree, painting, input, and lifecycle.
- **Styling** — colors, properties, and `ComponentStyle` with theme cascading.
- **Drag and drop** — receiving drops and starting drags, in one window or across several.
- **Effects** — GPU shader effects applied to a component subtree.
- **Caching** — cache a component's paint output to a GPU texture.
- **Snapshots** — capture a component subtree to a CPU-side `Image`.
- **Layout** — `FlexBox` and `Grid`, the CSS-modelled layout containers.
- **Windowing** — native and web windows that host the graphics context.
- **Events** — the message loop, timers, and event dispatch (`yup_events`).
- **Widgets** — buttons, sliders, labels, text editors, and audio displays
  (waveform, spectrogram, scope) from `yup_audio_gui`.
- **Artboards** — Rive artboards as components, with node access and
  ViewModel data binding.

## Guides

- [Component basics](component-basics.md) — the `Component`
  tree, painting, input, and lifecycle.
- [Drag and drop](component-drag-and-drop.md) — drop targets, drag
  sources, payloads, and the drag session.
- [Component styling](component-styling.md) — colors, properties,
  `ComponentStyle`, and `ApplicationTheme`.
- [The `.yui` language](yui-language.md) - the syntax of `.yui` files: blocks,
  elements, values, declarations and imports.
- [How YUI fits together](yui-architecture.md) - diagrams of the classes behind
  `.yui` documents, styling, drawings and bindings, and how data flows between them.
- [Data-driven drawing](yui-drawing.md) - paint components from a tree of
  shapes whose values are expressions over the component's properties.
- [Stylesheets](yui-styles.md) - selectors, the cascade, palettes,
  transitions, shadows and cursors for any component tree.
- [Reactive bindings](yui-reactivity.md) - properties computed from other
  properties, re-evaluated whenever what they read changes.
- [Complete UIs in `.yui`](yui-documents.md) - `YuiDocument`, tokens,
  composite components, handlers, actions and models.
- [Hot reloading `.yui` files](yui-hot-reload.md) - `YuiLoader`, watching
  files while developing and embedding them with `yup_add_yui_resources`.
- [Component effects (shaders)](component-effects.md) — apply GPU shader
  effects to `Component` subtrees.
- [Component caching](component-caching.md) — `setCachedToTexture`
  for GPU texture caching.
- [Components in 3D](component-3d.md) — present live, interactive
  components on 3D geometry with `setManuallyComposited` and `MeshSurfaceMapper`.
- [Component snapshots](component-snapshots.md) — `snapshotToImage` and
  `snapshotToTexture` for pixel capture.
- [Gestures](gestures.md) - touch and trackpad gestures: tap, long press,
  pinch, rotate, multi-finger pan and swipe, with `setWantedGestures`.
- [Component Layout](component-layout.md) — `FlexBox` and `Grid`, flexible and track-based
  layout modelled on CSS.
- [Component paint profiling](component-profiling.md) — measure and
  reduce the cost of `Component::paint`.
- [Toast notifications](toast-notifications.md) - the cross-platform `ToastNotification` utility and
  its `ToastTemplate`, delivered by the platform notification backend.

## Additional Components

- [Code editor](code-editor.md) — `CodeDocument`, `SyntaxDefinition`,
  `CodeTokeniser`, and the syntax-highlighting `CodeEditor` component.
- [Artboards (Rive)](artboard.md) — `ArtboardFile`, `Artboard`,
  `ArtboardNode`, and ViewModel data binding.
- [MIDI keyboard](midi-keyboard.md) - `MidiKeyboardComponent`, multitouch
  playing, scroll buttons and the available / visible key ranges.
- [List box](list-box.md) - `ListBox` and `ListBoxModel`, custom rows, change
  notifications, touch scrolling and pull-to-refresh.
- [Tree view](tree-view.md) - `TreeView` and `TreeViewItem`, lazy loading,
  custom content, drag and drop reordering, `DataTree` mirroring and saved
  openness state.
- [Tabs](tabs.md) - `TabBar`, `TabButton` and `TabComponent`: segmented
  controls and tabbed pages with animated selection, reordering, overflow and
  closable tabs.
- [File chooser](file-chooser.md) - `FileChooser` for opening and saving
  files, and how picks behave on the web.

```{toctree}
:hidden:
:maxdepth: 1

component-basics
layout
component-drag-and-drop
gestures
component-styling
yui-language
yui-architecture
yui-drawing
yui-styles
yui-reactivity
yui-documents
yui-hot-reload
component-effects
component-3d
component-caching
component-snapshots
component-profiling
code-editor
artboard
midi-keyboard
list-box
tree-view
tabs
file-chooser
```
