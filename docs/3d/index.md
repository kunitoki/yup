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
| `LightNode`    | Every slot lights the scene, up to `SceneRenderer::maxLights` lights. |
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

- triangle primitives with `POSITION`, `NORMAL`, `TEXCOORD_0` and `COLOR_0`,
  dense or sparse, any component type including normalized integers;
- metallic-roughness materials with their five textures, alpha mode and
  double-sided flag;
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
and repaints on every display frame.

### Rendering

`SceneRenderer` renders a scene into a texture and can be used without a
component. Shading follows the glTF BRDF (GGX, Smith, Schlick) with punctual
lights, a flat ambient term, normal mapping, alpha masking and blending, and
double-sided materials; the result is exposed, tone mapped and sRGB encoded.

Image quality:

- **MSAA** antialiases geometry edges with 4 samples per pixel by default.
  Change it with `getRenderer().setSampleCount (n)`; the count is limited to what
  the device supports, and 1 turns multisampling off.
- **Specular antialiasing** widens the highlights of very smooth surfaces where
  the normal changes quickly across a pixel, so they don't sparkle while moving.
- **Anisotropic filtering** keeps glTF textures sharp at grazing angles on
  devices that support it.

Image-based lighting, shadows, skinning and animation are not available yet.
