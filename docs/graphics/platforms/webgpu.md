# WebGPU

WebGPU is the modern web backend: a browser GPU API with real compute shaders
and storage resources, which WebGL2 does not have. It runs only in Emscripten
builds.

| | |
| --- | --- |
| `GpuPlatform` | `WebGPU` |
| Runs on | Emscripten / WASM |
| Default | Yes, when the build enables it |
| Shader variant | `wgsl` |
| Compute | Yes |
| Build flag | `ENABLE_EMSCRIPTEN_WEBGPU` on the target, on by default |

## Building

`yup_standalone_app` takes `ENABLE_EMSCRIPTEN_WEBGPU`, which defaults to `ON`.
It adds Emscripten's `emdawnwebgpu` port and defines `RIVE_WEBGPU`. Set it to
`OFF` and the same source builds against WebGL2 through the
[OpenGL ES](opengl.md) backend instead:

```cmake
yup_standalone_app (my_app
    ENABLE_EMSCRIPTEN_WEBGPU OFF
    ...)
```

See [building standalone applications](../../build-system/building-standalone.md)
for the rest of the Emscripten options.

## What you get

Compared with WebGL2, WebGPU is a large step up:

- **Compute.** `GpuComputePipeline` and `GpuComputePass` work, including
  storage textures written by compute and sampled later.
- **Fragment storage buffers** are available.
- **Dynamic uniform offsets** are available.

This makes WebGPU the only web target where GPU compute - simulation, image
processing, DSP on the GPU - is possible at all.

## Compatibility mode

A browser that cannot provide a full WebGPU device - an older GPU, or an
implementation layered over OpenGL ES - falls back to *compatibility mode*,
with reduced features and limits. Ask for it and degrade gracefully:

```cpp
if (device->isCompatibilityMode())
{
    // fewer features and smaller limits than a full WebGPU device
}
```

## Compute passes and frames

Unlike Metal, Direct3D and Vulkan, a compute pass begun on a frame is a
standalone pass on WebGPU, submitted when it finishes. It therefore runs
*before* every render pass of that frame: the render passes see what the
dispatches write, as long as the compute pass finishes first, but the
dispatches never see what the frame's render passes draw. Structure web compute
work as "dispatch, then draw".

## Shaders

Pipelines take the `wgsl` variant of a [shader
bundle](../rhi/offline-shaders.md). WGSL is restrictive compared with GLSL;
[GLSL on the WebGPU target](../rhi/wgsl-shaders.md) describes what the
transpiler translates, how it assigns bindings, and what WGSL cannot express.
Read that page before porting an existing shader to the web.

## Things to know

- There is no frame timing or device-loss reporting on WebGPU, and no driver
  pipeline cache.
- Audio needs Emscripten threads, which only cross-origin isolated pages can
  run. `YUP_ENABLE_EMSCRIPTEN_THREADS` is on by default; it is unrelated to the
  graphics backend, but it is the other thing that makes a web build fail to
  start.

## Related pages

- [Graphics platforms](index.md) - choosing a backend and the capability table.
- [OpenGL and OpenGL ES](opengl.md) - the WebGL2 fallback.
- [GLSL on the WebGPU target](../rhi/wgsl-shaders.md) - the shader translation.
- [Building standalone applications](../../build-system/building-standalone.md) -
  the Emscripten target options.
