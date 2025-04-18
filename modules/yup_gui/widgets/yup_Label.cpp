/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

Label::Label()
{
}

//==============================================================================

String Label::getText() const
{
    return text;
}

void Label::setText (String newText, NotificationType notification)
{
    //if (text == newText)
    //    return;

    text = std::move (newText);

    prepareText();
}

//==============================================================================

const Font& Label::getFont() const
{
    return font;
}

void Label::setFont (Font newFont)
{
    if (font == newFont)
        return;

    font = newFont;

    prepareText();
}

//==============================================================================

void Label::paint (Graphics& g)
{
    if (! strokeColor.isTransparent() && strokeWidth > 0.0f)
    {
        g.setStrokeColor (strokeColor);
        g.setStrokeWidth (strokeWidth);
        g.strokeFittedText (styledText, getLocalBounds());
    }

    g.setFillColor (fillColor);
    g.fillFittedText (styledText, getLocalBounds());

    //g.setStrokeColor (0xffff0000);
    //g.strokeRect (getLocalBounds());
}

//==============================================================================

void Label::resized()
{
    prepareText();
}

//==============================================================================

void Label::prepareText()
{
    auto fontSize = getHeight() * 0.8f;

    styledText.setMaxSize (getSize());
    styledText.setHorizontalAlign (StyledText::left);
    styledText.setVerticalAlign (StyledText::middle);
    styledText.setOverflow (StyledText::ellipsis);
    styledText.setWrap (StyledText::noWrap);

    styledText.clear();

    if (text.isNotEmpty())
    {
        styledText.appendText (text, nullptr, font.getFont(), fontSize);
        styledText.update();
    }

    repaint();
}

} // namespace yup
