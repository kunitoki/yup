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
    StyledText();

    void clear();

    void appendText (const Font& font,
                     float size,
                     float lineHeight,
                     const char text[]);

    rive::SimpleArray<rive::Paragraph>& getParagraphs()
    {
        return paragraphs;
    }

    const rive::SimpleArray<rive::Paragraph>& getParagraphs() const
    {
        return paragraphs;
    }

    std::vector<rive::TextRun>& getTextRuns()
    {
        return textRuns;
    }

    const std::vector<rive::TextRun>& getTextRuns() const
    {
        return textRuns;
    }

private:
    rive::TextRun append (const Font& font,
                          float size,
                          float lineHeight,
                          const char text[]);

    std::vector<rive::Unichar> unicodeChars;
    rive::SimpleArray<rive::Paragraph> paragraphs;
    std::vector<rive::TextRun> textRuns;
};

} // namespace yup
