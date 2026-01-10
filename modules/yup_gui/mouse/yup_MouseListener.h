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

/** A base class for receiving mouse events.

    To receive mouse events, create a subclass of MouseListener and implement the
    relevant callback methods. Then register your listener with a Component using
    the Component::addMouseListener() method.

    @see MouseEvent, Component::addMouseListener, Component::removeMouseListener
*/
class YUP_API MouseListener
{
public:
    /** Destructor. */
    virtual ~MouseListener() {}

    /** Called when the mouse enters a component.

        @param event The mouse event.
    */
    virtual void mouseEnter (const MouseEvent& event) {}

    /** Called when the mouse exits a component.

        @param event The mouse event.
    */
    virtual void mouseExit (const MouseEvent& event) {}

    /** Called when the mouse is double-clicked.

        @param event The mouse event.
    */
    virtual void mouseDoubleClick (const MouseEvent& event) {}

    /** Called when a mouse button is pressed.

        @param event The mouse event.
    */
    virtual void mouseDown (const MouseEvent& event) {}

    /** Called when the mouse is moved.

        @param event The mouse event.
    */
    virtual void mouseMove (const MouseEvent& event) {}

    /** Called when the mouse is dragged.

        @param event The mouse event.
    */
    virtual void mouseDrag (const MouseEvent& event) {}

    /** Called when a mouse button is released.

        @param event The mouse event.
    */
    virtual void mouseUp (const MouseEvent& event) {}

    /** Called when the mouse wheel is moved.

        @param event The mouse event.
        @param wheelData The data associated with the mouse wheel movement.
    */
    virtual void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData) {}

private:
    YUP_DECLARE_WEAK_REFERENCEABLE (MouseListener)
};

} // namespace yup
