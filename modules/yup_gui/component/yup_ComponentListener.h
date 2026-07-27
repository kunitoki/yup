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
