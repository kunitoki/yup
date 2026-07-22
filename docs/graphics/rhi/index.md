# RHI - GPU Rendering Hardware Interface

The **RHI** is YUP's backend-agnostic, low-level GPU layer. It sits below the 2D
`Graphics` API and above Rive's GPU abstraction, giving you direct control
over pipelines, render passes, buffers, and textures while remaining portable
across Metal, Direct3D, OpenGL and OpenGL ES, WebGL2 and WebGPU, and Vulkan (in progress).

Use the RHI when you need custom GPU work that the 2D `Graphics` API does not
express - 3D geometry, post-process effects, compute-style fullscreen passes, or
offscreen render-to-texture pipelines.

## When to use the RHI

| You want to…                                          | Use                              |
| ----------------------------------------------------- | -------------------------------- |
| Draw 2D vector content (paths, text, images)          | `Graphics` (not the RHI)         |
| Render custom geometry with your own shaders          | `GpuPipeline` + `GpuRenderPass`  |
| Apply a fullscreen post-process effect                | `GpuPipeline` (fullscreen)       |
| Render offscreen and sample the result as a texture   | `GpuTarget` or `GpuCanvas`       |
| Mix 2D drawing *and* custom passes on one surface      | `GpuCanvas`                      |

## Classes at a glance

- **`GpuFrame`** - RAII scope for one frame's GPU work. Begin, encode passes,
  submit.
- **`GpuRenderPass`** - records draw commands (pipeline, bindings, draws) into a
  render target within a frame.
- **`GpuPipeline`** - an immutable, compiled vertex + fragment pipeline plus
  fixed state.
- **`GpuPipelineCache`** - thread-safe compile-or-fetch cache for pipelines.
- **`GpuBuffer`** - an immutable vertex, index, or uniform buffer.
- **`GpuTexture`** - an opaque GPU texture, the currency between passes,
  `Image`, and `Graphics::drawTexture`.
- **`GpuTarget`** - a minimal offscreen render surface for render-pass-only work.
- **`GpuCanvas`** - an offscreen surface that adds 2D `Graphics` drawing on top
  of a target.

## In this area

- [Concepts & lifecycle](concepts.md) - the GPU bridge, GPU capability
  probing, and the frame/pass model.
- [Frames & render passes](frames-and-passes.md) - `GpuFrame`, `GpuRenderPass`,
  and `GpuRenderOptions`.
- [Pipelines & shaders](pipelines.md) - compiling pipelines, pipeline options,
  shader sources, binding maps, and the pipeline cache.
- [Offline shader compilation](offline-shaders.md) - build `.ysl` shader bundles
  ahead of time with `yup_add_shader_bundle` and the `yup_shader_bundler` tool.
- [Buffers & textures](buffers-and-textures.md) - `GpuBuffer` and `GpuTexture`.
- [Offscreen targets & canvases](targets.md) - `GpuTarget`, `GpuCanvas`, and CPU
  readback.
- [Walkthrough: the spinning cube](spinning-cube.md) - an end-to-end custom 3D
  render with a post-process blur.

```{toctree}
:hidden:
:maxdepth: 1

concepts
frames-and-passes
pipelines
offline-shaders
buffers-and-textures
targets
spinning-cube
```
