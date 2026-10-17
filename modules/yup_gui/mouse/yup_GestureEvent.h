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

class YUP_API Component;

//==============================================================================
/** The kinds of gesture a GestureRecognizer reports.

    Each GestureEvent carries exactly one of these values. Combine them with `|` to tell
    Component::setWantedGestures() which gestures a component wants.

    @see GestureEvent, Component::setWantedGestures
*/
enum class GestureType : uint32
{
    none = 0,
    tap = 1 << 0,       /**< One or more fingers touched and lifted without moving. */
    longPress = 1 << 1, /**< A single finger stayed down without moving. */
    pinch = 1 << 2,     /**< Two fingers moved apart or together, or a trackpad pinch. */
    rotate = 1 << 3,    /**< Two fingers twisted around their center. */
    pan = 1 << 4,       /**< Two or more fingers moved together. */
    swipe = 1 << 5,     /**< One or more fingers moved quickly in one direction and lifted. */
    all = tap | longPress | pinch | rotate | pan | swipe
};

YUP_DECLARE_SCOPED_ENUM_BITWISE_OPERATORS (GestureType)

//==============================================================================
/** Describes a touch or trackpad gesture delivered to a component.

    Continuous gestures (long press, pinch, rotate and pan) go through the began, changed and
    ended phases, or end with canceled. Discrete gestures (tap and swipe) arrive once, in the
    ended phase.

    Totals (scale, rotation, translation) are measured from the start of the gesture, and the
    deltas from the previous event of the same gesture, so an incremental consumer can just
    apply the deltas.

    @see GestureType, GestureListener::gesturePinch, Component::setWantedGestures
*/
class YUP_API GestureEvent
{
public:
    /** The phase of a gesture. */
    enum class Phase
    {
        began,    /**< The gesture was just recognized. */
        changed,  /**< A continuous gesture moved. */
        ended,    /**< The gesture finished normally, or a discrete gesture happened. */
        canceled  /**< The gesture was interrupted, for example by the system. */
    };

    /** The direction of a swipe. */
    enum class Direction
    {
        none,
        left,
        right,
        up,
        down
    };

    //==============================================================================
    /** Creates an empty event, of type GestureType::none in the ended phase. */
    GestureEvent() noexcept = default;

    /** Creates an event of a type and phase, with neutral values for everything else. */
    GestureEvent (GestureType type, Phase phase) noexcept;

    //==============================================================================
    /** Returns the gesture type: a single GestureType value. */
    GestureType getType() const noexcept { return type; }

    /** Returns the phase of the gesture. */
    Phase getPhase() const noexcept { return phase; }

    /** Returns a copy with a different phase. */
    GestureEvent withPhase (Phase newPhase) const noexcept;

    //==============================================================================
    /** Returns the gesture position: the center of its fingers, or the cursor for a trackpad.

        It is relative to the component receiving the event.
    */
    Point<float> getPosition() const noexcept { return position; }

    /** Returns a copy at a different position. */
    GestureEvent withPosition (Point<float> newPosition) const noexcept;

    /** Returns a copy with its position relative to a component.

        Positions are created relative to the top level component and converted to the
        receiving component before delivery.
    */
    GestureEvent withRelativePositionTo (Component* targetComponent) const noexcept;

    //==============================================================================
    /** Returns the number of fingers in the gesture, or 0 for a trackpad gesture. */
    int getTouchCount() const noexcept { return touchCount; }

    /** Returns a copy with a different number of fingers. */
    GestureEvent withTouchCount (int newTouchCount) const noexcept;

    /** Returns true if the gesture comes from a trackpad rather than a touch screen. */
    bool isTrackpad() const noexcept { return trackpad; }

    /** Returns a copy marked as coming from a trackpad, or not. */
    GestureEvent withTrackpad (bool isFromTrackpad) const noexcept;

    //==============================================================================
    /** Returns the scale since the start of a pinch: above 1 zooms in, below 1 zooms out. */
    float getScale() const noexcept { return scale; }

    /** Returns the scale factor since the previous pinch event. */
    float getScaleDelta() const noexcept { return scaleDelta; }

    /** Returns a copy with a different total and delta scale. */
    GestureEvent withScale (float newScale, float newScaleDelta) const noexcept;

    //==============================================================================
    /** Returns the rotation in radians since the start of a rotate, positive clockwise on screen. */
    float getRotation() const noexcept { return rotation; }

    /** Returns the rotation in radians since the previous rotate event. */
    float getRotationDelta() const noexcept { return rotationDelta; }

    /** Returns a copy with a different total and delta rotation. */
    GestureEvent withRotation (float newRotation, float newRotationDelta) const noexcept;

    //==============================================================================
    /** Returns how far the fingers moved since the start of a pan or swipe, in pixels. */
    Point<float> getTranslation() const noexcept { return translation; }

    /** Returns how far the fingers moved since the previous pan event, in pixels. */
    Point<float> getTranslationDelta() const noexcept { return translationDelta; }

    /** Returns a copy with a different total and delta translation. */
    GestureEvent withTranslation (Point<float> newTranslation, Point<float> newTranslationDelta) const noexcept;

    //==============================================================================
    /** Returns the finger velocity in pixels per second.

        Set on a pan that ends and on a swipe, so a view can keep moving with a fling.
    */
    Point<float> getVelocity() const noexcept { return velocity; }

    /** Returns a copy with a different velocity. */
    GestureEvent withVelocity (Point<float> newVelocity) const noexcept;

    //==============================================================================
    /** Returns the dominant direction of a swipe, or Direction::none for other gestures. */
    Direction getSwipeDirection() const noexcept { return swipeDirection; }

    /** Returns a copy with a different swipe direction. */
    GestureEvent withSwipeDirection (Direction newDirection) const noexcept;

    //==============================================================================
    /** Returns the number of consecutive taps: 1 for a tap, 2 for a double tap, and so on. */
    int getTapCount() const noexcept { return tapCount; }

    /** Returns a copy with a different tap count. */
    GestureEvent withTapCount (int newTapCount) const noexcept;

    //==============================================================================
    /** Returns the key modifiers held during the gesture. */
    KeyModifiers getModifiers() const noexcept { return modifiers; }

    /** Returns a copy with different key modifiers. */
    GestureEvent withModifiers (KeyModifiers newModifiers) const noexcept;

    //==============================================================================
    /** Returns the component the gesture was delivered to, if any. */
    Component* getSourceComponent() const noexcept { return sourceComponent; }

    /** Returns a copy with a different source component. */
    GestureEvent withSourceComponent (Component* newComponent) const noexcept;

private:
    GestureType type = GestureType::none;
    Phase phase = Phase::ended;
    Point<float> position;
    Point<float> translation;
    Point<float> translationDelta;
    Point<float> velocity;
    float scale = 1.0f;
    float scaleDelta = 1.0f;
    float rotation = 0.0f;
    float rotationDelta = 0.0f;
    int touchCount = 0;
    int tapCount = 0;
    Direction swipeDirection = Direction::none;
    KeyModifiers modifiers;
    Component* sourceComponent = nullptr;
    bool trackpad = false;
};

} // namespace yup
