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

//==============================================================================
/**
    One-dimensional scroll physics for touch-driven scrolling.

    A KineticScroller owns a scroll offset and moves it the way a touch list does: it follows a
    dragging pointer 1:1, resists with a rubber band past the edges, keeps going after release with
    a decaying fling, springs back to the nearest edge when it comes to rest outside the valid
    range, and can land exactly on a snap point chosen by a snap function.

    It is not a Component and never reads the clock: every time is passed in explicitly, either as
    the timestamp of a pointer sample or as the frame delta handed to update(). That makes it fully
    deterministic, so a test can drive it with made-up times, and it lets any scrolling component
    reuse it by feeding it pointer positions along its scroll axis and calling update() once per
    frame (typically from Component::refreshDisplay()).

    The offset grows when content moves towards the start of the viewport, so dragging the pointer
    towards the start increases it, exactly like a scroll position.

    @code
    KineticScroller scroller;
    scroller.setLimits (0.0f, contentSize - viewportSize, viewportSize);

    // mouseDown / mouseDrag / mouseUp
    scroller.beginDrag (event.getPosition().getY(), clock);
    scroller.dragTo (event.getPosition().getY(), clock);
    scroller.endDrag (clock);

    // refreshDisplay
    if (scroller.update (lastFrameTimeSeconds))
        layoutContent (scroller.getOffset());
    @endcode

    @see ListBox
*/
class YUP_API KineticScroller
{
public:
    //==============================================================================
    /** The tunable parts of the physics. */
    struct Options
    {
        /** Whether the offset may leave the valid range while dragging and flinging. When false,
            it is clamped to the edges instead, and a fling stops dead when it reaches one. */
        bool overscrollEnabled = true;

        /** How strongly dragging past an edge is resisted. Smaller values resist more. */
        float overscrollResistance = 0.55f;

        /** The furthest the offset may go past an edge, as a fraction of the viewport size. */
        float maxOverscrollFraction = 0.5f;

        /** Roughly how long, in seconds, the offset takes to spring back to an edge. */
        float bounceBackTime = 0.35f;

        /** The factor a fling's velocity is multiplied by every millisecond, in the open range (0, 1).
            Values closer to 1 fling further. */
        float deceleration = 0.998f;

        /** The fastest a fling may start, in points per second. */
        float maxFlingVelocity = 8000.0f;

        /** How long, in seconds, animateTo() takes to reach its target. */
        float scrollAnimationTime = 0.3f;

        //==============================================================================
        /** Sets whether overscroll is enabled. */
        Options& withOverscrollEnabled (bool shouldBeEnabled);

        /** Sets the rubber-band resistance. */
        Options& withOverscrollResistance (float newResistance);

        /** Sets the overscroll cap, as a fraction of the viewport size. */
        Options& withMaxOverscrollFraction (float newFraction);

        /** Sets the bounce back time in seconds. */
        Options& withBounceBackTime (float newSeconds);

        /** Sets the per-millisecond fling velocity multiplier. */
        Options& withDeceleration (float newDeceleration);

        /** Sets the fastest fling velocity, in points per second. */
        Options& withMaxFlingVelocity (float newVelocity);

        /** Sets the animateTo() duration in seconds. */
        Options& withScrollAnimationTime (float newSeconds);
    };

    //==============================================================================
    /** Creates a scroller at offset 0, with an empty range and the default options. */
    KineticScroller();

    /** Creates a scroller at offset 0, with an empty range and the given options.

        @param initialOptions  The physics options to start with, see setOptions().
    */
    explicit KineticScroller (const Options& initialOptions);

    //==============================================================================
    /** Replaces the physics options. Any motion in progress continues with the new values. */
    void setOptions (const Options& newOptions);

    /** Returns the physics options. */
    const Options& getOptions() const noexcept;

    //==============================================================================
    /** Sets the valid offset range and the size of the viewport along the scroll axis.

        A negative minimum is allowed: pull-to-refresh uses it to hold content below the leading
        edge while a refresh is running.

        When the scroller is idle an offset outside the new range is clamped straight away; while it
        is dragging or moving, the motion itself brings the offset back into range.

        @param minOffset      The smallest valid offset.
        @param maxOffset      The largest valid offset. Values below minOffset are treated as minOffset.
        @param viewportSize   The visible size along the scroll axis, which scales the overscroll.
    */
    void setLimits (float minOffset, float maxOffset, float viewportSize);

    /** Returns the smallest valid offset. */
    float getMinOffset() const noexcept;

    /** Returns the largest valid offset. */
    float getMaxOffset() const noexcept;

    //==============================================================================
    /** Sets the function that picks where a fling comes to rest.

        When set, endDrag() calls it with the offset the fling would naturally stop at and the release
        velocity, and the fling then lands exactly on the offset it returns, clamped to the valid
        range. Pass an empty function to disable snapping.
    */
    void setSnapFunction (std::function<float (float restOffset, float velocity)> newSnapFunction);

    //==============================================================================
    /** Jumps to an offset, clamped to the valid range, and stops any drag or motion. */
    void setOffset (float newOffset);

    /** Animates to an offset, clamped to the valid range, with an ease-out curve over
        Options::scrollAnimationTime. */
    void animateTo (float targetOffset);

    /** Shifts the offset, and any drag or motion in progress with it, without interrupting them.

        This is for content that grew or shrank before the visible part: moving the offset by the
        same amount keeps what is on screen in place, even under a finger or during a fling.

        @param delta  The amount to add to the offset.
    */
    void translate (float delta);

    /** Returns the current offset, which may lie outside the valid range while overscrolled. */
    float getOffset() const noexcept;

    /** Returns how far the offset lies outside the valid range: negative before the minimum,
        positive past the maximum and 0 inside it. */
    float getOverscroll() const noexcept;

    //==============================================================================
    /** Starts following a pointer. Any motion in progress stops where it is.

        @param pointerPosition  The pointer position along the scroll axis.
        @param timeSeconds      The time of this sample, on any clock the caller keeps consistent.
    */
    void beginDrag (float pointerPosition, double timeSeconds);

    /** Moves the offset with the pointer.

        @param pointerPosition  The pointer position along the scroll axis.
        @param timeSeconds      The time of this sample.
    */
    void dragTo (float pointerPosition, double timeSeconds);

    /** Releases the pointer, starting a fling, a snap or a bounce back as appropriate.

        The release velocity comes from the samples of the last ~100 ms. A pointer that rested for
        more than ~50 ms before being released does not fling at all.

        @param timeSeconds  The time of the release.
    */
    void endDrag (double timeSeconds);

    /** Stops any drag or motion, leaving the offset where it is. */
    void stop();

    //==============================================================================
    /** Advances the motion.

        @param deltaSeconds  The time since the previous call. Gaps longer than 1/15 s are treated
                             as 1/15 s, so a stalled frame never makes content jump.
        @return True while the offset is still moving.
    */
    bool update (double deltaSeconds);

    /** Returns true between beginDrag() and endDrag(). */
    bool isDragging() const noexcept;

    /** Returns true while a fling, a snap, a bounce back or an animateTo() is running. */
    bool isAnimating() const noexcept;

private:
    //==============================================================================
    enum class State
    {
        idle,
        dragging,
        flinging,
        springing,
        animating
    };

    struct Sample
    {
        double time;
        float position;
    };

    float clampToLimits (float value) const noexcept;
    float applyResistance (float excess) const noexcept;
    float removeResistance (float excess) const noexcept;
    float getOverscrollCap() const noexcept;
    float getDecayRate() const noexcept;
    float getSpringRate() const noexcept;
    float computeReleaseVelocity (double timeSeconds) const;
    void startFling (float velocity);
    void startSpring (float velocity);
    void stepFling (float deltaSeconds);
    void stepSpring (float deltaSeconds);

    //==============================================================================
    Options options;
    std::function<float (float, float)> snapFunction;

    State state = State::idle;
    float offset = 0.0f;
    float velocity = 0.0f;
    float minOffset = 0.0f;
    float maxOffset = 0.0f;
    float viewportSize = 0.0f;

    float dragStartPointer = 0.0f;
    float dragStartOffset = 0.0f;
    std::vector<Sample> samples;

    float flingTarget = 0.0f;
    float flingDecayRate = 1.0f;
    float springTarget = 0.0f;

    float animationStart = 0.0f;
    float animationTarget = 0.0f;
    float animationElapsed = 0.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KineticScroller)
};

} // namespace yup
