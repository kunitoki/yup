# Graphics platforms

YUP draws through one of several GPU backends. You rarely have to choose: a
window picks the best API the device supports, and the `Graphics` and
[RHI](../rhi/index.md) APIs look the same on all of them. This section is for
the times when it matters - shipping to a platform with a fallback, requesting
a specific backend, or finding out why a feature is missing on one target.

A backend is named by `yup::GpuPlatform`:

| `GpuPlatform` | Runs on | Shaders |
| --- | --- | --- |
| [`Metal`](metal.md) | macOS, iOS | MSL |
| [`Direct3D`](direct3d.md) | Windows | HLSL |
| [`Vulkan`](vulkan.md) | Android, and optionally Linux, Windows, macOS | SPIR-V |
| [`OpenGL`](opengl.md) | Linux, Windows, macOS | GLSL |
| [`OpenGLES`](opengl.md) | Android, WebGL2 in the browser | ESSL |
| [`WebGPU`](webgpu.md) | Emscripten / WASM | WGSL |
| [`Headless`](headless.md) | Everywhere, draws nothing | none |

## What a window picks

Each platform has an ordered list of APIs, and a window uses the first one the
device supports:

| Target | Order |
| --- | --- |
| macOS, iOS | Metal, or OpenGL when Metal is not compiled in |
| Windows | Direct3D, or OpenGL when Direct3D is not compiled in |
| Linux | OpenGL |
| Android | Vulkan, then OpenGL ES |
| Web (Emscripten) | WebGPU when built with it, otherwise WebGL2 through OpenGL ES |

Android is the only target that routinely falls back, because the device may
have no Vulkan driver. The check runs before the window exists: an Android
window can only ever be rendered by one API, so it cannot change later.

## Asking for a specific backend

Set it on the window's options:

```cpp
auto options = yup::ComponentNative::Options()
                   .withGraphicsApi (yup::GpuPlatform::OpenGLES);

addToDesktop (options);
```

The requested API moves to the front of the list. If the device cannot provide
it, the window falls back to the platform order and logs a line - it never
fails to open. To decide for yourself first, probe it:

```cpp
if (yup::GpuDevice::isPlatformSupported (yup::GpuPlatform::Vulkan, {}))
    options = options.withGraphicsApi (yup::GpuPlatform::Vulkan);
```

`isPlatformSupported` is cheap and needs no window. For every backend but
Vulkan it answers whether the backend was compiled in; for Vulkan it also looks
for a loader, a driver and a physical device good enough for the renderer, and
caches the answer for the process.

Without a window at all - GPU compute, offscreen rendering, image processing -
create the device directly:

```cpp
auto device = yup::GpuDevice::create (yup::GpuPlatform::Metal, {});
```

See [the RHI](../rhi/index.md) for what to do with it.

## What differs between backends

The drawing API does not change. These do, and each is a runtime query on
`GpuDevice` so you can branch on it instead of on the platform:

| Capability | Query | Available on |
| --- | --- | --- |
| Compute shaders | `isComputeAvailable()` | Metal, Direct3D, Vulkan, WebGPU, and OpenGL / OpenGL ES at a high enough version |
| Storage buffers in fragment shaders | `isFragmentStorageBufferAvailable()` | Metal, Direct3D, Vulkan, WebGPU |
| Storage textures written by compute | `isStorageTextureAvailable()` | Metal, Direct3D, Vulkan, WebGPU |
| Dynamic uniform buffer offsets | `isDynamicUniformOffsetAvailable()` | Metal, Vulkan, WebGPU, OpenGL / OpenGL ES / WebGL2, and Direct3D 11.1 |
| GPU frame timings | `isFrameTimingAvailable()` | Metal, and Vulkan where the queue writes timestamps |
| Device loss reporting | `isDeviceLost()` | Vulkan |
| Driver pipeline cache | `getPipelineCacheData()` | Vulkan |

```{note}
The OpenGL family is raster-only for storage resources: fragment shaders cannot
read storage buffers and compute cannot write storage textures, whatever the GL
version. Plan a fragment-shader path for anything that must also run on the web.
```

## How a frame reaches the screen

Every backend renders the window into an offscreen canvas, which a small pass
then copies to the swapchain. That indirection is what lets the same frame be
rotated on Android, written to an sRGB swapchain when the surface offers no
plain UNORM format, and composited with alpha for windows created with
`ComponentNative::Options::withTransparent`.

`GpuDevice::Options::vsync` synchronizes presentation to the display refresh;
it is off by default.

## Shaders

Shaders are written once and compiled per backend. An offline
[shader bundle](../rhi/offline-shaders.md) carries one variant per target -
`msl`, `hlsl`, `glsl`, `essl`, `wgsl`, `spirv` - and the backend picks the one
it needs when the pipeline is compiled. `compileFromGlsl` translates at runtime
instead, which needs the shader transpiler
(`YUP_ENABLE_SHADER_TRANSPILER`); see [GLSL on the WebGPU
target](../rhi/wgsl-shaders.md) for what the translation can and cannot
express.

## In this area

- [Direct3D](direct3d.md) - the default on Windows.
- [Metal](metal.md) - the default on macOS and iOS.
- [OpenGL and OpenGL ES](opengl.md) - the portable fallback, and WebGL2 in the
  browser.
- [Vulkan](vulkan.md) - the default on Android, optional elsewhere.
- [WebGPU](webgpu.md) - the modern web backend.
- [Headless](headless.md) - the do-nothing device, for tests and CI with no
  GPU.

```{toctree}
:hidden:
:maxdepth: 1

direct3d
metal
opengl
vulkan
webgpu
headless
```
