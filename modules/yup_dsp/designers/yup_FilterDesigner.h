/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
    Centralized filter coefficient designer for all filter types.

    This class provides static methods to design coefficients for various
    filter types, separating the coefficient calculation logic from the
    filter implementation classes. This allows for reusability and easier
    testing of coefficient generation algorithms.


    @see BiquadCoefficients, FilterBase
*/
template <typename CoeffType>
class FilterDesigner
{
public:
    //==============================================================================
    // RBJ (Audio EQ Cookbook) Filter Design
    //==============================================================================

    /**
        Designs RBJ lowpass filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designRbjLowpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designRbjImpl (FilterMode::lowpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
        Designs RBJ highpass filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designRbjHighpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designRbjImpl (FilterMode::highpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
        Designs RBJ bandpass filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designRbjBandpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designRbjImpl (FilterMode::bandpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
        Designs RBJ bandstop filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designRbjBandstop (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designRbjImpl (FilterMode::bandstop, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
        Designs RBJ peaking filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param gain       The gain in dB
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designRbjPeak (
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate) noexcept
    {
        return designRbjImpl (FilterMode::peak, frequency, q, gain, sampleRate);
    }

    /**
        Designs RBJ low shelf filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param gain       The gain in dB
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designRbjLowShelf (
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate) noexcept
    {
        return designRbjImpl (FilterMode::lowshelf, frequency, q, gain, sampleRate);
    }

    /**
        Designs RBJ high shelf filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param gain       The gain in dB
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designRbjHighShelf (
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate) noexcept
    {
        return designRbjImpl (FilterMode::highshelf, frequency, q, gain, sampleRate);
    }

    /**
        Designs RBJ allpass filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designRbjAllpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designRbjImpl (FilterMode::allpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

private:
    //==============================================================================
    /** RBJ implementation with type selection */
    static BiquadCoefficients<CoeffType> designRbjImpl (
        FilterMode filterMode,
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate) noexcept;
};

} // namespace yup
