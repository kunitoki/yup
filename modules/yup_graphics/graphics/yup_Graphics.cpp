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

//==============================================================================
namespace {

rive::StrokeJoin toStrokeJoin (StrokeJoin join) noexcept
{
    return static_cast<rive::StrokeJoin> (join);
}

rive::StrokeCap toStrokeCap (StrokeCap cap) noexcept
{
    return static_cast<rive::StrokeCap> (cap);
}

rive::Mat2D toMat2d (const yup::AffineTransform& t) noexcept
{
    return
    {
        t.getScaleX(),     // xx
        t.getShearX(),     // xy
        t.getShearY(),     // yx
        t.getScaleY(),     // yy
        t.getTranslateX(), // tx
        t.getTranslateY()  // ty
    };
}

rive::RawPath toRawPath (const Path& path)
{
    rive::RawPath rawPath;

    for (const auto& segment : path)
    {
        if (segment.type == Path::SegmentType::MoveTo)
            rawPath.moveTo (segment.x, segment.y);

        else if (segment.type == Path::SegmentType::LineTo)
            rawPath.lineTo (segment.x, segment.y);

        else if (segment.type == Path::SegmentType::QuadTo)
            rawPath.quadTo (segment.x, segment.y, segment.x1, segment.y1);

        else if (segment.type == Path::SegmentType::CubicTo)
            rawPath.cubicTo (segment.x, segment.y, segment.x1, segment.y1, segment.x2, segment.y2);
    }

    return rawPath;
}

rive::RawPath toRawPath (const Path& path, const AffineTransform& transform)
{
    if (transform.isIdentity())
        return toRawPath (path);

    rive::RawPath rawPath;

    for (const auto& segment : path)
    {
        if (segment.type == Path::SegmentType::MoveTo)
        {
            auto x = segment.x, y = segment.y;
            transform.transformPoints (x, y);
            rawPath.moveTo (x, y);
        }

        else if (segment.type == Path::SegmentType::LineTo)
        {
            auto x = segment.x, y = segment.y;
            transform.transformPoints (x, y);
            rawPath.lineTo (x, y);
        }

        else if (segment.type == Path::SegmentType::QuadTo)
        {
            auto x = segment.x, y = segment.y, x1 = segment.x1, y1 = segment.y1;
            transform.transformPoints (x, y, x1, y1);
            rawPath.quadTo (x, y, x1, y1);
        }

        else if (segment.type == Path::SegmentType::CubicTo)
        {
            auto x = segment.x, y = segment.y, x1 = segment.x1, y1 = segment.y1, x2 = segment.x2, y2 = segment.y2;
            transform.transformPoints (x, y, x1, y1, x2, y2);
            rawPath.cubicTo (x, y, x1, y1, x2, y2);
        }
    }

    return rawPath;
}

void convertRawPathToRenderPath (const rive::RawPath& input, rive::RenderPath* output)
{
    input.addTo (output);
}

void convertRawPathToRenderPath (const rive::RawPath& input, rive::RenderPath* output, const AffineTransform& transform)
{
    auto newInput = input.morph ([&transform](auto point)
    {
        transform.transformPoints (point.x, point.y);
        return point;
    });

    newInput.addTo (output);
}

rive::rcp<rive::RenderShader> toColorGradient (rive::Factory& factory, const ColorGradient& gradient)
{
    const uint32 colors[] = { gradient.getStartColor(), gradient.getFinishColor() };
    const float stops[] = { gradient.getStartDelta(), gradient.getFinishDelta() };

    if (gradient.getType() == ColorGradient::Linear)
    {
        return factory.makeLinearGradient (gradient.getStartX(),
                                           gradient.getStartY(),
                                           gradient.getFinishX(),
                                           gradient.getFinishY(),
                                           colors,
                                           stops,
                                           2);
    }
    else
    {
        return factory.makeRadialGradient (gradient.getStartX(),
                                           gradient.getStartY(),
                                           gradient.getRadius(),
                                           colors,
                                           stops,
                                           2);
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
Graphics::Graphics (GraphicsContext& context, rive::Renderer& renderer) noexcept
    : context (context)
    , factory (*context.factory())
    , renderer (renderer)
{
    renderOptions.emplace_back();
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

void Graphics::setStrokeColor (Color color)
{
    currentRenderOptions().strokeColor = color;
    currentRenderOptions().isCurrentStrokeColor = true;
}

Color Graphics::getStrokeColor() const
{
    return currentRenderOptions().strokeColor;
}

void Graphics::setFillColorGradient (ColorGradient gradient)
{
    currentRenderOptions().fillGradient = std::move (gradient);
    currentRenderOptions().isCurrentFillColor = false;
}

ColorGradient Graphics::getFillColorGradient() const
{
    return currentRenderOptions().fillGradient;
}

void Graphics::setStrokeColorGradient (ColorGradient gradient)
{
    currentRenderOptions().strokeGradient = std::move (gradient);
    currentRenderOptions().isCurrentStrokeColor = false;
}

ColorGradient Graphics::getStrokeColorGradient() const
{
    return currentRenderOptions().strokeGradient;
}

void Graphics::setStrokeWidth (float strokeWidth)
{
    currentRenderOptions().strokeWidth = strokeWidth;
}

float Graphics::getStrokeWidth() const
{
    return currentRenderOptions().strokeWidth;
}

void Graphics::setOpacity (float opacity)
{
    currentRenderOptions().opacity = jlimit (0.0f, 1.0f, opacity);
}

float Graphics::getOpacity() const
{
    return currentRenderOptions().opacity;
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

void Graphics::setDrawingArea (const Rectangle<float>& drawingArea)
{
    currentRenderOptions().drawingArea = drawingArea;
}

Rectangle<float> Graphics::getDrawingArea() const
{
    return currentRenderOptions().drawingArea;
}

void Graphics::setTransform (const AffineTransform& transform)
{
    currentRenderOptions().transform = transform;
}

AffineTransform Graphics::getTransform() const
{
    return currentRenderOptions().transform;
}

void Graphics::setClipPath (const Rectangle<float>& clipRect)
{
    Path path;
    path.addRectangle (clipRect);

    setClipPath (path);
}

void Graphics::setClipPath (const Path& clipPath)
{
    currentRenderOptions().clipPath = clipPath;

    auto rawPath = toRawPath (clipPath);
    auto renderPath = factory.makeRenderPath (rawPath, rive::FillRule::nonZero);
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
    path.moveTo (x1, y1);
    path.lineTo (x2, y2);

    auto rawPath = toRawPath (path, options.getTransform());
    renderStrokePath (rawPath, options);
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
    path.moveTo (area.getX(), area.getY());
    path.lineTo (area.getX() + area.getWidth(), area.getY());
    path.lineTo (area.getX() + area.getWidth(), area.getY() + area.getHeight());
    path.lineTo (area.getX(), area.getY() + area.getHeight());
    path.lineTo (area.getX(), area.getY());

    auto rawPath = toRawPath (path);
    renderFillPath (rawPath, options);
}

//==============================================================================
void Graphics::fillRect (float x, float y, float width, float height)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.moveTo (x, y);
    path.lineTo (x + width, y);
    path.lineTo (x + width, y + height);
    path.lineTo (x, y + height);
    path.lineTo (x, y);

    auto rawPath = toRawPath (path, options.getTransform());
    renderFillPath (rawPath, options);
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
    path.moveTo (x, y);
    path.lineTo (x + width, y);
    path.lineTo (x + width, y + height);
    path.lineTo (x, y + height);
    path.lineTo (x, y);

    auto rawPath = toRawPath (path, options.getTransform());
    renderStrokePath (rawPath, options);
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
        x, y, width, height,
        radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);

    auto rawPath = toRawPath (path, options.getTransform());
    renderFillPath (rawPath, options);
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
        x, y, width, height,
        radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);

    auto rawPath = toRawPath (path, options.getTransform());
    renderStrokePath (rawPath, options);
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

    auto rawPath = toRawPath (path, options.getTransform());
    renderStrokePath (rawPath, options);
}

//==============================================================================
void Graphics::fillPath (const Path& path)
{
    const auto& options = currentRenderOptions();

    auto rawPath = toRawPath (path, options.getTransform());
    renderFillPath (rawPath, options);
}

//==============================================================================
void Graphics::clipPath (const Rectangle<float>& area)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.moveTo (area.getX(), area.getY());
    path.lineTo (area.getX() + area.getWidth(), area.getY());
    path.lineTo (area.getX() + area.getWidth(), area.getY() + area.getHeight());
    path.lineTo (area.getX(), area.getY() + area.getHeight());
    path.lineTo (area.getX(), area.getY());

    clipPath (path);
}

void Graphics::clipPath (const Path& path)
{
    const auto& options = currentRenderOptions();

    auto rawPath = toRawPath (path, options.getTransform());
    auto renderPath = factory.makeEmptyRenderPath();

    convertRawPathToRenderPath (rawPath, renderPath.get());

    renderer.clipPath (renderPath.get());
}

//==============================================================================
void Graphics::renderStrokePath (rive::RawPath& rawPath, const RenderOptions& options)
{
    auto paint = factory.makeRenderPaint();
    paint->style (rive::RenderPaintStyle::stroke);
    paint->thickness (options.strokeWidth);
    paint->join (toStrokeJoin (options.join));
    paint->cap (toStrokeCap (options.cap));

    if (options.isStrokeColor())
        paint->color (options.getStrokeColor());
    else
        paint->shader (toColorGradient (factory, options.getStrokeColorGradient()));

    auto renderPath = factory.makeEmptyRenderPath();
    convertRawPathToRenderPath (rawPath, renderPath.get());

    renderer.drawPath (renderPath.get(), paint.get());
}

void Graphics::renderFillPath (rive::RawPath& rawPath, const RenderOptions& options)
{
    auto paint = factory.makeRenderPaint();
    paint->style (rive::RenderPaintStyle::fill);

    if (options.isFillColor())
        paint->color (options.getFillColor());
    else
        paint->shader (toColorGradient (factory, options.getFillColorGradient()));

    auto renderPath = factory.makeEmptyRenderPath();
    convertRawPathToRenderPath (rawPath, renderPath.get());

    renderer.drawPath (renderPath.get(), paint.get());
}

//==============================================================================
void Graphics::strokeFittedText (const StyledText& text, const Rectangle<float>& rect, rive::TextAlign align)
{
    const auto& options = currentRenderOptions();

    auto paint = factory.makeRenderPaint();
    paint->style (rive::RenderPaintStyle::fill);

    if (options.isStrokeColor())
        paint->color (options.getStrokeColor());
    else
        paint->shader (toColorGradient (factory, options.getStrokeColorGradient()));

    auto path = factory.makeEmptyRenderPath();

    std::size_t totalPathSize = 0;
    for (const auto& rawPath : text.getGlyphs())
        totalPathSize += rawPath.verbs().size();

    for (const auto& rawPath : text.getGlyphs())
        convertRawPathToRenderPath (rawPath, path.get(), options.getTransform());

    renderer.drawPath (path.get(), paint.get());
}

} // namespace yup
