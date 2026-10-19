# Components in a scene

Any component can be shown on a mesh of a scene and stay fully interactive:
clicks, drags, hover, keyboard focus and popups keep working. The component
stays a child of the `SceneComponent` that draws the scene, which renders it to
a texture, puts the texture in a material, and maps input from its own points
onto the mesh.

This page covers the scene side. The input hooks and `MeshSurfaceMapper` are
described in [Components in 3D](../ui/component-3d.md); the Scene 3D demo
(`examples/graphics/source/examples/Scene3D.h`) uses this pattern for the LCD
of the MPC model.

## The material

Create the texture without a GPU texture, and use it in the material of the
mesh that shows the component. As emissive, the component keeps its colors
whatever the lighting; as base color, it is lit like the rest of the scene.

```cpp
screenTexture = new yup::Texture (yup::GpuTexture::Ptr(),
                                  yup::GpuSamplerDesc (yup::GpuFilter::linear, yup::GpuWrapMode::clampToEdge),
                                  true);

auto material = yup::Material::Ptr (new yup::Material());
material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
material->emissiveTexture = screenTexture;
material->emissiveFactor = { 1.0f, 1.0f, 1.0f };

screen = scene->createEntity ("screen");
screen.attach<yup::MeshNode> (screenMesh);
screen.attach<yup::MaterialNode> (material);
```

The texture coordinates of the mesh decide where the component lands: `(0, 0)`
is its top left corner and `(1, 1)` its bottom right one. See
[Textures from the GPU](meshes-and-gltf.md#textures-from-the-gpu).

## Rendering the component

Mark the component as manually composited, so the host draws it instead of the
normal child painting, and hand its texture to the scene before drawing the
frame:

```cpp
ScreenView()
{
    panel.setManuallyComposited (true);
    addAndMakeVisible (panel);
}

void paint (yup::Graphics& g) override
{
    auto& context = g.getGraphicsContext();
    if (context.isGpuAvailable())
    {
        auto texture = panel.renderToTexture (context, 2.0f);
        if (panelRepainted)
            screenTexture->setGpuTexture (std::move (texture));

        panelRepainted = false;
    }

    yup::SceneComponent::paint (g);
    updateScreenMapping();
}
```

`renderToTexture()` only renders again when the panel repainted, but every
`setGpuTexture()` restarts the temporal accumulation of the full tier, so only
call it when the content changed: the demo sets a flag in the panel's
`paint()`. A scale above 1 keeps the panel sharp when the camera gets close.

## Mapping input

Give a `MeshSurfaceMapper` the same positions, texture coordinates and indices as
the mesh, and after each frame the matrix it was drawn with: the entity world
matrix followed by the camera view and projection.

```cpp
void updateScreenMapping()
{
    auto* camera = getScene()->getActiveCamera();
    if (camera == nullptr || getWidth() <= 0.0f || getHeight() <= 0.0f)
        return;

    const auto view = camera->getEntity().getWorldMatrix().inverted();
    const auto projection = camera->getProjectionMatrix (getWidth() / getHeight());

    const yup::SpinLock::ScopedLockType sl (mapperLock);
    mapper.setModelViewProjection (screen.getWorldMatrix().followedBy (view).followedBy (projection), getLocalBounds());
}
```

Then override `getChildPointFromLocal()` and `getLocalPointFromChild()` for the
panel, converting between viewport points and texture coordinates times the
panel size, as shown in [Routing input](../ui/component-3d.md#routing-input).
Return `std::nullopt` from `getChildPointFromLocal()` when the panel should not
take input, for example while the camera moves, and forward other children to
`SceneComponent`.

To let clicks outside the panel drive the camera, check
`mapper.hitTest (position)` in `mouseDown()`: unlike `viewportToUV()`, it does
not extrapolate beyond the mesh.
