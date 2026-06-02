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
/** Vector operations for interleaved complex<float> buffers.

    Complex values are stored as `[real, imaginary, real, imaginary, ...]`.
*/
class YUP_API ComplexVectorOperations
{
public:
    /** Adds the complex product of `a` and `b` to `y`.

        @param a             Input complex buffer.
        @param b             Input complex buffer.
        @param y             Destination complex buffer, accumulated in-place.
        @param complexPairs  Number of complex pairs, not number of floats.
    */
    static void YUP_CALLTYPE multiplyAccumulate (const float* a, const float* b, float* y, int complexPairs) noexcept;

    /** Multiplies two complex buffers and writes the result to `dest`.

        @param dest          Destination complex buffer.
        @param a             Input complex buffer.
        @param b             Input complex buffer.
        @param complexPairs  Number of complex pairs, not number of floats.
    */
    static void YUP_CALLTYPE multiply (float* dest, const float* a, const float* b, int complexPairs) noexcept;

    /** Computes the power spectrum of an interleaved complex buffer.

        @param dest          Destination buffer receiving `real * real + imaginary * imaginary`.
        @param src           Input complex buffer.
        @param complexPairs  Number of complex pairs, not number of floats.
    */
    static void YUP_CALLTYPE powerSpectrum (float* dest, const float* src, int complexPairs) noexcept;
};

} // namespace yup
