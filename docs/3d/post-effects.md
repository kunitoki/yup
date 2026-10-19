# Post effects and fog

**Bloom** adds a soft glow around highlights brighter than `bloomThreshold`. It
works on both tiers:

- On the full tier it is always available.
- On the raster tier it needs multisampled half-float render targets at the
  current sample count (`setSampleCount`). When the device lacks them, bloom is
  unavailable and the image is unchanged; the capabilities report it through
  `isAvailable (RenderCapabilities::Feature::bloom)`.
- Turning bloom on in the raster tier makes the scene render in half float and
  tone map after antialiasing. Silhouettes and transparent surfaces over the
  background still look as they do without bloom. Antialiased edges between
  surfaces of very different brightness blend differently: a bright edge over a
  dark surface looks bolder than without bloom.
- The glow strength depends somewhat on the resolution of the view: the same
  intensity looks a little different at different sizes.

## Depth of field

**Depth of field** blurs what lies in front of or behind the distance the camera
focuses at, like a photographic lens. Turn it on with `depthOfField`, and shape
it with the fields of the active `CameraNode`:

```cpp
auto* camera = scene->getActiveCamera();
camera->fStop = 1.8f;
camera->focusDistance = 2.5f;

auto settings = view.getRenderer().getRenderSettings();
settings.depthOfField = true;
view.getRenderer().setRenderSettings (settings);
view.repaint();
```

| Field                                 | Default       | Unit                  | What it does                                                    |
| ------------------------------------- | ------------- | --------------------- | --------------------------------------------------------------- |
| `RenderSettings::depthOfField`        | off           |                       | Draws the depth of field.                                       |
| `RenderSettings::depthOfFieldMaxSize` | 32            | pixels of the image   | The largest blur, as a diameter, at most 64. 0 or less blurs nothing. |
| `CameraNode::fStop`                   | 2.8           | f-number              | The aperture: smaller blurs more. 0 or less turns the blur off. |
| `CameraNode::focusDistance`           | 10            | meters                | The distance along the view that stays perfectly sharp.         |
| `CameraNode::apertureBlades`          | 0             | count                 | The shape of the out-of-focus highlights: round below 3, otherwise a polygon with that many sides. |
| `CameraNode::sensorHeight`            | 0.024 (24 mm) | meters (constant)     | With `yFov`, it sets the focal length of the lens.              |

World units are read as meters.

- It works on both tiers. On the raster tier it needs multisampled half-float
  render targets at the current sample count, as bloom does, and the depth and
  normal pass of ambient occlusion, which gives the blur its depth. Where the
  device lacks them, the setting is ignored, which
  `isAvailable (RenderCapabilities::Feature::depthOfField)` reports, and it is
  logged once in debug builds.
- On the raster tier, the frames it blurs render the scene in half float, as
  bloom does, and draw the depth and normal pass, as ambient occlusion does.
  Frames it does not blur skip both, so moving `fStop` to 0 or back switches
  the frame's route just like turning the setting off or on. The depth comes
  from one sample per pixel while the color is antialiased, so silhouettes
  against a far blurred background can differ from the sharp image by about a
  pixel. As with bloom, antialiased edges between surfaces of very different
  brightness blend differently in half float: a bright edge over a dark
  surface looks bolder.
- It is off by default and costs nothing while off. A camera that cannot blur
  (orthographic, `fStop` or `yFov` of 0 or less, or no active camera) costs
  nothing either, and neither does a `depthOfFieldMaxSize` of 0 or less.
- It runs after temporal anti-aliasing and before bloom, so changing it or the
  camera's depth of field fields keeps the accumulation: repaint after such a
  change.
- `RenderSettings::debugView` set to `circleOfConfusion` shows how much each
  pixel blurs, see [Render settings](render-settings.md).
- The blur carries the coverage of the scene, so silhouettes over the background
  color fade into it. What the blur spreads is composited over the background
  color in light rather than in display values: a bright highlight blurred over
  the background keeps the brightness it has over a surface or a visible
  environment, and the edges of blurred objects over the background look as
  they do over a dark environment. Pixels the blur leaves sharp, and frames
  without depth of field, keep the usual blend, where antialiased edges over
  the background match the raster tier. So do partly covered areas the blur
  leaves as they were, such as fog over the background color; highlights
  blurred over such a fogged background color therefore look dimmer than over
  a clear one, about half as bright at a fog transmittance of 0.8, while a
  visible environment is unaffected. Over a
  transparent background color, blurred frames output premultiplied light,
  which compositors adding it over what lies behind show correctly.
- Cost: six draws, all but the last at half resolution or less, plus a small
  per-pixel mask that tone mapping reads to tell blurred pixels from sharp
  ones. On an Apple M2 Max it adds about 0.2 to 0.3 ms per frame at 1280 x 800
  and 0.5 to 0.8 ms at 2560 x 1600, the most when every pixel is out of focus.
  On the raster tier the half-float scene and the depth and normal pass come on
  top: in all, about 0.2 to 0.3 ms of GPU time per frame at 1280 x 800 and
  1.0 ms at 2560 x 1600, or 0.7 ms there when ambient occlusion already draws
  the depth and normal pass.

## Motion blur

**Motion blur** smears what moves on screen along its motion, as a camera
shutter left open for part of each frame does. It covers both the camera's
motion and each object's own. Turn it on with `motionBlur`, and set how long the
shutter stays open with `motionBlurShutter`:

```cpp
auto settings = view.getRenderer().getRenderSettings();
settings.motionBlur = true;
settings.motionBlurShutter = 0.5f; // half of the frame time
view.getRenderer().setRenderSettings (settings);
view.repaint();
```

| Field                               | Default | Unit                    | What it does                                     |
| ----------------------------------- | ------- | ----------------------- | ------------------------------------------------ |
| `RenderSettings::motionBlur`        | off     |                         | Draws the motion blur.                           |
| `RenderSettings::motionBlurShutter` | 0.5     | fraction of a frame     | How long the shutter stays open, from 0 to 1. 0 or less blurs nothing. |

- It runs on the full tier only, which knows how each pixel moved. The raster
  tier ignores the setting, which is not an error, and
  `isAvailable (RenderCapabilities::Feature::motionBlur)` reports it.
- It is off by default and costs nothing while off. With it on, frames where
  nothing moved cost nothing either: still frames, the frames of a converged
  image and the first frame after the scene is shown draw no motion blur at
  all. A shutter of 0 or less blurs nothing.
- It runs after temporal anti-aliasing and the depth of field, and before bloom,
  so changing its settings keeps the accumulation: repaint after such a change.
- Motion is measured against the previous frame, so the frames that start
  over do not blur: after a resize, a change of tier or `taaSamples`,
  `restartAccumulation()` (use it on a camera cut) or a failed frame.
- A blurred frame is never the final image, even with `taaSamples` at 1: the
  renderer reports it as not converged, so the component draws one more frame,
  and once nothing moves the image is sharp again.
- `motionBlurShutter` is the fraction of the time between two frames the
  shutter stays open, from 0 to 1: at 1 a smear spans the whole motion since the
  previous frame, centered on where the object is now. A smear spans at most 32
  pixels of the image, so very fast motion is shortened.
- The smear carries the coverage of the scene, so a moving object over the
  background color smears into it, and what it spreads keeps its brightness
  there, as the depth of field's blur does. Still parts of a frame where
  something else moves keep the usual blend, and so do partly covered areas
  the smear leaves as they were, such as fog over the background color.
- How far each pixel moved comes from the previous frame the renderer drew, so
  render every frame of an animation: a renderer that skipped frames sees the
  whole jump as one motion.
- Each 16 x 16 pixel tile smears along one direction, the fastest within the
  48 x 48 pixels around it, and smears that reach the 32 pixel cap tie: where
  two fast motions in different directions meet, one of them smears along the
  other's.
- Cost: only on frames where something moves, four draws, all but the last
  at a quarter of the resolution or less, plus the same mask as the depth of
  field for tone mapping. Where everything around a pixel moves alike, as when
  the camera turns, the smear skips most of its work. On an Apple M2 Max, with
  the camera orbiting the Scene 3D model, it adds about 0.2 ms of GPU time per
  frame at 1280 x 800 and 0.7 ms at 2560 x 1600, plus 0.1 ms there for tone
  mapping. Frames where only lights change, with nothing moving on screen,
  cost exactly what they cost with it off.

## Fog

A `WorldFogNode` fills the whole scene with an exponential height fog. It
is part of the scene rather than of the render settings: a scene with fog asks
for it. It is analytic: volumetric fog, with fog volumes and light shafts, is not
in this release.

```cpp
auto& fog = scene->getRoot().attach<yup::WorldFogNode>().fog; // enabled
fog.density = 0.05f;       // extinction per meter at baseHeight
fog.heightFalloff = 0.3f;  // thinner above baseHeight, denser below
fog.color = { 0.8f, 0.85f, 0.9f };
view.repaint();
```

| Field                     | Default | Unit          | What it does                                                    |
| ------------------------- | ------- | ------------- | --------------------------------------------------------------- |
| `WorldFog::enabled`       | false   |               | Draws the fog. A default `WorldFogNode` turns it on.            |
| `WorldFog::density`       | 0.02    | per meter     | How much light the fog takes per meter of distance, at `baseHeight`. 0 or less draws nothing. |
| `WorldFog::heightFalloff` | 0       | per meter     | How fast the fog thins with height above `baseHeight`, and thickens below it: 0 is uniform, negative thickens upwards. |
| `WorldFog::baseHeight`    | 0       | meters        | The height where the fog has `density`.                         |
| `WorldFog::color`         | white   | linear RGB    | The share of the light the fog scatters back, per channel.      |
| `WorldFog::anisotropy`    | 0       | -0.95 to 0.95 | Henyey-Greenstein g: positive scatters forward, so the fog glows looking towards the sun. |

World units are read as meters. A fog with any field that is not a finite
number is not drawn.

- It works on both tiers. On the raster tier it needs multisampled half-float
  render targets at the current sample count, and the depth and normal pass of
  ambient occlusion, as depth of field does. Frames with fog then render the
  scene in half float and draw that pass, and the fog is the first post effect. Its depth comes from one
  sample per pixel while the color is antialiased, so fogged silhouettes against
  a far background can differ by about a pixel. Where the device lacks them,
  the fog is not drawn, which `isAvailable (RenderCapabilities::Feature::worldFog)`
  reports, and it is logged once in debug builds.
- It is off by default and costs nothing while off, even with its other fields
  set.
- On the full tier the fog is drawn before temporal anti-aliasing, so changing
  it restarts the accumulation. Screen-space reflections and GI read the
  previous frames, fog included, so a reflected surface is fogged once more.
- The fog dims each pixel by the transmittance from the camera to the first
  surface, and adds the light it scatters towards the camera over that
  distance. That light is the ambient color, the mean light of the environment
  times its intensity, and the first directional light that is not dark. It is
  single scattering without shadows: light reaches the fog everywhere, behind
  objects too.
- The scattering is physically scaled. A dense isotropic fog lit by a sun of
  intensity E shows E / (4 pi) times its color, a quarter of what a white wall
  facing that sun shows; forward scattering makes it brighter looking towards
  the sun. Ambient light and the environment come from every direction, so the
  same fog shows them at their full radiance.
- Where nothing was drawn, the fog reaches the camera's far plane, so the
  background color, or the visible environment, blends towards the fog. With an
  infinite far plane (`zFar` of 0, or not past `zNear`), a view that never
  leaves the fog is fully fogged: in uniform fog, looking level, or looking
  down into denser fog. A view climbing out of fog that thins upwards meets
  only a limited amount of it, so the background still shows through.
- Depth of field and motion blur keep the fog's blend with the background
  color: they composite in light only what they spread, and blurring the fog's
  even coverage spreads nothing. The other side of this: out-of-focus
  highlights spread over a partly fogged background color look dimmer than over
  a clear one, about half as bright at a transmittance of 0.8, since the fog
  already covers those pixels. A visible environment is unaffected.
- Transparent surfaces take the fog of what lies behind them, since only opaque
  surfaces write the depth the fog reads.
- Cost: one full-screen draw. On an Apple M2 Max, in the Scene 3D light show
  with the camera orbiting, it takes about 0.04 ms of GPU time per frame at
  1280 x 800 and 0.15 ms at 2560 x 1600 on the full tier. On the raster tier
  the half-float scene, the depth and normal pass and the tone mapping pass
  come on top: in all about 0.15 ms per frame at 1280 x 800 and 0.5 ms at
  2560 x 1600.
