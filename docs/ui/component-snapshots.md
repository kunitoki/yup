# Component Snapshots

Capturing a component's subtree into an `Image` or a GPU texture. Use
`snapshotToImage` for CPU-side pixel access and `snapshotToTexture` for
GPU-only compositing.

```cpp
#include <yup_gui/yup_gui.h>
```

---

## snapshotToImage — Grab Subtree to CPU Image

Captures the entire subtree (self + children + `paintOverChildren`) into a
CPU-side `Image`. This runs synchronously: it renders offscreen, reads the GPU
pixels back to CPU memory, populates `Image::getRawData()`, and returns.

```cpp
void MyComponent::onSnapshotClicked()
{
    // Obtain a GraphicsContext — typically captured during paint().
    auto snapshot = animatedArea.snapshotToImage (*gpuContext);
    if (snapshot.isValid())
    {
        preview.setImage (std::move (snapshot));
    }
}
```

### Including or excluding effects

The second parameter controls whether an active `ComponentEffect` is applied to
the snapshot:

```cpp
auto rawSnapshot    = comp.snapshotToImage (ctx, false); // no effect
auto effectSnapshot = comp.snapshotToImage (ctx, true);  // with effect (default)
```

When `includeEffects` is `true` and the component has a `ComponentEffect`:

1. The subtree renders to a first `GpuCanvas`.
2. `ComponentEffect::apply()` composites the result into a second canvas.
3. The second canvas is read back as the `Image`.

When `includeEffects` is `false`, the effect is skipped entirely and the raw
subtree pixels are returned.

### Constraints

- Requires a **real GPU context** (OpenGL, Metal, etc.). Headless contexts
  cannot create `GpuCanvas` and will return an invalid `Image`.
- The snapshot dimensions match the component's `getWidth()` × `getHeight()`.
  A zero-sized component returns an invalid `Image`.
- The call blocks until GPU readback completes; it is intended for occasional
  use (e.g., a "save screenshot" button), not per-frame capture.

---

## snapshotToTexture — Grab Subtree to GPU Texture

Like `snapshotToImage` but skips the CPU readback step, returning a
`GpuTexture::Ptr` instead of an `Image`. This is faster and uses less memory
when the snapshot is only needed for on-screen compositing:

```cpp
// Capture the component as a GPU texture, then draw it elsewhere.
auto tex = comp.snapshotToTexture (ctx, includeEffects);
if (tex != nullptr)
    g.drawTexture (tex, targetBounds);
```

The returned texture is only valid while the `GpuCanvas` (owned internally by
the `GpuTexture`) is alive. Store the returned pointer to keep it valid — when
the last reference is dropped, the texture is released.

```{tip}
Use `snapshotToImage` when you need CPU access to the pixels (save to file,
upload, pixel analysis). Use `snapshotToTexture` when the snapshot is drawn
back to the screen — it avoids the costly readback.
```

### Constraints

Same constraints as `snapshotToImage` — requires a real GPU context, matches
the component's dimensions, and blocks until GPU work is submitted (no CPU
readback wait means no GPU→CPU sync stall).

---

## Related

- [Component caching](component-caching.md) — `setCachedToTexture` and
  `isCachedToTexture`
- [Component effects (shaders)](component-effects.md)
- [Imaging — drawing images](../imaging/drawing.md)
- [Graphics RHI — `GpuCanvas`](../graphics/rhi/index.md)
