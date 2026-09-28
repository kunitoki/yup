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

        Calling it again with the same number of channels and no larger block
        size than before reuses the existing buffers, so it does not allocate.
        It still clears the history and the phase, so use setRatio() to change
        the ratio of a running stream without a discontinuity.

        @param sourceSampleRate  Sample rate of the input signal in Hz.
        @param targetSampleRate  Desired output sample rate in Hz.
        @param maxChannels       Maximum number of audio channels.
        @param maxBlockSize      Maximum number of input samples per call to resample().
    */
    void prepare (double sourceSampleRate, double targetSampleRate, int maxChannels, int maxBlockSize)
    {
        jassert (maxChannels > 0 && maxBlockSize > 0);

        setRatio (sourceSampleRate, targetSampleRate);

        currentPhase = 0.0;
        maxOutputSamples = static_cast<int> (maxBlockSize * oversampleFactor) + 1;

        xBufs.resize (static_cast<std::size_t> (maxChannels));
        for (auto& ch : xBufs)
            ch.assign (static_cast<std::size_t> (maxBlockSize + historySize), SampleType {});
    }

    /**
        Changes the conversion ratio while keeping the phase and the history.

        The stream continues without a discontinuity, which makes this suitable
        for smoothly varying the ratio from the audio thread: it does not
        allocate, and it only recomputes the sinc table when downsampling moves
        the cutoff, reusing a Kaiser window computed once.

        @param sourceSampleRate  Sample rate of the input signal in Hz.
        @param targetSampleRate  Desired output sample rate in Hz.
    */
    void setRatio (double sourceSampleRate, double targetSampleRate) noexcept
    {
        jassert (sourceSampleRate > 0.0 && targetSampleRate > 0.0);

        oversampleFactor = targetSampleRate / sourceSampleRate;

        const double normalizedCutoff = std::min (1.0, oversampleFactor) / 2.0;
        if (normalizedCutoff == tableCutoff)
            return;

        tableCutoff = normalizedCutoff;

        const CoeffType cutoff = static_cast<CoeffType> (
            std::min (sourceSampleRate, targetSampleRate) / 2.0);

        sincTable.configureWithCutoff (cutoff, static_cast<CoeffType> (sourceSampleRate));
        sincTable.applyWindow (getKaiserHalfWindow());
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
        jassert (numSamples + historySize <= static_cast<int> (xBufs[0].size()));

        const int outputCount = static_cast<int> (std::ceil ((numSamples - currentPhase) * oversampleFactor));
        const CoeffType gainScale = (oversampleFactor < 1.0)
                                      ? static_cast<CoeffType> (oversampleFactor)
                                      : CoeffType (1);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            // The input is staged after 2 * SincRadius samples of history, so every tap of
            // the kernel reads real past and future samples and the output lags by SincRadius.
            auto& xBuf = xBufs[static_cast<std::size_t> (ch)];
            std::copy (input[ch], input[ch] + numSamples, xBuf.begin() + historySize);

            const SampleType* center = xBuf.data() + SincRadius;

            for (int k = 0; k < outputCount; ++k)
            {
                const double virtualIndex = static_cast<double> (k) / oversampleFactor + currentPhase;
                const int index = static_cast<int> (virtualIndex);
                const int delta = static_cast<int> ((virtualIndex - index) * Resolution);
                const SampleType* x = center + index;

                if (delta != 0 || oversampleFactor < 1.0)
                {
                    CoeffType acc = CoeffType (0);

                    for (int n = -SincRadius; n <= SincRadius; ++n)
                        acc += sincTable (n, delta) * static_cast<CoeffType> (x[-n]);

                    output[ch][k] = static_cast<SampleType> (acc * gainScale);
                }
                else
                {
                    output[ch][k] = static_cast<SampleType> (static_cast<CoeffType> (x[0]) * gainScale);
                }
            }

            std::copy (xBuf.begin() + numSamples, xBuf.begin() + numSamples + historySize, xBuf.begin());
        }

        currentPhase = std::max (0.0, (currentPhase + static_cast<double> (outputCount) / oversampleFactor) - numSamples);
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
    using SincTableType = SincTable<CoeffType, Resolution, SincRadius>;

    static constexpr int historySize = 2 * SincRadius;

    static const typename SincTableType::HalfWindow& getKaiserHalfWindow() noexcept
    {
        struct KaiserHalfWindow
        {
            KaiserHalfWindow() noexcept { SincTableType::fillKaiserHalfWindow (values, CoeffType (5)); }

            typename SincTableType::HalfWindow values;
        };

        static const KaiserHalfWindow window;
        return window.values;
    }

    SincTableType sincTable;

    std::vector<std::vector<SampleType>> xBufs;

    double currentPhase = 0.0;
    double oversampleFactor = 1.0;
    double tableCutoff = 0.0;
    int maxOutputSamples = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Resampler)
};

//==============================================================================
/** @name Convenience type aliases for common resampling configurations */
///@{
using ResamplerFloat = Resampler<float, 16>;   /**< Resampler for float samples, 16-tap radius */
using ResamplerDouble = Resampler<double, 16>; /**< Resampler for double samples, 16-tap radius */
///@}

} // namespace yup
