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

namespace juce
{

//==============================================================================

class JUCE_API MouseEvent
{
public:
    enum Buttons
    {
        noButtons = 0x0000,
        leftButton = 0x0001,
        middleButton = 0x0002,
        rightButton = 0x0004,

        allButtons = leftButton | middleButton | rightButton,
    };

    constexpr MouseEvent() noexcept = default;

    constexpr MouseEvent (Buttons newButtons, KeyModifiers newModifiers, Point<float> newPosition) noexcept
        : buttons (newButtons)
        , modifiers (newModifiers)
        , position (newPosition)
    {
    }

    constexpr MouseEvent (const MouseEvent& other) noexcept = default;
    constexpr MouseEvent (MouseEvent&& other) noexcept = default;
    constexpr MouseEvent& operator= (const MouseEvent& other) noexcept = default;
    constexpr MouseEvent& operator= (MouseEvent&& other) noexcept = default;

    constexpr bool isLeftButtoDown() const noexcept
    {
        return buttons & leftButton;
    }

    constexpr bool isMiddleButtonDown() const noexcept
    {
        return buttons & middleButton;
    }

    constexpr bool isRightButtonDown() const noexcept
    {
        return buttons & rightButton;
    }

    constexpr bool isAnyButtonDown() const noexcept
    {
        return buttons & allButtons;
    }

    constexpr KeyModifiers getModifiers() const noexcept
    {
        return modifiers;
    }

    constexpr Point<float> getPosition() const noexcept
    {
        return position;
    }

    constexpr MouseEvent withButtons (Buttons buttonsToAdd) const noexcept
    {
        return { static_cast<Buttons> (buttons | buttonsToAdd), modifiers, position };
    }

    constexpr MouseEvent withoutButtons (Buttons buttonsToRemove) const noexcept
    {
        return { static_cast<Buttons> (buttons & ~buttonsToRemove), modifiers, position };
    }

    constexpr MouseEvent withModifiers (KeyModifiers newModifiers) const noexcept
    {
        return { buttons, newModifiers, position };
    }

    constexpr MouseEvent withPosition (Point<float> newPosition) const noexcept
    {
        return { buttons, modifiers, newPosition };
    }

    constexpr bool operator== (const MouseEvent& other) const noexcept
    {
        return buttons == other.buttons && modifiers == other.modifiers && position == other.position;
    }

    constexpr bool operator!= (const MouseEvent& other) const noexcept
    {
        return !(*this == other);
    }

private:
    Buttons buttons = noButtons;
    KeyModifiers modifiers;
    Point<float> position;
};

} // namespace juce
