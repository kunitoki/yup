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

const Identifier SwitchButton::Style::switchColorId = "switchButtonSwitch";
const Identifier SwitchButton::Style::switchOffBackgroundColorId = "switchButtonOffBackground";
const Identifier SwitchButton::Style::switchOnBackgroundColorId = "switchButtonOnBackground";

//==============================================================================

SwitchButton::SwitchButton (StringRef componentID, bool isVertical)
    : Button (componentID)
    , isVerticalValue (isVertical)
{
    setWantsKeyboardFocus (true);
    setMouseCursor (MouseCursor::Hand);
}

//==============================================================================

void SwitchButton::setToggleState (bool shouldBeToggled, NotificationType notification)
{
    if (toggleState != shouldBeToggled)
    {
        toggleState = shouldBeToggled;

        updateSwitchCirclePosition();

        if (notification != dontSendNotification)
        {
            toggleStateChanged();

            if (onClick)
                onClick();
        }

        repaint();
    }
}

//==============================================================================

void SwitchButton::setVertical (bool shouldBeVertical) noexcept
{
    if (isVerticalValue != shouldBeVertical)
    {
        isVerticalValue = shouldBeVertical;

        updateSwitchCirclePosition();

        repaint();
    }
}

//==============================================================================

void SwitchButton::setMillisecondsToSpendMoving (int newValue) noexcept
{
    millisecondsToSpendMoving = newValue;
}

//==============================================================================

void SwitchButton::paintButton (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================

void SwitchButton::refreshDisplay (double lastFrameTimeSeconds)
{
}

//==============================================================================

void SwitchButton::resized()
{
    Button::resized();

    updateSwitchCirclePosition();
}

void SwitchButton::mouseUp (const MouseEvent& event)
{
    if (getLocalBounds().contains (event.getPosition()))
        setToggleState (!toggleState, sendNotification);

    Button::mouseUp (event);
}

//==============================================================================

void SwitchButton::updateSwitchCirclePosition()
{
    auto bounds = getLocalBounds();

    if (!isVerticalValue)
    {
        switchCircleBounds = Rectangle<float> (
            getToggleState() ? bounds.getRight() - bounds.getHeight() : bounds.getX(),
            bounds.getY(),
            bounds.getHeight(),
            bounds.getHeight()
        );
    }
    else
    {
        switchCircleBounds = Rectangle<float> (
            bounds.getX(),
            getToggleState() ? bounds.getBottom() - bounds.getWidth() : bounds.getY(),
            bounds.getWidth(),
            bounds.getWidth()
        );
    }

    switchCircleBounds = switchCircleBounds.reduced (1).toNearestInt();

    repaint();
}

} // namespace yup
