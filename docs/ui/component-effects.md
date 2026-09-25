# Component Effects (Shaders)

This guide explains how to apply GPU shader effects to a `Component` and its
subtree via the `ComponentEffect` base class.

## Overview

A `ComponentEffect` renders the component subtree into an offscreen GPU texture,
then applies a post-processing shader before compositing the result back. This
enables visual effects such as blurs, edge detection, pixelation, colour
adjustments, and custom GLSL pipelines without modifying the component's
`paint()` code.

Effects are reference-counted and can be shared across multiple components.
When a component has an active effect, the rendering path automatically:

1. Renders the entire subtree (self + children + `paintOverChildren`) to an
   offscreen `GpuCanvas`.
2. Calls `ComponentEffect::apply()` with the input texture and destination
   bounds.
3. The effect draws its result into the main `Graphics` context.

Nested effects compose naturally: a child's effect completes before the parent's
effect captures the subtree, so a parent blur will blur an already-edged child.

The input texture is at device-pixel resolution: its size is the destination
`bounds` multiplied by the display scale (2× on a Retina display). Size
texel-space parameters such as blur radii or pixel block sizes from the texture,
not from `bounds`. The subtree is rendered at full opacity and the component's
opacity is applied once, when the effect draws into `g`.

---

## Step 1 — Subclass `ComponentEffect`

```cpp
#include <yup_gui/yup_gui.h>

class MyEffect final : public yup::ComponentEffect
{
public:
    void setParam (float value) { param = value; }

    void apply (yup::Graphics& g,
                yup::GpuTexture::Ptr inputTexture,
                yup::Rectangle<float> bounds) override
    {
        // Create a GpuPipeline lazily (compile once, reuse forever).
        if (pipeline == nullptr)
        {
            auto result = yup::GpuPipeline::compileFromGlsl (
                g.getGraphicsContext(), kVertSource, kFragSource, {});
            if (result.failed())
                return;
            pipeline = result.getValue();
        }

        // Apply the effect via a single render pass.
        yup::GpuTarget::Ptr target = yup::GpuTarget::create (
            g.getGraphicsContext(),
            inputTexture->getWidth(),
            inputTexture->getHeight());

        if (target == nullptr)
            return;

        auto frame = yup::GpuFrame::begin (g.getGraphicsContext());
        {
            struct Params { float p0, resX, resY, pad; };
            Params p { param,
                       (float) inputTexture->getWidth(),
                       (float) inputTexture->getHeight(), 0 };

            auto pass = target->beginRenderPass (
                frame, { true, yup::Colors::transparentBlack });
            pass.setPipeline (*pipeline);
            pass.setTexture (0, 0, inputTexture);
            pass.setUniformBuffer (0, 2, &p, sizeof (p));
            pass.draw (3);           // fullscreen triangle
            pass.finish();
        }
        frame.submit();

        g.drawTexture (target->asTexture(), bounds);
    }

private:
    float param = 8.0f;
    yup::GpuPipeline::Ptr pipeline;

    static constexpr const char* kVertSource = R"glsl(#version 450
void main() {
    float x = float((gl_VertexIndex & 1u) << 2u) - 1.0;
    float y = float((gl_VertexIndex & 2u) << 1u) - 1.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
}
)glsl";

    static constexpr const char* kFragSource = R"glsl(#version 450
layout(set = 0, binding = 0) uniform texture2D u_tex;
layout(set = 0, binding = 1) uniform sampler   u_samp;
// ... your shader code ...
)glsl";
};
```

```{note}
All effects use the same vertex shader (a fullscreen triangle generated from
`gl_VertexIndex`). You only need to write the fragment shader for your effect.
```

---

## Step 2 — Attach the Effect to a Component

```cpp
auto effect = yup::ReferenceCountedObjectPtr<MyEffect> (new MyEffect());
myComponent.setComponentEffect (effect);
```

To remove the effect:

```cpp
myComponent.setComponentEffect (nullptr);
```

---

## Built-in Effect Patterns

The [graphics example](../../examples/graphics/) demonstrates several reusable
effect patterns implemented as `ComponentEffect` subclasses. Each lives in
`examples/graphics/source/examples/ComponentEffectsDemo.h`:

| Effect | Shader | Parameter |
|---|---|---|
| Blur | Separable Gaussian (2-pass) | sigma |
| Pixelate | Block downsample | block size |
| Edge Detect | Sobel operator | threshold |
| Wave | Sinusoidal displacement | amplitude |
| Sharpen | Unsharp-mask kernel | strength |
| CRT Scan | Scanlines + vignette | intensity |

All single-pass effects share a `SinglePassEffect` base that handles pipeline
compilation, target resizing, and draw call boilerplate. The two-pass blur is
slightly different because it needs ping-pong `GpuTarget` objects for the
horizontal and vertical passes.

```{tip}
Extract your pipeline setup into a shared helper so each effect only provides a
fragment shader string and a parameter struct.
```

---

## Effect + Caching Interaction

When both `setCachedToTexture(true)` and `setComponentEffect(...)` are set on
the same component, the **effect takes priority**: the full subtree is rendered
offscreen, the effect is applied, and the result is stored in the cache.
Subsequent frames draw the cached (already-effected) texture directly.

If you need a cached background with an effect applied **only to the
background** (not the children), set `setCachedToTexture(true)` on the
background component and place animated children inside it. The children will
still repaint every frame on top of the cached (un-effected) background.

---

## Input Mapping for Distorting Effects

An effect that moves pixels (a wave, a lens, a zoom) displays the subtree
somewhere else than where it was laid out, so pointer input has to be mapped
back. Override `displayToContent()` with the same per-pixel mapping the shader
computes for its sample coordinate, and `contentToDisplay()` with its inverse:

```cpp
class ZoomEffect : public yup::ComponentEffect
{
public:
    void apply (Graphics& g, GpuTexture::Ptr input, Rectangle<float> bounds) override
    {
        zoom = currentZoom;  // publish what is drawn for the input mapping
        // ... draw input zoomed around the center ...
    }

    std::optional<Point<float>> displayToContent (Point<float> p, Rectangle<float> bounds) const override
    {
        const auto center = bounds.getCenter();
        return center + (p - center) / zoom.load();
    }

    std::optional<Point<float>> contentToDisplay (Point<float> p, Rectangle<float> bounds) const override
    {
        const auto center = bounds.getCenter();
        return center + (p - center) * zoom.load();
    }

private:
    std::atomic<float> zoom { 1.0f };
    float currentZoom = 2.0f;
};
```

- `displayToContent()` routes hit-testing, mouse and drag-and-drop events.
  `contentToDisplay()` backs `localToScreen()` and what is built on it, like popup
  placement and the text input caret rectangle. Without it, the identity is used
  as an approximation.
- Both run on the message thread while `apply()` runs on the render thread: read
  the parameters the last `apply()` published through atomics or a lock. This
  also keeps input consistent with what is on screen.
- Return a point even when it falls outside the bounds, so a captured drag keeps
  tracking. Return `std::nullopt` only for degenerate mappings: hit-testing then
  skips the component, and other conversions fall back to its position.
- Repainting anything inside a component with an effect repaints the whole
  component, since the effect can move pixels anywhere inside it.
- When an animated effect moves content under a pointer that stays still, the
  window re-checks the pointer after each painted frame and sends the
  enter / exit, move or drag events the new mapping implies, so hover and
  captured drags follow the animation.

The `Wave` effect of `ComponentEffectsDemo` mirrors its GLSL sampling on the CPU
this way.

---

## Related

- [Component caching](component-caching.md)
- [Component snapshots](component-snapshots.md)
- [Components in 3D](component-3d.md) — present a live component on a mesh
- [RHI pipelines](../graphics/rhi/pipelines.md) — `GpuPipeline` and
  `GpuRenderPass` details
- [Graphics `drawTexture`](../graphics/graphics-class.md)
- [`SpinningCubeDemo`](../../examples/graphics/) — full blur post-process
  example with live shader editing
