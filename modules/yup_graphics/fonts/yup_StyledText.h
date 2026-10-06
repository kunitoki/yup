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
/**
    A block of text in one or more fonts and colors, shaped, wrapped and laid out ready to draw.

    The text is built through a TextModifier from startUpdate(), which shapes and lays it out
    again when it goes out of scope:

    @code
    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ 300.0f, -1.0f });
        modifier.appendText ("Hello ", font);
        modifier.appendText ("world", Colors::orange, font.withHeight (24.0f));
    }

    g.fillFittedText (text, area);
    @endcode

    Shaping follows the Unicode rules for bidirectional text and line breaking. Characters
    missing from a run's font are shaped with Font::getColorEmojiFallbackFont(), and color
    glyphs (emoji) are drawn in their own colors.

    Keep a StyledText around while its content is unchanged: laying it out is the expensive
    part, drawing it again is cheap.

    @see Graphics::fillFittedText, Graphics::strokeFittedText

    @tags{Graphics}
*/
class YUP_API StyledText
{
public:
    //==============================================================================
    /** How lines are placed horizontally within the maximum width. */
    enum HorizontalAlign : uint8_t
    {
        left,     ///< Lines start at the left edge.
        center,   ///< Lines are centered.
        right,    ///< Lines end at the right edge.
        justified ///< Lines fill the width by widening the gaps between glyphs, except the last line.
    };

    /** How the text block is placed vertically within the area it is drawn into. */
    enum VerticalAlign : uint8_t
    {
        top,    ///< The text starts at the top.
        middle, ///< The text is centered vertically.
        bottom  ///< The text ends at the bottom.
    };

    /** What happens to text that does not fit within the maximum height. */
    enum TextOverflow : uint8_t
    {
        visible, ///< All lines are laid out, and may extend past the maximum height.
        ellipsis ///< The last line that fits ends with an ellipsis, and the text is clipped to its area when drawn.
    };

    /** Where the y coordinate 0 of the laid out text sits. */
    enum TextOrigin : uint8_t
    {
        topOrigin, ///< At the top of the first line.
        baseline   ///< At the baseline of the first line.
    };

    /** Whether lines wrap at the maximum width. */
    enum TextWrap : uint8_t
    {
        wrap = 0,  ///< Lines break at the maximum width, at word boundaries where possible.
        noWrap = 1 ///< Lines only break at newlines.
    };

    //==============================================================================
    /** Creates an empty text. */
    StyledText();

    //==============================================================================
    /** Returns true if no text has been appended. */
    bool isEmpty() const;

    /** Returns true if the text changed since it was last laid out.

        The layout queries and drawing need an up to date text: this is only true while a
        TextModifier is still alive.
    */
    bool needsUpdate() const;

    //==============================================================================
    /**
        Changes the content and layout settings of a StyledText.

        Obtain one with StyledText::startUpdate(). Changes are collected while the modifier
        lives, and the text is shaped and laid out once when it is destroyed.
    */
    struct TextModifier
    {
        /** Starts changing the given text. */
        TextModifier (StyledText& styledText);

        /** Shapes and lays out the text with all the changes made. */
        ~TextModifier();

        /** Removes all the text and its styles. */
        void clear();

        /** Appends text drawn with the Graphics fill or stroke when the text is drawn.

            @param text          The UTF-8 text to append. Newlines start new paragraphs.
            @param font          The font, whose height is the size of the text.
            @param lineHeight    The distance between baselines, or a negative value for the font's own line height.
            @param letterSpacing Extra space added after every glyph, in the same units as the font height.
        */
        void appendText (StringRef text,
                         const Font& font,
                         float lineHeight = -1.0f,
                         float letterSpacing = 0.0f);

        /** Appends text drawn in its own color, whatever the Graphics fill color is.

            @param text          The UTF-8 text to append. Newlines start new paragraphs.
            @param color         The color of this text.
            @param font          The font, whose height is the size of the text.
            @param lineHeight    The distance between baselines, or a negative value for the font's own line height.
            @param letterSpacing Extra space added after every glyph, in the same units as the font height.
        */
        void appendText (StringRef text,
                         Color color,
                         const Font& font,
                         float lineHeight = -1.0f,
                         float letterSpacing = 0.0f);

        /** @internal Appends text drawn with its own Rive paint. */
        void appendText (StringRef text,
                         rive::rcp<rive::RenderPaint> paint,
                         const Font& font,
                         float lineHeight = -1.0f,
                         float letterSpacing = 0.0f);

        /** Sets what happens to text that does not fit within the maximum height. */
        void setOverflow (TextOverflow value);

        /** Sets how lines are placed horizontally. */
        void setHorizontalAlign (HorizontalAlign value);

        /** Sets how the text block is placed vertically in the area it is drawn into. */
        void setVerticalAlign (VerticalAlign value);

        /** Sets the size the text is laid out in.

            A negative width disables wrapping. The height decides which lines fit when the
            overflow is TextOverflow::ellipsis.
        */
        void setMaxSize (const Size<float>& value);

        /** Sets the extra vertical space added after each paragraph. */
        void setParagraphSpacing (float value);

        /** Sets whether lines wrap at the maximum width. */
        void setWrap (TextWrap value);

    private:
        StyledText& styledText;
    };

    /** Returns a modifier to change the text: the text is laid out again when it is destroyed. */
    TextModifier startUpdate();

    //==============================================================================
    /** Returns what happens to text that does not fit within the maximum height. */
    TextOverflow getOverflow() const;

    /** Returns how lines are placed horizontally. */
    HorizontalAlign getHorizontalAlign() const;

    /** Returns how the text block is placed vertically. */
    VerticalAlign getVerticalAlign() const;

    /** Returns the size the text is laid out in, with negative values for no limit. */
    Size<float> getMaxSize() const;

    /** Returns the extra vertical space added after each paragraph. */
    float getParagraphSpacing() const;

    /** Returns whether lines wrap at the maximum width. */
    TextWrap getWrap() const;

    //==============================================================================
    /** Returns the bounds of the laid out text, relative to the text origin. */
    Rectangle<float> getComputedTextBounds() const;

    /** Returns where the text block sits within an area, following its alignment.

        @param area The area the text is drawn into.
        @returns    The offset of the text from the top left of the area.
    */
    Point<float> getOffset (const Rectangle<float>& area) const;

    //==============================================================================
    /** Returns the number of color glyphs (emoji) in the text, drawn in their own colors. */
    int getNumColorGlyphs() const;

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

    /** Returns the character index at the same horizontal position on an adjacent visual line.

        This uses the shaped and wrapped line data, so soft-wrapped lines are treated the same
        as explicit newline-separated lines.

        @param characterIndex   The current caret character index
        @param moveDown         True to move to the next visual line, false to move to the previous visual line

        @returns The character index on the adjacent visual line, or the nearest valid boundary
    */
    int getGlyphIndexOnAdjacentLine (int characterIndex, bool moveDown) const;

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
    /** Returns the horizontal alignment matching a Justification, left when it has none. */
    static HorizontalAlign horizontalAlignFromJustification (Justification justification);

    /** Returns the vertical alignment matching a Justification, middle when it has none. */
    static VerticalAlign verticalAlignFromJustification (Justification justification);

    //==============================================================================
    /** @internal Returns the laid out lines, in visual order. */
    Span<const rive::OrderedLine> getOrderedLines() const;

    /** @internal The outlines of all the glyphs drawn with one paint. */
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

    /** @internal Returns the outlines to draw, one per paint that has glyphs. */
    Span<const RenderStyle* const> getRenderStyles() const;

private:
    friend class TextModifier;
    friend class Graphics;

    struct ColorGlyphLayer
    {
        rive::Font::ColorGlyphLayer layer;
        Image image;

        // Built by Graphics on the first draw, with the factory that drew it
        mutable rive::Factory* preparedFactory = nullptr;
        mutable rive::rcp<rive::RenderPath> renderPath;
        mutable rive::rcp<rive::RenderPaint> renderPaint;
    };

    struct ColorGlyph
    {
        std::shared_ptr<const std::vector<ColorGlyphLayer>> layers;
        rive::Mat2D transform;
        std::size_t styleIndex = 0;
    };

    void clear();

    void appendText (StringRef text,
                     rive::rcp<rive::RenderPaint> paint,
                     const Font& font,
                     float lineHeight,
                     float letterSpacing);

    void appendText (StringRef text,
                     Color color,
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
    int findParagraphNewlinePosition (int orderedLineIndex) const;
    int findParagraphNewlinePositionByIndex (int paragraphIndex) const;
    std::shared_ptr<const std::vector<ColorGlyphLayer>> findColorGlyphLayers (const rive::rcp<rive::Font>& font, rive::GlyphID glyphId);
    static std::shared_ptr<const std::vector<ColorGlyphLayer>> decodeColorGlyphLayers (const rive::rcp<rive::Font>& font, rive::GlyphID glyphId);

    rive::SimpleArray<rive::Paragraph> shape;
    rive::SimpleArray<rive::SimpleArray<rive::GlyphLine>> lines;
    std::vector<rive::OrderedLine> orderedLines;
    rive::GlyphRun ellipsisRun;
    rive::StyledText styledTexts;
    std::vector<RenderStyle> styles;
    std::vector<RenderStyle*> renderStyles;
    std::vector<ColorGlyph> colorGlyphs;
    rive::GlyphLookup glyphLookup;
    std::unordered_map<uint32_t, rive::rcp<rive::RenderPaint>> colorPaints;

    std::unordered_map<const rive::Font*, std::unordered_map<rive::GlyphID, std::shared_ptr<const std::vector<ColorGlyphLayer>>>> colorGlyphLayerCache;

    TextOrigin origin = TextOrigin::topOrigin;
    TextOverflow overflow = TextOverflow::visible;
    HorizontalAlign horizontalAlign = HorizontalAlign::left;
    VerticalAlign verticalAlign = VerticalAlign::top;
    TextWrap textWrap = TextWrap::wrap;
    Size<float> maxSize = { -1.0f, -1.0f };
    float paragraphSpacing = 0.0f;
    Rectangle<float> bounds;
    bool isDirty = false;
    std::vector<float> paragraphYOffsets;
    std::vector<std::vector<float>> lineGlyphX;
    std::vector<float> lineEndX;
    float defaultLineHeight = 0.0f;
};

} // namespace yup
