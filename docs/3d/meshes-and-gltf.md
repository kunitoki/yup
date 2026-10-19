# Meshes, textures and glTF

Resources that entities share: meshes and textures built from code, and
glTF 2.0 assets turned into entity trees.

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
materials, textures and one [prefab](entities-and-nodes.md#prefabs) per glTF
scene, returned by `getPrefab()`. `instantiate()` builds a fresh entity tree in a
scene each time it is called, and all the trees share the same resources, so a
model can be instanced many times, in one or several scenes:

```cpp
auto model = yup::GltfModel::loadFromFile (file);
if (model.failed())
{
    DBG (model.getErrorMessage());
    return;
}

for (const auto& warning : model.getReference().getWarnings())
    DBG (warning);

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
