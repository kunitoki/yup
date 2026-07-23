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
/** A cubic bezier easing curve used to interpolate between animation keyframes.

    Stores two control points (x1, y1) and (x2, y2) in [0,1]^2 space matching
    the CSS cubic-bezier() convention. Evaluation uses Newton-Raphson iteration
    with an 11-sample precomputed lookup table, matching VInterpolator from rlottie.
*/
class YUP_API AnimationEasing
{
public:
    //==============================================================================
    /** Constructs a linear easing (equivalent to cubic-bezier(0,0,1,1)). */
    constexpr AnimationEasing() noexcept = default;

    /** Constructs a cubic bezier easing from two control points.

        @param x1 First control point x (must be in [0,1]).
        @param y1 First control point y.
        @param x2 Second control point x (must be in [0,1]).
        @param y2 Second control point y.
    */
    AnimationEasing (float x1, float y1, float x2, float y2) noexcept;

    AnimationEasing (const AnimationEasing&) noexcept = default;
    AnimationEasing (AnimationEasing&&) noexcept = default;
    AnimationEasing& operator= (const AnimationEasing&) noexcept = default;
    AnimationEasing& operator= (AnimationEasing&&) noexcept = default;

    //==============================================================================
    /** Returns the eased output progress for normalized input t in [0,1]. */
    [[nodiscard]] float evaluate (float t) const noexcept;

    /** Returns true if this is the identity (linear) easing. */
    [[nodiscard]] bool isLinear() const noexcept;

    /** Returns true if this is a hold (step) easing with no interpolation. */
    [[nodiscard]] bool isHold() const noexcept;

    //==============================================================================
    /** @{ Named easing presets. */
    [[nodiscard]] static AnimationEasing linear() noexcept;
    [[nodiscard]] static AnimationEasing easeIn() noexcept;
    [[nodiscard]] static AnimationEasing easeOut() noexcept;
    [[nodiscard]] static AnimationEasing easeInOut() noexcept;
    /** A hold easing that keeps the start value until the next keyframe. */
    [[nodiscard]] static AnimationEasing hold() noexcept;
    /** @} */

    /** Creates an easing from two Lottie keyframe tangent points (o/i in JSON). */
    [[nodiscard]] static AnimationEasing fromLottieTangents (Point<float> outTangent,
                                                             Point<float> inTangent) noexcept;

    //==============================================================================
    [[nodiscard]] float getX1() const noexcept { return cx1; }

    [[nodiscard]] float getY1() const noexcept { return cy1; }

    [[nodiscard]] float getX2() const noexcept { return cx2; }

    [[nodiscard]] float getY2() const noexcept { return cy2; }

    bool operator== (const AnimationEasing& other) const noexcept;
    bool operator!= (const AnimationEasing& other) const noexcept;

private:
    void buildTable() const noexcept;
    float getTForX (float x) const noexcept;
    static float calcBezier (float t, float a1, float a2) noexcept;
    static float getSlope (float t, float a1, float a2) noexcept;

    float cx1 = 0.0f, cy1 = 0.0f;
    float cx2 = 1.0f, cy2 = 1.0f;
    bool holdFrame = false;
    bool linearCached = true;

    static constexpr int kTableSize = 11;
    mutable float sampleTable[kTableSize] = {};
    mutable bool tableDirty = true;
};

} // namespace yup
