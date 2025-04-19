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
/** Contains information about a mouse event.

    This class is used to represent mouse events such as clicks, moves, drags, etc.
    It contains information about the mouse state, position, and which component
    the event occurred on.

    A MouseEvent object is passed to a component's mouse event methods, where you can
    query its properties to find out what the user is doing.

    @see Component::mouseMove, Component::mouseEnter, Component::mouseExit,
         Component::mouseDown, Component::mouseUp, Component::mouseDrag
*/
class JUCE_API MouseEvent
{
public:
    //==============================================================================
    /** Constants to indicate which mouse buttons are being held down.

        @see getButtons, isLeftButtonDown, isRightButtonDown, isMiddleButtonDown
    */
    enum Buttons
    {
        noButtons = 0x0000,      /**< No buttons pressed. */
        leftButton = 0x0001,     /**< The left mouse button. */
        middleButton = 0x0002,   /**< The middle mouse button. */
        rightButton = 0x0004,    /**< The right mouse button. */

        allButtons = leftButton | middleButton | rightButton  /**< Bitmask of all buttons. */
    };

    //==============================================================================
    /** Creates a default MouseEvent object.

        This creates an event describing a mouse movement to position (0, 0), with
        no buttons pressed and no key modifiers.
    */
    MouseEvent() noexcept = default;

    /** Creates a MouseEvent object.

        @param newButtons        The buttons that are currently held down
        @param newModifiers     The key modifiers that are currently active
        @param newPosition      The mouse position, relative to the component that receives the event
    */
    MouseEvent (Buttons newButtons, KeyModifiers newModifiers, const Point<float>& newPosition) noexcept;

    /** Creates a MouseEvent object.

        @param newButtons        The buttons that are currently held down
        @param newModifiers     The key modifiers that are currently active
        @param newPosition      The mouse position, relative to the component that receives the event
        @param sourceComponent  The component that the mouse event applies to
    */
    MouseEvent (Buttons newButtons, KeyModifiers newModifiers, const Point<float>& newPosition, Component* sourceComponent) noexcept;

    //==============================================================================
    /** Copy constructor. */
    MouseEvent (const MouseEvent& other) noexcept = default;

    /** Move constructor. */
    MouseEvent (MouseEvent&& other) noexcept = default;

    /** Copy assignment operator. */
    MouseEvent& operator= (const MouseEvent& other) noexcept = default;

    /** Move assignment operator. */
    MouseEvent& operator= (MouseEvent&& other) noexcept = default;

    //==============================================================================
    /** Returns true if the left mouse button is currently held down.

        @returns    true if the left button is down
    */
    bool isLeftButtoDown() const noexcept;

    /** Returns true if the middle mouse button is currently held down.

        @returns    true if the middle button is down
    */
    bool isMiddleButtonDown() const noexcept;

    /** Returns true if the right mouse button is currently held down.

        @returns    true if the right button is down
    */
    bool isRightButtonDown() const noexcept;

    /** Returns true if any mouse button is currently held down.

        @returns    true if any button is down
    */
    bool isAnyButtonDown() const noexcept;

    /** Returns the current mouse button state.

        @returns    a bitmask of the buttons that are currently held down

        @see Buttons
    */
    Buttons getButtons() const noexcept;

    /** Creates a copy of this event with the specified buttons added.

        @param buttonsToAdd    the buttons to add to the new event

        @returns              a new MouseEvent object
    */
    MouseEvent withButtons (Buttons buttonsToAdd) const noexcept;

    /** Creates a copy of this event with the specified buttons removed.

        @param buttonsToRemove    the buttons to remove from the new event

        @returns                 a new MouseEvent object
    */
    MouseEvent withoutButtons (Buttons buttonsToRemove) const noexcept;

    //==============================================================================
    /** Returns the key modifiers associated with this mouse event.

        @returns    the key modifiers currently active
    */
    KeyModifiers getModifiers() const noexcept;

    /** Creates a copy of this event with different modifiers.

        @param newModifiers    the new modifier flags to use

        @returns              a new MouseEvent object
    */
    MouseEvent withModifiers (KeyModifiers newModifiers) const noexcept;

    //==============================================================================
    /** Returns the mouse position associated with this event.

        @returns    the mouse position, relative to the component that received the event
    */
    Point<float> getPosition() const noexcept;

    /** Creates a copy of this event with a different position.

        @param newPosition    the new position to use

        @returns            a new MouseEvent object
    */
    MouseEvent withPosition (const Point<float>& newPosition) const noexcept;

    /** Creates a copy of this event with its position offset by the specified amount.

        @param translation    the offset to apply to the position

        @returns            a new MouseEvent object
    */
    MouseEvent withTranslatedPosition (const Point<float>& translation) const noexcept;

    //==============================================================================
    /** Returns the position at which the last mouse-down event occurred.

        @returns    the last mouse-down position, relative to the component that received the event
    */
    Point<float> getLastMouseDownPosition() const noexcept;

    /** Creates a copy of this event with a different last mouse-down position.

        @param newPosition    the new last mouse-down position to use

        @returns            a new MouseEvent object
    */
    MouseEvent withLastMouseDownPosition (const Point<float>& newPosition) const noexcept;

    /** Returns the time at which the last mouse-down event occurred.

        @returns    the time of the last mouse-down event
    */
    juce::Time getLastMouseDownTime() const noexcept;

    /** Creates a copy of this event with a different last mouse-down time.

        @param newTime    the new time to use

        @returns        a new MouseEvent object
    */
    MouseEvent withLastMouseDownTime (juce::Time newTime) const noexcept;

    //==============================================================================
    /** Returns the component that this event applies to.

        @returns    the component that the event occurred on
    */
    Component* getSourceComponent() const noexcept;

    /** Creates a copy of this event with a different source component.

        @param newComponent    the new component to use as the source

        @returns             a new MouseEvent object
    */
    MouseEvent withSourceComponent (Component* newComponent) const noexcept;

    //==============================================================================
    /** Compares two MouseEvent objects.

        @param other    the other event to compare with

        @returns       true if the events are identical
    */
    bool operator== (const MouseEvent& other) const noexcept;

    /** Compares two MouseEvent objects.

        @param other    the other event to compare with

        @returns       true if the events are different
    */
    bool operator!= (const MouseEvent& other) const noexcept;

private:
    MouseEvent (Buttons newButtons,
                KeyModifiers newModifiers,
                const Point<float>& newPosition,
                const Point<float>& lastMouseDownPosition,
                juce::Time lastMouseDownTime,
                Component* sourceComponent) noexcept;

    Buttons buttons = noButtons;
    KeyModifiers modifiers;
    Point<float> position;
    Point<float> lastMouseDownPosition;
    juce::Time lastMouseDownTime;
    Component* sourceComponent = nullptr;
};

} // namespace yup
