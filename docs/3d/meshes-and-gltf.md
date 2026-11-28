# Meshes, textures and glTF

Resources that entities share: meshes and textures built from code, and
glTF 2.0 assets turned into entity trees, with their skins, their morph
targets and their animations.

## Building meshes from code

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

`addPrimitive()` returns a `Result`: a primitive that is not made of triangles
of its vertices is not added, and the failure tells why.

`addPrimitive()` also takes the joints and weights of a skinned primitive and
its morph targets, see [Animation](animation.md#skins).

## Changing a mesh

`setVertices()` gives a primitive new vertices, keeping its indices and its
material: for a flag, a wave, anything computed on the CPU. The renderer
follows by itself.

```cpp
auto mesh = yup::Mesh::Ptr (new yup::Mesh ("flag"));
mesh->addPrimitive (std::move (vertices), std::move (indices), material);

// Later, each time the shape changes. The vertices as the mesh keeps them: addPrimitive()
// gives each triangle its own vertices when it generates flat normals
auto deformed = mesh->getPrimitive (0).vertices;

for (auto& vertex : deformed)
    vertex.position = wave (vertex.position, time);

mesh->setVertices (0, deformed);
view.repaint();                                      // a SceneComponent draws when it repaints
```

- **Same count, same order.** The indices refer to the vertices, so their
  number can't change: another count changes nothing, and `setVertices()`
  returns a failed `Result` telling why. To change the topology, build another
  mesh.
- **Taken as they are.** Nothing is generated: the caller writes the
  positions, the normals and the tangents.
- **They are copied** into the storage the primitive already has, so a call
  allocates nothing and a caller deforming at every frame keeps its own buffer.
- **The bounds follow**, and with them picking, the shadow fits and what a
  frame leaves out: a mesh whose bounds the camera does not see is not drawn,
  and a shadow map only draws the meshes that cast into it.
- **The renderer sees it.** `getChangeCount()` counts the calls: when it
  changes, the shadow maps the mesh casts into are drawn again and the
  accumulation of the full tier restarts, as for an entity that moved. So only
  call `setVertices()` when the vertices really changed, or the view keeps
  repainting.
- **A mesh is shared**: every entity drawing it shows the new vertices. Give
  each deformed instance a mesh of its own, as a
  [`DeformNode`](animation.md#skins) does.
- A mesh that changes shape has no motion of its own: the temporal
  anti-aliasing does not reproject it and motion blur does not smear it.
  Reflection probes and irradiance volumes do not bake again: call their
  `requestBake()`.

## Textures from the GPU

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
materials, textures, skins, animation clips and one
[prefab](entities-and-nodes.md#prefabs) per glTF scene, returned by
`getPrefab()`. `instantiate()` builds a fresh entity tree in a
scene each time it is called, and all the trees share the same resources, so a
model can be instanced many times, in one or several scenes:

```cpp
auto model = yup::GltfModel::loadFromFile (file);
if (model.failed())
{
    YUP_DBG (model.getErrorMessage());
    return;
}

for (const auto& warning : model.getReference().getWarnings())
    YUP_DBG (warning);

yup::Entity instance = model.getReference().instantiate (*scene, someEntity);
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
    [Clearcoat and sheen](showing-a-scene.md#clearcoat-and-sheen);
  - `KHR_materials_sheen` adds a velvet-like sheen;
- images stored in buffer views, data URIs or external files, including
  `EXT_texture_webp` sources;
- nodes with their transforms (a `matrix` is split into translation, rotation and
  scale), meshes, cameras and `KHR_lights_punctual` lights;
- skins, with the `JOINTS_0` and `WEIGHTS_0` of the primitives they move, and
  morph targets, as `POSITION`, `NORMAL` and `TANGENT` deltas. An instance draws
  such a mesh deformed, in the pose its nodes have;
- animations, each as an `AnimationClip`. An asset that has some gets an
  `AnimationNode` on the root entity of each instance, paused: see
  [Animation](animation.md).

Primitives that are not triangles are skipped with a warning, and so are the
joints beyond the four of `JOINTS_0` and the morph targets of texture
coordinates and colors. Files that require an extension `yup_3d` does not
support fail to load. Files that only use one load without what it changes,
with a warning naming the extension.

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

Files are not trusted in what they state either: what a model keeps is bounded
by the size of its buffers, and what would take it over is left out with a
warning, see [The budget of a model](animation.md#the-budget-of-a-model).
