# Images

`Image` is YUP's CPU-and-GPU bitmap. It stores pixel data as `ImagePixelData`
and can optionally carry a GPU texture for hardware-accelerated drawing. This
page covers creating images, pixel access, loading and saving through the format
codecs, and the bridge to the GPU.

## Pixel formats

`PixelFormat` selects the raw byte layout of the pixel buffer:

- `Grayscale` — single channel.
- `RGB` — three channels, no alpha.
- `RGBA` — four channels (the default).

Regardless of storage format, the color values passed to `setPixel` /
`getPixel` are always packed as `0xAARRGGBB` (ARGB).

## Creating an image

```cpp
Image image { 256, 256, PixelFormat::RGBA };   // width, height, format
image.fillColor (Colors::cornflowerblue);       // fill with a Color
```

## Pixel access

```cpp
image.setPixelColor (10, 20, Colors::red);       // by Color
image.setPixel (10, 20, 0xffff0000);             // by ARGB uint32

Color c   = image.getPixelColor (10, 20);
uint32 argb = image.getPixel (10, 20);

image.clear();                                   // all pixels to zero
```

Query geometry and access raw bytes:

```cpp
int w        = image.getWidth();
int h        = image.getHeight();
auto fmt     = image.getPixelFormat();
int stride   = image.getPixelStride();

Span<const uint8> bytes = image.getRawData();    // read-only
Span<uint8> writable    = image.getRawData();    // mutable
```

`duplicate()` makes a deep CPU copy (it does *not* copy any GPU texture).

## Loading images

### From memory

The quickest path decodes an in-memory buffer directly:

```cpp
auto result = Image::loadFromData (encodedBytes);
if (result.wasOk())
{
    Image image = result.getValue();
    // use image...
}
else
{
    DBG (result.getErrorMessage());
}
```

### From a file via the format manager

`ImageFormatManager` is the registry that maps files and streams to the right
codec. Register the built-in formats once, then create a reader:

```cpp
ImageFormatManager manager;
manager.registerDefaultFormats();               // BMP, PPM, PNG, JPEG, WebP, GIF

if (auto reader = manager.createReaderFor (File ("/path/to/photo.png")))
{
    Image image = reader->readImage();
    // use image...
}
```

Available formats depend on compile-time configuration:

| Format   | Dependency         | Notes                          |
| -------- | ------------------ | ------------------------------ |
| BMP      | none               | Always available.              |
| PPM/PGM/PBM | none            | Always available.              |
| PNG      | libpng             | `YUP_IMAGE_FORMAT_PNG`         |
| JPEG     | libjpeg-turbo      | `YUP_IMAGE_FORMAT_JPEG`        |
| WebP     | libwebp            | `YUP_IMAGE_FORMAT_WEBP`        |
| GIF      | libgif             | `YUP_IMAGE_FORMAT_GIF`         |

Pass a subset to `registerDefaultFormats()` to limit which codecs load, e.g.
`registerDefaultFormats (ImageFormatType::png | ImageFormatType::jpeg)`.

### Detecting format from a stream

When you have a stream but no filename, `createReaderFor(InputStream*)` sniffs
the magic bytes. Ownership of the stream is always consumed by the call:

```cpp
if (auto reader = manager.createReaderFor (stream.release()))
{
    Image image = reader->readImage();
}
```

### Animated images

Animated formats (e.g. GIF) expose multiple frames through the reader:

```cpp
if (auto reader = manager.createReaderFor (File ("/path/to/anim.gif")))
{
    int frames = reader->getNumFrames();
    for (int i = 0; i < frames; ++i)
        Image frame = reader->readFrame (i);
}
```

## Saving images

Create a writer for the destination file. The extension selects the codec:

```cpp
ImageFormatManager manager;
manager.registerDefaultFormats();

if (auto writer = manager.createWriterFor (File ("/path/to/out.png"),
                                           PixelFormat::RGBA))
{
    writer->writeImage (image);
}
```

`createWriterFor` also accepts metadata and a quality index for compressed
formats:

```cpp
StringPairArray metadata;
metadata.set ("Author", "YUP");

if (auto writer = manager.createWriterFor (File ("/path/to/out.jpg"),
                                           PixelFormat::RGBA,
                                           metadata,
                                           /* qualityOptionIndex */ 0))
{
    writer->writeImage (image);
}
```

## Images on the GPU

An `Image` can hold a GPU texture alongside (or instead of) its CPU pixels — the
key to fast, allocation-free compositing.

### Upload CPU pixels to the GPU

```cpp
image.createTextureIfNotPresent (context);   // uploads if not already present
auto texture = image.getGpuTexture();         // GpuTexture::Ptr
```

Call `invalidateTexture()` after mutating CPU pixels to force re-upload on the
next request.

### Wrap an existing GPU texture

`Image::fromTexture` wraps a `GpuTexture::Ptr` (e.g. from
[`GpuCanvas::asTexture()`](rhi/targets.md)) with no CPU-side allocation. The
resulting image is drawable but has no readable CPU pixels — `getPixel` /
`getRawData` will assert.

```cpp
Image gpuImage = Image::fromTexture (canvas->asTexture());
g.drawImage (gpuImage, targetArea);
```

### Drawing images

Both CPU- and GPU-backed images draw through `Graphics`:

```cpp
g.drawImageAt (image, { x, y });       // natural size at a point
g.drawImage (image, targetArea);        // scaled into a rectangle
```

For a texture you already hold, `Graphics::drawTexture` skips the `Image`
wrapper entirely — see [Buffers & textures](rhi/buffers-and-textures.md#gputexture).

## See also

- [The Graphics class](graphics-class.md) — `drawImage`, `drawImageAt`,
  `drawTexture`, and offscreen rendering into an `Image`.
- [RHI: Offscreen targets & canvases](rhi/targets.md) — producing GPU textures.
- [SVG](svg.md) — SVG `<image>` elements resolve to `Image` via the parse
  options.
