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

const Identifier TextButton::Style::backgroundColorId = "textButtonBackground";
const Identifier TextButton::Style::backgroundPressedColorId = "textButtonBackgroundPressed";
const Identifier TextButton::Style::textColorId = "textButtonText";
const Identifier TextButton::Style::textPressedColorId = "textButtonTextPressed";
const Identifier TextButton::Style::outlineColorId = "textButtonOutline";
const Identifier TextButton::Style::outlineFocusedColorId = "textButtonOutlineFocused";
const Identifier TextButton::Style::cornerRadiusId = "textButtonCornerRadius";
const Identifier TextButton::Style::outlineWidthId = "textButtonOutlineWidth";
const Identifier TextButton::Style::focusedOutlineWidthId = "textButtonFocusedOutlineWidth";

//==============================================================================

const ComponentTypeInfo& TextButton::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<TextButton> (Identifier ("TextButton"), Button::getClassTypeInfo())
                                 .color ("backgroundColor", Style::backgroundColorId)
                                 .color ("backgroundPressedColor", Style::backgroundPressedColorId)
                                 .color ("textColor", Style::textColorId)
                                 .color ("textPressedColor", Style::textPressedColorId)
                                 .color ("outlineColor", Style::outlineColorId)
                                 .color ("outlineFocusedColor", Style::outlineFocusedColorId)
                                 .number ("cornerRadius", Style::cornerRadiusId, 4.0, "The corner radius.")
                                 .number ("outlineWidth", Style::outlineWidthId, 1.0, "The outline width.")
                                 .number ("focusedOutlineWidth", Style::focusedOutlineWidthId, 2.0, "The outline width when focused.")
                                 .property (textPropertyId, &TextButton::getButtonText, &TextButton::setButtonText)
                                 .build();

    return info;
}

//==============================================================================

TextButton::TextButton (StringRef componentID)
    : Button (componentID)
    , buttonText (componentID)
{
    setOpaque (false);
}

//==============================================================================

void TextButton::setButtonText (StringRef newButtonText)
{
    if (buttonText != newButtonText)
    {
        buttonText = newButtonText;

        updateTextLayout();
        notifyPropertyChanged (textPropertyId);
    }
}

//==============================================================================

void TextButton::paintButton (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================

void TextButton::resized()
{
    updateTextLayout();
}

//==============================================================================

void TextButton::updateTextLayout()
{
    auto textBounds = getTextBounds();
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();

    auto modifier = styledText.startUpdate();
    modifier.setMaxSize (textBounds.getSize());
    modifier.setHorizontalAlign (StyledText::center);
    modifier.setVerticalAlign (StyledText::middle);
    modifier.setWrap (StyledText::noWrap);
    modifier.setOverflow (StyledText::ellipsis);
    modifier.clear();

    if (buttonText.isNotEmpty())
        modifier.appendText (buttonText, font.withHeight (jmin (font.getHeight(), getHeight() * 0.6f)));

    repaint();
}

//==============================================================================

Rectangle<float> TextButton::getTextBounds() const
{
    return getLocalBounds().reduced (proportionOfWidth (0.04f), proportionOfHeight (0.04f));
}

} // namespace yup
