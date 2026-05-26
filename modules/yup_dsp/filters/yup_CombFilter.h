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

    The filter uses a power-of-two circular delay line, cubic Hermite interpolation
    for fractional delay reads, clipped feedback resonance, and optional atan output
    saturation. The delay can be controlled directly in Hertz or from a MIDI note.

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

private:
    //==============================================================================
    static constexpr CoeffType delayRampStep = static_cast<CoeffType> (4);

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

    static CoeffType cubicHermite (
        CoeffType y0,
        CoeffType y1,
        CoeffType y2,
        CoeffType y3,
        CoeffType fraction) noexcept
    {
        const auto c0 = y1;
        const auto c1 = static_cast<CoeffType> (0.5) * (y2 - y0);
        const auto c2 = y0 - static_cast<CoeffType> (2.5) * y1 + static_cast<CoeffType> (2) * y2 - static_cast<CoeffType> (0.5) * y3;
        const auto c3 = static_cast<CoeffType> (0.5) * (y3 - y0) + static_cast<CoeffType> (1.5) * (y1 - y2);

        return ((c3 * fraction + c2) * fraction + c1) * fraction + c0;
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

    void rampDelay() noexcept
    {
        const auto delta = targetDelaySamples - currentDelaySamples;

        if (std::abs (delta) <= delayRampStep)
            currentDelaySamples = targetDelaySamples;
        else
            currentDelaySamples += delta > static_cast<CoeffType> (0) ? delayRampStep : -delayRampStep;
    }

    CoeffType readDelayedSample (CoeffType delaySamples) const noexcept
    {
        const auto readPosition = static_cast<CoeffType> (writeIndex)
                                - delaySamples
                                + static_cast<CoeffType> (delayLineSizeInSamples);
        const auto readIndex = static_cast<std::size_t> (std::floor (readPosition)) & delayLineMask;
        const auto fraction = readPosition - std::floor (readPosition);

        const auto y0 = static_cast<CoeffType> (delayLine[(readIndex - 1) & delayLineMask]);
        const auto y1 = static_cast<CoeffType> (delayLine[readIndex]);
        const auto y2 = static_cast<CoeffType> (delayLine[(readIndex + 1) & delayLineMask]);
        const auto y3 = static_cast<CoeffType> (delayLine[(readIndex + 2) & delayLineMask]);

        return cubicHermite (y0, y1, y2, y3, fraction);
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
        const auto delay = getDelayForResponse();
        const auto readDelay = static_cast<CoeffType> (std::ceil (delay));
        const auto fraction = readDelay - delay;
        const auto fraction2 = fraction * fraction;
        const auto fraction3 = fraction2 * fraction;

        const auto weight0 = static_cast<CoeffType> (-0.5) * fraction + fraction2 - static_cast<CoeffType> (0.5) * fraction3;
        const auto weight1 = static_cast<CoeffType> (1) - static_cast<CoeffType> (2.5) * fraction2 + static_cast<CoeffType> (1.5) * fraction3;
        const auto weight2 = static_cast<CoeffType> (0.5) * fraction + static_cast<CoeffType> (2) * fraction2 - static_cast<CoeffType> (1.5) * fraction3;
        const auto weight3 = static_cast<CoeffType> (-0.5) * fraction2 + static_cast<CoeffType> (0.5) * fraction3;
        const auto omega = frequencyToAngular (responseFrequency, static_cast<CoeffType> (this->sampleRate));

        return weight0 * polar (static_cast<CoeffType> (1), -omega * (readDelay + static_cast<CoeffType> (1)))
             + weight1 * polar (static_cast<CoeffType> (1), -omega * readDelay)
             + weight2 * polar (static_cast<CoeffType> (1), -omega * (readDelay - static_cast<CoeffType> (1)))
             + weight3 * polar (static_cast<CoeffType> (1), -omega * (readDelay - static_cast<CoeffType> (2)));
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
