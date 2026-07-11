# Drawables & SVG

`Drawable` parses and paints SVG content into a `Graphics` context. It is the
high-level entry point for vector artwork; the underlying parsed model is an
`SVGDocument`, and fitting/positioning reuses the shared `Fitting` and
`Justification` enums.

## Drawable

A `Drawable` holds a parsed SVG document and knows how to render it. Parse once,
paint many times.

### Parsing SVG

From a file or from an in-memory string:

```cpp
Drawable drawable;

if (drawable.parseSVG (File ("/path/to/icon.svg")))
{
    // ready to paint
}

// or from text
drawable.parseSVG (R"(<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/></svg>)");
```

`parseSVG` returns `true` on success. Call `clear()` to reset the drawable.

### Painting

The simplest form paints at the drawable's natural bounds using the current
`Graphics` transform:

```cpp
void MyComponent::paint (Graphics& g) override
{
    drawable.paint (g);
}
```

The fitted form scales and positions the artwork within a target rectangle:

```cpp
drawable.paint (g,
                getLocalBounds().to<float>(),
                Fitting::scaleToFit,
                Justification::center);
```

### Fitting modes

`Fitting` controls how the artwork is scaled into the target area:

| Mode            | Behavior                                                        |
| --------------- | --------------------------------------------------------------- |
| `none`          | No scaling.                                                      |
| `scaleToFit`    | Scale proportionally to fit inside (no cropping). **Default.**   |
| `fitWidth`      | Match width, preserve aspect ratio.                             |
| `fitHeight`     | Match height, preserve aspect ratio.                            |
| `scaleToFill`   | Scale proportionally to fill (may crop).                        |
| `fill`          | Stretch to fill (aspect ratio not preserved).                   |
| `tile`          | Repeat to fill.                                                 |
| `centerCrop`    | Like `scaleToFill`, keeping the center visible.                 |
| `centerInside`  | Like `scaleToFit`, but never upscales beyond original size.     |
| `stretchWidth`  | Stretch horizontally, preserve vertical size.                   |
| `stretchHeight` | Stretch vertically, preserve horizontal size.                   |

### Querying bounds

```cpp
Rectangle<float> bounds = drawable.getBounds();
```

Use the bounds to size a component or to compute an aspect-correct target area.

## SVGDocument & parse options

`Drawable::parseSVG` accepts a `ParseOptions` struct (aliased as
`Drawable::ParseOptions`) that controls how external references - images and
fonts - are resolved during parsing.

```cpp
Drawable::ParseOptions options;
options.baseDirectory    = File ("/assets/icons");  // resolves relative hrefs
options.allowDataImages  = true;                     // permit data: URIs
options.allowLocalImages = true;                    // permit local file hrefs

drawable.parseSVG (svgFile, options);
```

### Custom image resolver

Supply your own loader to intercept `<image>` hrefs - useful for virtual file
systems or embedded resources. Return `std::nullopt` to fall back to the default:

```cpp
options.imageResolver = [] (StringRef href, const File& baseDir) -> std::optional<Image>
{
    if (auto bytes = loadEmbeddedAsset (href))
        return Image::loadFromData (*bytes).getValue();

    return std::nullopt; // use default resolution
};
```

### Custom font resolver

SVG `<text>` elements resolve fonts through a resolver keyed by CSS font
properties. Return `std::nullopt` to use the default font lookup:

```cpp
options.fontResolver = [] (StringRef family, float size, int weight, bool italic)
    -> std::optional<Font>
{
    if (family == "Brand")
        return brandFont.withHeight (size);

    return std::nullopt;
};
```

`weight` follows CSS conventions (400 = normal, 700 = bold); `italic` is true for
italic or oblique styles.

## Supported SVG features

The parser handles a broad subset of SVG, including:

- Shapes and paths, with fills, strokes, dash arrays, and markers.
- Linear and radial gradients (`SVGGradient`).
- Clip paths (`SVGClipPath`) and masks (`SVGMask`).
- Patterns (`SVGPattern`) for tiled fills.
- Filters (`SVGFilter`).
- Text elements (with the font resolver above).
- Embedded and referenced images (with the image resolver above).
- CSS styling via `<style>` rules (`SVGCssRule`).
- `viewBox` with `preserveAspectRatio` fitting and justification.

```{note}
For direct access to the parsed model - elements, gradients, and bounds - an
`SVGDocument` exposes its `SVGData` through `visit()`. Most applications only
need `Drawable`; reach for `SVGDocument` when building custom SVG tooling.
```

## See also

- [The Graphics class](graphics-class.md) - the context `Drawable` paints into.
- [Images](../imaging/index.md) - how SVG `<image>` hrefs become `Image` objects.
- [Fonts & text layout](fonts.md) - how SVG `<text>` resolves fonts.
