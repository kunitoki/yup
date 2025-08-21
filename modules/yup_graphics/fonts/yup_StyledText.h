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

class YUP_API StyledText
{
public:
    //==============================================================================

    enum HorizontalAlign : uint8_t
    {
        left,
        center,
        right,
        justified
    };

    enum VerticalAlign : uint8_t
    {
        top,
        middle,
        bottom
    };

    enum TextOverflow : uint8_t
    {
        visible,
        ellipsis
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

    bool needsUpdate() const;

    //==============================================================================

    struct TextModifier
    {
        TextModifier (StyledText& styledText);
        ~TextModifier();

        void clear();

        void appendText (StringRef text,
                         const Font& font,
                         float lineHeight = -1.0f,
                         float letterSpacing = 0.0f);

        void appendText (StringRef text,
                         rive::rcp<rive::RenderPaint> paint,
                         const Font& font,
                         float lineHeight = -1.0f,
                         float letterSpacing = 0.0f);

        void setOverflow (TextOverflow value);

        void setHorizontalAlign (HorizontalAlign value);

        void setVerticalAlign (VerticalAlign value);

        void setMaxSize (const Size<float>& value);

        void setParagraphSpacing (float value);

        void setWrap (TextWrap value);

    private:
        StyledText& styledText;
    };

    TextModifier startUpdate();

    //==============================================================================

    TextOverflow getOverflow() const;

    HorizontalAlign getHorizontalAlign() const;

    VerticalAlign getVerticalAlign() const;

    Size<float> getMaxSize() const;

    float getParagraphSpacing() const;

    TextWrap getWrap() const;

    //==============================================================================

    Rectangle<float> getComputedTextBounds() const;

    Point<float> getOffset (const Rectangle<float>& area) const;

    //==============================================================================

    Span<const rive::OrderedLine> getOrderedLines() const;

    //==============================================================================

    struct RenderStyle
    {
        RenderStyle (rive::rcp<rive::RenderPaint> paint, rive::rcp<rive::RenderPath> path, bool isEmpty)
            : paint (std::move (paint))
            , path (std::move (path))
            , isEmpty (isEmpty)
        {
        }

        rive::rcp<rive::RenderPaint> paint;
        rive::rcp<rive::RenderPath> path;
        bool isEmpty;
    };

    Span<const RenderStyle* const> getRenderStyles() const;

    //==============================================================================

    /** Find the glyph index at a given position in the text area.

        @param position The position to check

        @returns The glyph index at the position, or -1 if not found
    */
    int getGlyphIndexAtPosition (const Point<float>& position) const;

    /** Get the bounds of the caret at a given character position.

        @param characterIndex The character index to get bounds for

        @returns Rectangle representing the caret bounds
    */
    Rectangle<float> getCaretBounds (int characterIndex) const;

    /** Returns all selection rectangles for multiline selections.

        @param startIndex   The start character index
        @param endIndex     The end character index
        @returns            A vector of rectangles representing the selection
    */
    std::vector<Rectangle<float>> getSelectionRectangles (int startIndex, int endIndex) const;

    /** Validates if a character index is within valid bounds.

        @param characterIndex   The character index to validate
        @returns                True if the index is valid
    */
    bool isValidCharacterIndex (int characterIndex) const;

    //==============================================================================

    static HorizontalAlign horizontalAlignFromJustification (Justification justification);
    static VerticalAlign verticalAlignFromJustification (Justification justification);

private:
    friend class TextModifier;

    void clear();

    void appendText (StringRef text,
                     rive::rcp<rive::RenderPaint> paint,
                     const Font& font,
                     float lineHeight,
                     float letterSpacing);

    void setOverflow (TextOverflow value);
    void setHorizontalAlign (HorizontalAlign value);
    void setVerticalAlign (VerticalAlign value);
    void setMaxSize (const Size<float>& value);
    void setParagraphSpacing (float value);
    void setWrap (TextWrap value);

    void update();

    rive::SimpleArray<rive::Paragraph> shape;
    rive::SimpleArray<rive::SimpleArray<rive::GlyphLine>> lines;
    std::vector<rive::OrderedLine> orderedLines;
    rive::GlyphRun ellipsisRun;
    rive::StyledText styledTexts;
    std::vector<RenderStyle> styles;
    std::vector<RenderStyle*> renderStyles;
    rive::GlyphLookup glyphLookup;

    TextOrigin origin = TextOrigin::topOrigin;
    TextOverflow overflow = TextOverflow::visible;
    HorizontalAlign horizontalAlign = HorizontalAlign::left;
    VerticalAlign verticalAlign = VerticalAlign::top;
    TextWrap textWrap = TextWrap::wrap;
    Size<float> maxSize = { -1.0f, -1.0f };
    float paragraphSpacing = 0.0f;
    Rectangle<float> bounds;
    bool isDirty = false;
};

} // namespace yup
