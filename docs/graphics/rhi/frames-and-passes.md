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

| Method               | Description                                                              |
| -------------------- | ------------------------------------------------------------------------ |
| `isValid()`          | True if the frame holds a valid GPU context.                             |
| `submit()`           | Submits all passes recorded since `begin()`. Idempotent; does not block. |
| `waitForGPU()`       | Submits if needed, waits for the frame's work, releases transient resources, returns a `Result`. |
| `getStatus()`        | The frame's GPU outcome, to check later without blocking.                |
| `setTimingEnabled()` | Measures the frame's GPU time (off by default). Call before `submit()`.  |
| `beginScope()`       | Opens a named scope whose passes are timed together; ends the open one.  |
| `endScope()`         | Ends the open scope.                                                     |

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

### Frame timing

A frame can measure how long the GPU spent executing it. Turn it on with
`setTimingEnabled (true)` before submitting, keep the status, and read
`getTimings()` once the frame completed:

```cpp
auto frame = GpuFrame::begin (device);
frame.setTimingEnabled (true);
const auto status = frame.getStatus();

// ... encode passes into `frame` ...

frame.submit();

// Later, for instance before the next frame
if (const auto timings = status.getTimings(); timings.available)
    YUP_DBG ("GPU frame time: " << timings.frameMilliseconds << " ms");
```

`frameMilliseconds` is the time from the GPU's start of the frame's command buffer
to its end. It does not include GPU work submitted outside the frame (texture
uploads, the window's own drawing), nor the time before the GPU started it. With
several frames in flight, though, it includes the time the frame ran alongside, or
waited behind, the frames before it, even with two in flight: wait for each measured
frame to read its own cost. A window presenting to the display keeps about three in
flight, so the Scene 3D demo's own readout reads higher than `just perf`.
`getTimings()` never blocks: the timings are unavailable while the frame is pending,
when it failed, and when it was not measured.

Timing is off by default, so frames that do not measure pay nothing. Only Metal
measures for now (`GpuDevice::isFrameTimingAvailable()`); on every other backend
the timings stay unavailable.

### Scope timing

A measured frame can also report the GPU time of named scopes. Turn timing on
**before the frame's first pass**, then open a scope before the passes it covers:

```cpp
auto frame = GpuFrame::begin (device);
frame.setTimingEnabled (true);
const auto status = frame.getStatus();

frame.beginScope ("shadows");
renderShadows (frame);

frame.beginScope ("lighting"); // ends "shadows"
renderLighting (frame);
frame.endScope();

frame.submit();

// Later
for (const auto& scope : status.getTimings().scopes)
    if (scope.available)
        YUP_DBG (scope.name << ": " << scope.milliseconds << " ms");
```

- `scopes` lists the scopes in the order they were opened. A name opened twice
  appears twice.
- Scopes do not nest: opening one ends the scope already open.
- A pass belongs to the scope that was open when it began. Its time is summed
  into that scope, including every pass a compute pass split in two.
- Passes outside any scope are not attributed to a scope.
- A scope is measured only if timing was on before its first pass began. A scope
  whose passes began earlier reports `available == false` and 0 ms. So does a scope
  the frame had no timestamp samples left for (about 1000 passes); the frame total
  stays valid.
- On a tile-based GPU a pass starts while the passes before it still shade, those
  of the previous frames included. Each pass is credited only the time past the end
  of the work before it, so the overlapped work counts for the earlier pass.
  - A scope's milliseconds rank what lengthens the frame, not a pass's own cost:
    work that ran entirely under an earlier pass reads 0. To learn a pass's own
    cost, compare the frame total with and without it.
  - The scopes need not add up to `frameMilliseconds`.
  - The first scope of a frame may include the time it overlapped GPU work of the
    device that is not a `GpuFrame`, such as the window's drawing and uploads.
- Only the passes of `GpuRenderPass` and `GpuComputePass` are timed. Rive drawing,
  and the passes of scripted canvases, are not part of the scope open meanwhile.
- A device keeps up to 8 frames' worth of timestamp samples in flight, because Metal
  allows 32 per process. A frame queued beyond that measures its total but not its
  scopes, so measure with at most 8 frames in flight. A device keeps the buffers it
  allocated until it is destroyed.
- A frame that measures only its total, on a device that measures scopes, still
  samples its passes, so later scopes are attributed correctly.

Only Metal measures scopes (`GpuDevice::isScopeTimingAvailable()`), on GPUs that
sample timestamps at stage boundaries. Elsewhere `scopes` is empty, and a frame
that does not measure only records the scope names.

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

### Per-draw uniforms

`setUniformBuffer` copies the bytes, so the caller's block can change right
after the call. A block the pipeline lists in
`GpuPipelineOptions::dynamicUniformBuffers` is written into the frame's uniform
buffer, which every pass of the frame shares: each block takes 256 bytes, or
its size rounded up to 256 when larger, in 64 KB chunks taken from the device's
pool, and a chunk returns to the pool once the frame retires. The draw binds its set at the blocks' offsets, so the
set is not rebuilt when only those bytes change. Setting identical bytes again
keeps the block where it is. See
[Dynamic uniform buffers](pipelines.md#dynamic-uniform-buffers).

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
