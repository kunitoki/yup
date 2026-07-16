# Buffers & Textures

## `GpuBuffer`

A reference-counted GPU buffer holding vertex, index, or uniform data. Buffers
are **immutable**: the data provided at creation is uploaded once and cannot be
changed afterward. For per-frame changing data (e.g. transforms), use a uniform
buffer set on the pass via `GpuRenderPass::setUniformBuffer()` instead.

```cpp
enum class GpuBufferType : uint8_t { vertex, index, uniform };

static GpuBuffer::Ptr GpuBuffer::create (GraphicsContext& ctx,
                                         GpuBufferType    type,
                                         const void*      data,
                                         size_t           byteSize);
```

Creating and binding vertex + index buffers:

```cpp
auto vbo = GpuBuffer::create (ctx, GpuBufferType::vertex, vertices, sizeof vertices);
auto ibo = GpuBuffer::create (ctx, GpuBufferType::index,  indices,  sizeof indices);

if (vbo == nullptr || ibo == nullptr)
    return; // ore unavailable or allocation failed

pass.setVertexBuffer (0, vbo);
pass.setIndexBuffer (GpuIndexFormat::uint16, ibo);
pass.drawIndexed ((uint32_t) std::size (indices));
```

| Method              | Description                                        |
| ------------------- | -------------------------------------------------- |
| `getType()`         | The buffer usage type.                             |
| `getSizeInBytes()`  | Size of the buffer in bytes.                       |
| `isValid()`         | True if the buffer holds a valid GPU resource.     |

```{note}
`create()` requires `enableOreContext = true` and returns `nullptr` on failure
(ore unavailable or allocation failed). Always null-check the result.
```

## `GpuTexture`

An opaque, reference-counted GPU texture. It is the **currency** that connects
render-pass output to `Image` and `Graphics::drawTexture`. The underlying GPU
resource lives as long as at least one `GpuTexture::Ptr` exists.

You do not construct textures directly. Obtain one from:

- `GpuCanvas::asTexture()` / `GpuTarget::asTexture()` - the rendered result.
- `Image::getGpuTexture()` - an image's backing texture.

| Method               | Description                                                  |
| -------------------- | ------------------------------------------------------------ |
| `getWidth()`         | Texture width in pixels.                                     |
| `getHeight()`        | Texture height in pixels.                                    |
| `isValid()`          | True if the texture holds valid GPU resources.               |
| `isRenderTarget()`   | True if produced by a render pass (e.g. a `GpuCanvas`).      |

### Sampling and compositing

Bind a texture into a pass as a shader input, or composite it back into the 2D
`Graphics` pipeline:

```cpp
// As a shader input in a custom pass:
pass.setTexture (0, 0, sceneCanvas->asTexture());

// Or composited via the 2D API:
mainGraphics.drawTexture (target->asTexture(), targetBounds);
```

Textures flow naturally between the RHI and the 2D stack: render offscreen with
a `GpuTarget` / `GpuCanvas`, take `asTexture()`, then either sample it in another
render pass or draw it with `Graphics::drawTexture`.
