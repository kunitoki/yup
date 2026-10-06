# Graphics

The graphics stack renders 2D vector content and GPU-accelerated scenes across
Metal, Direct3D, OpenGL / OpenGL ES, WebGL and WebGPU (WASM / Emscripten), and Vulkan.
It is built on the open source [Rive](https://rive.app/) renderer.

**Modules covered:** `yup_rhi`, `yup_graphics`, `yup_shading`, `yup_animation`.

## In this area

- [Primitives](primitives.md) - `Point`, `Rectangle`, `Color`, `ColorGradient`,
  `Path`, `AffineTransform`, `StrokeType`, `BlendMode` - the value types every
  drawing call consumes - plus `Vector3`, `Matrix4`, `Quaternion`, `Ray` and `MeshSurfaceMapper`
  for 3D.
- [The Graphics class](graphics-class.md) - the immediate-mode 2D drawing API:
  state model, fills, strokes, text, images, transforms, clipping, transparency
  layers, and offscreen rendering.
- [How to draw](drawing.md) - a task-oriented guide with copy-paste examples for
  common drawing patterns: backgrounds, buttons, text, paths, gradients, and more.
- [Fonts & text layout](fonts.md) - the `Font` resource, variable-font axes,
  OpenType features, `StyledText`, and themed font access.
- [Drawables & SVG](svg.md) - parsing and painting SVG with `Drawable`, fitting
  modes, and custom image/font resolvers.
- [RHI - GPU Rendering Hardware Interface](rhi/index.md) - the backend-agnostic
  low-level GPU layer: frames, render passes, pipelines, buffers, textures, and
  offscreen targets.

Bitmap image handling - creating, loading, saving, and drawing images - has its
own [Imaging](../imaging/index.md) area.

## Key building blocks

The `yup_graphics` and `yup_rhi` modules provide:

- **`GpuDevice`** (`yup_rhi`) - a reference-counted GPU device abstraction that
  owns the native GPU device and command queue without requiring a window.
  Created via `GpuDevice::create(GpuPlatform, Options)`. Supports
  `GpuPlatform::Metal`, `Direct3D`, `OpenGL`, `OpenGLES`, `WebGPU`, `Vulkan`, and `Headless`.
  Use `GpuDevice` directly for GPU compute (e.g. audio DSP on the GPU) — no window needed.
- **`GraphicsContext`** (`yup_graphics`) - wraps a `GpuDevice` and adds the
  window/swapchain layer plus Rive vector rendering. Created via
  `GraphicsContext::createContext(GpuPlatform, Options, GpuDevice::Ptr = {})`.
  When an existing `GpuDevice::Ptr` is provided, it shares the GPU device
  (useful when an audio processor already owns one).
- **`Graphics`** - the immediate-mode 2D drawing API (fills, strokes, paths,
  gradients, text, images, textures).
- **Primitives** - points, rectangles, sizes, affine transforms, and colors.
  Geometry primitives use the templated `.to<float>()` conversion, not `toFloat`.
- **Drawables, fonts, SVG, and image formats** for higher-level content.

```{note}
YUP uses American English: it is `Color` (not `Colour`) and `center` (not `centre`).
```

## Vulkan

Vulkan is the default graphics API on Android, with OpenGL ES as the fallback.
On the other platforms it can be built in and requested explicitly.

**Requirements.** Vulkan 1.1 (1.3 on PowerVR GPUs), and Android 10 on Android.
The Vulkan loader is never linked: SDL resolves it when a window is created, so
a device without a Vulkan driver keeps working and simply uses the fallback.

**Fallback.** Each platform has an ordered list of graphics APIs, and a window
uses the first one the device supports. On Android that is Vulkan, then OpenGL
ES. The check runs before the window exists, because an Android window can only
ever be rendered by one API.

**Forcing a backend.** Request one for a window with
`ComponentNative::Options::withGraphicsApi`:

```cpp
auto options = yup::ComponentNative::Options()
                   .withGraphicsApi (yup::GpuPlatform::OpenGLES);
```

A requested API the device can't provide falls back to the platform list, and a
line is logged. `GpuDevice::isPlatformSupported` runs the same check.

**Building it.** The `YUP_ENABLE_VULKAN` CMake option compiles the backend. It
defaults to ON for Android and OFF elsewhere; pass `-DYUP_ENABLE_VULKAN=ON` to
build it on Linux, Windows or macOS (macOS also needs MoltenVK). With
`just ninja OFF ON` the host build uses it. For Android debug builds,
`-DYUP_ANDROID_VALIDATION_LAYERS=ON` packages the Khronos validation layer
(`YUP_ANDROID_VALIDATION_LAYERS_PATH` points at the unzipped Android release of
Vulkan-ValidationLayers), and its messages show up in logcat.

## Related areas

- [UI](../ui/index.md) - `Component` painting and windowing that drive the
  graphics context.
- [Component paint profiling](../ui/component-profiling.md) - measuring
  paint cost.

```{toctree}
:hidden:
:maxdepth: 2

primitives
graphics-class
drawing
fonts
svg
rhi/index
```
