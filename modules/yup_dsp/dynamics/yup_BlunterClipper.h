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
    Blunter soft clipper based on a quadratic bounded clipping curve.

    Based on the research by Lorenzo Fiestas ("Quantifying Clipping Softness"),
    the Blunter was experimentally found to minimize peak second derivative
    (maximize softness) among the generated symmetric bounded clippers for the
    normalization range studied in the paper.

    The canonical Blunter formula on the unit domain:

    @code
    B(x) = x * (2 - |x|)   for |x| <= 1
    B(x) = sign(x)          for |x| >  1
    @endcode

    This gives a constant |B″(x)| = 2 across the entire knee region, spreading
    the transition evenly compared to tanh or hyperbolic clippers that
    concentrate distortion near the clip point.

    The processSample function computes:  B(x * inputGain) * outputGain

    - inputGain (A_in) controls the amount of distortion (overdrive). Values
      above 1.0 push the signal further into the non-linear region.
    - outputGain (A_out) compensates the volume after clipping.

    Note: B′(0) = 2, so signals well below the clip point are amplified by 2×.
    Set outputGain = 0.5 for unity gain at the origin if needed.

    Gain setters are not synchronized. Do not call them concurrently with
    processSample() or processBlock(); update parameters between processing
    passes, or wrap this class in a thread-safe/smoothed parameter layer.

    @tparam SampleType  The type of audio samples (float or double)
    @tparam CoeffType   The type used for internal calculations (defaults to double)
*/
template <typename SampleType, typename CoeffType = double>
class BlunterClipper
{
public:
    //==============================================================================
    /** Constructs a BlunterClipper with the given input and output gains.

        @param newInputGain   Scales the input before clipping (default: 1.0).
                              Higher values push more signal into the non-linear
                              region and produce more harmonic distortion.
        @param newOutputGain  Scales the output after clipping (default: 1.0).
                              Use 0.5 to restore unity gain at the origin
                              (since B′(0) = 2).
    */
    BlunterClipper (CoeffType newInputGain = static_cast<CoeffType> (1),
                    CoeffType newOutputGain = static_cast<CoeffType> (1)) noexcept
        : inputGain (sanitizeInputGain (newInputGain))
        , outputGain (sanitizeOutputGain (newOutputGain))
    {
    }

    //==============================================================================
    /** Sets the input gain (A_in), which controls distortion amount.

        @param newInputGain  The new input gain. Values less than or equal to
                             zero are clamped to the smallest positive value
                             representable by CoeffType.
    */
    void setInputGain (CoeffType newInputGain) noexcept
    {
        inputGain = sanitizeInputGain (newInputGain);
    }

    /** Returns the current input gain. */
    CoeffType getInputGain() const noexcept
    {
        return inputGain;
    }

    /** Sets the output gain (A_out), which compensates the volume after clipping.

        @param newOutputGain  The new output gain. Negative values are clamped
                              to zero to preserve monotonicity.
    */
    void setOutputGain (CoeffType newOutputGain) noexcept
    {
        outputGain = sanitizeOutputGain (newOutputGain);
    }

    /** Returns the current output gain. */
    CoeffType getOutputGain() const noexcept
    {
        return outputGain;
    }

    /** Sets both gains at once.

        @param newInputGain   The new input gain.
        @param newOutputGain  The new output gain.
    */
    void setParameters (CoeffType newInputGain, CoeffType newOutputGain) noexcept
    {
        inputGain = sanitizeInputGain (newInputGain);
        outputGain = sanitizeOutputGain (newOutputGain);
    }

    //==============================================================================
    /** No-op: this processor is stateless. */
    void reset() noexcept {}

    /** No-op: this processor is stateless.

        @param sampleRate        Unused.
        @param maximumBlockSize  Unused.
    */
    void prepare (double /*sampleRate*/, int /*maximumBlockSize*/) noexcept {}

    //==============================================================================
    /** Processes a single sample through the Blunter clipper.

        Computes: B(inputSample * inputGain) * outputGain

        @param inputSample  The input sample to process.
        @returns            The soft-clipped output sample.
    */
    SampleType processSample (SampleType inputSample) noexcept
    {
        const auto x = static_cast<CoeffType> (inputSample) * inputGain;
        return static_cast<SampleType> (blunter (x) * outputGain);
    }

    /** Processes a block of samples.

        @param inputBuffer   Pointer to the input samples.
        @param outputBuffer  Pointer to the output buffer.
        @param numSamples    Number of samples to process.
    */
    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    /** Processes a block of samples in-place.

        @param buffer      Pointer to the buffer to process in-place.
        @param numSamples  Number of samples to process.
    */
    void processInPlace (SampleType* buffer, int numSamples) noexcept
    {
        processBlock (buffer, buffer, numSamples);
    }

private:
    //==============================================================================
    /** Applies the canonical Blunter function: B(x) = x*(2-|x|), clamped to [-1, 1]. */
    static CoeffType blunter (CoeffType x) noexcept
    {
        if (x >= static_cast<CoeffType> (1))
            return static_cast<CoeffType> (1);

        if (x <= static_cast<CoeffType> (-1))
            return static_cast<CoeffType> (-1);

        return x * (static_cast<CoeffType> (2) - std::abs (x));
    }

    static CoeffType sanitizeInputGain (CoeffType value) noexcept
    {
        const auto minimumGain = std::numeric_limits<CoeffType>::epsilon();
        return value > minimumGain ? value : minimumGain;
    }

    static CoeffType sanitizeOutputGain (CoeffType value) noexcept
    {
        return value > static_cast<CoeffType> (0) ? value : static_cast<CoeffType> (0);
    }

    //==============================================================================
    CoeffType inputGain = static_cast<CoeffType> (1);
    CoeffType outputGain = static_cast<CoeffType> (1);

    //==============================================================================
    YUP_LEAK_DETECTOR (BlunterClipper)
};

//==============================================================================
/** Type aliases for convenience. */
using BlunterClipperFloat = BlunterClipper<float>;
using BlunterClipperDouble = BlunterClipper<double>;

} // namespace yup
