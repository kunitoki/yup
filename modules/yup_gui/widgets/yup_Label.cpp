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

const Identifier Label::Colors::fillColorId{ "Label_fillColorId" };
const Identifier Label::Colors::strokeColorId{ "Label_strokeColorId" };

//==============================================================================

Label::Label (StringRef componentID)
    : Component (componentID)
{
}

//==============================================================================

String Label::getText() const
{
    return text;
}

void Label::setText (String newText, NotificationType notification)
{
    if (text == newText)
        return;

    text = std::move (newText);
    invalidateCache();

    if (notification != dontSendNotification)
        // TODO: Send notification
        ;
}

//==============================================================================

std::optional<Font> Label::getFont() const
{
    return font;
}

void Label::setFont (Font newFont)
{
    if (font && *font == newFont)
        return;

    font = std::move (newFont);
    invalidateCache();
}

void Label::resetFont()
{
    font.reset();
}

//==============================================================================


void Label::setStrokeWidth (float newWidth) noexcept
{
    if (strokeWidth == newWidth)
        return;

    strokeWidth = newWidth;
    repaint();
}

//==============================================================================

void Label::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
    {
        if (needsUpdate)
            prepareText();

        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
    }
}

//==============================================================================

void Label::resized()
{
    invalidateCache();
}

//==============================================================================

void Label::prepareText()
{
    if (! needsUpdate)
        return;

    auto fontSize = getHeight() * 0.8f; // TODO - needs config

    styledText.setMaxSize (getSize());

    // TODO - all these needs config
    styledText.setHorizontalAlign (StyledText::left);
    styledText.setVerticalAlign (StyledText::middle);
    styledText.setOverflow (StyledText::ellipsis);
    styledText.setWrap (StyledText::noWrap);

    styledText.clear();

    if (text.isNotEmpty())
    {
        if (! font)
            font = ApplicationTheme::getGlobalTheme()->getDefaultFont();

        styledText.appendText (text, nullptr, font->getFont(), fontSize);
        styledText.update();
    }

    needsUpdate = false;
}

void Label::invalidateCache()
{
    needsUpdate = true;
    repaint();
}

} // namespace yup
