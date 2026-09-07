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

class YUP_API Component;

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
class YUP_API MouseEvent
{
public:
    //==============================================================================
    /** Constants to indicate which mouse buttons are being held down.

        @see getButtons, isLeftButtonDown, isRightButtonDown, isMiddleButtonDown
    */
    enum Buttons
    {
        noButtons = 0x0000,    /**< No buttons pressed. */
        leftButton = 0x0001,   /**< The left mouse button. */
        middleButton = 0x0002, /**< The middle mouse button. */
        rightButton = 0x0004,  /**< The right mouse button. */

        allButtons = leftButton | middleButton | rightButton /**< Bitmask of all buttons. */
    };

    //==============================================================================
    /** Creates a default MouseEvent object.

        This creates an event describing a mouse movement to position (0, 0), with
        no buttons pressed and no key modifiers.
    */
    MouseEvent() noexcept = default;

    /** Creates a MouseEvent object.

        @param newButtons       The buttons that are currently held down
        @param newModifiers     The key modifiers that are currently active
        @param newPosition      The mouse position, relative to the component that receives the event
    */
    MouseEvent (Buttons newButtons, KeyModifiers newModifiers, const Point<float>& newPosition) noexcept;

    /** Creates a MouseEvent object.

        @param newButtons       The buttons that are currently held down
        @param newModifiers     The key modifiers that are currently active
        @param newPosition      The mouse position, relative to the component that receives the event
        @param sourceComponent  The component that the mouse event applies to
    */
    MouseEvent (Buttons newButtons, KeyModifiers newModifiers, const Point<float>& newPosition, Component* sourceComponent) noexcept;

    //==============================================================================
    /** Creates a MouseEvent object with full state.

        @param newButtons             The buttons that are currently held down
        @param newModifiers           The key modifiers that are currently active
        @param newPosition            The mouse position, relative to the component that receives the event
        @param lastMouseDownPosition  The position where the last mouse down event occurred
        @param lastMouseDownTime      The time of the last mouse down event
        @param sourceComponent        The component that the mouse event applies to
        @param newTouchIndex          The zero-based touch index, or -1 for a mouse event
        @param newPressure            The touch pressure in the range 0.0 to 1.0
    */
    MouseEvent (Buttons newButtons,
                KeyModifiers newModifiers,
                const Point<float>& newPosition,
                const Point<float>& lastMouseDownPosition,
                yup::Time lastMouseDownTime,
                Component* sourceComponent,
                int newTouchIndex = -1,
                float newPressure = 0.0f) noexcept;

    //==============================================================================
    /** Copy constructor and assignment operators. */
    MouseEvent (const MouseEvent& other) noexcept = default;
    MouseEvent (MouseEvent&& other) noexcept = default;
    MouseEvent& operator= (const MouseEvent& other) noexcept = default;
    MouseEvent& operator= (MouseEvent&& other) noexcept = default;

    //==============================================================================
    /** Returns true if the left mouse button is currently held down.

        @returns true if the left button is down
    */
    bool isLeftButtonDown() const noexcept;

    /** Returns true if the middle mouse button is currently held down.

        @returns true if the middle button is down
    */
    bool isMiddleButtonDown() const noexcept;

    /** Returns true if the right mouse button is currently held down.

        @returns true if the right button is down
    */
    bool isRightButtonDown() const noexcept;

    /** Returns true if any mouse button is currently held down.

        @returns true if any button is down
    */
    bool isAnyButtonDown() const noexcept;

    /** Returns the current mouse button state.

        @returns a bitmask of the buttons that are currently held down

        @see Buttons
    */
    Buttons getButtons() const noexcept;

    /** Creates a copy of this event with the specified buttons added.

        @param buttonsToAdd the buttons to add to the new event

        @returns a new MouseEvent object
    */
    MouseEvent withButtons (Buttons buttonsToAdd) const noexcept;

    /** Creates a copy of this event with the specified buttons removed.

        @param buttonsToRemove the buttons to remove from the new event

        @returns a new MouseEvent object
    */
    MouseEvent withoutButtons (Buttons buttonsToRemove) const noexcept;

    //==============================================================================
    /** Returns the key modifiers associated with this mouse event.

        @returns    the key modifiers currently active
    */
    KeyModifiers getModifiers() const noexcept;

    /** Creates a copy of this event with different modifiers.

        @param newModifiers the new modifier flags to use

        @returns a new MouseEvent object
    */
    MouseEvent withModifiers (KeyModifiers newModifiers) const noexcept;

    //==============================================================================
    /** Returns the mouse position associated with this event.

        @returns    the mouse position, relative to the component that received the event
    */
    Point<float> getPosition() const noexcept;

    /** Returns the mouse position in absolute screen coordinates.

        This method converts the mouse position from component-relative coordinates
        to absolute screen coordinates by taking into account the component hierarchy.

        @returns    the mouse position in absolute screen coordinates
    */
    Point<float> getScreenPosition() const noexcept;

    /** Creates a copy of this event with a different position.

        @param newPosition the new position to use

        @returns a new MouseEvent object
    */
    MouseEvent withPosition (const Point<float>& newPosition) const noexcept;

    /** Creates a copy of this event with its position offset by the specified amount.

        @param translation the offset to apply to the position

        @returns a new MouseEvent object
    */
    MouseEvent withTranslatedPosition (const Point<float>& translation) const noexcept;

    /** Creates a copy of this event with its position relative to the specified component.

        This is used internally by the component system to ensure that mouse events
        are delivered with coordinates relative to the receiving component.

        @param targetComponent the component to make the position relative to

        @returns a new MouseEvent object with position relative to the target component
    */
    MouseEvent withRelativePositionTo (Component* targetComponent) const noexcept;

    //==============================================================================
    /** Returns the position at which the last mouse-down event occurred.

        @returns    the last mouse-down position, relative to the component that received the event
    */
    Point<float> getLastMouseDownPosition() const noexcept;

    /** Creates a copy of this event with a different last mouse-down position.

        @param newPosition the new last mouse-down position to use

        @returns a new MouseEvent object
    */
    MouseEvent withLastMouseDownPosition (const Point<float>& newPosition) const noexcept;

    /** Returns the time at which the last mouse-down event occurred.

        @returns    the time of the last mouse-down event
    */
    yup::Time getLastMouseDownTime() const noexcept;

    /** Creates a copy of this event with a different last mouse-down time.

        @param newTime the new time to use

        @returns a new MouseEvent object
    */
    MouseEvent withLastMouseDownTime (yup::Time newTime) const noexcept;

    //==============================================================================
    /** Returns the component that this event applies to.

        @returns the component that the event occurred on
    */
    Component* getSourceComponent() const noexcept;

    /** Creates a copy of this event with a different source component.

        @param newComponent the new component to use as the source

        @returns a new MouseEvent object
    */
    MouseEvent withSourceComponent (Component* newComponent) const noexcept;

    //==============================================================================
    /** Returns true if this event originated from a finger on a touchscreen.

        Touch events are delivered through the same mouse callbacks as regular
        mouse events, but carry a finger index, see getTouchIndex().

        @returns true if the event was generated by a finger on a touchscreen

        @see getTouchIndex, getPressure
    */
    bool isTouch() const noexcept;

    /** Returns the index of the finger that generated this event.

        Fingers are numbered from 0 as they touch the screen: the first finger
        behaves exactly like a mouse, and additional fingers deliver parallel
        mouseDown/mouseDrag/mouseUp events with distinct indices. A finger keeps
        its index for the whole time it stays in contact, so it can be used to
        track individual fingers across mouseDown, mouseDrag and mouseUp.

        @returns the zero-based finger index, or -1 for mouse-generated events

        @see isTouch
    */
    int getTouchIndex() const noexcept;

    /** Returns the pressure of the touch that generated this event.

        @returns a value between 0.0 and 1.0, or 0.0 for mouse-generated events

        @see isTouch
    */
    float getPressure() const noexcept;

    /** Creates a copy of this event with a different touch index.

        @param newTouchIndex the new finger index, or -1 to mark it as a mouse event

        @returns a new MouseEvent object

        @see getTouchIndex
    */
    MouseEvent withTouchIndex (int newTouchIndex) const noexcept;

    /** Creates a copy of this event with a different pressure.

        @param newPressure the new pressure value between 0.0 and 1.0

        @returns a new MouseEvent object

        @see getPressure
    */
    MouseEvent withPressure (float newPressure) const noexcept;

    //==============================================================================
    /** Compares two MouseEvent objects.

        @param other the other event to compare with

        @returns true if the events are identical
    */
    bool operator== (const MouseEvent& other) const noexcept;

    /** Compares two MouseEvent objects.

        @param other the other event to compare with

        @returns true if the events are different
    */
    bool operator!= (const MouseEvent& other) const noexcept;

private:
    Buttons buttons = noButtons;
    KeyModifiers modifiers;
    Point<float> position;
    Point<float> lastMouseDownPosition;
    yup::Time lastMouseDownTime;
    Component* sourceComponent = nullptr;
    int touchIndex = -1;
    float pressure = 0.0f;
};

} // namespace yup
