# UI

The GUI layer: components, windowing, event handling, layout, and widgets that
paint through the [graphics](../graphics/index.md) stack.

**Modules covered:** `yup_gui`, `yup_events`, `yup_audio_gui`.

```{warning}
**Work in progress.** This area is still being written. Concept guides for
windowing, layout, widgets, and theming are still to come.
```

## Topics

- **Components** — the `Component` tree, painting, input, and lifecycle.
- **Styling** — colors, metrics, and `ComponentStyle` with theme cascading.
- **Drag and drop** — receiving external file and text drops.
- **Effects** — GPU shader effects applied to a component subtree.
- **Caching** — cache a component's paint output to a GPU texture.
- **Snapshots** — capture a component subtree to a CPU-side `Image`.
- **Windowing** — native and web windows that host the graphics context.
- **Events** — the message loop, timers, and event dispatch (`yup_events`).
- **Widgets** — buttons, sliders, labels, text editors, and audio displays
  (waveform, spectrogram, scope) from `yup_audio_gui`.

## Guides

- [Component basics](component-basics.md) — the `Component`
  tree, painting, input, and lifecycle.
- [Drag and drop](component-drag-and-drop.md) — receiving external
  file and text payloads.
- [Component styling](component-styling.md) — colors, metrics,
  `ComponentStyle`, and `ApplicationTheme`.
- [Component effects (shaders)](component-effects.md) — apply GPU shader
  effects to `Component` subtrees.
- [Component caching](component-caching.md) — `setCachedToTexture`
  for GPU texture caching.
- [Component snapshots](component-snapshots.md) — `snapshotToImage` and
  `snapshotToTexture` for pixel capture.
- [Component paint profiling](component-profiling.md) — measure and
  reduce the cost of `Component::paint`.
- [Toast notifications](toast-notifications.md) - the cross-platform `ToastNotification` utility and
  its `ToastTemplate`, delivered by the platform notification backend.

## Additional Components

- [Code editor](code-editor.md) — `CodeDocument`, `SyntaxDefinition`,
  `CodeTokeniser`, and the syntax-highlighting `CodeEditor` component.

```{toctree}
:hidden:
:maxdepth: 1

component-basics
component-drag-and-drop
component-styling
component-effects
component-caching
component-snapshots
component-profiling
code-editor
```
