# YUI - the declarative UI language

**YUI** is YUP's declarative interface language. A `.yui` file describes a
component tree, how it is laid out, how it is painted and how its values are
wired together, in text that an application loads at startup or reloads while
it runs.

<p align="center">
  <img class="light-only rounded" src="../../_static/images/ui/yui/drawing_fluted-knob_light.jpg#gh-light-mode-only" alt="Three glossy black knobs at different values, each one a slider painted by the FlutedKnob drawing" width="640">
  <img class="dark-only rounded" src="../../_static/images/ui/yui/drawing_fluted-knob_dark.jpg#gh-dark-mode-only" alt="Three glossy black knobs at different values, each one a slider painted by the FlutedKnob drawing" width="640">
</p>

**Modules covered:** `yup_gui_markup` (the `Yui*` classes) on top of `yup_gui`,
with `yup_audio_gui` for parameter binding and audio components, and
`yup_renderer` for 3D scenes.

Everything YUI does is also reachable from C++ - it builds the same
[`Component`](../component-basics.md) tree, sets the same
[properties](../component-styling.md) and paints with the same
[`Graphics`](../../graphics/graphics-class.md) API. The language exists so that
layout, look and wiring live in files you can edit without recompiling.

YUI is opt-in: an interface written in C++ needs `yup_gui` only. To use it, add
the module to your target and include its header, which brings `yup_gui` along:

```cmake
yup_standalone_app (
    # ...
    MODULES
        yup::yup_gui
        yup::yup_gui_markup)
```

```cpp
#include <yup_gui_markup/yup_gui_markup.h>
```

## What a document is made of

| Block | What it declares | Page |
| --- | --- | --- |
| `import` | Another `.yui` file merged into this one | [The `.yui` language](language.md#imports) |
| `tokens` | Typed values shared by the whole document | [Complete UIs](documents.md#tokens) |
| `palette` | Colors derived from a few source colors | [Stylesheets](styles.md#palettes) |
| `style` | Selectors, the cascade, transitions and `@keyframes` | [Stylesheets](styles.md) |
| `drawing` | Shapes painting a component from its properties | [Data-driven drawing](drawing.md) |
| `shader` | A GLSL shader and its inputs, for effects, drawings, filters and 3D materials | [Shaders](shaders.md) |
| `component` | A new composite component type | [Complete UIs](documents.md#composite-components) |
| `Type { ... }` | The root component instance | [Complete UIs](documents.md) |

## In this area

Start with the language, then pick the area you need.

- [The `.yui` language](language.md) - the syntax of `.yui` files: blocks,
  elements, values, declarations and imports.
- [How YUI fits together](architecture.md) - diagrams of the classes behind
  `.yui` documents, styling, drawings and bindings, and how data flows between
  them.
- [Declarative layout](layout.md) - `Row`, `Column`, `FlexBox`, `Grid`,
  `Stack`, item sizing, base bounds and styled layouts.
- [Stylesheets](styles.md) - selectors, the cascade, palettes, transitions,
  shadows, glows, CSS filters and cursors for any component tree.
- [Data-driven drawing](drawing.md) - paint components from a tree of shapes
  whose values are expressions over the component's properties.
- [Widgets in drawings](widgets.md) - what each widget tells a drawing: its
  geometry, its data series and its parts.
- [Shaders](shaders.md) - GLSL shaders declared in a document: as the effect of
  a component, painted by a drawing, in a filter, or as a 3D material.
- [3D scenes](scenes.md) - `Scene3D`, cameras, lights, meshes, glTF models and
  materials written as elements, with clips, picking, draggable parts and 2D
  components on 3D objects.
- [Reactive bindings](reactivity.md) - properties computed from other
  properties, re-evaluated whenever what they read changes.
- [Complete UIs in `.yui`](documents.md) - `YuiDocument`, tokens, composite
  components, handlers, actions, models and components set up from C++.
- [Hot reloading `.yui` files](hot-reload.md) - `YuiLoader`, watching files
  while developing and embedding them, with the shaders, the models and the
  textures they read, with `yup_add_yui_resources`.
- [Tools for `.yui` files](tooling.md) - checking files from the command line
  and while configuring a build, the schema, importing SVGs and the VS Code
  extension.

## Related areas

- [Component basics](../component-basics.md) - the `Component` tree a document
  builds.
- [Component styling](../component-styling.md) - the property and theme model a
  stylesheet writes into.
- [Component layout](../component-layout.md) - `FlexBox` and `Grid` from C++.
- [Drawables & SVG](../../graphics/svg.md) - the SVG importer feeding
  `drawing` blocks.
- [3D](../../3d/index.md) - the scenes, the glTF loader and the renderer behind
  `Scene3D`.
- [Audio GUI](../../audio/audio-gui.md#in-yui-documents) - the keyboard, the
  meters and the analyzers a document creates, and how they get their state.
- [Python bindings for YUI](../../scripting/python-yui.md) - loading and
  driving documents from Python.

```{toctree}
:hidden:
:maxdepth: 1

language
architecture
layout
styles
drawing
widgets
shaders
scenes
reactivity
documents
hot-reload
tooling
```
