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
        applyDistortion (buf);  // buf is AudioBuffer<float>&
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

        Configures the internal windowed sinc tables and allocates per-channel
        history and staging buffers. Must be called before upsample() or downsample().

        @param sampleRate    Input sample rate in Hz.
        @param maxChannels   Maximum number of audio channels.
        @param maxBlockSize  Maximum input block size in samples.
    */
    void prepare (double sampleRate, int maxChannels, int maxBlockSize)
    {
        jassert (sampleRate > 0.0 && maxChannels > 0 && maxBlockSize > 0);

        interpolationTable.configure (static_cast<CoeffType> (sampleRate));
        interpolationTable.applyKaiserWindow (CoeffType (5));

        decimationTable.configureWithCutoff (static_cast<CoeffType> (sampleRate) * antiAliasCutoffRatio,
                                             static_cast<CoeffType> (sampleRate));
        decimationTable.applyKaiserWindow (CoeffType (5));

        normalizeFilterGains();

        const int maxInterpolated = maxBlockSize * OversampleFactor;

        interpolBeginBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius> {});
        interpolEndBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius> {});
        decimBeginBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius * OversampleFactor> {});
        decimEndBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius * OversampleFactor> {});

        xInterp.setSize (maxChannels, maxBlockSize + SincRadius);
        xInterp.clear();

        xDecim.setSize (maxChannels, maxInterpolated + SincRadius * OversampleFactor);
        xDecim.clear();

        oversampledBuffer.setSize (maxChannels, maxInterpolated, false, false, true);
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
        ScopedNoDenormals noDenormals;

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
        oversampledBuffer.setSize (currentNumChannels, currentOversampledSize, false, false, true);

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
                        acc += interpolationTable (n, delta) * static_cast<CoeffType> (xBuf[static_cast<std::size_t> (index - n)]);

                    for (int n = 1; n <= SincRadius; ++n)
                        acc += interpolationTable (n, delta) * static_cast<CoeffType> (beginBuf[SincRadius - n]);

                    *outBuf++ = static_cast<SampleType> (acc * interpolationGains[static_cast<std::size_t> (delta)]);
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
        by OversampleFactor.

        The usual caller reaches here after upsample() and some processing of the
        oversampled buffer (e.g. via processOversampledBlock()), but that order is
        not required: the oversampled buffer may equally be filled directly
        through getOversampledChannelData(), which is what a *decimate-first*
        user does - feeding audio in at the high rate, working at 1/N, and
        interpolating back with upsample(). The interpolation and decimation
        filters keep separate history, so one instance serves either direction.

        @param output      Array of write pointers, one per channel.
        @param numChannels Number of channels to write.
        @param numSamples  Number of output samples per channel; the oversampled
                           buffer is read as numSamples * OversampleFactor samples.
    */
    void downsample (SampleType* const* output, int numChannels, int numSamples) noexcept
    {
        ScopedNoDenormals noDenormals;

        const int interpolatedSize = numSamples * OversampleFactor;

        jassert (numChannels > 0 && numSamples > 0);
        jassert (numChannels <= xDecim.getNumChannels());
        jassert (numChannels <= oversampledBuffer.getNumChannels());
        jassert (interpolatedSize <= oversampledBuffer.getNumSamples());
        jassert (interpolatedSize + SincRadius * OversampleFactor <= xDecim.getNumSamples());

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* inBuf = oversampledBuffer.getReadPointer (ch);

            auto* xBuf = xDecim.getWritePointer (ch);
            auto& dEndBuf = decimEndBufs[static_cast<std::size_t> (ch)];

            for (int i = 0; i < interpolatedSize + SincRadius * OversampleFactor; ++i)
            {
                *xBuf++ = (i >= SincRadius * OversampleFactor)
                            ? inBuf[static_cast<std::size_t> (i - SincRadius * OversampleFactor)]
                            : dEndBuf[i];
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
                    acc += decimationTable[n] * static_cast<CoeffType> (beginBuf[SincRadius * OversampleFactor - n]);

                for (int n = 0; n >= -(SincRadius * OversampleFactor); --n)
                    acc += decimationTable[n] * static_cast<CoeffType> (xBuf[static_cast<std::size_t> (index - n)]);

                for (int i = 0; i < OversampleFactor; ++i)
                    beginBuf.push (xBuf[static_cast<std::size_t> (index + i)]);

                outputData[k] = static_cast<SampleType> (acc * decimationGain);
            }

            for (int i = 0; i < SincRadius * OversampleFactor; ++i)
                dEndBuf.push (xBuf[static_cast<std::size_t> (interpolatedSize + i)]);
        }

        currentOversampledSize = 0;
        currentNumChannels = 0;
    }

    //==============================================================================
    /**
        Invokes a callback with the internal oversampled multi-channel buffer.

        The callback receives a reference to the internal `AudioBuffer<SampleType>`.
        The buffer has the same channel count as the most recent upsample() call,
        and getOversampledNumSamples() samples per channel. Use this to apply
        processing at the elevated sample rate. If there is no pending
        oversampled block, the callback receives an empty buffer.

        @param callback  Callable with signature `void(AudioBuffer<SampleType>&)`.
    */
    template <typename Callable>
    void processOversampledBlock (Callable&& callback)
    {
        if (currentOversampledSize == 0 || currentNumChannels == 0)
        {
            AudioBuffer<SampleType> emptyBuffer;
            callback (emptyBuffer);
            return;
        }

        callback (oversampledBuffer);
    }

    //==============================================================================
    /**
        Returns a writable pointer to the data for a single oversampled channel.

        Writable, and bounded by what prepare() allocated rather than by the last
        upsample() call: filling this buffer directly is how a decimate-first
        caller hands high-rate audio to downsample() without upsampling first.

        @param channel  Zero-based channel index.
        @return         Pointer to the channel's oversampled samples, or nullptr
                        if the channel index is out of range or prepare() has not
                        been called.
    */
    forcedinline SampleType* getOversampledChannelData (int channel) noexcept
    {
        if (channel < 0 || channel >= oversampledBuffer.getNumChannels())
            return nullptr;

        return oversampledBuffer.getWritePointer (channel);
    }

    /**
        Returns a read-only pointer to the data for a single oversampled channel.

        @param channel  Zero-based channel index.
        @return         Pointer to the channel's oversampled samples, or nullptr
                        if the channel index is out of range or prepare() has not
                        been called.
    */
    const forcedinline SampleType* getOversampledChannelData (int channel) const noexcept
    {
        if (channel < 0 || channel >= oversampledBuffer.getNumChannels())
            return nullptr;

        return oversampledBuffer.getReadPointer (channel);
    }

    /**
        Returns the number of samples currently in each oversampled channel.

        Equal to the numSamples argument of the most recent pending upsample()
        call multiplied by OversampleFactor. Returns 0 before the first
        upsample() call, after downsample(), or after reset().
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
    void normalizeFilterGains() noexcept
    {
        for (int delta = 0; delta < OversampleFactor; ++delta)
        {
            CoeffType sum = CoeffType (0);

            for (int n = -SincRadius; n <= SincRadius; ++n)
                sum += interpolationTable (n, delta);

            jassert (sum != CoeffType (0));
            interpolationGains[static_cast<std::size_t> (delta)] = CoeffType (1) / sum;
        }

        CoeffType decimationSum = decimationTable[0];

        for (int n = 1; n <= SincRadius * OversampleFactor; ++n)
            decimationSum += CoeffType (2) * decimationTable[n];

        jassert (decimationSum != CoeffType (0));
        decimationGain = CoeffType (1) / decimationSum;
    }

    // Leave transition width before the original Nyquist frequency for decimation.
    static constexpr CoeffType antiAliasCutoffRatio = CoeffType (0.45);

    SincTable<CoeffType, OversampleFactor, SincRadius> interpolationTable;
    SincTable<CoeffType, OversampleFactor, SincRadius> decimationTable;
    std::array<CoeffType, static_cast<std::size_t> (OversampleFactor)> interpolationGains {};
    CoeffType decimationGain = CoeffType (1);

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
