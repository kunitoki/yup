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
    bandlimited interpolation and anti-aliasing. Each channel keeps the last
    kernel-length of samples contiguously in front of its staging buffer, so
    every output sample is a single contiguous dot product and multi-block
    (real-time) operation is seamless.

    Both kernels are Kaiser-windowed sincs (beta = 9, roughly 90 dB of
    stopband rejection when the radius allows it) designed in CoeffType and
    applied to SampleType data, accumulating in CoeffType. The interpolator
    cuts off at the input Nyquist frequency, the decimator at 0.45 times the
    input sample rate. The total round-trip latency is 2 * SincRadius input
    samples; generation followed by downsample() costs SincRadius.

    Typical usage in an audio effect:
    @code
    yup::Oversampler<float, 4, 16> os;
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
                             Higher values give a steeper transition and deeper
                             stopband at the cost of more computation and latency.
    @tparam CoeffType        Precision for filter design and accumulation (default double).
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

        Designs the interpolation and decimation kernels and allocates the
        per-channel staging buffers. Must be called before upsample() or downsample().

        @param sampleRate    Input sample rate in Hz.
        @param maxChannels   Maximum number of audio channels.
        @param maxBlockSize  Maximum input block size in samples.
    */
    void prepare (double sampleRate, int maxChannels, int maxBlockSize)
    {
        jassert (sampleRate > 0.0 && maxChannels > 0 && maxBlockSize > 0);

        buildInterpolationTaps (static_cast<CoeffType> (sampleRate));
        buildDecimationTaps (static_cast<CoeffType> (sampleRate));

        maxInputSamples = maxBlockSize;

        xInterp.setSize (maxChannels, maxBlockSize + interpolationHistory);
        xDecim.setSize (maxChannels, maxBlockSize * OversampleFactor + decimationHistory);
        oversampledBuffer.setSize (maxChannels, maxBlockSize * OversampleFactor, false, false, true);

        reset();
    }

    /**
        Resets all internal processing state.

        Clears all history so that a fresh processing session can begin
        without artifacts from a previous session. Filter coefficients are
        preserved; there is no need to call prepare() again.
    */
    void reset() noexcept
    {
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
        jassert (numSamples <= maxInputSamples);

        currentOversampledSize = numSamples * OversampleFactor;
        currentNumChannels = numChannels;
        oversampledBuffer.setSize (numChannels, currentOversampledSize, false, false, true);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* history = xInterp.getWritePointer (ch);
            FloatVectorOperations::copy (history + interpolationHistory, input[ch], numSamples);

            auto* out = oversampledBuffer.getWritePointer (ch);

            for (int i = 0; i < numSamples; ++i)
            {
                const auto* window = history + i;
                *out++ = window[SincRadius];

                for (int delta = 1; delta < OversampleFactor; ++delta)
                    *out++ = dotProduct (interpolationTaps.data() + delta * interpolationTapCount, window, static_cast<std::size_t> (interpolationTapCount));
            }

            std::copy (history + numSamples, history + numSamples + interpolationHistory, history);
        }
    }

    /** Starts a block generated directly at the oversampled rate.

        Call after prepare(), fill every sample obtained through
        getOversampledChannelData(), then call downsample(). No input upsampling
        is performed and no memory is allocated. Returns false for nonpositive
        sizes or sizes exceeding the prepared channel/block capacity; a pending
        block is left unchanged on failure. The buffer contents are unspecified.

        @param numChannels  Number of generated channels.
        @param numSamples   Number of samples per channel at the output rate.
        @see getGenerationLatencyInSamples
    */
    bool beginGeneration (int numChannels, int numSamples) noexcept
    {
        if (numChannels <= 0 || numSamples <= 0
            || numChannels > xInterp.getNumChannels()
            || numSamples > maxInputSamples)
            return false;

        currentOversampledSize = numSamples * OversampleFactor;
        currentNumChannels = numChannels;
        oversampledBuffer.setSize (numChannels, currentOversampledSize, false, false, true);
        return true;
    }

    /** Returns the latency of generation followed by downsample(), in output samples.

        Unlike getLatencyInSamples(), this excludes the input interpolation stage.
    */
    static constexpr int getGenerationLatencyInSamples() noexcept { return SincRadius; }

    /**
        Downsample the internal oversampled buffer into an output block.

        Applies a lowpass anti-aliasing FIR to the oversampled data and decimates
        by OversampleFactor. Must be called after the oversampled buffer has been
        processed (e.g. via processOversampledBlock()).

        @param output      Array of write pointers, one per channel.
        @param numChannels Number of channels to write (must match the numChannels
                           passed to the preceding upsample() or beginGeneration() call).
        @param numSamples  Number of output samples per channel (must match the numSamples
                           passed to the preceding upsample() or beginGeneration() call).
    */
    void downsample (SampleType* const* output, int numChannels, int numSamples) noexcept
    {
        ScopedNoDenormals noDenormals;

        jassert (numChannels > 0 && numSamples > 0);
        jassert (numChannels <= xDecim.getNumChannels());
        jassert (numChannels == currentNumChannels);
        jassert (currentOversampledSize > 0);
        jassert (numSamples * OversampleFactor == currentOversampledSize);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* history = xDecim.getWritePointer (ch);
            FloatVectorOperations::copy (history + decimationHistory, oversampledBuffer.getReadPointer (ch), currentOversampledSize);

            auto* out = output[ch];

            for (int k = 0; k < numSamples; ++k)
                out[k] = dotProduct (decimationTaps.data(), history + k * OversampleFactor, static_cast<std::size_t> (decimationTapCount));

            std::copy (history + currentOversampledSize, history + currentOversampledSize + decimationHistory, history);
        }

        currentOversampledSize = 0;
        currentNumChannels = 0;
    }

    //==============================================================================
    /**
        Invokes a callback with the internal oversampled multi-channel buffer.

        The callback receives a reference to the internal `AudioBuffer<SampleType>`.
        The buffer has the channel count of the most recent upsample() or
        beginGeneration() call,
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

        @param channel  Zero-based channel index.
        @return         Pointer to getOversampledNumSamples() contiguous samples,
                        or nullptr if the channel index is out of range, prepare()
                        has not been called, or the channel was not processed by
                        the most recent upsample() or beginGeneration() call.
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
                        or beginGeneration() call.
    */
    const forcedinline SampleType* getOversampledChannelData (int channel) const noexcept
    {
        if (channel < 0 || channel >= currentNumChannels)
            return nullptr;

        return oversampledBuffer.getReadPointer (channel);
    }

    /**
        Returns the number of samples currently in each oversampled channel.

        Equal to the numSamples argument of the pending upsample() or
        beginGeneration() call multiplied by OversampleFactor. Returns 0 before
        either call, after downsample(), or after reset().
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
    static constexpr int interpolationTapCount = 2 * SincRadius + 1;
    static constexpr int interpolationHistory = 2 * SincRadius;
    static constexpr int decimationTapCount = 2 * SincRadius * OversampleFactor + 1;
    static constexpr int decimationHistory = 2 * SincRadius * OversampleFactor;

    static constexpr CoeffType kaiserBeta = CoeffType (9);

    // Leave transition width before the original Nyquist frequency for decimation.
    static constexpr CoeffType antiAliasCutoffRatio = CoeffType (0.45);

    //==============================================================================
    // Phase-major taps, each phase normalized to unity DC gain. Phase 0 is the
    // pass-through sample and is never read.
    void buildInterpolationTaps (CoeffType sampleRate)
    {
        SincTable<CoeffType, OversampleFactor, SincRadius> table;
        table.configure (sampleRate);
        table.applyKaiserWindow (kaiserBeta);

        interpolationTaps.assign (static_cast<std::size_t> (OversampleFactor * interpolationTapCount), CoeffType (0));

        for (int delta = 1; delta < OversampleFactor; ++delta)
        {
            auto* taps = interpolationTaps.data() + delta * interpolationTapCount;
            CoeffType sum = CoeffType (0);

            for (int j = 0; j < interpolationTapCount; ++j)
            {
                taps[j] = table (SincRadius - j, delta);
                sum += taps[j];
            }

            jassert (sum != CoeffType (0));
            const CoeffType gain = CoeffType (1) / sum;

            for (int j = 0; j < interpolationTapCount; ++j)
                taps[j] *= gain;
        }
    }

    void buildDecimationTaps (CoeffType sampleRate)
    {
        SincTable<CoeffType, OversampleFactor, SincRadius> table;
        table.configureWithCutoff (sampleRate * antiAliasCutoffRatio, sampleRate);
        table.applyKaiserWindow (kaiserBeta);

        decimationTaps.resize (static_cast<std::size_t> (decimationTapCount));
        CoeffType sum = CoeffType (0);

        for (int j = 0; j < decimationTapCount; ++j)
        {
            decimationTaps[static_cast<std::size_t> (j)] = table[j - SincRadius * OversampleFactor];
            sum += decimationTaps[static_cast<std::size_t> (j)];
        }

        jassert (sum != CoeffType (0));
        const CoeffType gain = CoeffType (1) / sum;

        for (auto& tap : decimationTaps)
            tap *= gain;
    }

    //==============================================================================
    std::vector<CoeffType> interpolationTaps;
    std::vector<CoeffType> decimationTaps;

    AudioBuffer<SampleType> xInterp;
    AudioBuffer<SampleType> xDecim;
    AudioBuffer<SampleType> oversampledBuffer;
    int maxInputSamples = 0;
    int currentOversampledSize = 0;
    int currentNumChannels = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Oversampler)
};

//==============================================================================
/** @name Convenience type aliases for common oversampling configurations (latency 32 samples) */
using Oversampler2xFloat = Oversampler<float, 2, 16>;   /**< 2x oversampler, float, 16-tap radius */
using Oversampler4xFloat = Oversampler<float, 4, 16>;   /**< 4x oversampler, float, 16-tap radius */
using Oversampler8xFloat = Oversampler<float, 8, 16>;   /**< 8x oversampler, float, 16-tap radius */
using Oversampler16xFloat = Oversampler<float, 16, 16>; /**< 16x oversampler, float, 16-tap radius */
using Oversampler32xFloat = Oversampler<float, 32, 16>; /**< 32x oversampler, float, 16-tap radius */
using Oversampler2xDouble = Oversampler<double, 2, 16>; /**< 2x oversampler, double, 16-tap radius */
using Oversampler4xDouble = Oversampler<double, 4, 16>; /**< 4x oversampler, double, 16-tap radius */
using Oversampler8xDouble = Oversampler<double, 8, 16>; /**< 8x oversampler, double, 16-tap radius */
using Oversampler16xDouble = Oversampler<double, 16, 16>; /**< 16x oversampler, double, 16-tap radius */
using Oversampler32xDouble = Oversampler<double, 32, 16>; /**< 32x oversampler, double, 16-tap radius */

} // namespace yup
