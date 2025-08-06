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
    Soft clipper audio processor.

    This class implements a smooth saturation/clipping algorithm that prevents
    hard clipping by gradually compressing signals as they approach the maximum
    amplitude. The algorithm uses a hyperbolic curve to smoothly transition
    from linear to compressed regions.

    The soft clipping formula applied when signal exceeds threshold:
    - For positive signals: output = maxAmplitude - (A / (B + input))
    - For negative signals: output = -(maxAmplitude - (A / (B - input)))

    Where:
    - A = (maxAmplitude - clipThreshold)²
    - B = maxAmplitude - 2 * clipThreshold
    - clipThreshold = maxAmplitude * amount

    @tparam SampleType  The type of audio samples (float or double)
    @tparam CoeffType   The type for internal calculations (defaults to double)
*/
template <typename SampleType, typename CoeffType = double>
class SoftClipper
{
public:
    //==============================================================================
    /** Constructor with default parameters.

        @param maxAmplitude  The maximum output amplitude (default: 1.0)
        @param amount        The soft clip amount between 0-1 (default: 0.85)
                            Lower values = earlier/softer clipping
                            Higher values = later/harder clipping
    */
    SoftClipper (CoeffType maxAmplitude = static_cast<CoeffType> (1.0),
                 CoeffType amount = static_cast<CoeffType> (0.85)) noexcept
        : maxAmp (maxAmplitude)
        , clipAmount (amount)
    {
        updateCoefficients();
    }

    //==============================================================================
    /** Sets the maximum amplitude.

        @param newMaxAmplitude  The new maximum amplitude (typically 1.0)
    */
    void setMaxAmplitude (CoeffType newMaxAmplitude) noexcept
    {
        maxAmp = newMaxAmplitude;
        updateCoefficients();
    }

    /** Returns the current maximum amplitude. */
    CoeffType getMaxAmplitude() const noexcept
    {
        return maxAmp;
    }

    /** Sets the soft clipping amount.

        @param newAmount  The amount between 0-1 (0 = max softness, 1 = hardest)
    */
    void setAmount (CoeffType newAmount) noexcept
    {
        clipAmount = jlimit (static_cast<CoeffType> (0), static_cast<CoeffType> (1), newAmount);
        updateCoefficients();
    }

    /** Returns the current soft clipping amount. */
    CoeffType getAmount() const noexcept
    {
        return clipAmount;
    }

    /** Sets both parameters at once.

        @param newMaxAmplitude  The new maximum amplitude
        @param newAmount        The new soft clip amount (0-1)
    */
    void setParameters (CoeffType newMaxAmplitude, CoeffType newAmount) noexcept
    {
        maxAmp = newMaxAmplitude;
        clipAmount = jlimit (static_cast<CoeffType> (0), static_cast<CoeffType> (1), newAmount);
        updateCoefficients();
    }

    //==============================================================================
    /** Resets the processor state (no-op for this stateless processor). */
    void reset() noexcept
    {
        // Stateless processor - nothing to reset
    }

    /** Prepares the processor (no-op for this stateless processor).

        @param sampleRate        The sample rate (unused)
        @param maximumBlockSize  The maximum block size (unused)
    */
    void prepare (double /*sampleRate*/, int /*maximumBlockSize*/) noexcept
    {
        // Stateless processor - nothing to prepare
    }

    //==============================================================================
    /** Processes a single sample.

        @param inputSample  The input sample to process
        @returns           The soft-clipped output sample
    */
    SampleType processSample (SampleType inputSample) noexcept
    {
        const auto input = static_cast<CoeffType> (inputSample);

        if (input > clipThreshold)
        {
            const auto output = maxAmp - (clipA / (clipB + input));
            return static_cast<SampleType> (preventDenormal (output));
        }
        else if (input < -clipThreshold)
        {
            const auto output = -(maxAmp - (clipA / (clipB - input)));
            return static_cast<SampleType> (preventDenormal (output));
        }

        return inputSample;
    }

    /** Processes a block of samples.

        @param inputBuffer   Pointer to the input samples
        @param outputBuffer  Pointer to the output buffer
        @param numSamples    Number of samples to process
    */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    /** Processes a block of samples in-place.

        @param buffer      Pointer to the buffer to process
        @param numSamples  Number of samples to process
    */
    void processInPlace (SampleType* buffer, int numSamples) noexcept
    {
        processBlock (buffer, buffer, numSamples);
    }

    //==============================================================================
    /** Returns the clipping threshold. */
    CoeffType getClipThreshold() const noexcept
    {
        return clipThreshold;
    }

private:
    //==============================================================================
    /** Updates internal coefficients when parameters change. */
    void updateCoefficients() noexcept
    {
        clipThreshold = maxAmp * clipAmount;
        const auto diff = maxAmp - clipThreshold;
        clipA = diff * diff;
        clipB = maxAmp - static_cast<CoeffType> (2) * clipThreshold;
    }

    /** Prevents denormal numbers. */
    static CoeffType preventDenormal (CoeffType value) noexcept
    {
        const CoeffType denormalThreshold = std::numeric_limits<CoeffType>::min();
        return (std::abs (value) < denormalThreshold) ? static_cast<CoeffType> (0) : value;
    }

    //==============================================================================
    CoeffType maxAmp = static_cast<CoeffType> (1);
    CoeffType clipAmount = static_cast<CoeffType> (0.85);
    CoeffType clipThreshold = static_cast<CoeffType> (0.85);
    CoeffType clipA = static_cast<CoeffType> (0.0225);  // (1 - 0.85)^2
    CoeffType clipB = static_cast<CoeffType> (-0.7);    // 1 - 2*0.85

    //==============================================================================
    YUP_LEAK_DETECTOR (SoftClipper)
};

//==============================================================================
/** Type aliases for convenience */
using SoftClipperFloat = SoftClipper<float>;
using SoftClipperDouble = SoftClipper<double>;

} // namespace yup