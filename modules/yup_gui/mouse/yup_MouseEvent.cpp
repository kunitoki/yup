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

//==============================================================================

bool MouseEvent::isLeftButtoDown() const noexcept
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
    return { static_cast<Buttons> (buttons | buttonsToAdd), modifiers, position, sourceComponent };
}

MouseEvent MouseEvent::withoutButtons (Buttons buttonsToRemove) const noexcept
{
    return { static_cast<Buttons> (buttons & ~buttonsToRemove), modifiers, position, sourceComponent };
}

//==============================================================================

KeyModifiers MouseEvent::getModifiers() const noexcept
{
    return modifiers;
}

MouseEvent MouseEvent::withModifiers (KeyModifiers newModifiers) const noexcept
{
    return { buttons, newModifiers, position, sourceComponent };
}

//==============================================================================
Point<float> MouseEvent::getPosition() const noexcept
{
    return position;
}

MouseEvent MouseEvent::withPosition (const Point<float>& newPosition) const noexcept
{
    return { buttons, modifiers, newPosition, sourceComponent };
}

MouseEvent MouseEvent::withTranslatedPosition (const Point<float>& translation) const noexcept
{
    return { buttons, modifiers, position.translated (translation), sourceComponent };
}

//==============================================================================

Component* MouseEvent::getSourceComponent() const noexcept
{
    return sourceComponent;
}

MouseEvent MouseEvent::withSourceComponent (Component* newComponent) const noexcept
{
    return { buttons, modifiers, position, newComponent };
}

//==============================================================================

bool MouseEvent::operator== (const MouseEvent& other) const noexcept
{
    auto tie = [](const MouseEvent& x)
    {
        return std::make_tuple(
            x.buttons,
            x.modifiers,
            x.position,
            x.sourceComponent);
    };

    return tie (*this) == tie (other);
}

bool MouseEvent::operator!= (const MouseEvent& other) const noexcept
{
    return !(*this == other);
}

} // namespace yup
