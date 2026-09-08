# Buffers & Textures

## `GpuBuffer`

A reference-counted GPU buffer holding vertex, index, or uniform data. Buffers
are **immutable**: the data provided at creation is uploaded once and cannot be
changed afterward. For per-frame changing data (e.g. transforms), use a uniform
buffer set on the pass via `GpuRenderPass::setUniformBuffer()` instead.

```cpp
enum class GpuBufferType : uint8_t { vertex, index, uniform };

static GpuBuffer::Ptr GpuBuffer::create (GpuDevice::Ptr ctx,
                                         GpuBufferType    type,
                                         const void*      data,
                                         size_t           byteSize);
```

Creating and binding vertex + index buffers:

```cpp
auto vbo = GpuBuffer::create (ctx, GpuBufferType::vertex, vertices, sizeof vertices);
auto ibo = GpuBuffer::create (ctx, GpuBufferType::index,  indices,  sizeof indices);

if (vbo == nullptr || ibo == nullptr)
    return; // GPOU context unavailable or allocation failed

pass.setVertexBuffer (0, vbo);
pass.setIndexBuffer (GpuIndexFormat::uint16, ibo);
pass.drawIndexed ((uint32_t) std::size (indices));
```

| Method              | Description                                        |
| ------------------- | -------------------------------------------------- |
| `getType()`         | The buffer usage type.                             |
| `getSizeInBytes()`  | Size of the buffer in bytes.                       |
| `isValid()`         | True if the buffer holds a valid GPU resource.     |

## `GpuTexture`

An opaque, reference-counted GPU texture. It is the **currency** that connects
render-pass output to `Image` and `Graphics::drawTexture`. The underlying GPU
resource lives as long as at least one `GpuTexture::Ptr` exists.

Obtain one either by allocating it, or from an existing surface:

- `GpuTexture::create (device, desc)` - a new texture of any shape ore supports.
- `GpuCanvas::asTexture()` / `GpuTarget::asTexture()` - the rendered result.
- `Image::getGpuTexture()` - an image's backing texture.

| Method                       | Description                                                  |
| ---------------------------- | ------------------------------------------------------------ |
| `getWidth()`                 | Width of the base mip level in pixels.                       |
| `getHeight()`                | Height of the base mip level in pixels.                      |
| `getFormat()`                | The texel format.                                            |
| `getType()`                  | Storage shape: 2D, cube, 3D or 2D array.                     |
| `getMipLevels()`             | Number of allocated mip levels.                              |
| `getDepthOrArrayLayers()`    | Slice count (3D) or layer count (array / cube).              |
| `getSampleCount()`           | MSAA sample count, 1 when not multisampled.                  |
| `isValid()`                  | True if the texture holds valid GPU resources.               |
| `isRenderTarget()`           | True if it can be used as a render pass attachment.          |
| `upload (dataDesc)`          | Uploads CPU pixels into one mip level of one layer.          |

### Allocating a texture

`GpuTextureDesc` mirrors the backend descriptor one-to-one. Storage is allocated
for every mip level and layer, but nothing is written to it - fill the levels
with `upload()`, or by rendering into them via `GpuTarget::createFromTexture()`.

```cpp
GpuTextureDesc desc;
desc.width = desc.height = 128;
desc.depthOrArrayLayers  = 6;                          // a cube's six faces
desc.type                = GpuTextureType::cube;
desc.format              = GpuTextureFormat::rgba16float;
desc.mipLevels           = 5;
desc.renderTarget        = true;

auto cube = GpuTexture::create (device, desc);
```

Formats cover the full ore set: 8-bit `r/rg/rgba/bgra`, 16- and 32-bit float,
`rgb10a2unorm`, `r11g11b10float`, the depth/stencil formats, and the BC / ETC2 /
ASTC block formats. Availability is not uniform:

```cpp
// Block-compressed formats depend on the device.
if (! device->isFormatSupported (GpuTextureFormat::bc7unorm)) { ... }

// Float *render targets* are extension-gated on OpenGL ES / WebGL2.
auto hdrFormat = device->isFormatRenderable (GpuTextureFormat::rgba16float)
                   ? GpuTextureFormat::rgba16float
                   : GpuTextureFormat::rgba8unorm;
```

### Uploading pixels

```cpp
GpuTextureDataDesc data;
data.data     = pixels.data();
data.mipLevel = 0;
data.layer    = face;   // cube face, or array layer

texture->upload (data);
```

Zero `width` / `height` means "the whole mip level", and zero `bytesPerRow` means
tightly packed. Block-compressed formats have no bytes-per-texel, so they must
supply `bytesPerRow` explicitly. `upload()` is only valid on textures created by
`GpuTexture::create()`; textures wrapping rendered content return false.

## `GpuSampler`

Describes how a shader reads a texture. Every sampler binding a pipeline declares
gets a linear / clamp-to-edge sampler by default, so a `GpuSampler` is only
needed for slots wanting different filtering, wrapping, a mip filter, an explicit
LOD range or anisotropy.

```cpp
GpuSamplerDesc desc;
desc.minFilter = desc.magFilter = desc.mipmapFilter = GpuFilter::linear;
desc.wrapU = desc.wrapV = GpuWrapMode::repeat;
desc.maxLod = 4.0f;

auto sampler = GpuSampler::create (device, desc);
pass.setSampler (0, 1, sampler);
```

Set `GpuSamplerDesc::compare` to turn it into a comparison (shadow) sampler; a
binding declared as a comparison sampler requires it. `maxAnisotropy` above 1
requires `GpuDevice::isAnisotropicFilteringAvailable()`.

A LOD-clamped sampler is also the **portable** way to read one specific level of
a mip chain - see the warning under `GpuTarget::createFromTexture` in
[Offscreen Targets & Canvases](targets.md).

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
