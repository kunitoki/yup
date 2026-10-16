# RHI - GPU Rendering Hardware Interface

The **RHI** is YUP's backend-agnostic, low-level GPU layer, provided by the
`yup_rhi` module. It sits below the 2D `Graphics` API and above Rive's GPU
abstraction, giving you direct control over pipelines, render passes, buffers,
and textures while remaining portable across Metal, Direct3D, OpenGL / OpenGL ES,
WebGL2, WebGPU, and Vulkan.

Use the RHI when you need custom GPU work that the 2D `Graphics` API does not
express - 3D geometry, post-process effects, compute passes for DSP or simulation,
or offscreen render-to-texture pipelines. For GPU compute without any window or
graphics (e.g. audio DSP on the GPU), use `GpuDevice` directly — no
`GraphicsContext` or `yup_graphics` dependency needed.

## When to use the RHI

| You want to…                                          | Use                              |
| ----------------------------------------------------- | -------------------------------- |
| Draw 2D vector content (paths, text, images)          | `Graphics` (not the RHI)         |
| Render custom geometry with your own shaders          | `GpuPipeline` + `GpuRenderPass`  |
| Apply a fullscreen post-process effect                | `GpuPipeline` (fullscreen)       |
| Run GPU compute (DSP, simulation)                     | `GpuComputePipeline` + `GpuComputePass` |
| Render offscreen and sample the result as a texture   | `GpuTarget` or `GpuCanvas`       |
| Mix 2D drawing *and* custom passes on one surface     | `GpuCanvas`                      |

## Classes at a glance

- **`GpuDevice`** - a reference-counted GPU device abstraction. Owns the native
  device and command queue. Factories for all RHI resources start here.
- **`GpuFrame`** - RAII scope for one frame's GPU work. Begin, encode passes,
  submit.
- **`GpuRenderPass`** - records draw commands (pipeline, bindings, draws) into a
  render target within a frame.
- **`GpuComputePass`** - records compute dispatch commands (pipeline, storage
  buffers, uniforms) directly against the backend-native API.
- **`GpuPipeline`** - an immutable, compiled vertex + fragment pipeline plus
  fixed state.
- **`GpuComputePipeline`** - an immutable, compiled compute pipeline (single
  compute stage, native backend API, no ore dependency).
- **`GpuPipelineCache`** - thread-safe compile-or-fetch cache for pipelines.
- **`GpuBuffer`** - an immutable vertex, index, uniform, or storage buffer.
- **`GpuTexture`** - an opaque GPU texture, the currency between passes,
  `Image`, and `Graphics::drawTexture`.
- **`GpuTarget`** - a minimal offscreen render surface for render-pass-only work.
- **`GpuCanvas`** - an offscreen surface that adds 2D `Graphics` drawing on top
  of a target.

## Vulkan

The Vulkan backend (`GpuPlatform::Vulkan`) runs the whole RHI: render pipelines
and passes through Rive's ore layer, and native compute pipelines and passes.
Things to know when targeting it:

- **Shaders are SPIR-V.** `compileFromBundle` picks the bundle's `spirv`
  variant, see [the `spirv` target](offline-shaders.md#the-spirv-target), and
  `compileFromGlsl` compiles straight to SPIR-V. Compute pipelines need the shader
  binding map to build their layout, which both of these provide. A hand-made
  `GpuShaderSource` without a `bindingMap` is reflected when the pipeline is
  compiled, which needs the shader transpiler (`YUP_ENABLE_SHADER_TRANSPILER`).
- **One queue.** Rive, `GpuFrame`, offscreen readbacks and compute passes all
  submit to a single queue, so work runs in the order it was submitted. Like on
  Metal, a `GpuComputePass` submits its own command buffer when it finishes. A
  pass begun with `GpuComputePass::begin (frame)` records into the frame's
  command buffer instead and is submitted with it, as on Metal, so render passes
  recorded after it in the same frame use its results.
- **One frame counter.** Every submission takes a frame generation from the
  device, and resources are released once the GPU is known to have finished with
  them, so any number of `GpuFrame`s may be open at once.
- **Storage buffers are host visible.** `GpuDevice::readBuffer` and
  `updateBuffer` wait only for the last submission that used the buffer, then
  map it directly.
- **Device loss.** When the driver reports the device lost (a GPU reset or a
  driver crash), `GpuDevice::isDeviceLost` turns true and the device stops
  submitting work instead of hanging. Create a new device to render again.

## Fragment shader examples

Select "Curl Bands" in the graphics example, or launch it with `--demo CurlBands`.
For a single-demo build, select `YUP_EXAMPLE_GRAPHICS_DEMO=CurlBands`.

The shader is by Abhishek Shivakumar (@godofecht). It uses two drifting FBM
fields to displace coordinates with curl noise, then maps density and flow
to color bands. The demo supplies elapsed time and target dimensions through
a uniform buffer and uses the shared fullscreen vertex shader.

Source: `examples/graphics/data/shaders/curl_bands.frag`.

"Ribbon Clouds", also by Abhishek Shivakumar (@godofecht), accumulates 56 layers
of 3D FBM ribbons, clouds, and shells with exponential zoom. Open it with
`--demo RibbonClouds`, or select `YUP_EXAMPLE_GRAPHICS_DEMO=RibbonClouds` when
building. Both examples share the same fullscreen renderer.

Source: `examples/graphics/data/shaders/ribbon_clouds.frag`.
The vignette is clamped to zero so wide windows do not produce negative
inputs to gamma correction. The original exponential zoom is retained;
reopen the demo to restart it when large coordinates lose noise precision.

## In this area

- [Concepts & lifecycle](concepts.md) - the GPU bridge, GPU capability
  probing, and the frame/pass model.
- [Frames & render passes](frames-and-passes.md) - `GpuFrame`, `GpuRenderPass`,
  and `GpuRenderOptions`.
- [Pipelines & shaders](pipelines.md) - compiling pipelines, pipeline options,
  shader sources, binding maps, and the pipeline cache.
- [Offline shader compilation](offline-shaders.md) - build `.ysl` shader bundles
  ahead of time with `yup_add_shader_bundle` and the `yup_shader_bundler` tool.
- [Shader bundle binary format](shader-bundle-format.md) - the RIFF `YSLB` container
  specification (FourCCs, chunk layout, reflection blob, versioning).
- [GLSL on the WebGPU target](wgsl-shaders.md) - what the GLSL→WGSL transpiler
  translates, how it assigns bindings, and what WGSL can't express.
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
shader-bundle-format
wgsl-shaders
buffers-and-textures
targets
spinning-cube
```
