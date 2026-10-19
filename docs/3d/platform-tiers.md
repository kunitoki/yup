# Platform tiers

The renderer has two tiers, picked per device from the `tier` preference:

- **Raster** draws the scene straight to the target with forward shading and
  MSAA. It runs everywhere, and its output is the baseline the full tier is
  compared against.
- **Full** renders to an HDR (half-float) scene color, antialiases it with
  temporal anti-aliasing (TAA), applies post effects and tone maps last.

| Device                                  | `automatic`                     | `full` requested               |
| --------------------------------------- | ------------------------------- | ------------------------------ |
| Metal on desktop                        | Full                            | Full                           |
| Metal on iOS                            | Raster                          | Full                           |
| Vulkan on desktop                       | Full (experimental)             | Full (experimental)            |
| Vulkan on Android                       | Raster                          | Full (experimental)            |
| WebGPU on desktop browsers              | Full (experimental)             | Full (experimental)            |
| WebGPU on mobile browsers               | Raster                          | Full (experimental)            |
| Direct3D 11                             | Full (experimental)             | Full (experimental)            |
| OpenGL, OpenGL ES, WebGL2               | Raster                          | Raster                         |

`automatic` picks the full tier on desktop GPUs that pass the capability checks
(half-float render targets, enough color attachments, compute shaders with storage
buffers readable from fragment shaders, and a verified backend).
Today only Metal is verified. Vulkan, WebGPU and Direct3D 11 run the full tier too,
as an experiment while they are being validated (browsers in WebGPU compatibility mode
lack fragment storage buffers and stay on raster). OpenGL and OpenGL ES never run the full
tier. Mobile, Android and iOS browsers included, defaults to raster. Asking for `full` on a device that cannot run it
falls back to raster.

| Feature                           | Raster                                                                                     | Full                         |
| --------------------------------- | ------------------------------------------------------------------------------------------ | ---------------------------- |
| [Depth of field](post-effects.md#depth-of-field) | Off by default, where the device has multisampled half-float targets and the depth and normal pass | Off by default |
| [Motion blur](post-effects.md#motion-blur)       | No: the raster tier has no motion to read                                                  | Off by default               |
| [Fog](post-effects.md#fog)                       | When the scene has it, where the device has multisampled half-float targets and the depth and normal pass | When the scene has it |
| [Upscaling](#upscaling)           | No: it rebuilds the image with the temporal anti-aliasing of the full tier                 | Off by default               |

Volumetric fog and fog volumes, subsurface scattering and refraction are not in
this release, on either tier.

## Temporal anti-aliasing

On the full tier a still image is not final after the first frame. Each frame is
rendered with a slightly different sub-pixel offset and blended into a history,
so after `taaSamples` frames the edges are smooth and the image stops changing.
While the scene is still, each of those frames counts the same in the final
image, so raising `taaSamples` gives smoother edges and less grain in soft
shadows, at the cost of more frames before the image is final.

- `SceneComponent` keeps repainting by itself until the image has converged, then
  goes idle. `getRenderer().isConverged()` tells whether it has; it is always
  true on the raster tier.
- Most changes keep the history and only restart the convergence, so animated
  scenes stay smooth: moving the camera or objects, changing the content of a
  `Texture`, a material, a light, the ambient light, the environment, the world
  fog or the exposure. Once the scene is still again, the history counts as a
  single frame among the `taaSamples` that follow, so edges keep only a faint
  trace of the image before the change. After a large change, such as a cut to another
  shot, call `restartAccumulation()`.
- Changing the size of the view, the tier, `taaSamples`, `upscaling` or
  `renderScale` discards the history, and the image starts over.
- Tone mapping, the bloom, depth of field and motion blur settings, the depth
  of field fields of the camera and the debug view are applied after the
  accumulation, so changing them neither restarts nor discards it. Repaint
  after changing them: the component does not repaint by itself for them.
- When you change something the renderer cannot see, such as vertices edited in
  place in a `Mesh`, or a `GpuTexture` redrawn without calling
  `Texture::setGpuTexture()`, call `getRenderer().restartAccumulation()`. It
  discards the history, and the image starts over.
- Calling `Texture::setGpuTexture()` on every frame stops the image from ever
  converging, and the component then repaints forever. Call it only when the
  content changed.

## Upscaling

Most of a full tier frame is spent shading pixels, so shading fewer of them is
the largest saving. With `upscaling = Upscaling::temporal`, the scene is shaded
at `renderScale` of the view's width and height, and the temporal
anti-aliasing rebuilds the full-size image from the jittered frames: each frame
samples different points of every pixel, and the history gathers them.

```cpp
auto settings = view.getRenderer().getRenderSettings();
settings.upscaling = yup::RenderSettings::Upscaling::temporal;
settings.renderScale = 0.5f; // a quarter of the pixels
view.getRenderer().setRenderSettings (settings);
```

- A still image converges to one close to the full-size image, over about
  `taaSamples / (renderScale * renderScale)` frames: 64 at a scale of 0.5 with
  the default 16 samples.
- While things move, the image is softer than at full size, since each frame
  holds fewer samples.
- Textures are sampled at the mip level of the smaller size, so their fine detail
  stays softer at low scales.
- Effects after the accumulation (bloom, depth of field, motion blur, tone
  mapping) run at the full size.
- It needs the full tier and `taaSamples` above 1; otherwise frames are shaded at
  full size. `renderScale` is clamped to [0.5, 1], and a scale of 1 renders
  exactly as `off`. `FrameStats::internalWidth` and `internalHeight` report the
  size shaded.

GPU time of the Scene 3D demo (full tier, light show on, orbiting camera) on an
Apple M2 Max:

| View size            | Off     | `renderScale` 0.75 | `renderScale` 0.5 |
| -------------------- | ------- | ------------------ | ----------------- |
| 1578 x 1404 (demo)   | 6.3 ms  | 4.3 ms             | 3.2 ms            |
| 3456 x 2234 (Retina) | 22.0 ms | 13.9 ms            | 7.7 ms            |
