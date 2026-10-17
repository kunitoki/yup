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

GestureRecognizer::GestureRecognizer (Callback callbackToUse)
    : GestureRecognizer (std::move (callbackToUse), Options {})
{
}

GestureRecognizer::GestureRecognizer (Callback callbackToUse, Options optionsToUse)
    : callback (std::move (callbackToUse))
    , options (optionsToUse)
{
}

//==============================================================================

void GestureRecognizer::touchDown (int64 touchId, Point<float> position, double timeMs)
{
    if (findTouch (touchId) >= 0)
        return;

    if (touches.empty())
    {
        sequenceStartTime = timeMs;
        maxTouches = 0;
        moved = false;
        sequenceCanceled = false;
        continuousRecognized = false;
        pinchOrRotateRecognized = false;
        longPressDone = false;
    }

    endContinuous (GestureEvent::Phase::ended);

    // Another finger turns a long press into something else
    if (longPressActive)
        endLongPress (GestureEvent::Phase::canceled);

    touches.push_back ({ touchId, position, position });

    if (getNumTouches() > maxTouches)
    {
        maxTouches = getNumTouches();
        tapPosition = getStartCentroid();
        swipeEvaluated = false;
    }

    startGroup (timeMs);
}

void GestureRecognizer::touchMove (int64 touchId, Point<float> position, double timeMs)
{
    const auto index = findTouch (touchId);
    if (index < 0)
        return;

    updateMovement (index, position);

    if (sequenceCanceled)
        return;

    addSample (timeMs);

    if (longPressActive)
    {
        emit (GestureEvent (GestureType::longPress, GestureEvent::Phase::changed)
                  .withPosition (position)
                  .withTouchCount (1));
        return;
    }

    advanceTo (timeMs);

    if (getNumTouches() >= 2)
        updateContinuous();
}

void GestureRecognizer::touchUp (int64 touchId, Point<float> position, double timeMs)
{
    const auto index = findTouch (touchId);
    if (index < 0)
        return;

    updateMovement (index, position);

    if (! sequenceCanceled)
    {
        addSample (timeMs);

        // The first finger leaving the largest group decides the swipe, before the others follow
        if (! swipeEvaluated && getNumTouches() == maxTouches)
        {
            swipeEvaluated = true;
            evaluateSwipe();
        }
    }

    endContinuous (GestureEvent::Phase::ended);

    if (longPressActive)
        endLongPress (GestureEvent::Phase::ended);

    touches.erase (touches.begin() + index);

    if (touches.empty())
        evaluateTap (timeMs);
    else
        startGroup (timeMs);
}

void GestureRecognizer::touchCancel (int64 touchId)
{
    const auto index = findTouch (touchId);
    if (index < 0)
        return;

    sequenceCanceled = true;

    endContinuous (GestureEvent::Phase::canceled);

    if (longPressActive)
        endLongPress (GestureEvent::Phase::canceled);

    touches.erase (touches.begin() + index);
}

void GestureRecognizer::advanceTo (double timeMs)
{
    if (getNumTouches() != 1 || maxTouches != 1 || moved || sequenceCanceled || longPressActive || longPressDone)
        return;

    if (timeMs - sequenceStartTime < options.longPressDelay)
        return;

    longPressActive = true;
    longPressDone = true;

    emit (GestureEvent (GestureType::longPress, GestureEvent::Phase::began)
              .withPosition (touches.front().position)
              .withTouchCount (1));
}

void GestureRecognizer::cancelAll()
{
    endContinuous (GestureEvent::Phase::canceled);

    if (longPressActive)
        endLongPress (GestureEvent::Phase::canceled);

    touches.clear();
    samples.clear();
}

//==============================================================================

void GestureRecognizer::emit (const GestureEvent& event)
{
    if (callback)
        callback (event);
}

int GestureRecognizer::findTouch (int64 touchId) const noexcept
{
    for (std::size_t i = 0; i < touches.size(); ++i)
    {
        if (touches[i].id == touchId)
            return static_cast<int> (i);
    }

    return -1;
}

Point<float> GestureRecognizer::getCentroid() const noexcept
{
    if (touches.empty())
        return {};

    Point<float> sum;
    for (const auto& touch : touches)
        sum += touch.position;

    return sum / static_cast<float> (touches.size());
}

Point<float> GestureRecognizer::getStartCentroid() const noexcept
{
    if (touches.empty())
        return {};

    Point<float> sum;
    for (const auto& touch : touches)
        sum += touch.start;

    return sum / static_cast<float> (touches.size());
}

float GestureRecognizer::getSpan() const noexcept
{
    return touches.size() < 2 ? 0.0f : touches[0].position.distanceTo (touches[1].position);
}

float GestureRecognizer::getAngle() const noexcept
{
    if (touches.size() < 2)
        return 0.0f;

    const auto delta = touches[1].position - touches[0].position;
    return std::atan2 (delta.getY(), delta.getX());
}

Point<float> GestureRecognizer::getVelocity() const noexcept
{
    if (samples.size() < 2)
        return {};

    const auto& last = samples.back();
    const auto* reference = &samples.front();

    for (auto it = samples.rbegin() + 1; it != samples.rend(); ++it)
    {
        reference = &*it;

        if (it->time <= last.time - options.velocityWindow)
            break;
    }

    const auto seconds = static_cast<float> ((last.time - reference->time) / 1000.0);
    if (seconds <= 0.0f)
        return {};

    return (last.centroid - reference->centroid) / seconds;
}

void GestureRecognizer::addSample (double timeMs)
{
    samples.push_back ({ timeMs, getCentroid() });

    const auto oldest = timeMs - 2.0 * options.velocityWindow;
    while (samples.size() > 2 && samples[1].time <= oldest)
        samples.erase (samples.begin());
}

void GestureRecognizer::startGroup (double timeMs)
{
    samples.clear();
    addSample (timeMs);

    baseCentroid = getCentroid();
    baseSpan = jmax (1.0f, getSpan());
    previousAngle = getAngle();
    rotationTotal = 0.0f;
    lastScale = 1.0f;
    lastRotation = 0.0f;
    lastTranslation = {};
}

void GestureRecognizer::updateMovement (int touchIndex, Point<float> position)
{
    auto& touch = touches[static_cast<std::size_t> (touchIndex)];
    touch.position = position;

    if (touch.start.distanceTo (position) > options.tapSlop)
        moved = true;
}

void GestureRecognizer::updateContinuous()
{
    const auto centroid = getCentroid();
    const auto touchCount = getNumTouches();

    if (touchCount == 2)
    {
        const auto scale = getSpan() / baseSpan;

        if (! pinchActive && std::abs (scale - 1.0f) >= options.pinchThreshold)
        {
            pinchActive = true;
            pinchOrRotateRecognized = true;

            emit (GestureEvent (GestureType::pinch, GestureEvent::Phase::began)
                      .withPosition (centroid)
                      .withTouchCount (touchCount)
                      .withScale (scale, scale));

            lastScale = scale;
        }
        else if (pinchActive && scale != lastScale)
        {
            emit (GestureEvent (GestureType::pinch, GestureEvent::Phase::changed)
                      .withPosition (centroid)
                      .withTouchCount (touchCount)
                      .withScale (scale, scale / lastScale));

            lastScale = scale;
        }

        const auto angle = getAngle();
        auto angleDelta = angle - previousAngle;
        if (angleDelta > MathConstants<float>::pi)
            angleDelta -= MathConstants<float>::twoPi;
        else if (angleDelta < -MathConstants<float>::pi)
            angleDelta += MathConstants<float>::twoPi;

        previousAngle = angle;
        rotationTotal += angleDelta;

        if (! rotateActive && std::abs (rotationTotal) >= options.rotateThreshold)
        {
            rotateActive = true;
            pinchOrRotateRecognized = true;

            emit (GestureEvent (GestureType::rotate, GestureEvent::Phase::began)
                      .withPosition (centroid)
                      .withTouchCount (touchCount)
                      .withRotation (rotationTotal, rotationTotal));

            lastRotation = rotationTotal;
        }
        else if (rotateActive && rotationTotal != lastRotation)
        {
            emit (GestureEvent (GestureType::rotate, GestureEvent::Phase::changed)
                      .withPosition (centroid)
                      .withTouchCount (touchCount)
                      .withRotation (rotationTotal, rotationTotal - lastRotation));

            lastRotation = rotationTotal;
        }
    }

    const auto translation = centroid - baseCentroid;

    if (! panActive && translation.distanceTo ({}) >= options.panThreshold)
    {
        panActive = true;

        emit (GestureEvent (GestureType::pan, GestureEvent::Phase::began)
                  .withPosition (centroid)
                  .withTouchCount (touchCount)
                  .withTranslation (translation, translation));

        lastTranslation = translation;
    }
    else if (panActive && translation != lastTranslation)
    {
        emit (GestureEvent (GestureType::pan, GestureEvent::Phase::changed)
                  .withPosition (centroid)
                  .withTouchCount (touchCount)
                  .withTranslation (translation, translation - lastTranslation));

        lastTranslation = translation;
    }

    continuousRecognized = continuousRecognized || pinchActive || rotateActive || panActive;
}

void GestureRecognizer::endContinuous (GestureEvent::Phase phase)
{
    const auto centroid = getCentroid();
    const auto touchCount = getNumTouches();
    const auto wasPinching = std::exchange (pinchActive, false);
    const auto wasRotating = std::exchange (rotateActive, false);
    const auto wasPanning = std::exchange (panActive, false);

    if (wasPinching)
    {
        emit (GestureEvent (GestureType::pinch, phase)
                  .withPosition (centroid)
                  .withTouchCount (touchCount)
                  .withScale (lastScale, 1.0f));
    }

    if (wasRotating)
    {
        emit (GestureEvent (GestureType::rotate, phase)
                  .withPosition (centroid)
                  .withTouchCount (touchCount)
                  .withRotation (lastRotation, 0.0f));
    }

    if (wasPanning)
    {
        auto event = GestureEvent (GestureType::pan, phase)
                         .withPosition (centroid)
                         .withTouchCount (touchCount)
                         .withTranslation (lastTranslation, {});

        if (phase == GestureEvent::Phase::ended)
            event = event.withVelocity (getVelocity());

        emit (event);
    }
}

void GestureRecognizer::endLongPress (GestureEvent::Phase phase)
{
    longPressActive = false;

    const auto position = touches.empty() ? Point<float>() : touches.front().position;

    emit (GestureEvent (GestureType::longPress, phase)
              .withPosition (position)
              .withTouchCount (1));
}

void GestureRecognizer::evaluateSwipe()
{
    if (pinchOrRotateRecognized || longPressDone)
        return;

    const auto translation = getCentroid() - getStartCentroid();
    const auto velocity = getVelocity();
    const auto horizontal = std::abs (translation.getX()) >= std::abs (translation.getY());
    const auto distance = std::abs (horizontal ? translation.getX() : translation.getY());
    const auto speed = horizontal ? velocity.getX() : velocity.getY();
    const auto direction = horizontal ? translation.getX() : translation.getY();

    // The release speed must point the same way the fingers traveled
    if (distance < options.swipeMinDistance || std::abs (speed) < options.swipeMinVelocity || speed * direction <= 0.0f)
        return;

    const auto swipeDirection = horizontal ? (direction > 0.0f ? GestureEvent::Direction::right : GestureEvent::Direction::left)
                                           : (direction > 0.0f ? GestureEvent::Direction::down : GestureEvent::Direction::up);

    emit (GestureEvent (GestureType::swipe, GestureEvent::Phase::ended)
              .withPosition (getCentroid())
              .withTouchCount (getNumTouches())
              .withTranslation (translation, translation)
              .withVelocity (velocity)
              .withSwipeDirection (swipeDirection));
}

void GestureRecognizer::evaluateTap (double timeMs)
{
    if (sequenceCanceled || moved || longPressDone || continuousRecognized)
        return;

    if (timeMs - sequenceStartTime > options.tapMaxDuration)
        return;

    const auto isRepeat = lastTapCount > 0
                       && lastTapTouches == maxTouches
                       && sequenceStartTime - lastTapTime <= options.doubleTapInterval
                       && tapPosition.distanceTo (lastTapPosition) <= options.doubleTapSlop;

    lastTapCount = isRepeat ? lastTapCount + 1 : 1;
    lastTapTime = timeMs;
    lastTapPosition = tapPosition;
    lastTapTouches = maxTouches;

    emit (GestureEvent (GestureType::tap, GestureEvent::Phase::ended)
              .withPosition (tapPosition)
              .withTouchCount (maxTouches)
              .withTapCount (lastTapCount));
}

} // namespace yup
