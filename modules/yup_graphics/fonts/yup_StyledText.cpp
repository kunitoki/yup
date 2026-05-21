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
                                           float lineHeight,
                                           float letterSpacing)
{
    styledText.appendText (text, nullptr, font, lineHeight, letterSpacing);
}

void StyledText::TextModifier::appendText (StringRef text,
                                           rive::rcp<rive::RenderPaint> paint,
                                           const Font& font,
                                           float lineHeight,
                                           float letterSpacing)
{
    styledText.appendText (text, paint, font, lineHeight, letterSpacing);
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

StyledText::HorizontalAlign StyledText::horizontalAlignFromJustification (Justification justification)
{
    if (justification.testFlags (Justification::left))
        return StyledText::left;

    if (justification.testFlags (Justification::horizontalCenter))
        return StyledText::center;

    if (justification.testFlags (Justification::right))
        return StyledText::right;

    return StyledText::left;
}

StyledText::VerticalAlign StyledText::verticalAlignFromJustification (Justification justification)
{
    if (justification.testFlags (Justification::top))
        return StyledText::top;

    if (justification.testFlags (Justification::verticalCenter))
        return StyledText::middle;

    if (justification.testFlags (Justification::bottom))
        return StyledText::bottom;

    return StyledText::middle;
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

    float fontSize = font.getHeight();
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
    paragraphYOffsets.clear();
    defaultLineHeight = 0.0f;
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

    // Initialize defaultLineHeight from the first non-empty paragraph so that empty paragraphs
    // (consecutive \n) advance y by a sensible amount even before any non-empty para is seen.
    for (const auto& pgLines : lines)
    {
        if (! pgLines.empty())
        {
            defaultLineHeight = pgLines.back().bottom;
            break;
        }
    }

    float initialDefaultLineHeight = defaultLineHeight;

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
        {
            defaultLineHeight = paragraphLines.back().bottom;
            y += paragraphLines.back().bottom;
        }
        else
        {
            y += defaultLineHeight;
        }

        y += paragraphSpacing;
    }

    if (wantEllipsis && ellipsisLine == -1)
        ellipsisLine = 0;

    isEllipsisLineLast = lastLineIndex == ellipsisLine;

    int lineIndex = 0;
    paragraphIndex = 0;
    bounds = { 0.0f, minY, measuredWidth, jmax (minY, y - paragraphSpacing) - minY };

    paragraphYOffsets.clear();
    paragraphYOffsets.reserve (lines.size());
    defaultLineHeight = initialDefaultLineHeight;

    y = 0;
    if (origin == TextOrigin::baseline && ! lines.empty() && ! lines[0].empty())
        y -= lines[0][0].baseline;

    paragraphIndex = 0;

    for (const rive::SimpleArray<rive::GlyphLine>& paragraphLines : lines)
    {
        paragraphYOffsets.push_back (y);
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
        {
            defaultLineHeight = paragraphLines.back().bottom;
            y += paragraphLines.back().bottom;
        }
        else
        {
            y += defaultLineHeight;
        }

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

    for (int paraIdx = 0; paraIdx < static_cast<int> (lines.size()); ++paraIdx)
    {
        if (! lines[paraIdx].empty() || paraIdx >= static_cast<int> (paragraphYOffsets.size()))
            continue;

        const float paraTop = paragraphYOffsets[paraIdx];
        const float paraBottom = paraTop + defaultLineHeight;

        if (clickY >= paraTop && clickY < paraBottom)
            return findParagraphNewlinePositionByIndex (paraIdx);
    }

    int targetLineIndex = 0;
    float closestLineDistance = std::numeric_limits<float>::max();

    for (int lineIdx = 0; lineIdx < static_cast<int> (orderedLines.size()); ++lineIdx)
    {
        const auto& line = orderedLines[static_cast<size_t> (lineIdx)];
        const auto& glyphLine = line.glyphLine();
        const float lineCenter = line.y() + (glyphLine.top + glyphLine.bottom) * 0.5f;
        const float lineDistance = std::abs (clickY - lineCenter);

        if (lineDistance < closestLineDistance)
        {
            closestLineDistance = lineDistance;
            targetLineIndex = lineIdx;
        }
    }

    const rive::OrderedLine& targetLine = orderedLines[static_cast<size_t> (targetLineIndex)];
    const rive::GlyphLine& glyphLine = targetLine.glyphLine();

    if (clickX <= glyphLine.startX)
    {
        for (const auto& [glyphRun, glyphIndex] : targetLine)
        {
            if (glyphIndex < glyphRun->textIndices.size())
                return static_cast<int> (glyphRun->textIndices[glyphIndex]);
        }

        return findParagraphNewlinePosition (targetLineIndex);
    }

    // Find the closest character using the same xpos logic as getSelectionRectangles
    int bestCharIndex = 0;
    float minDistance = std::numeric_limits<float>::max();
    bool foundAnyGlyph = false;

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

    // If no glyph was found, this is an empty line (e.g. consecutive newlines)
    if (! foundAnyGlyph)
        return findParagraphNewlinePosition (targetLineIndex);

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

    // Newline characters have no glyph. Position the caret at the end of the line whose
    // paragraph the newline terminates. For empty lines (consecutive newlines) the caret
    // lands at startX, which is the left edge of that empty line. This avoids calling
    // lastCodePointIndex() on empty orderedLines (which would be UB).
    const auto& unichars = styledTexts.unichars();

    // Bug 2: caret is at end-of-text and the text ends with \n — the caret belongs on the
    // virtual new line created by the trailing newline, not at the end of the previous line.
    if (characterIndex == static_cast<int> (unichars.size()) && ! unichars.empty() && unichars.back() == '\n' && ! paragraphYOffsets.empty())
    {
        if (! lines.empty() && lines.back().empty())
            return { 0.0f, paragraphYOffsets.back(), 1.0f, defaultLineHeight };

        if (! orderedLines.empty())
        {
            const auto& lastLine = orderedLines.back();
            const auto& glyphLine = lastLine.glyphLine();
            return { 0.0f, lastLine.y() + glyphLine.bottom + paragraphSpacing, 1.0f, defaultLineHeight };
        }

        return { 0.0f, paragraphYOffsets.back(), 1.0f, defaultLineHeight };
    }

    if (characterIndex < static_cast<int> (unichars.size()) && unichars[characterIndex] == '\n')
    {
        // Count how many \n appear before characterIndex to identify the paragraph
        int targetParagraphIdx = 0;
        for (int i = 0; i < characterIndex; ++i)
            if (unichars[i] == '\n')
                ++targetParagraphIdx;

        // Walk the paragraph/line structure to find the last orderedLine of that paragraph
        int lineOffset = 0;
        for (int paraIdx = 0; paraIdx < static_cast<int> (lines.size()); ++paraIdx)
        {
            int numLinesInPara = static_cast<int> (lines[paraIdx].size());

            if (paraIdx == targetParagraphIdx)
            {
                // Bug 3: empty paragraph — numLinesInPara == 0 makes lastLineIdx wrong.
                // Use the stored Y offset directly instead of trying to index orderedLines.
                if (numLinesInPara == 0)
                {
                    float paraY = (paraIdx < static_cast<int> (paragraphYOffsets.size()))
                                    ? paragraphYOffsets[paraIdx]
                                    : 0.0f;
                    return { 0.0f, paraY, 1.0f, defaultLineHeight };
                }

                int lastLineIdx = lineOffset + numLinesInPara - 1;
                if (lastLineIdx >= static_cast<int> (orderedLines.size()))
                    break;

                const rive::OrderedLine& targetOLine = orderedLines[lastLineIdx];
                const rive::GlyphLine& gl = targetOLine.glyphLine();

                // Find end-X of visible content; stays at startX for empty paragraphs
                float endX = gl.startX;
                for (auto [gr, gi] : targetOLine)
                {
                    if (gi < gr->xpos.size())
                    {
                        endX = (gi + 1 < gr->xpos.size())
                                 ? gr->xpos[gi + 1]
                                 : gr->xpos[gi] + (gi < gr->advances.size() ? gr->advances[gi] : 0.0f);
                    }
                }

                return { endX, targetOLine.y() + gl.top, 1.0f, gl.bottom - gl.top };
            }

            lineOffset += numLinesInPara;
        }
    }

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
        if (characterIndex < static_cast<int> (line.lastCodePointIndex (glyphLookup)))
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

int StyledText::getGlyphIndexOnAdjacentLine (int characterIndex, bool moveDown) const
{
    jassert (! isDirty);
    if (isDirty || orderedLines.empty())
        return 0;

    characterIndex = jlimit (0, static_cast<int> (styledTexts.unichars().size()), characterIndex);

    auto caretBounds = getCaretBounds (characterIndex);
    if (caretBounds.isEmpty())
        return moveDown ? static_cast<int> (styledTexts.unichars().size()) : 0;

    struct VisualLine
    {
        float center = 0.0f;
        int orderedLineIndex = -1;
        int emptyParagraphIndex = -1;
    };

    std::vector<VisualLine> visualLines;
    visualLines.reserve (orderedLines.size() + lines.size());

    int orderedLineOffset = 0;
    for (int paraIdx = 0; paraIdx < static_cast<int> (lines.size()); ++paraIdx)
    {
        const int numLinesInPara = static_cast<int> (lines[paraIdx].size());

        if (numLinesInPara == 0)
        {
            if (paraIdx < static_cast<int> (paragraphYOffsets.size()))
                visualLines.push_back ({ paragraphYOffsets[paraIdx] + defaultLineHeight * 0.5f, -1, paraIdx });
        }
        else
        {
            for (int lineIdx = 0; lineIdx < numLinesInPara; ++lineIdx)
            {
                const int orderedLineIndex = orderedLineOffset + lineIdx;
                if (orderedLineIndex >= static_cast<int> (orderedLines.size()))
                    continue;

                const auto& line = orderedLines[static_cast<size_t> (orderedLineIndex)];
                const auto& glyphLine = line.glyphLine();
                visualLines.push_back ({ line.y() + (glyphLine.top + glyphLine.bottom) * 0.5f, orderedLineIndex, -1 });
            }
        }

        orderedLineOffset += numLinesInPara;
    }

    if (visualLines.empty())
        return moveDown ? static_cast<int> (styledTexts.unichars().size()) : 0;

    int currentLineIndex = 0;
    float closestLineDistance = std::numeric_limits<float>::max();
    const float caretCenterY = caretBounds.getCenterY();

    for (int lineIndex = 0; lineIndex < static_cast<int> (visualLines.size()); ++lineIndex)
    {
        const float lineDistance = std::abs (caretCenterY - visualLines[static_cast<size_t> (lineIndex)].center);

        if (lineDistance < closestLineDistance)
        {
            closestLineDistance = lineDistance;
            currentLineIndex = lineIndex;
        }
    }

    const int targetLineIndex = currentLineIndex + (moveDown ? 1 : -1);
    if (targetLineIndex < 0)
        return 0;

    if (targetLineIndex >= static_cast<int> (visualLines.size()))
        return static_cast<int> (styledTexts.unichars().size());

    const auto& targetVisualLine = visualLines[static_cast<size_t> (targetLineIndex)];
    if (targetVisualLine.emptyParagraphIndex >= 0)
        return findParagraphNewlinePositionByIndex (targetVisualLine.emptyParagraphIndex);

    const auto& currentVisualLine = visualLines[static_cast<size_t> (currentLineIndex)];

    struct LineInfo
    {
        int end = 0;
        bool hasGlyph = false;
        bool hasNonWhitespace = false;
    };

    auto getLineInfo = [this] (const auto& line)
    {
        LineInfo info;
        const auto& unichars = styledTexts.unichars();

        for (const auto& [glyphRun, glyphIndex] : line)
        {
            if (glyphIndex >= glyphRun->textIndices.size())
                continue;

            const int charIndex = static_cast<int> (glyphRun->textIndices[glyphIndex]);
            info.end = jmax (info.end, charIndex + 1);
            info.hasGlyph = true;

            if (charIndex < static_cast<int> (unichars.size()) && ! CharacterFunctions::isWhitespace (static_cast<yup_wchar> (unichars[charIndex])))
            {
                info.hasNonWhitespace = true;
            }
        }

        return info;
    };

    const auto currentLineInfo = currentVisualLine.orderedLineIndex >= 0
                                   ? getLineInfo (orderedLines[static_cast<size_t> (currentVisualLine.orderedLineIndex)])
                                   : LineInfo {};

    const auto& targetLine = orderedLines[static_cast<size_t> (targetVisualLine.orderedLineIndex)];
    const auto& targetGlyphLine = targetLine.glyphLine();
    const float targetY = targetLine.y() + (targetGlyphLine.top + targetGlyphLine.bottom) * 0.5f;

    if (moveDown && currentLineInfo.hasGlyph && ! currentLineInfo.hasNonWhitespace && characterIndex >= currentLineInfo.end)
        return getGlyphIndexAtPosition ({ targetGlyphLine.startX, targetY });

    return getGlyphIndexAtPosition ({ caretBounds.getX(), targetY });
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

//==============================================================================

int StyledText::findParagraphNewlinePositionByIndex (int paragraphIndex) const
{
    const auto& unichars = styledTexts.unichars();
    int nlCount = 0;
    for (int i = 0; i < static_cast<int> (unichars.size()); ++i)
    {
        if (unichars[i] == '\n')
        {
            if (nlCount == paragraphIndex)
                return i;
            ++nlCount;
        }
    }
    return static_cast<int> (unichars.size());
}

int StyledText::findParagraphNewlinePosition (int orderedLineIndex) const
{
    const auto& unichars = styledTexts.unichars();
    int lineOffset = 0;

    for (int paraIdx = 0; paraIdx < static_cast<int> (lines.size()); ++paraIdx)
    {
        int numLinesInPara = static_cast<int> (lines[paraIdx].size());

        if (orderedLineIndex >= lineOffset && orderedLineIndex < lineOffset + numLinesInPara)
        {
            int nlCount = 0;
            for (int i = 0; i < static_cast<int> (unichars.size()); ++i)
            {
                if (unichars[i] == '\n')
                {
                    if (nlCount == paraIdx)
                        return i;
                    ++nlCount;
                }
            }

            // The orderedLine belongs to the last paragraph which has no trailing '\n'.
            // Return end-of-text so the caret lands at the very end.
            return static_cast<int> (unichars.size());
        }

        lineOffset += numLinesInPara;
    }

    return 0;
}

} // namespace yup
