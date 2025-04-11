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

rive::TextAlign toTextAlign (StyledText::Alignment align) noexcept
{
    if (align == StyledText::left)
        return rive::TextAlign::left;

    if (align == StyledText::center)
        return rive::TextAlign::center;

    if (align == StyledText::right)
        return rive::TextAlign::right;

    return rive::TextAlign::left;
}

//==============================================================================

StyledText::StyledText()
{
}

//==============================================================================

void StyledText::clear()
{
    unicodeChars.clear();
    textRuns.clear();
    glyphPaths.clear();

    paragraphs = rive::SimpleArray<rive::Paragraph> {};
}

//==============================================================================

void StyledText::appendText (const Font& font,
                             float fontSize,
                             float lineHeight,
                             StringRef text)
{
    textRuns.push_back (append (font, fontSize, lineHeight, text));

    jassert (font.getFont() != nullptr);
    paragraphs = font.getFont()->shapeText (unicodeChars, textRuns);
}

//==============================================================================

rive::TextRun StyledText::append (const Font& font,
                                  float fontSize,
                                  float lineHeight,
                                  StringRef text)
{
    const uint8_t* ptr = (const uint8_t*) (const char*) text;
    uint32_t codepointsCount = 0;

    while (*ptr != '\0')
    {
        unicodeChars.push_back (rive::UTF::NextUTF8 (&ptr));
        codepointsCount += 1;
    }

    return { font.getFont(), fontSize, lineHeight, 0.0f, codepointsCount };
}

//==============================================================================

void StyledText::layout (const Rectangle<float>& rect, Alignment hAlign, VerticalAlignment vAlign)
{
    glyphPaths.clear();

    // We'll measure line breaks into linesArray for each paragraph, 
    // then compute total height to allow vertical alignment.
    rive::SimpleArray<rive::SimpleArray<rive::GlyphLine>> linesArray (paragraphs.size());

    // 1) Break lines using the given width
    float paragraphWidth = rect.getWidth();
    std::size_t paragraphIndex = 0;
    for (const auto& paragraph : paragraphs)
    {
        linesArray[paragraphIndex] = rive::GlyphLine::BreakLines (paragraph.runs, paragraphWidth);
        paragraphIndex++;
    }

    // 2) Compute line spacing (this sets baseline, top, bottom for each line)
    paragraphIndex = 0;
    for (const auto& paragraph : paragraphs)
    {
        rive::SimpleArray<rive::GlyphLine>& lines = linesArray[paragraphIndex];

        rive::GlyphLine::ComputeLineSpacing (paragraphIndex == 0,
                                             lines,
                                             paragraph.runs,
                                             paragraphWidth,
                                             toTextAlign (hAlign));
        paragraphIndex++;
    }

    // 3) Measure total text height for vertical alignment
    float totalHeight = 0.0f;
    for (size_t i = 0; i < paragraphs.size(); ++i)
    {
        const auto& lines = linesArray[i];
        if (! lines.empty())
        {
            // The 'bottom' of the last line in each paragraph
            // extends from the paragraph baseline down to the last line's bottom.
            totalHeight += lines.back().bottom;
        }
    }

    // If you have extra spacing between paragraphs, you can add it to totalHeight here.

    // 4) Compute vertical offset based on alignment
    // By default we start layout at rect.getY().
    float yOffset = 0.0f;
    const float boxHeight = rect.getHeight();

    switch (vAlign)
    {
        case top:    /* no offset */ break;
        case middle: yOffset = (boxHeight - totalHeight) * 0.5f; break;
        case bottom: yOffset =  (boxHeight - totalHeight);       break;
    }

    // 5) Now do the actual layout, offset by yOffset. (Horizontally, we rely on Rive's line alignment.)
    float x = rect.getX();
    float y = rect.getY() + yOffset;

    paragraphIndex = 0;
    for (const auto& paragraph : paragraphs)
    {
        const auto& lines = linesArray[paragraphIndex];

        // Layout each line in the paragraph
        y = layoutParagraph(paragraph, lines, { x, y });

        paragraphIndex++;
    }
}

//==============================================================================

Path StyledText::getGlyphsPath (const AffineTransform& transform) const
{
    Path path;

    for (auto rawPath : glyphPaths)
        path.appendPath (rive::make_rcp<rive::RiveRenderPath> (rive::FillRule::nonZero, rawPath), transform);

    return path;
}

//==============================================================================

const std::vector<rive::RawPath>& StyledText::getGlyphs() const
{
    return glyphPaths;
}

//==============================================================================

int StyledText::getNumParagraphs() const
{
    return static_cast<int> (paragraphs.size());
}

const rive::Paragraph& StyledText::getParagraph (int index) const
{
    if (isPositiveAndBelow (index, getNumParagraphs()))
        return paragraphs[index];

    static rive::Paragraph empty;
    return empty;
}

//==============================================================================

float StyledText::layoutText (const rive::GlyphRun& run,
                              unsigned startIndex,
                              unsigned endIndex,
                              rive::Vec2D origin)
{
    auto font = run.font.get();
    const auto scale = rive::Mat2D::fromScale (run.size, run.size);

    float x = origin.x;
    jassert (startIndex >= 0 && endIndex <= run.glyphs.size());

    int i, end, inc;
    if (run.dir() == rive::TextDirection::rtl)
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

    while (i != end)
    {
        auto trans = rive::Mat2D::fromTranslate (x, origin.y);
        x += run.advances[i];

        auto rawpath = font->getPath (run.glyphs[i]);
        rawpath.transformInPlace (trans * scale);

        glyphPaths.push_back (std::move (rawpath));

        i += inc;
    }

    return x;
}

float StyledText::layoutParagraph (const rive::Paragraph& paragraph,
                                   const rive::SimpleArray<rive::GlyphLine>& lines,
                                   rive::Vec2D origin)
{
    float y = origin.y;

    for (const auto& line : lines)
    {
        float x = line.startX + origin.x;

        int runIndex, endRun, runInc;
        if (paragraph.baseDirection() == rive::TextDirection::rtl)
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
            int endGIndex   = runIndex == line.endRunIndex   ? line.endGlyphIndex   : (int) run.glyphs.size();

            float nextX = layoutText (run,
                                      startGIndex,
                                      endGIndex,
                                      { x, y + line.baseline });

            x = nextX;
            runIndex += runInc;
        }

        // Move down for next line in the same paragraph
        y += (line.bottom - line.top); // line height
    }

    return y;
}

} // namespace yup
