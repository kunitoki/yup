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

class JUCE_API StyledText
{
public:
    //==============================================================================
    enum Alignment
    {
        left,
        center,
        right
    };

    enum VerticalAlignment
    {
        top,
        middle,
        bottom
    };

    //==============================================================================
    StyledText();

    //==============================================================================
    void clear();

    //==============================================================================
    void appendText (const Font& font,
                     float fontSize,
                     float lineHeight,
                     StringRef text);

    //==============================================================================
    void layout (const Rectangle<float>& rect, Alignment hAlign = center, VerticalAlignment vAlign = top);

    Path getGlyphsPath (const AffineTransform& transform) const;

    //==============================================================================
    /** @internal */
    const std::vector<rive::RawPath>& getGlyphs() const;
    /** @internal */
    int getNumParagraphs() const;
    /** @internal */
    const rive::Paragraph& getParagraph (int index) const;

private:
    rive::TextRun append (const Font& font,
                          float fontSize,
                          float lineHeight,
                          StringRef text);

    float layoutText (const rive::GlyphRun& run,
                      unsigned startIndex,
                      unsigned endIndex,
                      rive::Vec2D origin);

    float layoutParagraph (const rive::Paragraph& paragraph,
                           const rive::SimpleArray<rive::GlyphLine>& lines,
                           rive::Vec2D origin);

    std::vector<rive::Unichar> unicodeChars;
    rive::SimpleArray<rive::Paragraph> paragraphs;
    std::vector<rive::TextRun> textRuns;
    std::vector<rive::RawPath> glyphPaths;
};

} // namespace yup
