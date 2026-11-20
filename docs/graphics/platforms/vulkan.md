# Vulkan

Vulkan is the default backend on Android, with OpenGL ES as the fallback. On
desktop it can be built in and requested explicitly - it is the most capable
backend YUP has, and the only one that reports device loss and persists a
driver pipeline cache.

| | |
| --- | --- |
| `GpuPlatform` | `Vulkan` |
| Runs on | Android; optionally Linux, Windows, macOS (through MoltenVK) |
| Default | Yes on Android, no elsewhere |
| Shader variant | `spirv` |
| Compute | Yes |
| Build flag | `YUP_ENABLE_VULKAN`, on by default for Android, off elsewhere |

## Requirements

Vulkan 1.1, or 1.3 on PowerVR GPUs, and Android 10 on Android.

The Vulkan loader is never linked. SDL resolves it when a window is created, so
a device with no Vulkan driver keeps working and simply uses the fallback.
`GpuDevice::isPlatformSupported (GpuPlatform::Vulkan, options)` runs the same
check - loader, driver, and a physical device that meets the renderer's
requirements - and caches the answer for the process.

## Building

```sh
cmake -B build -DYUP_ENABLE_VULKAN=ON
```

`YUP_ENABLE_VULKAN` defaults to ON for Android and OFF everywhere else. Turn it
on to build the backend on Linux, Windows or macOS; macOS additionally needs
MoltenVK. With the repository's `just ninja OFF ON` the host build uses it.

For Android debug builds, `-DYUP_ANDROID_VALIDATION_LAYERS=ON` packages the
Khronos validation layer and its messages show up in logcat.
`YUP_ANDROID_VALIDATION_LAYERS_PATH` points at the unzipped Android release of
Vulkan-ValidationLayers.

## Choosing it at runtime

On Android nothing is needed: a window tries Vulkan first and falls back to
OpenGL ES when the device cannot provide it. The check runs before the window
exists, because an Android window can only ever be rendered by one API.

Elsewhere, request it:

```cpp
auto options = yup::ComponentNative::Options()
                   .withGraphicsApi (yup::GpuPlatform::Vulkan);
```

A requested API the device can't provide falls back to the platform list, and a
line is logged.

## What you get

Vulkan runs the whole RHI: render pipelines and passes through Rive's ore
layer, and native compute pipelines and passes.

- **Compute**, including storage textures written by compute and read later.
  A pass begun with `GpuComputePass::begin (frame)` records into the frame's
  command buffer and is submitted with it, so render passes recorded after it
  in the same frame use its results.
- **Fragment storage buffers** and **dynamic uniform offsets**.
- **Frame timings**, on queues that write timestamps:
  `GpuFrame::setTimingEnabled()` and `GpuFrame::beginScope()` return real GPU
  times.
- **A driver pipeline cache.** `GpuDevice::getPipelineCacheData()` returns every
  pipeline the device compiled so far - call it late, typically on shutdown -
  and `Options::pipelineCacheData` passes it back on the next launch. This is
  worth doing: it is the difference between a cold first frame and a warm one.
- **Device-loss reporting**, see below.

## Shaders

Pipelines take the `spirv` variant of a [shader
bundle](../rhi/offline-shaders.md), and `compileFromGlsl` compiles straight to
SPIR-V.

Compute pipelines need the shader binding map to build their layout, which both
of those provide. A hand-made `GpuShaderSource` without a `bindingMap` is
reflected when the pipeline is compiled, which needs the shader transpiler
(`YUP_ENABLE_SHADER_TRANSPILER`).

## One queue, one frame counter

Rive, `GpuFrame`, offscreen readbacks and compute passes all submit to a single
queue, so work runs in the order it was submitted.

Every submission takes a frame generation from the device, and resources are
released once the GPU is known to have finished with them, so any number of
`GpuFrame`s may be open at once.

Storage buffers are host visible: `GpuDevice::readBuffer` and `updateBuffer`
wait only for the last submission that used the buffer, then map it directly.

## Presenting

A window renders into an offscreen canvas that a small pass copies to the
swapchain. That pass also:

- rotates the image on Android, so the compositor never has to when the device
  is turned (the swapchain matches the display orientation),
- presents correctly to sRGB swapchains, when the surface offers no plain UNORM
  format, and
- keeps the alpha for windows created with
  `ComponentNative::transparentWindow`, when the surface supports compositing
  with alpha.

## Device loss

Vulkan is the only backend that reports it. After a GPU reset, a hang or a
driver crash, `GpuDevice::isDeviceLost()` turns true and stays true: the
device's frames, offscreen renders and compute passes do nothing and its
windows stop presenting, rather than hanging on the GPU. The application keeps
running. Create a new device to render again.

```cpp
if (device->isDeviceLost())
{
    // tear down GPU resources and create a new GpuDevice
}
```

## Related pages

- [Graphics platforms](index.md) - choosing a backend and the capability table.
- [RHI](../rhi/index.md) - the GPU layer this backend implements.
- [Offline shader compilation](../rhi/offline-shaders.md) - producing the
  `spirv` variant.
- [OpenGL and OpenGL ES](opengl.md) - the Android fallback.
