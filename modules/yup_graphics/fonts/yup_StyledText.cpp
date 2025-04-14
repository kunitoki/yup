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

rive::TextAlign toTextAlign (StyledText::HorizontalAlign align) noexcept
{
    if (align == StyledText::left)
        return rive::TextAlign::left;

    if (align == StyledText::center)
        return rive::TextAlign::center;

    if (align == StyledText::right)
        return rive::TextAlign::right;

    return rive::TextAlign::left;
}

rive::TextWrap toTextWrap (StyledText::TextWrap wrap) noexcept
{
    if (wrap == StyledText::wrap)
        return rive::TextWrap::wrap;

    if (wrap == StyledText::noWrap)
        return rive::TextWrap::noWrap;

    return rive::TextWrap::noWrap;
}

//==============================================================================

StyledText::StyledText()
{
}

//==============================================================================

bool StyledText::isEmpty() const
{
    return styledTexts.empty();
}

//==============================================================================

void StyledText::clear()
{
    styledTexts.clear();
    styles.clear();

    isDirty = true;
}

//==============================================================================

StyledText::TextOverflow StyledText::getOverflow() const
{
    return overflow;
}

void StyledText::setOverflow (TextOverflow value)
{
    if (overflow != value)
    {
        overflow = value;
        isDirty = true;
    }
}

//==============================================================================

StyledText::HorizontalAlign StyledText::getHorizontalAlign() const
{
    return horizontalAlign;
}

void StyledText::setHorizontalAlign (HorizontalAlign value)
{
    if (horizontalAlign != value)
    {
        horizontalAlign = value;
        isDirty = true;
    }
}

//==============================================================================

StyledText::VerticalAlign StyledText::getVerticalAlign() const
{
    return verticalAlign;
}

void StyledText::setVerticalAlign (VerticalAlign value)
{
    if (verticalAlign != value)
    {
        verticalAlign = value;
        isDirty = true;
    }
}

//==============================================================================

Size<float> StyledText::getMaxSize() const
{
    return maxSize;
}

void StyledText::setMaxSize (const Size<float>& value)
{
    if (maxSize != value)
    {
        maxSize = value;
        isDirty = true;
    }
}

//==============================================================================

float StyledText::getParagraphSpacing() const
{
    return paragraphSpacing;
}

void StyledText::setParagraphSpacing (float value)
{
    if (paragraphSpacing != value)
    {
        paragraphSpacing = value;
        isDirty = true;
    }
}

//==============================================================================

StyledText::TextWrap StyledText::getWrap() const
{
    return textWrap;
}

void StyledText::setWrap (TextWrap value)
{
    if (textWrap != value)
    {
        textWrap = value;
        isDirty = true;
    }
}

//==============================================================================

void StyledText::appendText (StringRef text,
                             rive::rcp<rive::RenderPaint> paint,
                             const Font& font,
                             float fontSize,
                             float lineHeight,
                             float letterSpacing)
{
    int styleIndex = 0;

    for (RenderStyle& style : styles)
    {
        if (style.paint == paint)
            break;

        styleIndex++;
    }

    if (styleIndex == styles.size())
    {
        auto path = rive::make_rcp<rive::RiveRenderPath>();
        styles.emplace_back (paint, std::move (path), true);
    }

    styledTexts.append (font.getFont(), fontSize, lineHeight, letterSpacing, (const char*) text, styleIndex);

    isDirty = true;
}

//==============================================================================

void StyledText::update()
{
    if (! isDirty)
        return;

    auto clearDirtyFlag = ErasedScopeGuard ([this] { isDirty = false; });

    for (RenderStyle& style : styles)
    {
        style.path->rewind();
        style.isEmpty = true;
    }

    renderStyles.clear();
    if (styledTexts.empty())
        return;

    auto runs = styledTexts.runs();
    shape = runs[0].font->shapeText (styledTexts.unichars(), runs);
    lines = rive::Text::BreakLines (shape,
                                    maxSize.getWidth(), // -1.0f
                                    toTextAlign (horizontalAlign),
                                    toTextWrap (textWrap));

    orderedLines.clear();
    ellipsisRun = {};

    // build render styles.
    if (shape.empty())
    {
        bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
        return;
    }

    // Build up ordered runs as we go.
    int paragraphIndex = 0;
    float y = 0.0f;
    float minY = 0.0f;
    float measuredWidth = 0.0f;
    if (origin == TextOrigin::baseline && !lines.empty() && !lines[0].empty())
    {
        y -= lines[0][0].baseline;
        minY = y;
    }

    int ellipsisLine = -1;
    bool isEllipsisLineLast = false;
    bool wantEllipsis = (overflow == TextOverflow::ellipsis);

    int lastLineIndex = -1;
    for (const rive::SimpleArray<rive::GlyphLine>& paragraphLines : lines)
    {
        const rive::Paragraph& paragraph = shape[paragraphIndex++];
        for (const rive::GlyphLine& line : paragraphLines)
        {
            const rive::GlyphRun& endRun = paragraph.runs[line.endRunIndex];
            const rive::GlyphRun& startRun = paragraph.runs[line.startRunIndex];

            float width = endRun.xpos[line.endGlyphIndex] - startRun.xpos[line.startGlyphIndex];
            if (width > measuredWidth)
                measuredWidth = width;

            lastLineIndex++;
            if (wantEllipsis && y + line.bottom <= maxSize.getHeight())
                ellipsisLine++;
        }

        if (! paragraphLines.empty())
            y += paragraphLines.back().bottom;

        y += paragraphSpacing;
    }

    if (wantEllipsis && ellipsisLine == -1)
        ellipsisLine = 0;

    isEllipsisLineLast = lastLineIndex == ellipsisLine;

    int lineIndex = 0;
    paragraphIndex = 0;
    bounds = { 0.0f, minY, measuredWidth, jmax (minY, y - paragraphSpacing) - minY };

    y = 0;
    if (origin == TextOrigin::baseline && !lines.empty() && !lines[0].empty())
        y -= lines[0][0].baseline;

    paragraphIndex = 0;

    for (const rive::SimpleArray<rive::GlyphLine>& paragraphLines : lines)
    {
        const rive::Paragraph& paragraph = shape[paragraphIndex++];
        for (const rive::GlyphLine& line : paragraphLines)
        {
            switch (overflow)
            {
                case TextOverflow::hidden:
                    if (y + line.bottom > maxSize.getHeight())
                        return;

                    break;

                default:
                    break;
            }

            if (lineIndex >= orderedLines.size())
            {
                orderedLines.emplace_back (
                    rive::OrderedLine (paragraph,
                                       line,
                                       maxSize.getWidth(),
                                       ellipsisLine == lineIndex,
                                       isEllipsisLineLast,
                                       &ellipsisRun));
            }

            float x = line.startX;
            float renderY = y + line.baseline;

            for (auto [run, glyphIndex] : orderedLines[lineIndex])
            {
                const rive::Font* font = run->font.get();
                const rive::Vec2D& offset = run->offsets[glyphIndex];

                rive::GlyphID glyphId = run->glyphs[glyphIndex];
                float advance = run->advances[glyphIndex];

                rive::RawPath path = font->getPath (glyphId);

                path.transformInPlace (rive::Mat2D (run->size,
                                                    0.0f,
                                                    0.0f,
                                                    run->size,
                                                    x + offset.x,
                                                    renderY + offset.y));
                x += advance;

                jassert (run->styleId < styles.size());
                RenderStyle* style = &styles[run->styleId];
                jassert (style != nullptr);
                path.addTo (style->path.get());

                if (style->isEmpty)
                {
                    // This was the first path added to the style, so let's mark it in our draw list.
                    style->isEmpty = false;

                    renderStyles.push_back (style);
                }
            }

            // Early return if we're done after ellipsis line
            if (lineIndex == ellipsisLine)
                return;

            lineIndex++;
        }

        if (! paragraphLines.empty())
            y += paragraphLines.back().bottom;

        y += paragraphSpacing;
    }
}

//==============================================================================

Rectangle<float> StyledText::getBounds()
{
    update();
    return bounds;
}

//==============================================================================

const std::vector<rive::OrderedLine>& StyledText::getOrderedLines()
{
    update();
    return orderedLines;
}

const std::vector<StyledText::RenderStyle*>& StyledText::getRenderStyles()
{
    update();
    return renderStyles;
}

} // namespace yup
