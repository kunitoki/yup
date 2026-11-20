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

#pragma once

namespace yup
{

class Component;

//==============================================================================
/**
    Intercepts input events before they reach the component tree.

    A filter is consulted by the windowing layer for every mouse, wheel and key event, before
    the event is dispatched to the component under the pointer or to the focused component.
    Returning true from a callback consumes the event, so the application never sees it.

    This is what a tool overlaying a running interface needs: an inspector picking components
    with the mouse must swallow the clicks it uses, without adding anything to the component
    tree it inspects. Unlike Desktop::addGlobalMouseListener, which only observes, a filter
    decides whether the event goes on.

    Filters are called in the order they were added, and the first one consuming an event stops
    the others. A filter must be removed before it is destroyed.

    @code
    class PickFilter : public DesktopInputFilter
    {
        bool mouseEventFiltered (InputEventType type, Component& root, const MouseEvent& event) override
        {
            if (type != InputEventType::mouseDown)
                return false;

            select (root.findComponentAt (event.getPosition()));
            return true; // The application never sees this click
        }
    };

    Desktop::getInstance()->addInputFilter (&filter);
    @endcode

    @see Desktop::addInputFilter, MouseListener

    @tags{UI}
*/
class YUP_API DesktopInputFilter
{
public:
    //==============================================================================
    /** The kind of mouse event being filtered. */
    enum class InputEventType
    {
        mouseDown, /**< A button was pressed. */
        mouseUp,   /**< A button was released. */
        mouseMove, /**< The pointer moved with no button down. */
        mouseDrag, /**< The pointer moved with a button down. */
        mouseEnter,/**< The pointer entered the window. */
        mouseExit  /**< The pointer left the window. */
    };

    //==============================================================================
    /** Destructor. The filter must already have been removed from the Desktop. */
    virtual ~DesktopInputFilter() = default;

    //==============================================================================
    /** Called before a mouse event is dispatched to the components of a window.

        @param type           The kind of event.
        @param rootComponent  The root component of the window the event happened in.
        @param event          The event, positioned in the coordinates of @p rootComponent.

        @returns True to consume the event, so that it is not dispatched.
    */
    virtual bool mouseEventFiltered (InputEventType type, Component& rootComponent, const MouseEvent& event) { return false; }

    /** Called before a mouse wheel event is dispatched to the components of a window.

        @param rootComponent  The root component of the window the event happened in.
        @param event          The event, positioned in the coordinates of @p rootComponent.
        @param wheelData      The wheel movement.

        @returns True to consume the event, so that it is not dispatched.
    */
    virtual bool mouseWheelFiltered (Component& rootComponent, const MouseEvent& event, const MouseWheelData& wheelData) { return false; }

    /** Called before a key press is dispatched to the focused component of a window.

        @param rootComponent  The root component of the window the event happened in.
        @param keys           The key pressed.
        @param position       The pointer position, in the coordinates of @p rootComponent.

        @returns True to consume the event, so that it is not dispatched.
    */
    virtual bool keyDownFiltered (Component& rootComponent, const KeyPress& keys, const Point<float>& position) { return false; }

    /** Called before a key release is dispatched to the focused component of a window.

        @param rootComponent  The root component of the window the event happened in.
        @param keys           The key released.
        @param position       The pointer position, in the coordinates of @p rootComponent.

        @returns True to consume the event, so that it is not dispatched.
    */
    virtual bool keyUpFiltered (Component& rootComponent, const KeyPress& keys, const Point<float>& position) { return false; }
};

} // namespace yup
