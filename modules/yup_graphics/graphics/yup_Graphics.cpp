/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

namespace
{

//==============================================================================

rive::StrokeJoin toStrokeJoin (StrokeJoin join) noexcept
{
    return static_cast<rive::StrokeJoin> (join);
}

//==============================================================================

rive::StrokeCap toStrokeCap (StrokeCap cap) noexcept
{
    return static_cast<rive::StrokeCap> (cap);
}

//==============================================================================

rive::BlendMode toBlendMode (BlendMode blendMode) noexcept
{
    switch (blendMode)
    {
        case BlendMode::SrcOver:
            return rive::BlendMode::srcOver;

        case BlendMode::Screen:
            return rive::BlendMode::screen;

        case BlendMode::Overlay:
            return rive::BlendMode::overlay;

        case BlendMode::Darken:
            return rive::BlendMode::darken;

        case BlendMode::Lighten:
            return rive::BlendMode::lighten;

        case BlendMode::ColorDodge:
            return rive::BlendMode::colorDodge;

        case BlendMode::ColorBurn:
            return rive::BlendMode::colorBurn;

        case BlendMode::HardLight:
            return rive::BlendMode::hardLight;

        case BlendMode::SoftLight:
            return rive::BlendMode::softLight;

        case BlendMode::Difference:
            return rive::BlendMode::difference;

        case BlendMode::Exclusion:
            return rive::BlendMode::exclusion;

        case BlendMode::Multiply:
            return rive::BlendMode::multiply;

        case BlendMode::Hue:
            return rive::BlendMode::hue;

        case BlendMode::Saturation:
            return rive::BlendMode::saturation;

        case BlendMode::Color:
            return rive::BlendMode::color;

        case BlendMode::Luminosity:
            return rive::BlendMode::luminosity;

        default:
            return rive::BlendMode::srcOver;
    }
}

//==============================================================================

void convertRawPathToRenderPath (const rive::RawPath& input, rive::RenderPath* output)
{
    input.addTo (output);
}

void convertRawPathToRenderPath (const rive::RawPath& input, rive::RenderPath* output, const AffineTransform& transform)
{
    if (transform.isIdentity())
    {
        convertRawPathToRenderPath (input, output);
    }
    else
    {
        auto newInput = input.transform (transform.toMat2D());
        newInput.addTo (output);
    }
}

//==============================================================================

rive::rcp<rive::RenderShader> toColorGradient (rive::Factory& factory, const ColorGradient& gradient, const AffineTransform& transform)
{
    const auto& colorStops = gradient.getStops();

    if (colorStops.empty())
        return nullptr;

    // Handle single color stop as solid color
    if (colorStops.size() == 1)
    {
        uint32 color = colorStops[0].color;
        float stops[] = { 0.0f };
        return factory.makeLinearGradient (0.0f, 0.0f, 1.0f, 0.0f, &color, stops, 1);
    }

    // Create dynamic arrays for colors and stops
    std::vector<uint32> colors;
    std::vector<float> stops;

    colors.reserve (colorStops.size());
    stops.reserve (colorStops.size());

    for (const auto& stop : colorStops)
    {
        colors.push_back (stop.color);
        stops.push_back (stop.delta);
    }

    if (gradient.getType() == ColorGradient::Linear)
    {
        float x1 = gradient.getStartX();
        float y1 = gradient.getStartY();
        float x2 = gradient.getFinishX();
        float y2 = gradient.getFinishY();
        transform.transformPoints (x1, y1, x2, y2);

        return factory.makeLinearGradient (x1, y1, x2, y2, colors.data(), stops.data(), colors.size());
    }
    else
    {
        float x1 = gradient.getStartX();
        float y1 = gradient.getStartY();
        float radiusX = gradient.getRadius();
        [[maybe_unused]] float radiusY = gradient.getRadius();
        transform.transformPoints (x1, y1, radiusX, radiusY);

        return factory.makeRadialGradient (x1, y1, radiusX, colors.data(), stops.data(), colors.size());
    }
}

//==============================================================================
StyledText::HorizontalAlign toHorizontalAlign (Justification justification)
{
    if (justification.testFlags (Justification::left))
        return StyledText::left;
    else if (justification.testFlags (Justification::right))
        return StyledText::right;
    else if (justification.testFlags (Justification::horizontalCenter))
        return StyledText::center;
    else
        return StyledText::center;
}

StyledText::VerticalAlign toVerticalAlign (Justification justification)
{
    if (justification.testFlags (Justification::top))
        return StyledText::top;
    else if (justification.testFlags (Justification::bottom))
        return StyledText::bottom;
    else if (justification.testFlags (Justification::verticalCenter))
        return StyledText::middle;
    else
        return StyledText::middle;
}

} // namespace

//==============================================================================
Graphics::SavedState::SavedState (Graphics& g)
    : g (std::addressof (g))
{
}

Graphics::SavedState::SavedState (SavedState&& other)
    : g (std::exchange (other.g, nullptr))
{
}

Graphics::SavedState& Graphics::SavedState::operator= (SavedState&& other)
{
    g = std::exchange (other.g, nullptr);
    return *this;
}

Graphics::SavedState::~SavedState()
{
    restore();
}

void Graphics::SavedState::restore()
{
    if (auto graphics = std::exchange (g, nullptr))
        graphics->restoreState();
}

//==============================================================================
Graphics::Graphics (GraphicsContext& context, rive::Renderer& renderer, float scale) noexcept
    : context (context)
    , factory (*context.factory())
    , renderer (renderer)
    , contextScale (scale)
{
    renderOptions.emplace_back();

    currentRenderOptions().scale = scale;
}

//==============================================================================

float Graphics::getContextScale() const
{
    return contextScale;
}

//==============================================================================
rive::Factory* Graphics::getFactory()
{
    return std::addressof (factory);
}

rive::Renderer* Graphics::getRenderer()
{
    return std::addressof (renderer);
}

//==============================================================================
Graphics::RenderOptions& Graphics::currentRenderOptions()
{
    jassert (! renderOptions.empty());

    return renderOptions.back();
}

const Graphics::RenderOptions& Graphics::currentRenderOptions() const
{
    jassert (! renderOptions.empty());

    return renderOptions.back();
}

//==============================================================================
Graphics::SavedState Graphics::saveState()
{
    jassert (! renderOptions.empty());

    renderOptions.emplace_back (renderOptions.back());

    renderer.save();

    return { *this };
}

void Graphics::restoreState()
{
    renderer.restore();

    renderOptions.pop_back();
}

//==============================================================================
void Graphics::setFillColor (Color color)
{
    currentRenderOptions().fillColor = color;
    currentRenderOptions().isCurrentFillColor = true;
}

Color Graphics::getFillColor() const
{
    return currentRenderOptions().fillColor;
}

//==============================================================================
void Graphics::setStrokeColor (Color color)
{
    currentRenderOptions().strokeColor = color;
    currentRenderOptions().isCurrentStrokeColor = true;
}

Color Graphics::getStrokeColor() const
{
    return currentRenderOptions().strokeColor;
}

//==============================================================================
void Graphics::setFillColorGradient (ColorGradient gradient)
{
    currentRenderOptions().fillGradient = std::move (gradient);
    currentRenderOptions().isCurrentFillColor = false;
}

ColorGradient Graphics::getFillColorGradient() const
{
    return currentRenderOptions().fillGradient;
}

//==============================================================================
void Graphics::setStrokeColorGradient (ColorGradient gradient)
{
    currentRenderOptions().strokeGradient = std::move (gradient);
    currentRenderOptions().isCurrentStrokeColor = false;
}

ColorGradient Graphics::getStrokeColorGradient() const
{
    return currentRenderOptions().strokeGradient;
}

//==============================================================================
void Graphics::setFeather (float feather)
{
    currentRenderOptions().feather = jmax (0.0f, feather);
}

float Graphics::getFeather() const
{
    return currentRenderOptions().feather;
}

//==============================================================================
void Graphics::setOpacity (float opacity)
{
    currentRenderOptions().opacity = jlimit (0.0f, 1.0f, opacity);
}

float Graphics::getOpacity() const
{
    return currentRenderOptions().opacity;
}

//==============================================================================
void Graphics::setStrokeType (StrokeType strokeType)
{
    auto& options = currentRenderOptions();

    options.strokeWidth = jmax (0.0f, strokeType.getWidth());
    options.join = strokeType.getJoin();
    options.cap = strokeType.getCap();
}

StrokeType Graphics::getStrokeType() const
{
    auto& options = currentRenderOptions();

    return StrokeType (options.strokeWidth, options.join, options.cap);
}

void Graphics::setStrokeWidth (float strokeWidth)
{
    currentRenderOptions().strokeWidth = jmax (0.0f, strokeWidth);
}

float Graphics::getStrokeWidth() const
{
    return currentRenderOptions().strokeWidth;
}

void Graphics::setStrokeJoin (StrokeJoin join)
{
    currentRenderOptions().join = join;
}

StrokeJoin Graphics::getStrokeJoin() const
{
    return currentRenderOptions().join;
}

void Graphics::setStrokeCap (StrokeCap cap)
{
    currentRenderOptions().cap = cap;
}

StrokeCap Graphics::getStrokeCap() const
{
    return currentRenderOptions().cap;
}

//==============================================================================
void Graphics::setBlendMode (BlendMode blendMode)
{
    currentRenderOptions().blendMode = blendMode;
}

BlendMode Graphics::getBlendMode() const
{
    return currentRenderOptions().blendMode;
}

//==============================================================================
void Graphics::setDrawingArea (const Rectangle<float>& drawingArea)
{
    currentRenderOptions().drawingArea = drawingArea;
}

Rectangle<float> Graphics::getDrawingArea() const
{
    return currentRenderOptions().drawingArea;
}

//==============================================================================
void Graphics::setTransform (const AffineTransform& transform)
{
    currentRenderOptions().transform = transform;
}

void Graphics::addTransform (const AffineTransform& transform)
{
    currentRenderOptions().transform = currentRenderOptions().transform.followedBy (transform);
}

AffineTransform Graphics::getTransform() const
{
    return currentRenderOptions().transform;
}

//==============================================================================
void Graphics::setClipPath (const Rectangle<float>& clipRect)
{
    Path path;
    path.addRectangle (clipRect);

    setClipPath (path);
}

void Graphics::setClipPath (const Path& clipPath)
{
    auto& options = currentRenderOptions();

    options.clipPath = clipPath;

    auto renderPath = rive::make_rcp<rive::RiveRenderPath>();
    renderPath->fillRule (rive::FillRule::nonZero);
    renderPath->addRenderPath (clipPath.getRenderPath(), options.getLocalTransform().toMat2D());

    renderer.clipPath (renderPath.get());
}

Path Graphics::getClipPath() const
{
    return currentRenderOptions().clipPath;
}

//==============================================================================
void Graphics::strokeLine (float x1, float y1, float x2, float y2)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.reserveSpace (2);
    path.moveTo (x1, y1);
    path.lineTo (x2, y2);

    renderStrokePath (path, options, options.getTransform());
}

void Graphics::strokeLine (const Point<float>& p1, const Point<float>& p2)
{
    strokeLine (p1.getX(), p1.getY(), p2.getX(), p2.getY());
}

//==============================================================================
void Graphics::fillAll()
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRectangle (options.getDrawingArea().withZeroPosition());

    renderFillPath (path, options, options.getTransform());
}

//==============================================================================
void Graphics::fillRect (float x, float y, float width, float height)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRectangle (x, y, width, height);

    renderFillPath (path, options, options.getTransform());
}

void Graphics::fillRect (const Rectangle<float>& r)
{
    fillRect (r.getX(), r.getY(), r.getWidth(), r.getHeight());
}

//==============================================================================
void Graphics::strokeRect (float x, float y, float width, float height)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRectangle (x, y, width, height);

    renderStrokePath (path, options, options.getTransform());
}

void Graphics::strokeRect (const Rectangle<float>& r)
{
    strokeRect (r.getX(), r.getY(), r.getWidth(), r.getHeight());
}

//==============================================================================
void Graphics::fillRoundedRect (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRoundedRectangle (
        x, y, width, height, radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);

    renderFillPath (path, options, options.getTransform());
}

void Graphics::fillRoundedRect (float x, float y, float width, float height, float radius)
{
    fillRoundedRect (x, y, width, height, radius, radius, radius, radius);
}

void Graphics::fillRoundedRect (const Rectangle<float>& r, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    fillRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);
}

void Graphics::fillRoundedRect (const Rectangle<float>& r, float radius)
{
    fillRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radius, radius, radius, radius);
}

//==============================================================================
void Graphics::strokeRoundedRect (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRoundedRectangle (
        x, y, width, height, radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);

    renderStrokePath (path, options, options.getTransform());
}

void Graphics::strokeRoundedRect (float x, float y, float width, float height, float radius)
{
    strokeRoundedRect (x, y, width, height, radius, radius, radius, radius);
}

void Graphics::strokeRoundedRect (const Rectangle<float>& r, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight)
{
    strokeRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);
}

void Graphics::strokeRoundedRect (const Rectangle<float>& r, float radius)
{
    strokeRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radius, radius, radius, radius);
}

//==============================================================================
void Graphics::fillEllipse (const Rectangle<float>& r)
{
    Path path;
    path.addEllipse (r);

    fillPath (path);
}

void Graphics::fillEllipse (float x, float y, float width, float height)
{
    Path path;
    path.addEllipse (x, y, width, height);

    fillPath (path);
}

void Graphics::strokeEllipse (const Rectangle<float>& r)
{
    Path path;
    path.addEllipse (r);

    strokePath (path);
}

void Graphics::strokeEllipse (float x, float y, float width, float height)
{
    Path path;
    path.addEllipse (x, y, width, height);

    strokePath (path);
}

//==============================================================================
void Graphics::strokePath (const Path& path)
{
    const auto& options = currentRenderOptions();

    renderStrokePath (path, options, options.getTransform());
}

//==============================================================================
void Graphics::fillPath (const Path& path)
{
    const auto& options = currentRenderOptions();

    renderFillPath (path, options, options.getTransform());
}

//==============================================================================
void Graphics::renderStrokePath (const Path& path, const RenderOptions& options, const AffineTransform& transform)
{
    rive::RiveRenderPaint paint;
    paint.style (rive::RenderPaintStyle::stroke);
    paint.blendMode (toBlendMode (options.blendMode));
    paint.thickness (options.getStrokeWidth());
    paint.join (toStrokeJoin (options.join));
    paint.cap (toStrokeCap (options.cap));

    if (options.isStrokeColor())
        paint.color (options.getStrokeColor());
    else
        paint.shader (toColorGradient (factory, options.getStrokeColorGradient(), transform));

    renderer.save();
    renderer.transform (transform.toMat2D());
    renderer.drawPath (path.getRenderPath(), std::addressof (paint));
    renderer.restore();
}

void Graphics::renderFillPath (const Path& path, const RenderOptions& options, const AffineTransform& transform)
{
    rive::RiveRenderPaint paint;
    paint.style (rive::RenderPaintStyle::fill);
    paint.blendMode (toBlendMode (options.blendMode));
    paint.feather (options.feather);

    if (options.isFillColor())
        paint.color (options.getFillColor());
    else
        paint.shader (toColorGradient (factory, options.getFillColorGradient(), transform));

    renderer.save();
    renderer.transform (transform.toMat2D());
    renderer.drawPath (path.getRenderPath(), std::addressof (paint));
    renderer.restore();
}

//==============================================================================
void Graphics::drawImageAt (const Image& image, const Point<float>& pos)
{
    auto renderContext = context.renderContext();
    if (renderContext == nullptr)
        return;

    if (! image.createTextureIfNotPresent (context))
        return;

    const auto& options = currentRenderOptions();

    static const auto unitRectPath = []
    {
        auto unitRectPath = rive::make_rcp<rive::RiveRenderPath>();
        unitRectPath->line ({ 1, 0 });
        unitRectPath->line ({ 1, 1 });
        unitRectPath->line ({ 0, 1 });
        return unitRectPath;
    }();

    rive::RiveRenderPaint paint;
    paint.image (image.getTexture(), jlimit (0.0f, 1.0f, options.opacity));
    paint.blendMode (toBlendMode (options.blendMode));

    renderer.save();
    renderer.scale (image.getWidth(), image.getHeight());
    renderer.transform (options.getTransform().toMat2D());
    renderer.drawPath (unitRectPath.get(), std::addressof (paint));
    renderer.restore();
}

//==============================================================================
void Graphics::fillFittedText (const StyledText& text, const Rectangle<float>& rect)
{
    jassert (! text.needsUpdate());
    if (text.needsUpdate() || text.isEmpty())
        return;

    const auto& options = currentRenderOptions();

    rive::RiveRenderPaint paint;
    paint.style (rive::RenderPaintStyle::fill);
    paint.blendMode (toBlendMode (options.blendMode));
    paint.feather (options.feather);

    if (options.isFillColor())
        paint.color (options.getFillColor());
    else
        paint.shader (toColorGradient (factory, options.getFillColorGradient(), options.getTransform()));

    renderFittedText (text, rect, std::addressof (paint));
}

void Graphics::fillFittedText (const String& text, const Font& font, const Rectangle<float>& rect, Justification justification)
{
    StyledText styledText;
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (rect.getSize());
        modifier.appendText (text, font);
        modifier.setHorizontalAlign (toHorizontalAlign (justification));
        modifier.setVerticalAlign (toVerticalAlign (justification));
    }

    fillFittedText (styledText, rect);
}

void Graphics::strokeFittedText (const StyledText& text, const Rectangle<float>& rect)
{
    jassert (! text.needsUpdate());
    if (text.needsUpdate() || text.isEmpty())
        return;

    const auto& options = currentRenderOptions();

    rive::RiveRenderPaint paint;
    paint.style (rive::RenderPaintStyle::stroke);
    paint.blendMode (toBlendMode (options.blendMode));
    paint.thickness (options.getStrokeWidth());
    paint.join (toStrokeJoin (options.join));
    paint.cap (toStrokeCap (options.cap));

    if (options.isStrokeColor())
        paint.color (options.getStrokeColor());
    else
        paint.shader (toColorGradient (factory, options.getStrokeColorGradient(), options.getTransform()));

    renderFittedText (text, rect, std::addressof (paint));
}

void Graphics::strokeFittedText (const String& text, const Font& font, const Rectangle<float>& rect, Justification justification)
{
    StyledText styledText;
    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (rect.getSize());
        modifier.appendText (text, font);
        modifier.setHorizontalAlign (toHorizontalAlign (justification));
        modifier.setVerticalAlign (toVerticalAlign (justification));
    }

    strokeFittedText (styledText, rect);
}

void Graphics::renderFittedText (const StyledText& text, const Rectangle<float>& rect, rive::RiveRenderPaint* paint)
{
    jassert (! text.needsUpdate());
    if (text.needsUpdate() || text.isEmpty())
        return;

    const auto& options = currentRenderOptions();

    renderer.save();

    rive::RawPath path;
    path.addRect (rect.toAABB());
    path.transformInPlace (options.getFixedTransform().toMat2D());
    auto renderPath = rive::make_rcp<rive::RiveRenderPath> (rive::FillRule::clockwise, path);
    renderer.clipPath (renderPath.get());

    auto offset = text.getOffset (rect); // We will just use vertical offset
    auto transform = options.getTransform (rect.getX(), rect.getY() + offset.getY());
    renderer.transform (transform.toMat2D());

    for (auto style : text.getRenderStyles())
        renderer.drawPath (style->path.get(), (paint != nullptr) ? paint : style->paint.get());

    renderer.restore();
}

} // namespace yup
