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
/** Real spherical harmonics up to the second band, nine coefficients, for diffuse light.

    The CPU reference of the shaders' spherical harmonics: radiance is projected on a
    latitude-longitude grid (with the parameterization of EnvironmentMap::getDirection()), then
    convolved with the clamped cosine into irradiance coefficients that evaluate as a plain
    polynomial of the normal (Ramamoorthi and Hanrahan, "An Efficient Representation for
    Irradiance Environment Maps", SIGGRAPH 2001).

    Below the float floor the coefficients are stored in rgba8: coefficient 0 in RGBM, every
    other one as its ratio to coefficient 0, which ratioBound keeps within [0, 1].

    @code
    const auto irradiance = SphericalHarmonics::toIrradiance (SphericalHarmonics::project (sky));
    const auto diffuse = SphericalHarmonics::evaluateIrradiance (irradiance, normal);
    @endcode

    @internal
*/
class YUP_API SphericalHarmonics
{
public:
    //==============================================================================
    /** Nine RGB coefficients, in the order 1, y, z, x, xy, yz, 3z^2 - 1, xz, x^2 - y^2. */
    using Coefficients = std::array<Vector3<float>, 9>;

    /** The upper bound of |coefficient i| / coefficient 0, for every i > 0, of the irradiance
        coefficients of any non-negative radiance (reached by a single direction).
    */
    static constexpr float ratioBound = 2.0f;

    //==============================================================================
    /** Returns the real SH basis, with its constants, along a unit direction. */
    static std::array<float, 9> evaluateBasis (const Vector3<float>& direction) noexcept;

    /** Projects a radiance function onto the basis, integrating over a latitude-longitude grid
        of width x width / 2 cells. CPU reference for the GPU projection pass.

        @param radiance  The linear RGB radiance along a unit direction, called once per cell center.
        @param width     The number of cells around the vertical axis, at least 2.
    */
    static Coefficients project (const std::function<Vector3<float> (const Vector3<float>&)>& radiance, int width = 128);

    /** Projects a latitude-longitude image onto the basis, like the other overload, taking the
        radiance of every cell from its texel instead of sampling a function at its center.

        @param pixels  Linear RGBA pixels, four floats per texel, rows from the zenith down, like
                       the image EnvironmentMap samples.
        @param width   The width of the image in texels.
        @param height  The height of the image in texels.
    */
    static Coefficients project (Span<const float> pixels, int width, int height);

    /** Convolves radiance coefficients with the clamped cosine and divides by pi, folding the
        basis constants in, so evaluateIrradiance() is a plain polynomial of the normal.
    */
    static Coefficients toIrradiance (const Coefficients& radiance) noexcept;

    /** Evaluates irradiance coefficients along a unit normal, clamped at zero. With
        @p numBands = 2 (or less) only the first four coefficients (L0 + L1) are used.
    */
    static Vector3<float> evaluateIrradiance (const Coefficients& irradiance, const Vector3<float>& normal, int numBands = 3) noexcept;

    //==============================================================================
    /** Encodes coefficient i > 0 relative to coefficient 0 into [0, 1], for rgba8 storage below
        the float floor. Coefficient 0 itself is stored in RGBM. A non-positive @p dc encodes 0.5.
    */
    static float encodeRatio (float coefficient, float dc) noexcept;

    /** Inverse of encodeRatio(). */
    static float decodeRatio (float encoded, float dc) noexcept;
};

} // namespace yup
