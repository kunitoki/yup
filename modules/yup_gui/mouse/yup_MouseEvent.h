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

class JUCE_API Component;

//==============================================================================
class JUCE_API MouseEvent
{
public:
    //==============================================================================
    enum Buttons
    {
        noButtons = 0x0000,
        leftButton = 0x0001,
        middleButton = 0x0002,
        rightButton = 0x0004,

        allButtons = leftButton | middleButton | rightButton,
    };

    //==============================================================================
    MouseEvent() noexcept = default;
    MouseEvent (Buttons newButtons, KeyModifiers newModifiers, const Point<float>& newPosition) noexcept;
    MouseEvent (Buttons newButtons, KeyModifiers newModifiers, const Point<float>& newPosition, Component* sourceComponent) noexcept;

    //==============================================================================
    MouseEvent (const MouseEvent& other) noexcept = default;
    MouseEvent (MouseEvent&& other) noexcept = default;
    MouseEvent& operator= (const MouseEvent& other) noexcept = default;
    MouseEvent& operator= (MouseEvent&& other) noexcept = default;

    //==============================================================================
    bool isLeftButtoDown() const noexcept;
    bool isMiddleButtonDown() const noexcept;
    bool isRightButtonDown() const noexcept;
    bool isAnyButtonDown() const noexcept;
    MouseEvent withButtons (Buttons buttonsToAdd) const noexcept;
    MouseEvent withoutButtons (Buttons buttonsToRemove) const noexcept;

    //==============================================================================
    KeyModifiers getModifiers() const noexcept;
    MouseEvent withModifiers (KeyModifiers newModifiers) const noexcept;

    //==============================================================================
    Point<float> getPosition() const noexcept;
    MouseEvent withPosition (const Point<float>& newPosition) const noexcept;
    MouseEvent withTranslatedPosition (const Point<float>& translation) const noexcept;

    //==============================================================================
    Component* getSourceComponent() const noexcept;
    MouseEvent withSourceComponent (Component* newComponent) const noexcept;

    //==============================================================================
    bool operator== (const MouseEvent& other) const noexcept;
    bool operator!= (const MouseEvent& other) const noexcept;

private:
    Buttons buttons = noButtons;
    KeyModifiers modifiers;
    Point<float> position;
    Component* sourceComponent = nullptr;
};

} // namespace yup
