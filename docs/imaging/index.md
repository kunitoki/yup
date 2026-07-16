# Imaging

Everything about bitmap images in YUP: the `Image` container and its pixel
storage, loading and saving through the pluggable format codecs, and drawing
images (CPU- or GPU-backed) into a `Graphics` context.

**Module covered:** `yup_graphics` (the `imaging/` and `formats/` groups).

## In this area

- [Images & pixels](pixels.md) - the `Image` and `ImagePixelData` types,
  `PixelFormat`, creating images, and reading/writing individual pixels.
- [Loading images](loading.md) - decode from memory, files, and streams;
  `ImageFormatManager`, readers, metadata, and animated frames.
- [Saving images](saving.md) - encode to files and streams; writers, pixel
  formats, metadata, quality options, and animated GIF output.
- [Drawing images](drawing.md) - `drawImage`, `drawImageAt`, `drawTexture`, and
  the CPU↔GPU texture bridge.

## Quick example

```cpp
// Load
ImageFormatManager formats;
formats.registerDefaultFormats();

Image image;
if (auto reader = formats.createReaderFor (File ("/path/photo.png")))
    image = reader->readImage();

// Draw (inside a Component::paint)
g.drawImage (image, getLocalBounds().to<float>());

// Save
if (auto writer = formats.createWriterFor (File ("/path/out.jpg")))
    writer->writeImage (image);
```

## See also

- [The Graphics class](../graphics/graphics-class.md) - the drawing context.
- [Drawables & SVG](../graphics/svg.md) - SVG `<image>` hrefs resolve to `Image`.
- [RHI: Offscreen targets & canvases](../graphics/rhi/targets.md) - producing GPU
  textures that can back an `Image`.

```{toctree}
:hidden:
:maxdepth: 1

pixels
loading
saving
drawing
```
