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

const Identifier ToggleButton::Style::backgroundColorId = "toggleButtonBackground";
const Identifier ToggleButton::Style::backgroundToggledColorId = "toggleButtonBackgroundToggled";
const Identifier ToggleButton::Style::textColorId = "toggleButtonText";
const Identifier ToggleButton::Style::textToggledColorId = "toggleButtonTextToggled";
const Identifier ToggleButton::Style::borderColorId = "toggleButtonBorder";
const Identifier ToggleButton::Style::borderToggledColorId = "toggleButtonBorderToggled";

//==============================================================================

ToggleButton::ToggleButton (StringRef componentID)
    : Button (componentID)
{
    setOpaque (false);
}

//==============================================================================

void ToggleButton::setToggleState (bool shouldBeToggled, NotificationType notification)
{
    if (toggleState != shouldBeToggled)
    {
        toggleState = shouldBeToggled;

        if (notification != dontSendNotification)
        {
            toggleStateChanged();

            if (onClick)
                onClick();
        }

        repaint();
    }
}

void ToggleButton::setButtonText (String newText)
{
    if (buttonText != newText)
    {
        buttonText = newText;
        resized();
    }
}

//==============================================================================

void ToggleButton::paintButton (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================

void ToggleButton::resized()
{
    if (buttonText.isNotEmpty())
    {
        auto bounds = getLocalBounds();
        auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();

        {
            auto modifier = styledText.startUpdate();
            modifier.setMaxSize (bounds.getSize());
            modifier.setHorizontalAlign (StyledText::center);
            modifier.setVerticalAlign (StyledText::middle);
            modifier.clear();
            modifier.appendText (buttonText, nullptr, font, 16.0f);
        }
    }
}

//==============================================================================

void ToggleButton::mouseUp (const MouseEvent& event)
{
    if (getLocalBounds().contains (event.getPosition()))
        setToggleState (! toggleState, sendNotification);

    Button::mouseUp (event);
}

//==============================================================================

void ToggleButton::focusGained()
{
    hasFocus = true;
    repaint();
}

void ToggleButton::focusLost()
{
    hasFocus = false;
    repaint();
}

} // namespace yup
