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
    Fractional-delay feedback comb filter.

    The filter uses a power-of-two circular delay line read through a Kaiser-windowed
    sinc of up to 24 taps, clipped feedback resonance, and optional atan output
    saturation. The sinc stays flat and in phase up to near Nyquist, so the teeth keep
    their height and harmonic spacing whatever the fractional delay, where a short
    polynomial interpolator would damp the upper ones by up to tens of decibels under
    strong feedback, and being stateless it modulates without zipper noise. Delays
    shorter than 13 samples use proportionally shorter kernels. The delay can be
    controlled directly in Hertz or from a MIDI note.

    Parameter ranges are normalized for feedback and saturation:
    - frequency: fundamental frequency in Hz
    - feedback: 0..1, internally mapped to a stable feedback gain
    - saturation: 0..1, applied at the filter output

    @tparam SampleType  Type for audio samples (float or double)
    @tparam CoeffType   Type for internal coefficients
*/
template <typename SampleType, typename CoeffType = double>
class CombFilter : public FilterBase<SampleType, CoeffType>
{
public:
    //==============================================================================
    static constexpr std::size_t defaultDelayLineSize = 16384;

    //==============================================================================
    /** Creates a comb filter with the default delay-line size. */
    CombFilter()
    {
        initialiseDelayLine();
        setParameters (static_cast<CoeffType> (440), static_cast<CoeffType> (0), static_cast<CoeffType> (0), this->sampleRate);
    }

    /** Creates a comb filter with a custom power-of-two delay-line size. */
    explicit CombFilter (std::size_t delayLineSize)
        : delayLineSizeInSamples (nextPowerOfTwo (std::max<std::size_t> (4, delayLineSize)))
        , delayLineMask (delayLineSizeInSamples - 1)
    {
        initialiseDelayLine();
        setParameters (static_cast<CoeffType> (440), static_cast<CoeffType> (0), static_cast<CoeffType> (0), this->sampleRate);
    }

    //==============================================================================
    /**
        Sets all comb filter parameters.

        @param frequencyHz  Fundamental frequency in Hz
        @param feedback     Normalized feedback amount in the range 0..1
        @param saturation   Normalized output saturation amount in the range 0..1
        @param sampleRate   Sample rate in Hz
    */
    void setParameters (
        CoeffType frequencyHz,
        CoeffType feedback,
        CoeffType saturation,
        double sampleRate) noexcept
    {
        const auto normalizedFeedback = sanitizeNormalized (feedback);
        const auto normalizedSaturation = sanitizeNormalized (saturation);
        const auto sanitizedSampleRate = sanitizeSampleRate (sampleRate);
        const auto sanitizedFrequency = sanitizeFrequency (frequencyHz, sanitizedSampleRate);

        if (! approximatelyEqual (frequency, sanitizedFrequency)
            || ! approximatelyEqual (feedbackAmount, normalizedFeedback)
            || ! approximatelyEqual (saturationAmount, normalizedSaturation)
            || ! approximatelyEqual (this->sampleRate, static_cast<double> (sanitizedSampleRate)))
        {
            frequency = sanitizedFrequency;
            feedbackAmount = normalizedFeedback;
            saturationAmount = normalizedSaturation;
            this->sampleRate = sanitizedSampleRate;

            updateDerivedParameters();
        }
    }

    /**
        Sets the comb delay from a MIDI note number.

        The conversion uses the equal-tempered A440 mapping.
    */
    void setParametersFromNote (
        CoeffType midiNote,
        CoeffType feedback,
        CoeffType saturation,
        double sampleRate) noexcept
    {
        const auto frequencyHz = static_cast<CoeffType> (440)
                               * std::pow (static_cast<CoeffType> (2), (midiNote - static_cast<CoeffType> (69)) / static_cast<CoeffType> (12));
        setParameters (frequencyHz, feedback, saturation, sampleRate);
    }

    /** Sets the expected signal range used before and after saturation. */
    void setSignalRange (CoeffType range) noexcept
    {
        jassert (range > static_cast<CoeffType> (0));

        signalRange = jmax (range, std::numeric_limits<CoeffType>::min());
        inverseSignalRange = static_cast<CoeffType> (1) / signalRange;
    }

    /** Returns the expected signal range. */
    CoeffType getSignalRange() const noexcept
    {
        return signalRange;
    }

    /** Returns the comb fundamental frequency in Hz. */
    CoeffType getFrequency() const noexcept
    {
        return frequency;
    }

    /** Returns the normalized feedback amount. */
    CoeffType getFeedback() const noexcept
    {
        return feedbackAmount;
    }

    /** Returns the normalized saturation amount. */
    CoeffType getSaturation() const noexcept
    {
        return saturationAmount;
    }

    /** Returns the current target delay in samples. */
    CoeffType getDelayInSamples() const noexcept
    {
        return targetDelaySamples;
    }

    //==============================================================================
    FilterModeType getSupportedModes() const noexcept override
    {
        return FilterMode::peak;
    }

    void reset() noexcept override
    {
        std::fill (delayLine.begin(), delayLine.end(), static_cast<SampleType> (0));
        writeIndex = 0;
        currentDelaySamples = targetDelaySamples;
    }

    void prepare (double sampleRate, int maximumBlockSize) override
    {
        this->sampleRate = sanitizeSampleRate (sampleRate);
        this->maximumBlockSize = maximumBlockSize;

        initialiseDelayLine();
        updateDerivedParameters();
        reset();
    }

    SampleType processSample (SampleType inputSample) noexcept override
    {
        rampDelay();

        const auto input = static_cast<CoeffType> (inputSample) * inverseSignalRange;
        const auto delayedSignal = readDelayedSample (currentDelaySamples);
        const auto delayLineInput = input + clipResonance (delayLineFeedback * delayedSignal);

        delayLine[writeIndex] = static_cast<SampleType> (delayLineInput);
        writeIndex = (writeIndex + 1) & delayLineMask;

        const auto output = saturateOutput (input + delayedSignal * static_cast<CoeffType> (0.5));

        return static_cast<SampleType> (output * signalRange);
    }

    void processBlock (const SampleType* inputBuffer, SampleType* outputBuffer, int numSamples) noexcept override
    {
        if (inputBuffer == nullptr || outputBuffer == nullptr)
            return;

        for (int i = 0; i < numSamples; ++i)
            outputBuffer[i] = processSample (inputBuffer[i]);
    }

    /**
        Returns the linearized comb response at the given frequency.

        Saturation and feedback clipping are nonlinear and intentionally omitted
        from this transfer-function estimate.
    */
    Complex<CoeffType> getComplexResponse (CoeffType responseFrequency) const override
    {
        const auto delayed = getFractionalDelayResponse (responseFrequency);
        const auto denominator = Complex<CoeffType> (static_cast<CoeffType> (1)) - delayLineFeedback * delayed;

        if (std::abs (denominator) <= std::numeric_limits<CoeffType>::epsilon())
            return Complex<CoeffType> (static_cast<CoeffType> (1))
                 + (static_cast<CoeffType> (0.5) * delayed)
                       / (denominator + Complex<CoeffType> (std::numeric_limits<CoeffType>::epsilon()));

        return Complex<CoeffType> (static_cast<CoeffType> (1))
             + (static_cast<CoeffType> (0.5) * delayed) / denominator;
    }

    /**
        Returns the magnitudes the comb's troughs and teeth reach around the given frequency.

        The teeth repeat at every multiple of the comb frequency and narrow as the
        feedback grows, so a response sampled at a few points lands on or between them
        unpredictably. These are the smooth envelopes through their bottoms and tops
        instead, such as for a display too coarse to resolve them. Linearized like
        getComplexResponse().
    */
    Range<CoeffType> getMagnitudeResponseRange (CoeffType responseFrequency) const
    {
        // Over a tooth the delay's phase turns once, tracing a circle centered on the real
        // axis, whose nearest and farthest points from the origin lie on that axis.
        const auto delayGain = std::abs (getFractionalDelayResponse (responseFrequency));
        const auto feedbackGain = delayLineFeedback * delayGain;

        return {
            static_cast<CoeffType> (1) - static_cast<CoeffType> (0.5) * delayGain / (static_cast<CoeffType> (1) + feedbackGain),
            static_cast<CoeffType> (1) + static_cast<CoeffType> (0.5) * delayGain / jmax (std::numeric_limits<CoeffType>::epsilon(), static_cast<CoeffType> (1) - feedbackGain)
        };
    }

private:
    //==============================================================================
    static constexpr CoeffType delayGlideSeconds = static_cast<CoeffType> (0.002);
    static constexpr CoeffType maxDelaySlope = static_cast<CoeffType> (1);
    static constexpr int maxKernelHalfLength = 12;
    static constexpr int kernelPhases = 128;

    /** The taps of the windowed sinc reading a delay, their weights blended between two phases. */
    struct DelayKernel
    {
        std::size_t firstTapDelay;
        int length;
        const float* lowerPhase;
        const float* upperPhase;
        CoeffType blend;

        CoeffType getWeight (int tap) const noexcept
        {
            return static_cast<CoeffType> (lowerPhase[tap]) + blend * static_cast<CoeffType> (upperPhase[tap] - lowerPhase[tap]);
        }
    };

    static std::size_t nextPowerOfTwo (std::size_t value) noexcept
    {
        --value;

        for (std::size_t i = 1; i < sizeof (std::size_t) * 8; i <<= 1)
            value |= value >> i;

        return value + 1;
    }

    static CoeffType sanitizeSampleRate (double sampleRate) noexcept
    {
        return jmax (static_cast<CoeffType> (11025), static_cast<CoeffType> (sampleRate));
    }

    static CoeffType sanitizeNormalized (CoeffType value) noexcept
    {
        return jlimit (static_cast<CoeffType> (0), static_cast<CoeffType> (1), value);
    }

    CoeffType sanitizeFrequency (CoeffType frequencyHz, CoeffType sampleRate) const noexcept
    {
        const auto minFrequency = sampleRate / static_cast<CoeffType> (delayLineSizeInSamples - 1);
        const auto maxFrequency = sampleRate * static_cast<CoeffType> (0.45);

        return jlimit (minFrequency, maxFrequency, frequencyHz);
    }

    static CoeffType fastAtan (CoeffType input) noexcept
    {
        static constexpr auto b = static_cast<CoeffType> (0.596227);

        const auto sign = input < static_cast<CoeffType> (0) ? static_cast<CoeffType> (-1) : static_cast<CoeffType> (1);
        const auto bx = std::abs (b * input);
        const auto numerator = bx + input * input;
        const auto atanFirstQuadrant = numerator / (static_cast<CoeffType> (1) + bx + numerator);

        return sign * atanFirstQuadrant;
    }

    static CoeffType clipResonance (CoeffType input) noexcept
    {
        return std::tanh (input * static_cast<CoeffType> (0.25)) * static_cast<CoeffType> (4);
    }

    void initialiseDelayLine()
    {
        getKernelTable(); // Builds the shared table here rather than on the audio thread.

        if (delayLine.size() != delayLineSizeInSamples)
            delayLine.assign (delayLineSizeInSamples, static_cast<SampleType> (0));
    }

    void updateDerivedParameters() noexcept
    {
        targetDelaySamples = jlimit (
            static_cast<CoeffType> (1),
            static_cast<CoeffType> (delayLineSizeInSamples - 1),
            static_cast<CoeffType> (this->sampleRate) / frequency);
        delayLineFeedback = jmin (static_cast<CoeffType> (0.985), std::sqrt (feedbackAmount));
        delayGlideCoefficient = static_cast<CoeffType> (1) - std::exp (static_cast<CoeffType> (-1) / (delayGlideSeconds * static_cast<CoeffType> (this->sampleRate)));

        if (saturationAmount > static_cast<CoeffType> (0))
        {
            saturationPreScale = square (saturationAmount * static_cast<CoeffType> (5));
            saturationPostScale = static_cast<CoeffType> (1) / fastAtan (saturationPreScale)
                                * std::pow (static_cast<CoeffType> (4), -saturationAmount);
        }
        else
        {
            saturationPreScale = static_cast<CoeffType> (1);
            saturationPostScale = static_cast<CoeffType> (1);
        }

        if (currentDelaySamples <= static_cast<CoeffType> (0))
            currentDelaySamples = targetDelaySamples;
    }

    /** Glides the delay towards its target in octaves, so a sweep pitches smoothly: chasing
        it at a fixed rate whenever the target moves turns long delays into a crackle.
        The glide is capped at one sample per sample, so the read head never runs backwards
        nor faster than double speed, which scrubs long delays into noise.
    */
    void rampDelay() noexcept
    {
        const auto glided = currentDelaySamples * std::pow (targetDelaySamples / currentDelaySamples, delayGlideCoefficient);

        currentDelaySamples += jlimit (-maxDelaySlope, maxDelaySlope, glided - currentDelaySamples);
    }

    static double besselI0 (double x) noexcept
    {
        auto sum = 1.0;
        auto term = 1.0;

        for (int k = 1; k < 32; ++k)
        {
            term *= square (x / (2.0 * k));
            sum += term;
        }

        return sum;
    }

    /** Kernels for every half-length up to the maximum, each tabulated at kernelPhases + 1
        fractions from 0 to 1, as consecutive rows of 2 * halfLength normalized taps.
    */
    static const std::vector<float>& getKernelTable()
    {
        static const auto table = []
        {
            std::vector<float> values;
            values.reserve (static_cast<std::size_t> (maxKernelHalfLength * (maxKernelHalfLength + 1) * (kernelPhases + 1)));

            std::vector<double> taps;

            for (int halfLength = 1; halfLength <= maxKernelHalfLength; ++halfLength)
            {
                const auto beta = 4.0 + halfLength / 4.0;

                for (int phase = 0; phase <= kernelPhases; ++phase)
                {
                    taps.clear();
                    auto sum = 0.0;

                    for (int k = 1 - halfLength; k <= halfLength; ++k)
                    {
                        const auto t = k - phase / static_cast<double> (kernelPhases);
                        const auto sinc = std::abs (t) < 1.0e-9 ? 1.0 : std::sin (MathConstants<double>::pi * t) / (MathConstants<double>::pi * t);
                        const auto window = besselI0 (beta * std::sqrt (jmax (0.0, 1.0 - square (t / halfLength)))) / besselI0 (beta);

                        taps.push_back (sinc * window);
                        sum += taps.back();
                    }

                    for (const auto tap : taps)
                        values.push_back (static_cast<float> (tap / sum));
                }
            }

            return values;
        }();

        return table;
    }

    /** Centers the kernel on the delay, as long as the delay leaves it enough written samples. */
    static DelayKernel getDelayKernel (CoeffType delaySamples) noexcept
    {
        const auto wholeDelay = std::ceil (delaySamples);
        const auto halfLength = jlimit (1, maxKernelHalfLength, static_cast<int> (wholeDelay) - 1);
        const auto phase = (wholeDelay - delaySamples) * static_cast<CoeffType> (kernelPhases);
        const auto row = jmin (kernelPhases - 1, static_cast<int> (phase));
        const auto length = 2 * halfLength;
        const auto* lowerPhase = getKernelTable().data() + halfLength * (halfLength - 1) * (kernelPhases + 1) + row * length;

        return { static_cast<std::size_t> (wholeDelay) + static_cast<std::size_t> (halfLength - 1), length, lowerPhase, lowerPhase + length, phase - static_cast<CoeffType> (row) };
    }

    CoeffType readDelayedSample (CoeffType delaySamples) const noexcept
    {
        const auto kernel = getDelayKernel (delaySamples);
        const auto firstIndex = writeIndex - kernel.firstTapDelay;
        auto sum = static_cast<CoeffType> (0);

        for (int tap = 0; tap < kernel.length; ++tap)
            sum += kernel.getWeight (tap) * static_cast<CoeffType> (delayLine[(firstIndex + static_cast<std::size_t> (tap)) & delayLineMask]);

        return sum;
    }

    CoeffType saturateOutput (CoeffType input) const noexcept
    {
        if (saturationAmount <= static_cast<CoeffType> (0))
            return input;

        return fastAtan (saturationPreScale * input) * saturationPostScale;
    }

    CoeffType getDelayForResponse() const noexcept
    {
        return targetDelaySamples > static_cast<CoeffType> (0)
                 ? targetDelaySamples
                 : jlimit (
                       static_cast<CoeffType> (1),
                       static_cast<CoeffType> (delayLineSizeInSamples - 1),
                       static_cast<CoeffType> (this->sampleRate) / frequency);
    }

    Complex<CoeffType> getFractionalDelayResponse (CoeffType responseFrequency) const noexcept
    {
        const auto kernel = getDelayKernel (getDelayForResponse());
        const auto omega = frequencyToAngular (responseFrequency, static_cast<CoeffType> (this->sampleRate));
        auto response = Complex<CoeffType> (static_cast<CoeffType> (0));

        for (int tap = 0; tap < kernel.length; ++tap)
            response += kernel.getWeight (tap) * polar (static_cast<CoeffType> (1), -omega * static_cast<CoeffType> (kernel.firstTapDelay - static_cast<std::size_t> (tap)));

        return response;
    }

    //==============================================================================
    std::size_t delayLineSizeInSamples = defaultDelayLineSize;
    std::size_t delayLineMask = defaultDelayLineSize - 1;
    std::vector<SampleType> delayLine;
    std::size_t writeIndex = 0;

    CoeffType frequency = static_cast<CoeffType> (440);
    CoeffType feedbackAmount = static_cast<CoeffType> (0);
    CoeffType saturationAmount = static_cast<CoeffType> (0);
    CoeffType signalRange = static_cast<CoeffType> (1);
    CoeffType inverseSignalRange = static_cast<CoeffType> (1);
    CoeffType delayLineFeedback = static_cast<CoeffType> (0);
    CoeffType delayGlideCoefficient = static_cast<CoeffType> (1);
    CoeffType currentDelaySamples = static_cast<CoeffType> (0);
    CoeffType targetDelaySamples = static_cast<CoeffType> (0);
    CoeffType saturationPreScale = static_cast<CoeffType> (1);
    CoeffType saturationPostScale = static_cast<CoeffType> (1);

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CombFilter)
};

//==============================================================================
using CombFilterFloat = CombFilter<float>;
using CombFilterDouble = CombFilter<double>;

} // namespace yup
