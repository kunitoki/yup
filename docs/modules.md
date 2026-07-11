# Modules

YUP is organized into focused modules. Each module is a self-contained unit with
its own [module declaration](build-system/module-format.md) and follows the
`yup_*` naming convention. You depend on exactly the modules you need; transitive
dependencies are pulled in automatically by the [CMake API](build-system/cmake-api.md).

```{note}
All `yup_*` modules share the same version number. Modules also depend on a few
bundled third-party libraries (`zlib`, `rive`, `rive_renderer`, `libclipper2`,
`xsimd`), which are resolved for you by the build system. In the diagrams below,
third-party dependencies are drawn with a dashed outline.
```

Each module is listed below with its direct dependencies. Arrows point from a
module to the things it depends on.

## Core

Foundational modules used across the whole framework. See the [Core](core/index.md) area.

### yup_core

The foundation every other module builds on: strings, containers, files and
streams, memory management, math, time, threading, networking, and data
interchange. It has no YUP dependencies and only pulls in `zlib` for
compression.

```mermaid
flowchart LR
    yup_core:::self --> zlib:::ext
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_simd

Vectorized math primitives across SSE, AVX, FMA, NEON, and Apple's Accelerate
framework, wrapping the `xsimd` library behind a YUP-friendly API. Used wherever
tight numeric loops matter — DSP, audio, and graphics.

```mermaid
flowchart LR
    yup_simd:::self --> yup_core
    yup_simd --> xsimd:::ext
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

## Events & multithreading

The event loop and messaging layer. See the [Multithreading](multithreading/index.md)
area; threading primitives themselves live in `yup_core`.

### yup_events

The application event loop and messaging layer: the message thread, timers,
async callbacks, and inter-object notifications. It underpins any interactive or
long-running YUP application.

```mermaid
flowchart LR
    yup_events:::self --> yup_core
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

## Graphics

The rendering stack. See the [Graphics](graphics/index.md) area; bitmap image
handling is documented separately in the [Imaging](imaging/index.md) area.

### yup_shading

Shader authoring support: shader-source containers, transpilation, and the
`.ysl` shader-bundle format consumed by the graphics RHI for
[offline shader compilation](graphics/rhi/offline-shaders.md).

```mermaid
flowchart LR
    yup_shading:::self --> yup_core
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_graphics

The 2D drawing stack and the low-level GPU RHI, rendered through the Rive
renderer. Covers the graphics context, primitives, paths, fonts, SVG, imaging,
and GPU pipelines.

```mermaid
flowchart LR
    yup_graphics:::self --> yup_core
    yup_graphics --> yup_simd
    yup_graphics --> yup_shading
    yup_graphics --> rive:::ext
    yup_graphics --> rive_renderer:::ext
    yup_graphics --> libclipper2:::ext
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_animation

A Lottie-compatible animation model with playback, rendering, and export,
layered on top of the graphics stack.

```mermaid
flowchart LR
    yup_animation:::self --> yup_core
    yup_animation --> yup_graphics
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

## Data

Structured, observable data models. See the [Data](data/index.md) area.

### yup_data_model

The `DataTree` data model: hierarchical, observable, transactional,
schema-validatable data with an XPath-like query engine.

```mermaid
flowchart LR
    yup_data_model:::self --> yup_events
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

## UI

The GUI toolkit. See the [UI](ui/index.md) area.

### yup_gui

The GUI toolkit: components, windowing, events, layout, and widgets that paint
through the graphics stack and bind to the data model.

```mermaid
flowchart LR
    yup_gui:::self --> yup_events
    yup_gui --> yup_data_model
    yup_gui --> yup_graphics
    yup_gui --> rive:::ext
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_audio_gui

Audio-specific GUI components — waveforms, meters, spectrum analyzers, and
audio-graph editor views — bridging the UI and audio stacks.

```mermaid
flowchart LR
    yup_audio_gui:::self --> yup_audio_basics
    yup_audio_gui --> yup_audio_formats
    yup_audio_gui --> yup_audio_processors
    yup_audio_gui --> yup_audio_graph
    yup_audio_gui --> yup_dsp
    yup_audio_gui --> yup_gui
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

## Audio

The audio-first stack. See the [Audio](audio/index.md) area.

### yup_audio_basics

Core audio and MIDI data types: audio buffers, MIDI messages, synthesis
helpers, and processing utilities.

```mermaid
flowchart LR
    yup_audio_basics:::self --> yup_core
    yup_audio_basics --> yup_simd
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_audio_devices

Real-time audio and MIDI I/O: enumerate, open, and stream from the platform's
audio and MIDI devices.

```mermaid
flowchart LR
    yup_audio_devices:::self --> yup_audio_basics
    yup_audio_devices --> yup_events
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_audio_formats

Reading and writing audio files across the supported sound formats.

```mermaid
flowchart LR
    yup_audio_formats:::self --> yup_audio_basics
    yup_audio_formats --> yup_simd
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_dsp

DSP building blocks: filters and filter designers, crossovers, and spectral
analysis.

```mermaid
flowchart LR
    yup_dsp:::self --> yup_core
    yup_dsp --> yup_audio_basics
    yup_dsp --> yup_simd
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_audio_processors

The `AudioProcessor` model — the unit of audio processing that plugins and the
audio graph are built from — plus parameter handling.

```mermaid
flowchart LR
    yup_audio_processors:::self --> yup_audio_basics
    yup_audio_processors --> yup_data_model
    yup_audio_processors --> yup_dsp
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_audio_graph

A node-based processing graph of `AudioProcessor`s for routing audio and MIDI,
backed by the data model.

```mermaid
flowchart LR
    yup_audio_graph:::self --> yup_audio_processors
    yup_audio_graph --> yup_data_model
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_audio_plugin_client

Wrap your own `AudioProcessor` as a CLAP or VST3 plugin (and a standalone app).
See [Building plugins](build-system/building-plugins.md).

```mermaid
flowchart LR
    yup_audio_plugin_client:::self --> yup_audio_processors
    yup_audio_plugin_client --> yup_gui
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

### yup_audio_plugin_host

In-process hosting of third-party VST3, CLAP, LV2, and AU (v2/v3) plugins.

```mermaid
flowchart LR
    yup_audio_plugin_host:::self --> yup_audio_processors
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

## Scripting

Bindings for driving YUP from scripts. See the [Scripting](scripting/index.md) area.

### yup_python

Python bindings for creating and driving YUP applications from scripts.

```mermaid
flowchart LR
    yup_python:::self --> yup_core
    classDef self fill:#6366f1,color:#fff,stroke:#4f46e5;
    classDef ext fill:#f3f4f6,color:#374151,stroke:#9ca3af,stroke-dasharray:4 3;
```

## Complete dependency graph

The full `yup_*` module graph in one view (third-party dependencies omitted for
clarity).

```mermaid
flowchart TD
    simd[yup_simd] --> core[yup_core]
    events[yup_events] --> core
    shading[yup_shading] --> core
    python[yup_python] --> core

    graphics[yup_graphics] --> core
    graphics --> simd
    graphics --> shading

    animation[yup_animation] --> core
    animation --> graphics

    data[yup_data_model] --> events

    gui[yup_gui] --> events
    gui --> data
    gui --> graphics

    ab[yup_audio_basics] --> core
    ab --> simd

    dsp[yup_dsp] --> core
    dsp --> ab
    dsp --> simd

    afmt[yup_audio_formats] --> ab
    afmt --> simd

    adev[yup_audio_devices] --> ab
    adev --> events

    aproc[yup_audio_processors] --> ab
    aproc --> data
    aproc --> dsp

    agraph[yup_audio_graph] --> aproc
    agraph --> data

    aclient[yup_audio_plugin_client] --> aproc
    aclient --> gui

    ahost[yup_audio_plugin_host] --> aproc

    agui[yup_audio_gui] --> ab
    agui --> afmt
    agui --> aproc
    agui --> agraph
    agui --> dsp
    agui --> gui
```

## See also

- [Module format](build-system/module-format.md) - how a module is declared.
- [CMake API](build-system/cmake-api.md) - how to add modules to your target.
