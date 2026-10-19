# Render settings

`RenderSettings` chooses how a scene is rendered. Set it on the renderer, then
repaint:

```cpp
yup::RenderSettings settings;
settings.bloom = true;
settings.bloomIntensity = 0.1f;
settings.taaSamples = 8;

view.getRenderer().setRenderSettings (settings);
scene->getRoot().attach<yup::ToneMappingNode> (yup::ToneMappingOperator::agx);
scene->getRoot().attach<yup::ExposureNode> (1.2f);
```

| Setting          | Default     | What it does                                                                                   |
| ---------------- | ----------- | ---------------------------------------------------------------------------------------------- |
| `tier`           | `automatic` | `automatic`, `raster` or `full`, see [Platform tiers](platform-tiers.md).                        |
| `taaSamples`     | 16          | Full tier: how many jittered frames are accumulated before the image is final. Fewer converge sooner and antialias less. |
| `upscaling`      | `off`       | Full tier: `temporal` shades the scene smaller and rebuilds the full image over several frames, see [Upscaling](platform-tiers.md#upscaling). |
| `renderScale`    | 0.5         | With `upscaling`: the share of the width and height the scene is shaded at, from 0.5 to 1. |
| `bloom`          | off         | Glow around bright highlights, see [Post effects](post-effects.md).                              |
| `bloomThreshold` | 1.0         | Exposed radiance above which pixels glow.                                                      |
| `bloomIntensity` | 0.05        | How much of the blurred glow is added to the image.                                            |
| `bloomRadius`    | 0.85        | From 0 to 1: how far the glow spreads.                                                         |
| `ambientOcclusion` | unset     | Darkens the indirect light in creases, see [Ambient occlusion](indirect-lighting.md#ambient-occlusion). Unset follows the tier: on for full, off for raster. |
| `aoDistance`     | 0.2         | How far, in scene units, surfaces occlude each other for ambient occlusion. 0 or less turns it off. |
| `screenSpaceReflections` | unset | Smooth surfaces reflect what the camera sees, see [Screen-space reflections](indirect-lighting.md#screen-space-reflections). Unset follows the tier: on for full; raster never has them. Needs `taaSamples` above 1. |
| `screenSpaceGI` | unset | Nearby surfaces light each other, see [Screen-space diffuse GI](indirect-lighting.md#screen-space-diffuse-gi). Unset is off on every tier; raster never has it. Needs `ambientOcclusion` and `taaSamples` above 1. |
| `depthOfField` | off | Blurs what lies away from the camera's focus, see [Depth of field](post-effects.md#depth-of-field). |
| `depthOfFieldMaxSize` | 32 | The largest depth of field blur, as a diameter in pixels of the image, at most 64. |
| `motionBlur` | off | Full tier: smears what moves on screen, see [Motion blur](post-effects.md#motion-blur). |
| `motionBlurShutter` | 0.5 | The fraction of the frame time the shutter stays open, from 0 to 1. 0 or less blurs nothing. |
| `debugView` | `none` | Full tier: shows an intermediate image instead of the final one, see below. |

**Debug view.** `debugView` shows what one effect computes, in place of the
final image, to tune it:

- `ambientOcclusion`: the ambient occlusion as gray, white where nothing is
  occluded;
- `reflections`: the screen-space reflections, exposed and weighted by how
  confident each one is;
- `indirectDiffuse`: the light the screen-space diffuse GI gathers, exposed,
  before the color of the surfaces is applied;
- `circleOfConfusion`: how much the depth of field blurs each pixel, brighter as
  the blur nears `depthOfFieldMaxSize` (the square root of its share, so blurs of
  a pixel or two show): orange in front of the focus, blue behind it, black in
  focus. It needs `depthOfField` on.

The image is shown pixel for pixel, without tone mapping, bloom or background,
so the half-resolution effects show their 2 x 2 blocks. It shows the effect of a
single frame, before temporal anti-aliasing averages it: the ambient occlusion
and the reflections look grainy, and stay so once the image has converged. An
effect that is off, unavailable, or has no history to read yet shows black. The
raster tier ignores `debugView`. It changes nothing before the accumulation, so
switching it neither restarts nor discards it, and `none` costs nothing.

Settings a device or tier cannot provide are ignored instead of failing. Ask
`getRenderer().getCapabilities()` what is in effect after a frame was rendered:
it returns `nullptr` before the first one.

```cpp
if (auto* caps = view.getRenderer().getCapabilities())
{
    auto tier = caps->getTier(); // RenderCapabilities::Tier::raster or ::full
    bool glows = caps->isEnabled (yup::RenderCapabilities::Feature::bloom, view.getRenderer().getRenderSettings());
}
```

Related [scene features](showing-a-scene.md#scene-features):

- **Exposure** (`ExposureNode`) multiplies the light before tone mapping.
- **Tone mapping** (`ToneMappingNode`) maps the exposed light into the
  displayable range: `reinhard` (the default) rolls highlights off softly,
  `aces` adds contrast and saturation, and `agx` is a filmic curve that keeps
  saturated highlights from skewing. AgX is YUP's own fit of the look, opt-in
  like ACES.
- **Background color** (`BackgroundNode`) is display-referred on both
  tiers: it is never tone mapped nor affected by exposure, and shows exactly as
  set.

## Frame statistics

`SceneRenderer::setFrameStatsEnabled (true)` makes the renderer measure what its
frames cost on the GPU and what they drew. `getLastFrameStats()` returns the
`FrameStats` of the latest frame the GPU finished, a few frames behind the last
render, without waiting for it:

```cpp
auto& renderer = view.getRenderer();
renderer.setFrameStatsEnabled (true);

// Later, for example from a timer
const auto& stats = renderer.getLastFrameStats();
if (stats.gpu.available)
    YUP_DBG ("GPU " << stats.gpu.frameMilliseconds << " ms at " << stats.internalWidth << "x" << stats.internalHeight);

for (const auto& scope : stats.gpu.scopes)
    if (scope.available)
        YUP_DBG (scope.name << ": " << scope.milliseconds << " ms");
```

- It is off by default, and frames pay nothing for it while off, until a frame
  of the device has measured: the device's later frames then pay a GPU
  timestamp pair each when they complete.
- GPU times are measured on Metal only for now. Elsewhere `gpu.available` is
  false, and the draw counts (`drawItems`, `shadowCasterDraws`,
  `shadowsRedrawn`) and the size are still reported.
- `gpu.scopes` lists the passes the frame recorded, in order: `probeBake`,
  `shadows`, `lightCulling`, `gbuffer`, `ambientOcclusion`, `hiZ`,
  `screenSpaceReflections`, `lighting`, `indirectDiffuse`,
  `depthNormalPrepass`, `ambientOcclusion` (raster), `forward`, `fog`,
  `temporalAA`, `depthOfField`, `motionBlur`, `bloom`, `toneMapping`,
  `debugView`. A pass
  the frame does not run is not listed, and one that fails reads 0. `shadows`
  is listed in every frame, and reads 0 when the frame kept the previous
  shadow maps.
- With screen-space GI on, `ambientOcclusion` includes the GI gather, which
  traces in the same draws, and `indirectDiffuse` is only adding the gathered
  light to the scene.
- A pass's time is how much it lengthened the frame on the GPU, not what it
  costs on its own. The GPU overlaps passes, so each one is credited only the
  time past the end of the work before it: a pass hidden under an earlier one
  reads 0, and the passes need not add up to the frame total. To learn what a
  pass costs on its own, compare the frame total with and without it.
- The first pass that recorded work (usually `lightCulling`, or `shadows` when
  the shadow maps were redrawn) may also include time it overlapped work outside
  the renderer's frames, such as window drawing and uploads.
- The pass times stay meaningful with up to 8 frames in flight. The frame total
  includes the time the frame waited behind earlier ones, even with 2 in flight,
  so wait for each measured frame to read its own cost (`just perf` does).
- A window repainting a `SceneComponent` keeps about 3 frames in flight (the
  drawables of a Metal window), so the demo's own readout reads higher than
  `just perf`.
- The stats move on only when the renderer renders (or `waitForGPU()` or
  `bakeProbes()` runs): a view that stopped repainting keeps its last ones.
- With profiling enabled, the CPU side of the same passes shows in the traces
  under the same names.
- The Scene 3D demo shows them in its Render settings panel.
