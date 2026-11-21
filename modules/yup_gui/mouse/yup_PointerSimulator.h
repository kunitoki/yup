/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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
/**
    Sends a component and what it holds the pointer events a window would send them.

    A component on no window gets no pointer event, so it can't be clicked or dragged. The
    simulator stands for the pointer of a window showing the component: it finds what is under
    a position, enters and leaves components as it moves, and gives a press to what it is over
    until the release. It is meant for tools showing an interface with no window and for tests.

    @code
    PointerSimulator pointer (panel);

    pointer.click (button.getBounds().getCenter());
    pointer.drag ({ 20.0f, 55.0f }, { 180.0f, 55.0f });
    @endcode

    Positions are in the coordinates of the component given to the constructor. Keyboard focus
    and popups need a window: a click gives no focus, and a menu it asks for does not open.

    @see Component::setHovered, Component::setPressed

    @tags{UI}
*/
class YUP_API PointerSimulator
{
public:
    //==============================================================================
    /** Creates a simulator for a component, the one a window would show.

        @param topLevelComponent  The component the events are sent to, with what it holds. It
                                  must be visible, as nothing is under the pointer in a hidden
                                  component, and outlive the simulator.
    */
    explicit PointerSimulator (Component& topLevelComponent);

    /** Releases the button still pressed and takes the pointer out of the component. */
    ~PointerSimulator();

    //==============================================================================
    /** Moves the pointer.

        The component the pointer was over is left and the one it gets over is entered, then the
        move is sent to it. While a button is pressed, the move is a drag of the component pressed.

        @param position  Where the pointer goes.

        @returns The component the move or the drag was sent to, nullptr when there is none.
    */
    Component* moveTo (Point<float> position);

    /** Presses a button where the pointer is.

        @param button     The button to press.
        @param modifiers  The modifier keys held, until the release.

        @returns The component under the pointer, which is pressed until release(), or nullptr.
    */
    Component* press (MouseEvent::Buttons button = MouseEvent::leftButton, KeyModifiers modifiers = {});

    /** Releases the button pressed.

        @returns The component that was pressed, nullptr when none was or when it is gone.
    */
    Component* release();

    /** Takes the pointer out of the component, as when it leaves a window. */
    void leave();

    //==============================================================================
    /** Moves the pointer to a position, then presses and releases the left button.

        @returns The component clicked, or nullptr.
    */
    Component* click (Point<float> position);

    /** Clicks twice at a position, the second click being a double click.

        @returns The component clicked, or nullptr.
    */
    Component* doubleClick (Point<float> position);

    /** Presses the left button at a position, moves to another in steps and releases it.

        @param from      Where the button is pressed.
        @param to        Where it is released.
        @param numSteps  How many moves are made between the two, at least one.

        @returns The component dragged, or nullptr.
    */
    Component* drag (Point<float> from, Point<float> to, int numSteps = 8);

    /** Moves the pointer to a position without a move event, as a wheel does, and turns the wheel.

        @param position  Where the pointer is.
        @param delta     How far the wheel turns on each axis.

        @returns The component the wheel was sent to, which passes it on to its parents when it
                 does not use it, or nullptr.
    */
    Component* wheel (Point<float> position, Point<float> delta);

    //==============================================================================
    /** Returns where the pointer is. */
    Point<float> getPosition() const noexcept { return position; }

    /** Returns the component the pointer is over, or nullptr. */
    Component* getComponentUnderPointer() const noexcept { return underPointer.get(); }

    /** Returns the component pressed, or nullptr while no button is pressed. */
    Component* getPressedComponent() const noexcept { return pressed.get(); }

private:
    MouseEvent makeEvent (Component* source) const;
    void updateComponentUnderPointer();

    Component& topLevel;
    WeakReference<Component> underPointer;
    WeakReference<Component> pressed;
    Point<float> position;
    std::optional<Point<float>> pressPosition;
    std::optional<Time> pressTime;
    MouseEvent::Buttons buttons = MouseEvent::noButtons;
    KeyModifiers modifiers;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PointerSimulator)
};

} // namespace yup
