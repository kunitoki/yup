# The Graphics Class

`Graphics` is the high-level, immediate-mode 2D drawing interface. It renders
fills, strokes, paths, text, images, and GPU textures into a rendering surface
through the active [`GraphicsContext`](#the-graphicscontext). You rarely
construct one yourself — the most common way to obtain a `Graphics` is inside
[`Component::paint`](../ui/index.md).

```cpp
void MyComponent::paint (Graphics& g) override
{
    g.setFillColor (Colors::cornflowerblue);
    g.fillAll();
}
```

## The state model

`Graphics` is a stateful drawing context. Drawing calls use the *current* state:
fill color / gradient, stroke color / gradient, stroke type, blend mode,
opacity, transform, drawing area, and clip path. You set state, then draw.

```cpp
g.setFillColor (Colors::white);   // set state
g.fillRect (bounds);              // draw with current state
g.setStrokeColor (Colors::black); // change state
g.setStrokeWidth (2.0f);
g.strokeRect (bounds);            // draw with new state
```

### Saving and restoring state

Use `saveState()` to snapshot the full state and restore it automatically. It
returns a `SavedState` RAII guard: when it leaves scope (or you call
`restore()`), the previous state is reinstated. This is the safe way to apply a
temporary transform, clip, or color.

```cpp
{
    auto saved = g.saveState();

    g.setTransform (AffineTransform::rotation (angle, cx, cy));
    g.setClipPath (clipBounds);
    g.setFillColor (Colors::red);
    g.fillPath (needle);
} // state restored here
```

## Colors, gradients, and strokes

Fills and strokes each have an independent color *or* gradient. Setting a color
switches that channel to solid; setting a gradient switches it to the gradient.

```cpp
g.setFillColor (Colors::orange);              // solid fill
g.setFillColorGradient (myGradient);          // gradient fill

g.setStrokeColor (Colors::black);             // solid stroke
g.setStrokeWidth (3.0f);                      // convenience for stroke width
g.setStrokeType ({ 3.0f, StrokeJoin::Round, StrokeCap::Round });
g.setStrokeJoin (StrokeJoin::Bevel);
g.setStrokeCap (StrokeCap::Square);
g.setStrokeMiterLimit (4.0f);
```

See [Primitives](primitives.md#stroke-types) for the stroke types and
[gradients](primitives.md#colorgradient).

## Opacity, blending, and feather

```cpp
g.setOpacity (0.5f);                  // 0..1, applies to subsequent drawing
g.setBlendMode (BlendMode::Multiply); // compositing mode
g.setFeather (2.0f);                  // soft edge falloff
```

## Transform, drawing area, and clip

- **Transform** — an `AffineTransform` applied to all subsequent geometry.
  `setTransform` replaces it; `addTransform` composes on top of the current one.
- **Drawing area** — the rectangle drawing is offset into and clipped against
  (`setDrawingArea` / `getDrawingArea`). `fillAll()` fills this area.
- **Clip path** — a rectangle or `Path` that constrains drawing
  (`setClipPath` / `getClipPath`).

```cpp
g.addTransform (AffineTransform::translation (10.0f, 10.0f));
g.setClipPath (contentBounds);
```

## Drawing operations

### Fills and strokes

Every shape has a `fill*` (solid interior) and `stroke*` (outline) form. Stroke
width and style come from the current stroke state.

| Shape          | Fill                                        | Stroke                                        |
| -------------- | ------------------------------------------- | --------------------------------------------- |
| Whole area     | `fillAll()`                                 | —                                             |
| Line           | —                                           | `strokeLine (x1, y1, x2, y2)`                 |
| Rectangle      | `fillRect (r)`                              | `strokeRect (r)`                              |
| Rounded rect   | `fillRoundedRect (r, radius)`               | `strokeRoundedRect (r, radius)`               |
| Ellipse        | `fillEllipse (r)`                           | `strokeEllipse (r)`                           |
| Path           | `fillPath (path)`                           | `strokePath (path)`                           |

Rounded rectangles accept either a uniform radius or four per-corner radii
(top-left, top-right, bottom-left, bottom-right).

```cpp
g.setFillColor (Colors::darkslategray);
g.fillRoundedRect (bounds, 8.0f);

g.setStrokeColor (Colors::white);
g.setStrokeWidth (1.5f);
g.strokeRoundedRect (bounds, 8.0f);
```

### Text

Text is drawn *fitted* into a rectangle with a `Font` and `Justification`. Fonts
come from the `ApplicationTheme`, not constructed inline.

```cpp
auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();

g.setFillColor (Colors::white);
g.fillFittedText ("Hello YUP", font, textBounds, Justification::center);
```

`StyledText` overloads (`fillFittedText (const StyledText&, ...)`) draw
pre-laid-out attributed text. There are `strokeFittedText` variants for outlined
text.

### Images and GPU textures

```cpp
g.drawImageAt (image, { x, y });          // draw at a point, natural size
g.drawImage (image, targetRect);          // scale into a rectangle
g.drawTexture (gpuTexture, targetRect);   // draw a GPU texture directly
```

`drawTexture` composites a [`GpuTexture`](rhi/buffers-and-textures.md#gputexture)
straight from the RHI (e.g. `GpuCanvas::asTexture()`) without allocating a
CPU-side image — the fast path for GPU-generated content.

## Transparency layers

For correct *group* opacity — where a set of overlapping shapes must fade as a
single unit — use a transparency layer. It renders offscreen, then composites
the finished result back with the layer opacity.

```cpp
auto layer = g.beginTransparencyLayer (targetArea, 0.5f);
if (layer.isValid())
{
    auto& lg = layer.getGraphics();   // draws in layer-local coordinates
    lg.setFillColor (Colors::red);
    lg.fillEllipse (circleA);
    lg.fillEllipse (circleB);         // overlap won't double up when faded
    layer.commit();
}
```

Inside the layer, coordinates are layer-local: the target area's top-left is
`(0, 0)`.

## Offscreen rendering into an Image

`Graphics` can be constructed to render into an `Image` on the GPU and read the
result back on the CPU — useful for thumbnails, export, and tests.

```cpp
Image target { 512, 512, PixelFormat::RGBA };
{
    Graphics g { context, target };
    g.setFillColor (Colors::cornflowerblue);
    g.fillAll();
    g.readPixelsToImage(); // fills target's CPU pixel buffer
}
// target now holds the rendered pixels
```

Related methods: `isOffscreen()`, `commitOffscreenTarget()`, `commitToImage()`
(sets the rendered GPU texture on the Image for later `drawImage` without a CPU
round-trip), and `readPixelsToImage()`.

```{tip}
For richer GPU work — custom pipelines, render passes, and post-processing —
step down to the [RHI](rhi/index.md). `GpuCanvas` in particular bridges 2D
`Graphics` drawing and low-level render passes on one surface.
```

## The GraphicsContext

`GraphicsContext` abstracts the active rendering backend behind one interface.
It is created via a static factory with an `Api` and `Options`:

```cpp
GraphicsContext::Options options;              // enableOreContext = true by default
auto context = GraphicsContext::createContext (GraphicsContext::Metal, options);
```

Supported `Api` values: `Headless`, `OpenGL`, `OpenGLES`, `Direct3D`, `Metal`,
`WebGPU`. Key `Options` include `retinaDisplay`, `allowHeadlessRendering`,
`enableReadPixels`, and `enableOreContext` (the RHI GPU context).

The context owns backend resources and exposes capability probes such as
`getApi()`, `dpiScale(handle)`, and `isGpuAvailable()`. In a typical
application the windowing layer creates and drives the context for you, handing
a ready-to-use `Graphics` to each `Component::paint`.

## See also

- [Primitives](primitives.md) — the value types drawing calls consume.
- [How to draw](drawing.md) — a practical, task-oriented drawing guide.
- [RHI](rhi/index.md) — the low-level GPU layer beneath `Graphics`.
