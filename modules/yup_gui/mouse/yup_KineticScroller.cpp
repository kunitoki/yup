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

namespace
{

constexpr double kineticMaxFrameGapSeconds = 1.0 / 15.0;
constexpr double kineticVelocityWindowSeconds = 0.1;
constexpr double kineticRestBeforeReleaseSeconds = 0.05;
constexpr float kineticSettleDistance = 0.5f;
constexpr float kineticSettleVelocity = 10.0f;

} // namespace

//==============================================================================
KineticScroller::Options& KineticScroller::Options::withOverscrollEnabled (bool shouldBeEnabled)
{
    overscrollEnabled = shouldBeEnabled;
    return *this;
}

KineticScroller::Options& KineticScroller::Options::withOverscrollResistance (float newResistance)
{
    overscrollResistance = newResistance;
    return *this;
}

KineticScroller::Options& KineticScroller::Options::withMaxOverscrollFraction (float newFraction)
{
    maxOverscrollFraction = newFraction;
    return *this;
}

KineticScroller::Options& KineticScroller::Options::withBounceBackTime (float newSeconds)
{
    bounceBackTime = newSeconds;
    return *this;
}

KineticScroller::Options& KineticScroller::Options::withDeceleration (float newDeceleration)
{
    deceleration = newDeceleration;
    return *this;
}

KineticScroller::Options& KineticScroller::Options::withMaxFlingVelocity (float newVelocity)
{
    maxFlingVelocity = newVelocity;
    return *this;
}

KineticScroller::Options& KineticScroller::Options::withScrollAnimationTime (float newSeconds)
{
    scrollAnimationTime = newSeconds;
    return *this;
}

//==============================================================================
KineticScroller::KineticScroller() = default;

KineticScroller::KineticScroller (const Options& initialOptions)
{
    setOptions (initialOptions);
}

//==============================================================================
void KineticScroller::setOptions (const Options& newOptions)
{
    jassert (newOptions.deceleration > 0.0f && newOptions.deceleration < 1.0f);

    options = newOptions;
}

const KineticScroller::Options& KineticScroller::getOptions() const noexcept
{
    return options;
}

//==============================================================================
void KineticScroller::setLimits (float newMinOffset, float newMaxOffset, float newViewportSize)
{
    minOffset = newMinOffset;
    maxOffset = jmax (newMinOffset, newMaxOffset);
    viewportSize = jmax (0.0f, newViewportSize);

    if (state == State::idle)
        offset = clampToLimits (offset);
    else if (state == State::animating)
        animationTarget = clampToLimits (animationTarget);
    else if (state == State::springing)
        springTarget = clampToLimits (springTarget);
}

float KineticScroller::getMinOffset() const noexcept
{
    return minOffset;
}

float KineticScroller::getMaxOffset() const noexcept
{
    return maxOffset;
}

void KineticScroller::setSnapFunction (std::function<float (float, float)> newSnapFunction)
{
    snapFunction = std::move (newSnapFunction);
}

//==============================================================================
void KineticScroller::setOffset (float newOffset)
{
    stop();

    offset = clampToLimits (newOffset);
}

void KineticScroller::animateTo (float targetOffset)
{
    stop();

    animationStart = offset;
    animationTarget = clampToLimits (targetOffset);
    animationElapsed = 0.0f;

    if (animationTarget == offset || options.scrollAnimationTime <= 0.0f)
    {
        offset = animationTarget;
        return;
    }

    state = State::animating;
}

void KineticScroller::translate (float delta)
{
    offset += delta;
    dragStartOffset += delta;
    flingTarget += delta;
    animationStart += delta;
    animationTarget = clampToLimits (animationTarget + delta);

    if (state == State::idle)
        offset = clampToLimits (offset);
}

float KineticScroller::getOffset() const noexcept
{
    return offset;
}

float KineticScroller::getOverscroll() const noexcept
{
    if (offset < minOffset)
        return offset - minOffset;

    if (offset > maxOffset)
        return offset - maxOffset;

    return 0.0f;
}

//==============================================================================
void KineticScroller::beginDrag (float pointerPosition, double timeSeconds)
{
    stop();

    state = State::dragging;
    dragStartPointer = pointerPosition;

    // Catching content mid-bounce: start from the unresisted offset that maps onto where it is now.
    if (offset < minOffset)
        dragStartOffset = minOffset - removeResistance (minOffset - offset);
    else if (offset > maxOffset)
        dragStartOffset = maxOffset + removeResistance (offset - maxOffset);
    else
        dragStartOffset = offset;

    samples.push_back ({ timeSeconds, pointerPosition });
}

void KineticScroller::dragTo (float pointerPosition, double timeSeconds)
{
    if (state != State::dragging)
        return;

    const auto rawOffset = dragStartOffset - (pointerPosition - dragStartPointer);

    if (rawOffset < minOffset)
        offset = minOffset - applyResistance (minOffset - rawOffset);
    else if (rawOffset > maxOffset)
        offset = maxOffset + applyResistance (rawOffset - maxOffset);
    else
        offset = rawOffset;

    // A pointer that reports the same position again is resting, which must not refresh its timestamp.
    if (! samples.empty() && samples.back().position == pointerPosition)
        return;

    samples.push_back ({ timeSeconds, pointerPosition });

    const auto windowStart = timeSeconds - kineticVelocityWindowSeconds;
    const auto firstInWindow = std::find_if (samples.begin(), samples.end(), [&] (const Sample& s)
    {
        return s.time >= windowStart;
    });

    samples.erase (samples.begin(), firstInWindow);
}

void KineticScroller::endDrag (double timeSeconds)
{
    if (state != State::dragging)
        return;

    const auto releaseVelocity = computeReleaseVelocity (timeSeconds);

    samples.clear();
    state = State::idle;

    if (getOverscroll() != 0.0f)
        startSpring (releaseVelocity);
    else
        startFling (releaseVelocity);
}

void KineticScroller::stop()
{
    state = State::idle;
    velocity = 0.0f;
    samples.clear();
}

//==============================================================================
bool KineticScroller::update (double deltaSeconds)
{
    if (! isAnimating())
        return false;

    const auto dt = static_cast<float> (jlimit (0.0, kineticMaxFrameGapSeconds, deltaSeconds));

    switch (state)
    {
        case State::flinging:
            stepFling (dt);
            break;

        case State::springing:
            stepSpring (dt);
            break;

        case State::animating:
        {
            animationElapsed += dt;

            if (animationElapsed >= options.scrollAnimationTime)
            {
                offset = animationTarget;
                state = State::idle;
                break;
            }

            const auto t = 1.0f - animationElapsed / options.scrollAnimationTime;
            offset = animationStart + (animationTarget - animationStart) * (1.0f - t * t * t);
            break;
        }

        case State::idle:
        case State::dragging:
            break;
    }

    return true;
}

bool KineticScroller::isDragging() const noexcept
{
    return state == State::dragging;
}

bool KineticScroller::isAnimating() const noexcept
{
    return state == State::flinging || state == State::springing || state == State::animating;
}

//==============================================================================
float KineticScroller::clampToLimits (float value) const noexcept
{
    return jlimit (minOffset, maxOffset, value);
}

float KineticScroller::applyResistance (float excess) const noexcept
{
    if (! options.overscrollEnabled || viewportSize <= 0.0f)
        return 0.0f;

    const auto resistance = jmax (0.001f, options.overscrollResistance);
    const auto resisted = viewportSize * (1.0f - 1.0f / (excess * resistance / viewportSize + 1.0f));

    return jmin (resisted, getOverscrollCap());
}

float KineticScroller::removeResistance (float excess) const noexcept
{
    if (! options.overscrollEnabled || viewportSize <= 0.0f)
        return 0.0f;

    const auto resistance = jmax (0.001f, options.overscrollResistance);
    const auto fraction = jmin (excess / viewportSize, 0.999f);

    return (viewportSize / resistance) * (1.0f / (1.0f - fraction) - 1.0f);
}

float KineticScroller::getOverscrollCap() const noexcept
{
    return options.overscrollEnabled ? jmax (0.0f, options.maxOverscrollFraction) * viewportSize : 0.0f;
}

float KineticScroller::getDecayRate() const noexcept
{
    return -1000.0f * std::log (jlimit (0.5f, 0.99999f, options.deceleration));
}

float KineticScroller::getSpringRate() const noexcept
{
    return 5.0f / jmax (0.01f, options.bounceBackTime);
}

float KineticScroller::computeReleaseVelocity (double timeSeconds) const
{
    if (samples.empty())
        return 0.0f;

    const auto& last = samples.back();

    if (timeSeconds - last.time > kineticRestBeforeReleaseSeconds)
        return 0.0f;

    const auto& first = samples.front();
    const auto elapsed = last.time - first.time;

    if (elapsed <= 0.0)
        return 0.0f;

    const auto pointerVelocity = static_cast<float> ((last.position - first.position) / elapsed);

    return jlimit (-options.maxFlingVelocity, options.maxFlingVelocity, -pointerVelocity);
}

void KineticScroller::startFling (float releaseVelocity)
{
    const auto decayRate = getDecayRate();
    auto target = offset + releaseVelocity / decayRate;

    if (snapFunction)
    {
        target = clampToLimits (snapFunction (target, releaseVelocity));
        releaseVelocity = (target - offset) * decayRate;
    }

    if (std::abs (target - offset) < kineticSettleDistance)
    {
        if (snapFunction)
            offset = target;

        return;
    }

    flingTarget = target;
    velocity = releaseVelocity;
    state = State::flinging;
}

void KineticScroller::startSpring (float initialVelocity)
{
    if (! options.overscrollEnabled)
    {
        stop();
        offset = clampToLimits (offset);
        return;
    }

    springTarget = clampToLimits (offset);
    velocity = initialVelocity;
    state = State::springing;
}

void KineticScroller::stepFling (float deltaSeconds)
{
    const auto decayRate = getDecayRate();
    const auto newVelocity = velocity * std::exp (-decayRate * deltaSeconds);

    offset += (velocity - newVelocity) / decayRate;
    velocity = newVelocity;

    if (offset < minOffset || offset > maxOffset)
    {
        startSpring (velocity);
        return;
    }

    if (std::abs (flingTarget - offset) < kineticSettleDistance)
    {
        offset = flingTarget;
        stop();
    }
}

void KineticScroller::stepSpring (float deltaSeconds)
{
    // A critically damped spring, integrated in closed form so the result does not depend on the frame rate.
    const auto rate = getSpringRate();
    const auto displacement = offset - springTarget;
    const auto c = velocity + rate * displacement;
    const auto decay = std::exp (-rate * deltaSeconds);

    offset = springTarget + (displacement + c * deltaSeconds) * decay;
    velocity = (velocity - rate * c * deltaSeconds) * decay;

    const auto cap = getOverscrollCap();

    if (offset > maxOffset + cap)
    {
        offset = maxOffset + cap;
        velocity = jmin (velocity, 0.0f);
    }
    else if (offset < minOffset - cap)
    {
        offset = minOffset - cap;
        velocity = jmax (velocity, 0.0f);
    }

    if (std::abs (offset - springTarget) < kineticSettleDistance && std::abs (velocity) < kineticSettleVelocity)
    {
        offset = springTarget;
        stop();
    }
}

} // namespace yup
