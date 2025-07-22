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

#include <vector>
#include <complex>
#include <array>

namespace yup
{

//==============================================================================
/**
    Centralized filter coefficient designer for all filter types.

    This class provides static methods to design coefficients for various
    filter types, separating the coefficient calculation logic from the
    filter implementation classes. This allows for reusability and easier
    testing of coefficient generation algorithms.

    Features:
    - Analog prototype design (Butterworth, Chebyshev, Bessel, Elliptic)
    - Digital filter design (FIR windowing, IIR bilinear transform)
    - Classical filter responses (lowpass, highpass, bandpass, bandstop)
    - Virtual analog filter coefficients (TPT-based designs)
    - Specialized filter types (crossovers, parametric EQs)

    @see BiquadCoefficients, FirstOrderCoefficients, FilterBase
*/
template <typename CoeffType>
class FilterDesigner
{
public:
    //==============================================================================
    // FIR Filter Design
    //==============================================================================

    /**
        Designs FIR lowpass filter coefficients using windowing method.

        @param coeffs        Pre-allocated vector to store coefficients (size determines filter length)
        @param cutoff        The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param parameter     Window parameter (Kaiser beta, Gaussian sigma, etc.)
    */
    static void designFirLowpass (
        std::vector<CoeffType>& coeffs,
        CoeffType cutoff,
        double sampleRate,
        WindowType windowType = WindowType::kaiser,
        CoeffType parameter = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        designFIRLowpassImpl (coeffs, cutoff, sampleRate);
        WindowFunctions<CoeffType>::applyWindow (windowType, coeffs, parameter);
    }

    /**
        Designs FIR highpass filter coefficients using windowing method.

        @param coeffs        Pre-allocated vector to store coefficients (size determines filter length)
        @param cutoff        The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param parameter     Window parameter (Kaiser beta, Gaussian sigma, etc.)
    */
    static void designFirHighpass (
        std::vector<CoeffType>& coeffs,
        CoeffType cutoff,
        double sampleRate,
        WindowType windowType = WindowType::kaiser,
        CoeffType parameter = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        designFIRHighpassImpl (coeffs, cutoff, sampleRate);
        WindowFunctions<CoeffType>::applyWindow (windowType, coeffs, parameter);
    }

    /**
        Designs FIR bandpass filter coefficients using windowing method.

        @param coeffs        Pre-allocated vector to store coefficients (size determines filter length)
        @param lowCutoff     The low cutoff frequency in Hz
        @param highCutoff    The high cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param parameter     Window parameter (Kaiser beta, Gaussian sigma, etc.)
    */
    static void designFirBandpass (
        std::vector<CoeffType>& coeffs,
        CoeffType lowCutoff,
        CoeffType highCutoff,
        double sampleRate,
        WindowType windowType = WindowType::kaiser,
        CoeffType parameter = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        designFIRBandpassImpl (coeffs, lowCutoff, highCutoff, sampleRate);
        WindowFunctions<CoeffType>::applyWindow (windowType, coeffs, parameter);
    }

    /**
        Designs FIR bandstop filter coefficients using windowing method.

        @param coeffs        Pre-allocated vector to store coefficients (size determines filter length)
        @param lowCutoff     The low cutoff frequency in Hz
        @param highCutoff    The high cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param parameter     Window parameter (Kaiser beta, Gaussian sigma, etc.)
    */
    static void designFirBandstop (
        std::vector<CoeffType>& coeffs,
        CoeffType lowCutoff,
        CoeffType highCutoff,
        double sampleRate,
        WindowType windowType = WindowType::kaiser,
        CoeffType parameter = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        designFIRBandstopImpl (coeffs, lowCutoff, highCutoff, sampleRate);
        WindowFunctions<CoeffType>::applyWindow (windowType, coeffs, parameter);
    }

    //==============================================================================
    // Butterworth Filter Design
    //==============================================================================

    /**
        Designs Butterworth lowpass filter coefficients using vector reference.

        @param coeffs        Vector reference to receive coefficients
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
    */
    static void designButterworthLowpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        designButterworthImpl (coeffs, false, order, frequency, sampleRate);
    }

    /**
        Designs Butterworth highpass filter coefficients using vector reference.

        @param coeffs        Vector reference to receive coefficients
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
    */
    static void designButterworthHighpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        designButterworthImpl (coeffs, true, order, frequency, sampleRate);
    }

    /**
        Designs Butterworth allpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param sampleRate    The sample rate in Hz
    */
    static void designButterworthAllpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        double sampleRate
    ) noexcept
    {
        designButterworthAllpassImpl (coeffs, order, sampleRate);
    }

#if 0
    /**
        Designs Butterworth bandpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param centerFreq    The center frequency in Hz
        @param bandwidth     The bandwidth in Hz
        @param sampleRate    The sample rate in Hz
    */
    static void designButterworthBandpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType centerFreq,
        CoeffType bandwidth,
        double sampleRate
    ) noexcept
    {
        designButterworthBandpassImpl (coeffs, order, centerFreq, bandwidth, sampleRate);
    }

    /**
        Designs Butterworth bandstop filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param centerFreq    The center frequency in Hz
        @param bandwidth     The bandwidth in Hz
        @param sampleRate    The sample rate in Hz
    */
    static void designButterworthBandstop (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType centerFreq,
        CoeffType bandwidth,
        double sampleRate
    ) noexcept
    {
        designButterworthBandstopImpl (coeffs, order, centerFreq, bandwidth, sampleRate);
    }
#endif

    //==============================================================================
    // Chebyshev Filter Design
    //==============================================================================

    /**
        Designs Chebyshev Type I lowpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param ripple        Passband ripple in dB
    */
    static void designChebyshev1Lowpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5)
    ) noexcept
    {
        designChebyshev1Impl (coeffs, false, order, frequency, sampleRate, ripple);
    }

    /**
        Designs Chebyshev Type I highpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param ripple        Passband ripple in dB
    */
    static void designChebyshev1Highpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5)
    ) noexcept
    {
        designChebyshev1Impl (coeffs, true, order, frequency, sampleRate, ripple);
    }

    /**
        Designs Chebyshev Type II lowpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param stopbandAtten Stopband attenuation in dB
    */
    static void designChebyshev2Lowpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        designChebyshev2Impl (coeffs, false, order, frequency, sampleRate, stopbandAtten);
    }

    /**
        Designs Chebyshev Type II highpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param stopbandAtten Stopband attenuation in dB
    */
    static void designChebyshev2Highpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        designChebyshev2Impl (coeffs, true, order, frequency, sampleRate, stopbandAtten);
    }

    //==============================================================================
    // Bessel Filter Design
    //==============================================================================

    /**
        Designs Bessel lowpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
    */
    static void designBesselLowpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        designBesselImpl (coeffs, false, order, frequency, sampleRate);
    }

    /**
        Designs Bessel highpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
    */
    static void designBesselHighpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        designBesselImpl (coeffs, true, order, frequency, sampleRate);
    }

    //==============================================================================
    // Elliptic Filter Design
    //==============================================================================

    /**
        Designs Elliptic lowpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param ripple        Passband ripple in dB
        @param stopbandAtten Stopband attenuation in dB
    */
    static void designEllipticLowpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5),
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        designEllipticImpl (coeffs, false, order, frequency, sampleRate, ripple, stopbandAtten);
    }

    /**
        Designs Elliptic highpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param ripple        Passband ripple in dB
        @param stopbandAtten Stopband attenuation in dB
    */
    static void designEllipticHighpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5),
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        designEllipticImpl (coeffs, true, order, frequency, sampleRate, ripple, stopbandAtten);
    }

    /**
        Designs Elliptic allpass filter coefficients into pre-allocated vector.

        @param coeffs            Output vector for coefficients (resized if needed)
        @param order             The filter order
        @param sampleRate        The sample rate in Hz
        @param ripple            Passband ripple in dB
        @param stopbandAtten     Stopband attenuation in dB
    */
    static void designEllipticAllpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5),
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        designEllipticAllpassImpl (coeffs, order, sampleRate, ripple, stopbandAtten);
    }

    //==============================================================================
    // Legendre Filter Design
    //==============================================================================

    /**
        Designs Legendre lowpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
    */
    static void designLegendreLowpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        designLegendreImpl (coeffs, false, order, frequency, sampleRate);
    }

    /**
        Designs Legendre highpass filter coefficients into pre-allocated vector.

        @param coeffs        Output vector for coefficients (resized if needed)
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
    */
    static void designLegendreHighpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        designLegendreImpl (coeffs, true, order, frequency, sampleRate);
    }

#if 0
    /**
        Designs Legendre bandpass filter coefficients.

        @param order         The filter order
        @param centerFreq    The center frequency in Hz
        @param bandwidth     The bandwidth in octaves
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    static void designLegendreBandpass (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType centerFreq,
        CoeffType bandwidth,
        double sampleRate
    ) noexcept
    {
        designLegendreBandpassImpl (coeffs, order, centerFreq, bandwidth, sampleRate);
    }

    /**
        Designs Legendre bandstop filter coefficients.

        @param order         The filter order
        @param centerFreq    The center frequency in Hz
        @param bandwidth     The bandwidth in octaves
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    static void designLegendreBandstop (
        std::vector<BiquadCoefficients<CoeffType>>& coeffs,
        int order,
        CoeffType centerFreq,
        CoeffType bandwidth,
        double sampleRate
    ) noexcept
    {
        designLegendreBandstopImpl (coeffs, order, centerFreq, bandwidth, sampleRate);
    }
#endif

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
        double sampleRate
    ) noexcept
    {
        return designRbjImpl (0, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
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
        double sampleRate
    ) noexcept
    {
        return designRbjImpl (1, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
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
        double sampleRate
    ) noexcept
    {
        return designRbjImpl (2, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
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
        double sampleRate
    ) noexcept
    {
        return designRbjImpl (3, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
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
        double sampleRate
    ) noexcept
    {
        return designRbjImpl (4, frequency, q, gain, sampleRate);
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
        double sampleRate
    ) noexcept;

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
        double sampleRate
    ) noexcept
    {
        return designRbjImpl (5, frequency, q, gain, sampleRate);
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
        double sampleRate
    ) noexcept
    {
        return designRbjImpl (6, frequency, q, gain, sampleRate);
    }

    //==============================================================================
    // TPT (Topology Preserving Transform) Virtual Analog Filter Design
    //==============================================================================

    /**
        Designs TPT lowpass filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param sampleRate The sample rate in Hz
        @returns         Coefficients for TPT filter implementation [g, G]
    */
    static std::array<CoeffType, 2> designTptLowpass (
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto g = std::tan (omega / static_cast<CoeffType> (2.0));
        return { g, static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + g) };
    }

    /**
        Designs TPT highpass filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param sampleRate The sample rate in Hz
        @returns         Coefficients for TPT filter implementation [g, G]
    */
    static std::array<CoeffType, 2> designTptHighpass (
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto g = std::tan (omega / static_cast<CoeffType> (2.0));
        return { g, static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + g) };
    }

    /**
        Designs TPT state variable filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param resonance  The resonance amount (0-1)
        @param sampleRate The sample rate in Hz
        @returns         Coefficients for TPT SVF implementation [g, k, a1, a2, a3]
    */
    static std::array<CoeffType, 3> designTptSvf (
        CoeffType frequency,
        CoeffType resonance,
        double sampleRate
    ) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto g = std::tan (omega / static_cast<CoeffType> (2.0));
        const auto k = static_cast<CoeffType> (2.0) - static_cast<CoeffType> (2.0) * resonance;
        const auto G = g / (static_cast<CoeffType> (1.0) + g * (g + k));
        return { g, k, G };
    }

    /**
        Designs Moog Ladder filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param resonance  The resonance amount (0-1)
        @param sampleRate The sample rate in Hz
        @returns         Coefficients for Moog Ladder implementation [g, k, outputGain]
    */
    static std::array<CoeffType, 3> designMoogLadder (
        CoeffType frequency,
        CoeffType resonance,
        double sampleRate
    ) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto g = std::tan (omega / static_cast<CoeffType> (2.0));
        const auto k = static_cast<CoeffType> (4.0) * resonance;
        const auto outputGain = static_cast<CoeffType> (1.0) + resonance * static_cast<CoeffType> (0.5);
        return { g, k, outputGain };
    }

    /**
        Designs Korg MS-20 filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param resonance  The resonance amount (0-1)
        @param sampleRate The sample rate in Hz
        @returns         Coefficients for Korg MS-20 implementation [g, k, nonLinearGain, saturationAmount]
    */
    static std::array<CoeffType, 4> designKorgMs20 (
        CoeffType frequency,
        CoeffType resonance,
        double sampleRate
    ) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto g = std::tan (omega / static_cast<CoeffType> (2.0));

        // MS-20 uses a different resonance characteristic
        const auto k = resonance * static_cast<CoeffType> (3.5) + static_cast<CoeffType> (0.5);

        // Non-linear gain compensation for MS-20 character
        const auto nonLinearGain = static_cast<CoeffType> (1.0) + resonance * static_cast<CoeffType> (0.7);

        // Saturation amount increases with resonance
        const auto saturationAmount = resonance * static_cast<CoeffType> (0.8);

        return { g, k, nonLinearGain, saturationAmount };
    }

    /**
        Designs Roland TB-303 diode ladder filter coefficients.

        @param frequency  The cutoff frequency in Hz
        @param resonance  The resonance amount (0-1)
        @param sampleRate The sample rate in Hz
        @returns         Coefficients for TB-303 implementation [g1, g2, g3, g4, feedbackGain, inputGain, outputGain]
    */
    static std::array<CoeffType, 7> designTb303 (
        CoeffType frequency,
        CoeffType resonance,
        double sampleRate
    ) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto g = std::tan (omega / static_cast<CoeffType> (2.0));

        // TB-303 uses asymmetric stage gains to model diode ladder behavior
        const auto g1 = g * static_cast<CoeffType> (1.0);    // First stage (strongest)
        const auto g2 = g * static_cast<CoeffType> (0.9);    // Second stage
        const auto g3 = g * static_cast<CoeffType> (0.8);    // Third stage
        const auto g4 = g * static_cast<CoeffType> (0.7);    // Fourth stage (weakest)

        // TB-303 feedback is more aggressive than Moog
        const auto feedbackGain = resonance * static_cast<CoeffType> (4.8) + static_cast<CoeffType> (0.2);

        // Input gain varies with resonance to maintain consistent level
        const auto inputGain = static_cast<CoeffType> (1.0) + resonance * static_cast<CoeffType> (0.3);

        // Output gain compensation for TB-303 character
        const auto outputGain = static_cast<CoeffType> (1.2) + resonance * static_cast<CoeffType> (0.8);

        return { g1, g2, g3, g4, feedbackGain, inputGain, outputGain };
    }

private:
    //==============================================================================
    /** Design Butterworth filter coefficients into pre-allocated vector */
    static void designButterworthImpl (
        std::vector<BiquadCoefficients<CoeffType>>& sections,
        bool isHighpass, int order, CoeffType frequency, double sampleRate) noexcept;

    /** Designs Chebyshev Type I filter coefficients */
    static void designChebyshev1Impl (
        std::vector<BiquadCoefficients<CoeffType>>& sections,
        bool isHighpass, int order, CoeffType frequency, double sampleRate, CoeffType ripple) noexcept;

    /** Designs Chebyshev Type II filter coefficients */
    static void designChebyshev2Impl (
        std::vector<BiquadCoefficients<CoeffType>>& sections,
        bool isHighpass, int order, CoeffType frequency, double sampleRate, CoeffType stopbandAtten) noexcept;

    /** Designs Bessel filter coefficients */
    static void designBesselImpl (
        std::vector<BiquadCoefficients<CoeffType>>& sections,
        bool isHighpass, int order, CoeffType frequency, double sampleRate) noexcept;

    /** Designs Elliptic filter coefficients */
    static void designEllipticImpl (
        std::vector<BiquadCoefficients<CoeffType>>& sections,
        bool isHighpass, int order, CoeffType frequency, double sampleRate, CoeffType ripple, CoeffType stopbandAtten) noexcept;

    /** Designs Elliptic allpass filter coefficients */
    static void designEllipticAllpassImpl (
        std::vector<BiquadCoefficients<CoeffType>>& sections,
        int order, double sampleRate, CoeffType ripple, CoeffType stopbandAtten) noexcept;

    /** Designs Butterworth allpass filter coefficients */
    static void designButterworthAllpassImpl (
        std::vector<BiquadCoefficients<CoeffType>>& sections,
        int order, double sampleRate) noexcept;

    /** Designs Legendre filter coefficients */
    static void designLegendreImpl (
        std::vector<BiquadCoefficients<CoeffType>>& sections,
        bool isHighpass, int order, CoeffType frequency, double sampleRate) noexcept;

#if 0
    /** Designs Legendre bandpass filter coefficients */
    static void designLegendreBandpassImpl (
        std::vector<BiquadCoefficients<CoeffType>>& sections,
        int order, CoeffType centerFreq, CoeffType bandwidth, double sampleRate) noexcept
    {
        // Calculate low and high cutoff frequencies
        const auto q = centerFreq / (bandwidth * static_cast<CoeffType> (0.693));  // Convert octaves to Q
        const auto lowFreq = centerFreq / std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        const auto highFreq = centerFreq * std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));

        // Design cascaded highpass and lowpass sections
        auto highpassSections = designLegendreImpl (true, order, lowFreq, sampleRate);
        auto lowpassSections = designLegendreImpl (false, order, highFreq, sampleRate);

        // Combine sections
        std::vector<BiquadCoefficients<CoeffType>> sections;
        sections.reserve (highpassSections.size() + lowpassSections.size());
        sections.insert (sections.end(), highpassSections.begin(), highpassSections.end());
        sections.insert (sections.end(), lowpassSections.begin(), lowpassSections.end());

        return sections;
    }

    /** Designs Legendre bandstop filter coefficients */
    static std::vector<BiquadCoefficients<CoeffType>> designLegendreBandstopImpl (
        int order, CoeffType centerFreq, CoeffType bandwidth, double sampleRate) noexcept
    {
        // For bandstop, we use parallel lowpass and highpass branches
        // This is a simplified implementation
        const auto q = centerFreq / (bandwidth * static_cast<CoeffType> (0.693));
        const auto lowFreq = centerFreq / std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        const auto highFreq = centerFreq * std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));

        // Use lowpass at lower frequency
        auto sections = designLegendreImpl (false, order, lowFreq, sampleRate);

        // Add highpass sections at higher frequency
        auto highpassSections = designLegendreImpl (true, order, highFreq, sampleRate);
        sections.insert (sections.end(), highpassSections.begin(), highpassSections.end());

        return sections;
    }

    /** Designs Butterworth bandpass filter coefficients */
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthBandpassImpl (
        int order, CoeffType centerFreq, CoeffType bandwidth, double sampleRate) noexcept
    {
        // Calculate low and high cutoff frequencies
        const auto q = centerFreq / bandwidth;
        const auto lowFreq = centerFreq / std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        const auto highFreq = centerFreq * std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));

        // Design cascaded lowpass and highpass sections
        auto lowpassSections = designButterworthImpl (false, order, highFreq, sampleRate);
        auto highpassSections = designButterworthImpl (true, order, lowFreq, sampleRate);

        // Combine sections
        std::vector<BiquadCoefficients<CoeffType>> sections;
        sections.reserve (lowpassSections.size() + highpassSections.size());
        sections.insert (sections.end(), lowpassSections.begin(), lowpassSections.end());
        sections.insert (sections.end(), highpassSections.begin(), highpassSections.end());

        return sections;
    }

    /** Designs Butterworth bandstop filter coefficients */
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthBandstopImpl (
        int order, CoeffType centerFreq, CoeffType bandwidth, double sampleRate) noexcept
    {
        // For bandstop, we use a parallel combination approach
        // This is a simplified implementation - real bandstop would need more sophisticated design
        const auto q = centerFreq / bandwidth;
        const auto lowFreq = centerFreq / std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        const auto highFreq = centerFreq * std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));

        // Create notch sections using RBJ bandstop design
        std::vector<BiquadCoefficients<CoeffType>> sections;
        const int numSections = (order + 1) / 2;
        sections.reserve (static_cast<size_t> (numSections));

        for (int i = 0; i < numSections; ++i)
        {
            // Use RBJ bandstop design for each section
            const auto sectionQ = q * static_cast<CoeffType> (numSections);
            auto coeffs = designRbjImpl (3, centerFreq, sectionQ, static_cast<CoeffType> (0.0), sampleRate);
            sections.emplace_back (coeffs);
        }

        return sections;
    }
#endif

    /** RBJ implementation with type selection */
    static BiquadCoefficients<CoeffType> designRbjImpl (
        int filterType,
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate
    ) noexcept;

    /** Designs FIR lowpass coefficients */
    static void designFIRLowpassImpl (std::vector<CoeffType>& coeffs, CoeffType cutoff, double sampleRate) noexcept;

    /** Designs FIR highpass coefficients */
    static void designFIRHighpassImpl (std::vector<CoeffType>& coeffs, CoeffType cutoff, double sampleRate) noexcept;

    /** Designs FIR bandpass coefficients */
    static void designFIRBandpassImpl (std::vector<CoeffType>& coeffs, CoeffType lowCutoff, CoeffType highCutoff, double sampleRate) noexcept;

    /** Designs FIR bandstop coefficients */
    static void designFIRBandstopImpl (std::vector<CoeffType>& coeffs, CoeffType lowCutoff, CoeffType highCutoff, double sampleRate) noexcept;

    //==============================================================================
    // Notch Filter Design Methods
    //==============================================================================

    /**
        Designs a notch filter using allpass-based algorithm.

        @param frequency     The notch frequency in Hz
        @param depth         The notch depth (0.0 to 1.0)
        @param sampleRate    The sample rate in Hz
        @returns            Biquad coefficients for the notch filter
    */
    static BiquadCoefficients<CoeffType> designNotchAllpass (CoeffType frequency, CoeffType depth, double sampleRate) noexcept
    {
        const auto normalizedFreq = frequency / sampleRate;
        const auto k2 = depth * static_cast<CoeffType> (0.95); // Limit to avoid instability
        const auto cosine = std::cos (MathConstants<CoeffType>::twoPi * normalizedFreq);
        const auto b_coeff = -cosine * (static_cast<CoeffType> (1.0) + k2);

        // Convert allpass structure to biquad form
        // Allpass: G(z) = (z^2 + b*z + k2) / (k2*z^2 + b*z + 1)
        // Notch: H(z) = 0.5 * (1 + G(z))

        const auto b0 = static_cast<CoeffType> (0.5) * (static_cast<CoeffType> (1.0) + k2);
        const auto b1 = static_cast<CoeffType> (0.5) * b_coeff;
        const auto b2 = static_cast<CoeffType> (0.5) * (static_cast<CoeffType> (1.0) + k2);
        const auto a1 = b_coeff;
        const auto a2 = k2;

        return BiquadCoefficients<CoeffType> (b0, b1, b2, a1, a2);
    }

    /**
        Designs a notch filter using traditional biquad algorithm.

        @param frequency     The notch frequency in Hz
        @param depth         The notch depth (0.0 to 1.0)
        @param sampleRate    The sample rate in Hz
        @returns            Biquad coefficients for the notch filter
    */
    static BiquadCoefficients<CoeffType> designNotchBiquad (CoeffType frequency, CoeffType depth, double sampleRate) noexcept
    {
        const auto normalizedFreq = frequency / sampleRate;
        const auto Y = depth * static_cast<CoeffType> (0.9); // Depth control
        const auto B = -std::cos (MathConstants<CoeffType>::twoPi * normalizedFreq); // Frequency control
        const auto gain = (static_cast<CoeffType> (1.0) + B) * static_cast<CoeffType> (0.5);

        // Biquad coefficients from spuce notch_iir design
        const auto b0 = gain;
        const auto b1 = gain * Y * (static_cast<CoeffType> (1.0) + B);
        const auto b2 = gain * B;
        const auto a1 = static_cast<CoeffType> (2.0) * Y;
        const auto a2 = static_cast<CoeffType> (1.0);

        return BiquadCoefficients<CoeffType> (b0, b1, b2, a1, a2);
    }

    /**
        Designs a cut/boost filter that can function as notch or peak.

        @param frequency     The center frequency in Hz
        @param depth         The depth parameter (0.0 to 1.0)
        @param boost         The boost amount (-1.0 to 1.0, negative=cut, positive=boost)
        @param sampleRate    The sample rate in Hz
        @returns            Biquad coefficients for the cut/boost filter
    */
    static BiquadCoefficients<CoeffType> designCutBoost (CoeffType frequency, CoeffType depth, CoeffType boost, double sampleRate) noexcept
    {
        const auto normalizedFreq = frequency / sampleRate;
        const auto k2 = depth * static_cast<CoeffType> (0.95);
        const auto cosine = std::cos (MathConstants<CoeffType>::twoPi * normalizedFreq);
        const auto b_coeff = -cosine * (static_cast<CoeffType> (1.0) + k2);

        // Cut/boost control
        const auto k0 = boost;
        const auto k = (static_cast<CoeffType> (1.0) - k0) / (static_cast<CoeffType> (1.0) + k0);
        const auto g = static_cast<CoeffType> (0.5) * (static_cast<CoeffType> (1.0) + k0);

        // Convert to biquad form: H(z) = g * (1 + k * G_allpass(z))
        const auto b0 = g * (static_cast<CoeffType> (1.0) + k * k2);
        const auto b1 = g * k * b_coeff;
        const auto b2 = g * (static_cast<CoeffType> (1.0) + k * k2);
        const auto a1 = b_coeff;
        const auto a2 = k2;

        return BiquadCoefficients<CoeffType> (b0, b1, b2, a1, a2);
    }

    /**
        Designs a notch filter using RBJ (Robert Bristow-Johnson) parametric formula.
        This creates a very narrow notch with adjustable Q factor.

        @param frequency     The notch frequency in Hz
        @param Q             The Q factor (higher Q = narrower notch)
        @param sampleRate    The sample rate in Hz
        @returns            Biquad coefficients for the notch filter
    */
    static BiquadCoefficients<CoeffType> designNotchParametric (CoeffType frequency, CoeffType Q, double sampleRate) noexcept
    {
        const auto omega = MathConstants<CoeffType>::twoPi * frequency / sampleRate;
        const auto sin_omega = std::sin (omega);
        const auto cos_omega = std::cos (omega);
        const auto alpha = sin_omega / (static_cast<CoeffType> (2.0) * Q);

        // RBJ Notch filter coefficients
        const auto b0 = static_cast<CoeffType> (1.0);
        const auto b1 = static_cast<CoeffType> (-2.0) * cos_omega;
        const auto b2 = static_cast<CoeffType> (1.0);
        const auto a0 = static_cast<CoeffType> (1.0) + alpha;
        const auto a1 = static_cast<CoeffType> (-2.0) * cos_omega;
        const auto a2 = static_cast<CoeffType> (1.0) - alpha;

        // Normalize by a0
        return BiquadCoefficients<CoeffType> (b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0);
    }

    /**
        Designs multiple cascaded notch filters for harmonic rejection.

        @param fundamentalFreq   The fundamental frequency in Hz
        @param numHarmonics      Number of harmonics to notch (including fundamental)
        @param depth             The notch depth (0.0 to 1.0)
        @param sampleRate        The sample rate in Hz
        @returns                Vector of biquad coefficients, one per harmonic
    */
    static std::vector<BiquadCoefficients<CoeffType>> designHarmonicNotches (
        CoeffType fundamentalFreq,
        int numHarmonics,
        CoeffType depth,
        double sampleRate
    ) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> coeffs;
        coeffs.reserve (static_cast<size_t> (numHarmonics));

        for (int i = 1; i <= numHarmonics; ++i)
        {
            const auto harmonicFreq = fundamentalFreq * static_cast<CoeffType> (i);
            if (harmonicFreq < sampleRate * static_cast<CoeffType> (0.45)) // Avoid Nyquist issues
            {
                coeffs.push_back (designNotchAllpass (harmonicFreq, depth, sampleRate));
            }
        }

        return coeffs;
    }

    //==============================================================================
    // Parametric Filter Design Methods
    //==============================================================================

    /**
        Designs a parametric bell/peak filter for EQ applications.

        @param frequency     The center frequency in Hz
        @param gainDb        The gain in dB (positive = boost, negative = cut)
        @param Q             The Q factor (higher Q = narrower band)
        @param sampleRate    The sample rate in Hz
        @returns            Biquad coefficients for the parametric filter
    */
    static BiquadCoefficients<CoeffType> designParametricBell (CoeffType frequency, CoeffType gainDb, CoeffType Q, double sampleRate) noexcept
    {
        const auto omega = MathConstants<CoeffType>::twoPi * frequency / sampleRate;
        const auto sin_omega = std::sin (omega);
        const auto cos_omega = std::cos (omega);
        const auto A = std::pow (static_cast<CoeffType> (10.0), gainDb / static_cast<CoeffType> (40.0));
        const auto alpha = sin_omega / (static_cast<CoeffType> (2.0) * Q);

        const auto b0_raw = static_cast<CoeffType> (1.0) + alpha * A;
        const auto b1_raw = static_cast<CoeffType> (-2.0) * cos_omega;
        const auto b2_raw = static_cast<CoeffType> (1.0) - alpha * A;
        const auto a0_raw = static_cast<CoeffType> (1.0) + alpha / A;
        const auto a1_raw = static_cast<CoeffType> (-2.0) * cos_omega;
        const auto a2_raw = static_cast<CoeffType> (1.0) - alpha / A;

        // Normalize by a0
        return BiquadCoefficients<CoeffType> (b0_raw / a0_raw, b1_raw / a0_raw, b2_raw / a0_raw, a1_raw / a0_raw, a2_raw / a0_raw);
    }

    /**
        Designs a low shelf filter for bass control.

        @param frequency     The shelf frequency in Hz
        @param gainDb        The gain in dB (positive = boost, negative = cut)
        @param slope         The shelf slope factor (0.1 to 2.0, 1.0 = standard)
        @param sampleRate    The sample rate in Hz
        @returns            Biquad coefficients for the low shelf filter
    */
    static BiquadCoefficients<CoeffType> designLowShelf (CoeffType frequency, CoeffType gainDb, CoeffType slope, double sampleRate) noexcept
    {
        const auto omega = MathConstants<CoeffType>::twoPi * frequency / sampleRate;
        const auto sin_omega = std::sin (omega);
        const auto cos_omega = std::cos (omega);
        const auto A = std::pow (static_cast<CoeffType> (10.0), gainDb / static_cast<CoeffType> (40.0));
        const auto S = slope;
        const auto beta = std::sqrt (A) / S;

        const auto b0_raw = A * ((A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cos_omega + beta * sin_omega);
        const auto b1_raw = static_cast<CoeffType> (2.0) * A * ((A - static_cast<CoeffType> (1.0)) - (A + static_cast<CoeffType> (1.0)) * cos_omega);
        const auto b2_raw = A * ((A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cos_omega - beta * sin_omega);
        const auto a0_raw = (A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cos_omega + beta * sin_omega;
        const auto a1_raw = static_cast<CoeffType> (-2.0) * ((A - static_cast<CoeffType> (1.0)) + (A + static_cast<CoeffType> (1.0)) * cos_omega);
        const auto a2_raw = (A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cos_omega - beta * sin_omega;

        // Normalize by a0
        return BiquadCoefficients<CoeffType> (b0_raw / a0_raw, b1_raw / a0_raw, b2_raw / a0_raw, a1_raw / a0_raw, a2_raw / a0_raw);
    }

    /**
        Designs a high shelf filter for treble control.

        @param frequency     The shelf frequency in Hz
        @param gainDb        The gain in dB (positive = boost, negative = cut)
        @param slope         The shelf slope factor (0.1 to 2.0, 1.0 = standard)
        @param sampleRate    The sample rate in Hz
        @returns            Biquad coefficients for the high shelf filter
    */
    static BiquadCoefficients<CoeffType> designHighShelf (CoeffType frequency, CoeffType gainDb, CoeffType slope, double sampleRate) noexcept
    {
        const auto omega = MathConstants<CoeffType>::twoPi * frequency / sampleRate;
        const auto sin_omega = std::sin (omega);
        const auto cos_omega = std::cos (omega);
        const auto A = std::pow (static_cast<CoeffType> (10.0), gainDb / static_cast<CoeffType> (40.0));
        const auto S = slope;
        const auto beta = std::sqrt (A) / S;

        const auto b0_raw = A * ((A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cos_omega + beta * sin_omega);
        const auto b1_raw = static_cast<CoeffType> (-2.0) * A * ((A - static_cast<CoeffType> (1.0)) + (A + static_cast<CoeffType> (1.0)) * cos_omega);
        const auto b2_raw = A * ((A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cos_omega - beta * sin_omega);
        const auto a0_raw = (A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cos_omega + beta * sin_omega;
        const auto a1_raw = static_cast<CoeffType> (2.0) * ((A - static_cast<CoeffType> (1.0)) - (A + static_cast<CoeffType> (1.0)) * cos_omega);
        const auto a2_raw = (A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cos_omega - beta * sin_omega;

        // Normalize by a0
        return BiquadCoefficients<CoeffType> (b0_raw / a0_raw, b1_raw / a0_raw, b2_raw / a0_raw, a1_raw / a0_raw, a2_raw / a0_raw);
    }

    /**
        Designs a tilt filter that provides opposite responses at low and high frequencies.

        @param frequency     The center frequency in Hz
        @param gainDb        The gain in dB (positive tilts up high frequencies, negative tilts up low)
        @param sampleRate    The sample rate in Hz
        @returns            Biquad coefficients for the tilt filter
    */
    static BiquadCoefficients<CoeffType> designTiltFilter (CoeffType frequency, CoeffType gainDb, double sampleRate) noexcept
    {
        // Tilt filter combines low and high shelf responses
        const auto omega = MathConstants<CoeffType>::twoPi * frequency / sampleRate;
        const auto cos_omega = std::cos (omega);
        const auto sin_omega = std::sin (omega);
        const auto A = std::pow (static_cast<CoeffType> (10.0), gainDb / static_cast<CoeffType> (40.0));
        const auto sqrt_A = std::sqrt (A);

        // Tilt filter coefficients based on shelving filter theory
        const auto beta = sqrt_A * sin_omega;

        const auto b0_raw = sqrt_A * ((sqrt_A + static_cast<CoeffType> (1.0)) + (sqrt_A - static_cast<CoeffType> (1.0)) * cos_omega + beta);
        const auto b1_raw = static_cast<CoeffType> (-2.0) * sqrt_A * ((sqrt_A - static_cast<CoeffType> (1.0)) + (sqrt_A + static_cast<CoeffType> (1.0)) * cos_omega);
        const auto b2_raw = sqrt_A * ((sqrt_A + static_cast<CoeffType> (1.0)) + (sqrt_A - static_cast<CoeffType> (1.0)) * cos_omega - beta);
        const auto a0_raw = (sqrt_A + static_cast<CoeffType> (1.0)) - (sqrt_A - static_cast<CoeffType> (1.0)) * cos_omega + beta / sqrt_A;
        const auto a1_raw = static_cast<CoeffType> (2.0) * ((sqrt_A - static_cast<CoeffType> (1.0)) - (sqrt_A + static_cast<CoeffType> (1.0)) * cos_omega);
        const auto a2_raw = (sqrt_A + static_cast<CoeffType> (1.0)) - (sqrt_A - static_cast<CoeffType> (1.0)) * cos_omega - beta / sqrt_A;

        // Normalize by a0
        return BiquadCoefficients<CoeffType> (b0_raw / a0_raw, b1_raw / a0_raw, b2_raw / a0_raw, a1_raw / a0_raw, a2_raw / a0_raw);
    }

    /**
        Designs a multi-band parametric equalizer as a cascade of filters.

        @param bands        Vector of band parameters (frequency, gain, Q)
        @param sampleRate   The sample rate in Hz
        @returns           Vector of biquad coefficients, one per band
    */
    static std::vector<BiquadCoefficients<CoeffType>> designMultiBandEQ (
        const std::vector<std::tuple<CoeffType, CoeffType, CoeffType>>& bands,
        double sampleRate
    ) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> coeffs;
        coeffs.reserve (bands.size());

        for (const auto& band : bands)
        {
            const auto frequency = std::get<0> (band);
            const auto gainDb = std::get<1> (band);
            const auto Q = std::get<2> (band);

            // Skip bands with zero gain (no effect)
            if (std::abs (gainDb) > static_cast<CoeffType> (0.1))
            {
                coeffs.push_back (designParametricBell (frequency, gainDb, Q, sampleRate));
            }
        }

        return coeffs;
    }

private:
};

} // namespace yup
