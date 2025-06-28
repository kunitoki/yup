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

SwitchButton::SwitchButton (StringRef componentID, bool isInverted, bool isVertical)
    : Button (componentID)
    , isInvertedValue (isInverted)
    , isVerticalValue (isVertical)
{
    setWantsKeyboardFocus (true);
    setMouseCursor (MouseCursor::Hand);

    switchCircle.setWantsKeyboardFocus (false);
    //switchCircle.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (switchCircle);
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

bool SwitchButton::getSwitchState() const noexcept
{
    return isInvertedValue ? !toggleState : toggleState;
}

void SwitchButton::setInverted (bool shouldBeInverted) noexcept
{
    if (isInvertedValue != shouldBeInverted)
    {
        isInvertedValue = shouldBeInverted;
        updateSwitchCirclePosition();
        repaint();
    }
}

void SwitchButton::setVertical (bool shouldBeVertical) noexcept
{
    if (isVerticalValue != shouldBeVertical)
    {
        isVerticalValue = shouldBeVertical;
        updateSwitchCirclePosition();
        repaint();
    }
}

void SwitchButton::setMillisecondsToSpendMoving (int newValue) noexcept
{
    millisecondsToSpendMoving = newValue;
}

//==============================================================================

void SwitchButton::paintButton (Graphics& g)
{
    auto bounds = getSwitchBounds();

    auto cornerSize = (isVerticalValue ? bounds.getWidth() : bounds.getHeight()) * 0.5f;

    // Draw shadow/outline
    g.setStrokeColor (Colors::black.withAlpha (0.1f));
    g.setStrokeWidth (2.0f);
    g.strokeRoundedRect (bounds, cornerSize);

    // Fill background based on switch state
    auto bgColor = findColor (getSwitchState() ? Style::switchOnBackgroundColorId : Style::switchOffBackgroundColorId)
                       .value_or (getSwitchState() ? Colors::limegreen : Colors::darkgray);

    g.setFillColor (bgColor);
    g.fillRoundedRect (bounds, cornerSize);
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

Rectangle<float> SwitchButton::getSwitchBounds() const
{
    return getLocalBounds().reduced (4, 4);
}

void SwitchButton::updateSwitchCirclePosition()
{
    auto bounds = getSwitchBounds();

    Rectangle<float> switchCircleBounds;
    if (!isVerticalValue)
    {
        switchCircleBounds = Rectangle<float> (
            getSwitchState() ? bounds.getRight() - bounds.getHeight() : bounds.getX(),
            bounds.getY(),
            bounds.getHeight(),
            bounds.getHeight()
        );
    }
    else
    {
        switchCircleBounds = Rectangle<float> (
            bounds.getX(),
            getSwitchState() ? bounds.getBottom() - bounds.getWidth() : bounds.getY(),
            bounds.getWidth(),
            bounds.getWidth()
        );
    }

    switchCircle.setBounds (switchCircleBounds.reduced (1).toNearestInt());
}

//==============================================================================
SwitchButton::SwitchCircle::SwitchCircle()
    : Component ("switchCircle")
{
}

void SwitchButton::SwitchCircle::paint (Graphics& g)
{
    auto bounds = getLocalBounds();
    auto radius = bounds.getHeight() * 0.5f;

    auto switchColor = findColor (SwitchButton::Style::switchColorId)
                           .value_or (Colors::white);

    g.setFillColor (switchColor);
    g.fillRoundedRect (bounds, radius);

    // Add a subtle shadow
    g.setStrokeColor (Colors::black.withAlpha (0.2f));
    g.setStrokeWidth (1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), radius - 0.5f);
}

} // namespace yup
