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
                             float size,
                             float lineHeight,
                             const char text[])
{
    textRuns.push_back (append (font, size, lineHeight, text));

    jassert (font.getFont() != nullptr);
    paragraphs = font.getFont()->shapeText (unicodeChars, textRuns);
}

//==============================================================================

rive::TextRun StyledText::append (const Font& font,
                                  float size,
                                  float lineHeight,
                                  const char text[])
{
    const uint8_t* ptr = (const uint8_t*) text;
    uint32_t n = 0;

    while (*ptr != '\0')
    {
        unicodeChars.push_back (rive::UTF::NextUTF8 (&ptr));
        n += 1;
    }

    return { font.getFont(), size, lineHeight, 0.0f, n };
}

//==============================================================================

void StyledText::layout (const Rectangle<float>& rect, Alignment align)
{
    glyphPaths.clear();

    float x = rect.getX();
    float y = rect.getY();
    float paragraphWidth = rect.getWidth();
    float lineHeight = 11.0f;

    float totalTextHeight = paragraphs.size() * lineHeight;

    rive::SimpleArray<rive::SimpleArray<rive::GlyphLine>> linesArray (paragraphs.size());

    std::size_t paragraphIndex = 0;
    for (const auto& paragraph : paragraphs)
    {
        linesArray[paragraphIndex] = rive::GlyphLine::BreakLines (paragraph.runs, paragraphWidth);

        ++paragraphIndex;
    }

    paragraphIndex = 0;
    for (const auto& paragraph : paragraphs)
    {
        rive::SimpleArray<rive::GlyphLine>& lines = linesArray[paragraphIndex];

        rive::GlyphLine::ComputeLineSpacing (paragraphIndex == 0,
                                             lines,
                                             paragraph.runs,
                                             paragraphWidth,
                                             toTextAlign (align));

        y = layoutParagraph (paragraph,
                             lines,
                             { x, y });

        y += lineHeight;

        ++paragraphIndex;
    }
}

//==============================================================================

const std::vector<rive::RawPath>& StyledText::getGlyphs() const
{
    return glyphPaths;
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

            x = layoutText (run,
                            startGIndex,
                            endGIndex,
                            { x, origin.y + line.baseline });

            runIndex += runInc;
        }
    }

    return origin.y + lines.back().bottom;
}

} // namespace yup
