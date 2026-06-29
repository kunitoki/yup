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
/** A single cubic bezier curve defined by four control points.

    Provides point-at, length, tangent angle, and subdivision operations
    matching rlottie's VBezier. Used by the animation renderer for trim
    operations, path morphing, and spatial position interpolation.

    Key methods:
    - pointAt(t): De Casteljau evaluation at parameter t in [0,1]
    - length() : Adaptive recursive arc length (converges when chord ≈ polyline)
    - tAtLength(len): Binary search for the parameter corresponding to arc length
    - splitAtLength(len, left, right): Split the curve at a given arc length
    - angleAt(t): Tangent angle at parameter t
    - parameterSplitLeft(t, left): Subdivide the curve at parameter t using
      de Casteljau's algorithm (left receives [0,t] portion, *this becomes [t,1])
*/
class YUP_API CubicBezier
{
public:
    //==============================================================================
    /** Constructs an empty bezier (all points at origin). */
    CubicBezier() = default;

    /** Constructs from four explicit control points. */
    CubicBezier (Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3);

    /** Factory: creates a bezier from four control points (equivalent to constructor). */
    [[nodiscard]] static CubicBezier fromPoints (Point<float> p0, Point<float> p1, Point<float> p2, Point<float> p3);

    //==============================================================================
    /** Control points (P0, P1, P2, P3). */
    [[nodiscard]] Point<float> p0() const noexcept { return { x1, y1 }; }

    [[nodiscard]] Point<float> p1() const noexcept { return { x2, y2 }; }

    [[nodiscard]] Point<float> p2() const noexcept { return { x3, y3 }; }

    [[nodiscard]] Point<float> p3() const noexcept { return { x4, y4 }; }

    //==============================================================================
    /** Evaluates the curve at parameter t in [0,1] using numerically stable
        de Casteljau's algorithm. */
    [[nodiscard]] Point<float> pointAt (float t) const noexcept;

    /** Returns the tangent angle (in radians) at parameter t. */
    [[nodiscard]] float angleAt (float t) const noexcept;

    /** Returns the adaptive approximate arc length. */
    [[nodiscard]] float length() const noexcept;

    /** Binary search for the parameter t corresponding to arc length @p len.
        @param len        The target arc length.
        @param totalLen   Pre-computed total length (from length()). */
    [[nodiscard]] float tAtLength (float len, float totalLen) const noexcept;

    /** Binary search for the parameter t corresponding to arc length @p len.
        Computes total length internally. */
    [[nodiscard]] float tAtLength (float len) const noexcept;

    //==============================================================================
    /** Splits the curve at arc length @p len.
        @param left   Receives the [0, len] portion.
        @param right  Receives the [len, total] portion. */
    void splitAtLength (float len, CubicBezier& left, CubicBezier& right) const noexcept;

    /** Splits the curve at parameter t=0.5 into two halves. */
    void split (CubicBezier& firstHalf, CubicBezier& secondHalf) const noexcept;

    /** Subdivides the curve using de Casteljau's algorithm.
        After the call, *left contains the [0, t] portion and *this is modified
        to contain the [t, 1] portion.

        @param t      The subdivision parameter in [0,1].
        @param left   Receives the left (first) portion of the curve. */
    void parameterSplitLeft (float t, CubicBezier& left) noexcept;

    /** Returns a new bezier representing the sub-curve on parameter interval [t0, t1]. */
    [[nodiscard]] CubicBezier onInterval (float t0, float t1) const noexcept;

    //==============================================================================
    /** Returns the derivative at t. */
    [[nodiscard]] Point<float> derivative (float t) const noexcept;

private:
    float x1 = 0.0f, y1 = 0.0f;
    float x2 = 0.0f, y2 = 0.0f;
    float x3 = 0.0f, y3 = 0.0f;
    float x4 = 0.0f, y4 = 0.0f;
};

} // namespace yup
