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
    A fixed capacity multistage curve made of breakpoints joined by shaped segments.

    Each point has a time, a level in [0, 1] and the curve of the segment leading into
    it, in [-1, 1]. Zero is a straight line; positive values start slow and end fast,
    negative values start fast and end slow. The segment shape is

        y = (exp (k c t) - 1) / (exp (k c) - 1)

    with k = curveSteepness, which is monotone, hits both endpoints exactly and has a
    closed form inverse, so an editor can grab any point along a segment. Times are in
    whatever unit the player wants: seconds for a CurveEnvelope, one normalized cycle
    for an LFO.

    The curve optionally carries a sustain point and a loop between two points, which
    CurveEnvelope honors while a note is held. Everything is stored inline, so a curve
    is trivially copyable and copying it never allocates, which makes it safe to hand
    over to the audio thread by value.

    @tparam FloatType  float or double.

    @see CurveEnvelope, LFO
*/
template <typename FloatType = float>
class BreakpointCurve
{
public:
    //==============================================================================
    /** The number of points a curve can hold. */
    static constexpr int maxPoints = 32;

    /** The k of the segment shape: a full curve spans about 52 dB of slope. */
    static constexpr FloatType curveSteepness = FloatType (6);

    /** One breakpoint. */
    struct Point
    {
        FloatType time = FloatType (0);  /**< Position along the curve, never before the previous point. */
        FloatType level = FloatType (0); /**< Level in [0, 1]. */
        FloatType curve = FloatType (0); /**< Shape of the segment ending here, in [-1, 1]. Ignored for the first point. */

        bool operator== (const Point&) const = default;
    };

    //==============================================================================
    /** Creates an empty curve. */
    BreakpointCurve() = default;

    /** Removes every point and marker. */
    void clear() noexcept
    {
        *this = {};
    }

    /** Inserts a point after any point at the same time.

        Level and curve are clamped to their ranges. Sustain and loop markers keep
        pointing at the same points.

        @returns The index of the new point, or -1 when the curve is full.
    */
    int addPoint (FloatType time, FloatType level, FloatType curve = FloatType (0)) noexcept
    {
        if (numPoints >= maxPoints)
            return -1;

        int index = numPoints;

        while (index > 0 && points[static_cast<std::size_t> (index - 1)].time > time)
            --index;

        for (int i = numPoints; i > index; --i)
            points[static_cast<std::size_t> (i)] = points[static_cast<std::size_t> (i - 1)];

        points[static_cast<std::size_t> (index)] = sanitize ({ time, level, curve });
        ++numPoints;

        shiftMarkersAfterInsert (index);
        return index;
    }

    /** Removes a point. Markers on it are cleared, markers after it shift down. */
    void removePoint (int index) noexcept
    {
        if (! isPositiveAndBelow (index, numPoints))
            return;

        for (int i = index; i < numPoints - 1; ++i)
            points[static_cast<std::size_t> (i)] = points[static_cast<std::size_t> (i + 1)];

        --numPoints;
        points[static_cast<std::size_t> (numPoints)] = {};

        sustainIndex = shiftMarkerAfterRemove (sustainIndex, index);
        loopStart = shiftMarkerAfterRemove (loopStart, index);
        loopEnd = shiftMarkerAfterRemove (loopEnd, index);

        if (loopStart < 0 || loopEnd < 0)
            clearLoop();
    }

    /** Replaces a point, keeping the order of times.

        The time is clamped between the neighboring points, the level to [0, 1] and the
        curve to [-1, 1].
    */
    void setPoint (int index, Point point) noexcept
    {
        if (! isPositiveAndBelow (index, numPoints))
            return;

        const auto lower = index > 0 ? points[static_cast<std::size_t> (index - 1)].time : std::numeric_limits<FloatType>::lowest();
        const auto upper = index < numPoints - 1 ? points[static_cast<std::size_t> (index + 1)].time : std::numeric_limits<FloatType>::max();

        point.time = jlimit (lower, upper, point.time);
        points[static_cast<std::size_t> (index)] = sanitize (point);
    }

    /** Returns the number of points. */
    int getNumPoints() const noexcept { return numPoints; }

    /** Returns a point. The index must be in range. */
    const Point& getPoint (int index) const noexcept
    {
        jassert (isPositiveAndBelow (index, numPoints));
        return points[static_cast<std::size_t> (jlimit (0, maxPoints - 1, index))];
    }

    /** Returns the time of the last point, or zero for an empty curve. */
    FloatType getLength() const noexcept
    {
        return numPoints > 0 ? points[static_cast<std::size_t> (numPoints - 1)].time : FloatType (0);
    }

    //==============================================================================
    /** Selects the point a held note stops at. -1, or an index out of range, clears it. */
    void setSustainIndex (int index) noexcept
    {
        sustainIndex = isPositiveAndBelow (index, numPoints) ? index : -1;
    }

    /** Returns the sustain point, or -1. */
    int getSustainIndex() const noexcept { return sustainIndex; }

    /** Loops a held note from point end back to point start. Clears the loop unless start < end. */
    void setLoop (int start, int end) noexcept
    {
        if (start >= 0 && start < end && end < numPoints)
        {
            loopStart = start;
            loopEnd = end;
        }
        else
        {
            clearLoop();
        }
    }

    /** Removes the loop. */
    void clearLoop() noexcept
    {
        loopStart = -1;
        loopEnd = -1;
    }

    /** Returns true when a loop is set. */
    bool hasLoop() const noexcept { return loopStart >= 0; }

    /** Returns the loop start point, or -1. */
    int getLoopStart() const noexcept { return loopStart; }

    /** Returns the loop end point, or -1. */
    int getLoopEnd() const noexcept { return loopEnd; }

    //==============================================================================
    /** Returns the level at a time. Before the first point and after the last one the
        curve holds the nearest level; an empty curve is zero.
    */
    FloatType evaluate (FloatType time) const noexcept
    {
        if (numPoints == 0)
            return FloatType (0);

        const auto segment = getSegmentAt (time);

        if (segment < 0 || time <= points[0].time)
            return points[0].level;

        const auto& from = points[static_cast<std::size_t> (segment - 1)];
        const auto& to = points[static_cast<std::size_t> (segment)];

        if (time >= to.time)
            return to.level;

        return segmentValue (from.level, to.level, to.curve, (time - from.time) / (to.time - from.time));
    }

    /** Returns the index of the point ending the segment that contains a time.

        Times before the curve map to the first segment, times after it to the last.
        A point sharing its time with the next one belongs to the later segment.

        @returns An index in [1, getNumPoints() - 1], or -1 for fewer than two points.
    */
    int getSegmentAt (FloatType time) const noexcept
    {
        if (numPoints < 2)
            return -1;

        for (int i = 1; i < numPoints; ++i)
            if (time < points[static_cast<std::size_t> (i)].time)
                return i;

        return numPoints - 1;
    }

    //==============================================================================
    /** Returns the normalized segment shape at t in [0, 1] for a curve in [-1, 1]. */
    static FloatType shape (FloatType curve, FloatType t) noexcept
    {
        const auto kc = curveSteepness * curve;

        if (std::abs (kc) < linearThreshold)
            return t;

        return static_cast<FloatType> (std::expm1 (kc * t) / std::expm1 (kc));
    }

    /** Returns the t in [0, 1] at which shape (curve, t) equals a value in [0, 1]. */
    static FloatType inverseShape (FloatType curve, FloatType value) noexcept
    {
        const auto kc = curveSteepness * curve;

        if (std::abs (kc) < linearThreshold)
            return value;

        return static_cast<FloatType> (std::log1p (value * std::expm1 (kc)) / kc);
    }

    /** Returns the curve whose shape passes through a value at t = 0.5, clamped to [-1, 1].

        This is how an editor turns a dragged segment midpoint into a curve.
    */
    static FloatType curveForMidpoint (FloatType value) noexcept
    {
        constexpr auto epsilon = FloatType (1e-6);

        const auto y = jlimit (epsilon, FloatType (1) - epsilon, value);
        return jlimit (FloatType (-1), FloatType (1), FloatType (2) / curveSteepness * std::log (FloatType (1) / y - FloatType (1)));
    }

    /** Returns the level t of the way through a segment going from one level to another. */
    static FloatType segmentValue (FloatType from, FloatType to, FloatType curve, FloatType t) noexcept
    {
        return from + (to - from) * shape (curve, t);
    }

    //==============================================================================
    /** Compares points and markers. */
    bool operator== (const BreakpointCurve&) const = default;

private:
    //==============================================================================
    static constexpr FloatType linearThreshold = FloatType (1e-6);

    static Point sanitize (Point point) noexcept
    {
        point.level = jlimit (FloatType (0), FloatType (1), point.level);
        point.curve = jlimit (FloatType (-1), FloatType (1), point.curve);
        return point;
    }

    void shiftMarkersAfterInsert (int index) noexcept
    {
        for (auto* marker : { &sustainIndex, &loopStart, &loopEnd })
            if (*marker >= index)
                ++(*marker);
    }

    static int shiftMarkerAfterRemove (int marker, int removed) noexcept
    {
        if (marker == removed)
            return -1;

        return marker > removed ? marker - 1 : marker;
    }

    //==============================================================================
    std::array<Point, maxPoints> points {};
    int numPoints = 0;
    int sustainIndex = -1;
    int loopStart = -1;
    int loopEnd = -1;
};

} // namespace yup
