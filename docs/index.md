# YUP Documentation

**YUP** is a C++20 framework for building native applications, audio tools, and
audio plugins from a single codebase across desktop, mobile, and the web. It
combines permissively licensed JUCE7-derived foundations with modern GPU-native
rendering through the open source [Rive](https://rive.app/) renderer and YUP's
own graphics, GUI, DSP, audio graph, and plugin layers.

```{warning}
YUP is under active early-stage development. APIs may change while the framework
is being shaped. This documentation tracks the current state of the code in the
repository.
```

## Documentation areas

The documentation is organized by functional area. Each area contains its own
concept guides, walkthroughs, and reference material.

- [Getting Started](getting-started/index.md) - install YUP, build the examples, and create your first project.
- [Graphics](graphics/index.md) - the graphics context, 2D drawing, the **RHI** GPU layer, shaders, fonts, SVG, and animation.
- [Imaging](imaging/index.md) - bitmap images: pixels, loading, saving, and drawing.
- [UI](ui/index.md) - components, windowing, events, layout, and widgets.
- [Audio](audio/index.md) - audio devices, formats, DSP, the audio graph, processors, and plugin hosting/client wrappers.
- [Data](data/index.md) - the `DataTree` model and serialization.
- [Multithreading](multithreading/index.md) - threads, the message manager, and thread-safe patterns.
- [Core](core/index.md) - core utilities, files, strings, memory, and SIMD.
- [Scripting](scripting/index.md) - the Python bindings layer.
- [Build System](build-system/index.md) - the CMake API, module format, and packaging.

## Quick links

- [All modules and their usage](modules.md) - the full module list, grouped by area, with dependencies.
- Example apps: [graphics](https://github.com/kunitoki/yup/tree/main/examples/graphics), [audio graph](https://github.com/kunitoki/yup/tree/main/examples/audiograph), [plugin](https://github.com/kunitoki/yup/tree/main/examples/plugin), [console](https://github.com/kunitoki/yup/tree/main/examples/console).
- [Building standalone applications](build-system/building-standalone.md)
- [Building audio plugins](build-system/building-plugins.md)

## External resources

- [YUP on GitHub](https://github.com/kunitoki/yup) - source code, issues, releases, discussion and roadmaps.
- [YUP on DeepWiki](https://deepwiki.com/kunitoki/yup) - high level information on the framework extracted by a llm.

```{toctree}
:hidden:
:maxdepth: 2

getting-started/index
modules
graphics/index
imaging/index
ui/index
audio/index
data/index
multithreading/index
core/index
scripting/index
build-system/index
```
