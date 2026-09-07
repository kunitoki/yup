# Frames & Render Passes

## `GpuFrame`

`GpuFrame` is a move-only, stack-allocated RAII scope for a single frame's GPU
work. It wraps the GPU context begin → submit → wait lifecycle and owns the transient
GPU resources (uniform buffers, texture views, samplers) created while encoding
its passes.

```cpp
static GpuFrame GpuFrame::begin (GpuDevice::Ptr device);
```

Begin a frame, encode one or more render passes into it, then submit:

```cpp
auto frame = GpuFrame::begin (device);
if (! frame.isValid())
    return; // No GPU context.

// ... encode passes into `frame` ...

frame.submit();
```

### Methods

| Method            | Description                                                              |
| ----------------- | ------------------------------------------------------------------------ |
| `isValid()`       | True if the frame holds a valid GPU context.                             |
| `submit()`        | Submits all passes recorded since `begin()`. Idempotent; does not block. |
| `waitForGPU()`    | Blocks until submitted work completes and releases transient resources.  |

`submit()` returns `false` if the frame is invalid or was already submitted. The
destructor submits the frame automatically if you have not already done so.

```{warning}
Do not submit a frame while one of its render passes is still open. Finish every
pass first (explicitly, or by letting it leave scope).
```

## `GpuRenderPass`

A `GpuRenderPass` records draw commands into one GPU render pass that outputs
to a target's backing texture. Obtain one from a target's `beginRenderPass()`:

```cpp
GpuRenderPass GpuCanvas::beginRenderPass (GpuFrame& frame, const GpuRenderOptions& options = {});
GpuRenderPass GpuTarget::beginRenderPass (GpuFrame& frame, const GpuRenderOptions& options = {});
```

Like `GpuFrame`, it is move-only stack RAII: the destructor finishes the pass if
`finish()` was not called.

### Binding and drawing

All mutable state lives on the pass, so a single immutable `GpuPipeline` can be
reused across many passes with different bindings.

```cpp
auto pass = canvas->beginRenderPass (frame, { true, background });
if (! pass.isValid())
    return;

pass.setPipeline (pipeline);
pass.setUniformBuffer (0, 0, &uniforms, sizeof uniforms);
pass.setTexture (0, 1, sceneTexture);
pass.setVertexBuffer (0, vertexBuffer);
pass.setIndexBuffer (GpuIndexFormat::uint16, indexBuffer);
pass.drawIndexed (indexCount);
pass.finish();
```

| Method                                             | Description                                                           |
| -------------------------------------------------- | --------------------------------------------------------------------- |
| `isValid()`                                        | True if the pass holds a valid encoding target.                       |
| `setPipeline (pipeline)`                           | Sets the compiled pipeline used by subsequent draws.                  |
| `setTexture (group, binding, texture)`             | Binds a texture to a `(group, binding)` slot; last write wins.        |
| `setSampler (group, binding, sampler)`             | Overrides the pipeline's default sampler for a slot.                  |
| `setUniformBuffer (group, binding, data, size)`    | Copies uniform data to a slot immediately; last write wins.           |
| `setVertexBuffer (slot, buffer)`                   | Binds a vertex buffer for custom geometry.                            |
| `setIndexBuffer (format, buffer)`                  | Binds an index buffer for `drawIndexed()`.                            |
| `setColorAttachment (index, texture, …)`           | Binds an extra colour attachment (MRT); index 1..3.                   |
| `setDepthStencilAttachment (texture, …)`           | Binds the depth/stencil attachment for the pass.                      |
| `setResolveTarget (index, texture, …)`             | MSAA resolve destination for a colour attachment.                     |
| `setViewport (x, y, w, h, …)`                      | Restricts rendering to a sub-rectangle; sticky across draws.          |
| `setScissorRect (x, y, w, h)`                      | Discards fragments outside the rectangle; sticky across draws.        |
| `setStencilReference (ref)`                        | Stencil test reference value; sticky across draws.                    |
| `setBlendColor (color)`                            | Constant for the `blendColor` blend factors; sticky across draws.     |
| `draw (vertexCount, instanceCount, …)`             | Non-indexed, optionally instanced draw.                               |
| `drawIndexed (indexCount, instanceCount, …)`       | Indexed, optionally instanced draw.                                   |
| `finish()`                                         | Encodes recorded draws and closes the pass. Idempotent.               |

Binding state is mutable between draws: a second `draw()` in the same pass sees
whatever pipeline, textures and uniforms were set most recently. The attachments
are cleared before the **first** draw only - every later draw loads them - so
several draws accumulate into one surface and share one depth buffer.

### Fullscreen passes

For a fullscreen post-process that generates its vertices from the vertex index,
bind **no** vertex buffers and issue a three-vertex draw:

```cpp
pass.setPipeline (blurPipeline);
pass.setTexture (0, 0, sourceTexture);
pass.setUniformBuffer (0, 1, &blurParams, sizeof blurParams);
pass.draw (3); // fullscreen triangle
pass.finish();
```

## `GpuRenderOptions`

Controls attachment load behavior for a pass:

```cpp
struct GpuRenderOptions
{
    GpuRenderOptions (bool clear, GpuColor clearColor);              // two-state form
    GpuRenderOptions (GpuLoadOp, GpuStoreOp, GpuColor clearColor);   // explicit form

    GpuLoadOp  loadOp     = GpuLoadOp::clear;       // clear / load / dontCare
    GpuStoreOp storeOp    = GpuStoreOp::store;      // store / discard
    GpuColor   clearColor = Colors::transparentBlack;
};
```

- `GpuLoadOp::clear` clears the target to `clearColor` before drawing.
- `GpuLoadOp::load` preserves the existing contents - useful when layering
  multiple passes onto the same target.
- `GpuLoadOp::dontCare` leaves the contents undefined; the cheapest option when
  the pass writes every pixel.

The `{ bool, GpuColor }` form is a shorthand for `clear` versus `load`.

Depth and stencil attachments have their own `GpuDepthStencilOptions` with
independent load / store ops and clear values, passed to
`setDepthStencilAttachment()`.

```cpp
// Clear to a solid background:
auto pass = target->beginRenderPass (frame, { true, Colors::black });

// Draw over existing contents:
auto overlay = target->beginRenderPass (frame, { false, Colors::transparentBlack });
```
