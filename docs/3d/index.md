# 3D

Build 3D worlds out of entities, load glTF 2.0 assets into them, and show them
in a component, rendered through the [RHI](../graphics/rhi/index.md) on every
backend.

**Modules covered:** `yup_3d` (with the `tinygltf` third-party module).

## Concepts

- **`EntityNode`** - an element of the scene tree. It has a name, a local
  transform (position, `Quaternion` rotation, scale), a visibility flag that
  hides its whole subtree, child entities and attached **parts**.
- **Parts** (`Node` subclasses) - give an entity its behavior. `MeshNode` draws
  a mesh, `CameraNode` and `LightNode` place a camera and a light, and
  `MaterialNode` overrides the material of every mesh in the subtree. An entity
  with no parts is a plain group.
- **Resources** - `Mesh`, `Material` and `Texture` are reference counted and
  shared: the same mesh can be drawn by any number of entities. Their CPU data
  is the source of truth; GPU buffers and textures are created the first time
  they are rendered.
- **`Scene`** - a root entity, the active camera and the global lighting
  settings (background, flat ambient light, exposure, a default light).
- **`SceneComponent`** - a `Component` that renders a scene and draws it over
  its whole area.

Entities, parts and scenes are meant to be used from the message thread, like
components.

## Entities and parts

Entities are reference counted. Adding an entity that already has a parent
moves it, and adding an entity under itself or one of its descendants is
rejected.

```cpp
auto body = yup::EntityNode::Ptr (new yup::EntityNode ("body"));
body->setPosition ({ 0.0f, 1.0f, 0.0f });
body->setRotation (yup::Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));

auto wheel = yup::EntityNode::Ptr (new yup::EntityNode ("wheel"));
body->addChild (wheel);
```

Parts are addressed by their exact type and a **slot**, so an entity can hold
several parts of the same type. `attach<T>()` uses slot 0; `attach<T, N>()` uses
slot N, and attaching to an occupied slot replaces the part there. The `...At`
forms take the slot at runtime:

```cpp
body->attach<yup::MeshNode> (bodyMesh);          // slot 0
body->attach<yup::MeshNode, 1> (decalMesh);      // slot 1

if (auto* decal = body->getNode<yup::MeshNode, 1>())
    decal->mesh = otherDecal;

for (int slot = 0; slot < 4; ++slot)
    wheel->attachAt<yup::LightNode> (slot, yup::LightNode::Type::point);

body->detach<yup::MeshNode, 1>();
```

How several parts of one type behave:

| Part           | Several slots                                                        |
| -------------- | -------------------------------------------------------------------- |
| `MeshNode`     | Every slot is drawn.                                                 |
| `LightNode`    | Every slot lights the scene, see [Lights](#lights).                  |
| `MaterialNode` | Only the lowest slot overrides; the others are free for your use.    |
| `CameraNode`   | Pick one with `Scene::setActiveCamera (entity, slot)`.               |

A `MaterialNode` overrides the material of every mesh in the entity's subtree,
its own meshes included. The nearest override up the tree wins.

### Custom parts

Subclass `Node` to add behavior. `Scene::update()` calls `update()` on every
part of every visible entity:

```cpp
class Spinner : public yup::Node
{
public:
    void update (double deltaSeconds) override
    {
        angle += static_cast<float> (deltaSeconds);
        getEntity()->setRotation (yup::Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, angle));
    }

private:
    float angle = 0.0f;
};

entity->attach<Spinner>();
```

### Transforms and bounds

The local matrix applies scale, then rotation, then translation, and is cached
until the transform changes. `getWorldMatrix()` walks up through the parents.
`computeWorldBounds()` returns the world space box around every mesh in the
visible subtree, which is handy to frame a model with the camera.

### Building meshes from code

```cpp
auto mesh = yup::Mesh::Ptr (new yup::Mesh ("quad"));

std::vector<yup::Mesh::Vertex> vertices (4);
vertices[0].position = { -1.0f, -1.0f, 0.0f };
vertices[1].position = { 1.0f, -1.0f, 0.0f };
vertices[2].position = { 1.0f, 1.0f, 0.0f };
vertices[3].position = { -1.0f, 1.0f, 0.0f };

auto material = yup::Material::Ptr (new yup::Material());
material->baseColorFactor = { 0.8f, 0.2f, 0.2f, 1.0f };
material->metallicFactor = 0.0f;

mesh->addPrimitive (vertices, { 0, 1, 2, 0, 2, 3 }, material);
```

Triangles are counter-clockwise when front facing. When every normal is zero,
flat normals are generated. Materials follow the glTF metallic-roughness model:
factors multiply their textures, the base color and emissive textures are sRGB,
and alpha can be opaque, masked or blended.

### Textures from the GPU

A `Texture` can also hold a `GpuTexture` instead of an `Image`, for example a
component rendered with `Component::renderToTexture()`. Replace its content with
`setGpuTexture()` before the scene is drawn:

```cpp
screenTexture = new yup::Texture (yup::GpuTexture::Ptr(), {}, true); // sRGB
screenMaterial->emissiveTexture = screenTexture;

void paint (yup::Graphics& g) override // in a SceneComponent subclass
{
    screenTexture->setGpuTexture (screen.renderToTexture (g.getGraphicsContext(), 2.0f));
    yup::SceneComponent::paint (g);
}
```

Only call `setGpuTexture()` when the content really changed, for example after
redrawing the component: every call restarts the temporal accumulation of the
full tier, so calling it on every paint keeps the view repainting forever.

Mark the component with `setManuallyComposited (true)` and map input onto it with
a `MeshSurfaceMapper`, as the Scene 3D example does for the screen of its model.

## Loading glTF

`GltfModel` loads `.gltf` and `.glb` files and converts them into meshes,
materials, textures and entity templates. `createEntity()` builds a fresh entity
tree each time it is called, and all the trees share the same resources, so a
model can be instanced anywhere:

```cpp
auto model = yup::GltfModel::loadFromFile (file);
if (model.failed())
{
    DBG (model.getErrorMessage());
    return;
}

for (const auto& warning : model.getReference().getWarnings())
    DBG (warning);

someEntity->addChild (model.getReference().createEntity());
```

What is converted:

- triangle primitives with `POSITION`, `NORMAL`, `TANGENT`, `TEXCOORD_0` and
  `COLOR_0`, dense or sparse, any component type including normalized integers.
  Normal maps follow the authored tangents; without them, the tangent frame is
  derived from the texture coordinates;
- metallic-roughness materials with their five textures, alpha mode and
  double-sided flag, plus the factors of five material extensions (their
  textures are ignored, with a warning):
  - `KHR_materials_emissive_strength` multiplies the emissive color, for
    emitters brighter than 1, such as screens and lamps that glow with bloom;
  - `KHR_materials_ior` sets the index of refraction of non-metals, which
    changes how strongly they reflect when seen head-on;
  - `KHR_materials_specular` scales and tints those reflections;
  - `KHR_materials_clearcoat` adds a clear varnish over the material, see
    [Clearcoat and sheen](#clearcoat-and-sheen);
  - `KHR_materials_sheen` adds a velvet-like sheen;
- images stored in buffer views, data URIs or external files, including
  `EXT_texture_webp` sources;
- nodes with their transforms (a `matrix` is split into translation, rotation and
  scale), meshes, cameras and `KHR_lights_punctual` lights.

Skinning, morph targets and animations are ignored, and primitives that are not
triangles are skipped with a warning. Files that require an extension `yup_3d`
does not support fail to load.

```{note}
Images are decoded with the image formats of `yup_graphics`. Link `libpng`,
`libjpeg` and/or `libwebp` into your target for the textures your assets use;
a texture whose image can't be decoded is left out with a warning. An
`EXT_texture_webp` texture falls back to its regular source when WebP isn't
available.
```

External buffers and images are only read from the folder of the file (or the
`baseDirectory` passed to `loadFromData()`) and its subdirectories: absolute
paths and `..` segments are rejected.

## Showing a scene

```cpp
class Viewer : public yup::Component
{
public:
    explicit Viewer (const yup::GltfModel& model)
    {
        auto scene = yup::Scene::Ptr (new yup::Scene());
        scene->getRoot()->addChild (model.createEntity());

        auto camera = yup::EntityNode::Ptr (new yup::EntityNode ("camera"));
        camera->attach<yup::CameraNode>();
        camera->setPosition ({ 0.0f, 0.5f, 4.0f });
        scene->getRoot()->addChild (camera);
        scene->setActiveCamera (camera.get());

        view.setScene (scene);
        addAndMakeVisible (view);
    }

    void resized() override { view.setBounds (getLocalBounds()); }

private:
    yup::SceneComponent view;
};
```

Cameras look down the -Z axis of their entity with +Y up, like glTF cameras.
When the active camera's entity is gone or has no camera in the chosen slot, the
first camera found in the visible tree is used; with no camera at all, a default
perspective camera at (0, 0, 5) looks at the origin. Without any `LightNode`, a
default directional light is used unless you turn it off with
`Scene::setUsingDefaultLight (false)`.

`SceneComponent` draws only when it repaints: call `repaint()` after changing the
scene. For animated scenes, `setContinuousUpdates (true)` calls `Scene::update()`
and repaints on every display frame. On the full tier it also keeps repainting by
itself until the image has converged, see [Platform tiers](#platform-tiers).

### Rendering

`SceneRenderer` renders a scene into a texture and can be used without a
component. Shading follows the glTF BRDF (GGX, Smith, Schlick) with punctual
and area lights, a flat ambient term, environment lighting, shadows, normal
mapping, alpha masking and blending, and double-sided materials; the result is
exposed, tone mapped and sRGB encoded. Skinning and animation are not available
yet.

**Deferred and forward.** On the full tier, opaque and masked materials are
drawn once into a set of intermediate targets, then shaded in a single pass per
pixel, so overlapping objects and many lights cost little. Blended materials are
drawn after that with forward shading, over the shaded image. On the raster tier
every material is drawn with forward shading. Both paths shade with the same
model, so a material looks the same on either tier.

Image quality:

- **MSAA** antialiases geometry edges with 4 samples per pixel by default.
  Change it with `getRenderer().setSampleCount (n)`; the count is limited to what
  the device supports, and 1 turns multisampling off. It only affects the raster
  tier: the full tier antialiases with TAA instead.
- **Specular antialiasing** widens the highlights of very smooth surfaces where
  the normal changes quickly across a pixel, so they don't sparkle while moving.
- **Anisotropic filtering** keeps glTF textures sharp at grazing angles on
  devices that support it.

### Clearcoat and sheen

Two optional layers go over a material, after the glTF extensions of the same
names:

- **Clearcoat:** a clear varnish, as on car paint, lacquered wood or a glossy
  knob cap. `clearcoatFactor` sets its strength (0 for none) and
  `clearcoatRoughnessFactor` how blurred its reflections are. It reflects the
  lights, the reflection probes and the environment through its own sharp
  highlight, on top of the material's own reflections, and dims what lies under
  it, the emission included, a little head-on and more at grazing angles.
- **Sheen:** the soft glow of velvet, felt or rubber at grazing angles.
  `sheenColorFactor` sets its color (black for none) and `sheenRoughnessFactor`
  how wide the glow spreads. It reflects the lights and the diffuse light of the
  ambient color, the irradiance volumes and the environment, and dims the
  material under it.

```cpp
auto paint = yup::Material::Ptr (new yup::Material());
paint->baseColorFactor = { 0.6f, 0.05f, 0.05f, 1.0f };
paint->metallicFactor = 0.0f;
paint->clearcoatFactor = 1.0f;
paint->clearcoatRoughnessFactor = 0.05f;
```

Both factors default to 0, which shades a material exactly as without the
layers. A frame that draws any material with a layer shades with heavier shaders
on the raster tier, and for its opaque materials on the full tier; frames
without layered materials, reflection probes and irradiance volumes included,
never run them. Layered surfaces cost about as much again as plain ones.

The textures of both extensions (`clearcoatTexture`,
`clearcoatRoughnessTexture`, `clearcoatNormalTexture`, `sheenColorTexture` and
`sheenRoughnessTexture`) are ignored. Without a clearcoat normal texture the
varnish follows the surface without its normal map on the raster tier and for
the full tier's blended materials. On the full tier, opaque materials have only
their normal-mapped normal, so there the clearcoat highlights follow the normal
map too. Reflection probes and irradiance volumes capture the materials without
their layers.

Two approximations keep layered frames fast:

- The sheen dims the material under it by how much it reflects towards the
  viewer, the same for every light. The glTF extension also accounts for the
  angle of each light, which this skips: under a white sheen, a light at 60
  degrees from the surface normal lights the material up to about a quarter too
  brightly (20% of its brightness without the sheen). Darker sheens, and lights
  facing the surface, show less.
- Under an area light (a `LightNode` with a radius), the sheen is lit as if the
  light came from its center. It is exact for small lights and smooth for large
  ones, but a large, close light gives a slightly different sheen than a bank of
  small ones would.

### Lights

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

#### Area lights

Give a `LightNode` a `radius` to turn it into an area light, on both tiers. Its
highlights widen and soften on smooth surfaces. A small or distant emitter gives
the same light as the punctual one; a large, close one is integrated over its
whole size, without the part a surface's horizon hides:

- point lights become spheres of that radius, in world units;
- spot lights become spheres of that radius too, still limited by their cone;
- directional lights become a disk in the sky, `radius` being its angular radius
  in radians (the sun is about 0.0047).

```cpp
auto& bulb = lamp->attach<yup::LightNode> (yup::LightNode::Type::point);
bulb.radius = 0.1f;
```

A radius of 0, the default, keeps the light punctual. Area lights have a cost:
a frame where any light has a radius shades with the area light shaders, which
are heavier for every pixel; when all radii are 0 the renderer switches back to
the lighter punctual shaders. Very smooth surfaces seen at
grazing angles can show a misplaced highlight under an area light. The radius
also softens the shadows of the light, see [Lights and shadows](#lights-and-shadows).

### Environment lighting and reflections

An `EnvironmentMap` is the light surrounding the scene. Set one with
`Scene::setEnvironment()` and every surface reflects it, sharply when smooth and
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

scene->setEnvironment (new yup::EnvironmentMap (sky));
scene->setEnvironmentIntensity (1.0f);
scene->setAmbientColor (yup::Colors::black); // the environment replaces the flat ambient
```

Small bright lights such as a sun keep their energy at every roughness, so they
show as sharp glints on smooth surfaces and as broad highlights on rough ones.
The constructor's `width` sets the sharpness of the reflections (the cube faces
are a quarter of it) and `numLevels` the number of roughness mips.

- `setEnvironmentVisible (true)` draws the environment behind the scene instead
  of the background color, and `setEnvironmentBlur()` softens it, from 0 (sharp)
  to 1, like a shallow depth of field.
- `setToneMapping (Scene::ToneMapping::aces)` maps the light with the ACES
  filmic curve, which suits bright environments better than the default Reinhard.
  `Scene::ToneMapping::agx` is a third option, a filmic curve that keeps very
  saturated highlights from skewing in hue. See [Render settings](#render-settings).

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
- the background blur (`setEnvironmentBlur()`) now shows the same filtered
  levels as the reflections, a slightly different look from the former blur.

### Indirect lighting overview

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
screen-space effects nothing while they are off. To see what an effect adds,
show it alone with `RenderSettings::debugView`, see [Render settings](#render-settings).

### Ambient occlusion

Ambient occlusion darkens the light a surface receives from its surroundings
where nearby geometry hides part of them: in creases, where objects meet the
ground, and between knobs and buttons on a panel. It is traced on screen every
frame from the depth and normals of the visible surfaces.

```cpp
yup::RenderSettings settings;
settings.ambientOcclusion = true; // on raster; the full tier has it on by default
settings.aoDistance = 0.1f;       // how far, in scene units, surfaces occlude each other
scene->setRenderSettings (settings);
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

### Reflection probes

An environment lies infinitely far away, so every surface reflects the same sky.
A `ReflectionProbeNode` captures the scene itself around a point instead: the
surfaces near it reflect what really surrounds them, such as the walls of a
room, the panel around a knob, or a light the camera does not see.

```cpp
auto probe = yup::EntityNode::Ptr (new yup::EntityNode ("probe"));
probe->setPosition ({ 0.0f, 1.0f, 0.0f }); // where the scene is captured from

auto& node = probe->attach<yup::ReflectionProbeNode>();
node.shape = yup::ReflectionProbeNode::Shape::box;
node.influenceExtents = { 2.0f, 1.0f, 2.0f }; // the room, around the entity
scene->getRoot()->addChild (probe);
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

### Irradiance volumes

The environment and the ambient color light every surface alike, whatever
surrounds it. An `IrradianceVolumeNode` bakes the diffuse light inside a box
instead, so a floor next to a red wall turns a little red, and a corner behind a
wall stays out of the light of the room next door.

```cpp
auto volume = yup::EntityNode::Ptr (new yup::EntityNode ("volume"));
volume->setPosition ({ 0.0f, 1.5f, 0.0f }); // the center of the box

auto& node = volume->attach<yup::IrradianceVolumeNode>();
node.extents = { 4.5f, 2.0f, 3.5f };          // half the size of the box, around the entity
node.resolutionX = 6;                         // probes along each axis
node.resolutionY = 2;
node.resolutionZ = 4;
scene->getRoot()->addChild (volume);
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

### Screen-space reflections

Reflection probes and the environment are captured from a few fixed points, so
they cannot show what moves, nor the exact surroundings of every surface. On the
full tier, smooth surfaces also reflect what the camera sees: a glossy floor
shows the objects standing on it, a varnished knob the panel next to it, from the
previous frame of the image.

```cpp
yup::RenderSettings settings;
settings.screenSpaceReflections = false; // on by default on the full tier
scene->setRenderSettings (settings);
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

### Screen-space diffuse GI

Reflection probes, irradiance volumes and the environment light a surface from
afar; the ambient occlusion only darkens what nearby objects hide. On the full
tier, screen-space diffuse global illumination also lights each surface with what
those nearby objects send back: a red wall tints the floor at its foot, a lit
panel brightens the knob next to it. It is off by default:

```cpp
yup::RenderSettings settings;
settings.screenSpaceGI = true; // off by default on every tier
scene->setRenderSettings (settings);
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

## Lights and shadows

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
auto sun = yup::EntityNode::Ptr (new yup::EntityNode ("sun"));
sun->setRotation (yup::Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -1.0f));
auto& sunLight = sun->attach<yup::LightNode>();
sunLight.intensity = 3.0f;
sunLight.castsShadows = true;
sunLight.radius = 0.02f;        // angular radius in radians
sunLight.contactShadows = true;
scene->getRoot()->addChild (sun);

// A spot light 2 units above the origin, shining down
auto lamp = yup::EntityNode::Ptr (new yup::EntityNode ("lamp"));
lamp->setPosition ({ 0.0f, 2.0f, 0.0f });
lamp->setRotation (yup::Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -yup::MathConstants<float>::halfPi));
auto& spot = lamp->attach<yup::LightNode> (yup::LightNode::Type::spot);
spot.intensity = 8.0f;
spot.range = 5.0f;
spot.outerConeAngle = 0.6f;
spot.castsShadows = true;
spot.radius = 0.05f;            // sphere radius in world units
scene->getRoot()->addChild (lamp);
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
them smoother, see [Temporal anti-aliasing](#temporal-anti-aliasing).

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

## Render settings

`RenderSettings` chooses how a scene is rendered. Set it on the scene, then
repaint:

```cpp
yup::RenderSettings settings;
settings.bloom = true;
settings.bloomIntensity = 0.1f;
settings.taaSamples = 8;

scene->setRenderSettings (settings);
scene->setToneMapping (yup::Scene::ToneMapping::agx);
scene->setExposure (1.2f);
```

| Setting          | Default     | What it does                                                                                   |
| ---------------- | ----------- | ---------------------------------------------------------------------------------------------- |
| `tier`           | `automatic` | `automatic`, `raster` or `full`, see [Platform tiers](#platform-tiers).                        |
| `taaSamples`     | 16          | Full tier: how many jittered frames are accumulated before the image is final. Fewer converge sooner and antialias less. |
| `bloom`          | off         | Glow around bright highlights, see [Post effects](#post-effects).                              |
| `bloomThreshold` | 1.0         | Exposed radiance above which pixels glow.                                                      |
| `bloomIntensity` | 0.05        | How much of the blurred glow is added to the image.                                            |
| `bloomRadius`    | 0.85        | From 0 to 1: how far the glow spreads.                                                         |
| `ambientOcclusion` | unset     | Darkens the indirect light in creases, see [Ambient occlusion](#ambient-occlusion). Unset follows the tier: on for full, off for raster. |
| `aoDistance`     | 0.2         | How far, in scene units, surfaces occlude each other for ambient occlusion. 0 or less turns it off. |
| `screenSpaceReflections` | unset | Smooth surfaces reflect what the camera sees, see [Screen-space reflections](#screen-space-reflections). Unset follows the tier: on for full; raster never has them. Needs `taaSamples` above 1. |
| `screenSpaceGI` | unset | Nearby surfaces light each other, see [Screen-space diffuse GI](#screen-space-diffuse-gi). Unset is off on every tier; raster never has it. Needs `ambientOcclusion` and `taaSamples` above 1. |
| `depthOfField` | off | Full tier: blurs what lies away from the camera's focus, see [Depth of field](#depth-of-field). |
| `depthOfFieldMaxSize` | 32 | The largest depth of field blur, as a diameter in pixels of the image, at most 64. |
| `debugView` | `none` | Full tier: shows an intermediate image instead of the final one, see below. |

**Debug view.** `debugView` shows what one screen-space effect computes, in
place of the final image, to tune it:

- `ambientOcclusion`: the ambient occlusion as gray, white where nothing is
  occluded;
- `reflections`: the screen-space reflections, exposed and weighted by how
  confident each one is;
- `indirectDiffuse`: the light the screen-space diffuse GI gathers, exposed,
  before the color of the surfaces is applied.

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
    bool glows = caps->isEnabled (yup::RenderCapabilities::Feature::bloom, scene->getRenderSettings());
}
```

Related scene settings:

- **Exposure** (`Scene::setExposure`) multiplies the light before tone mapping.
- **Tone mapping** (`Scene::setToneMapping`) maps the exposed light into the
  displayable range: `reinhard` (the default) rolls highlights off softly,
  `aces` adds contrast and saturation, and `agx` is a filmic curve that keeps
  saturated highlights from skewing. AgX is YUP's own fit of the look, opt-in
  like ACES.
- **Background color** (`Scene::setBackgroundColor`) is display-referred on both
  tiers: it is never tone mapped nor affected by exposure, and shows exactly as
  set.

## Platform tiers

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
| Direct3D 11, Vulkan, WebGPU             | Raster, for now                 | Raster, for now                |
| OpenGL, OpenGL ES, WebGL2               | Raster                          | Raster                         |

`automatic` picks the full tier on desktop GPUs that pass the capability checks
(half-float render targets, enough color attachments, compute shaders with storage
buffers readable from fragment shaders, and a verified backend).
Today only Metal is verified; Direct3D 11, Vulkan and WebGPU stay on raster until
the full tier is validated on them. OpenGL and OpenGL ES never run the full
tier. Mobile defaults to raster. Asking for `full` on a device that cannot run it
falls back to raster.

### Temporal anti-aliasing

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
  `Texture`, a material, a light, the ambient light, the environment or the
  exposure. Once the scene is still again, the history counts as a single
  frame among the `taaSamples` that follow, so edges keep only a faint trace of
  the image before the change. After a large change, such as a cut to another
  shot, call `restartAccumulation()`.
- Changing the size of the view, the tier or `taaSamples` discards the history,
  and the image starts over.
- Tone mapping, the bloom and depth of field settings, the depth of field
  fields of the camera and the debug view are applied after the accumulation,
  so changing them neither restarts nor discards it. Repaint after changing
  them: the component does not repaint by itself for them.
- When you change something the renderer cannot see, such as vertices edited in
  place in a `Mesh`, or a `GpuTexture` redrawn without calling
  `Texture::setGpuTexture()`, call `getRenderer().restartAccumulation()`. It
  discards the history, and the image starts over.
- Calling `Texture::setGpuTexture()` on every frame stops the image from ever
  converging, and the component then repaints forever. Call it only when the
  content changed.

## Post effects

**Bloom** adds a soft glow around highlights brighter than `bloomThreshold`. It
works on both tiers:

- On the full tier it is always available.
- On the raster tier it needs multisampled half-float render targets at the
  current sample count (`setSampleCount`). When the device lacks them, bloom is
  unavailable and the image is unchanged; the capabilities report it through
  `isAvailable (RenderCapabilities::Feature::bloom)`.
- Turning bloom on in the raster tier makes the scene render in half float and
  tone map after antialiasing. Silhouettes and transparent surfaces over the
  background still look as they do without bloom. Where two surfaces meet inside
  a pixel, the brighter one weighs a little less than without bloom.
- The glow strength depends somewhat on the resolution of the view: the same
  intensity looks a little different at different sizes.

### Depth of field

**Depth of field** blurs what lies in front of or behind the distance the camera
focuses at, like a photographic lens. Turn it on with `depthOfField`, then shape
it with the fields of the active `CameraNode`:

| Field            | Default | Unit                     | What it does                                                    |
| ---------------- | ------- | ------------------------ | --------------------------------------------------------------- |
| `fStop`          | 2.8     | f-number                 | The aperture: smaller blurs more. 0 or less turns the blur off. |
| `focusDistance`  | 10      | world units, read as meters | The distance along the view that stays perfectly sharp.      |
| `apertureBlades` | 0       | count                    | The shape of the out-of-focus highlights: round below 3, otherwise a polygon with that many sides. |
| `sensorHeight`   | 0.024 (24 mm) | world units, read as meters (constant) | With `yFov`, it sets the focal length of the lens. |

```cpp
auto* camera = scene->getActiveCamera();
camera->fStop = 1.8f;
camera->focusDistance = 2.5f;

auto settings = scene->getRenderSettings();
settings.depthOfField = true;
scene->setRenderSettings (settings);
view.repaint();
```

- It is available on the full tier only for now; the raster tier ignores the
  setting, which `isAvailable (RenderCapabilities::Feature::depthOfField)`
  reports.
- It is off by default and costs nothing while off. A camera that cannot blur
  (orthographic, `fStop` or `yFov` of 0 or less, or no active camera) costs
  nothing either, and neither does a `depthOfFieldMaxSize` of 0 or less.
- It runs after temporal anti-aliasing and before bloom, so changing it or the
  camera's depth of field fields keeps the accumulation: repaint after such a
  change.
- `depthOfFieldMaxSize` caps the blur, as a diameter in pixels of the image, at
  most 64.
- The blur carries the coverage of the scene, so silhouettes over the background
  color fade into it. On frames it blurs, the image is composited over the
  background color in light rather than in display values: a bright highlight
  blurred over the background keeps the brightness it has over a surface or a
  visible environment, and the edges of blurred objects over the background
  look as they do over a dark environment. Frames without depth of field keep
  the usual blend, where antialiased edges over the background match the
  raster tier. Over a transparent background color, blurred frames output
  premultiplied light, which compositors adding it over what lies behind show
  correctly.
- Cost: six draws, all but the last at half resolution or less. On an Apple
  M2 Max it adds about 0.2 to 0.3 ms per frame at 1280 x 800 and 0.5 to 0.8 ms at
  2560 x 1600, the most when every pixel is out of focus.
