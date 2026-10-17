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

/** A base class for receiving touch and trackpad gestures.

    Component derives from it, so a component overrides the callbacks it handles after asking
    for them with Component::setWantedGestures(). Other objects receive the gestures of a
    component by registering with Component::addGestureListener().

    @see GestureEvent, GestureType, Component::setWantedGestures, Component::addGestureListener
*/
class YUP_API GestureListener
{
public:
    /** Destructor. */
    virtual ~GestureListener() = default;

    /** Called when a tap is recognized on a component wanting GestureType::tap.

        Double taps arrive as a second tap with GestureEvent::getTapCount() of 2.

        @param event The gesture event.
        @see Component::setWantedGestures
    */
    virtual void gestureTap (const GestureEvent& event) {}

    /** Called when a long press begins, moves or ends on a component wanting GestureType::longPress.

        @param event The gesture event.
    */
    virtual void gestureLongPress (const GestureEvent& event) {}

    /** Called when a pinch begins, changes or ends on a component wanting GestureType::pinch.

        Touch screen pinches and trackpad pinches both arrive here.

        @param event The gesture event.
    */
    virtual void gesturePinch (const GestureEvent& event) {}

    /** Called when a rotate begins, changes or ends on a component wanting GestureType::rotate.

        @param event The gesture event.
    */
    virtual void gestureRotate (const GestureEvent& event) {}

    /** Called when a multi-finger pan begins, changes or ends on a component wanting GestureType::pan.

        @param event The gesture event.
    */
    virtual void gesturePan (const GestureEvent& event) {}

    /** Called when a swipe is recognized on a component wanting GestureType::swipe.

        @param event The gesture event.
    */
    virtual void gestureSwipe (const GestureEvent& event) {}

private:
    YUP_DECLARE_WEAK_REFERENCEABLE (GestureListener)
};

} // namespace yup
