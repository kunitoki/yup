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
    Designs coefficients for nonlinear analog-model filters.

    These designs are intended for topology-preserving filters and ladder models
    where the coefficient set describes a filter topology rather than a plain
    transfer-function biquad.
*/
template <typename CoeffType>
class AnalogFilterDesigner
{
public:
    /**
        Designs coefficients for the shared atan saturator.

        @param drive  Normalized drive amount in the range 0..1
    */
    static AnalogSaturatorCoefficients<CoeffType> designSaturator (CoeffType drive) noexcept;

    /**
        Designs a two-pole topology-preserving state-variable filter.

        @param mode                  Output mode. Supported modes are low-pass,
                                     high-pass, band-pass, peak, and band-stop.
        @param frequency             Cutoff or center frequency in Hz
        @param sampleRate            Sample rate in Hz
        @param normalizedResonance   Normalized resonance amount in the range 0..1
    */
    static AnalogTwoPoleCoefficients<CoeffType> designTwoPole (
        FilterModeType mode,
        CoeffType frequency,
        double sampleRate,
        CoeffType normalizedResonance) noexcept;

    /**
        Designs a vowel/formant filter as three cascaded two-pole peak filters.

        @param vowel                 Normalized vowel position in the range 0..1
        @param sampleRate            Sample rate in Hz
        @param normalizedResonance   Normalized formant resonance in the range 0..1
    */
    static AnalogVowelCoefficients<CoeffType> designVowel (
        CoeffType vowel,
        double sampleRate,
        CoeffType normalizedResonance) noexcept;

    /**
        Designs coefficients for a Korg35-inspired filter.

        @param mode                  Supported modes are low-pass, band-pass, and high-pass
        @param frequency             Cutoff frequency in Hz
        @param sampleRate            Sample rate in Hz
        @param resonance             Normalized resonance amount in the range 0..1
        @param saturation            Normalized saturation amount in the range 0..1
    */
    static AnalogKorg35Coefficients<CoeffType> designKorg35 (
        FilterModeType mode,
        CoeffType frequency,
        double sampleRate,
        CoeffType resonance,
        CoeffType saturation) noexcept;

    /**
        Designs coefficients for a four-pole Moog ladder-style filter.

        @param mode                  Ladder output tap mix
        @param frequency             Cutoff frequency in Hz
        @param sampleRate            Sample rate in Hz
        @param resonance             Normalized resonance amount in the range 0..1
        @param saturation            Normalized saturation amount in the range 0..1
    */
    static AnalogMoogLadderCoefficients<CoeffType> designMoogLadder (
        AnalogMoogLadderMode mode,
        CoeffType frequency,
        double sampleRate,
        CoeffType resonance,
        CoeffType saturation) noexcept;

    /**
        Designs coefficients for a Roland diode-ladder-inspired low-pass filter.

        @param frequency             Cutoff frequency in Hz
        @param sampleRate            Sample rate in Hz
        @param resonance             Normalized resonance amount in the range 0..1
        @param saturation            Normalized saturation amount in the range 0..1
    */
    static AnalogRolandDiodeCoefficients<CoeffType> designRolandDiode (
        CoeffType frequency,
        double sampleRate,
        CoeffType resonance,
        CoeffType saturation) noexcept;

private:
    static CoeffType sanitizeFrequency (CoeffType frequency, double sampleRate) noexcept;
    static CoeffType sanitizeSampleRate (double sampleRate) noexcept;
    static CoeffType sanitizeNormalized (CoeffType value) noexcept;
};

} // namespace yup
