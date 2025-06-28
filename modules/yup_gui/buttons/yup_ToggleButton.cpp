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
const Identifier ToggleButton::Colors::backgroundColorId = "toggleButtonBackground";
const Identifier ToggleButton::Colors::backgroundToggledColorId = "toggleButtonBackgroundToggled";
const Identifier ToggleButton::Colors::textColorId = "toggleButtonText";
const Identifier ToggleButton::Colors::textToggledColorId = "toggleButtonTextToggled";
const Identifier ToggleButton::Colors::borderColorId = "toggleButtonBorder";
const Identifier ToggleButton::Colors::borderToggledColorId = "toggleButtonBorderToggled";

//==============================================================================
ToggleButton::ToggleButton (StringRef componentID)
    : Button (componentID)
{
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
    auto bounds = getLocalBounds();

    // Get colors based on toggle state
    auto bgColor = toggleState
                     ? findColor (Colors::backgroundToggledColorId).value_or (Color (0xff4a90e2))
                     : findColor (Colors::backgroundColorId).value_or (Color (0xfff0f0f0));

    auto textColor = toggleState
                       ? findColor (Colors::textToggledColorId).value_or (Color (0xffffffff))
                       : findColor (Colors::textColorId).value_or (Color (0xff333333));

    auto borderColor = toggleState
                         ? findColor (Colors::borderToggledColorId).value_or (Color (0xff357abd))
                         : findColor (Colors::borderColorId).value_or (Color (0xffcccccc));

    // Adjust colors for button state
    if (isButtonDown())
    {
        bgColor = bgColor.darker (0.1f);
        borderColor = borderColor.darker (0.1f);
    }
    else if (isButtonOver())
    {
        bgColor = bgColor.brighter (0.05f);
        borderColor = borderColor.brighter (0.05f);
    }

    // Draw background
    g.setFillColor (bgColor);
    g.fillRoundedRect (bounds, 4.0f);

    // Draw border
    g.setStrokeColor (borderColor);
    g.setStrokeWidth (hasFocus ? 2.0f : 1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);

    // Draw text
    if (buttonText.isNotEmpty())
    {
        g.setFillColor (textColor);
        g.fillFittedText (styledText, bounds);
    }
}

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

void ToggleButton::mouseUp (const MouseEvent& event)
{
    if (getLocalBounds().contains (event.getPosition()))
        setToggleState (! toggleState, sendNotification);

    Button::mouseUp (event);
}

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
