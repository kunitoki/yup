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

    enum HorizontalAlign : uint8_t
    {
        left,
        center,
        right
    };

    enum VerticalAlign : uint8_t
    {
        top,
        middle,
        bottom
    };

    enum TextDirection : uint8_t
    {
        ltr = 0,
        rtl = 1
    };

    enum TextOverflow : uint8_t
    {
        visible,
        hidden,
        ellipsis,
        fit
    };

    enum TextOrigin : uint8_t
    {
        topOrigin,
        baseline
    };

    enum TextWrap : uint8_t
    {
        wrap = 0,
        noWrap = 1
    };

    //==============================================================================

    StyledText();

    //==============================================================================

    bool isEmpty() const;

    //==============================================================================

    void clear();

    //==============================================================================

    void appendText (StringRef text,
                     rive::rcp<rive::RenderPaint> paint,
                     const Font& font,
                     float fontSize = 16.0f,
                     float lineHeight = -1.0f,
                     float letterSpacing = 0.0f);

    //==============================================================================

    void update();

    //==============================================================================

    TextOverflow getOverflow() const;
    void setOverflow (TextOverflow value);

    HorizontalAlign getHorizontalAlign() const;
    void setHorizontalAlign (HorizontalAlign value);

    VerticalAlign getVerticalAlign() const;
    void setVerticalAlign (VerticalAlign value);

    Size<float> getMaxSize() const;
    void setMaxSize (const Size<float>& value);

    float getParagraphSpacing() const;
    void setParagraphSpacing (float value);

    TextWrap getWrap() const;
    void setWrap (TextWrap value);

    //==============================================================================

    Rectangle<float> getBounds();

    //==============================================================================

    const std::vector<rive::OrderedLine>& getOrderedLines();

    //==============================================================================

    struct RenderStyle
    {
        RenderStyle (
            rive::rcp<rive::RenderPaint> paint,
            rive::rcp<rive::RenderPath> path,
            bool isEmpty)
            : paint (std::move (paint))
            , path (std::move (path))
            , isEmpty (isEmpty)
        {
        }

        rive::rcp<rive::RenderPaint> paint;
        rive::rcp<rive::RenderPath> path;
        bool isEmpty;
    };

    const std::vector<RenderStyle*>& getRenderStyles();

private:
    rive::SimpleArray<rive::Paragraph> shape;
    rive::SimpleArray<rive::SimpleArray<rive::GlyphLine>> lines;
    std::vector<rive::OrderedLine> orderedLines;
    rive::GlyphRun ellipsisRun;
    rive::StyledText styledTexts;
    std::vector<RenderStyle> styles;
    std::vector<RenderStyle*> renderStyles;

    TextOrigin origin = TextOrigin::topOrigin;
    TextOverflow overflow = TextOverflow::visible;
    HorizontalAlign horizontalAlign = HorizontalAlign::left;
    VerticalAlign verticalAlign = VerticalAlign::middle;
    TextWrap textWrap = TextWrap::wrap;
    Size<float> maxSize = { -1.0f, -1.0f };
    float paragraphSpacing = 0.0f;
    Rectangle<float> bounds;
    bool isDirty = false;
};

} // namespace yup
