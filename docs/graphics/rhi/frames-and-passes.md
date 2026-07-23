# Frames & Render Passes

## `GpuFrame`

`GpuFrame` is a move-only, stack-allocated RAII scope for a single frame's GPU
work. It wraps the GPU context begin → submit → wait lifecycle and owns the transient
GPU resources (uniform buffers, texture views, samplers) created while encoding
its passes.

```cpp
static GpuFrame GpuFrame::begin (GraphicsContext& ctx);
```

Begin a frame, encode one or more render passes into it, then submit:

```cpp
auto frame = GpuFrame::begin (ctx);
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

pass.setPipeline (*pipeline);
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
| `setUniformBuffer (group, binding, data, size)`    | Copies uniform data to a slot immediately; last write wins.           |
| `setVertexBuffer (slot, buffer)`                   | Binds a vertex buffer for custom geometry.                            |
| `setIndexBuffer (format, buffer)`                  | Binds an index buffer for `drawIndexed()`.                            |
| `draw (vertexCount)`                               | Non-indexed draw.                                                     |
| `drawIndexed (indexCount)`                         | Indexed draw using the bound vertex + index buffers.                  |
| `finish()`                                         | Encodes recorded draws and closes the pass. Idempotent.               |

### Fullscreen passes

For a fullscreen post-process that generates its vertices from the vertex index,
bind **no** vertex buffers and issue a three-vertex draw:

```cpp
pass.setPipeline (*blurPipeline);
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
    bool  clear      = true;                      // clear vs. load existing contents
    Color clearColor = Colors::transparentBlack;  // used when clear == true
};
```

- `clear = true` clears the target to `clearColor` before drawing (`LoadOp::clear`).
- `clear = false` preserves the existing contents (`LoadOp::load`) - useful when
  layering multiple passes onto the same target.

```cpp
// Clear to a solid background:
auto pass = target->beginRenderPass (frame, { true, Colors::cornflowerblue });

// Draw over existing contents:
auto overlay = target->beginRenderPass (frame, { false, Colors::transparentBlack });
```
