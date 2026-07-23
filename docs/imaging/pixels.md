# Images & Pixels

`Image` is YUP's bitmap type. It owns a reference-counted `ImagePixelData` buffer
on the CPU and can optionally carry a GPU texture for hardware-accelerated
drawing. This page covers the pixel model, creating images, and direct pixel
access.

## Pixel formats

`PixelFormat` selects the raw byte layout of the pixel buffer:

| Format             | Bytes/pixel | Layout                          |
| ------------------ | ----------- | ------------------------------- |
| `PixelFormat::Grayscale` | 1     | 8-bit luminance                 |
| `PixelFormat::RGB`       | 3     | red, green, blue byte order     |
| `PixelFormat::RGBA`      | 4     | red, green, blue, alpha (default) |

```{important}
Storage layout and color values are separate concerns. Whatever the storage
`PixelFormat`, the color values passed to and returned from `setPixel` /
`getPixel` are always packed as **ARGB** (`0xAARRGGBB`). The `Color` overloads
(`setPixelColor` / `getPixelColor`) hide the packing entirely.
```

## Creating an image

```cpp
Image image { 256, 256, PixelFormat::RGBA };  // width, height, format
image.fillColor (Colors::cornflowerblue);      // fill with a Color
```

`PixelFormat` defaults to `RGBA`, so `Image { w, h }` gives you a 32-bit image.
A default-constructed `Image {}` is empty; check `isValid()` before use.

Dimensions must be positive - constructing `ImagePixelData` with a non-positive
width or height throws `std::invalid_argument`.

## Geometry & format queries

```cpp
int  w      = image.getWidth();
int  h      = image.getHeight();
auto fmt    = image.getPixelFormat();
int  stride = image.getPixelStride();   // bytes per pixel (1 / 3 / 4)
bool ok     = image.isValid();
```

## Reading & writing pixels

```cpp
// Write
image.setPixelColor (10, 20, Colors::red);   // by Color
image.setPixel (10, 20, 0xffff0000);          // by ARGB uint32

// Read
Color  c    = image.getPixelColor (10, 20);
uint32 argb = image.getPixel (10, 20);

// Whole-image operations
image.fillColor (Colors::black);              // by Color
image.fill (0xff000000);                       // by ARGB uint32
image.clear();                                 // all bytes to zero
```

```{note}
Pixel coordinates are bounds-checked and throw `std::out_of_range` if outside
the image. GPU-only images (see [drawing](drawing.md#wrap-an-existing-gpu-texture))
have no CPU pixels and will assert on pixel access.
```

## Raw buffer access

For bulk processing, get a `Span` over the raw bytes. Rows are laid out top to
bottom with `getPixelStride()` bytes per pixel:

```cpp
Span<const uint8> bytes = image.getRawData();  // read-only
Span<uint8> writable    = image.getRawData();  // mutable

// Direct access to the underlying pixel data object
ImagePixelData& data = image.getPixelData();
size_t rowBytes = (size_t) data.getWidth() * data.getPixelFormat() ... // via stride
```

`ImagePixelData` is move-only and reference-counted; an `Image` shares it via a
`Ptr`. To get an independent CPU copy, use `duplicate()`:

```cpp
Image copy = image.duplicate();  // deep CPU copy (does NOT copy any GPU texture)
```

## Next steps

- [Loading images](loading.md) - get pixels from files, memory, and streams.
- [Saving images](saving.md) - encode pixels to disk.
- [Drawing images](drawing.md) - paint an image and use it on the GPU.
