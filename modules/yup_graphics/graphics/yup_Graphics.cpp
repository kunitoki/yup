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
    const uint32 colors[] = { gradient.getStartColor(), gradient.getFinishColor() };

    float stops[] = { gradient.getStartDelta(), gradient.getFinishDelta() };
    transform.transformPoints (stops[0], stops[1]);

    if (gradient.getType() == ColorGradient::Linear)
    {
        float x1 = gradient.getStartX();
        float y1 = gradient.getStartY();
        float x2 = gradient.getFinishX();
        float y2 = gradient.getFinishY();
        transform.transformPoints (x1, y1, x2, y2);

        return factory.makeLinearGradient (x1, y1, x2, y2, colors, stops, sizeof (colors));
    }
    else
    {
        float x1 = gradient.getStartX();
        float y1 = gradient.getStartY();
        float radiusX = gradient.getRadius();
        [[maybe_unused]] float radiusY = gradient.getRadius();
        transform.transformPoints (x1, y1, radiusX, radiusY);

        return factory.makeRadialGradient (x1, y1, radiusX, colors, stops, sizeof (colors));
    }
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
    if (g != nullptr)
        g->restoreState();
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
void Graphics::setStrokeWidth (float strokeWidth)
{
    currentRenderOptions().strokeWidth = jmax (0.0f, strokeWidth);
}

float Graphics::getStrokeWidth() const
{
    return currentRenderOptions().strokeWidth;
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
void Graphics::setStrokeJoin (StrokeJoin join)
{
    currentRenderOptions().join = join;
}

StrokeJoin Graphics::getStrokeJoin() const
{
    return currentRenderOptions().join;
}

//==============================================================================
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
    renderPath->addRenderPath (clipPath.getRenderPath(), options.getUntranslatedTransform().toMat2D());

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
    const auto& area = options.getDrawingArea();

    Path path;
    path.reserveSpace (5);
    path.moveTo (area.getX(), area.getY());
    path.lineTo (area.getX() + area.getWidth(), area.getY());
    path.lineTo (area.getX() + area.getWidth(), area.getY() + area.getHeight());
    path.lineTo (area.getX(), area.getY() + area.getHeight());
    path.lineTo (area.getX(), area.getY());

    renderFillPath (path, options, options.getUntranslatedTransform());
}

//==============================================================================
void Graphics::fillRect (float x, float y, float width, float height)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.reserveSpace (5);
    path.moveTo (x, y);
    path.lineTo (x + width, y);
    path.lineTo (x + width, y + height);
    path.lineTo (x, y + height);
    path.lineTo (x, y);

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
    path.reserveSpace (5);
    path.moveTo (x, y);
    path.lineTo (x + width, y);
    path.lineTo (x + width, y + height);
    path.lineTo (x, y + height);
    path.lineTo (x, y);

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
    paint.thickness (options.getStrokeWidth());
    paint.join (toStrokeJoin (options.join));
    paint.cap (toStrokeCap (options.cap));

    if (options.isStrokeColor())
        paint.color (options.getStrokeColor());
    else
        paint.shader (toColorGradient (factory, options.getStrokeColorGradient(), transform));

    auto renderPath = rive::make_rcp<rive::RiveRenderPath>();
    renderPath->addRenderPath (path.getRenderPath(), transform.toMat2D());
    renderer.drawPath (renderPath.get(), std::addressof (paint));
}

void Graphics::renderFillPath (const Path& path, const RenderOptions& options, const AffineTransform& transform)
{
    rive::RiveRenderPaint paint;
    paint.style (rive::RenderPaintStyle::fill);
    paint.feather (options.feather);

    if (options.isFillColor())
        paint.color (options.getFillColor());
    else
        paint.shader (toColorGradient (factory, options.getFillColorGradient(), transform));

    auto renderPath = rive::make_rcp<rive::RiveRenderPath>();
    renderPath->addRenderPath (path.getRenderPath(), transform.toMat2D());
    renderer.drawPath (renderPath.get(), std::addressof (paint));
}

//==============================================================================
void Graphics::drawImageAt (const Image& image, const Point<float>& pos)
{
    auto renderContext = context.renderContext();
    if (renderContext == nullptr)
        return;

    const auto& options = currentRenderOptions();

    renderer.save();
    renderer.scale (image.getWidth(), image.getHeight());
    renderer.transform (options.getTransform().toMat2D());

    if (! image.createTextureIfNotPresent (context))
        return;

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
    //paint.feather (options.feather);

    renderer.drawPath (unitRectPath.get(), std::addressof (paint));

    renderer.restore();
}

//==============================================================================
void Graphics::fillFittedText (StyledText& text, const Rectangle<float>& rect)
{
    text.update();
    if (text.isEmpty())
        return;

    const auto& options = currentRenderOptions();

    auto paint = factory.makeRenderPaint();
    paint->style (rive::RenderPaintStyle::fill);
    paint->feather (options.feather);

    if (options.isFillColor())
        paint->color (options.getFillColor());
    else
        paint->shader (toColorGradient (factory, options.getFillColorGradient(), options.getTransform()));

    float offsetX = rect.getX();
    float offsetY = rect.getY();

    if (text.getHorizontalAlign() == StyledText::center)
        offsetX += (rect.getWidth() - text.getBounds().getWidth()) * 0.5f;
    else if (text.getHorizontalAlign() == StyledText::right)
        offsetX += (rect.getWidth() - text.getBounds().getWidth());

    if (text.getVerticalAlign() == StyledText::middle)
        offsetY += (rect.getHeight() - text.getBounds().getHeight()) * 0.5f;
    else if (text.getVerticalAlign() == StyledText::bottom)
        offsetY += (rect.getHeight() - text.getBounds().getHeight());

    auto transform = options.transform
        .translated (options.drawingArea.getX(), options.drawingArea.getY())
        .translated (offsetX, offsetY)
        .scaled (options.scale);

    renderer.save();

    Path p;
    p.addRectangle (rect);
    p.transform (options.getTransform());
    renderer.clipPath (p.getRenderPath());

    renderer.transform (transform.toMat2D());

    for (auto style : text.getRenderStyles())
        renderer.drawPath (style->path.get(), (paint != nullptr) ? paint.get() : style->paint.get());

    renderer.restore();
}

void Graphics::strokeFittedText (StyledText& text, const Rectangle<float>& rect)
{
    text.update();
    if (text.isEmpty())
        return;

    const auto& options = currentRenderOptions();

    auto paint = factory.makeRenderPaint();
    paint->style (rive::RenderPaintStyle::stroke);

    if (options.isStrokeColor())
        paint->color (options.getStrokeColor());
    else
        paint->shader (toColorGradient (factory, options.getStrokeColorGradient(), options.getTransform()));

    float offsetX = rect.getX();
    float offsetY = rect.getY();

    if (text.getHorizontalAlign() == StyledText::center)
        offsetX += (rect.getWidth() - text.getBounds().getWidth()) * 0.5f;
    else if (text.getHorizontalAlign() == StyledText::right)
        offsetX += (rect.getWidth() - text.getBounds().getWidth());

    if (text.getVerticalAlign() == StyledText::middle)
        offsetY += (rect.getHeight() - text.getBounds().getHeight()) * 0.5f;
    else if (text.getVerticalAlign() == StyledText::bottom)
        offsetY += (rect.getHeight() - text.getBounds().getHeight());

    auto transform = options.transform
        .translated (options.drawingArea.getX(), options.drawingArea.getY())
        .translated (offsetX, offsetY)
        .scaled (options.scale);

    renderer.save();

    Path p;
    p.addRectangle (rect);
    p.transform (options.getTransform());
    renderer.clipPath (p.getRenderPath());

    renderer.transform (transform.toMat2D());

    for (auto style : text.getRenderStyles())
        renderer.drawPath (style->path.get(), (paint != nullptr) ? paint.get() : style->paint.get());

    renderer.restore();
}

} // namespace yup
