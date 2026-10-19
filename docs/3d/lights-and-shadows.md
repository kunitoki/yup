# Lights and shadows

## Lights

A scene can hold any number of `LightNode`s: directional lights, in lux, and
point and spot lights, in candela. A point or spot light stops at its `range`,
or where it gets too dim to see when it has none. Which lights shade a surface
depends on the tier:

- **Full:** every light reaching a surface shades it. Directional lights reach
  everything; the view is split into small blocks, and each block is shaded by up
  to 63 of the point and spot lights reaching it.
- **Raster:** each object is lit by its 8 most influential lights: directional
  lights first, then the brightest and nearest point and spot lights that reach
  it. When more lights reach an object, the ones close to that cutoff are dimmed,
  so a moving light fades in and out of an object instead of switching on or off.

### Area lights

Give a `LightNode` a `radius` to turn it into an area light, on both tiers. Its
highlights widen and soften on smooth surfaces. A small or distant emitter gives
the same light as the punctual one; a large, close one is integrated over its
whole size, without the part a surface's horizon hides:

- point lights become spheres of that radius, in world units;
- spot lights become spheres of that radius too, still limited by their cone;
- directional lights become a disk in the sky, `radius` being its angular radius
  in radians (the sun is about 0.0047).

```cpp
auto& bulb = lamp.attach<yup::LightNode> (yup::LightNode::Type::point);
bulb.radius = 0.1f;
```

A radius of 0, the default, keeps the light punctual. Area lights have a cost:
a frame where any light has a radius shades with the area light shaders, which
are heavier for every pixel; when all radii are 0 the renderer switches back to
the lighter punctual shaders. Very smooth surfaces seen at
grazing angles can show a misplaced highlight under an area light. The radius
also softens the shadows of the light, see [Shadows](#shadows).

## Shadows

Set `castsShadows` on any `LightNode` to make it cast shadows of the opaque
objects, on both tiers:

- **Directional lights.** The first shadowed directional light in the scene
  gets cascaded shadow maps: the view is cut into depth ranges, each with its own
  map, so shadows stay sharp near the camera and still reach far away. When the
  whole scene fits one map at least as sharp as the nearest cascade, a single map
  fitted to the scene replaces them. Other directional lights light the scene
  without shadows.
- **Point and spot lights** share one shadow atlas: a spot light takes one view
  of it and a point light six. Lights that look larger on screen get larger
  views: up to half the atlas side while every light still gets a view at least
  as large as with a quarter, a quarter otherwise, so a few lights get the
  sharpest shadows and many still fit. Each tier has a budget of views (see the
  table below); when more lights ask for shadows, the most important ones get
  them and the others light without shadows.

Shadow maps are kept on both tiers until something they show changes: the
casters, the shadow casting lights or the shadow settings. Each cascade and each
point or spot light view is drawn again on its own, only when a caster it reaches
changes, so an object moving outside a light's reach costs that light nothing.
Camera moves redraw the cascades, and the point and spot light views when a
light's size on screen changes enough to resize them. A shadowed point or spot
light coming or going, or growing on screen past a quarter of the atlas, can
move and resize the other lights' views, which are then redrawn. A camera
orbiting a compact scene usually redraws no shadow at all. The renderer cannot
see vertices edited in place in a `Mesh`: after such edits call
`getRenderer().restartAccumulation()`, on either tier, which also redraws every
shadow map.

A shadow map the device cannot create, such as a large atlas on a device short
of memory, leaves the lights that need it without shadow maps instead of failing
the frame (their contact shadows stay); the renderer does not ask for that size
again until the shadow settings change it. A device that cannot compile the
shadow pipelines renders every frame without shadows, contact shadows included.

```cpp
// A sun from above, slightly soft, with contact shadows on the full tier
auto sun = scene->createEntity ("sun");
sun.attach<yup::TransformNode>().setRotation (yup::Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -1.0f));
auto& sunLight = sun.attach<yup::LightNode>();
sunLight.intensity = 3.0f;
sunLight.castsShadows = true;
sunLight.radius = 0.02f;        // angular radius in radians
sunLight.contactShadows = true;

// A spot light 2 units above the origin, shining down
auto lamp = scene->createEntity ("lamp");
lamp.attach<yup::TransformNode> (yup::Vector3<float> { 0.0f, 2.0f, 0.0f },
                                 yup::Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -yup::MathConstants<float>::halfPi));
auto& spot = lamp.attach<yup::LightNode> (yup::LightNode::Type::spot);
spot.intensity = 8.0f;
spot.range = 5.0f;
spot.outerConeAngle = 0.6f;
spot.castsShadows = true;
spot.radius = 0.05f;            // sphere radius in world units
```

**Soft shadows.** A light's `radius` (see [Area lights](#area-lights)) also sets
how soft its shadows are: the penumbra widens with the size of the light and
with the distance between the caster and the surface it shades, so a shadow is
sharp where an object touches the ground and softer further away. Even there a
soft shadow is blurred over at least two texels of its shadow map, so its edge
does not show the map's texel steps where the shadow starts at its caster. A
radius of 0 gives hard shadows with slightly filtered edges. Soft shadows are on
by default on the full tier; on the raster tier set `RenderSettings::softShadows`
to `true` to turn them on, at a lower quality. On the full tier the penumbrae are
refined over the frames that temporal anti-aliasing accumulates, so they are grainy
while the scene moves and smooth once it is still; a higher `taaSamples` makes
them smoother, see [Temporal anti-aliasing](platform-tiers.md#temporal-anti-aliasing).

**Shadow bias.** `LightNode::shadowBias` scales the small depth offset that
keeps lit surfaces from shadowing themselves. The default, 1, suits most scenes.
Raise it when lit surfaces show stripes or dots of shadow (shadow acne); lower
it when shadows look detached from the objects casting them. 0 removes the
depth offset of the surfaces receiving the shadow; the offset along their normal
and the slope bias of the casters remain.

**Contact shadows.** Set `LightNode::contactShadows` to add short shadows where
objects meet, finer than a shadow map can show, such as under buttons or knobs
sitting on a panel. They are traced on screen up to `contactShadowDistance`
towards the light, and `contactShadowThickness` is how thick the objects they
hit are assumed to be. They work with or without `castsShadows`, and only on
the full tier; the raster tier ignores them.

The shadow settings in `RenderSettings`:

| Setting            | Default | What it does                                                                                       |
| ------------------ | ------- | -------------------------------------------------------------------------------------------------- |
| `sunCascades`      | 0       | Cascades of the shadowed directional light, from 1 to 4. 0 picks the tier default. More cascades keep shadows sharp further away, at the cost of one more map each. |
| `shadowResolution` | 2048    | Texels per side of each cascade, from 256 to 4096. It also sizes the point and spot light atlas.   |
| `softShadows`      | unset   | Soft shadows from the light `radius`. Unset follows the tier: on for full, off for raster.         |
| `contactShadows`   | on      | Contact shadows for the lights that ask for them. Full tier only.                                  |

What each tier gets:

|                              | Raster                          | Full                                  |
| ---------------------------- | ------------------------------- | ------------------------------------- |
| Directional light cascades   | 2 by default                    | 4 by default                          |
| Point and spot light atlas   | `shadowResolution` per side, rounded down to a power of two | twice `shadowResolution`, rounded down to a power of two, up to 4096 |
| Point and spot light views   | up to 16                        | about 60, shared with the cascades    |
| Soft shadows                 | opt-in                          | on, refined by TAA                    |
| Contact shadows              | no                              | yes                                   |
