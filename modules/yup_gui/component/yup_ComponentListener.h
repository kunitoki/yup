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

class Component;

//==============================================================================

/** A base class for receiving component lifecycle and paint measurement events. */
class YUP_API ComponentListener
{
public:
    /** Destructor. */
    virtual ~ComponentListener() = default;

    /** Called when a component's position changes. */
    virtual void componentMoved (Component& component)
    {
        ignoreUnused (component);
    }

    /** Called when a component's size changes. */
    virtual void componentResized (Component& component)
    {
        ignoreUnused (component);
    }

    /** Called when a component is about to be deleted. */
    virtual void componentBeingDeleted (Component& component)
    {
        ignoreUnused (component);
    }

    /** Called when a component property changes, by name.

        This fires for store-backed property writes (setProperty, setColor, removeProperty) and
        for state-backed changes a widget reports through Component::notifyPropertyChanged().
    */
    virtual void componentPropertyChanged (Component& component, const Identifier& name)
    {
        ignoreUnused (component, name);
    }

    /** Called when an enabled component is double clicked, before the component itself is told.

        Return true to take the double click: Component::mouseDoubleClick() is then not called, so
        what the component does itself on a double click, as a Slider going back to its default
        value, does not happen. The other listeners are still asked, and the mouse listeners are
        told as before. It is how the `onDoubleClick` handlers of `.yui` run.

        @param component  The component that was double clicked.
        @param event      The mouse event, relative to the component.

        @returns True to handle the double click in place of the component.
    */
    virtual bool componentDoubleClicked (Component& component, const MouseEvent& event)
    {
        ignoreUnused (component, event);
        return false;
    }

    /** Called after a component paint pass completes with measured paint data.

        Timing fields in measurement contain raw high-resolution tick counts. Listeners
        that need wall-clock units are responsible for converting them.
    */
    virtual void componentPaintCompleted (Component& component, const ComponentPaintMetrics& metrics)
    {
        ignoreUnused (component, metrics);
    }

private:
    YUP_DECLARE_WEAK_REFERENCEABLE (ComponentListener)
};

} // namespace yup
