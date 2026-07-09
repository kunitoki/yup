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
    Nonlinear traits for a hard clipper: f(x) = clamp(x, -1, 1).
*/
struct HardClipperTraits
{
    /** Applies hard clipping: returns x clamped to [-1, 1]. */
    template <typename T>
    static T f (T x) noexcept
    {
        return std::clamp (x, T (-1), T (1));
    }

    /** Adds u-space breakpoints at the clipping thresholds ξ = ±1.
        Called by AaIirAntialiaser to split the integration interval at the
        exact points where f changes from constant (−1 or +1) to linear (ξ).
        pts must have capacity for at least 2 additional entries beyond nPts. */
    template <typename T>
    static void fillBreakpoints (T x0, T delta, T* pts, int& nPts) noexcept
    {
        const T uMinus = (T (-1) - x0) / delta; // u where ξ = −1
        const T uPlus = (T (+1) - x0) / delta;  // u where ξ = +1
        if (uMinus > T (0) && uMinus < T (1))
            pts[nPts++] = uMinus;
        if (uPlus > T (0) && uPlus < T (1))
            pts[nPts++] = uPlus;
    }
};

//==============================================================================
/**
    Nonlinear traits for a hyperbolic-tangent soft clipper.

    Smooth everywhere - no fillBreakpoints needed.  A single affine segment
    over [x_n, x_{n+1}] gives an accurate approximation because tanh has no
    derivative discontinuities.
*/
struct TanhClipperTraits
{
    /** Applies tanh saturation: output ∈ (−1, 1) for any finite input. */
    template <typename T>
    static T f (T x) noexcept
    {
        return std::tanh (x);
    }
};

//==============================================================================
/** Convenience alias: AA-IIR antialiaser with hard-clipper nonlinearity. */
template <typename SampleType, typename CoeffType = double>
using HardClipper = AaIirAntialiaser<SampleType, HardClipperTraits, CoeffType>;

/** Hard-clipper AA-IIR antialiaser with float precision. */
using HardClipperFloat = HardClipper<float>;

/** Hard-clipper AA-IIR antialiaser with double precision. */
using HardClipperDouble = HardClipper<double>;

} // namespace yup
