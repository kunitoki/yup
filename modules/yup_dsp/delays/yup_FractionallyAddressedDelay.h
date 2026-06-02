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
    Fractionally-Addressed Delay (FAD) line.

    Implements the delay-line architecture proposed by Davide Rocchesso (1999),
    in which a single fractional pointer is used for both reading and writing.
    This gives two practical advantages over the classic FIR (two-pointer) delay:

    1. **Lower frequency-dependent attenuation.** The mean attenuation at 2/3
       Nyquist is ~0.5 dB vs. ~1.2 dB for the linearly-interpolated FIR line.

    2. **Tension-model modulation.** Varying the delay length changes the
       effective propagation speed rather than the physical length, so the pitch
       shift follows exp(−k) rather than (1+k). This avoids the Doppler artifact
       of FIR delay lines, making the FAD well-suited for smooth chorus, flanger,
       and waveguide pitch-bending effects.

    ### Usage

    @code
    yup::FractionallyAddressedDelay<float> fad;
    fad.setMaxDelaySamples (4096);   // allocates the buffer
    fad.setDelaySamples (2048.0f);   // sets the delay (may be fractional)

    for (int i = 0; i < blockSize; ++i)
        output[i] = fad.processSample (input[i]);
    @endcode

    ### Algorithm

    Given buffer size `B` and increment `I = B / delaySamples`:

    @code
    fph       = floor(phase)
    output    = lerp(buf[fph], buf[(fph+1) & mask], frac(phase))    // linear read
    numWrites = (fph - phaseOldInt + B) & mask
    for i in 0..numWrites-1:
        buf[(phaseOldInt+1+i) & mask] = lerp(prevInput, input, (i+1)/numWrites)
    phaseOldInt = fph
    phase      += I;  if phase >= B then phase -= B
    @endcode

    Read uses linear interpolation between adjacent cells. Write uses linear
    interpolation between the previous and current input to fill the gap when
    `I > 1`. When `I < 1` no write is performed and the pointer re-reads existing
    buffer content, naturally implementing delays longer than the buffer size.

    @note `setMaxDelaySamples()` must be called before any processing.

    @tparam SampleType  Type of audio samples (float or double).
    @tparam CoeffType   Type used for internal computation (defaults to double).
*/
template <typename SampleType, typename CoeffType = double>
class FractionallyAddressedDelay
{
public:
    //==============================================================================
    /** Default constructor. Call setMaxDelaySamples() before processing. */
    FractionallyAddressedDelay() noexcept = default;

    //==============================================================================
    /** Allocates the internal buffer to hold at least @p maxDelaySamples cells.

        The actual buffer size is rounded up to the next power of two for
        efficient modular addressing. Resets all internal state.

        @param maxDelaySamples  Maximum number of delay samples required (>= 1).
    */
    void setMaxDelaySamples (int maxDelaySamples)
    {
        jassert (maxDelaySamples >= 1);

        bufferSize = nextPowerOfTwo (jmax (2, maxDelaySamples));
        bufferMask = bufferSize - 1;
        delayBuffer.assign (static_cast<std::size_t> (bufferSize), SampleType {});

        reset();
    }

    /** Returns the allocated buffer size (a power of two >= the requested size). */
    int getBufferSize() const noexcept { return bufferSize; }

    //==============================================================================
    /** Sets the current delay length in samples (may be fractional).

        Internally computes the phase increment `I = bufferSize / delaySamples`.
        A larger delay produces a smaller increment and a slower pointer advance.

        @param newDelaySamples  Desired delay in samples, clamped to a minimum
                                of 1 to prevent a zero or negative increment.
    */
    void setDelaySamples (CoeffType newDelaySamples) noexcept
    {
        const CoeffType clamped = jmax (CoeffType (1), newDelaySamples);
        increment = static_cast<CoeffType> (bufferSize) / clamped;
    }

    /** Returns the current delay in samples derived from the stored increment. */
    CoeffType getDelaySamples() const noexcept
    {
        if (bufferSize == 0 || increment <= CoeffType (0))
            return CoeffType (0);

        return static_cast<CoeffType> (bufferSize) / increment;
    }

    //==============================================================================
    /** No-op: this processor requires no sample-rate-dependent setup. */
    void prepare (double /*sampleRate*/, int /*maximumBlockSize*/) noexcept {}

    /** Clears the delay buffer and resets the phase pointers. */
    void reset() noexcept
    {
        std::fill (delayBuffer.begin(), delayBuffer.end(), SampleType {});
        phase = CoeffType (0);
        phaseOldInt = bufferSize > 0 ? bufferSize - 1 : 0;
        prevInput = SampleType {};
    }

    //==============================================================================
    /** Processes a single sample through the FAD line.

        @param inputSample  The input sample to write into the delay.
        @returns            The linearly-interpolated delayed output sample.
    */
    forcedinline SampleType processSample (SampleType inputSample) noexcept
    {
        const int fph = static_cast<int> (phase) & bufferMask;
        const CoeffType frac = phase - std::floor (phase);

        // Linear read: samples following the phase pointer
        const CoeffType y0 = static_cast<CoeffType> (delayBuffer[static_cast<std::size_t> (fph)]);
        const CoeffType y1 = static_cast<CoeffType> (delayBuffer[static_cast<std::size_t> ((fph + 1) & bufferMask)]);
        const SampleType output = static_cast<SampleType> (y0 + frac * (y1 - y0));

        // Fill buffer cells between the previous and current phase floor
        const int numWrites = (fph - phaseOldInt + bufferSize) & bufferMask;

        if (numWrites > 0)
        {
            const CoeffType prevIn = static_cast<CoeffType> (prevInput);
            const CoeffType currIn = static_cast<CoeffType> (inputSample);
            const CoeffType invWrites = CoeffType (1) / static_cast<CoeffType> (numWrites);

            for (int i = 0; i < numWrites; ++i)
            {
                const CoeffType writeFrac = static_cast<CoeffType> (i + 1) * invWrites;
                const int writeIdx = (phaseOldInt + 1 + i) & bufferMask;
                delayBuffer[static_cast<std::size_t> (writeIdx)] =
                    static_cast<SampleType> (prevIn + writeFrac * (currIn - prevIn));
            }
        }

        phaseOldInt = fph;
        prevInput = inputSample;

        phase += increment;

        if (phase >= static_cast<CoeffType> (bufferSize))
            phase -= static_cast<CoeffType> (bufferSize);

        return output;
    }

    /** Processes a block of samples.

        @param inputBuffer   Pointer to input samples.
        @param outputBuffer  Pointer to the output buffer (must not alias inputBuffer).
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
        for (int i = 0; i < numSamples; ++i)
            buffer[i] = processSample (buffer[i]);
    }

private:
    //==============================================================================
    std::vector<SampleType> delayBuffer;
    int bufferSize = 0;
    int bufferMask = 0;
    CoeffType phase = CoeffType (0);
    int phaseOldInt = 0;
    SampleType prevInput = SampleType {};
    CoeffType increment = CoeffType (1);

    //==============================================================================
    YUP_LEAK_DETECTOR (FractionallyAddressedDelay)
};

//==============================================================================
/** Type aliases for convenience. */
using FractionallyAddressedDelayFloat = FractionallyAddressedDelay<float>;
using FractionallyAddressedDelayDouble = FractionallyAddressedDelay<double>;

} // namespace yup
