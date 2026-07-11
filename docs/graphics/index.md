# Graphics

The graphics stack renders 2D vector content and GPU-accelerated scenes across
Metal, Direct3D, OpenGL / OpenGL ES, WebGL, and (in progress) Vulkan / WebGPU.
It is built on the open source [Rive](https://rive.app/) renderer.

**Modules covered:** `yup_graphics`, `yup_shading`, `yup_animation`.

## In this area

- [RHI - GPU Rendering Hardware Interface](rhi/index.md) - the backend-agnostic
  low-level GPU layer: frames, render passes, pipelines, buffers, textures, and
  offscreen targets.

## Key building blocks

The `yup_graphics` module provides:

- **`GraphicsContext`** - abstracts the active rendering backend (`Api::Metal`,
  `Api::Direct3D`, `Api::OpenGL`, `Api::OpenGLES`, `Api::WebGPU`, `Api::Headless`),
  exposes DPI scaling, offscreen target creation, and the GPU capability probe
  `isGpuAvailable()`.
- **`Graphics`** - the immediate-mode 2D drawing API (fills, strokes, paths,
  gradients, text, images, textures).
- **Primitives** - points, rectangles, sizes, affine transforms, and colors.
  Geometry primitives use the templated `.to<float>()` conversion, not `toFloat`.
- **Drawables, fonts, SVG, and image formats** for higher-level content.

```{note}
YUP uses American English: it is `Color` (not `Colour`) and `center` (not
`centred`). Fonts are obtained via `ApplicationTheme`, not constructed inline.
```

## Related areas

- [UI](../ui/index.md) - `Component` painting and windowing that drive the
  graphics context.
- [Profiling component paint](../ui/profiling-component-paint.md) - measuring
  paint cost.

```{toctree}
:hidden:
:maxdepth: 2

rhi/index
```
