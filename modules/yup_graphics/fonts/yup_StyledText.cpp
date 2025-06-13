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

namespace
{

rive::TextAlign toTextAlign (StyledText::HorizontalAlign align) noexcept
{
    if (align == StyledText::left || align == StyledText::justified)
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

} // namespace

//==============================================================================

StyledText::TextModifier::TextModifier (StyledText& styledText)
    : styledText (styledText)
{
}

StyledText::TextModifier::~TextModifier()
{
    styledText.update();
}

void StyledText::TextModifier::clear()
{
    styledText.clear();
}

void StyledText::TextModifier::appendText (StringRef text,
                                           const Font& font,
                                           float fontSize,
                                           float lineHeight,
                                           float letterSpacing)
{
    styledText.appendText (text, nullptr, font, fontSize, lineHeight, letterSpacing);
}

void StyledText::TextModifier::appendText (StringRef text,
                                           rive::rcp<rive::RenderPaint> paint,
                                           const Font& font,
                                           float fontSize,
                                           float lineHeight,
                                           float letterSpacing)
{
    styledText.appendText (text, paint, font, fontSize, lineHeight, letterSpacing);
}

void StyledText::TextModifier::setOverflow (StyledText::TextOverflow value)
{
    styledText.setOverflow (value);
}

void StyledText::TextModifier::setHorizontalAlign (StyledText::HorizontalAlign value)
{
    styledText.setHorizontalAlign (value);
}

void StyledText::TextModifier::setVerticalAlign (StyledText::VerticalAlign value)
{
    styledText.setVerticalAlign (value);
}

void StyledText::TextModifier::setMaxSize (const Size<float>& value)
{
    styledText.setMaxSize (value);
}

void StyledText::TextModifier::setParagraphSpacing (float value)
{
    styledText.setParagraphSpacing (value);
}

void StyledText::TextModifier::setWrap (StyledText::TextWrap value)
{
    styledText.setWrap (value);
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

bool StyledText::needsUpdate() const
{
    return isDirty;
}

//==============================================================================

StyledText::TextModifier StyledText::startUpdate()
{
    return { *this };
}

//==============================================================================

void StyledText::clear()
{
    styledTexts.clear();
    styles.clear();

    update();
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

    auto clearDirtyFlag = ErasedScopeGuard ([this]
    {
        isDirty = false;
    });

    for (RenderStyle& style : styles)
    {
        style.path->rewind();
        style.isEmpty = true;
    }

    renderStyles.clear();
    if (styledTexts.empty())
        return;

    orderedLines.clear();
    ellipsisRun = {};

    const auto& runs = styledTexts.runs();
    if (runs[0].font == nullptr)
        return;

    shape = runs[0].font->shapeText (styledTexts.unichars(), runs);
    lines = rive::Text::BreakLines (shape,
                                    maxSize.getWidth(), // -1.0f
                                    toTextAlign (horizontalAlign),
                                    toTextWrap (textWrap));

    if (shape.empty())
    {
        bounds = { 0.0f, 0.0f, 0.0f, 0.0f };
        return;
    }

    // Compute glyph lookup for text positioning
    glyphLookup.compute (styledTexts.unichars(), shape);

    // Build up ordered runs as we go.
    int paragraphIndex = 0;
    float y = 0.0f;
    float minY = 0.0f;
    float measuredWidth = 0.0f;
    if (origin == TextOrigin::baseline && ! lines.empty() && ! lines[0].empty())
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
    if (origin == TextOrigin::baseline && ! lines.empty() && ! lines[0].empty())
        y -= lines[0][0].baseline;

    paragraphIndex = 0;

    for (const rive::SimpleArray<rive::GlyphLine>& paragraphLines : lines)
    {
        const rive::Paragraph& paragraph = shape[paragraphIndex++];
        for (const rive::GlyphLine& line : paragraphLines)
        {
            if (lineIndex >= orderedLines.size())
            {
                orderedLines.emplace_back (
                    rive::OrderedLine (paragraph,
                                       line,
                                       maxSize.getWidth(),
                                       ellipsisLine == lineIndex,
                                       isEllipsisLineLast,
                                       &ellipsisRun,
                                       y));
            }

            float x = line.startX;
            float renderY = y + line.baseline;
            float adjustX = 0.0f;

            if (horizontalAlign == HorizontalAlign::justified && lineIndex != lastLineIndex)
            {
                float renderX = x;
                int numGlyphs = 0;

                for (const auto& [run, glyphIndex] : orderedLines[lineIndex])
                {
                    const rive::Vec2D& offset = run->offsets[glyphIndex];
                    renderX += run->advances[glyphIndex] + offset.x;

                    ++numGlyphs;
                }

                if (renderX < measuredWidth)
                    adjustX = (measuredWidth - renderX) / numGlyphs;
            }

            for (const auto& [run, glyphIndex] : orderedLines[lineIndex])
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
                x += advance + adjustX;

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

int StyledText::getGlyphIndexAtPosition (const Point<float>& position) const
{
    jassert (! isDirty);
    if (isDirty || orderedLines.empty())
        return 0;

    float clickX = position.getX();
    float clickY = position.getY();

    // Use the same approach as getSelectionRectangles to find the line
    int targetLineIndex = -1;

    for (size_t lineIdx = 0; lineIdx < orderedLines.size(); ++lineIdx)
    {
        const rive::OrderedLine& line = orderedLines[lineIdx];
        const rive::GlyphLine& glyphLine = line.glyphLine();

        float lineY = line.y();
        float lineTop = lineY + glyphLine.top;
        float lineBottom = lineY + glyphLine.bottom;

        // Check if click is within this line's vertical bounds (same as getSelectionRectangles)
        if (clickY >= lineTop && clickY <= lineBottom)
        {
            targetLineIndex = static_cast<int> (lineIdx);
            break;
        }
        // If click is above the first line, use the first line
        else if (lineIdx == 0 && clickY < lineTop)
        {
            targetLineIndex = 0;
            break;
        }
        // If click is below all lines, use the last line
        else if (lineIdx == orderedLines.size() - 1 && clickY > lineBottom)
        {
            targetLineIndex = static_cast<int> (lineIdx);
            break;
        }
    }

    if (targetLineIndex == -1)
        return static_cast<int> (styledTexts.unichars().size());

    const rive::OrderedLine& targetLine = orderedLines[targetLineIndex];
    const rive::GlyphLine& glyphLine = targetLine.glyphLine();

    // Find the closest character using the same xpos logic as getSelectionRectangles
    int bestCharIndex = 0;
    float minDistance = std::numeric_limits<float>::max();
    bool foundAnyGlyph = false;

    // If click is before the line start, return the first character in the line
    if (clickX <= glyphLine.startX)
    {
        for (const auto& [glyphRun, glyphIndex] : targetLine)
        {
            if (glyphIndex < glyphRun->textIndices.size())
                return static_cast<int> (glyphRun->textIndices[glyphIndex]);
        }

        return 0;
    }

    for (const auto& [glyphRun, glyphIndex] : targetLine)
    {
        // Check if this glyph run has valid data (same check as getSelectionRectangles)
        if (glyphIndex >= glyphRun->textIndices.size() || glyphIndex >= glyphRun->xpos.size())
            continue;

        uint32_t textIndex = glyphRun->textIndices[glyphIndex];
        int charIndex = static_cast<int> (textIndex);

        // Use the same X positioning logic as getSelectionRectangles
        float glyphX = glyphRun->xpos[glyphIndex];
        float nextGlyphX = (glyphIndex + 1 < glyphRun->xpos.size()) ? glyphRun->xpos[glyphIndex + 1] : glyphX + (glyphIndex < glyphRun->advances.size() ? glyphRun->advances[glyphIndex] : 0);

        // Check if click is within this character's bounds
        if (clickX >= glyphX && clickX <= nextGlyphX)
        {
            // Return the closest boundary
            float midPoint = (glyphX + nextGlyphX) * 0.5f;
            return (clickX <= midPoint) ? charIndex : charIndex + 1;
        }

        // Calculate distances to start and end of this character
        float distanceToStart = std::abs (clickX - glyphX);
        float distanceToEnd = std::abs (clickX - nextGlyphX);

        // Check if click is closer to the start of this character
        if (distanceToStart < minDistance)
        {
            minDistance = distanceToStart;
            bestCharIndex = charIndex;
            foundAnyGlyph = true;
        }

        // Check if click is closer to the end of this character
        if (distanceToEnd < minDistance)
        {
            minDistance = distanceToEnd;
            bestCharIndex = charIndex + 1;
            foundAnyGlyph = true;
        }
    }

    // If no glyph was found, return the start of this line
    if (! foundAnyGlyph)
    {
        // Find the first character in this line
        for (const auto& [glyphRun, glyphIndex] : targetLine)
        {
            if (glyphIndex < glyphRun->textIndices.size())
                return static_cast<int> (glyphRun->textIndices[glyphIndex]);
        }

        return 0;
    }

    // Ensure the result is within valid bounds
    return jlimit (0, static_cast<int> (styledTexts.unichars().size()), bestCharIndex);
}

//==============================================================================

Rectangle<float> StyledText::getCaretBounds (int characterIndex) const
{
    jassert (! isDirty);
    if (isDirty || orderedLines.empty())
        return {};

    // Handle bounds checking
    if (characterIndex < 0)
        characterIndex = 0;

    // Use the same approach as getSelectionRectangles
    for (size_t lineIdx = 0; lineIdx < orderedLines.size(); ++lineIdx)
    {
        const rive::OrderedLine& line = orderedLines[lineIdx];
        const rive::GlyphLine& glyphLine = line.glyphLine();

        float lineY = line.y();
        float lineHeight = glyphLine.bottom - glyphLine.top;

        for (const auto& [glyphRun, glyphIndex] : line)
        {
            // Check if this glyph run has valid data
            if (glyphIndex >= glyphRun->textIndices.size() || glyphIndex >= glyphRun->xpos.size())
                continue;

            uint32_t textIndex = glyphRun->textIndices[glyphIndex];
            int charIndex = static_cast<int> (textIndex);

            // Check if this is our target character
            if (charIndex == characterIndex)
            {
                float caretX = glyphRun->xpos[glyphIndex];
                const float caretWidth = 1.0f;

                return Rectangle<float> (
                    caretX,
                    lineY + glyphLine.top,
                    caretWidth,
                    lineHeight);
            }
            // Check if we've passed our target character (for end-of-line positioning)
            else if (charIndex > characterIndex)
            {
                float caretX = glyphRun->xpos[glyphIndex];
                const float caretWidth = 1.0f;

                return Rectangle<float> (
                    caretX,
                    lineY + glyphLine.top,
                    caretWidth,
                    lineHeight);
            }
        }

        // If we've checked all glyphs in this line and character index is beyond them,
        // position at the end of this line
        if (characterIndex <= static_cast<int> (line.lastCodePointIndex (glyphLookup)))
        {
            // Find the rightmost position in this line
            float endX = glyphLine.startX;
            for (auto [glyphRun, glyphIndex] : line)
            {
                if (glyphIndex < glyphRun->xpos.size())
                {
                    if (glyphIndex + 1 < glyphRun->xpos.size())
                        endX = glyphRun->xpos[glyphIndex + 1];
                    else
                        endX = glyphRun->xpos[glyphIndex] + (glyphIndex < glyphRun->advances.size() ? glyphRun->advances[glyphIndex] : 0);
                }
            }

            const float caretWidth = 1.0f;
            return Rectangle<float> (
                endX,
                lineY + glyphLine.top,
                caretWidth,
                lineHeight);
        }
    }

    // If character index is beyond all text, position at the end of the last line
    if (! orderedLines.empty())
    {
        const rive::OrderedLine& lastLine = orderedLines.back();
        const rive::GlyphLine& glyphLine = lastLine.glyphLine();

        float lineY = lastLine.y();
        float lineHeight = glyphLine.bottom - glyphLine.top;

        // Find the rightmost position in the last line
        float endX = glyphLine.startX;
        for (const auto& [glyphRun, glyphIndex] : lastLine)
        {
            if (glyphIndex < glyphRun->xpos.size())
            {
                if (glyphIndex + 1 < glyphRun->xpos.size())
                    endX = glyphRun->xpos[glyphIndex + 1];
                else
                    endX = glyphRun->xpos[glyphIndex] + (glyphIndex < glyphRun->advances.size() ? glyphRun->advances[glyphIndex] : 0);
            }
        }

        const float caretWidth = 1.0f;
        return Rectangle<float> (
            endX,
            lineY + glyphLine.top,
            caretWidth,
            lineHeight);
    }

    return {};
}

//==============================================================================

std::vector<Rectangle<float>> StyledText::getSelectionRectangles (int startIndex, int endIndex) const
{
    std::vector<Rectangle<float>> rectangles;

    jassert (! isDirty);
    if (isDirty || orderedLines.empty() || startIndex < 0 || endIndex < 0 || startIndex >= endIndex)
        return rectangles;

    rectangles.reserve (orderedLines.size());

    // Use the orderedLines to find selection rectangles
    for (size_t lineIdx = 0; lineIdx < orderedLines.size(); ++lineIdx)
    {
        const rive::OrderedLine& line = orderedLines[lineIdx];
        const rive::GlyphLine& glyphLine = line.glyphLine();

        // Track selection bounds for this line
        float selectionStartX = -1.0f;
        float selectionEndX = -1.0f;
        float lineY = line.y();
        float lineHeight = glyphLine.bottom - glyphLine.top;

        bool hasSelectionInLine = false;

        for (auto [glyphRun, glyphIndex] : line)
        {
            // Check if this glyph run has valid data
            if (glyphIndex >= glyphRun->textIndices.size() || glyphIndex >= glyphRun->xpos.size())
                continue;

            uint32_t textIndex = glyphRun->textIndices[glyphIndex];
            int charIndex = static_cast<int> (textIndex);

            // Check if this character is within the selection
            if (charIndex >= startIndex && charIndex < endIndex)
            {
                float glyphX = glyphRun->xpos[glyphIndex];
                float nextGlyphX = (glyphIndex + 1 < glyphRun->xpos.size()) ? glyphRun->xpos[glyphIndex + 1] : glyphX + (glyphIndex < glyphRun->advances.size() ? glyphRun->advances[glyphIndex] : 0);

                if (! hasSelectionInLine)
                {
                    selectionStartX = glyphX;
                    selectionEndX = nextGlyphX;
                    hasSelectionInLine = true;
                }
                else
                {
                    selectionStartX = std::min (selectionStartX, glyphX);
                    selectionEndX = std::max (selectionEndX, nextGlyphX);
                }
            }
        }

        // If this line has selection, add a rectangle for it
        if (hasSelectionInLine && selectionStartX >= 0.0f && selectionEndX > selectionStartX)
        {
            rectangles.push_back (Rectangle<float> (
                selectionStartX,
                lineY + glyphLine.top,
                selectionEndX - selectionStartX,
                lineHeight));
        }
    }

    return rectangles;
}

//==============================================================================

Rectangle<float> StyledText::getComputedTextBounds() const
{
    jassert (! isDirty);
    return bounds;
}

//==============================================================================

Point<float> StyledText::getOffset (const Rectangle<float>& area) const
{
    jassert (! isDirty);
    if (isDirty)
        return {};

    auto result = Point<float> { 0.0f, 0.0f };

    if (getHorizontalAlign() == StyledText::center)
        result.setX ((area.getWidth() - bounds.getWidth()) * 0.5f);
    else if (getHorizontalAlign() == StyledText::right)
        result.setX (area.getWidth() - bounds.getWidth());

    if (getVerticalAlign() == StyledText::middle)
        result.setY ((area.getHeight() - bounds.getHeight()) * 0.5f);
    else if (getVerticalAlign() == StyledText::bottom)
        result.setY (area.getHeight() - bounds.getHeight());

    return result;
}

//==============================================================================

Span<const rive::OrderedLine> StyledText::getOrderedLines() const
{
    jassert (! isDirty);
    return orderedLines;
}

Span<const StyledText::RenderStyle* const> StyledText::getRenderStyles() const
{
    jassert (! isDirty);
    return renderStyles;
}

//==============================================================================

bool StyledText::isValidCharacterIndex (int characterIndex) const
{
    jassert (! isDirty);
    if (isDirty || characterIndex < 0)
        return false;

    if (glyphLookup.size() == 0)
        return characterIndex == 0;

    return characterIndex <= (int) styledTexts.unichars().size();
}

} // namespace yup
