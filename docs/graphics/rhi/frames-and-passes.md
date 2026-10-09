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
| `waitForGPU()`    | Submits if needed, waits for the frame's work, releases transient resources, returns a `Result`. |
| `getStatus()`     | The frame's GPU outcome, to check later without blocking.                |

`submit()` returns `false` if the frame is invalid or was already submitted. The
destructor submits the frame automatically if you have not already done so.

`waitForGPU()` blocks on Metal and Vulkan. On OpenGL, Direct3D 11 and WebGPU the
work is already ordered for any later readback, so it returns at once. It fails
when the frame could not be submitted or, on Metal, when the GPU could not execute
it (for instance when it ran out of memory), instead of letting you read back a
blank target.

To learn the outcome without blocking, keep `getStatus()` and `poll()` it later,
for instance before the next frame. A frame that was waited on or whose status
was taken reports its failure there only; the failures of other frames are
collected by `GpuDevice::takeGpuFailure()`. `SceneRenderer` checks its own frames
this way: its next `render()` starts the accumulation over, still renders, and
reports the failure through `getLastGpuFailure()`; `SceneRenderer::waitForGPU()`
waits for them.

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
| `setTexture (group, binding, texture, view)`       | Binds only the mip levels and layers `view` selects (see below).      |
| `setSampler (group, binding, sampler)`             | Overrides the pipeline's default sampler for a slot.                  |
| `setUniformBuffer (group, binding, data, size)`    | Copies uniform data to a slot immediately; last write wins.           |
| `setStorageBuffer (group, binding, buffer)`        | Binds a storage buffer for read-only shader access (see below).       |
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

### Sampling one mip level

To read one level of a texture while rendering into another level of the same
texture (a downsample chain or bloom), bind a narrowed view. The shader sees only
the selected levels, so lod 0 is the view's `baseMipLevel`:

```cpp
auto nextLevel = GpuTarget::createFromTexture (device, chain, { /* mip */ level + 1, /* layer */ 0 });

auto pass = nextLevel->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
pass.setPipeline (downsamplePipeline);
pass.setTexture (0, 0, chain, GpuTextureViewDesc (level, 0)); // samples mip `level` only
pass.draw (3);
pass.finish();
```

A view of a cube or array texture must also set `dimension` and `layerCount`. On
OpenGL and OpenGL ES the mip range is honored but a layer range is not, and one
texture should not be bound to two slots of the same draw with different mip
ranges: both slots would see the range applied last. Since OpenGL cannot honor
it, debug builds assert on every backend when a draw binds one texture to two
slots with different views, which keeps the code portable.

### Reading storage buffers

Vertex and fragment shaders can read a `GpuBufferType::storage` buffer, such as
one a compute pass filled. Declare the block `readonly`; a draw meeting a
writable block asserts, draws nothing and returns `false`:

```glsl
layout(std430, set = 0, binding = 0) readonly buffer Lights { vec4 colors[]; };
```

```cpp
if (device->isFragmentStorageBufferAvailable())
    pass.setStorageBuffer (0, 0, lightsBuffer);
```

The buffer stays alive until the GPU has finished with it, even if you drop it
right after the draw. Fragment storage buffers are currently available on Metal,
Direct3D 11, Vulkan and WebGPU. Elsewhere
`isFragmentStorageBufferAvailable()` returns false and `setStorageBuffer` is
ignored with an assertion; OpenGL, OpenGL ES and WebGL2 are raster-only and never
support them.

On WebGPU, availability follows the device's `maxStorageBuffersInFragmentStage`
limit. Core devices always provide them, while a compatibility mode device (see
`GpuDevice::isCompatibilityMode()`) may report none.

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
`setDepthStencilAttachment()` - or to `GpuTarget::beginRenderPass()` when the
target itself is a depth texture (see [depth-only targets](targets.md#depth-only-targets)).

```cpp
// Clear to a solid background:
auto pass = target->beginRenderPass (frame, { true, Colors::black });

// Draw over existing contents:
auto overlay = target->beginRenderPass (frame, { false, Colors::transparentBlack });
```
