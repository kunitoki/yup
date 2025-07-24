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

const Identifier Label::Style::textFillColorId { "Label_textFillColorId" };
const Identifier Label::Style::textStrokeColorId { "Label_textStrokeColorId" };
const Identifier Label::Style::backgroundColorId { "Label_backgroundColorId" };
const Identifier Label::Style::outlineColorId { "Label_outlineColorId" };

//==============================================================================

Label::Label (StringRef componentID)
    : Component (componentID)
{
    setOpaque (false);
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
    auto fontToUse = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (fontSize);
    if (font)
        fontToUse = *font;

    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (getSize());
        modifier.setHorizontalAlign (StyledText::left);
        modifier.setVerticalAlign (StyledText::middle);
        modifier.setOverflow (StyledText::ellipsis);
        modifier.setWrap (StyledText::noWrap);

        modifier.clear();

        if (text.isNotEmpty())
            modifier.appendText (text, fontToUse);
    }

    needsUpdate = false;
}

void Label::invalidateCache()
{
    needsUpdate = true;
    repaint();
}

} // namespace yup
