# Components in 3D

A component tree can be presented on arbitrary 3D geometry (a flat quad at an
angle, a curved panel, a cube face) while staying fully interactive: clicks,
drags, hover, mouse wheel, keyboard focus, text editing, popups and drag and drop
keep working through the normal dispatch.

The idea is that the presented component stays **in the hierarchy**, as a child of
the component that draws the 3D scene (the *host*). The host renders it to a
texture, draws the texture on its mesh, and tells the input system how its own
points map onto the child.

---

## Compositing the child yourself

```cpp
panel.setManuallyComposited (true);   // the host composites the panel itself
addAndMakeVisible (panel);
```

A component marked with `setManuallyComposited (true)` is no longer drawn by the
normal child painting pass; it stays visible, hit-testable and focusable, and the
host composites it in its own `paint()`:

```cpp
void paint (yup::Graphics& g) override
{
    auto texture = panel.renderToTexture (g.getGraphicsContext());
    // ... draw the mesh sampling texture, with your GpuPipeline ...
}
```

`renderToTexture()` renders the subtree, including its component effect, into a
texture owned by the component and reused across calls. It only renders again
when something in the subtree repainted, or the size changed, since the last
call, so an idle panel costs nothing.

Repainting anything inside a manually composited component repaints the whole host,
since the host decides where the texture ends up.

Popups follow the same path: `ComboBox` adds its menu to
`getPopupParentComponent()`, the closest transformed or manually composited
ancestor, so the menu opens on the surface next to the widget that opened it.
Use it for your own popups too.

---

## Routing input

Every input path (hit-testing, mouse events, drag and drop, `screenToLocal()`)
maps points one parent to child step at a time through
`Component::getChildPointFromLocal()`. The default removes the child position and
applies the inverse child transform. A host overrides it for its presented child,
together with the inverse `getLocalPointFromChild()`, which backs
`localToScreen()`, popup placement and the text input caret rectangle:

```cpp
std::optional<Point<float>> getChildPointFromLocal (const Component& child, Point<float> p) const override
{
    if (&child != &panel)
        return Component::getChildPointFromLocal (child, p);

    const SpinLock::ScopedLockType sl (lock);
    if (auto uv = mapper.viewportToUV (p))
        return Point<float> (uv->getX() * panel.getWidth(), uv->getY() * panel.getHeight());

    return std::nullopt;
}

std::optional<Point<float>> getLocalPointFromChild (const Component& child, Point<float> p) const override
{
    if (&child != &panel)
        return Component::getLocalPointFromChild (child, p);

    const SpinLock::ScopedLockType sl (lock);
    return mapper.uvToViewport ({ p.getX() / panel.getWidth(), p.getY() / panel.getHeight() });
}
```

The contract of these hooks:

- **Return a point even outside the child bounds.** Hit-testing rejects points
  outside the child by itself, but a captured drag needs the extrapolated
  position to keep tracking when the pointer leaves the surface.
- **Return `std::nullopt` only for degenerate mappings**, such as a surface seen
  from behind. Hit-testing then skips the child, and the conversions that must
  return a point fall back to removing the child position.
- **Expect synthetic events.** When the surface moves under a pointer that stays
  still (auto rotation, animated camera), the window re-evaluates the pointer
  after each painted frame and sends the enter / exit, move or drag events the
  new mapping implies.
- **Read a snapshot of the last drawn state.** The hooks run on the message
  thread while painting runs on the render thread: publish the camera and mesh
  from `paint()` under a lock or through atomics. This also keeps input consistent
  with what is on screen.

---

## MeshSurfaceMapper

`MeshSurfaceMapper` does the geometry: given the mesh positions, texture
coordinates and indices, and the model-view-projection matrix used to draw them,
it maps between viewport points and texture coordinates.

```cpp
mapper.setMesh (positions, uvs, indices);             // the same data uploaded to the GPU
mapper.setModelViewProjection (mvp, getLocalBounds()); // the camera that was drawn
mapper.setBackFaceCulling (true);                      // match the pipeline cull mode

if (auto hit = mapper.hitTest (point))       // nearest real intersection
    useUV (hit->uv, hit->depth);

auto uv = mapper.viewportToUV (point);       // hit, or extrapolated off the surface
auto viewportPoint = mapper.uvToViewport (uv); // inverse, for localToScreen
```

- Build the matrix with `Matrix4`, as `model.followedBy (view).followedBy (projection)`,
  and upload `mvp.getData()` as the shader uniform. When the vertex shader only
  applies that matrix, the CPU picking matches the GPU rasterization exactly.
- Texture coordinates must follow the convention the shader samples with. Canvas
  textures have v pointing down like component coordinates, so sampling the
  texture coordinates as they are keeps the two consistent.
- With several surfaces, compare `Hit::depth` to resolve occlusion: the lowest
  depth is the closest.
- Picking is O(triangles) per event, which is fine for UI meshes.

---

## The curved panel pattern

`examples/graphics/source/examples/Component3DDemo.h` puts everything together:

1. A `CurvedPanelView` host owns a widget panel marked `setManuallyComposited (true)`.
2. A cylindrical section is generated on the CPU; the same arrays are uploaded to
   the GPU and given to `MeshSurfaceMapper::setMesh()` under a `SpinLock`.
3. `paint()` calls `panel.renderToTexture()`, draws the mesh with depth testing and
   back face culling, then publishes the drawn matrix with
   `mapper.setModelViewProjection()` under the lock.
4. The host overrides `getChildPointFromLocal()` / `getLocalPointFromChild()` as
   shown above, and orbits the camera when the empty space around the panel is
   dragged.

```{note}
`renderToTexture()` renders at one pixel per point, so a panel viewed up close
on a high density display looks soft. Size the panel accordingly.
```

---

## Related

- [Component basics](component-basics.md) — transforms and coordinate conversion
- [Component effects](component-effects.md) — input mapping for distorting effects
- [Primitives](../graphics/primitives.md) — `Vector3`, `Matrix4`, `Ray`, `MeshSurfaceMapper`
- [RHI pipelines](../graphics/rhi/pipelines.md) — `GpuPipeline` and `GpuRenderPass`
