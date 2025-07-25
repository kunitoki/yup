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
    lowpass,   /**< Low-pass filter */
    highpass,  /**< High-pass filter */
    bandpass,  /**< Band-pass filter */
    bandstop,  /**< Band-stop (notch) filter */
    peak,      /**< Peaking filter */
    lowshelf,  /**< Low-shelf filter */
    highshelf, /**< High-shelf filter */
    allpass    /**< All-pass filter */
};

//==============================================================================
/**
    Base interface for all digital filters.

    Provides a common interface for filter processing with both per-sample and block processing capabilities.

    Uses dual-precision architecture:
    - SampleType: for audio buffer processing (float/double)
    - CoeffType: for internal coefficients (defaults to double for precision)

    All concrete filter implementations should inherit from this class.

    @tparam SampleType  Type for audio samples (float or double)
    @tparam CoeffType   Type for internal coefficients (defaults to double)

    @see Biquad, FirstOrder
*/
template <typename SampleType, typename CoeffType = double>
class FilterBase
{
public:
    //==============================================================================
    using SamplesType = SampleType;
    using CoefficientTypes = CoeffType;

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
    virtual void prepare (double sampleRate, int maximumBlockSize) = 0;

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
    virtual CoeffType getMagnitudeResponse (CoeffType frequency) const
    {
        auto response = getComplexResponse (frequency);
        return std::abs (response);
    }

    /**
        Returns the phase response at the given frequency.

        @param frequency  The frequency in Hz
        @returns         The phase response in radians
    */
    virtual CoeffType getPhaseResponse (CoeffType frequency) const
    {
        auto response = getComplexResponse (frequency);
        return std::arg (response);
    }

    /**
        Returns the complex frequency response at the given frequency.

        @param frequency  The frequency in Hz
        @returns         The complex frequency response
    */
    virtual Complex<CoeffType> getComplexResponse (CoeffType frequency) const = 0;

    //==============================================================================
    /**
        Returns the poles and zeros of this filter.

        @param poles  The poles.
        @param zeros  The zeros.
    */
    virtual void getPolesZeros (
        ComplexVector<CoeffType>& poles,
        ComplexVector<CoeffType>& zeros) const
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

} // namespace yup
