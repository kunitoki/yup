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
/** Halfband filter families available to HalfbandOversampler. */
enum class HalfbandFilterType
{
    linearPhaseFIR, /**< Kaiser-windowed halfband FIR: exact linear phase and integer latency. */
    polyphaseIIR    /**< Elliptic allpass polyphase halfband: a few multiplies per sample and
                         minimal latency, at the cost of a nonlinear phase near the band edge. */
};

//==============================================================================
/** Filter design targets applied by HalfbandOversampler::prepare(). */
struct HalfbandOversamplerDesign
{
    /** Filter family used by every stage. */
    HalfbandFilterType filterType = HalfbandFilterType::linearPhaseFIR;

    /** Minimum stopband rejection of every stage, in dB. */
    double stopbandAttenuationDb = 100.0;

    /** Passband edge as a fraction of the input sample rate, in (0, 0.5). */
    double passbandEdge = 0.45;

    /** Stopband edge of the stage next to the input rate, as a fraction of the
        input sample rate, in (passbandEdge, 1 - passbandEdge].

        0.5 (the default) rejects everything above the input Nyquist, so nothing
        folds back into the band. 1 - passbandEdge makes that stage a pure
        halfband: about a quarter of the cost and half the latency, but content
        between 0.5 and 1 - passbandEdge folds into the top of the band with only
        partial attenuation. Values in between trade one for the other. Only
        linearPhaseFIR honours this; polyphaseIIR is always a pure halfband.
    */
    double stopbandEdge = 0.5;
};

//==============================================================================
/**
    Multi-channel power-of-two oversampler built from a cascade of 2x halfband stages.

    Each stage doubles or halves the rate with a halfband filter, whose every
    other tap is zero, so a stage costs about a quarter of its nominal length.
    Only the stage next to the base rate has to be steep; every further stage
    only rejects what would fold into the passband and shrinks to a handful of
    taps. Compared with a single polyphase sinc kernel this gives a deeper
    stopband and a steeper edge for less work, and the advantage grows with the
    factor.

    Two filter families are available through Design::filterType:
    - linearPhaseFIR designs Kaiser-windowed halfbands whose stopband is verified
      numerically at prepare() time. Latency is an exact integer number of input
      samples on both the round trip and the generation path.
    - polyphaseIIR designs elliptic halfbands realised as two allpass branches.
      They cost a few multiplies per sample and have a fraction of the FIR
      latency, but the phase is nonlinear near the passband edge and the latency
      reported is the low-frequency group delay rounded to the nearest sample.

    The passband extends to Design::passbandEdge times the input rate (0.45 by
    default, 19.8 kHz at 44.1 kHz) with at least Design::stopbandAttenuationDb of
    rejection (100 dB by default) from Design::stopbandEdge upwards. With the
    default edge of 0.5 the linear-phase FIR rejects everything above the input
    Nyquist, so nothing folds back into the band; that first stage is then a
    general polyphase lowpass rather than a halfband and dominates the cost.
    Setting stopbandEdge to 1 - passbandEdge makes it a pure halfband, about a
    quarter of the cost and half the latency, at the price of content between
    0.5 and 0.55 of the input rate folding into the top of the band. The IIR
    family is always a pure halfband and shows that fold-back.

    Typical usage:
    @code
    yup::HalfbandOversampler<float, 4> os;
    os.prepare (44100.0, 2, 512);

    os.upsample (inputPtrs, numChannels, numSamples);
    os.processOversampledBlock ([&] (auto& buf) { applyDistortion (buf); });
    os.downsample (outputPtrs, numChannels, numSamples);
    @endcode

    @tparam SampleType       Audio sample type (float or double).
    @tparam OversampleFactor Integer upsample ratio, a power of two (2, 4, 8, …).
    @tparam CoeffType        Precision for filter design, coefficients and accumulation (default double).

    @see SincOversampler
*/
template <typename SampleType, int OversampleFactor, typename CoeffType = double>
class HalfbandOversampler
{
public:
    static_assert (OversampleFactor >= 2 && isPowerOfTwo (OversampleFactor), "OversampleFactor must be a power of two >= 2");

    /** Number of 2x stages in the cascade (log2 of the factor). */
    static constexpr int numStages = std::bit_width (static_cast<unsigned int> (OversampleFactor)) - 1;

    /** Filter design targets, applied by prepare(); shared by every instantiation. */
    using Design = HalfbandOversamplerDesign;

    //==============================================================================
    /** Default constructor. Call prepare() before any processing. */
    HalfbandOversampler() = default;

    /** Destructor. */
    ~HalfbandOversampler() = default;

    //==============================================================================
    /**
        Prepares the oversampler for processing.

        Designs every stage from the requested Design and allocates the staging
        buffers. Must be called before upsample(), beginGeneration() or downsample().
        Not realtime-safe.

        @param sampleRate    Input sample rate in Hz (the design itself is rate independent).
        @param maxChannels   Maximum number of audio channels.
        @param maxBlockSize  Maximum input block size in samples.
        @param newDesign     Filter family and quality targets.
    */
    void prepare (double sampleRate, int maxChannels, int maxBlockSize, const Design& newDesign = {})
    {
        jassert (sampleRate > 0.0 && maxChannels > 0 && maxBlockSize > 0);
        jassert (newDesign.stopbandAttenuationDb >= 20.0);
        jassert (newDesign.passbandEdge > 0.0 && newDesign.passbandEdge < 0.5);
        ignoreUnused (sampleRate);

        design = newDesign;
        design.stopbandAttenuationDb = jlimit (20.0, 200.0, design.stopbandAttenuationDb);
        design.passbandEdge = jlimit (0.05, 0.49, design.passbandEdge);
        design.stopbandEdge = jlimit (design.passbandEdge + 0.01, 1.0 - design.passbandEdge, design.stopbandEdge);
        maxChannelCount = maxChannels;
        maxInputSamples = maxBlockSize;

        double interpolationDelay = 0.0;

        for (int s = 0; s < numStages; ++s)
        {
            auto& stage = stages[static_cast<std::size_t> (s)];
            const int lowRate = 1 << s;

            const double passband = (s == 0 ? design.passbandEdge : 0.5) / (2 * lowRate);
            const double stopband = (s == 0 && design.filterType == HalfbandFilterType::linearPhaseFIR)
                                      ? design.stopbandEdge / 2.0
                                      : 0.5 - passband;

            if (design.filterType == HalfbandFilterType::linearPhaseFIR)
            {
                designFirStage (stage, passband, stopband);
                interpolationDelay += static_cast<double> (stage.halfLength) / (2 * lowRate);
            }
            else
            {
                designIirStage (stage, stopband - passband);
                interpolationDelay += stage.lowFrequencyDelay / lowRate;
            }

            const int lowBlock = maxBlockSize * lowRate;

            stage.interpolationInput.setSize (maxChannels, lowBlock + stage.halfLength);
            stage.interpolationOutput.setSize (maxChannels, 2 * lowBlock);
            stage.evenInput.setSize (maxChannels, lowBlock + stage.halfLength);
            stage.oddInput.setSize (maxChannels, lowBlock + stage.halfLength);
            stage.decimationOutput.setSize (maxChannels, lowBlock);

            const auto numSections = stage.directAllpass.size() + stage.delayedAllpass.size();
            stage.upStates.assign (static_cast<std::size_t> (maxChannels) * 2 * numSections, CoeffType (0));
            stage.downStates.assign (static_cast<std::size_t> (maxChannels) * (2 * numSections + 1), CoeffType (0));
        }

        if (design.filterType == HalfbandFilterType::linearPhaseFIR)
        {
            const double rounded = std::ceil (interpolationDelay);
            paddingSamples = roundToInt ((rounded - interpolationDelay) * OversampleFactor);
            generationLatency = static_cast<int> (rounded);
            roundTripLatency = 2 * generationLatency;
        }
        else
        {
            paddingSamples = 0;
            generationLatency = roundToInt (interpolationDelay);
            roundTripLatency = roundToInt (2.0 * interpolationDelay);
        }

        auto& top = stages[static_cast<std::size_t> (numStages - 1)];
        top.interpolationOutput.setSize (maxChannels, maxBlockSize * OversampleFactor + paddingSamples);
        paddedInput.setSize (maxChannels, maxBlockSize * OversampleFactor + paddingSamples);
        oversampledBuffer.setSize (maxChannels, maxBlockSize * OversampleFactor, false, false, true);

        reset();
    }

    /**
        Resets all internal processing state.

        Clears every stage's history and filter state; the design is preserved,
        so there is no need to call prepare() again.
    */
    void reset() noexcept
    {
        for (auto& stage : stages)
        {
            stage.interpolationInput.clear();
            stage.interpolationOutput.clear();
            stage.evenInput.clear();
            stage.oddInput.clear();
            stage.decimationOutput.clear();
            std::fill (stage.upStates.begin(), stage.upStates.end(), CoeffType (0));
            std::fill (stage.downStates.begin(), stage.downStates.end(), CoeffType (0));
        }

        paddedInput.clear();
        oversampledBuffer.clear();

        currentOversampledSize = 0;
        currentNumChannels = 0;
    }

    //==============================================================================
    /**
        Upsample an input block into the internal oversampled buffer.

        After this call the internal buffer holds numSamples * OversampleFactor
        bandlimited samples per channel, accessible via getOversampledChannelData()
        or processOversampledBlock().

        @param input       Array of read pointers, one per channel (channel-major).
        @param numChannels Number of channels to process (must be <= maxChannels from prepare()).
        @param numSamples  Number of input samples per channel (must be <= maxBlockSize).
    */
    void upsample (const SampleType* const* input, int numChannels, int numSamples) noexcept
    {
        ScopedNoDenormals noDenormals;

        jassert (numChannels > 0 && numSamples > 0);
        jassert (numChannels <= maxChannelCount);
        jassert (numSamples <= maxInputSamples);

        currentOversampledSize = numSamples * OversampleFactor;
        currentNumChannels = numChannels;
        oversampledBuffer.setSize (numChannels, currentOversampledSize, false, false, true);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const SampleType* current = input[ch];
            int count = numSamples;

            for (int s = 0; s < numStages; ++s)
            {
                auto& stage = stages[static_cast<std::size_t> (s)];
                auto* out = stage.interpolationOutput.getWritePointer (ch) + ((s == numStages - 1) ? paddingSamples : 0);

                if (design.filterType == HalfbandFilterType::linearPhaseFIR)
                    interpolateFir (stage, ch, current, out, count);
                else
                    interpolateIir (stage, ch, current, out, count);

                current = out;
                count *= 2;
            }

            // The top stage writes behind paddingSamples carried from the previous
            // block, which rounds the cascade's delay to a whole input sample.
            auto* top = stages[static_cast<std::size_t> (numStages - 1)].interpolationOutput.getWritePointer (ch);
            FloatVectorOperations::copy (oversampledBuffer.getWritePointer (ch), top, count);
            std::copy (top + count, top + count + paddingSamples, top);
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
            || numChannels > maxChannelCount
            || numSamples > maxInputSamples)
            return false;

        currentOversampledSize = numSamples * OversampleFactor;
        currentNumChannels = numChannels;
        oversampledBuffer.setSize (numChannels, currentOversampledSize, false, false, true);
        return true;
    }

    /**
        Downsample the internal oversampled buffer into an output block.

        Runs the decimation cascade on the oversampled data. Must be called after
        the oversampled buffer has been processed (e.g. via processOversampledBlock()).

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
        jassert (numChannels == currentNumChannels);
        jassert (currentOversampledSize > 0);
        jassert (numSamples * OversampleFactor == currentOversampledSize);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const SampleType* current = oversampledBuffer.getReadPointer (ch);
            int count = currentOversampledSize;

            auto* padded = paddedInput.getWritePointer (ch);

            if (paddingSamples > 0)
            {
                FloatVectorOperations::copy (padded + paddingSamples, current, count);
                current = padded;
            }

            for (int s = numStages - 1; s >= 0; --s)
            {
                auto& stage = stages[static_cast<std::size_t> (s)];
                auto* out = (s == 0) ? output[ch] : stage.decimationOutput.getWritePointer (ch);

                if (design.filterType == HalfbandFilterType::linearPhaseFIR)
                    decimateFir (stage, ch, current, out, count);
                else
                    decimateIir (stage, ch, current, out, count);

                current = out;
                count /= 2;
            }

            if (paddingSamples > 0)
                std::copy (padded + currentOversampledSize, padded + currentOversampledSize + paddingSamples, padded);
        }

        currentOversampledSize = 0;
        currentNumChannels = 0;
    }

    //==============================================================================
    /**
        Invokes a callback with the internal oversampled multi-channel buffer.

        The callback receives a reference to the internal `AudioBuffer<SampleType>`
        with the channel count of the most recent upsample() or beginGeneration()
        call and getOversampledNumSamples() samples per channel. If there is no
        pending oversampled block, the callback receives an empty buffer.

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

    /** @copydoc getOversampledChannelData(int) */
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
        Returns the round-trip latency of upsample() followed by downsample().

        Exact for linearPhaseFIR; the low-frequency group delay rounded to the
        nearest sample for polyphaseIIR. Valid after prepare().

        @return Latency in input-rate samples.
    */
    forcedinline int getLatencyInSamples() const noexcept
    {
        return roundTripLatency;
    }

    /** Returns the latency of generation followed by downsample(), in output samples.

        Unlike getLatencyInSamples(), this excludes the input interpolation stage.
        Valid after prepare().
    */
    forcedinline int getGenerationLatencyInSamples() const noexcept
    {
        return generationLatency;
    }

    /** Returns the design applied by the last prepare() call. */
    const Design& getDesign() const noexcept
    {
        return design;
    }

    /**
        Returns the filter order of one stage: the FIR length for linearPhaseFIR,
        the elliptic order for polyphaseIIR. Stage 0 runs next to the input rate.
    */
    int getStageFilterOrder (int stage) const noexcept
    {
        if (stage < 0 || stage >= numStages)
            return 0;

        const auto& s = stages[static_cast<std::size_t> (stage)];

        if (design.filterType == HalfbandFilterType::linearPhaseFIR)
            return 2 * s.halfLength + 1;

        return 2 * static_cast<int> (s.directAllpass.size() + s.delayedAllpass.size()) + 1;
    }

private:
    //==============================================================================
    struct Stage
    {
        int halfLength = 0;                         // FIR group delay in high-rate samples; length is 2 * halfLength + 1
        bool halfband = true;                       // odd branch is the pure delay 0.5 * z^-halfLength
        std::vector<CoeffType> evenDecimation;      // even polyphase branch (halfLength + 1 taps), summing to 0.5
        std::vector<CoeffType> evenInterpolation;   // the same taps scaled by 2
        std::vector<CoeffType> oddDecimation;       // odd polyphase branch (halfLength taps), empty for halfbands
        std::vector<CoeffType> oddInterpolation;    // the same taps scaled by 2

        std::vector<CoeffType> directAllpass;     // IIR sections of the direct branch
        std::vector<CoeffType> delayedAllpass;    // IIR sections of the branch behind the unit delay
        double lowFrequencyDelay = 0.0;           // IIR group delay at DC, in low-rate samples

        AudioBuffer<SampleType> interpolationInput;
        AudioBuffer<SampleType> interpolationOutput;
        AudioBuffer<SampleType> evenInput;
        AudioBuffer<SampleType> oddInput;
        AudioBuffer<SampleType> decimationOutput;

        std::vector<CoeffType> upStates;
        std::vector<CoeffType> downStates;
    };

    //==============================================================================
    static double kaiserBeta (double attenuationDb) noexcept
    {
        if (attenuationDb > 50.0)
            return 0.1102 * (attenuationDb - 8.7);

        if (attenuationDb > 21.0)
            return 0.5842 * std::pow (attenuationDb - 21.0, 0.4) + 0.07886 * (attenuationDb - 21.0);

        return 0.0;
    }

    // Builds the nonzero polyphase branch of a Kaiser-windowed halfband of the
    // given length (which must be 3 mod 4), normalized to a DC gain of exactly 1.
    // Kaiser-windowed linear-phase lowpass of odd length with the given cutoff (fraction
    // of the stage rate), split into its even and odd polyphase branches. Each branch is
    // normalized to a DC gain of exactly 0.5, which also zeroes the response at the stage
    // Nyquist. A halfband (cutoff 0.25) leaves the odd branch with only its center tap.
    static void buildLowpassBranches (int length, double cutoff, double beta, std::vector<double>& even, std::vector<double>& odd)
    {
        const int center = (length - 1) / 2;
        even.assign (static_cast<std::size_t> (center + 1), 0.0);
        odd.assign (static_cast<std::size_t> (center), 0.0);

        const auto tap = [&] (int n)
        {
            const double x = 2.0 * cutoff * (n - center);
            const double sinc = (n == center) ? 1.0 : std::sin (MathConstants<double>::pi * x) / (MathConstants<double>::pi * x);
            return 2.0 * cutoff * sinc * WindowFunctions<double>::kaiser (n, length, beta);
        };

        for (int j = 0; j <= center; ++j)
            even[static_cast<std::size_t> (j)] = tap (2 * j);

        for (int j = 0; j < center; ++j)
            odd[static_cast<std::size_t> (j)] = tap (2 * j + 1);

        for (auto* branch : { &even, &odd })
        {
            double sum = 0.0;

            for (const auto value : *branch)
                sum += value;

            for (auto& value : *branch)
                value *= 0.5 / sum;
        }
    }

    // Worst magnitude of the symmetric lowpass over [stopbandEdge, 0.5] of the stage
    // rate, sampled densely enough to catch every ripple of a filter of this length.
    static double worstStopbandGain (const std::vector<double>& even, const std::vector<double>& odd, double stopbandEdge) noexcept
    {
        const int center = static_cast<int> (odd.size());
        const int length = 2 * center + 1;
        const int numPoints = 8 * length;
        double worst = 0.0;

        for (int p = 0; p <= numPoints; ++p)
        {
            const double omega = MathConstants<double>::twoPi * (stopbandEdge + (0.5 - stopbandEdge) * p / numPoints);
            double response = 0.0;

            for (int j = 0; j <= center; ++j)
                response += even[static_cast<std::size_t> (j)] * std::cos (omega * (2 * j - center));

            for (int j = 0; j < center; ++j)
                response += odd[static_cast<std::size_t> (j)] * std::cos (omega * (2 * j + 1 - center));

            worst = jmax (worst, std::abs (response));
        }

        return worst;
    }

    void designFirStage (Stage& stage, double passband, double stopband) const
    {
        const double attenuation = design.stopbandAttenuationDb;
        const double beta = kaiserBeta (attenuation);
        const double stopbandGain = std::pow (10.0, -attenuation / 20.0);
        const double transition = stopband - passband;
        const double cutoff = (passband + stopband) / 2.0;
        const bool halfband = std::abs (cutoff - 0.25) < 1e-9;

        int length = static_cast<int> (std::ceil ((attenuation - 8.0) / (2.285 * MathConstants<double>::twoPi * transition) + 1.0));
        length = jmax (length, 7);

        const auto roundLength = [halfband] (int n)
        {
            if (halfband)
                while (n % 4 != 3)
                    ++n;
            else if (n % 2 == 0)
                ++n;

            return n;
        };

        length = roundLength (length);
        std::vector<double> even, odd;

        for (;;)
        {
            buildLowpassBranches (length, cutoff, beta, even, odd);

            if (worstStopbandGain (even, odd, stopband) <= stopbandGain || length >= maxFirLength)
                break;

            length = roundLength (length + 2);
        }

        stage.halfLength = (length - 1) / 2;
        stage.halfband = halfband;

        stage.evenDecimation.resize (even.size());
        stage.evenInterpolation.resize (even.size());

        for (std::size_t j = 0; j < even.size(); ++j)
        {
            stage.evenDecimation[j] = static_cast<CoeffType> (even[j]);
            stage.evenInterpolation[j] = static_cast<CoeffType> (2.0 * even[j]);
        }

        stage.oddDecimation.clear();
        stage.oddInterpolation.clear();

        if (! halfband)
        {
            stage.oddDecimation.resize (odd.size());
            stage.oddInterpolation.resize (odd.size());

            for (std::size_t j = 0; j < odd.size(); ++j)
            {
                stage.oddDecimation[j] = static_cast<CoeffType> (odd[j]);
                stage.oddInterpolation[j] = static_cast<CoeffType> (2.0 * odd[j]);
            }
        }

        stage.directAllpass.clear();
        stage.delayedAllpass.clear();
        stage.lowFrequencyDelay = 0.0;
    }

    // Elliptic halfband as two allpass branches (Valenzuela & Constantinides). The
    // odd-numbered sections form the direct branch, the even-numbered ones sit
    // behind the unit delay; both branches share the same group delay at DC.
    void designIirStage (Stage& stage, double transition) const
    {
        const double wt = MathConstants<double>::twoPi * transition;
        const double ds = std::pow (10.0, -design.stopbandAttenuationDb / 20.0);
        const double k = square (std::tan ((MathConstants<double>::pi - wt) / 4.0));
        const double kp = std::sqrt (1.0 - k * k);
        const double e = 0.5 * (1.0 - std::sqrt (kp)) / (1.0 + std::sqrt (kp));
        const double q = e + 2.0 * std::pow (e, 5) + 15.0 * std::pow (e, 9) + 150.0 * std::pow (e, 13);
        const double k1 = ds * ds / (1.0 - ds * ds);

        int order = static_cast<int> (std::ceil (std::log (k1 * k1 / 16.0) / std::log (q)));
        order = jmax (order, 3);

        if (order % 2 == 0)
            ++order;

        const int numSections = (order - 1) / 2;
        stage.directAllpass.clear();
        stage.delayedAllpass.clear();
        stage.lowFrequencyDelay = 0.0;

        for (int i = 1; i <= numSections; ++i)
        {
            double numerator = 0.0;

            for (int m = 0; m < 64; ++m)
            {
                const double delta = ((m % 2 == 0) ? 1.0 : -1.0) * std::pow (q, m * (m + 1)) * std::sin ((2 * m + 1) * MathConstants<double>::pi * i / order);
                numerator += delta;

                if (std::abs (delta) < 1e-100)
                    break;
            }

            numerator *= 2.0 * std::pow (q, 0.25);
            double denominator = 0.0;

            for (int m = 1; m < 64; ++m)
            {
                const double delta = ((m % 2 == 0) ? 1.0 : -1.0) * std::pow (q, m * m) * std::cos (2 * m * MathConstants<double>::pi * i / order);
                denominator += delta;

                if (std::abs (delta) < 1e-100)
                    break;
            }

            denominator = 1.0 + 2.0 * denominator;

            const double w = numerator / denominator;
            const double a = std::sqrt (jmax (0.0, (1.0 - w * w * k) * (1.0 - w * w / k))) / (1.0 + w * w);
            const double alpha = (1.0 - a) / (1.0 + a);

            if (i % 2 == 1)
            {
                stage.directAllpass.push_back (static_cast<CoeffType> (alpha));
                stage.lowFrequencyDelay += (1.0 - alpha) / (1.0 + alpha);
            }
            else
            {
                stage.delayedAllpass.push_back (static_cast<CoeffType> (alpha));
            }
        }

        stage.halfLength = 0;
        stage.halfband = true;
        stage.evenDecimation.clear();
        stage.evenInterpolation.clear();
        stage.oddDecimation.clear();
        stage.oddInterpolation.clear();
    }

    //==============================================================================
    void interpolateFir (Stage& stage, int channel, const SampleType* input, SampleType* output, int count) noexcept
    {
        const int history = stage.halfLength;
        auto* buffer = stage.interpolationInput.getWritePointer (channel);

        FloatVectorOperations::copy (buffer + history, input, count);

        const auto* evenTaps = stage.evenInterpolation.data();
        const auto numEven = stage.evenInterpolation.size();
        const auto* oddTaps = stage.oddInterpolation.data();
        const auto numOdd = stage.oddInterpolation.size();
        const int passThrough = (history - 1) / 2 + 1;

        if (stage.halfband)
        {
            for (int m = 0; m < count; ++m)
            {
                *output++ = dotProduct (evenTaps, buffer + m, numEven);
                *output++ = buffer[m + passThrough];
            }
        }
        else
        {
            for (int m = 0; m < count; ++m)
            {
                *output++ = dotProduct (evenTaps, buffer + m, numEven);
                *output++ = dotProduct (oddTaps, buffer + m + 1, numOdd);
            }
        }

        std::copy (buffer + count, buffer + count + history, buffer);
    }

    void decimateFir (Stage& stage, int channel, const SampleType* input, SampleType* output, int count) noexcept
    {
        const int history = stage.halfLength;
        const int half = count / 2;

        auto* even = stage.evenInput.getWritePointer (channel);
        auto* odd = stage.oddInput.getWritePointer (channel);

        for (int i = 0; i < half; ++i)
        {
            even[history + i] = input[2 * i];
            odd[history + i] = input[2 * i + 1];
        }

        const auto* evenTaps = stage.evenDecimation.data();
        const auto numEven = stage.evenDecimation.size();
        const auto* oddTaps = stage.oddDecimation.data();
        const auto numOdd = stage.oddDecimation.size();
        const int passThrough = (history - 1) / 2;

        if (stage.halfband)
        {
            for (int m = 0; m < half; ++m)
                output[m] = dotProduct (evenTaps, even + m, numEven) + static_cast<SampleType> (0.5) * odd[m + passThrough];
        }
        else
        {
            for (int m = 0; m < half; ++m)
                output[m] = dotProduct (evenTaps, even + m, numEven) + dotProduct (oddTaps, odd + m, numOdd);
        }

        std::copy (even + half, even + half + history, even);
        std::copy (odd + half, odd + half + history, odd);
    }

    // First-order allpass (alpha + z^-1) / (1 + alpha z^-1) in one multiply.
    static forcedinline CoeffType allpassTick (CoeffType alpha, CoeffType input, CoeffType* state) noexcept
    {
        const CoeffType output = state[0] + alpha * (input - state[1]);
        state[0] = input;
        state[1] = output;
        return output;
    }

    static forcedinline CoeffType runAllpassChain (const std::vector<CoeffType>& alphas, CoeffType input, CoeffType* states) noexcept
    {
        for (std::size_t i = 0; i < alphas.size(); ++i)
            input = allpassTick (alphas[i], input, states + 2 * i);

        return input;
    }

    void interpolateIir (Stage& stage, int channel, const SampleType* input, SampleType* output, int count) noexcept
    {
        const auto numSections = stage.directAllpass.size() + stage.delayedAllpass.size();
        auto* direct = stage.upStates.data() + static_cast<std::size_t> (channel) * 2 * numSections;
        auto* delayed = direct + 2 * stage.directAllpass.size();

        for (int m = 0; m < count; ++m)
        {
            const auto x = static_cast<CoeffType> (input[m]);
            *output++ = static_cast<SampleType> (runAllpassChain (stage.directAllpass, x, direct));
            *output++ = static_cast<SampleType> (runAllpassChain (stage.delayedAllpass, x, delayed));
        }
    }

    void decimateIir (Stage& stage, int channel, const SampleType* input, SampleType* output, int count) noexcept
    {
        const auto numSections = stage.directAllpass.size() + stage.delayedAllpass.size();
        auto* direct = stage.downStates.data() + static_cast<std::size_t> (channel) * (2 * numSections + 1);
        auto* delayed = direct + 2 * stage.directAllpass.size();
        auto& previousOdd = direct[2 * numSections];

        const int half = count / 2;

        for (int m = 0; m < half; ++m)
        {
            const auto even = static_cast<CoeffType> (input[2 * m]);
            const auto oddBefore = (m == 0) ? previousOdd : static_cast<CoeffType> (input[2 * m - 1]);

            output[m] = static_cast<SampleType> (CoeffType (0.5) * (runAllpassChain (stage.directAllpass, even, direct)
                                                                    + runAllpassChain (stage.delayedAllpass, oddBefore, delayed)));
        }

        previousOdd = static_cast<CoeffType> (input[count - 1]);
    }

    //==============================================================================
    static constexpr int maxFirLength = 4095;

    Design design;
    std::array<Stage, static_cast<std::size_t> (numStages)> stages;

    AudioBuffer<SampleType> paddedInput;
    AudioBuffer<SampleType> oversampledBuffer;
    int paddingSamples = 0;
    int generationLatency = 0;
    int roundTripLatency = 0;
    int maxChannelCount = 0;
    int maxInputSamples = 0;
    int currentOversampledSize = 0;
    int currentNumChannels = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HalfbandOversampler)
};

//==============================================================================
/** @name Convenience type aliases for common oversampling configurations (100 dB, passband 0.45) */
using HalfbandOversampler2xFloat = HalfbandOversampler<float, 2>;     /**< 2x halfband oversampler, float */
using HalfbandOversampler4xFloat = HalfbandOversampler<float, 4>;     /**< 4x halfband oversampler, float */
using HalfbandOversampler8xFloat = HalfbandOversampler<float, 8>;     /**< 8x halfband oversampler, float */
using HalfbandOversampler16xFloat = HalfbandOversampler<float, 16>;   /**< 16x halfband oversampler, float */
using HalfbandOversampler32xFloat = HalfbandOversampler<float, 32>;   /**< 32x halfband oversampler, float */
using HalfbandOversampler2xDouble = HalfbandOversampler<double, 2>;   /**< 2x halfband oversampler, double */
using HalfbandOversampler4xDouble = HalfbandOversampler<double, 4>;   /**< 4x halfband oversampler, double */
using HalfbandOversampler8xDouble = HalfbandOversampler<double, 8>;   /**< 8x halfband oversampler, double */
using HalfbandOversampler16xDouble = HalfbandOversampler<double, 16>; /**< 16x halfband oversampler, double */
using HalfbandOversampler32xDouble = HalfbandOversampler<double, 32>; /**< 32x halfband oversampler, double */

} // namespace yup
