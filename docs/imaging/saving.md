# Saving Images

Encoding uses the same `ImageFormatManager` registry as [loading](loading.md).
Create a writer for the destination - the file extension selects the codec -
then hand it an `Image`.

## To a file

```cpp
ImageFormatManager formats;
formats.registerDefaultFormats();

if (auto writer = formats.createWriterFor (File ("/path/out.png")))
    writer->writeImage (image);           // returns true on success
```

`createWriterFor` returns `nullptr` if no registered format handles the
extension, so always null-check it.

## Pixel format, metadata & quality

The full `createWriterFor` signature lets you choose the encoded pixel layout,
embed metadata, and pick a quality level for compressed formats:

```cpp
std::unique_ptr<ImageFormatWriter> createWriterFor (
    const File&            file,
    PixelFormat            pixelFormat       = PixelFormat::RGBA,
    const StringPairArray& metadataValues    = {},
    int                    qualityOptionIndex = 0);
```

```cpp
StringPairArray metadata;
metadata.set ("title", "Sunset");
metadata.set ("software", "YUP");

if (auto writer = formats.createWriterFor (File ("/path/out.jpg"),
                                           PixelFormat::RGB,
                                           metadata,
                                           /* qualityOptionIndex */ 2))
{
    writer->writeImage (image);
    writer->flush();   // usually optional; forces buffered data out
}
```

Standard metadata keys mirror the reader side: `"dpiX"`, `"dpiY"`,
`"colorSpace"`, `"title"`, `"software"`, `"comment"`.

### Quality options

Compressed formats expose named quality levels. Query them from the
`ImageFormat` and pass the chosen index to `createWriterFor`:

```cpp
// getQualityOptions() returns labels like {"Low", "Medium", "High"};
// isCompressed() is false for BMP/PPM and true for PNG/WebP/JPEG.
```

`qualityOptionIndex` is ignored by lossless/uncompressed formats.

### Supported pixel formats per codec

Before writing, a format advertises which pixel layouts it accepts via
`getPossiblePixelFormats()`. If you request a format the codec does not natively
support, the writer performs an internal conversion. Which codecs are available
depends on the linked modules - see the [available formats
table](loading.md#available-formats).

## To a stream

Formats can encode to any `OutputStream` - construct the writer at the
`ImageFormat` level when you need stream output rather than a file:

```cpp
// via a concrete/registered ImageFormat instance
if (auto writer = pngFormat.createWriterFor (outStream.release(),
                                             PixelFormat::RGBA,
                                             {}, 0))
{
    writer->writeImage (image);
}
```

The writer takes ownership of the destination stream.

## Animated GIF output

Only the GIF writer supports animation (`supportsAnimation()` returns true).
Bracket the frames between `beginAnimation()` and `endAnimation()`:

```cpp
if (auto writer = formats.createWriterFor (File ("/path/anim.gif")))
{
    if (writer->supportsAnimation())
    {
        writer->beginAnimation (/* loopCount */ 0);   // 0 = loop forever

        for (auto& [frame, delayMs] : frames)
            writer->writeFrame (frame, delayMs);        // RGBA expected

        writer->endAnimation();
    }
}
```

Each `writeFrame` quantizes the frame to a 256-color palette and appends a
Graphic Control Extension with the given delay. `loopCount` follows the reader
convention: `0` = infinite, `1` = play once, `N` = play N times. Calling the
animation methods on a non-animation writer asserts and returns `false`.

## See also

- [Loading images](loading.md) - the decode side.
- [Images & pixels](pixels.md) - preparing pixel data to save.
