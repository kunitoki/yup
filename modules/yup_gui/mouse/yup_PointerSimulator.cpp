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

PointerSimulator::PointerSimulator (Component& topLevelComponent)
    : topLevel (topLevelComponent)
{
}

PointerSimulator::~PointerSimulator()
{
    release();
    leave();
}

//==============================================================================

MouseEvent PointerSimulator::makeEvent (Component* source) const
{
    auto event = MouseEvent()
                     .withButtons (buttons)
                     .withModifiers (modifiers)
                     .withPosition (position);

    if (pressPosition.has_value())
        event = event.withLastMouseDownPosition (*pressPosition);

    if (pressTime.has_value())
        event = event.withLastMouseDownTime (*pressTime);

    return source != nullptr ? event.withSourceComponent (source).withRelativePositionTo (source) : event;
}

void PointerSimulator::updateComponentUnderPointer()
{
    const WeakReference<Component> previous (underPointer);
    const WeakReference<Component> current (topLevel.findComponentAtForMouseEvent (position));

    underPointer = current;

    if (previous == current)
        return;

    if (previous != nullptr)
        previous->internalMouseExit (makeEvent (previous.get()));

    // What handled the exit may have removed the component the pointer is now over
    if (current != nullptr)
        current->internalMouseEnter (makeEvent (current.get()));
}

//==============================================================================

Component* PointerSimulator::moveTo (Point<float> newPosition)
{
    position = newPosition;

    if (buttons != MouseEvent::noButtons)
    {
        if (pressed != nullptr)
            pressed->internalMouseDrag (makeEvent (pressed.get()));

        return pressed.get();
    }

    updateComponentUnderPointer();

    if (underPointer != nullptr)
        underPointer->internalMouseMove (makeEvent (underPointer.get()));

    return underPointer.get();
}

Component* PointerSimulator::press (MouseEvent::Buttons button, KeyModifiers heldModifiers)
{
    if (buttons != MouseEvent::noButtons)
        release();

    updateComponentUnderPointer();

    buttons = button;
    modifiers = heldModifiers;
    pressed = underPointer;

    if (pressed != nullptr)
        pressed->internalMouseDown (makeEvent (pressed.get()));

    pressPosition = position;
    pressTime = Time::getCurrentTime();

    return pressed.get();
}

Component* PointerSimulator::release()
{
    if (buttons == MouseEvent::noButtons)
        return nullptr;

    const WeakReference<Component> released (pressed);

    buttons = MouseEvent::noButtons;

    if (released != nullptr)
        released->internalMouseUp (makeEvent (released.get()));

    pressed = nullptr;
    modifiers = {};

    updateComponentUnderPointer();

    return released.get();
}

void PointerSimulator::leave()
{
    if (underPointer != nullptr)
        underPointer->internalMouseExit (makeEvent (underPointer.get()));

    underPointer = nullptr;
}

//==============================================================================

Component* PointerSimulator::click (Point<float> newPosition)
{
    moveTo (newPosition);
    press();

    return release();
}

Component* PointerSimulator::doubleClick (Point<float> newPosition)
{
    click (newPosition);

    // A window tells the double click when the second press is released, before that release
    if (auto* clicked = press())
    {
        const WeakReference<Component> reference (clicked);

        clicked->internalMouseDoubleClick (makeEvent (clicked));

        if (reference == nullptr)
            pressed = nullptr;
    }

    return release();
}

Component* PointerSimulator::drag (Point<float> from, Point<float> to, int numSteps)
{
    moveTo (from);
    press();

    numSteps = jmax (1, numSteps);

    for (int step = 1; step <= numSteps; ++step)
        moveTo (from + (to - from) * (static_cast<float> (step) / static_cast<float> (numSteps)));

    return release();
}

Component* PointerSimulator::wheel (Point<float> newPosition, Point<float> delta)
{
    position = newPosition;

    if (buttons == MouseEvent::noButtons)
        updateComponentUnderPointer();

    auto* target = pressed != nullptr ? pressed.get() : underPointer.get();

    if (target != nullptr)
        target->internalMouseWheel (makeEvent (target), MouseWheelData (delta.getX(), delta.getY()));

    return target;
}

} // namespace yup
