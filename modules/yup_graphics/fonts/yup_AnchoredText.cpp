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

namespace yup
{

//==============================================================================
AnchoredText::AnchoredText (StringRef text, const Font& font, Point<float> anchorPoint, Anchor anchor, float letterSpacing)
{
    if (text.isEmpty())
        return;

    const auto fontSize = font.getHeight();
    auto shaped = std::make_shared<StyledText>();

    {
        auto modifier = shaped->startUpdate();
        modifier.setMaxSize (Size<float> (jmax (fontSize, static_cast<float> (text.length()) * fontSize * 2.0f), fontSize * 4.0f));
        modifier.setWrap (StyledText::noWrap);
        modifier.setHorizontalAlign (StyledText::left);
        modifier.setVerticalAlign (StyledText::top);
        modifier.appendText (text, font, -1.0f, letterSpacing);
    }

    const auto computedTextBounds = shaped->getComputedTextBounds();
    const auto fontAscent = font.getAscent();
    const auto fontDescent = font.getDescent();
    const auto hasUsableFontMetrics = fontAscent < 0.0f && fontDescent > fontAscent;
    const auto ascent = hasUsableFontMetrics ? fontAscent : -0.8f;
    const auto descent = hasUsableFontMetrics ? fontDescent : 0.2f;
    const auto metricsHeight = (descent - ascent) * fontSize;
    const auto textWidth = jmax (fontSize, computedTextBounds.getWidth());
    const auto textHeight = jmax (computedTextBounds.getHeight(), metricsHeight);

    auto textX = anchorPoint.getX();
    if (anchor == Anchor::middle)
        textX -= textWidth * 0.5f;
    else if (anchor == Anchor::end)
        textX -= textWidth;

    styledText = std::move (shaped);
    bounds = Rectangle<float> (textX, anchorPoint.getY() + (ascent * fontSize), textWidth, textHeight + fontSize * 0.25f);
}

const StyledText& AnchoredText::getStyledText() const noexcept
{
    jassert (styledText != nullptr);
    return *styledText;
}

} // namespace yup
