# Direct3D

Direct3D 11 is the default backend on Windows.

| | |
| --- | --- |
| `GpuPlatform` | `Direct3D` |
| Runs on | Windows |
| Default | Yes |
| Shader variant | `hlsl` |
| Compute | Yes |
| Build flag | `YUP_RIVE_USE_D3D`, on by default |

## Building

The Direct3D backend is compiled into every Windows build. Define
`YUP_RIVE_USE_D3D=0` to leave it out, and Windows windows fall back to
[OpenGL](opengl.md).

## What you get

- **Compute.** `GpuComputePipeline` and `GpuComputePass` are supported,
  including storage textures. Compute resources must be declared in group
  (set) 0, as on Metal.
- **Fragment storage buffers** are available, so a fragment shader can read
  what a compute pass wrote.
- **Dynamic uniform offsets** need the Direct3D 11.1 runtime, with constant
  buffer offsetting and no-overwrite maps of dynamic constant buffers. Query
  `GpuDevice::isDynamicUniformOffsetAvailable()` rather than assuming;
  pipelines simply ignore `GpuPipelineOptions::dynamicUniformBuffers` when it
  is false.

A compute pass begun on a frame records into the immediate context the frame
draws with, so its dispatches run in order with the frame's render passes.

## Shaders

Pipelines take the `hlsl` variant of a [shader
bundle](../rhi/offline-shaders.md). `compileFromGlsl` works when the shader
transpiler is built in.

## Things to know

- There is no frame timing: `isFrameTimingAvailable()` is false, and frames
  report their timings unavailable. Profile on Metal or Vulkan.
- There is no device-loss reporting; `isDeviceLost()` is always false.
- `getPipelineCacheData()` returns nothing.
- Direct3D has no current-context affinity, so `Options::contextActivator` is
  ignored and offscreen GPU work can be started from any thread.
- Windows can refuse a swapchain for a while, as it does in a session that is
  not interactive (a process started over SSH). The window logs the error,
  skips its frames and retries, so it starts drawing once a swapchain can be
  created.

## When to prefer Vulkan on Windows

[Vulkan](vulkan.md) can be built on Windows with `-DYUP_ENABLE_VULKAN=ON` and
requested explicitly. It adds device-loss reporting, a persistent driver
pipeline cache and frame timings. It is not the default, so only take it if you
are prepared to test the driver matrix that comes with it.

## Related pages

- [Graphics platforms](index.md) - choosing a backend and the capability table.
- [RHI](../rhi/index.md) - the GPU layer this backend implements.
- [Offline shader compilation](../rhi/offline-shaders.md) - producing the
  `hlsl` variant.
