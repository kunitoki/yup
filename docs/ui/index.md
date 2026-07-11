# UI

The GUI layer: components, windowing, event handling, layout, and widgets that
paint through the [graphics](../graphics/index.md) stack.

**Modules covered:** `yup_gui`, `yup_events`, `yup_audio_gui`.

```{warning}
**Work in progress.** This area is still being written and currently covers only
the [profiling guide](profiling-component-paint.md) below. Concept guides for the
component model, windowing, layout, widgets, and theming are still to come.
```

## Topics

- **Components** - the `Component` tree, painting, hit-testing, and focus.
- **Windowing** - native and web windows that host the graphics context.
- **Events** - the message loop, timers, and event dispatch (`yup_events`).
- **Widgets** - buttons, sliders, labels, text editors, and audio displays
  (waveform, spectrogram, scope) from `yup_audio_gui`.

## Guides

- [Profiling component paint](profiling-component-paint.md) - measure and reduce
  the cost of `Component::paint`.

```{toctree}
:hidden:
:maxdepth: 1

profiling-component-paint
```
