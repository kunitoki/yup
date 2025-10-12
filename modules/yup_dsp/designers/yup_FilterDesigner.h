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

    This class provides static methods to design coefficients for various filter types, separating the coefficient
    calculation logic from the filter implementation classes. This allows for reusability and easier testing of
    coefficient generation algorithms.

    @see BiquadCoefficients, FilterBase
*/
template <typename CoeffType>
class FilterDesigner
{
public:
    //==============================================================================
    // First Order Filter Design
    //==============================================================================

    /** First order implementation with mode selection */
    static FirstOrderCoefficients<CoeffType> designFirstOrder (
        FilterModeType filterMode,
        CoeffType frequency,
        CoeffType gain,
        double sampleRate) noexcept;

    /**
        Configures the filter as a one-pole lowpass.

        @param frequency   The cutoff frequency in Hz
        @param sampleRate  The sample rate in Hz
    */
    static FirstOrderCoefficients<CoeffType> designFirstOrderLowpass (
        CoeffType frequency,
        double sampleRate) noexcept
    {
        return designFirstOrder (FilterMode::lowpass, frequency, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
       Configures the filter as a one-pole highpass.

       @param frequency   The cutoff frequency in Hz
       @param sampleRate  The sample rate in Hz
    */
    static FirstOrderCoefficients<CoeffType> designFirstOrderHighpass (
        CoeffType frequency,
        double sampleRate) noexcept
    {
        return designFirstOrder (FilterMode::highpass, frequency, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
       Configures the filter as a low-shelf.

       @param frequency   The shelf frequency in Hz
       @param gainDb      The shelf gain in decibels
       @param sampleRate  The sample rate in Hz
    */
    static FirstOrderCoefficients<CoeffType> designFirstOrderLowShelf (
        CoeffType frequency,
        CoeffType gainDb,
        double sampleRate) noexcept
    {
        return designFirstOrder (FilterMode::lowshelf, frequency, gainDb, sampleRate);
    }

    /**
       Configures the filter as a high-shelf.

       @param frequency   The shelf frequency in Hz
       @param gainDb      The shelf gain in decibels
       @param sampleRate  The sample rate in Hz
    */
    static FirstOrderCoefficients<CoeffType> designFirstOrderHighShelf (
        CoeffType frequency,
        CoeffType gainDb,
        double sampleRate) noexcept
    {
        return designFirstOrder (FilterMode::highshelf, frequency, gainDb, sampleRate);
    }

    /**
       Configures the filter as a first-order allpass.

       @param frequency   The characteristic frequency in Hz
       @param sampleRate  The sample rate in Hz
    */
    static FirstOrderCoefficients<CoeffType> designFirstOrderAllpass (
        CoeffType frequency,
        double sampleRate) noexcept
    {
        return designFirstOrder (FilterMode::allpass, frequency, static_cast<CoeffType> (0.0), sampleRate);
    }

    //==============================================================================
    // RBJ (Audio EQ Cookbook) Filter Design
    //==============================================================================

    /** RBJ implementation with type selection */
    static BiquadCoefficients<CoeffType> designRbj (
        FilterModeType filterMode,
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate) noexcept;

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
        return designRbj (FilterMode::lowpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
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
        return designRbj (FilterMode::highpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
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
        return designRbj (FilterMode::bandpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
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
        return designRbj (FilterMode::bandstop, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
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
        return designRbj (FilterMode::peak, frequency, q, gain, sampleRate);
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
        return designRbj (FilterMode::lowshelf, frequency, q, gain, sampleRate);
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
        return designRbj (FilterMode::highshelf, frequency, q, gain, sampleRate);
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
        return designRbj (FilterMode::allpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    //==============================================================================
    // Zoelzer Filter Design
    //==============================================================================

    /** Zoelzer implementation with mode selection */
    static BiquadCoefficients<CoeffType> designZoelzer (
        FilterModeType filterMode,
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate) noexcept;

    /**
        Designs Zoelzer lowpass filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designZoelzerLowpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designZoelzer (FilterMode::lowpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
        Designs Zoelzer highpass filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designZoelzerHighpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designZoelzer (FilterMode::highpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
        Designs Zoelzer bandpass filter coefficients (constant skirt gain, peak gain = Q).

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designZoelzerBandpassCsg (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designZoelzer (FilterMode::bandpassCsg, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
        Designs Zoelzer bandpass filter coefficients (constant peak gain = 0dB).

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designZoelzerBandpassCpg (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designZoelzer (FilterMode::bandpassCpg, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
        Designs Zoelzer notch filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designZoelzerNotch (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designZoelzer (FilterMode::bandstop, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    /**
        Designs Zoelzer peaking filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param gain       The gain in dB
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designZoelzerPeaking (
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate) noexcept
    {
        return designZoelzer (FilterMode::peak, frequency, q, gain, sampleRate);
    }

    /**
        Designs Zoelzer low shelf filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param gain       The gain in dB
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designZoelzerLowShelf (
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate) noexcept
    {
        return designZoelzer (FilterMode::lowshelf, frequency, q, gain, sampleRate);
    }

    /**
        Designs Zoelzer high shelf filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param gain       The gain in dB
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designZoelzerHighShelf (
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate) noexcept
    {
        return designZoelzer (FilterMode::highshelf, frequency, q, gain, sampleRate);
    }

    /**
        Designs Zoelzer allpass filter coefficients.

        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    static BiquadCoefficients<CoeffType> designZoelzerAllpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate) noexcept
    {
        return designZoelzer (FilterMode::allpass, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }

    //==============================================================================
    // Butterworth Filter Design
    //==============================================================================

    /** Butterworth implementation with mode selection */
    static int designButterworth (
        FilterModeType filterMode,
        int order,
        CoeffType frequency,
        CoeffType frequency2,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& coefficients) noexcept;

    /**
        Designs Butterworth lowpass filter coefficients.

        @param order          The filter order (2, 4, 8, 16, 32)
        @param frequency      The cutoff frequency in Hz
        @param sampleRate     The sample rate in Hz
        @param workspace      Pre-allocated workspace to avoid allocations
        @param coefficients   Output vector for biquad coefficients

        @returns              Number of biquad sections created
    */
    static int designButterworthLowpass (
        int order,
        CoeffType frequency,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& coefficients) noexcept
    {
        return designButterworth (FilterMode::lowpass, order, frequency, static_cast<CoeffType> (0.0), sampleRate, coefficients);
    }

    /**
        Designs Butterworth highpass filter coefficients.

        @param order          The filter order (2, 4, 8, 16, 32)
        @param frequency      The cutoff frequency in Hz
        @param sampleRate     The sample rate in Hz
        @param workspace      Pre-allocated workspace to avoid allocations
        @param coefficients   Output vector for biquad coefficients

        @returns              Number of biquad sections created
    */
    static int designButterworthHighpass (
        int order,
        CoeffType frequency,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& coefficients) noexcept
    {
        return designButterworth (FilterMode::highpass, order, frequency, static_cast<CoeffType> (0.0), sampleRate, coefficients);
    }

    /**
        Designs Butterworth bandpass filter coefficients.

        @param order          The filter order (2, 4, 8, 16, 32)
        @param lowFreq        The lower cutoff frequency in Hz
        @param highFreq       The upper cutoff frequency in Hz
        @param sampleRate     The sample rate in Hz
        @param workspace      Pre-allocated workspace to avoid allocations
        @param coefficients   Output vector for biquad coefficients

        @returns              Number of biquad sections created
    */
    static int designButterworthBandpass (
        int order,
        CoeffType lowFreq,
        CoeffType highFreq,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& coefficients) noexcept
    {
        return designButterworth (FilterMode::bandpass, order, lowFreq, highFreq, sampleRate, coefficients);
    }

    /**
        Designs Butterworth bandstop filter coefficients.

        @param order          The filter order (2, 4, 8, 16, 32)
        @param lowFreq        The lower cutoff frequency in Hz
        @param highFreq       The upper cutoff frequency in Hz
        @param sampleRate     The sample rate in Hz
        @param workspace      Pre-allocated workspace to avoid allocations
        @param coefficients   Output vector for biquad coefficients

        @returns              Number of biquad sections created
    */
    static int designButterworthBandstop (
        int order,
        CoeffType lowFreq,
        CoeffType highFreq,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& coefficients) noexcept
    {
        return designButterworth (FilterMode::bandstop, order, lowFreq, highFreq, sampleRate, coefficients);
    }

    /**
        Designs Butterworth allpass filter coefficients.

        @param order          The filter order (2, 4, 8, 16, 32)
        @param frequency      The characteristic frequency in Hz
        @param sampleRate     The sample rate in Hz
        @param workspace      Pre-allocated workspace to avoid allocations
        @param coefficients   Output vector for biquad coefficients

        @returns              Number of biquad sections created
    */
    static int designButterworthAllpass (
        int order,
        CoeffType frequency,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& coefficients) noexcept
    {
        return designButterworth (FilterMode::allpass, order, frequency, static_cast<CoeffType> (0.0), sampleRate, coefficients);
    }

    //==============================================================================
    // Linkwitz-Riley Filter Design
    //==============================================================================

    /**
        General Linkwitz-Riley crossover designer with order specification.

        @param order          The filter order (2, 4, 8, 16)
        @param crossoverFreq  The crossover frequency in Hz
        @param sampleRate     The sample rate in Hz
        @param lowCoeffs      Output vector for lowpass biquad coefficients
        @param highCoeffs     Output vector for highpass biquad coefficients

        @returns              Number of biquad sections created
    */
    static int designLinkwitzRiley (
        int order,
        CoeffType crossoverFreq,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& lowCoeffs,
        std::vector<BiquadCoefficients<CoeffType>>& highCoeffs) noexcept;

    /**
        Designs Linkwitz-Riley (LR2) 2nd order crossover coefficients.

        Linkwitz-Riley filters are created by cascading two identical Butterworth
        filters, resulting in complementary magnitude responses that sum to unity
        gain with phase alignment at the crossover frequency.

        @param crossoverFreq  The crossover frequency in Hz
        @param sampleRate     The sample rate in Hz
        @param lowCoeffs      Output coefficients for lowpass section
        @param highCoeffs     Output coefficients for highpass section

        @returns              True if coefficients were successfully calculated
    */
    static bool designLinkwitzRiley2 (
        CoeffType crossoverFreq,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& lowCoeffs,
        std::vector<BiquadCoefficients<CoeffType>>& highCoeffs) noexcept
    {
        return designLinkwitzRiley (2, crossoverFreq, sampleRate, lowCoeffs, highCoeffs);
    }

    /**
        Designs Linkwitz-Riley 4th order crossover coefficients.

        @param crossoverFreq  The crossover frequency in Hz
        @param sampleRate     The sample rate in Hz
        @param lowCoeffs      Output vector for lowpass biquad coefficients
        @param highCoeffs     Output vector for highpass biquad coefficients

        @returns              Number of biquad sections created (2 for LR4)
    */
    static int designLinkwitzRiley4 (
        CoeffType crossoverFreq,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& lowCoeffs,
        std::vector<BiquadCoefficients<CoeffType>>& highCoeffs) noexcept
    {
        return designLinkwitzRiley (4, crossoverFreq, sampleRate, lowCoeffs, highCoeffs);
    }

    /**
        Designs Linkwitz-Riley 8th order crossover coefficients.

        @param crossoverFreq  The crossover frequency in Hz
        @param sampleRate     The sample rate in Hz
        @param lowCoeffs      Output vector for lowpass biquad coefficients
        @param highCoeffs     Output vector for highpass biquad coefficients

        @returns              Number of biquad sections created (4 for LR8)
    */
    static int designLinkwitzRiley8 (
        CoeffType crossoverFreq,
        double sampleRate,
        std::vector<BiquadCoefficients<CoeffType>>& lowCoeffs,
        std::vector<BiquadCoefficients<CoeffType>>& highCoeffs) noexcept
    {
        return designLinkwitzRiley (8, crossoverFreq, sampleRate, lowCoeffs, highCoeffs);
    }

    //==============================================================================
    // FIR Filter Design
    //==============================================================================

    /**
        Designs FIR lowpass filter coefficients using windowed sinc method.

        @param numCoefficients  The number of filter coefficients (filter order + 1)
        @param cutoffFreq       The cutoff frequency in Hz
        @param sampleRate       The sample rate in Hz
        @param windowType       The window function to apply (default: Hanning)

        @returns               Vector of FIR coefficients suitable for DirectFIR
    */
    static void designFIRLowpass (
        std::vector<CoeffType>& coefficients,
        int numCoefficients,
        CoeffType cutoffFreq,
        double sampleRate,
        WindowType windowType = WindowType::hann,
        CoeffType windowParameter = CoeffType (8)) noexcept;

    /**
        Designs FIR highpass filter coefficients using windowed sinc method.

        @param numCoefficients  The number of filter coefficients (filter order + 1)
        @param cutoffFreq       The cutoff frequency in Hz
        @param sampleRate       The sample rate in Hz
        @param windowType       The window function to apply (default: Hanning)

        @returns               Vector of FIR coefficients suitable for DirectFIR
    */
    static void designFIRHighpass (
        std::vector<CoeffType>& coefficients,
        int numCoefficients,
        CoeffType cutoffFreq,
        double sampleRate,
        WindowType windowType = WindowType::hann,
        CoeffType windowParameter = CoeffType (8)) noexcept;

    /**
        Designs FIR bandpass filter coefficients using windowed sinc method.

        @param numCoefficients  The number of filter coefficients (filter order + 1)
        @param lowCutoffFreq    The lower cutoff frequency in Hz
        @param highCutoffFreq   The upper cutoff frequency in Hz
        @param sampleRate       The sample rate in Hz
        @param windowType       The window function to apply (default: Hanning)

        @returns               Vector of FIR coefficients suitable for DirectFIR
    */
    static void designFIRBandpass (
        std::vector<CoeffType>& coefficients,
        int numCoefficients,
        CoeffType lowCutoffFreq,
        CoeffType highCutoffFreq,
        double sampleRate,
        WindowType windowType = WindowType::hann,
        CoeffType windowParameter = CoeffType (8)) noexcept;

    /**
        Designs FIR bandstop filter coefficients using windowed sinc method.

        @param numCoefficients  The number of filter coefficients (filter order + 1)
        @param lowCutoffFreq    The lower cutoff frequency in Hz
        @param highCutoffFreq   The upper cutoff frequency in Hz
        @param sampleRate       The sample rate in Hz
        @param windowType       The window function to apply (default: Hanning)

        @returns               Vector of FIR coefficients suitable for DirectFIR
    */
    static void designFIRBandstop (
        std::vector<CoeffType>& coefficients,
        int numCoefficients,
        CoeffType lowCutoffFreq,
        CoeffType highCutoffFreq,
        double sampleRate,
        WindowType windowType = WindowType::hann,
        CoeffType windowParameter = CoeffType (8)) noexcept;
};

} // namespace yup
