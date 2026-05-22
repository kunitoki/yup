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
    Multi-channel asynchronous resampler for arbitrary sample-rate conversion.

    Resampler converts between any two sample rates (including non-integer
    ratios such as 44100 Hz → 48000 Hz) using a polyphase windowed sinc
    filter with high-resolution phase lookup.  Per-channel history buffers
    ensure seamless multi-block (real-time) processing.

    Key behaviours:
    - Automatically scales gain to compensate when downsampling.
    - Phase state persists across blocks, so consecutive calls produce a
      continuous, gapless output stream.
    - Call reset() to restart the phase accumulator (e.g. after a transport loop).

    @code
    yup::Resampler<float, 8> resampler;
    resampler.prepare (44100.0, 48000.0, 2, 512);

    // Inside your audio callback:
    int produced = resampler.resample (inputPtrs, outputPtrs, 2, numSamples);
    @endcode

    @tparam SampleType  Audio sample type (float or double).
    @tparam SincRadius  Half-width of the sinc kernel in source-rate samples.
    @tparam Resolution  Number of fractional phase sub-steps (default 256).
                        Higher values give more accurate interpolation at the
                        cost of a larger lookup table.
    @tparam CoeffType   Precision for internal filter coefficients (default double).
*/
template <typename SampleType, int SincRadius, int Resolution = 256, typename CoeffType = double>
class Resampler
{
public:
    static_assert (SincRadius >= 1, "SincRadius must be at least 1");
    static_assert (Resolution >= 2, "Resolution must be at least 2");

    //==============================================================================
    /** Default constructor. Call prepare() before any processing. */
    Resampler() = default;

    /** Destructor. */
    ~Resampler() = default;

    //==============================================================================
    /**
        Prepares the resampler for processing.

        Configures the internal windowed sinc table, allocates per-channel
        history buffers, and initialises the phase accumulator.  Must be called
        before resample().

        @param sourceSampleRate  Sample rate of the input signal in Hz.
        @param targetSampleRate  Desired output sample rate in Hz.
        @param maxChannels       Maximum number of audio channels.
        @param maxBlockSize      Maximum number of input samples per call to resample().
    */
    void prepare (double sourceSampleRate, double targetSampleRate, int maxChannels, int maxBlockSize)
    {
        jassert (sourceSampleRate > 0.0 && targetSampleRate > 0.0);
        jassert (maxChannels > 0 && maxBlockSize > 0);

        const CoeffType cutoff = static_cast<CoeffType> (
            std::min (sourceSampleRate, targetSampleRate) / 2.0);

        sincTable.configureWithCutoff (cutoff, static_cast<CoeffType> (sourceSampleRate));
        sincTable.applyKaiserWindow (CoeffType (5));

        currentPhase = 0.0;
        oversampleFactor = targetSampleRate / sourceSampleRate;
        maxOutputSamples = static_cast<int> (maxBlockSize * oversampleFactor) + 1;

        beginBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius> {});
        endBufs.assign (maxChannels, CircularBuffer<SampleType, SincRadius> {});

        xBufs.assign (maxChannels, std::vector<SampleType> (static_cast<std::size_t> (maxBlockSize + SincRadius + 1), SampleType {}));
    }

    /**
        Resets the phase accumulator and all history buffers.

        Call this when restarting processing after a discontinuity such as a
        transport loop.  The filter coefficients remain valid; there is no need
        to call prepare() again.
    */
    void reset() noexcept
    {
        currentPhase = 0.0;

        for (auto& b : beginBufs)
            b.clear();

        for (auto& b : endBufs)
            b.clear();

        for (auto& ch : xBufs)
            std::fill (ch.begin(), ch.end(), SampleType {});
    }

    //==============================================================================
    /**
        Converts numSamples input samples into output samples at the target rate.

        @param input       Array of read pointers, one per channel (channel-major).
        @param output      Array of write pointers, one per channel.  The caller
                           must allocate at least ceil(numSamples * targetRate / sourceRate) + 1
                           samples per channel in the output buffers.
        @param numChannels Number of channels to process.
        @param numSamples  Number of input samples per channel.
        @return            Number of output samples written per channel.
    */
    int resample (const SampleType* const* input, SampleType* const* output, int numChannels, int numSamples) noexcept
    {
        jassert (numChannels > 0 && numSamples > 0);
        jassert (numChannels <= static_cast<int> (xBufs.size()));
        jassert (numSamples + SincRadius + 1 <= static_cast<int> (xBufs[0].size()));

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& xBuf = xBufs[static_cast<std::size_t> (ch)];
            auto& eBuf = endBufs[static_cast<std::size_t> (ch)];

            for (int i = 0; i < numSamples + SincRadius; ++i)
            {
                xBuf[static_cast<std::size_t> (i)] = (i >= SincRadius)
                                                       ? input[ch][i - SincRadius]
                                                       : eBuf[i];
            }
        }

        const int outputCount = static_cast<int> ((numSamples - currentPhase) * oversampleFactor);
        const CoeffType gainScale = (oversampleFactor < 1.0)
                                      ? static_cast<CoeffType> (oversampleFactor)
                                      : CoeffType (1);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& xBuf = xBufs[static_cast<std::size_t> (ch)];
            auto& bBuf = beginBufs[static_cast<std::size_t> (ch)];

            for (int k = 0; k < outputCount; ++k)
            {
                const double virtualIndex = static_cast<double> (k) / oversampleFactor + currentPhase;
                const int index = static_cast<int> (virtualIndex);
                const int delta = static_cast<int> ((virtualIndex - index) * Resolution);

                if (delta != 0)
                {
                    CoeffType acc = CoeffType (0);

                    for (int n = -SincRadius; n <= 0; ++n)
                        acc += sincTable (n, delta)
                             * static_cast<CoeffType> (xBuf[static_cast<std::size_t> (index - n)]);

                    for (int n = 1; n <= SincRadius; ++n)
                        acc += sincTable (n, delta)
                             * static_cast<CoeffType> (bBuf[SincRadius - n]);

                    output[ch][k] = static_cast<SampleType> (acc * gainScale);
                }
                else
                {
                    output[ch][k] = static_cast<SampleType> (
                        static_cast<CoeffType> (xBuf[static_cast<std::size_t> (index)]) * gainScale);

                    if (index > 0)
                        bBuf.push (xBuf[static_cast<std::size_t> (index - 1)]);
                    else
                        bBuf.push (bBuf[SincRadius - 1]);
                }
            }

            beginBufs[static_cast<std::size_t> (ch)].push (xBuf[static_cast<std::size_t> (numSamples - 1)]);

            for (int i = 0; i < SincRadius; ++i)
                endBufs[static_cast<std::size_t> (ch)].push (xBuf[static_cast<std::size_t> (numSamples + i)]);
        }

        currentPhase = (currentPhase + static_cast<double> (outputCount) / oversampleFactor) - numSamples;
        return outputCount;
    }

    //==============================================================================
    /**
        Returns the processing latency introduced by the resampler.

        @return Latency in input-rate samples (= SincRadius).
    */
    int getLatencyInSamples() const noexcept
    {
        return SincRadius;
    }

private:
    //==============================================================================
    SincTable<CoeffType, Resolution, SincRadius> sincTable;

    std::vector<CircularBuffer<SampleType, SincRadius>> beginBufs;
    std::vector<CircularBuffer<SampleType, SincRadius>> endBufs;
    std::vector<std::vector<SampleType>> xBufs;

    double currentPhase = 0.0;
    double oversampleFactor = 1.0;
    int maxOutputSamples = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Resampler)
};

//==============================================================================
/** @name Convenience type aliases for common resampling configurations */
///@{
using ResamplerFloat = Resampler<float, 8>;   /**< Resampler for float samples, 8-tap radius */
using ResamplerDouble = Resampler<double, 8>; /**< Resampler for double samples, 8-tap radius */
///@}

} // namespace yup
