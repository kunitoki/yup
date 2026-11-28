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
const Identifier ToggleButton::Style::cornerRadiusId = "toggleButtonCornerRadius";
const Identifier ToggleButton::Style::outlineWidthId = "toggleButtonOutlineWidth";
const Identifier ToggleButton::Style::focusedOutlineWidthId = "toggleButtonFocusedOutlineWidth";
const Identifier ToggleButton::Style::textPartId = "text";

//==============================================================================

const ComponentTypeInfo& ToggleButton::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<ToggleButton> (Identifier ("ToggleButton"), Button::getClassTypeInfo())
                                 .color ("backgroundColor", Style::backgroundColorId)
                                 .color ("backgroundToggledColor", Style::backgroundToggledColorId)
                                 .color ("textColor", Style::textColorId)
                                 .color ("textToggledColor", Style::textToggledColorId)
                                 .color ("borderColor", Style::borderColorId)
                                 .color ("borderToggledColor", Style::borderToggledColorId)
                                 .number ("cornerRadius", Style::cornerRadiusId, 4.0, "The corner radius.")
                                 .number ("outlineWidth", Style::outlineWidthId, 1.0, "The outline width.")
                                 .number ("focusedOutlineWidth", Style::focusedOutlineWidthId, 2.0, "The outline width when focused.")
                                 .state (checkedPropertyId, &ToggleButton::getToggleState, [] (ToggleButton& b, bool v) { b.setToggleState (v); })
                                 .property (textPropertyId, &ToggleButton::getButtonText, &ToggleButton::setButtonText)
                                 .fixedPart (Style::textPartId, "The text as the button laid it out, filled with the fill color.")
                                 .build();

    return info;
}

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

        auto bailOutChecker = BailOutChecker (this);

        notifyPropertyChanged (checkedPropertyId);
        if (bailOutChecker.shouldBailOut())
            return;

        if (toggleState)
        {
            turnOffOtherButtonsInGroup();
            if (bailOutChecker.shouldBailOut())
                return;
        }

        sendChangeNotification (notification, [this]
        {
            auto innerBailOut = BailOutChecker (this);

            toggleStateChanged();
            if (innerBailOut.shouldBailOut())
                return;

            if (onClick)
                onClick();
        });

        if (! bailOutChecker.shouldBailOut())
            repaint();
    }
}

void ToggleButton::setButtonText (String newText)
{
    if (buttonText != newText)
    {
        buttonText = newText;
        notifyPropertyChanged (textPropertyId);
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
            modifier.appendText (buttonText, font.withHeight (jmin (font.getHeight(), getHeight() * 0.6f)));
        }
    }
}

//==============================================================================

void ToggleButton::clicked()
{
    if (selectValueOnClick())
        return;

    // The button of a radio group that is on stays on when clicked. A change of state runs onClick itself
    if (toggleState && getRadioGroupId() != 0)
        Button::clicked();
    else
        setToggleState (! toggleState, sendNotification);
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
