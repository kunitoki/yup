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
- [All modules and their usage](modules.md) - the full module list, grouped by area, with dependencies.
- [Build System](build-system/index.md) - the CMake API, module format, and packaging.
- [Core](core/index.md) - core utilities, files, strings, memory, and SIMD.
- [Multithreading](multithreading/index.md) - threads, the message manager, and thread-safe patterns.
- [Data](data/index.md) - the `DataTree` model and serialization.
- [Graphics](graphics/index.md) - the graphics context, 2D drawing, the **RHI** GPU layer, shaders, fonts, SVG, and animation.
- [Imaging](imaging/index.md) - bitmap images: pixels, loading, saving, and drawing.
- [UI](ui/index.md) - components, windowing, events, layout, and widgets.
- [DSP](dsp/index.md) - filters, filter design, FFTs, dynamics, metering, onset detection, convolution, resampling, and time-stretching.
- [Audio](audio/index.md) - audio devices, formats, DSP, the audio graph, processors, and plugin hosting/client wrappers.
- [Scripting](scripting/index.md) - the Python bindings layer.

## Quick links

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
build-system/index
core/index
multithreading/index
data/index
graphics/index
imaging/index
ui/index
dsp/index
audio/index
scripting/index
```
