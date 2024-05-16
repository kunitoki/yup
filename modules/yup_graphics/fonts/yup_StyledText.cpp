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

StyledText::StyledText()
{
}

//==============================================================================

void StyledText::clear()
{
    unicodeChars.clear();
    textRuns.clear();
    paragraphs = rive::SimpleArray<rive::Paragraph>{};
}

//==============================================================================

void StyledText::appendText (const Font& font,
                             float size,
                             float lineHeight,
                             const char text[])
{
    textRuns.push_back (append (font, size, lineHeight, text));

    paragraphs = font.getFont()->shapeText (unicodeChars, textRuns);
}

//==============================================================================

rive::TextRun StyledText::append (const Font& font,
                                  float size,
                                  float lineHeight,
                                  const char text[])
{
    const uint8_t* ptr = (const uint8_t*)text;
    uint32_t n = 0;

    while (*ptr != '\0')
    {
        unicodeChars.push_back (rive::UTF::NextUTF8 (&ptr));
        n += 1;
    }

    return { font.getFont(), size, lineHeight, 0.0f, n };
}

} // namespace yup
