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
    Multi-channel integer-factor oversampler using windowed sinc interpolation.

    Oversampler up- and downsamples audio by an integer factor with
    bandlimited interpolation and anti-aliasing.  Internal per-channel history
    buffers allow seamless multi-block (real-time) operation.

    Typical usage in an audio effect:
    @code
    yup::Oversampler<float, 4, 8> os;
    os.prepare (44100.0, 2, 512);

    // Inside your audio callback:
    os.upsample (inputPtrs, numChannels, numSamples);
    os.processOversampledBlock ([&] (auto& buf)
    {
        applyDistortion (buf);  // buf is std::vector<std::vector<float>>&
    });
    os.downsample (outputPtrs, numChannels, numSamples);
    @endcode

    @tparam SampleType       Audio sample type (float or double).
    @tparam OversampleFactor Integer upsample ratio (2, 4, 8, …).
    @tparam SincRadius       Half-width of the sinc kernel in original-rate samples.
                             Higher values give better stopband rejection at the
                             cost of more computation.
    @tparam CoeffType        Precision for internal filter coefficients (default double).
*/
template <typename SampleType, int OversampleFactor, int SincRadius, typename CoeffType = double>
class Oversampler
{
public:
    static_assert (OversampleFactor >= 2, "OversampleFactor must be at least 2");
    static_assert (SincRadius >= 1, "SincRadius must be at least 1");

    //==============================================================================
    /** Default constructor. Call prepare() before any processing. */
    Oversampler() = default;

    /** Destructor. */
    ~Oversampler() = default;

    //==============================================================================
    /**
        Prepares the oversampler for processing.

        Configures the internal windowed sinc table and allocates per-channel
        history and staging buffers. Must be called before upsample() or downsample().

        @param sampleRate    Input sample rate in Hz.
        @param maxChannels   Maximum number of audio channels.
        @param maxBlockSize  Maximum input block size in samples.
    */
    void prepare (double sampleRate, int maxChannels, int maxBlockSize)
    {
        jassert (sampleRate > 0.0 && maxChannels > 0 && maxBlockSize > 0);

        sincTable.configure (static_cast<CoeffType> (sampleRate));
        sincTable.applyKaiserWindow (CoeffType (5));

        const int maxInterpolated = maxBlockSize * OversampleFactor;

        interpolBeginBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius> {});
        interpolEndBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius> {});
        decimBeginBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius * OversampleFactor> {});
        decimEndBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius * OversampleFactor> {});

        xInterp.setSize (maxChannels, maxBlockSize + SincRadius);
        xInterp.clear();

        xDecim.setSize (maxChannels, maxInterpolated + SincRadius * OversampleFactor);
        xDecim.clear();

        oversampledBuffer.setSize (maxChannels, maxInterpolated);
        oversampledBuffer.clear();

        currentOversampledSize = 0;
        currentNumChannels = 0;
    }

    /**
        Resets all internal processing state.

        Clears all history buffers so that a fresh processing session can begin
        without artifacts from a previous session. Filter coefficients are
        preserved; there is no need to call prepare() again.
    */
    void reset() noexcept
    {
        for (auto& b : interpolBeginBufs)
            b.clear();

        for (auto& b : interpolEndBufs)
            b.clear();

        for (auto& b : decimBeginBufs)
            b.clear();

        for (auto& b : decimEndBufs)
            b.clear();

        xInterp.clear();
        xDecim.clear();
        oversampledBuffer.clear();

        currentOversampledSize = 0;
        currentNumChannels = 0;
    }

    //==============================================================================
    /**
        Upsample an input block into the internal oversampled buffer.

        After this call the internal buffer holds numSamples * OversampleFactor
        bandlimited interpolated samples per channel, accessible via
        getOversampledChannelData() or processOversampledBlock().

        @param input       Array of read pointers, one per channel (channel-major).
        @param numChannels Number of channels to process (must be <= maxChannels from prepare()).
        @param numSamples  Number of input samples per channel (must be <= maxBlockSize).
    */
    void upsample (const SampleType* const* input, int numChannels, int numSamples) noexcept
    {
        jassert (numChannels > 0 && numSamples > 0);
        jassert (numChannels <= xInterp.getNumChannels());
        jassert (numSamples + SincRadius <= xInterp.getNumSamples());

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto* inputData = input[ch];

            auto* xBuf = xInterp.getWritePointer (ch);
            auto& endBuf = interpolEndBufs[static_cast<std::size_t> (ch)];

            for (int i = 0; i < numSamples + SincRadius; ++i)
                *xBuf++ = (i >= SincRadius) ? inputData[i - SincRadius] : endBuf[i];
        }

        currentOversampledSize = numSamples * OversampleFactor;
        currentNumChannels = numChannels;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* xBuf = xInterp.getReadPointer (ch);
            auto& beginBuf = interpolBeginBufs[static_cast<std::size_t> (ch)];
            auto& endBuf = interpolEndBufs[static_cast<std::size_t> (ch)];

            auto* outBuf = oversampledBuffer.getWritePointer (ch);
            *outBuf++ = *xBuf;

            for (int k = 1; k < currentOversampledSize; ++k)
            {
                const int delta = k % OversampleFactor;
                const int index = k / OversampleFactor;

                if (delta != 0)
                {
                    CoeffType acc = CoeffType (0);

                    for (int n = -SincRadius; n <= 0; ++n)
                        acc += sincTable (n, delta) * static_cast<CoeffType> (xBuf[static_cast<std::size_t> (index - n)]);

                    for (int n = 1; n <= SincRadius; ++n)
                        acc += sincTable (n, delta) * static_cast<CoeffType> (beginBuf[SincRadius - n]);

                    *outBuf++ = static_cast<SampleType> (acc);
                }
                else
                {
                    *outBuf++ = xBuf[static_cast<std::size_t> (index)];
                    beginBuf.push (xBuf[static_cast<std::size_t> (index - 1)]);
                }
            }

            beginBuf.push (xBuf[static_cast<std::size_t> (numSamples - 1)]);

            for (int i = 0; i < SincRadius; ++i)
                endBuf.push (xBuf[static_cast<std::size_t> (numSamples + i)]);
        }
    }

    /**
        Downsample the internal oversampled buffer into an output block.

        Applies a lowpass anti-aliasing FIR to the oversampled data and decimates
        by OversampleFactor. Must be called after the oversampled buffer has been
        processed (e.g. via processOversampledBlock()).

        @param output      Array of write pointers, one per channel.
        @param numChannels Number of channels to write (must be <= maxChannels from prepare()).
        @param numSamples  Number of output samples per channel (must match the numSamples
                           passed to the preceding upsample() call).
    */
    void downsample (SampleType* const* output, int numChannels, int numSamples) noexcept
    {
        jassert (numChannels > 0 && numSamples > 0);
        jassert (numChannels <= xDecim.getNumChannels());
        jassert (currentOversampledSize > 0);

        const int interpolatedSize = currentOversampledSize;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* inBuf = oversampledBuffer.getReadPointer (ch);

            auto* xBuf = xDecim.getWritePointer (ch);
            auto& dEndBuf = decimEndBufs[static_cast<std::size_t> (ch)];

            for (int i = 0; i < interpolatedSize + SincRadius * OversampleFactor; ++i)
            {
                auto* currentBuf = inBuf + static_cast<std::size_t> (i - SincRadius * OversampleFactor);

                *xBuf++ = (i >= SincRadius * OversampleFactor) ? *currentBuf : dEndBuf[i];
            }
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* outputData = output[ch];

            auto* xBuf = xDecim.getReadPointer (ch);
            auto& beginBuf = decimBeginBufs[static_cast<std::size_t> (ch)];
            auto& dEndBuf = decimEndBufs[static_cast<std::size_t> (ch)];

            for (int k = 0; k < numSamples; ++k)
            {
                const int index = OversampleFactor * k;
                CoeffType acc = CoeffType (0);

                for (int n = 1; n <= SincRadius * OversampleFactor; ++n)
                    acc += sincTable[n] * static_cast<CoeffType> (beginBuf[SincRadius * OversampleFactor - n]);

                for (int n = 0; n >= -(SincRadius * OversampleFactor); --n)
                    acc += sincTable[n] * static_cast<CoeffType> (xBuf[static_cast<std::size_t> (index - n)]);

                for (int i = 0; i < OversampleFactor; ++i)
                    beginBuf.push (xBuf[static_cast<std::size_t> (index + i)]);

                outputData[k] = static_cast<SampleType> (acc / static_cast<CoeffType> (OversampleFactor));
            }

            for (int i = 0; i < SincRadius * OversampleFactor; ++i)
                dEndBuf.push (xBuf[static_cast<std::size_t> (interpolatedSize + i)]);
        }
    }

    //==============================================================================
    /**
        Invokes a callback with the internal oversampled multi-channel buffer.

        The callback receives a reference to the internal `AudioBuffer<SampleType>`, where each inner
        vector has getOversampledNumSamples() elements. Use this to apply processing at the elevated
        sample rate.

        @param callback  Callable with signature `void(AudioBuffer<SampleType>&)`.
    */
    template <typename Callable>
    void processOversampledBlock (Callable&& callback)
    {
        callback (oversampledBuffer);
    }

    //==============================================================================
    /**
        Returns a writable pointer to the data for a single oversampled channel.

        @param channel  Zero-based channel index.
        @return         Pointer to getOversampledNumSamples() contiguous samples,
                        or nullptr if the channel index is out of range, prepare()
                        has not been called, or the channel was not processed by
                        the most recent upsample() call.
    */
    forcedinline SampleType* getOversampledChannelData (int channel) noexcept
    {
        if (channel < 0 || channel >= currentNumChannels)
            return nullptr;

        return oversampledBuffer.getWritePointer (channel);
    }

    /**
        Returns a read-only pointer to the data for a single oversampled channel.

        @param channel  Zero-based channel index.
        @return         Pointer to getOversampledNumSamples() contiguous samples,
                        or nullptr if the channel index is out of range or the
                        channel was not processed by the most recent upsample()
                        call.
    */
    const forcedinline SampleType* getOversampledChannelData (int channel) const noexcept
    {
        if (channel < 0 || channel >= currentNumChannels)
            return nullptr;

        return oversampledBuffer.getReadPointer (channel);
    }

    /**
        Returns the number of samples currently in each oversampled channel.

        Equal to the numSamples argument of the most recent upsample() call
        multiplied by OversampleFactor.  Returns 0 before the first upsample()
        call or after reset().
    */
    forcedinline int getOversampledNumSamples() const noexcept
    {
        return currentOversampledSize;
    }

    /**
        Returns the processing latency introduced by the oversampler.

        @return Latency in input-rate samples (= 2 * SincRadius).
    */
    forcedinline int getLatencyInSamples() const noexcept
    {
        return 2 * SincRadius;
    }

private:
    //==============================================================================
    SincTable<CoeffType, OversampleFactor, SincRadius> sincTable;

    std::vector<CircularBuffer<SampleType, SincRadius>> interpolBeginBufs;
    std::vector<CircularBuffer<SampleType, SincRadius>> interpolEndBufs;
    std::vector<CircularBuffer<SampleType, SincRadius * OversampleFactor>> decimBeginBufs;
    std::vector<CircularBuffer<SampleType, SincRadius * OversampleFactor>> decimEndBufs;

    AudioBuffer<SampleType> xInterp;
    AudioBuffer<SampleType> xDecim;
    AudioBuffer<SampleType> oversampledBuffer;
    int currentOversampledSize = 0;
    int currentNumChannels = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oversampler)
};

//==============================================================================
/** @name Convenience type aliases for common oversampling configurations */
using Oversampler2xFloat = Oversampler<float, 2, 8>;   /**< 2x oversampler, float, 8-tap radius */
using Oversampler4xFloat = Oversampler<float, 4, 8>;   /**< 4x oversampler, float, 8-tap radius */
using Oversampler8xFloat = Oversampler<float, 8, 8>;   /**< 8x oversampler, float, 8-tap radius */
using Oversampler2xDouble = Oversampler<double, 2, 8>; /**< 2x oversampler, double, 8-tap radius */
using Oversampler4xDouble = Oversampler<double, 4, 8>; /**< 4x oversampler, double, 8-tap radius */
using Oversampler8xDouble = Oversampler<double, 8, 8>; /**< 8x oversampler, double, 8-tap radius */

} // namespace yup
