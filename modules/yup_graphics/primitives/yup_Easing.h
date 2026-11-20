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
/** A cubic bezier easing curve, used to interpolate between keyframes.

    Follows the CSS cubic-bezier() convention: the curve is a CubicBezier running
    from (0,0) to (1,1) with the two control points (x1, y1) and (x2, y2).
    Evaluating at a progress x solves the curve for the parameter whose x is the
    given progress and returns the matching y. Evaluation is stateless, so a
    shared easing can be evaluated from several threads.
*/
class YUP_API Easing
{
public:
    //==============================================================================
    /** Constructs a linear easing (equivalent to cubic-bezier(0,0,1,1)). */
    constexpr Easing() noexcept = default;

    /** Constructs a cubic bezier easing from two control points.

        @param x1 First control point x (must be in [0,1]).
        @param y1 First control point y.
        @param x2 Second control point x (must be in [0,1]).
        @param y2 Second control point y.
    */
    Easing (float x1, float y1, float x2, float y2) noexcept;

    //==============================================================================
    /** Returns the eased output progress for normalized input t in [0,1]. */
    [[nodiscard]] float evaluate (float t) const noexcept;

    /** Returns true if this is the identity (linear) easing. */
    [[nodiscard]] bool isLinear() const noexcept;

    /** Returns true if this is a hold (step) easing with no interpolation. */
    [[nodiscard]] bool isHold() const noexcept { return holdFrame; }

    //==============================================================================
    /** Returns the curve from (0,0) to (1,1) whose inner points are the control points. */
    [[nodiscard]] const CubicBezier& getCurve() const noexcept { return curve; }

    /** Returns the first control point x. */
    [[nodiscard]] float getX1() const noexcept { return curve.p1().getX(); }

    /** Returns the first control point y. */
    [[nodiscard]] float getY1() const noexcept { return curve.p1().getY(); }

    /** Returns the second control point x. */
    [[nodiscard]] float getX2() const noexcept { return curve.p2().getX(); }

    /** Returns the second control point y. */
    [[nodiscard]] float getY2() const noexcept { return curve.p2().getY(); }

    /** Returns true if both easings hold, or have the same control points. */
    bool operator== (const Easing& other) const noexcept;

    /** Returns true if this is not equal to another easing. */
    bool operator!= (const Easing& other) const noexcept;

    //==============================================================================
    /** A linear easing (equivalent to cubic-bezier(0,0,1,1)). */
    [[nodiscard]] static Easing linear() noexcept;

    /** An ease-in easing (equivalent to cubic-bezier(0.42,0,1,1)). */
    [[nodiscard]] static Easing easeIn() noexcept;

    /** An ease-out easing (equivalent to cubic-bezier(0,0,0.58,1)). */
    [[nodiscard]] static Easing easeOut() noexcept;

    /** An ease-in-out easing (equivalent to cubic-bezier(0.42,0,0.58,1)). */
    [[nodiscard]] static Easing easeInOut() noexcept;

    /** A hold easing that keeps the start value until the next keyframe. */
    [[nodiscard]] static Easing hold() noexcept;

    /** Creates an easing from two keyframe tangent points (o/i in Lottie JSON). */
    [[nodiscard]] static Easing fromTangents (Point<float> outTangent, Point<float> inTangent) noexcept;

private:
    float solveCurveX (float x) const noexcept;

    CubicBezier curve { { 0.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f } };
    bool holdFrame = false;
};

} // namespace yup
