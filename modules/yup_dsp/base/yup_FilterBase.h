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
/** Common filter modes enumeration */
enum class FilterMode
{
    lowpass,      /**< Low-pass filter */
    highpass,     /**< High-pass filter */
    bandpass,     /**< Band-pass filter */
    bandstop,     /**< Band-stop (notch) filter */
    peak,         /**< Peaking filter */
    lowshelf,     /**< Low-shelf filter */
    highshelf,    /**< High-shelf filter */
    allpass       /**< All-pass filter */
};

//==============================================================================
/**
    Base interface for all digital filters.

    Provides a common interface for filter processing with both per-sample
    and block processing capabilities. Uses dual-precision architecture:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal coefficients (defaults to double for precision)

    All concrete filter implementations should inherit from this class.

    @tparam SampleType  Type for audio samples (float or double)
    @tparam CoeffType   Type for internal coefficients (defaults to double)

    @see Biquad, StateVariableFilter, FirstOrderFilter
*/
template <typename SampleType, typename CoeffType = double>
class FilterBase
{
public:
    //==============================================================================
    /** Default constructor */
    FilterBase() = default;

    /** Virtual destructor */
    virtual ~FilterBase() = default;

    //==============================================================================
    /** Resets the filter's internal state to zero */
    virtual void reset() noexcept = 0;

    /**
        Prepares the filter for processing with the given sample rate and block size.

        @param sampleRate     The sample rate in Hz
        @param maximumBlockSize  The maximum number of samples that will be processed at once
    */
    virtual void prepare (double sampleRate, int maximumBlockSize) noexcept = 0;

    /**
        Processes a single sample.

        @param inputSample  The input sample to process
        @returns           The filtered output sample
    */
    virtual SampleType processSample (SampleType inputSample) noexcept = 0;

    /**
        Processes a block of samples.

        @param inputBuffer   Pointer to the input samples
        @param outputBuffer  Pointer to the output buffer
        @param numSamples    Number of samples to process
    */
    virtual void processBlock (const SampleType* inputBuffer,
                              SampleType* outputBuffer,
                              int numSamples) noexcept = 0;

    /**
        Processes a block of samples in-place.

        @param buffer      Pointer to the buffer containing input samples, will be overwritten with output
        @param numSamples  Number of samples to process
    */
    virtual void processInPlace (SampleType* buffer, int numSamples) noexcept
    {
        processBlock (buffer, buffer, numSamples);
    }

    //==============================================================================
    /**
        Returns the magnitude response at the given frequency.

        @param frequency  The frequency in Hz
        @returns         The magnitude response (linear scale)
    */
    virtual CoeffType getMagnitudeResponse (CoeffType frequency) const noexcept
    {
        auto response = getComplexResponse (frequency);
        return std::abs (response);
    }

    /**
        Returns the phase response at the given frequency.

        @param frequency  The frequency in Hz
        @returns         The phase response in radians
    */
    virtual CoeffType getPhaseResponse (CoeffType frequency) const noexcept
    {
        auto response = getComplexResponse (frequency);
        return std::arg (response);
    }

    /**
        Returns the complex frequency response at the given frequency.

        @param frequency  The frequency in Hz
        @returns         The complex frequency response
    */
    virtual DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency) const noexcept = 0;

    //==============================================================================
    /**
        Returns the poles and zeros of this filter.

        @param poles  The poles.
        @param zeros  The zeros.
    */
    virtual void getPolesZeros (
        std::vector<DspMath::Complex<CoeffType>>& poles,
        std::vector<DspMath::Complex<CoeffType>>& zeros) const
    {
        poles.clear();
        zeros.clear();
    }

protected:
    //==============================================================================
    double sampleRate = 44100.0;
    int maximumBlockSize = 512;

private:
    //==============================================================================
    YUP_LEAK_DETECTOR (FilterBase)
};

//==============================================================================
/** 
    First-order filter coefficient storage.
    
    Stores coefficients for first-order IIR filters in the form:
    y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]
    
    Uses CoeffType for internal precision (default double) while supporting 
    different SampleType for audio processing.
*/
template <typename CoeffType = double>
struct FirstOrderCoefficients
{
    CoeffType a1 = 0;  // Feedback coefficient
    CoeffType b0 = 1, b1 = 0;  // Feedforward coefficients

    FirstOrderCoefficients() = default;

    FirstOrderCoefficients (CoeffType b0_, CoeffType b1_, CoeffType a1_) noexcept
        : a1 (a1_), b0 (b0_), b1 (b1_)
    {
    }

    /** Returns the complex frequency response for these coefficients */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency, double sampleRate) const noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto z = DspMath::polar (static_cast<CoeffType> (1.0), -omega);

        auto numerator = DspMath::Complex<CoeffType> (b0) + DspMath::Complex<CoeffType> (b1) * z;
        auto denominator = DspMath::Complex<CoeffType> (1.0) + DspMath::Complex<CoeffType> (a1) * z;

        return numerator / denominator;
    }
};

//==============================================================================
/**
    Filter coefficient storage for biquad filters.

    Stores the coefficients for a second-order IIR filter in the form:
    y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]

    Uses CoeffType for internal precision (default double) while supporting
    different SampleType for audio processing.
*/
template <typename CoeffType = double>
struct BiquadCoefficients
{
    CoeffType a0 = 1, a1 = 0, a2 = 0;  // Denominator coefficients (a0 is typically normalized to 1)
    CoeffType b0 = 1, b1 = 0, b2 = 0;  // Numerator coefficients

    BiquadCoefficients() = default;

    BiquadCoefficients (CoeffType b0_, CoeffType b1_, CoeffType b2_, CoeffType a0_, CoeffType a1_) noexcept
        : a0 (a0_)
        , a1 (a1_)
        , a2 (0.0f)
        , b0 (b0_)
        , b1 (b1_)
        , b2 (b2_)
    {
    }

    BiquadCoefficients (CoeffType b0_, CoeffType b1_, CoeffType b2_, CoeffType a0_, CoeffType a1_, CoeffType a2_) noexcept
        : a0 (a0_)
        , a1 (a1_)
        , a2 (a2_)
        , b0 (b0_)
        , b1 (b1_)
        , b2 (b2_)
    {
    }

    /** Normalizes coefficients so that a0 = 1 */
    void normalize() noexcept
    {
        if (a0 != static_cast<CoeffType> (0.0))
        {
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
            a1 /= a0;
            a2 /= a0;
            a0 = static_cast<CoeffType> (1.0);
        }
    }

    /** Returns the complex frequency response for these coefficients */
    DspMath::Complex<CoeffType> getComplexResponse (CoeffType frequency, double sampleRate) const noexcept
    {
        const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
        const auto z = DspMath::polar (static_cast<CoeffType> (1.0), -omega);
        const auto z2 = z * z;

        auto numerator = DspMath::Complex<CoeffType> (b0) + DspMath::Complex<CoeffType> (b1) * z + DspMath::Complex<CoeffType> (b2) * z2;
        auto denominator = DspMath::Complex<CoeffType> (a0) + DspMath::Complex<CoeffType> (a1) * z + DspMath::Complex<CoeffType> (a2) * z2;

        return numerator / denominator;
    }
};

//==============================================================================
/**
    Filter coefficient storage for state variable filters.
*/
template <typename CoeffType = double>
struct StateVariableCoefficients
{
    CoeffType k = static_cast<CoeffType> (1.0);
    CoeffType g = static_cast<CoeffType> (1.0);
    CoeffType damping = static_cast<CoeffType> (1.0);

    StateVariableCoefficients() = default;

    StateVariableCoefficients (CoeffType k_, CoeffType g_, CoeffType damping_) noexcept
        : k (k_)
        , g (g_)
        , damping (damping_)
    {
    }
};

} // namespace yup
