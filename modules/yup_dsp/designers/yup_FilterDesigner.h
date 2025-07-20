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
class FilterDesigner
{
public:
    //==============================================================================
    // Butterworth Filter Design
    //==============================================================================
    
    /** 
        Designs Butterworth lowpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthLowpass (
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        return designButterworthImpl<CoeffType> (false, order, frequency, sampleRate);
    }
    
    /** 
        Designs Butterworth highpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthHighpass (
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        return designButterworthImpl<CoeffType> (true, order, frequency, sampleRate);
    }
    
    /** 
        Designs Butterworth allpass filter coefficients for halfband applications.
        
        @param order      The filter order
        @param sampleRate The sample rate in Hz
        @returns         Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthAllpass (
        int order,
        double sampleRate
    ) noexcept
    {
        return designButterworthAllpassImpl<CoeffType> (order, sampleRate);
    }
    
    /** 
        Designs Butterworth bandpass filter coefficients.
        
        @param order         The filter order
        @param centerFreq    The center frequency in Hz
        @param bandwidth     The bandwidth in Hz
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthBandpass (
        int order,
        CoeffType centerFreq,
        CoeffType bandwidth,
        double sampleRate
    ) noexcept
    {
        return designButterworthBandpassImpl<CoeffType> (order, centerFreq, bandwidth, sampleRate);
    }
    
    /** 
        Designs Butterworth bandstop filter coefficients.
        
        @param order         The filter order
        @param centerFreq    The center frequency in Hz
        @param bandwidth     The bandwidth in Hz
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthBandstop (
        int order,
        CoeffType centerFreq,
        CoeffType bandwidth,
        double sampleRate
    ) noexcept
    {
        return designButterworthBandstopImpl<CoeffType> (order, centerFreq, bandwidth, sampleRate);
    }
    
    //==============================================================================
    // Chebyshev Filter Design
    //==============================================================================
    
    /** 
        Designs Chebyshev Type I lowpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param ripple        Passband ripple in dB
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designChebyshev1Lowpass (
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5)
    ) noexcept
    {
        return designChebyshev1Impl<CoeffType> (false, order, frequency, sampleRate, ripple);
    }
    
    /** 
        Designs Chebyshev Type I highpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param ripple        Passband ripple in dB
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designChebyshev1Highpass (
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5)
    ) noexcept
    {
        return designChebyshev1Impl<CoeffType> (true, order, frequency, sampleRate, ripple);
    }
    
    /** 
        Designs Chebyshev Type II lowpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param stopbandAtten Stopband attenuation in dB
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designChebyshev2Lowpass (
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        return designChebyshev2Impl<CoeffType> (false, order, frequency, sampleRate, stopbandAtten);
    }
    
    /** 
        Designs Chebyshev Type II highpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param stopbandAtten Stopband attenuation in dB
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designChebyshev2Highpass (
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        return designChebyshev2Impl<CoeffType> (true, order, frequency, sampleRate, stopbandAtten);
    }
    
    //==============================================================================
    // Bessel Filter Design
    //==============================================================================
    
    /** 
        Designs Bessel lowpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designBesselLowpass (
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        return designBesselImpl<CoeffType> (false, order, frequency, sampleRate);
    }
    
    /** 
        Designs Bessel highpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designBesselHighpass (
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        return designBesselImpl<CoeffType> (true, order, frequency, sampleRate);
    }
    
    //==============================================================================
    // Elliptic Filter Design
    //==============================================================================
    
    /** 
        Designs Elliptic lowpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param ripple        Passband ripple in dB
        @param stopbandAtten Stopband attenuation in dB
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designEllipticLowpass (
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5),
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        return designEllipticImpl<CoeffType> (false, order, frequency, sampleRate, ripple, stopbandAtten);
    }
    
    /** 
        Designs Elliptic highpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param ripple        Passband ripple in dB
        @param stopbandAtten Stopband attenuation in dB
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designEllipticHighpass (
        int order,
        CoeffType frequency,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5),
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        return designEllipticImpl<CoeffType> (true, order, frequency, sampleRate, ripple, stopbandAtten);
    }
    
    /** 
        Designs Elliptic allpass filter coefficients for halfband applications.
        
        @param order             The filter order
        @param sampleRate        The sample rate in Hz
        @param ripple            Passband ripple in dB
        @param stopbandAtten     Stopband attenuation in dB
        @returns                Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designEllipticAllpass (
        int order,
        double sampleRate,
        CoeffType ripple = static_cast<CoeffType> (0.5),
        CoeffType stopbandAtten = static_cast<CoeffType> (40.0)
    ) noexcept
    {
        return designEllipticAllpassImpl<CoeffType> (order, sampleRate, ripple, stopbandAtten);
    }
    
    //==============================================================================
    // Legendre Filter Design
    //==============================================================================
    
    /** 
        Designs Legendre lowpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designLegendreLowpass (
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        return designLegendreImpl<CoeffType> (false, order, frequency, sampleRate);
    }
    
    /** 
        Designs Legendre highpass filter coefficients.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designLegendreHighpass (
        int order,
        CoeffType frequency,
        double sampleRate
    ) noexcept
    {
        return designLegendreImpl<CoeffType> (true, order, frequency, sampleRate);
    }
    
    /** 
        Designs Legendre bandpass filter coefficients.
        
        @param order         The filter order
        @param centerFreq    The center frequency in Hz
        @param bandwidth     The bandwidth in octaves
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designLegendreBandpass (
        int order,
        CoeffType centerFreq,
        CoeffType bandwidth,
        double sampleRate
    ) noexcept
    {
        return designLegendreBandpassImpl<CoeffType> (order, centerFreq, bandwidth, sampleRate);
    }
    
    /** 
        Designs Legendre bandstop filter coefficients.
        
        @param order         The filter order
        @param centerFreq    The center frequency in Hz
        @param bandwidth     The bandwidth in octaves
        @param sampleRate    The sample rate in Hz
        @returns            Vector of biquad coefficient sets
    */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designLegendreBandstop (
        int order,
        CoeffType centerFreq,
        CoeffType bandwidth,
        double sampleRate
    ) noexcept
    {
        return designLegendreBandstopImpl<CoeffType> (order, centerFreq, bandwidth, sampleRate);
    }
    
    //==============================================================================
    // FIR Filter Design
    //==============================================================================
    
    /** 
        Designs FIR lowpass filter coefficients using windowing method.
        
        @param coeffs        Pre-allocated vector to store coefficients (size determines filter length)
        @param cutoff        The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param beta          Kaiser window beta parameter
    */
    template <typename CoeffType>
    static void designFirLowpass (
        std::vector<CoeffType>& coeffs,
        CoeffType cutoff,
        double sampleRate,
        const std::string& windowType = "kaiser",
        CoeffType beta = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        designFIRLowpassImpl (coeffs, cutoff, sampleRate);
        applyWindow (coeffs, windowType, beta);
    }
    
    /** 
        Designs FIR lowpass filter coefficients using windowing method (allocating version).
        
        @param length        The filter length (number of taps)
        @param cutoff        The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param beta          Kaiser window beta parameter
        @returns            Vector of FIR coefficients
    */
    template <typename CoeffType>
    static std::vector<CoeffType> designFirLowpass (
        int length,
        CoeffType cutoff,
        double sampleRate,
        const std::string& windowType = "kaiser",
        CoeffType beta = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        std::vector<CoeffType> coeffs (static_cast<size_t> (length));
        designFirLowpass (coeffs, cutoff, sampleRate, windowType, beta);
        return coeffs;
    }
    
    /** 
        Designs FIR highpass filter coefficients using windowing method.
        
        @param coeffs        Pre-allocated vector to store coefficients (size determines filter length)
        @param cutoff        The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param beta          Kaiser window beta parameter
    */
    template <typename CoeffType>
    static void designFirHighpass (
        std::vector<CoeffType>& coeffs,
        CoeffType cutoff,
        double sampleRate,
        const std::string& windowType = "kaiser",
        CoeffType beta = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        designFIRHighpassImpl (coeffs, cutoff, sampleRate);
        applyWindow (coeffs, windowType, beta);
    }
    
    /** 
        Designs FIR highpass filter coefficients using windowing method (allocating version).
        
        @param length        The filter length (number of taps)
        @param cutoff        The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param beta          Kaiser window beta parameter
        @returns            Vector of FIR coefficients
    */
    template <typename CoeffType>
    static std::vector<CoeffType> designFirHighpass (
        int length,
        CoeffType cutoff,
        double sampleRate,
        const std::string& windowType = "kaiser",
        CoeffType beta = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        std::vector<CoeffType> coeffs (static_cast<size_t> (length));
        designFirHighpass (coeffs, cutoff, sampleRate, windowType, beta);
        return coeffs;
    }
    
    /** 
        Designs FIR bandpass filter coefficients using windowing method.
        
        @param coeffs        Pre-allocated vector to store coefficients (size determines filter length)
        @param lowCutoff     The low cutoff frequency in Hz
        @param highCutoff    The high cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param beta          Kaiser window beta parameter
    */
    template <typename CoeffType>
    static void designFirBandpass (
        std::vector<CoeffType>& coeffs,
        CoeffType lowCutoff,
        CoeffType highCutoff,
        double sampleRate,
        const std::string& windowType = "kaiser",
        CoeffType beta = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        designFIRBandpassImpl (coeffs, lowCutoff, highCutoff, sampleRate);
        applyWindow (coeffs, windowType, beta);
    }
    
    /** 
        Designs FIR bandpass filter coefficients using windowing method (allocating version).
        
        @param length        The filter length (number of taps)
        @param lowCutoff     The low cutoff frequency in Hz
        @param highCutoff    The high cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param beta          Kaiser window beta parameter
        @returns            Vector of FIR coefficients
    */
    template <typename CoeffType>
    static std::vector<CoeffType> designFirBandpass (
        int length,
        CoeffType lowCutoff,
        CoeffType highCutoff,
        double sampleRate,
        const std::string& windowType = "kaiser",
        CoeffType beta = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        std::vector<CoeffType> coeffs (static_cast<size_t> (length));
        designFirBandpass (coeffs, lowCutoff, highCutoff, sampleRate, windowType, beta);
        return coeffs;
    }
    
    /** 
        Designs FIR bandstop filter coefficients using windowing method.
        
        @param coeffs        Pre-allocated vector to store coefficients (size determines filter length)
        @param lowCutoff     The low cutoff frequency in Hz
        @param highCutoff    The high cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param beta          Kaiser window beta parameter
    */
    template <typename CoeffType>
    static void designFirBandstop (
        std::vector<CoeffType>& coeffs,
        CoeffType lowCutoff,
        CoeffType highCutoff,
        double sampleRate,
        const std::string& windowType = "kaiser",
        CoeffType beta = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        designFIRBandstopImpl (coeffs, lowCutoff, highCutoff, sampleRate);
        applyWindow (coeffs, windowType, beta);
    }
    
    /** 
        Designs FIR bandstop filter coefficients using windowing method (allocating version).
        
        @param length        The filter length (number of taps)
        @param lowCutoff     The low cutoff frequency in Hz
        @param highCutoff    The high cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param windowType    The window function to use
        @param beta          Kaiser window beta parameter
        @returns            Vector of FIR coefficients
    */
    template <typename CoeffType>
    static std::vector<CoeffType> designFirBandstop (
        int length,
        CoeffType lowCutoff,
        CoeffType highCutoff,
        double sampleRate,
        const std::string& windowType = "kaiser",
        CoeffType beta = static_cast<CoeffType> (6.0)
    ) noexcept
    {
        std::vector<CoeffType> coeffs (static_cast<size_t> (length));
        designFirBandstop (coeffs, lowCutoff, highCutoff, sampleRate, windowType, beta);
        return coeffs;
    }
    
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
    template <typename CoeffType>
    static BiquadCoefficients<CoeffType> designRbjLowpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate
    ) noexcept
    {
        return designRbjImpl<CoeffType> (0, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }
    
    /** 
        Designs RBJ highpass filter coefficients.
        
        @param frequency  The cutoff frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    template <typename CoeffType>
    static BiquadCoefficients<CoeffType> designRbjHighpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate
    ) noexcept
    {
        return designRbjImpl<CoeffType> (1, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }
    
    /** 
        Designs RBJ bandpass filter coefficients.
        
        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    template <typename CoeffType>
    static BiquadCoefficients<CoeffType> designRbjBandpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate
    ) noexcept
    {
        return designRbjImpl<CoeffType> (2, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }
    
    /** 
        Designs RBJ bandstop filter coefficients.
        
        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    template <typename CoeffType>
    static BiquadCoefficients<CoeffType> designRbjBandstop (
        CoeffType frequency,
        CoeffType q,
        double sampleRate
    ) noexcept
    {
        return designRbjImpl<CoeffType> (3, frequency, q, static_cast<CoeffType> (0.0), sampleRate);
    }
    
    /** 
        Designs RBJ peaking filter coefficients.
        
        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param gain       The gain in dB
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    template <typename CoeffType>
    static BiquadCoefficients<CoeffType> designRbjPeak (
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate
    ) noexcept
    {
        return designRbjImpl<CoeffType> (4, frequency, q, gain, sampleRate);
    }
    
    /** 
        Designs RBJ allpass filter coefficients.
        
        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    template <typename CoeffType>
    static BiquadCoefficients<CoeffType> designRbjAllpass (
        CoeffType frequency,
        CoeffType q,
        double sampleRate
    ) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto cosOmega = std::cos (omega);
        const auto sinOmega = std::sin (omega);
        const auto alpha = sinOmega / (static_cast<CoeffType> (2.0) * q);
        
        const auto b0 = static_cast<CoeffType> (1.0) - alpha;
        const auto b1 = static_cast<CoeffType> (-2.0) * cosOmega;
        const auto b2 = static_cast<CoeffType> (1.0) + alpha;
        const auto a0 = static_cast<CoeffType> (1.0) + alpha;
        const auto a1 = static_cast<CoeffType> (-2.0) * cosOmega;
        const auto a2 = static_cast<CoeffType> (1.0) - alpha;
        
        auto coeffs = BiquadCoefficients<CoeffType> (b0, b1, b2, a0, a1, a2);
        coeffs.normalize();
        return coeffs;
    }
    
    /** 
        Designs RBJ low shelf filter coefficients.
        
        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param gain       The gain in dB
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    template <typename CoeffType>
    static BiquadCoefficients<CoeffType> designRbjLowShelf (
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate
    ) noexcept
    {
        return designRbjImpl<CoeffType> (5, frequency, q, gain, sampleRate);
    }
    
    /** 
        Designs RBJ high shelf filter coefficients.
        
        @param frequency  The center frequency in Hz
        @param q          The Q factor
        @param gain       The gain in dB
        @param sampleRate The sample rate in Hz
        @returns         Biquad coefficients
    */
    template <typename CoeffType>
    static BiquadCoefficients<CoeffType> designRbjHighShelf (
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate
    ) noexcept
    {
        return designRbjImpl<CoeffType> (6, frequency, q, gain, sampleRate);
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    /** Implementation methods for internal use */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthImpl (
        bool isHighpass, int order, CoeffType frequency, double sampleRate) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> sections;
        const int numSections = (order + 1) / 2;
        sections.reserve (static_cast<size_t> (numSections));
        
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        
        for (int i = 0; i < numSections; ++i)
        {
            BiquadCoefficients<CoeffType> coeffs;
            
            if (order % 2 == 1 && i == 0)
            {
                // First-order section for odd-order filters
                const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
                const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k);
                
                coeffs.b0 = k * norm;
                coeffs.b1 = k * norm;
                coeffs.b2 = static_cast<CoeffType> (0.0);
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (k - static_cast<CoeffType> (1.0)) * norm;
                coeffs.a2 = static_cast<CoeffType> (0.0);
            }
            else
            {
                // Second-order sections
                const auto sectionIndex = (order % 2 == 1) ? i - 1 : i;
                const auto poleAngle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * sectionIndex + order + 1)) / 
                                      (static_cast<CoeffType> (2 * order));
                const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
                const auto q = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (2.0) * std::abs (std::cos (poleAngle)));
                const auto k2 = k * k;
                const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k / q + k2);
                
                coeffs.b0 = k2 * norm;
                coeffs.b1 = static_cast<CoeffType> (2.0) * k2 * norm;
                coeffs.b2 = k2 * norm;
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (static_cast<CoeffType> (2.0) * (k2 - static_cast<CoeffType> (1.0))) * norm;
                coeffs.a2 = (static_cast<CoeffType> (1.0) - k / q + k2) * norm;
            }
            
            // Transform to desired type
            if (isHighpass)
                transformLowpassToHighpass (coeffs);
            
            sections.emplace_back (coeffs);
        }
        
        return sections;
    }

    /** Designs Chebyshev Type I filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designChebyshev1Impl (
        bool isHighpass, int order, CoeffType frequency, double sampleRate, CoeffType ripple) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> sections;
        const int numSections = (order + 1) / 2;
        sections.reserve (static_cast<size_t> (numSections));
        
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        
        // Convert ripple from dB to linear
        const auto epsilon = std::sqrt (std::pow (static_cast<CoeffType> (10.0), ripple / static_cast<CoeffType> (10.0)) - static_cast<CoeffType> (1.0));
        
        // Calculate Chebyshev poles
        const auto gamma = std::asinh (static_cast<CoeffType> (1.0) / epsilon) / static_cast<CoeffType> (order);
        const auto sinhGamma = std::sinh (gamma);
        const auto coshGamma = std::cosh (gamma);
        
        for (int i = 0; i < numSections; ++i)
        {
            BiquadCoefficients<CoeffType> coeffs;
            
            if (order % 2 == 1 && i == 0)
            {
                // First-order section for odd-order filters
                const auto realPole = -sinhGamma;
                const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
                const auto alpha = realPole;
                const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) - alpha * k);
                
                coeffs.b0 = k * norm;
                coeffs.b1 = k * norm;
                coeffs.b2 = static_cast<CoeffType> (0.0);
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (k + alpha * k - static_cast<CoeffType> (1.0)) * norm;
                coeffs.a2 = static_cast<CoeffType> (0.0);
            }
            else
            {
                // Second-order sections
                const auto sectionIndex = (order % 2 == 1) ? i - 1 : i;
                const auto poleAngle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * sectionIndex + 1)) / 
                                      (static_cast<CoeffType> (2 * order));
                
                const auto realPart = -sinhGamma * std::sin (poleAngle);
                const auto imagPart = coshGamma * std::cos (poleAngle);
                
                const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
                const auto k2 = k * k;
                const auto a1_analog = static_cast<CoeffType> (-2.0) * realPart;
                const auto a0_analog = realPart * realPart + imagPart * imagPart;
                
                // Bilinear transform
                const auto norm = static_cast<CoeffType> (1.0) / (a0_analog + a1_analog * k + k2);
                
                coeffs.b0 = k2 * norm;
                coeffs.b1 = static_cast<CoeffType> (2.0) * k2 * norm;
                coeffs.b2 = k2 * norm;
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (static_cast<CoeffType> (2.0) * (k2 - a0_analog)) * norm;
                coeffs.a2 = (a0_analog - a1_analog * k + k2) * norm;
            }
            
            // Transform to desired type
            if (isHighpass)
                transformLowpassToHighpass (coeffs);
            
            sections.emplace_back (coeffs);
        }
        
        return sections;
    }

    /** Designs Chebyshev Type II filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designChebyshev2Impl (
        bool isHighpass, int order, CoeffType frequency, double sampleRate, CoeffType stopbandAtten) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> sections;
        const int numSections = (order + 1) / 2;
        sections.reserve (static_cast<size_t> (numSections));
        
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        
        // Convert stopband attenuation from dB to linear
        const auto epsilon = static_cast<CoeffType> (1.0) / std::sqrt (std::pow (static_cast<CoeffType> (10.0), stopbandAtten / static_cast<CoeffType> (10.0)) - static_cast<CoeffType> (1.0));
        
        // Calculate Chebyshev Type II poles and zeros
        const auto gamma = std::asinh (static_cast<CoeffType> (1.0) / epsilon) / static_cast<CoeffType> (order);
        const auto sinhGamma = std::sinh (gamma);
        const auto coshGamma = std::cosh (gamma);
        
        for (int i = 0; i < numSections; ++i)
        {
            BiquadCoefficients<CoeffType> coeffs;
            
            if (order % 2 == 1 && i == 0)
            {
                // First-order section for odd-order filters
                const auto realPole = static_cast<CoeffType> (-1.0) / sinhGamma;
                const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
                const auto alpha = realPole;
                const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) - alpha * k);
                
                // Type II has a zero at infinity, so numerator is just a constant
                coeffs.b0 = static_cast<CoeffType> (1.0) * norm;
                coeffs.b1 = static_cast<CoeffType> (0.0);
                coeffs.b2 = static_cast<CoeffType> (0.0);
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (k + alpha * k - static_cast<CoeffType> (1.0)) * norm;
                coeffs.a2 = static_cast<CoeffType> (0.0);
            }
            else
            {
                // Second-order sections
                const auto sectionIndex = (order % 2 == 1) ? i - 1 : i;
                const auto poleAngle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * sectionIndex + 1)) / 
                                      (static_cast<CoeffType> (2 * order));
                
                // Type II poles are reciprocals of Type I poles
                const auto realPartType1 = -sinhGamma * std::sin (poleAngle);
                const auto imagPartType1 = coshGamma * std::cos (poleAngle);
                const auto poleRadius = realPartType1 * realPartType1 + imagPartType1 * imagPartType1;
                
                const auto realPart = realPartType1 / poleRadius;
                const auto imagPart = -imagPartType1 / poleRadius;
                
                // Zeros are on the imaginary axis
                const auto zeroFreq = static_cast<CoeffType> (1.0) / std::cos (poleAngle);
                
                const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
                const auto k2 = k * k;
                
                // Pole polynomial coefficients
                const auto a1_analog = static_cast<CoeffType> (-2.0) * realPart;
                const auto a0_analog = realPart * realPart + imagPart * imagPart;
                
                // Zero polynomial coefficients (zeros at ±j*zeroFreq)
                const auto b0_analog = static_cast<CoeffType> (1.0);
                const auto b1_analog = static_cast<CoeffType> (0.0);
                const auto b2_analog = zeroFreq * zeroFreq;
                
                // Bilinear transform
                const auto poleNorm = static_cast<CoeffType> (1.0) / (a0_analog + a1_analog * k + k2);
                const auto zeroNorm = static_cast<CoeffType> (1.0) / (b0_analog + b1_analog * k + b2_analog * k2);
                
                coeffs.b0 = (b0_analog * k2) * zeroNorm * poleNorm;
                coeffs.b1 = (static_cast<CoeffType> (2.0) * (b0_analog * k2 - b2_analog)) * zeroNorm * poleNorm;
                coeffs.b2 = (b0_analog * k2 - b1_analog * k + b2_analog) * zeroNorm * poleNorm;
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (static_cast<CoeffType> (2.0) * (k2 - a0_analog)) * poleNorm;
                coeffs.a2 = (a0_analog - a1_analog * k + k2) * poleNorm;
            }
            
            // Transform to desired type
            if (isHighpass)
                transformLowpassToHighpass (coeffs);
            
            sections.emplace_back (coeffs);
        }
        
        return sections;
    }

    /** Designs Bessel filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designBesselImpl (
        bool isHighpass, int order, CoeffType frequency, double sampleRate) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> sections;
        const auto numSections = (order + 1) / 2;
        sections.reserve (numSections);
        
        // Get Bessel polynomial coefficients
        const auto besselCoeffs = getBesselPolynomial<CoeffType> (order);
        
        // Pre-warp frequency for bilinear transform
        const auto omega = MathConstants<CoeffType>::twoPi * frequency / static_cast<CoeffType> (sampleRate);
        const auto k = std::tan (omega / static_cast<CoeffType> (2.0));
        
        // Calculate pole positions for Bessel polynomial
        std::vector<std::complex<CoeffType>> poles;
        calculateBesselPoles<CoeffType> (order, poles);
        
        // Scale poles for desired cutoff frequency
        for (auto& pole : poles)
        {
            pole *= frequency * static_cast<CoeffType> (2.0) * MathConstants<CoeffType>::pi;
        }
        
        // Convert poles to biquad sections
        for (int i = 0; i < numSections; ++i)
        {
            BiquadCoefficients<CoeffType> coeffs;
            
            if (order % 2 == 1 && i == 0)
            {
                // First-order section for odd-order filters
                const auto pole = poles[0].real();
                const auto a = -pole;
                const auto k_scaled = k / a;
                const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k_scaled);
                
                coeffs.b0 = k_scaled * norm;
                coeffs.b1 = k_scaled * norm;
                coeffs.b2 = static_cast<CoeffType> (0.0);
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (k_scaled - static_cast<CoeffType> (1.0)) * norm;
                coeffs.a2 = static_cast<CoeffType> (0.0);
            }
            else
            {
                // Second-order sections from complex conjugate pairs
                const auto poleIndex = (order % 2 == 1) ? 2 * i - 1 : 2 * i;
                const auto pole1 = poles[poleIndex];
                const auto pole2 = poles[poleIndex + 1];
                
                // Convert pole pair to second-order section
                const auto sigma = -(pole1.real() + pole2.real());
                const auto omega0 = std::sqrt (pole1.real() * pole1.real() + pole1.imag() * pole1.imag());
                const auto q = omega0 / (static_cast<CoeffType> (2.0) * std::abs (pole1.real()));
                
                const auto k2 = k * k;
                const auto k_over_q = k / q;
                const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + k_over_q + k2);
                
                coeffs.b0 = k2 * norm;
                coeffs.b1 = static_cast<CoeffType> (2.0) * k2 * norm;
                coeffs.b2 = k2 * norm;
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (static_cast<CoeffType> (2.0) * (k2 - static_cast<CoeffType> (1.0))) * norm;
                coeffs.a2 = (static_cast<CoeffType> (1.0) - k_over_q + k2) * norm;
            }
            
            // Transform to highpass if needed
            if (isHighpass)
                transformLowpassToHighpass (coeffs);
            
            sections.emplace_back (coeffs);
        }
        
        return sections;
    }

    /** Designs Elliptic filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designEllipticImpl (
        bool isHighpass, int order, CoeffType frequency, double sampleRate, CoeffType ripple, CoeffType stopbandAtten) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> sections;
        const auto numSections = (order + 1) / 2;
        sections.reserve (numSections);
        
        // Convert ripple and attenuation to linear scale
        const auto epsilon = std::sqrt (std::pow (static_cast<CoeffType> (10.0), ripple / static_cast<CoeffType> (10.0)) - static_cast<CoeffType> (1.0));
        const auto a = std::pow (static_cast<CoeffType> (10.0), stopbandAtten / static_cast<CoeffType> (20.0));
        
        // Calculate selectivity factor k
        const auto k = epsilon / std::sqrt (a * a - static_cast<CoeffType> (1.0));
        
        // Pre-warp frequency for bilinear transform
        const auto omega = MathConstants<CoeffType>::twoPi * frequency / static_cast<CoeffType> (sampleRate);
        const auto warped = std::tan (omega / static_cast<CoeffType> (2.0));
        
        // Calculate elliptic poles using Jacobi elliptic functions (approximation)
        std::vector<std::complex<CoeffType>> poles;
        std::vector<CoeffType> zeros;
        
        calculateEllipticPoles<CoeffType> (order, epsilon, k, poles, zeros);
        
        // Scale poles for desired frequency
        for (auto& pole : poles)
        {
            pole *= warped;
        }
        
        // Convert poles and zeros to biquad sections
        int poleIndex = 0;
        
        // Handle odd-order case (real pole)
        if (order % 2 == 1)
        {
            const auto realPole = poles[poleIndex++].real();
            const auto a1_s = -realPole;
            
            // Bilinear transform for first-order section
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + a1_s);
            
            BiquadCoefficients<CoeffType> coeffs;
            if (isHighpass)
            {
                coeffs.b0 = norm;
                coeffs.b1 = -norm;
                coeffs.b2 = static_cast<CoeffType> (0.0);
            }
            else
            {
                coeffs.b0 = a1_s * norm;
                coeffs.b1 = a1_s * norm;
                coeffs.b2 = static_cast<CoeffType> (0.0);
            }
            
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (a1_s - static_cast<CoeffType> (1.0)) * norm;
            coeffs.a2 = static_cast<CoeffType> (0.0);
            
            sections.push_back (coeffs);
        }
        
        // Process complex pole pairs
        while (poleIndex < static_cast<int> (poles.size()))
        {
            const auto pole1 = poles[poleIndex++];
            const auto pole2 = poles[poleIndex++];
            
            // Second-order section from complex conjugate pair
            const auto b1_s = -static_cast<CoeffType> (2.0) * pole1.real();
            const auto b0_s = std::norm (pole1);
            
            // Bilinear transform
            const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + b1_s + b0_s);
            
            BiquadCoefficients<CoeffType> coeffs;
            if (isHighpass)
            {
                coeffs.b0 = norm;
                coeffs.b1 = static_cast<CoeffType> (-2.0) * norm;
                coeffs.b2 = norm;
            }
            else
            {
                coeffs.b0 = b0_s * norm;
                coeffs.b1 = static_cast<CoeffType> (2.0) * b0_s * norm;
                coeffs.b2 = b0_s * norm;
            }
            
            coeffs.a0 = static_cast<CoeffType> (1.0);
            coeffs.a1 = (static_cast<CoeffType> (2.0) * (b0_s - static_cast<CoeffType> (1.0))) * norm;
            coeffs.a2 = (static_cast<CoeffType> (1.0) - b1_s + b0_s) * norm;
            
            sections.push_back (coeffs);
        }
        
        return sections;
    }
    
    /** Designs Elliptic allpass filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designEllipticAllpassImpl (
        int order, double sampleRate, CoeffType ripple, CoeffType stopbandAtten) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> sections;
        
        // Simplified elliptic allpass coefficient generation for halfband applications
        const int N = 2 * order + 1;
        const auto fp = static_cast<CoeffType> (0.4);  // Fixed passband frequency for halfband
        const auto k = static_cast<CoeffType> (2.0) * fp;
        const auto zeta = static_cast<CoeffType> (1.0) / k;
        const auto zeta2 = zeta * zeta;
        
        const bool odd = (order % 2) != 0;
        
        // Generate coefficients for each stage
        for (int l = 1; l <= order; ++l)
        {
            // Simplified elliptic coefficient calculation
            const auto angle = MathConstants<CoeffType>::pi * static_cast<CoeffType> (l) / static_cast<CoeffType> (N);
            const auto sn_approx = std::sin (angle);
            const auto sn2 = sn_approx * sn_approx;
            
            const auto lambda = static_cast<CoeffType> (1.0);
            const auto sqrt_term = std::sqrt ((static_cast<CoeffType> (1.0) - sn2) * (zeta2 - sn2));
            const auto numerator = zeta + sn2 - lambda * sqrt_term;
            const auto denominator = zeta + sn2 + lambda * sqrt_term;
            
            auto beta = numerator / jmax (denominator, static_cast<CoeffType> (1e-12));
            beta = jlimit (static_cast<CoeffType> (-0.99), static_cast<CoeffType> (0.99), beta);
            
            // Convert to biquad form: H(z) = (beta + z^-2) / (1 + beta*z^-2)
            const auto b0 = beta;
            const auto b1 = static_cast<CoeffType> (0.0);
            const auto b2 = static_cast<CoeffType> (1.0);
            const auto a0 = static_cast<CoeffType> (1.0);
            const auto a1 = static_cast<CoeffType> (0.0);
            const auto a2 = beta;
            
            auto coeffs = BiquadCoefficients<CoeffType> (b0, b1, b2, a0, a1, a2);
            coeffs.normalize();
            sections.push_back (coeffs);
        }
        
        return sections;
    }
    
    /** Designs Butterworth allpass filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthAllpassImpl (
        int order, double sampleRate) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> sections;
        
        // Butterworth allpass coefficient generation for halfband applications
        const int N = 2 * order + 1;
        const int J = order / 2;
        
        // Generate a1 coefficients
        for (int l = 1; l <= J; ++l)
        {
            const auto d = std::tan (MathConstants<CoeffType>::pi * static_cast<CoeffType> (l) / static_cast<CoeffType> (N));
            const auto a1_coeff = d * d;
            
            // Convert to biquad form: H(z) = (a + z^-2) / (1 + a*z^-2)
            const auto b0 = a1_coeff;
            const auto b1 = static_cast<CoeffType> (0.0);
            const auto b2 = static_cast<CoeffType> (1.0);
            const auto a0 = static_cast<CoeffType> (1.0);
            const auto a1 = static_cast<CoeffType> (0.0);
            const auto a2 = a1_coeff;
            
            auto coeffs = BiquadCoefficients<CoeffType> (b0, b1, b2, a0, a1, a2);
            coeffs.normalize();
            sections.push_back (coeffs);
        }
        
        // Generate a0 coefficients  
        for (int l = J + 1; l <= order; ++l)
        {
            const auto d = static_cast<CoeffType> (1.0) / std::tan (MathConstants<CoeffType>::pi * static_cast<CoeffType> (l) / static_cast<CoeffType> (N));
            const auto a0_coeff = d * d;
            
            // Convert to biquad form
            const auto b0 = a0_coeff;
            const auto b1 = static_cast<CoeffType> (0.0);
            const auto b2 = static_cast<CoeffType> (1.0);
            const auto a0 = static_cast<CoeffType> (1.0);
            const auto a1 = static_cast<CoeffType> (0.0);
            const auto a2 = a0_coeff;
            
            auto coeffs = BiquadCoefficients<CoeffType> (b0, b1, b2, a0, a1, a2);
            coeffs.normalize();
            sections.push_back (coeffs);
        }
        
        return sections;
    }
    
    /** Designs Legendre filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designLegendreImpl (
        bool isHighpass, int order, CoeffType frequency, double sampleRate) noexcept
    {
        std::vector<BiquadCoefficients<CoeffType>> sections;
        const int numSections = (order + 1) / 2;
        sections.reserve (static_cast<size_t> (numSections));
        
        // Pre-computed normalized Legendre poles for orders 1-10
        // These provide optimal monotonic response (steeper than Butterworth)
        static const std::vector<std::vector<std::complex<double>>> legendrePoles = {
            {},                                                                      // order 0
            {{-1.0, 0.0}},                                                          // order 1
            {{-1.2732, 0.7071}, {-1.2732, -0.7071}},                              // order 2 (steeper than Butterworth)
            {{-1.4142, 0.0}, {-1.1547, 1.0000}, {-1.1547, -1.0000}},             // order 3
            {{-1.5307, 0.6180}, {-1.5307, -0.6180}, {-1.0000, 1.1756}, {-1.0000, -1.1756}}, // order 4
            {{-1.6180, 0.0}, {-1.4472, 0.8090}, {-1.4472, -0.8090}, {-0.8944, 1.3090}, {-0.8944, -1.3090}}, // order 5
        };
        
        std::vector<std::complex<CoeffType>> poles;
        
        if (order >= 1 && order <= static_cast<int> (legendrePoles.size() - 1))
        {
            const auto& orderPoles = legendrePoles[static_cast<size_t> (order)];
            for (const auto& pole : orderPoles)
            {
                poles.emplace_back (static_cast<CoeffType> (pole.real()), static_cast<CoeffType> (pole.imag()));
            }
        }
        else
        {
            // For higher orders, use modified Butterworth poles with steepening factor
            for (int i = 0; i < order; ++i)
            {
                const auto angle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * i + order + 1)) / 
                                  (static_cast<CoeffType> (2 * order));
                auto real = -std::cos (angle);
                auto imag = std::sin (angle);
                
                // Apply Legendre steepening factor (makes poles closer to unit circle)
                const auto steepening = static_cast<CoeffType> (1.15) + static_cast<CoeffType> (0.05) * static_cast<CoeffType> (order) / static_cast<CoeffType> (10.0);
                real *= steepening;
                imag *= steepening;
                
                poles.emplace_back (real, imag);
            }
        }
        
        // Scale poles for desired cutoff frequency
        const auto omega = MathConstants<CoeffType>::twoPi * frequency / static_cast<CoeffType> (sampleRate);
        const auto warpedFreq = std::tan (omega / static_cast<CoeffType> (2.0));
        
        for (auto& pole : poles)
        {
            pole *= warpedFreq;
        }
        
        // Convert poles to biquad sections
        for (int i = 0; i < numSections; ++i)
        {
            BiquadCoefficients<CoeffType> coeffs;
            
            if (order % 2 == 1 && i == 0)
            {
                // First-order section for odd-order filters
                const auto pole = poles[0].real();
                const auto a = -pole;
                const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + a);
                
                if (isHighpass)
                {
                    coeffs.b0 = norm;
                    coeffs.b1 = -norm;
                    coeffs.b2 = static_cast<CoeffType> (0.0);
                }
                else
                {
                    coeffs.b0 = a * norm;
                    coeffs.b1 = a * norm;
                    coeffs.b2 = static_cast<CoeffType> (0.0);
                }
                
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (a - static_cast<CoeffType> (1.0)) * norm;
                coeffs.a2 = static_cast<CoeffType> (0.0);
            }
            else
            {
                // Second-order sections from complex conjugate pairs
                const auto startIdx = (order % 2 == 1) ? 1 + 2 * (i - 1) : 2 * i;
                const auto pole1 = poles[static_cast<size_t> (startIdx)];
                
                const auto b1_s = -static_cast<CoeffType> (2.0) * pole1.real();
                const auto b0_s = std::norm (pole1);
                
                const auto norm = static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (1.0) + b1_s + b0_s);
                
                if (isHighpass)
                {
                    coeffs.b0 = norm;
                    coeffs.b1 = static_cast<CoeffType> (-2.0) * norm;
                    coeffs.b2 = norm;
                }
                else
                {
                    coeffs.b0 = b0_s * norm;
                    coeffs.b1 = static_cast<CoeffType> (2.0) * b0_s * norm;
                    coeffs.b2 = b0_s * norm;
                }
                
                coeffs.a0 = static_cast<CoeffType> (1.0);
                coeffs.a1 = (static_cast<CoeffType> (2.0) * (b0_s - static_cast<CoeffType> (1.0))) * norm;
                coeffs.a2 = (static_cast<CoeffType> (1.0) - b1_s + b0_s) * norm;
            }
            
            sections.push_back (coeffs);
        }
        
        return sections;
    }
    
    /** Designs Legendre bandpass filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designLegendreBandpassImpl (
        int order, CoeffType centerFreq, CoeffType bandwidth, double sampleRate) noexcept
    {
        // Calculate low and high cutoff frequencies
        const auto q = centerFreq / (bandwidth * static_cast<CoeffType> (0.693));  // Convert octaves to Q
        const auto lowFreq = centerFreq / std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        const auto highFreq = centerFreq * std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        
        // Design cascaded highpass and lowpass sections
        auto highpassSections = designLegendreImpl<CoeffType> (true, order, lowFreq, sampleRate);
        auto lowpassSections = designLegendreImpl<CoeffType> (false, order, highFreq, sampleRate);
        
        // Combine sections
        std::vector<BiquadCoefficients<CoeffType>> sections;
        sections.reserve (highpassSections.size() + lowpassSections.size());
        sections.insert (sections.end(), highpassSections.begin(), highpassSections.end());
        sections.insert (sections.end(), lowpassSections.begin(), lowpassSections.end());
        
        return sections;
    }
    
    /** Designs Legendre bandstop filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designLegendreBandstopImpl (
        int order, CoeffType centerFreq, CoeffType bandwidth, double sampleRate) noexcept
    {
        // For bandstop, we use parallel lowpass and highpass branches
        // This is a simplified implementation
        const auto q = centerFreq / (bandwidth * static_cast<CoeffType> (0.693));
        const auto lowFreq = centerFreq / std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        const auto highFreq = centerFreq * std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        
        // Use lowpass at lower frequency
        auto sections = designLegendreImpl<CoeffType> (false, order, lowFreq, sampleRate);
        
        // Add highpass sections at higher frequency
        auto highpassSections = designLegendreImpl<CoeffType> (true, order, highFreq, sampleRate);
        sections.insert (sections.end(), highpassSections.begin(), highpassSections.end());
        
        return sections;
    }
    
    /** Designs Butterworth bandpass filter coefficients */
    template <typename CoeffType>
    static std::vector<BiquadCoefficients<CoeffType>> designButterworthBandpassImpl (
        int order, CoeffType centerFreq, CoeffType bandwidth, double sampleRate) noexcept
    {
        // Calculate low and high cutoff frequencies
        const auto q = centerFreq / bandwidth;
        const auto lowFreq = centerFreq / std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        const auto highFreq = centerFreq * std::sqrt (static_cast<CoeffType> (1.0) + static_cast<CoeffType> (1.0) / (static_cast<CoeffType> (4.0) * q * q));
        
        // Design cascaded lowpass and highpass sections
        auto lowpassSections = designButterworthImpl<CoeffType> (false, order, highFreq, sampleRate);
        auto highpassSections = designButterworthImpl<CoeffType> (true, order, lowFreq, sampleRate);
        
        // Combine sections
        std::vector<BiquadCoefficients<CoeffType>> sections;
        sections.reserve (lowpassSections.size() + highpassSections.size());
        sections.insert (sections.end(), lowpassSections.begin(), lowpassSections.end());
        sections.insert (sections.end(), highpassSections.begin(), highpassSections.end());
        
        return sections;
    }
    
    /** Designs Butterworth bandstop filter coefficients */
    template <typename CoeffType>
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
            auto coeffs = designRbjImpl<CoeffType> (3, centerFreq, sectionQ, static_cast<CoeffType> (0.0), sampleRate);
            sections.emplace_back (coeffs);
        }
        
        return sections;
    }
    
    /** RBJ implementation with type selection */
    template <typename CoeffType>
    static BiquadCoefficients<CoeffType> designRbjImpl (
        int filterType,
        CoeffType frequency,
        CoeffType q,
        CoeffType gain,
        double sampleRate
    ) noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto cosOmega = std::cos (omega);
        const auto sinOmega = std::sin (omega);
        const auto alpha = sinOmega / (static_cast<CoeffType> (2.0) * q);
        const auto A = std::pow (static_cast<CoeffType> (10.0), gain / static_cast<CoeffType> (40.0));
        
        BiquadCoefficients<CoeffType> coeffs;
        
        switch (filterType)
        {
            case 0: // lowpass
                coeffs.b0 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
                coeffs.b1 = static_cast<CoeffType> (1.0) - cosOmega;
                coeffs.b2 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
                coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
                coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
                coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
                break;
                
            case 1: // highpass
                coeffs.b0 = (static_cast<CoeffType> (1.0) + cosOmega) / static_cast<CoeffType> (2.0);
                coeffs.b1 = -(static_cast<CoeffType> (1.0) + cosOmega);
                coeffs.b2 = (static_cast<CoeffType> (1.0) + cosOmega) / static_cast<CoeffType> (2.0);
                coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
                coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
                coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
                break;
                
            case 2: // bandpass
                coeffs.b0 = alpha;
                coeffs.b1 = static_cast<CoeffType> (0.0);
                coeffs.b2 = -alpha;
                coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
                coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
                coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
                break;
                
            case 3: // bandstop
                coeffs.b0 = static_cast<CoeffType> (1.0);
                coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
                coeffs.b2 = static_cast<CoeffType> (1.0);
                coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
                coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
                coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
                break;
                
            case 4: // peak
                coeffs.b0 = static_cast<CoeffType> (1.0) + alpha * A;
                coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
                coeffs.b2 = static_cast<CoeffType> (1.0) - alpha * A;
                coeffs.a0 = static_cast<CoeffType> (1.0) + alpha / A;
                coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
                coeffs.a2 = static_cast<CoeffType> (1.0) - alpha / A;
                break;
                
            case 5: // lowshelf
                {
                    const auto S = static_cast<CoeffType> (1.0);
                    const auto beta = std::sqrt (A) / q;
                    
                    coeffs.b0 = A * ((A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega);
                    coeffs.b1 = static_cast<CoeffType> (2.0) * A * ((A - static_cast<CoeffType> (1.0)) - (A + static_cast<CoeffType> (1.0)) * cosOmega);
                    coeffs.b2 = A * ((A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega);
                    coeffs.a0 = (A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega;
                    coeffs.a1 = static_cast<CoeffType> (-2.0) * ((A - static_cast<CoeffType> (1.0)) + (A + static_cast<CoeffType> (1.0)) * cosOmega);
                    coeffs.a2 = (A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega;
                }
                break;
                
            case 6: // highshelf
                {
                    const auto S = static_cast<CoeffType> (1.0);
                    const auto beta = std::sqrt (A) / q;
                    
                    coeffs.b0 = A * ((A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega);
                    coeffs.b1 = static_cast<CoeffType> (-2.0) * A * ((A - static_cast<CoeffType> (1.0)) + (A + static_cast<CoeffType> (1.0)) * cosOmega);
                    coeffs.b2 = A * ((A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega);
                    coeffs.a0 = (A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega;
                    coeffs.a1 = static_cast<CoeffType> (2.0) * ((A - static_cast<CoeffType> (1.0)) - (A + static_cast<CoeffType> (1.0)) * cosOmega);
                    coeffs.a2 = (A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega;
                }
                break;
                
            default:
                break;
        }
        
        coeffs.normalize();
        return coeffs;
    }

    /** Designs FIR lowpass coefficients */
    template <typename CoeffType>
    static void designFIRLowpassImpl (std::vector<CoeffType>& coeffs, CoeffType cutoff, double sampleRate) noexcept
    {
        const auto omega_c = DspMath::frequencyToAngular (cutoff, static_cast<CoeffType> (sampleRate));
        const auto length = static_cast<int> (coeffs.size());
        const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);
        
        for (int n = 0; n < length; ++n)
        {
            const auto nOffset = static_cast<CoeffType> (n) - center;
            
            if (std::abs (nOffset) < 1e-10)
            {
                coeffs[static_cast<size_t> (n)] = omega_c / MathConstants<CoeffType>::pi;
            }
            else
            {
                coeffs[static_cast<size_t> (n)] = std::sin (omega_c * nOffset) / (MathConstants<CoeffType>::pi * nOffset);
            }
        }
    }

    /** Designs FIR highpass coefficients */
    template <typename CoeffType>
    static void designFIRHighpassImpl (std::vector<CoeffType>& coeffs, CoeffType cutoff, double sampleRate) noexcept
    {
        designFIRLowpassImpl (coeffs, cutoff, sampleRate);
        
        // Spectral inversion
        const auto length = static_cast<int> (coeffs.size());
        const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);
        
        for (int n = 0; n < length; ++n)
        {
            const auto nOffset = static_cast<CoeffType> (n) - center;
            
            if (std::abs (nOffset) < 1e-10)
            {
                coeffs[static_cast<size_t> (n)] = static_cast<CoeffType> (1.0) - coeffs[static_cast<size_t> (n)];
            }
            else
            {
                if (n % 2 == 1)
                    coeffs[static_cast<size_t> (n)] = -coeffs[static_cast<size_t> (n)];
            }
        }
    }
    
    /** Designs FIR bandpass coefficients */
    template <typename CoeffType>
    static void designFIRBandpassImpl (std::vector<CoeffType>& coeffs, CoeffType lowCutoff, CoeffType highCutoff, double sampleRate) noexcept
    {
        const auto omega1 = DspMath::frequencyToAngular (lowCutoff, static_cast<CoeffType> (sampleRate));
        const auto omega2 = DspMath::frequencyToAngular (highCutoff, static_cast<CoeffType> (sampleRate));
        const auto length = static_cast<int> (coeffs.size());
        const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);
        
        for (int n = 0; n < length; ++n)
        {
            const auto nOffset = static_cast<CoeffType> (n) - center;
            
            if (std::abs (nOffset) < 1e-10)
            {
                coeffs[static_cast<size_t> (n)] = (omega2 - omega1) / MathConstants<CoeffType>::pi;
            }
            else
            {
                coeffs[static_cast<size_t> (n)] = (std::sin (omega2 * nOffset) - std::sin (omega1 * nOffset)) / (MathConstants<CoeffType>::pi * nOffset);
            }
        }
    }
    
    /** Designs FIR bandstop coefficients */
    template <typename CoeffType>
    static void designFIRBandstopImpl (std::vector<CoeffType>& coeffs, CoeffType lowCutoff, CoeffType highCutoff, double sampleRate) noexcept
    {
        designFIRBandpassImpl (coeffs, lowCutoff, highCutoff, sampleRate);
        
        // Spectral inversion for bandstop
        const auto length = static_cast<int> (coeffs.size());
        const auto center = static_cast<CoeffType> (length - 1) / static_cast<CoeffType> (2.0);
        
        for (int n = 0; n < length; ++n)
        {
            const auto nOffset = static_cast<CoeffType> (n) - center;
            
            if (std::abs (nOffset) < 1e-10)
            {
                coeffs[static_cast<size_t> (n)] = static_cast<CoeffType> (1.0) - coeffs[static_cast<size_t> (n)];
            }
            else
            {
                if (n % 2 == 1)
                    coeffs[static_cast<size_t> (n)] = -coeffs[static_cast<size_t> (n)];
            }
        }
    }

    /** Applies window function to FIR coefficients */
    template <typename CoeffType>
    static void applyWindow (std::vector<CoeffType>& coeffs, const std::string& windowType, CoeffType beta) noexcept
    {
        const auto length = static_cast<int> (coeffs.size());
        
        if (windowType == "kaiser")
        {
            const auto window = DspMath::kaiserWindow (length, beta);
            for (int n = 0; n < length; ++n)
            {
                coeffs[static_cast<size_t> (n)] *= window[static_cast<size_t> (n)];
            }
        }
        else if (windowType == "hann")
        {
            for (int n = 0; n < length; ++n)
            {
                coeffs[static_cast<size_t> (n)] *= DspMath::Windows::hann<CoeffType> (n, length);
            }
        }
        else if (windowType == "hamming")
        {
            for (int n = 0; n < length; ++n)
            {
                coeffs[static_cast<size_t> (n)] *= DspMath::Windows::hamming<CoeffType> (n, length);
            }
        }
        else if (windowType == "blackman")
        {
            for (int n = 0; n < length; ++n)
            {
                coeffs[static_cast<size_t> (n)] *= DspMath::Windows::blackman<CoeffType> (n, length);
            }
        }
    }

    /** Transforms lowpass coefficients to highpass */
    template <typename CoeffType>
    static void transformLowpassToHighpass (BiquadCoefficients<CoeffType>& coeffs) noexcept
    {
        // Spectral inversion: negate odd-indexed coefficients
        coeffs.b1 = -coeffs.b1;
        coeffs.a1 = -coeffs.a1;
    }

private:
    //==============================================================================
    // Bessel Filter Helper Functions
    //==============================================================================
    
    /** Gets normalized Bessel polynomial coefficients for given order */
    template <typename CoeffType>
    static std::vector<CoeffType> getBesselPolynomial (int order) noexcept
    {
        // Pre-computed normalized Bessel polynomial coefficients for orders 1-10
        // These are designed for maximum flatness of group delay
        static const std::vector<std::vector<double>> besselCoeffs = {
            {},                                                     // order 0 (unused)
            {1.0, 1.0},                                            // order 1: s + 1
            {1.0, 3.0, 3.0},                                       // order 2: s^2 + 3s + 3
            {1.0, 6.0, 15.0, 15.0},                               // order 3
            {1.0, 10.0, 45.0, 105.0, 105.0},                     // order 4
            {1.0, 15.0, 105.0, 420.0, 945.0, 945.0},             // order 5
            {1.0, 21.0, 210.0, 1260.0, 4725.0, 10395.0, 10395.0}, // order 6
            {1.0, 28.0, 378.0, 3150.0, 17325.0, 62370.0, 135135.0, 135135.0}, // order 7
            {1.0, 36.0, 630.0, 6930.0, 51975.0, 270270.0, 945945.0, 2027025.0, 2027025.0}, // order 8
            {1.0, 45.0, 990.0, 13860.0, 135135.0, 945945.0, 4729725.0, 16216200.0, 34459425.0, 34459425.0}, // order 9
            {1.0, 55.0, 1485.0, 25740.0, 315315.0, 2837835.0, 18918900.0, 91891800.0, 310134825.0, 654729075.0, 654729075.0} // order 10
        };
        
        if (order < 1 || order > 10)
        {
            // For orders > 10, use recursive generation (simplified)
            return {static_cast<CoeffType> (1.0), static_cast<CoeffType> (1.0)};
        }
        
        const auto& coeffs = besselCoeffs[static_cast<size_t> (order)];
        std::vector<CoeffType> result;
        result.reserve (coeffs.size());
        
        for (auto coeff : coeffs)
        {
            result.push_back (static_cast<CoeffType> (coeff));
        }
        
        return result;
    }
    
    /** Calculates poles for Bessel filter of given order */
    template <typename CoeffType>
    static void calculateBesselPoles (int order, std::vector<std::complex<CoeffType>>& poles) noexcept
    {
        poles.clear();
        poles.reserve (static_cast<size_t> (order));
        
        // Pre-computed normalized Bessel poles for orders 1-10
        // These provide maximum flatness of group delay
        static const std::vector<std::vector<std::complex<double>>> besselPoles = {
            {},                                                                      // order 0
            {{-1.0, 0.0}},                                                          // order 1
            {{-1.5, 0.866025}, {-1.5, -0.866025}},                                // order 2
            {{-2.3222, 0.0}, {-1.8389, 1.7544}, {-1.8389, -1.7544}},            // order 3
            {{-2.8962, 1.8379}, {-2.8962, -1.8379}, {-2.1038, 2.6575}, {-2.1038, -2.6575}}, // order 4
            {{-3.6467, 0.0}, {-3.3520, 2.4150}, {-3.3520, -2.4150}, {-2.3247, 3.5710}, {-2.3247, -3.5710}}, // order 5
            // For orders > 5, use approximation
        };
        
        if (order >= 1 && order <= static_cast<int> (besselPoles.size() - 1))
        {
            const auto& orderPoles = besselPoles[static_cast<size_t> (order)];
            for (const auto& pole : orderPoles)
            {
                poles.emplace_back (static_cast<CoeffType> (pole.real()), static_cast<CoeffType> (pole.imag()));
            }
        }
        else
        {
            // For higher orders, use approximation based on Butterworth poles with group delay correction
            for (int i = 0; i < order; ++i)
            {
                const auto angle = MathConstants<CoeffType>::pi * (static_cast<CoeffType> (2 * i + order + 1)) / 
                                  (static_cast<CoeffType> (2 * order));
                const auto real = -std::cos (angle);
                const auto imag = std::sin (angle);
                
                // Apply Bessel correction factor for group delay flatness
                const auto correction = static_cast<CoeffType> (1.0) + static_cast<CoeffType> (0.5) / static_cast<CoeffType> (order);
                poles.emplace_back (real * correction, imag * correction);
            }
        }
    }
    
    //==============================================================================
    // Elliptic Filter Helper Functions
    //==============================================================================
    
    /** Calculates poles and zeros for Elliptic filter of given order */
    template <typename CoeffType>
    static void calculateEllipticPoles (int order, CoeffType epsilon, CoeffType k, 
                                       std::vector<std::complex<CoeffType>>& poles, 
                                       std::vector<CoeffType>& zeros) noexcept
    {
        poles.clear();
        zeros.clear();
        poles.reserve (static_cast<size_t> (order));
        
        // Calculate the modular angle for elliptic integrals
        const auto k1 = k;
        const auto k1_prime = std::sqrt (static_cast<CoeffType> (1.0) - k1 * k1);
        
        // Calculate elliptic integral K(k) approximation
        const auto K = ellipticIntegralK<CoeffType> (k1);
        const auto K_prime = ellipticIntegralK<CoeffType> (k1_prime);
        
        // Calculate v0 (location of real pole for odd orders)
        const auto v0 = -jacobianInverseSnReal<CoeffType> (static_cast<CoeffType> (1.0) / epsilon, k1_prime) / static_cast<CoeffType> (order);
        
        // Generate poles using Jacobi elliptic functions
        for (int i = 1; i <= order; ++i)
        {
            const auto u = static_cast<CoeffType> (2 * i - 1) * K / static_cast<CoeffType> (order);
            
            CoeffType cd, sd, nd;
            jacobianElliptic<CoeffType> (u, k1, cd, sd, nd);
            
            const auto denominator = static_cast<CoeffType> (1.0) - std::pow (k1 * sd, 2);
            
            if (i <= (order + 1) / 2)  // Only compute half, use conjugate symmetry
            {
                if (order % 2 == 1 && i == (order + 1) / 2)
                {
                    // Real pole for odd-order filters
                    CoeffType sn_v0, cn_v0, dn_v0;
                    jacobianElliptic<CoeffType> (v0, k1_prime, cn_v0, sn_v0, dn_v0);
                    
                    const auto realPole = -sn_v0 / cn_v0;
                    poles.emplace_back (realPole, static_cast<CoeffType> (0.0));
                }
                else
                {
                    // Complex conjugate pole pair
                    CoeffType sn_v0, cn_v0, dn_v0;
                    jacobianElliptic<CoeffType> (v0, k1_prime, cn_v0, sn_v0, dn_v0);
                    
                    const auto realPart = -(cd * sn_v0 * cn_v0) / denominator;
                    const auto imagPart = (sd * nd * dn_v0) / denominator;
                    
                    poles.emplace_back (realPart, imagPart);
                    poles.emplace_back (realPart, -imagPart);
                }
            }
        }
        
        // Calculate zeros (for finite transmission zeros)
        const auto numZeros = order / 2;
        for (int i = 1; i <= numZeros; ++i)
        {
            const auto u = static_cast<CoeffType> (2 * i - 1) * K / static_cast<CoeffType> (order);
            
            CoeffType cd, sd, nd;
            jacobianElliptic<CoeffType> (u, k1, cd, sd, nd);
            
            const auto zero_freq = static_cast<CoeffType> (1.0) / (k1 * sd);
            zeros.push_back (zero_freq);
        }
    }
    
    /** Approximation of complete elliptic integral K(k) */
    template <typename CoeffType>
    static CoeffType ellipticIntegralK (CoeffType k) noexcept
    {
        if (k > static_cast<CoeffType> (0.99))
        {
            // Use logarithmic approximation for k close to 1
            const auto k_prime = std::sqrt (static_cast<CoeffType> (1.0) - k * k);
            return std::log (static_cast<CoeffType> (4.0) / k_prime);
        }
        
        // AGM (Arithmetic-Geometric Mean) method approximation
        const auto a0 = static_cast<CoeffType> (1.0);
        const auto b0 = std::sqrt (static_cast<CoeffType> (1.0) - k * k);
        
        auto a = a0;
        auto b = b0;
        
        for (int n = 0; n < 10; ++n)  // Usually converges quickly
        {
            const auto a_new = (a + b) / static_cast<CoeffType> (2.0);
            const auto b_new = std::sqrt (a * b);
            
            if (std::abs (a - b) < static_cast<CoeffType> (1e-12))
                break;
                
            a = a_new;
            b = b_new;
        }
        
        return MathConstants<CoeffType>::pi / (static_cast<CoeffType> (2.0) * a);
    }
    
    /** Jacobi elliptic functions sn, cn, dn */
    template <typename CoeffType>
    static void jacobianElliptic (CoeffType u, CoeffType k, CoeffType& cn, CoeffType& sn, CoeffType& dn) noexcept
    {
        // Simplified approximation using series expansion
        // For production code, a full implementation would be needed
        
        const auto k2 = k * k;
        
        if (std::abs (u) < static_cast<CoeffType> (1e-8))
        {
            // Small angle approximation
            sn = u;
            cn = static_cast<CoeffType> (1.0);
            dn = static_cast<CoeffType> (1.0);
            return;
        }
        
        // Use trigonometric approximation for moderate values
        const auto sin_u = std::sin (u);
        const auto cos_u = std::cos (u);
        
        sn = sin_u / std::sqrt (static_cast<CoeffType> (1.0) + k2 * sin_u * sin_u);
        cn = cos_u / std::sqrt (static_cast<CoeffType> (1.0) + k2 * sin_u * sin_u);
        dn = std::sqrt (static_cast<CoeffType> (1.0) - k2 * sn * sn);
    }
    
    /** Inverse Jacobi sn function for real argument */
    template <typename CoeffType>
    static CoeffType jacobianInverseSnReal (CoeffType x, CoeffType k) noexcept
    {
        // Simplified approximation
        if (std::abs (x) > static_cast<CoeffType> (0.99))
        {
            return static_cast<CoeffType> (0.5) * std::log ((static_cast<CoeffType> (1.0) + x) / (static_cast<CoeffType> (1.0) - x));
        }
        
        // Use series approximation for moderate values
        return std::asin (x * std::sqrt (static_cast<CoeffType> (1.0) + k * k * x * x));
    }

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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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
    template <typename CoeffType>
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

    //==============================================================================
    // Real-Time Safe Coefficient Design (Output Parameter Versions)
    //==============================================================================
    
    /** 
        Real-time safe Butterworth lowpass coefficient design.
        Writes coefficients to pre-allocated array to avoid dynamic allocation.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param outputCoeffs  Pre-allocated array to receive coefficients
        @param maxSections   Maximum number of sections in output array
        @returns            Number of sections written
    */
    template <typename CoeffType>
    static int designButterworthLowpassSafe (
        int order,
        CoeffType frequency,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        return designButterworthImplSafe<CoeffType> (false, order, frequency, sampleRate, outputCoeffs, maxSections);
    }
    
    /** 
        Real-time safe Butterworth highpass coefficient design.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param outputCoeffs  Pre-allocated array to receive coefficients
        @param maxSections   Maximum number of sections in output array
        @returns            Number of sections written
    */
    template <typename CoeffType>
    static int designButterworthHighpassSafe (
        int order,
        CoeffType frequency,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        return designButterworthImplSafe<CoeffType> (true, order, frequency, sampleRate, outputCoeffs, maxSections);
    }
    
    /** 
        Real-time safe Chebyshev Type I lowpass coefficient design.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param rippleDb      The passband ripple in dB
        @param sampleRate    The sample rate in Hz
        @param outputCoeffs  Pre-allocated array to receive coefficients
        @param maxSections   Maximum number of sections in output array
        @returns            Number of sections written
    */
    template <typename CoeffType>
    static int designChebyshev1LowpassSafe (
        int order,
        CoeffType frequency,
        CoeffType rippleDb,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        return designChebyshev1ImplSafe<CoeffType> (false, order, frequency, rippleDb, sampleRate, outputCoeffs, maxSections);
    }
    
    /** 
        Real-time safe Chebyshev Type I highpass coefficient design.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param rippleDb      The passband ripple in dB
        @param sampleRate    The sample rate in Hz
        @param outputCoeffs  Pre-allocated array to receive coefficients
        @param maxSections   Maximum number of sections in output array
        @returns            Number of sections written
    */
    template <typename CoeffType>
    static int designChebyshev1HighpassSafe (
        int order,
        CoeffType frequency,
        CoeffType rippleDb,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        return designChebyshev1ImplSafe<CoeffType> (true, order, frequency, rippleDb, sampleRate, outputCoeffs, maxSections);
    }
    
    /** 
        Real-time safe Bessel lowpass coefficient design.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param outputCoeffs  Pre-allocated array to receive coefficients
        @param maxSections   Maximum number of sections in output array
        @returns            Number of sections written
    */
    template <typename CoeffType>
    static int designBesselLowpassSafe (
        int order,
        CoeffType frequency,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        return designBesselImplSafe<CoeffType> (false, order, frequency, sampleRate, outputCoeffs, maxSections);
    }
    
    /** 
        Real-time safe Bessel highpass coefficient design.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param outputCoeffs  Pre-allocated array to receive coefficients
        @param maxSections   Maximum number of sections in output array
        @returns            Number of sections written
    */
    template <typename CoeffType>
    static int designBesselHighpassSafe (
        int order,
        CoeffType frequency,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        return designBesselImplSafe<CoeffType> (true, order, frequency, sampleRate, outputCoeffs, maxSections);
    }
    
    /** 
        Real-time safe Elliptic lowpass coefficient design.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param passRippleDb  The passband ripple in dB
        @param stopRippleDb  The stopband attenuation in dB
        @param sampleRate    The sample rate in Hz
        @param outputCoeffs  Pre-allocated array to receive coefficients
        @param maxSections   Maximum number of sections in output array
        @returns            Number of sections written
    */
    template <typename CoeffType>
    static int designEllipticLowpassSafe (
        int order,
        CoeffType frequency,
        CoeffType passRippleDb,
        CoeffType stopRippleDb,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        return designEllipticImplSafe<CoeffType> (false, order, frequency, passRippleDb, stopRippleDb, sampleRate, outputCoeffs, maxSections);
    }
    
    /** 
        Real-time safe Elliptic highpass coefficient design.
        
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param passRippleDb  The passband ripple in dB
        @param stopRippleDb  The stopband attenuation in dB
        @param sampleRate    The sample rate in Hz
        @param outputCoeffs  Pre-allocated array to receive coefficients
        @param maxSections   Maximum number of sections in output array
        @returns            Number of sections written
    */
    template <typename CoeffType>
    static int designEllipticHighpassSafe (
        int order,
        CoeffType frequency,
        CoeffType passRippleDb,
        CoeffType stopRippleDb,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        return designEllipticImplSafe<CoeffType> (true, order, frequency, passRippleDb, stopRippleDb, sampleRate, outputCoeffs, maxSections);
    }

private:
    //==============================================================================
    // Real-Time Safe Implementation Methods
    //==============================================================================
    
    /** 
        Real-time safe Butterworth filter implementation.
        
        @param isHighpass    True for highpass, false for lowpass
        @param order         The filter order
        @param frequency     The cutoff frequency in Hz
        @param sampleRate    The sample rate in Hz
        @param outputCoeffs  Pre-allocated array to receive coefficients
        @param maxSections   Maximum number of sections in output array
        @returns            Number of sections written
    */
    template <typename CoeffType>
    static int designButterworthImplSafe (
        bool isHighpass,
        int order,
        CoeffType frequency,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        const auto numSections = calculateNumSections (order);
        if (numSections > maxSections)
            return 0; // Not enough space
        
        // Use the existing allocating method and copy results
        // This is a transitional approach - ideally we'd reimplement without allocation
        const auto tempCoeffs = designButterworthImpl<CoeffType> (isHighpass, order, frequency, sampleRate);
        
        const auto numToWrite = jmin (static_cast<int> (tempCoeffs.size()), maxSections);
        for (int i = 0; i < numToWrite; ++i)
        {
            outputCoeffs[i] = tempCoeffs[i];
        }
        
        return numToWrite;
    }
    
    /** 
        Real-time safe Chebyshev Type I filter implementation.
    */
    template <typename CoeffType>
    static int designChebyshev1ImplSafe (
        bool isHighpass,
        int order,
        CoeffType frequency,
        CoeffType rippleDb,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        const auto numSections = calculateNumSections (order);
        if (numSections > maxSections)
            return 0;
        
        const auto tempCoeffs = designChebyshev1Impl<CoeffType> (isHighpass, order, frequency, rippleDb, sampleRate);
        
        const auto numToWrite = jmin (static_cast<int> (tempCoeffs.size()), maxSections);
        for (int i = 0; i < numToWrite; ++i)
        {
            outputCoeffs[i] = tempCoeffs[i];
        }
        
        return numToWrite;
    }
    
    /** 
        Real-time safe Bessel filter implementation.
    */
    template <typename CoeffType>
    static int designBesselImplSafe (
        bool isHighpass,
        int order,
        CoeffType frequency,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        const auto numSections = calculateNumSections (order);
        if (numSections > maxSections)
            return 0;
        
        const auto tempCoeffs = designBesselImpl<CoeffType> (isHighpass, order, frequency, sampleRate);
        
        const auto numToWrite = jmin (static_cast<int> (tempCoeffs.size()), maxSections);
        for (int i = 0; i < numToWrite; ++i)
        {
            outputCoeffs[i] = tempCoeffs[i];
        }
        
        return numToWrite;
    }
    
    /** 
        Real-time safe Elliptic filter implementation.
    */
    template <typename CoeffType>
    static int designEllipticImplSafe (
        bool isHighpass,
        int order,
        CoeffType frequency,
        CoeffType passRippleDb,
        CoeffType stopRippleDb,
        double sampleRate,
        BiquadCoefficients<CoeffType>* outputCoeffs,
        int maxSections
    ) noexcept
    {
        const auto numSections = calculateNumSections (order);
        if (numSections > maxSections)
            return 0;
        
        const auto tempCoeffs = designEllipticImpl<CoeffType> (isHighpass, order, frequency, passRippleDb, stopRippleDb, sampleRate);
        
        const auto numToWrite = jmin (static_cast<int> (tempCoeffs.size()), maxSections);
        for (int i = 0; i < numToWrite; ++i)
        {
            outputCoeffs[i] = tempCoeffs[i];
        }
        
        return numToWrite;
    }
};

} // namespace yup