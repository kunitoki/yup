# Environment and indirect lighting

## Environment lighting and reflections

An `EnvironmentMap` is the light surrounding the scene. Attach it with an
`EnvironmentNode` and every surface reflects it, sharply when smooth and
blurred when rough, and receives its soft diffuse light. Metals and glossy
plastics, which only had highlights from the punctual lights, look like the real
material.

The environment is sampled once on the CPU from any function returning the
radiance along a direction, for example an analytic sky or a studio of soft
boxes. Everything else happens on the GPU: the first time a renderer draws it,
it is turned into a cube and filtered for every roughness, one mip each, in a
frame of its own that never stalls the one drawing it. The environment itself
never changes and holds no GPU objects, so several scenes, windows and devices
can share one:

```cpp
auto sky = [] (const yup::Vector3<float>& direction)
{
    return direction.getY() > 0.0f ? yup::Vector3<float> (0.4f, 0.6f, 1.0f)
                                    : yup::Vector3<float> (0.05f, 0.05f, 0.05f);
};

auto& environment = scene->getRoot().attach<yup::EnvironmentNode> (new yup::EnvironmentMap (sky));
environment.intensity = 1.0f;
scene->getRoot().attach<yup::AmbientLightNode> (yup::Colors::black); // the environment replaces the flat ambient
```

Small bright lights such as a sun keep their energy at every roughness, so they
show as sharp glints on smooth surfaces and as broad highlights on rough ones.
The constructor's `width` sets the sharpness of the reflections (the cube faces
are a quarter of it) and `numLevels` the number of roughness mips.

- `EnvironmentNode::visible` draws the environment behind the scene instead
  of the background color, and `EnvironmentNode::blur` softens it, from 0 (sharp)
  to 1, like a shallow depth of field.
- A `ToneMappingNode` with `ToneMappingOperator::aces` maps the light with the ACES
  filmic curve, which suits bright environments better than the default Reinhard.
  `ToneMappingOperator::agx` is a third option, a filmic curve that keeps very
  saturated highlights from skewing in hue. See [Render settings](render-settings.md).

On devices that cannot render half floats, the environment cube, the reflection
probes and the irradiance volumes hold their light in 8 bits per channel
(RGBM), which reaches up to 8 times the brightness of white: brighter
highlights, such as a small sun, are clamped there and reflect dimmer than on
other devices.

**Upgrading.** The `EnvironmentMap` constructor is unchanged, but the map no
longer keeps filtered images or GPU objects. `getLevelWidth`, `getLevelHeight`,
`getLevelPixels`, `getBlurForRoughness`, `getIrradiance`, `getGpuTexture` and
`getGpuSampler` are removed:

- for the diffuse light along a normal, pass `getIrradianceCoefficients()` to
  `SphericalHarmonics::evaluateIrradiance`;
- `getFaceSize()` gives the size of the cube faces, a quarter of `width`;
- the background blur (`EnvironmentNode::blur`) now shows the same filtered
  levels as the reflections, a slightly different look from the former blur.

## Indirect lighting overview

Besides the lights, a surface receives the light bouncing off everything around
it. Five features provide it, from the farthest to the closest, and add up:

| Feature | What it gives | Raster | Full | Cost when on (Apple M2 Max) |
| ------- | ------------- | ------ | ---- | --------------------------- |
| [Reflection probes](#reflection-probes) | Reflections of the scene around a point | Each object blends its 2 probes, picked by its bounds | Per pixel, up to 64 a frame | Baked when added, moved or asked for |
| [Irradiance volumes](#irradiance-volumes) | Diffuse light baked in a box | Blended from the 8 probes around a surface, with visibility; one volume per object | The same per pixel, up to 8 volumes a frame | Baked when added, moved or asked for |
| [Ambient occlusion](#ambient-occlusion) | Darkens creases and contacts | Opt-in, at half resolution | On by default, at half resolution | About 1 ms at 2560 x 1600 |
| [Screen-space reflections](#screen-space-reflections) | Reflections of what the camera sees | No | On by default | About 0.2 ms on the Scene 3D demo at 1280 x 800, 0.7 ms at 2560 x 1600 |
| [Screen-space diffuse GI](#screen-space-diffuse-gi) | Light bounced by nearby surfaces | No | Off by default | About 0.3 ms at 1280 x 800, 1.2 ms at 2560 x 1600 |

Where none of them reaches, the environment and the ambient color light the
surface. Probes and volumes cost nothing until a scene has them, and the
screen-space effects nothing while they are off. On the full tier, a scene with
probes or volumes pays for them only on the pixels they reach: the rest of the
frame shades as without them. To see what an effect adds,
show it alone with `RenderSettings::debugView`, see [Render settings](render-settings.md).

## Ambient occlusion

Ambient occlusion darkens the light a surface receives from its surroundings
where nearby geometry hides part of them: in creases, where objects meet the
ground, and between knobs and buttons on a panel. It is traced on screen every
frame from the depth and normals of the visible surfaces.

```cpp
yup::RenderSettings settings;
settings.ambientOcclusion = true; // on raster; the full tier has it on by default
settings.aoDistance = 0.1f;       // how far, in scene units, surfaces occlude each other
view.getRenderer().setRenderSettings (settings);
```

- Only the indirect light is darkened: the flat ambient light and the
  environment. Lights, and their shadows, are never occluded.
- The diffuse light of the environment is taken along each surface's bent
  normal, the mean direction of the sky it can still see: next to a wall, a
  surface is lit by the open side rather than by the wall's direction.
- The environment reflections are occluded too, less so at grazing angles, so
  an open, shiny floor keeps its reflections while a crease loses them.
- The material's own occlusion texture still applies; where both darken a
  surface, the darker one wins.
- Transparent (blended) surfaces are not occluded: the occlusion is traced from
  the opaque surfaces behind them.
- `aoDistance` should match the scale of the details you want shaded. Too
  large, and objects cast dark halos on everything around them. Close to the
  camera, the occlusion reaches at most a tenth of the view height on screen. An
  `aoDistance` of 0 or less turns ambient occlusion off.

Ambient occlusion is traced at half resolution on both tiers, then filtered and
stretched over the frame:

| Tier   | Default | How it is drawn                                                                 |
| ------ | ------- | ------------------------------------------------------------------------------- |
| Full   | on      | Its noise changes every frame and is averaged by TAA, so it settles once the image converges; without TAA it is traced with twice the samples and stays still. Stretched with depth- and normal-aware weights, so it mostly stays inside the edges of objects; features thinner than about 2 pixels take the occlusion of their neighbors. It costs about 1 ms per frame at 2560 x 1600 on an Apple M2 Max: set `ambientOcclusion = false` to save it. |
| Raster | off     | After one extra depth and normal pass, stretched by plain filtering: thin objects can show a halo of occlusion about one pixel wide along their edges. |

On raster, ambient occlusion needs half-float render targets: on devices without
them the setting is ignored and the frame renders as if it were off.
`getCapabilities()->isEnabled (RenderCapabilities::Feature::ambientOcclusion, settings)`
tells whether it was asked for and is available. It stays true when the device
fails to create the occlusion targets or shaders at run time, which renders the
frame without ambient occlusion and logs it once in debug builds.

## Reflection probes

An environment lies infinitely far away, so every surface reflects the same sky.
A `ReflectionProbeNode` captures the scene itself around a point instead: the
surfaces near it reflect what really surrounds them, such as the walls of a
room, the panel around a knob, or a light the camera does not see.

```cpp
auto probe = scene->createEntity ("probe");
probe.attach<yup::TransformNode> (yup::Vector3<float> { 0.0f, 1.0f, 0.0f }); // where the scene is captured from

auto& node = probe.attach<yup::ReflectionProbeNode>();
node.shape = yup::ReflectionProbeNode::Shape::box;
node.influenceExtents = { 2.0f, 1.0f, 2.0f }; // the room, around the entity
```

- The influence is a sphere (`influenceRadius`) or a box (`influenceExtents`)
  centered on the entity, which moves, turns and scales it. Surfaces inside it
  take their reflections from the probe, fading to the environment over
  `falloff` towards its border.
- `parallaxCorrection`, on by default, projects the reflections onto the
  influence shape, so a box that matches a room keeps the reflections of its
  walls in place on every surface. Turn it off for a probe over open space.
- Where influences overlap, the smaller one takes priority, so a small probe
  inside a large one wins near what it encloses. A surface blends at most two
  probes, and the environment takes what they leave; without an environment
  that share reflects black. On the raster tier each object picks its two
  probes as a whole, from the influences its bounds touch: split large objects,
  such as a long floor, that span more than two probes.
- Probes replace the environment's reflections only: the diffuse light still
  comes from the environment, the ambient color and the irradiance volumes. `intensity` scales the
  captured light.
- A capture renders the scene from the entity's origin as the raster tier does,
  without exposure or tone mapping, with the environment behind it and hard
  shadows, but without ambient occlusion or the other probes. The sun's shadow
  covers the whole scene; point and spot light shadows are drawn at a low
  resolution. Its faces are 128 pixels wide; `clipNear` keeps nearby geometry
  out of it, up to half the capture's far plane.

A probe is captured again when:

- it is attached, or `ReflectionProbeNode::requestBake()` is called;
- one of its own fields or its entity's transform changes;
- `SceneRenderer::bakeProbes()` finds it never baked.

Changes to the rest of the scene never capture it again by themselves, so the
reflections show the scene as it was: call `requestBake()` after editing what
surrounds the probe. Each capture costs six renders of the scene plus filtering.
`render()` captures the pending probes at the start of the frame; call
`bakeProbes (device, scene)` beforehand, for example behind a loading screen, to
move that cost. On the full tier at most 64 probes shade a frame, the largest
ones being left out beyond that. A capture the device cannot draw never fails a
frame: the probe reflects nothing meanwhile, and after two failures in a row the
renderer stops capturing until `bakeProbes()` is called again.

## Irradiance volumes

The environment and the ambient color light every surface alike, whatever
surrounds it. An `IrradianceVolumeNode` bakes the diffuse light inside a box
instead, so a floor next to a red wall turns a little red, and a corner behind a
wall stays out of the light of the room next door.

```cpp
auto volume = scene->createEntity ("volume");
volume.attach<yup::TransformNode> (yup::Vector3<float> { 0.0f, 1.5f, 0.0f }); // the center of the box

auto& node = volume.attach<yup::IrradianceVolumeNode>();
node.extents = { 4.5f, 2.0f, 3.5f };          // half the size of the box, around the entity
node.resolutionX = 6;                         // probes along each axis
node.resolutionY = 2;
node.resolutionZ = 4;
```

- The box is divided into `resolutionX` x `resolutionY` x `resolutionZ` cells
  (from 2 to 7 along each axis, `IrradianceVolumeNode::maxResolution`), with a
  probe at the center of each. A probe
  captures the scene around it into a 32 pixel cube, keeps its first two bands of
  spherical harmonics, and the distances to the geometry around it.
- A surface inside the box takes its diffuse light from the 8 probes around it,
  weighted by distance, by which side of the surface they are on, and by whether
  they can see it (McGuire et al. 2019): a probe on the other side of a wall does
  not light it. The volume fades to the environment's diffuse light over
  `falloff` (in world units) towards its border. `intensity` scales the baked
  light, which the environment intensity does not scale; `normalBias` moves the
  visibility test off the surface, against shadow acne.
- Volumes give diffuse light only: reflections still come from the reflection
  probes and the environment. Where volumes overlap, the smallest one is used.
  The raster tier picks the volume of each object by the center of its bounds,
  so an object spanning two volumes takes all of its light from one of them;
  the full tier picks per pixel, and shades up to 8 volumes a frame. The raster
  pick also differs in that it tests the box's world-aligned bounds (a rotated
  box reaches past its corners there), it keeps a volume that is still baking
  (that object then takes the environment's light meanwhile), and between equal
  boxes it keeps the first one in the scene, where the full tier keeps the oldest.
- Where a volume fades out over its falloff, or where none of its probes can see
  a surface (a closed cabinet without a probe inside), the environment takes the
  rest of the light, not a larger volume around it: give a volume nested in
  another a `falloff` of 0, or its border shows the environment.
- One bounce: a capture is lit by the environment, the lights and their shadows,
  as a reflection probe capture is, but not by the volumes or the probes, so the
  light the volume adds is the light of the scene bounced once.
- On devices without half float render targets the bake keeps the first band in
  RGBM and the second as ratios to it, slightly less precise, and skips the
  visibility: light then leaks through walls between the probes. Keep the probes
  out of walls there, or use more of them.
- Any one volume fits the atlases; many large ones can fill them, and a volume
  left without room lights nothing (logged once in debug builds). Below the float
  floor there is no visibility, so they hold about five times more probes.

A volume is baked again under the same rules as a reflection probe: when it is
attached, on `IrradianceVolumeNode::requestBake()`, when one of its own fields or
its entity's transform changes, and by `SceneRenderer::bakeProbes()`. Changes to
the rest of the scene never bake it again by themselves. A bake costs six renders
of the scene per probe (288 for a 6 x 2 x 4 volume). A frame captures at most 48
probe positions, so a larger volume, or several volumes, bake over several
frames, and each volume lights nothing until it is complete. Bake them ahead
with `bakeProbes (device, scene)` behind a loading screen, calling it until it
returns 0. A volume whose bake fails leaves the reflection probes baking, and
the other way round.

## Screen-space reflections

Reflection probes and the environment are captured from a few fixed points, so
they cannot show what moves, nor the exact surroundings of every surface. On the
full tier, smooth surfaces also reflect what the camera sees: a glossy floor
shows the objects standing on it, a varnished knob the panel next to it, from the
previous frame of the image.

```cpp
yup::RenderSettings settings;
settings.screenSpaceReflections = false; // on by default on the full tier
view.getRenderer().setRenderSettings (settings);
```

- A reflection is found by marching a ray over the depth of the frame. Where the
  ray leaves the screen, goes behind what the camera sees, or turns back towards
  the camera, the surface reflects the reflection probes and the environment
  instead, blending into them near the screen edges. Turning the camera can
  therefore change a reflection: what is off screen is not reflected.
- Only surfaces with a roughness below 0.6 trace rays. From 0.5 to 0.6 they fade
  into the probes and the environment, which reflect the rough surfaces alone:
  their wide, blurred reflections need many rays to settle, and the probes are as
  good there.
- Every reflection is traced at half resolution, one ray per 2 x 2 pixels around
  the sharpest part of the reflection, and each pixel combines the rays of its
  neighbors. Glossy reflections are noisy at first and settle as the image
  converges, like soft shadows: temporal anti-aliasing averages the rays, which
  change every frame.
- What the reflection replaces is the reflected light itself: the surface keeps
  its reflectance, its ambient occlusion and its material occlusion. The flat
  ambient color is not a reflection and still adds to it.
- The clearcoat layer of a material keeps the probes and the environment, while
  the material under it reflects the screen: an approximation, since a clearcoat
  is often the sharpest reflection of a surface.
- Transparent (blended) surfaces do not trace reflections, and a ray never stops
  on them. Where one covers the surface a ray finds, the reflection shows that
  surface seen through it, as the previous frame showed it.

Screen-space reflections need the history of temporal anti-aliasing, so
`taaSamples` of 1 turns them off; the raster tier never has them and ignores the
setting. The first frame after the renderer is created or resized has no history
yet and renders without them. They cost about 0.7 ms per frame at 2560 x 1600 and
1.2 ms at 3456 x 2234 on an Apple M2 Max: set `screenSpaceReflections = false` to
save it. A device that cannot compile them renders without them, which is logged
once in debug builds.

## Screen-space diffuse GI

Reflection probes, irradiance volumes and the environment light a surface from
afar; the ambient occlusion only darkens what nearby objects hide. On the full
tier, screen-space diffuse global illumination also lights each surface with what
those nearby objects send back: a red wall tints the floor at its foot, a lit
panel brightens the knob next to it. It is off by default:

```cpp
yup::RenderSettings settings;
settings.screenSpaceGI = true; // off by default on every tier
view.getRenderer().setRenderSettings (settings);
```

- It extends the ambient occlusion: the search that finds what hides each part
  of a surface's sky also takes the light of what it found, from the previous
  frame of the image. So it gathers only within `aoDistance` (0.2 by default),
  and at most a tenth of the view height on screen: raise `aoDistance` for a
  wider reach, at the cost of a coarser ambient occlusion.
- The gathered light adds to what the probes, the volumes and the environment
  give the rest of the sky, so an open surface looks the same with and without
  it. Colored materials, clearcoats and sheens take it as they take the
  environment's diffuse light.
- It is the light the previous frame shows, so it builds up over frames: one
  bounce per frame, and the light that bounced in the previous frame bounces
  again. Like the soft shadows, it is noisy while the image changes and settles
  as it converges.
- Only what the camera sees counts. Something off screen, behind another object,
  or seen only from its back lights nothing, and a thin object seen edge on
  (a wall seen from straight above) gives little or no light.
- Only the diffuse light of non-metals takes it: metals, which have no diffuse
  color, get none, and the reflections of the hidden part of the sky (the
  specular light it loses to the ambient occlusion, including the light bounced
  inside rough surfaces) are not replaced either.
- It gathers the image as the camera saw it, highlights included, so a shiny
  object's highlight bounces onto its surroundings as if every viewer saw it.

Screen-space diffuse GI needs ambient occlusion and the history of temporal
anti-aliasing (`taaSamples` above 1): without either it is off, and the raster
tier ignores the setting. A frame without that history (the first one after
the renderer is created or resized, after `restartAccumulation()` or a change of
`taaSamples`) renders without it. It costs about 0.3 ms per frame at
1280 x 800 and 1.2 ms at 2560 x 1600 on an Apple M2 Max, and nothing while it is
off. A device that cannot compile it renders without it, which is logged once in
debug builds.
