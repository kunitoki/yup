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

GestureEvent::GestureEvent (GestureType gestureType, Phase gesturePhase) noexcept
    : type (gestureType)
    , phase (gesturePhase)
{
}

GestureEvent GestureEvent::withPhase (Phase newPhase) const noexcept
{
    auto result = *this;
    result.phase = newPhase;
    return result;
}

GestureEvent GestureEvent::withPosition (Point<float> newPosition) const noexcept
{
    auto result = *this;
    result.position = newPosition;
    return result;
}

GestureEvent GestureEvent::withRelativePositionTo (Component* targetComponent) const noexcept
{
    if (targetComponent == nullptr)
        return *this;

    auto result = *this;
    result.position = targetComponent->getLocalPointFromTopLevel (position);
    result.sourceComponent = targetComponent;
    return result;
}

GestureEvent GestureEvent::withTouchCount (int newTouchCount) const noexcept
{
    auto result = *this;
    result.touchCount = newTouchCount;
    return result;
}

GestureEvent GestureEvent::withTrackpad (bool isFromTrackpad) const noexcept
{
    auto result = *this;
    result.trackpad = isFromTrackpad;
    return result;
}

GestureEvent GestureEvent::withScale (float newScale, float newScaleDelta) const noexcept
{
    auto result = *this;
    result.scale = newScale;
    result.scaleDelta = newScaleDelta;
    return result;
}

GestureEvent GestureEvent::withRotation (float newRotation, float newRotationDelta) const noexcept
{
    auto result = *this;
    result.rotation = newRotation;
    result.rotationDelta = newRotationDelta;
    return result;
}

GestureEvent GestureEvent::withTranslation (Point<float> newTranslation, Point<float> newTranslationDelta) const noexcept
{
    auto result = *this;
    result.translation = newTranslation;
    result.translationDelta = newTranslationDelta;
    return result;
}

GestureEvent GestureEvent::withVelocity (Point<float> newVelocity) const noexcept
{
    auto result = *this;
    result.velocity = newVelocity;
    return result;
}

GestureEvent GestureEvent::withSwipeDirection (Direction newDirection) const noexcept
{
    auto result = *this;
    result.swipeDirection = newDirection;
    return result;
}

GestureEvent GestureEvent::withTapCount (int newTapCount) const noexcept
{
    auto result = *this;
    result.tapCount = newTapCount;
    return result;
}

GestureEvent GestureEvent::withModifiers (KeyModifiers newModifiers) const noexcept
{
    auto result = *this;
    result.modifiers = newModifiers;
    return result;
}

GestureEvent GestureEvent::withSourceComponent (Component* newComponent) const noexcept
{
    auto result = *this;
    result.sourceComponent = newComponent;
    return result;
}

} // namespace yup
