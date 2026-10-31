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
/** Memory layout of the complex buffers processed by ComplexVectorOperations.

    Both layouts store pairs of values: `[real, imaginary, real, imaginary, ...]`.
*/
enum class ComplexLayout
{
    /** Every pair is a complex value. */
    interleaved,

    /** The packed spectrum of a real FFT (see FFTProcessor::performRealFFTForwardPacked()).

        Pair k is bin k, except for pair 0 which holds the real DC and Nyquist bins as
        two real values: `[DC, Nyquist, re(1), im(1), ..., re(N/2 - 1), im(N/2 - 1)]`.
        A spectrum of an N point transform has N / 2 pairs.
    */
    packedReal
};

//==============================================================================
/** Vector operations on buffers of complex values, accelerated with SIMD instructions.

    Buffers hold pairs of values in one of the ComplexLayout formats, and every count is a
    number of pairs, not of values. Each operation exists for float and double samples.

    @code
    FFTProcessor<float> fft (1024);
    std::vector<float> a (1024), b (1024);

    fft.performRealFFTForwardPacked (a.data(), a.data());
    fft.performRealFFTForwardPacked (b.data(), b.data());
    ComplexVectorOperations::multiplyConjugate (a.data(), a.data(), b.data(), 512, ComplexLayout::packedReal);
    fft.performRealFFTInversePacked (a.data(), a.data()); // circular cross-correlation
    @endcode
*/
class YUP_API ComplexVectorOperations
{
public:
    //==============================================================================
    /** Writes the complex product of `a` and `b` to `dest`.

        With ComplexLayout::packedReal the DC and Nyquist values of pair 0 are multiplied as reals.

        @param dest          Destination buffer, may be `a` or `b`.
        @param a             Input buffer.
        @param b             Input buffer.
        @param complexPairs  Number of pairs, not number of values.
        @param layout        Layout of the three buffers.
    */
    static void YUP_CALLTYPE multiply (float* dest, const float* a, const float* b, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;
    static void YUP_CALLTYPE multiply (double* dest, const double* a, const double* b, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;

    /** Writes the product of `a` and the complex conjugate of `b` to `dest`, the spectrum of a cross-correlation.

        With ComplexLayout::packedReal the DC and Nyquist values of pair 0 are multiplied as reals.

        @param dest          Destination buffer, may be `a` or `b`.
        @param a             Input buffer.
        @param b             Input buffer, conjugated.
        @param complexPairs  Number of pairs, not number of values.
        @param layout        Layout of the three buffers.
    */
    static void YUP_CALLTYPE multiplyConjugate (float* dest, const float* a, const float* b, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;
    static void YUP_CALLTYPE multiplyConjugate (double* dest, const double* a, const double* b, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;

    /** Adds the complex product of `a` and `b` to `y`.

        With ComplexLayout::packedReal the DC and Nyquist values of pair 0 are multiplied as reals.

        @param a             Input buffer.
        @param b             Input buffer.
        @param y             Destination buffer, accumulated in place.
        @param complexPairs  Number of pairs, not number of values.
        @param layout        Layout of the three buffers.
    */
    static void YUP_CALLTYPE multiplyAccumulate (const float* a, const float* b, float* y, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;
    static void YUP_CALLTYPE multiplyAccumulate (const double* a, const double* b, double* y, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;

    //==============================================================================
    /** Writes the squared magnitude `real * real + imaginary * imaginary` of each pair to `dest`.

        With ComplexLayout::packedReal `dest[0]` is the squared DC value (Nyquist is not reported).

        @param dest          Destination buffer of `complexPairs` values, may be `src`.
        @param src           Input buffer.
        @param complexPairs  Number of pairs, not number of values.
        @param layout        Layout of `src`.
    */
    static void YUP_CALLTYPE powerSpectrum (float* dest, const float* src, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;
    static void YUP_CALLTYPE powerSpectrum (double* dest, const double* src, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;

    /** Writes the magnitude of each pair to `dest`.

        With ComplexLayout::packedReal `dest[0]` is the magnitude of DC (Nyquist is not reported).

        @param dest          Destination buffer of `complexPairs` values, may be `src`.
        @param src           Input buffer.
        @param complexPairs  Number of pairs, not number of values.
        @param layout        Layout of `src`.
    */
    static void YUP_CALLTYPE magnitude (float* dest, const float* src, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;
    static void YUP_CALLTYPE magnitude (double* dest, const double* src, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;

    /** Writes the phase of each pair, in radians, to `dest`.

        With ComplexLayout::packedReal `dest[0]` is the phase of DC: 0 or pi (Nyquist is not reported).

        @param dest          Destination buffer of `complexPairs` values, may be `src`.
        @param src           Input buffer.
        @param complexPairs  Number of pairs, not number of values.
        @param layout        Layout of `src`.
    */
    static void YUP_CALLTYPE phase (float* dest, const float* src, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;
    static void YUP_CALLTYPE phase (double* dest, const double* src, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;

    //==============================================================================
    /** Splits the pairs of `src` into separate real and imaginary buffers.

        With ComplexLayout::interleaved both destinations receive `complexPairs` values. With
        ComplexLayout::packedReal `real` receives `complexPairs + 1` values, from DC to Nyquist,
        and `imag` the `complexPairs - 1` imaginary parts of bins 1 to `complexPairs - 1`.

        @param real          Destination of the real parts.
        @param imag          Destination of the imaginary parts.
        @param src           Input buffer.
        @param complexPairs  Number of pairs, not number of values.
        @param layout        Layout of `src`.
    */
    static void YUP_CALLTYPE deinterleave (float* real, float* imag, const float* src, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;
    static void YUP_CALLTYPE deinterleave (double* real, double* imag, const double* src, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;

    /** Joins separate real and imaginary buffers into pairs, the inverse of deinterleave().

        @param dest          Destination buffer.
        @param real          Real parts, sized as for deinterleave().
        @param imag          Imaginary parts, sized as for deinterleave().
        @param complexPairs  Number of pairs, not number of values.
        @param layout        Layout of `dest`.
    */
    static void YUP_CALLTYPE interleave (float* dest, const float* real, const float* imag, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;
    static void YUP_CALLTYPE interleave (double* dest, const double* real, const double* imag, int complexPairs, ComplexLayout layout = ComplexLayout::interleaved) noexcept;
};

} // namespace yup
