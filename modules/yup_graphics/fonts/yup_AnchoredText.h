/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#pragma once

namespace yup
{

//==============================================================================
/** A single line of text placed from a point on its baseline, the way SVG text is.

    Unlike text fitted in a rectangle, the text keeps the size of its font and starts, is centered
    or ends at the anchor point, which lies on the baseline of the text.

    @code
    const AnchoredText title ("Volume", font, { 12.0f, 40.0f }, AnchoredText::Anchor::middle);
    g.fillFittedText (title.getStyledText(), title.getBounds());
    @endcode

    @see StyledText, Graphics::fillFittedText

    @tags{Graphics}
*/
class YUP_API AnchoredText
{
public:
    //==============================================================================
    /** The part of the text placed on the anchor point. */
    enum class Anchor
    {
        start,  ///< The text starts at the anchor point.
        middle, ///< The text is centered on the anchor point.
        end     ///< The text ends at the anchor point.
    };

    //==============================================================================
    /** Creates an empty text. */
    AnchoredText() = default;

    /** Lays out a line of text.

        @param text           The text, drawn on a single line.
        @param font           The font, whose height is the size of the text.
        @param anchorPoint    The point on the baseline the text is placed from.
        @param anchor         The part of the text placed on the anchor point.
        @param letterSpacing  Extra space added after every glyph, in the same units as the font height.
    */
    AnchoredText (StringRef text, const Font& font, Point<float> anchorPoint, Anchor anchor = Anchor::start, float letterSpacing = 0.0f);

    //==============================================================================
    /** Returns true if there is no text to draw. */
    bool isEmpty() const noexcept { return styledText == nullptr; }

    /** Returns the laid out text, to draw in getBounds(). Only valid when the text is not empty. */
    const StyledText& getStyledText() const noexcept;

    /** Returns the rectangle the text is drawn in, with its first line at the top left. */
    Rectangle<float> getBounds() const noexcept { return bounds; }

private:
    std::shared_ptr<const StyledText> styledText;
    Rectangle<float> bounds;
};

} // namespace yup
