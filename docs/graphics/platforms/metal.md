# Metal

Metal is the default backend on macOS and iOS, and the one to target for any
Apple release. It supports everything the [RHI](../rhi/index.md) exposes.

| | |
| --- | --- |
| `GpuPlatform` | `Metal` |
| Runs on | macOS, iOS |
| Default | Yes, on both |
| Shader variant | `msl` |
| Compute | Yes |
| Build flag | `YUP_RIVE_USE_METAL`, on by default |

## Building

Nothing to configure: the Metal backend is compiled into every Apple build.
Define `YUP_RIVE_USE_METAL=0` to leave it out, in which case Apple windows fall
back to [OpenGL](opengl.md) - useful only when you are deliberately testing
that path.

## What you get

Metal is the most complete backend after Vulkan:

- **Compute.** `GpuComputePipeline` and `GpuComputePass` are fully supported,
  including storage textures written by compute and read by later passes.
  Compute resources must be declared in group (set) 0.
- **Frame timings.** `GpuDevice::isFrameTimingAvailable()` is true, so
  `GpuFrame::setTimingEnabled()` returns real GPU times, and
  `GpuFrame::beginScope()` measures inside a frame. This is the backend to
  profile on; see [component paint
  profiling](../../ui/component-profiling.md).
- **Dynamic uniform offsets.** `GpuPipelineOptions::dynamicUniformBuffers`
  works, so one uniform buffer can feed many draws at different offsets.
- **Fragment storage buffers** and all the other storage paths.

A compute pass begun with `GpuComputePass::begin (frame)` is encoded into the
frame's command buffer and runs in order with the frame's render passes: passes
begun before it read nothing it writes, passes begun after see all of it.

## Shaders

Pipelines take the `msl` variant of a [shader
bundle](../rhi/offline-shaders.md). `compileFromGlsl` also works when the
shader transpiler is built in, which is convenient while prototyping but adds
translation to startup.

## Things to know

- Metal is thread-safe in the way the RHI needs: unlike OpenGL there is no
  current-context affinity, so `Options::contextActivator` is ignored and
  offscreen work can be started from any thread.
- There is no device-loss reporting; `isDeviceLost()` is always false.
- `getPipelineCacheData()` returns nothing - Metal has no driver pipeline cache
  to persist.
- On iOS, Metal is the only usable backend. Vulkan is explicitly not supported
  there, and the GL path is a desktop fallback.

## Related pages

- [Graphics platforms](index.md) - choosing a backend and the capability table.
- [RHI](../rhi/index.md) - the GPU layer this backend implements.
- [Offline shader compilation](../rhi/offline-shaders.md) - producing the `msl`
  variant.
