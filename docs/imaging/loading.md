# Loading Images

YUP decodes images through a small, pluggable codec layer. There are two entry
points: a one-shot decode of an in-memory buffer, and the `ImageFormatManager`
registry for files and streams (with metadata and multi-frame support).

## From a memory buffer

The quickest path decodes encoded bytes (PNG, JPEG, …) directly. It returns a
`ResultValue<Image>` so you can inspect the error on failure:

```cpp
auto result = Image::loadFromData (encodedBytes);   // Span<const uint8>
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

Pass `ImageFormat::Options` as a second argument to extract metadata:

```cpp
auto opts = ImageFormat::Options()
               .withMetadata (true)
               .withRawChunks (true);
auto result = Image::loadFromData (encodedBytes, opts);
```

## The ImageFormatManager

`ImageFormatManager` is the registry that maps files and streams to the right
codec. Register the built-in formats once, then create readers:

```cpp
ImageFormatManager formats;
formats.registerDefaultFormats();   // all built-ins
```

Register a subset with the `ImageFormatType` bitmask when you want to limit
which codecs are linked/used:

```cpp
formats.registerDefaultFormats (ImageFormatType::png | ImageFormatType::jpeg);
```

You can also register your own [`ImageFormat`](#custom-formats) implementation
with `registerFormat (std::move (myFormat))`.

### Available formats

Support for each compressed format is enabled simply by **linking its
third-party module** into your target (see [Modules](../modules.md) and the
[CMake API](../build-system/cmake-api.md)). There is no build flag to set - YUP
detects the linked library and turns the format on automatically. BMP and
PPM/PGM/PBM have no dependency and are always available.

| Format      | `ImageFormatType` | Module to link |
| ----------- | ----------------- | -------------- |
| BMP         | `bmp`             | *(built in)*   |
| PPM/PGM/PBM | `ppm`             | *(built in)*   |
| TGA         | `tga`             | *(built in)*   |
| PNG         | `png`             | `libpng`       |
| JPEG        | `jpeg`            | `libjpeg`      |
| WebP        | `webp`            | `libwebp`      |
| GIF         | `gif`             | `libgif`       |
| TIFF        | `tiff`            | `libtiff`      |

```cmake
yup_standalone_app(
    # ...
    MODULES
        yup::yup_graphics
        libpng      # enables PNG
        libjpeg     # enables JPEG
        libwebp)    # enables WebP
```

```{note}
The corresponding `YUP_IMAGE_FORMAT_PNG` / `_JPEG` / `_WEBP` / `_GIF` config
macros default to on and follow the linked libraries automatically. You only
need to touch them in the rare case where a library is linked (for some other
reason) but you want to *exclude* its image format - set the macro to `0`.
```


## From a file

`createReaderFor(File)` picks the codec by file extension:

```cpp
if (auto reader = formats.createReaderFor (File ("/path/photo.png")))
{
    Image image = reader->readImage();   // invalid Image on failure
}
```

## From a stream (format auto-detection)

When you have a stream but no filename, `createReaderFor(InputStream*)` sniffs
the magic bytes of each registered format. The stream must be seekable.
Ownership of the stream is **always** consumed by the call - on success the
reader owns it; on failure it is deleted:

```cpp
std::unique_ptr<InputStream> stream = someFile.createInputStream();
if (auto reader = formats.createReaderFor (stream.release()))
{
    Image image = reader->readImage();
}
```

## Reader properties & metadata

After a reader is created, its header-derived properties are populated. These
are plain public fields on `ImageFormatReader`:

```cpp
int    w   = reader->width;
int    h   = reader->height;
auto   fmt = reader->pixelFormat;
```

By default metadata extraction is **disabled** — the `metadata` field is
`nullptr`. To opt in, pass `ImageFormat::Options` when creating the reader:

```cpp
// Request text metadata and DPI
auto reader = formats.createReaderFor (stream.release(),
                                       ImageFormat::Options().withMetadata (true));

if (auto metadata = reader->metadata)
{
    double dx = metadata->dpiX;   // 0.0 if not present
    double dy = metadata->dpiY;

    // Arbitrary key/value pairs from the file
    // Standard keys: "Title", "Software", "Comment", "dpiX", "dpiY"
    String title = metadata->textEntries.getValue ("Title", {});

    // Raw binary chunks (EXIF, ICC profile, XMP packet)
    if (auto* exif = metadata->getRawChunk ("jpeg/exif"))
        processExif (exif->getData(), exif->getSize());
}
```

For `Image::loadFromData()`, pass options as the second argument:

```cpp
auto result = Image::loadFromData (bytes,
                                   ImageFormat::Options().withRawChunks (true));
```

`getFormatName()` returns the human-readable format label (e.g. `"PNG Image"`).

## Animated images

Formats that support animation — GIF, WebP, and PNG (APNG) — expose multiple
frames through the same reader API. Non-animated readers report a single frame,
so the same code works for both:

```cpp
if (auto reader = formats.createReaderFor (File ("/path/anim.gif")))
{
    if (reader->isAnimated())
    {
        int frames = reader->getFrameCount();
        int loops  = reader->getLoopCount();   // 0 = infinite, 1 = once, N = N times

        for (int i = 0; i < frames; ++i)
        {
            Image frame = reader->readFrame (i);
            int   delay = reader->getFrameDelayMs (i);  // display duration
        }
    }
    else
    {
        Image image = reader->readImage();
    }
}
```

### Zero-allocation frame decoding

For playback loops, decode into an existing `Image` to reuse its buffer. If the
destination already has the right width, height, and `PixelFormat::RGBA`, no
allocation occurs:

```cpp
Image frame { reader->width, reader->height, PixelFormat::RGBA };
for (int i = 0; i < reader->getFrameCount(); ++i)
{
    if (reader->readFrame (i, frame))   // writes into frame's buffer
        present (frame);
}
```

## Custom formats

To support a format the built-ins don't cover, subclass `ImageFormat` and
register it. Implement the format identity, detection, and reader/writer
factories:

- `getFormatName()`, `getFileExtensions(Mode)`
- `canHandleFile(file, mode)` and/or `canHandleStream(stream, mode)` (magic-byte
  detection - read the header, seek back to 0, return the match)
- `createReaderFor(InputStream*)` / `createWriterFor(...)`
- `getPossiblePixelFormats()`, `isCompressed()`, `getQualityOptions()`

```cpp
formats.registerFormat (std::make_unique<MyImageFormat>());
```

## See also

- [Saving images](saving.md) - the encode side of the same codec layer.
- [Images & pixels](pixels.md) - what a decoded `Image` contains.
- [Drawing images](drawing.md) - display a loaded image.
