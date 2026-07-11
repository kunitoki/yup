# Drawing Images

Once you have an `Image`, drawing it is a single `Graphics` call. Images can be
CPU-backed, GPU-backed, or both - YUP uploads and caches a GPU texture as needed.

## Basic drawing

Inside a `Component::paint` (or any `Graphics` scope):

```cpp
g.drawImageAt (image, { x, y });        // natural size, top-left at the point
g.drawImage (image, targetArea);         // scaled to fill a rectangle
```

`drawImage` stretches the image to the target `Rectangle<float>`. To preserve
aspect ratio, compute the target rectangle yourself (see the
[Rectangle](../graphics/primitives.md#rectangle) helpers) or use a
[`Drawable`](../graphics/svg.md) for vector content.

## Images on the GPU

An `Image` can carry a GPU texture alongside (or instead of) its CPU pixels.
This is what makes repeated drawing allocation-free and fast.

### Upload CPU pixels to the GPU

```cpp
image.createTextureIfNotPresent (context);   // uploads once if not present
auto texture = image.getGpuTexture();         // GpuTexture::Ptr (may be null)
```

Call `invalidateTexture()` after mutating the CPU pixels so the next request
re-uploads:

```cpp
image.setPixelColor (0, 0, Colors::red);
image.invalidateTexture();                     // GPU copy is now stale
```

You can also assign a texture directly with `setGpuTexture (tex)`.

### Wrap an existing GPU texture

`Image::fromTexture` wraps a `GpuTexture::Ptr` - for example the result of
[`GpuCanvas::asTexture()`](../graphics/rhi/targets.md) - with **no** CPU-side
allocation. The image is drawable but has no readable CPU pixels, so
`getPixel` / `getRawData` will assert:

```cpp
Image gpuImage = Image::fromTexture (canvas->asTexture());
g.drawImage (gpuImage, targetArea);            // composited on the GPU
```

Returns an invalid `Image` if the texture is null or invalid.

### Draw a texture without an Image

When you already hold a `GpuTexture::Ptr`, skip the `Image` wrapper entirely and
draw it straight into the 2D pipeline - the fastest path for GPU-generated
content:

```cpp
g.drawTexture (canvas->asTexture(), targetArea);
```

See [RHI: Buffers & textures](../graphics/rhi/buffers-and-textures.md#gputexture)
for where `GpuTexture` values come from.

## Rendering into an image (offscreen)

You can also go the other direction: render 2D content into an `Image` on the
GPU and read it back on the CPU - useful for thumbnails, export, and tests.

```cpp
Image target { 512, 512, PixelFormat::RGBA };
{
    Graphics g { context, target };   // offscreen frame begins immediately
    g.setFillColor (Colors::cornflowerblue);
    g.fillAll();
    g.setFillColor (Colors::white);
    g.fillEllipse (Rectangle<float> { 64.0f, 64.0f, 384.0f, 384.0f });
    g.readPixelsToImage();             // fills target's CPU pixel buffer
}
// target now holds both a GPU texture and readable CPU pixels
```

Related `Graphics` methods: `isOffscreen()`, `commitOffscreenTarget()`,
`commitToImage()` (sets the rendered GPU texture on the target `Image` for later
`drawImage` without a CPU round-trip), and `readPixelsToImage()`.

For full custom GPU pipelines and render passes, step down to the
[RHI](../graphics/rhi/index.md).

## See also

- [The Graphics class](../graphics/graphics-class.md) - `drawImage`,
  `drawImageAt`, `drawTexture`, and offscreen rendering.
- [Images & pixels](pixels.md) · [Loading](loading.md) · [Saving](saving.md)
- [RHI: Offscreen targets & canvases](../graphics/rhi/targets.md)
