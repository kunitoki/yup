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

MouseEvent::MouseEvent (Buttons newButtons, KeyModifiers newModifiers, const Point<float>& newPosition) noexcept
    : MouseEvent (newButtons, newModifiers, newPosition, nullptr)
{
}

MouseEvent::MouseEvent (Buttons newButtons, KeyModifiers newModifiers, const Point<float>& newPosition, Component* sourceComponent) noexcept
    : buttons (newButtons)
    , modifiers (newModifiers)
    , position (newPosition)
    , sourceComponent (sourceComponent)
{
}

MouseEvent::MouseEvent (Buttons newButtons,
                        KeyModifiers newModifiers,
                        const Point<float>& newPosition,
                        const Point<float>& lastMouseDownPosition,
                        yup::Time lastMouseDownTime,
                        Component* sourceComponent,
                        int newTouchIndex,
                        float newPressure) noexcept
    : buttons (newButtons)
    , modifiers (newModifiers)
    , position (newPosition)
    , lastMouseDownPosition (lastMouseDownPosition)
    , lastMouseDownTime (lastMouseDownTime)
    , sourceComponent (sourceComponent)
    , touchIndex (newTouchIndex >= 0 ? newTouchIndex : -1)
    , pressure (newPressure)
{
}

//==============================================================================

bool MouseEvent::isLeftButtonDown() const noexcept
{
    return buttons & leftButton;
}

bool MouseEvent::isMiddleButtonDown() const noexcept
{
    return buttons & middleButton;
}

bool MouseEvent::isRightButtonDown() const noexcept
{
    return buttons & rightButton;
}

bool MouseEvent::isAnyButtonDown() const noexcept
{
    return buttons & allButtons;
}

MouseEvent::Buttons MouseEvent::getButtons() const noexcept
{
    return buttons;
}

MouseEvent MouseEvent::withButtons (Buttons buttonsToAdd) const noexcept
{
    return { static_cast<Buttons> (buttons | buttonsToAdd), modifiers, position, lastMouseDownPosition, lastMouseDownTime, sourceComponent, touchIndex, pressure };
}

MouseEvent MouseEvent::withoutButtons (Buttons buttonsToRemove) const noexcept
{
    return { static_cast<Buttons> (buttons & ~buttonsToRemove), modifiers, position, lastMouseDownPosition, lastMouseDownTime, sourceComponent, touchIndex, pressure };
}

//==============================================================================

KeyModifiers MouseEvent::getModifiers() const noexcept
{
    return modifiers;
}

MouseEvent MouseEvent::withModifiers (KeyModifiers newModifiers) const noexcept
{
    return { buttons, newModifiers, position, lastMouseDownPosition, lastMouseDownTime, sourceComponent, touchIndex, pressure };
}

//==============================================================================
Point<float> MouseEvent::getPosition() const noexcept
{
    return position;
}

Point<float> MouseEvent::getScreenPosition() const noexcept
{
    if (sourceComponent == nullptr)
        return position;

    // Get the source component's screen position and add our relative position
    return sourceComponent->getScreenPosition() + position;
}

MouseEvent MouseEvent::withPosition (const Point<float>& newPosition) const noexcept
{
    return { buttons, modifiers, newPosition, lastMouseDownPosition, lastMouseDownTime, sourceComponent, touchIndex, pressure };
}

MouseEvent MouseEvent::withTranslatedPosition (const Point<float>& translation) const noexcept
{
    return { buttons, modifiers, position.translated (translation), lastMouseDownPosition, lastMouseDownTime, sourceComponent, touchIndex, pressure };
}

MouseEvent MouseEvent::withRelativePositionTo (Component* targetComponent) const noexcept
{
    if (targetComponent == nullptr)
        return *this;

    // Calculate the position relative to the target component
    auto relativePos = position;

    // Walk up the component hierarchy to find the offset from the top-level component
    auto currentComponent = targetComponent;
    while (currentComponent != nullptr && currentComponent->getParentComponent() != nullptr)
    {
        relativePos = relativePos - currentComponent->getBounds().getPosition();
        currentComponent = currentComponent->getParentComponent();
    }

    // Also translate the last mouse down position if it exists
    auto relativeLastPos = lastMouseDownPosition;
    if (lastMouseDownPosition != Point<float>() && targetComponent != nullptr)
    {
        currentComponent = targetComponent;
        while (currentComponent != nullptr && currentComponent->getParentComponent() != nullptr)
        {
            relativeLastPos = relativeLastPos - currentComponent->getBounds().getPosition();
            currentComponent = currentComponent->getParentComponent();
        }
    }

    return { buttons, modifiers, relativePos, relativeLastPos, lastMouseDownTime, targetComponent, touchIndex, pressure };
}

//==============================================================================

Point<float> MouseEvent::getLastMouseDownPosition() const noexcept
{
    return lastMouseDownPosition;
}

MouseEvent MouseEvent::withLastMouseDownPosition (const Point<float>& newPosition) const noexcept
{
    return { buttons, modifiers, position, newPosition, lastMouseDownTime, sourceComponent, touchIndex, pressure };
}

yup::Time MouseEvent::getLastMouseDownTime() const noexcept
{
    return lastMouseDownTime;
}

MouseEvent MouseEvent::withLastMouseDownTime (yup::Time newTime) const noexcept
{
    return { buttons, modifiers, position, lastMouseDownPosition, newTime, sourceComponent, touchIndex, pressure };
}

//==============================================================================

Component* MouseEvent::getSourceComponent() const noexcept
{
    return sourceComponent;
}

MouseEvent MouseEvent::withSourceComponent (Component* newComponent) const noexcept
{
    return { buttons, modifiers, position, lastMouseDownPosition, lastMouseDownTime, newComponent, touchIndex, pressure };
}

//==============================================================================

bool MouseEvent::isTouch() const noexcept
{
    return touchIndex >= 0;
}

int MouseEvent::getTouchIndex() const noexcept
{
    return touchIndex;
}

float MouseEvent::getPressure() const noexcept
{
    return pressure;
}

MouseEvent MouseEvent::withTouchIndex (int newTouchIndex) const noexcept
{
    return { buttons, modifiers, position, lastMouseDownPosition, lastMouseDownTime, sourceComponent, newTouchIndex >= 0 ? newTouchIndex : -1, pressure };
}

MouseEvent MouseEvent::withPressure (float newPressure) const noexcept
{
    return { buttons, modifiers, position, lastMouseDownPosition, lastMouseDownTime, sourceComponent, touchIndex, newPressure };
}

//==============================================================================

bool MouseEvent::operator== (const MouseEvent& other) const noexcept
{
    auto tie = [] (const MouseEvent& x)
    {
        return std::make_tuple (
            x.buttons,
            x.modifiers,
            x.position,
            x.lastMouseDownPosition,
            x.lastMouseDownTime,
            x.sourceComponent,
            x.touchIndex,
            x.pressure);
    };

    return tie (*this) == tie (other);
}

bool MouseEvent::operator!= (const MouseEvent& other) const noexcept
{
    return ! (*this == other);
}

} // namespace yup
