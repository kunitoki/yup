# OpenGL and OpenGL ES

The GL family is the portable fallback: desktop OpenGL on Linux, Windows and
macOS, OpenGL ES on Android, and WebGL2 - which is OpenGL ES 3.0 - in the
browser. It is the default only on Linux, and the fallback everywhere else.

| | |
| --- | --- |
| `GpuPlatform` | `OpenGL`, `OpenGLES` |
| Runs on | Linux, Windows, macOS, Android, WebGL2 in the browser |
| Default | On Linux; on Android after Vulkan; on the web without WebGPU |
| Shader variant | `glsl` (desktop), `essl` (ES / WebGL2) |
| Compute | Desktop GL 4.3+ or GLES 3.1+ only, never on WebGL2 |
| Build flag | `YUP_RIVE_USE_OPENGL`, on by default |

## Versions

The renderer targets OpenGL 4.5 and OpenGL ES 3.1 by default. Those are set by
`YUP_RIVE_OPENGL_MAJOR` / `YUP_RIVE_OPENGL_MINOR` and
`YUP_RIVE_OPENGLES_MAJOR` / `YUP_RIVE_OPENGLES_MINOR`, which you can lower if
you must support older drivers - at the cost of compute, which needs GL 4.3 or
GLES 3.1 at minimum.

## What you get, and what you don't

GL is the most restricted backend. Everything `Graphics` draws works; the
GPU-compute side of the [RHI](../rhi/index.md) mostly does not:

- **Compute** is available only with desktop GL 4.3+ or GLES 3.1+ entry points,
  and the `YUP_RHI_USE_GL_COMPUTE` build flag - on by default for non-WASM GL
  builds. The device probes the runtime version as well, so always ask
  `GpuDevice::isComputeAvailable()` instead of assuming.
- **WebGL2 has no compute at all.** It is GLES 3.0, which does not have compute
  shaders or shader storage buffers.
- **Fragment shaders cannot read storage buffers.**
  `isFragmentStorageBufferAvailable()` is false on desktop GL, GLES and WebGL2
  alike.
- **Compute cannot write storage textures.** `isStorageTextureAvailable()` is
  false throughout the family.
- **Dynamic uniform offsets** do work, on all three.
- No frame timings, no device-loss reporting, no driver pipeline cache.

```{admonition} Designing for the web
:class: tip
If a feature has to run in a browser without WebGPU, it has to run as a
fragment shader reading textures. Storage buffers and compute passes are not an
option there. Writing the fragment-shader path first and treating compute as an
optimization keeps one code path working everywhere.
```

A compute pass begun on a frame - `GpuComputePass::begin (frame)` - is a
standalone pass on GL, submitted when it finishes, exactly like
`begin (device)`.

## Threads and the current context

GL contexts are thread-affine, and the windowing layer may bind the context to
a dedicated render thread. Any GPU work started from another thread - a
[snapshot](../../ui/component-snapshots.md) taken on the message thread, an
offscreen target released there - must run with the context current.

`GpuDevice::Options::contextActivator` is the hook for that, and
`GpuDevice::runOnGraphicsContext (fn)` routes work through it. On every other
backend `runOnGraphicsContext` just calls `fn`, so writing against it costs
nothing and keeps the GL path correct:

```cpp
device->runOnGraphicsContext ([&]
{
    // create, update or release GPU resources here
});
```

This is the single most common source of GL-only bugs. If something works on
Metal or Direct3D and misbehaves on Linux, look here first.

## Shaders

Desktop GL pipelines take the `glsl` variant of a [shader
bundle](../rhi/offline-shaders.md); GLES and WebGL2 take `essl`. Since the
source language is GLSL, `compileFromGlsl` is the most natural fit here - still
gated on the shader transpiler being built in.

## Related pages

- [Graphics platforms](index.md) - choosing a backend and the capability table.
- [WebGPU](webgpu.md) - the other web backend, with full compute support.
- [Vulkan](vulkan.md) - what Android uses before falling back here.
- [RHI](../rhi/index.md) - the GPU layer this backend implements.
