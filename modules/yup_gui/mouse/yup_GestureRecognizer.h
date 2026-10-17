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
/** Turns raw touch points into GestureEvents.

    The recognizer is platform free: the windowing layer feeds it every finger, with a time in
    milliseconds, and it reports the gestures it recognizes through a callback. Positions are
    reported in the same coordinate space the touches are fed in.

    Recognized gestures:
    - tap: all fingers lifted, within Options::tapMaxDuration, without moving. Consecutive taps
      with the same number of fingers in the same place count up (double tap, triple tap).
    - longPress: one finger held still for Options::longPressDelay. Moving that finger
      afterwards reports changed, lifting it ends the long press.
    - pinch and rotate: two fingers moving apart, together or around each other.
    - pan: two or more fingers moving together. It ends with the release velocity, for flings.
    - swipe: fingers moving fast in one direction and lifting, reported once with its direction.

    Pinch, rotate and pan can run at the same time, from the same fingers. When the number of
    fingers changes, the running continuous gestures end and start again from the new fingers.

    Time only moves forward through the calls, so a long press needs advanceTo() calls while a
    finger rests; the windowing layer schedules one for the long press delay.

    @see GestureEvent, Component::setWantedGestures
*/
class YUP_API GestureRecognizer
{
public:
    /** Thresholds controlling when gestures are recognized. Distances are in pixels, times in milliseconds. */
    struct Options
    {
        /** How far a finger may move and still count as a tap or a long press. */
        float tapSlop = 10.0f;

        /** The longest a tap may last, from the first finger down to the last finger up. */
        double tapMaxDuration = 300.0;

        /** The longest gap between a tap and the next one for them to count as a double tap. */
        double doubleTapInterval = 300.0;

        /** How far apart two taps may be and still count as a double tap. */
        float doubleTapSlop = 40.0f;

        /** How long a finger must rest to start a long press. */
        double longPressDelay = 500.0;

        /** The relative change in finger distance that starts a pinch, e.g. 0.08 for 8%. */
        float pinchThreshold = 0.08f;

        /** The finger rotation in radians that starts a rotate. */
        float rotateThreshold = 0.15f;

        /** How far the fingers must move together to start a pan. */
        float panThreshold = 12.0f;

        /** The shortest distance a swipe must cover. */
        float swipeMinDistance = 40.0f;

        /** The lowest release speed, along the swipe direction, for a swipe in pixels per second. */
        float swipeMinVelocity = 300.0f;

        /** The window over which the release velocity is measured. */
        double velocityWindow = 100.0;
    };

    /** The callback receiving recognized gestures. */
    using Callback = std::function<void (const GestureEvent&)>;

    //==============================================================================
    /** Creates a recognizer reporting gestures to a callback, with the default thresholds. */
    explicit GestureRecognizer (Callback callback);

    /** Creates a recognizer reporting gestures to a callback, with custom thresholds. */
    GestureRecognizer (Callback callback, Options options);

    /** Returns the thresholds in use. */
    const Options& getOptions() const noexcept { return options; }

    //==============================================================================
    /** Feeds a finger touching down. Fingers already down are ignored. */
    void touchDown (int64 touchId, Point<float> position, double timeMs);

    /** Feeds a finger moving. Unknown fingers are ignored. */
    void touchMove (int64 touchId, Point<float> position, double timeMs);

    /** Feeds a finger lifting. Unknown fingers are ignored. */
    void touchUp (int64 touchId, Point<float> position, double timeMs);

    /** Feeds a finger canceled by the system.

        Running gestures report canceled, and no tap or swipe is reported until every finger has lifted.
    */
    void touchCancel (int64 touchId);

    /** Moves time forward without touch input, so a resting finger can start a long press. */
    void advanceTo (double timeMs);

    /** Cancels every running gesture and forgets all fingers. */
    void cancelAll();

    //==============================================================================
    /** Returns the number of fingers currently down. */
    int getNumTouches() const noexcept { return static_cast<int> (touches.size()); }

private:
    struct Touch
    {
        int64 id = 0;
        Point<float> start;
        Point<float> position;
    };

    struct Sample
    {
        double time = 0.0;
        Point<float> centroid;
    };

    void emit (const GestureEvent& event);
    int findTouch (int64 touchId) const noexcept;
    Point<float> getCentroid() const noexcept;
    Point<float> getStartCentroid() const noexcept;
    float getSpan() const noexcept;
    float getAngle() const noexcept;
    Point<float> getVelocity() const noexcept;
    void addSample (double timeMs);
    void startGroup (double timeMs);
    void updateMovement (int touchIndex, Point<float> position);
    void updateContinuous();
    void endContinuous (GestureEvent::Phase phase);
    void endLongPress (GestureEvent::Phase phase);
    void evaluateSwipe();
    void evaluateTap (double timeMs);

    Callback callback;
    Options options;

    std::vector<Touch> touches;
    std::vector<Sample> samples;

    double sequenceStartTime = 0.0;
    Point<float> tapPosition;
    int maxTouches = 0;
    bool moved = false;
    bool sequenceCanceled = false;
    bool continuousRecognized = false;
    bool pinchOrRotateRecognized = false;
    bool swipeEvaluated = false;

    bool longPressActive = false;
    bool longPressDone = false;

    Point<float> baseCentroid;
    float baseSpan = 1.0f;
    float previousAngle = 0.0f;
    float rotationTotal = 0.0f;
    float lastScale = 1.0f;
    float lastRotation = 0.0f;
    Point<float> lastTranslation;
    bool pinchActive = false;
    bool rotateActive = false;
    bool panActive = false;

    double lastTapTime = 0.0;
    Point<float> lastTapPosition;
    int lastTapCount = 0;
    int lastTapTouches = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GestureRecognizer)
};

} // namespace yup
