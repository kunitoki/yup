# Offscreen Targets & Canvases

The RHI offers two offscreen render surfaces. Both are reference-counted, both
support `GpuRenderPass` rendering, texture sampling, and CPU readback - but they
differ in whether they carry a 2D drawing context.

| | `GpuTarget` | `GpuCanvas` |
| --- | --- | --- |
| Render-pass rendering (`beginRenderPass`) | ✅ | ✅ |
| 2D `Graphics` drawing (`beginDraw`) | ❌ | ✅ |
| Dedicated render context | No (uses main context) | Yes |
| Cost | Minimal | Higher (allocates a 2D context) |
| Use for | Custom pipeline work, post-process | Mixed 2D drawing + custom passes |

**Rule of thumb:** if you only need custom `GpuPipeline` passes, use `GpuTarget`.
If you also need to draw 2D vector content onto the surface, use `GpuCanvas`.

## `GpuTarget`

The minimal offscreen render surface. It allocates a backing texture from the
context's main render context - no dedicated 2D context is created.

```cpp
static GpuTarget::Ptr GpuTarget::create (GraphicsContext& ctx, int width, int height);
```

```cpp
auto target = GpuTarget::create (ctx, 256, 256);
if (target != nullptr)
{
    auto frame = GpuFrame::begin (ctx);
    auto pass  = target->beginRenderPass (frame, { true, Colors::transparentBlack });
    pass.setPipeline (*pipeline);
    pass.draw (3);
    pass.finish();
    frame.submit();

    mainGraphics.drawTexture (target->asTexture(), targetBounds);
}
```

| Method                              | Description                                            |
| ----------------------------------- | ------------------------------------------------------ |
| `getWidth()` / `getHeight()`        | Target dimensions in pixels.                           |
| `beginRenderPass (frame, options)`  | Begins a render pass targeting the backing texture.    |
| `asTexture()`                       | GPU-texture view of the rendered result.               |
| `asImage()`                         | An `Image` with GPU texture + CPU pixels populated.    |
| `readPixels (dst, byteSize)`        | Reads pixels back to CPU memory.                       |

## `GpuCanvas`

Builds on a `GpuTarget` but is backed by a **dedicated render context**, adding
2D `Graphics` drawing via `beginDraw()` / `commit()`. It consolidates creation,
rendering, and readback of an offscreen surface into one object, replacing the
lower-level `GraphicsContext::createOffscreenTarget` / `beginOffscreen` /
`endOffscreen` API.

```cpp
static GpuCanvas::Ptr GpuCanvas::create (GraphicsContext& ctx, int width, int height);
```

### 2D drawing path

```cpp
auto canvas = GpuCanvas::create (ctx, 256, 256);
if (canvas != nullptr)
{
    auto& g = canvas->beginDraw();
    g.setFillColor (Colors::cornflowerblue);
    g.fillAll();

    // asTexture() auto-commits, so no explicit commit() is needed.
    mainGraphics.drawTexture (canvas->asTexture(), targetBounds);
}
```

`beginDraw()` opens (or reopens) a 2D frame and returns the `Graphics` to draw
into. On the first call it opens a fresh offscreen 2D GPU frame; subsequent calls
discard the previous frame's `Graphics` and reopen a new one on the same
already-allocated target, avoiding per-frame GPU resource reallocation.

### Custom-pass path

A canvas can also be the target of a `GpuRenderPass`, exactly like `GpuTarget`:

```cpp
auto pass = canvas->beginRenderPass (frame, { true, background });
pass.setPipeline (*pipeline);
pass.drawIndexed (indexCount);
pass.finish();
```

| Method                              | Description                                                       |
| ----------------------------------- | ----------------------------------------------------------------- |
| `getTarget()`                       | The underlying `GpuTarget` backing this canvas.                   |
| `getWidth()` / `getHeight()`        | Canvas dimensions in pixels.                                      |
| `beginRenderPass (frame, options)`  | Begins a render pass targeting the backing texture.               |
| `beginDraw()`                       | Opens/reopens a 2D frame; returns the `Graphics` to draw into.    |
| `commit()`                          | Finalizes an open 2D command. Usually unnecessary (auto-commits). |
| `asTexture()`                       | GPU-texture view; auto-commits an open 2D frame.                  |
| `asImage()`                         | `Image` with GPU texture + CPU pixels; auto-commits.              |
| `readPixels (dst, byteSize)`        | Reads pixels back to CPU memory; auto-commits.                    |

## CPU readback

Both surfaces can copy their rendered pixels back to CPU memory. The destination
buffer must hold at least `getWidth() * getHeight() * 4` bytes (RGBA,
top-to-bottom row order):

```cpp
std::vector<uint8_t> pixels ((size_t) canvas->getWidth() * canvas->getHeight() * 4);
if (canvas->readPixels (pixels.data(), pixels.size()))
{
    // pixels now holds RGBA rows, top to bottom
}
```

`readPixels()` returns `false` if readback is unavailable for the backend or
fails. For a ready-made image, `asImage()` wraps the texture and fills the
CPU-side pixel data in one call. For GPU-only compositing without readback,
prefer `asTexture()` + `Graphics::drawTexture`, which avoids the CPU round-trip.

```{note}
Readback flushes pending GPU work and can stall the CPU. Use it for
screenshots, tests, and export paths - not on the hot per-frame path.
```
