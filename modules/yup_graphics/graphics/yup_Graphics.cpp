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

rive::RawPath toRawPath (const Path& path, const AffineTransform& transform)
{
    rive::RawPath rawPath;

    for (const auto& segment : path)
    {
        if (segment.type == Path::SegmentType::MoveTo)
        {
            auto x = segment.x;
            auto y = segment.y;

            transform.transformPoints (x, y);

            rawPath.moveTo (x, y);
        }
        else if (segment.type == Path::SegmentType::LineTo)
        {
            auto x = segment.x;
            auto y = segment.y;

            transform.transformPoints (x, y);

            rawPath.lineTo (x, y);
        }
        else if (segment.type == Path::SegmentType::QuadTo)
        {
            auto x = segment.x;
            auto y = segment.y;
            auto x1 = segment.x1;
            auto y1 = segment.y1;

            transform.transformPoints (x, y, x1, y1);

            rawPath.quadTo (x, y, x1, y1);
        }
        else if (segment.type == Path::SegmentType::CubicTo)
        {
            auto x = segment.x;
            auto y = segment.y;
            auto x1 = segment.x1;
            auto y1 = segment.y1;
            auto x2 = segment.x2;
            auto y2 = segment.y2;

            transform.transformPoints (x, y, x1, y1, x2, y2);

            rawPath.cubicTo (x, y, x1, y1, x2, y2);
        }
    }

    return rawPath;
}

void convertRawPathToCommandPath (const rive::RawPath& input, rive::CommandPath* output, const AffineTransform& transform)
{
    auto morphed = input.morph ([&transform](rive::Vec2D v)
    {
        transform.transformPoints (v.x, v.y);
        return v;
    });

    morphed.addTo (output);
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

float drawText (Graphics& g,
                const rive::GlyphRun& run,
                unsigned startIndex,
                unsigned endIndex,
                rive::rcp<rive::RenderPaint> paint,
                const AffineTransform& transform,
                rive::Vec2D origin)
{
    auto font = run.font.get();
    const auto scale = rive::Mat2D::fromScale (run.size, run.size);

    float x = origin.x;
    jassert (startIndex >= 0 && endIndex <= run.glyphs.size());

    int i, end, inc;
    if (run.dir == rive::TextDirection::rtl)
    {
        i = endIndex - 1;
        end = startIndex - 1;
        inc = -1;
    }
    else
    {
        i = startIndex;
        end = endIndex;
        inc = 1;
    }

    auto renderer = g.getRenderer();
    auto factory = g.getFactory();

    while (i != end)
    {
        auto trans = rive::Mat2D::fromTranslate (x, origin.y);
        x += run.advances[i];

        auto rawpath = font->getPath (run.glyphs[i]);
        rawpath.transformInPlace (trans * scale);

        auto path = factory->makeEmptyRenderPath();
        convertRawPathToCommandPath (rawpath, path.get(), transform);

        renderer->drawPath (path.get(), paint.get());

        i += inc;
    }

    return x;
}

float drawParagraph (Graphics& g,
                     const rive::Paragraph& paragraph,
                     const rive::SimpleArray<rive::GlyphLine>& lines,
                     rive::rcp<rive::RenderPaint> paint,
                     const AffineTransform& transform,
                     rive::Vec2D origin)
{
    for (const auto& line : lines)
    {
        float x = line.startX + origin.x;

        int runIndex, endRun, runInc;
        if (paragraph.baseDirection == rive::TextDirection::rtl)
        {
            runIndex = line.endRunIndex;
            endRun = line.startRunIndex - 1;
            runInc = -1;
        }
        else
        {
            runIndex = line.startRunIndex;
            endRun = line.endRunIndex + 1;
            runInc = 1;
        }

        while (runIndex != endRun)
        {
            const auto& run = paragraph.runs[runIndex];

            int startGIndex = runIndex == line.startRunIndex ? line.startGlyphIndex : 0;
            int endGIndex = runIndex == line.endRunIndex ? line.endGlyphIndex : static_cast<int> (run.glyphs.size());

            x = drawText (g,
                          run,
                          startGIndex,
                          endGIndex,
                          paint,
                          transform,
                          { x, origin.y + line.baseline });

            runIndex += runInc;
        }
    }

    return origin.y + lines.back().bottom;
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
void Graphics::setColor (Color color)
{
    currentRenderOptions().color = color;
    currentRenderOptions().isCurrentBrushColor = true;
}

Color Graphics::getColor() const
{
    return currentRenderOptions().color;
}

void Graphics::setColorGradient (ColorGradient gradient)
{
    currentRenderOptions().gradient = std::move (gradient);
    currentRenderOptions().isCurrentBrushColor = false;
}

ColorGradient Graphics::getColorGradient() const
{
    return currentRenderOptions().gradient;
}

void Graphics::setOpacity (uint8 opacity)
{
    currentRenderOptions().alpha = opacity;
}

uint8 Graphics::getOpacity() const
{
    return currentRenderOptions().alpha;
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

//==============================================================================
void Graphics::drawLine (float x1, float y1, float x2, float y2, float thickness)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.moveTo (x1, y1);
    path.lineTo (x2, y2);

    auto rawPath = toRawPath (path, options.getTransform());
    renderDrawPath (rawPath, options, thickness);
}

void Graphics::drawLine (const Point<float>& p1, const Point<float>& p2, float thickness)
{
    drawLine (p1.getX(), p1.getY(), p2.getX(), p2.getY(), thickness);
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

    auto rawPath = toRawPath (path, options.getTransform());
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
void Graphics::drawRect (float x, float y, float width, float height, float thickness)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.moveTo (x, y);
    path.lineTo (x + width, y);
    path.lineTo (x + width, y + height);
    path.lineTo (x, y + height);
    path.lineTo (x, y);

    auto rawPath = toRawPath (path, options.getTransform());
    renderDrawPath (rawPath, options, thickness);
}

void Graphics::drawRect (const Rectangle<float>& r, float thickness)
{
    drawRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), thickness);
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
void Graphics::drawRoundedRect (float x, float y, float width, float height, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight, float thickness)
{
    const auto& options = currentRenderOptions();

    Path path;
    path.addRoundedRectangle (
        x, y, width, height,
        radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight);

    auto rawPath = toRawPath (path, options.getTransform());
    renderDrawPath (rawPath, options, thickness);
}

void Graphics::drawRoundedRect (float x, float y, float width, float height, float radius, float thickness)
{
    drawRoundedRect (x, y, width, height, radius, radius, radius, radius, thickness);
}

void Graphics::drawRoundedRect (const Rectangle<float>& r, float radiusTopLeft, float radiusTopRight, float radiusBottomLeft, float radiusBottomRight, float thickness)
{
    drawRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radiusTopLeft, radiusTopRight, radiusBottomLeft, radiusBottomRight, thickness);
}

void Graphics::drawRoundedRect (const Rectangle<float>& r, float radius, float thickness)
{
    drawRoundedRect (r.getX(), r.getY(), r.getWidth(), r.getHeight(), radius, radius, radius, radius, thickness);
}

//==============================================================================
void Graphics::drawPath (const Path& path, float thickness)
{
    const auto& options = currentRenderOptions();

    auto rawPath = toRawPath (path, options.getTransform());
    renderDrawPath (rawPath, options, thickness);
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

    auto renderPath = factory.makeRenderPath (rawPath, rive::FillRule::nonZero);
    renderer.clipPath (renderPath.get());
}

//==============================================================================
void Graphics::renderDrawPath (rive::RawPath& rawPath, const RenderOptions& options, float thickness)
{
    auto paint = factory.makeRenderPaint();
    paint->style (rive::RenderPaintStyle::stroke);
    paint->thickness (thickness);
    paint->join (toStrokeJoin (options.join));
    paint->cap (toStrokeCap (options.cap));

    if (options.isColor())
        paint->color (options.getColor());
    else
        paint->shader (toColorGradient (factory, options.getColorGradient()));

    auto renderPath = factory.makeRenderPath (rawPath, rive::FillRule::nonZero);
    renderer.drawPath (renderPath.get(), paint.get());
}

void Graphics::renderFillPath (rive::RawPath& rawPath, const RenderOptions& options)
{
    auto paint = factory.makeRenderPaint();
    paint->style (rive::RenderPaintStyle::fill);

    if (options.isColor())
        paint->color (options.getColor());
    else
        paint->shader (toColorGradient (factory, options.getColorGradient()));

    auto renderPath = factory.makeRenderPath (rawPath, rive::FillRule::nonZero);
    renderer.drawPath (renderPath.get(), paint.get());
}

//==============================================================================
void Graphics::drawFittedText (const StyledText& text, const Rectangle<float>& rect, rive::TextAlign align)
{
    const auto& options = currentRenderOptions();

    auto transform = getTransform();

    float y = rect.getY();
    float paragraphWidth = rect.getWidth(); // -1.0
    float lineHeight = 21.0f;

    float totalTextHeight = text.getParagraphs().size() * lineHeight;

    auto paint = factory.makeRenderPaint();
    paint->style (rive::RenderPaintStyle::fill);

    if (options.isColor())
        paint->color (options.getColor());
    else
        paint->shader (toColorGradient (factory, options.getColorGradient()));

    rive::SimpleArray<rive::SimpleArray<rive::GlyphLine>> linesArray (text.getParagraphs().size());

    std::size_t paragraphIndex = 0;
    for (const auto& paragraph : text.getParagraphs())
    {
        linesArray[paragraphIndex] = rive::GlyphLine::BreakLines (paragraph.runs, paragraphWidth); // -1.0f

        //if (autoWidth)
        //{
        //    paragraphWidth =
        //        std::max (paragraphWidth,
        //                  rive::GlyphLine::ComputeMaxWidth (linesArray[paragraphIndex], paragraph.runs));
        //}

        ++paragraphIndex;
    }

    paragraphIndex = 0;
    for (const auto& paragraph : text.getParagraphs())
    {
        rive::SimpleArray<rive::GlyphLine>& lines = linesArray[paragraphIndex];

        rive::GlyphLine::ComputeLineSpacing (paragraphIndex == 0,
                                             lines,
                                             paragraph.runs,
                                             paragraphWidth,
                                             align);

        y = drawParagraph (*this,
                           paragraph,
                           lines,
                           paint,
                           options.getTransform(),
                           { 0, y });

        y += lineHeight;

        ++paragraphIndex;
    }
}

} // namespace yup
