/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
    Linkwitz-Riley crossover filter implementation.

    This class implements the Linkwitz-Riley crossover filter, also known as
    "Butterworth squared". It provides simultaneous lowpass and highpass
    outputs with complementary magnitude responses that sum to unity gain
    and maintain phase coherence.

    The Linkwitz-Riley filter is created by cascading two identical Butterworth
    filters of order N/2, resulting in an overall filter of order N with
    -6dB crossover point and phase alignment between outputs.

    Features:
    - Template-based order specification (2nd, 4th, 8th order)
    - Stereo processing with separate left/right channels
    - Complementary lowpass/highpass outputs
    - Phase-aligned crossover design
    - Efficient cascaded biquad implementation

    @see ButterworthFilter, FilterDesigner
*/
template <typename SampleType, typename CoeffType = double, int Order = 4>
class LinkwitzRileyFilter
{
    static_assert (Order >= 2, "Order must be at least 2");
    static_assert ((Order & 1) == 0, "Order must be even");

    //==============================================================================
    /** Number of cascaded stages (Order/2) */
    static constexpr int numStages = Order / 2;

public:
    //==============================================================================
    /** Default constructor */
    LinkwitzRileyFilter()
        : LinkwitzRileyFilter (static_cast<CoeffType> (1000.0))
    {
    }

    /** Constructor with initial parameters */
    LinkwitzRileyFilter (CoeffType crossoverFreq)
    {
        setParameters (crossoverFreq, 44100.0);

        reset();
    }

    //==============================================================================
    /**
        Sets the crossover parameters.

        @param crossoverFreq  The crossover frequency in Hz
        @param sampleRate     The sample rate in Hz
    */
    void setParameters (CoeffType crossoverFreq, double sampleRate) noexcept
    {
        jassert (crossoverFreq > static_cast<CoeffType> (0.0));
        jassert (sampleRate > 0.0);

        if (! approximatelyEqual (frequency, crossoverFreq) || ! approximatelyEqual (this->sampleRate, sampleRate))
        {
            frequency = crossoverFreq;
            this->sampleRate = sampleRate;
            updateCoefficients();
        }
    }

    /**
        Sets the crossover frequency.

        @param crossoverFreq  The crossover frequency in Hz
    */
    void setFrequency (CoeffType crossoverFreq) noexcept
    {
        jassert (crossoverFreq > static_cast<CoeffType> (0.0));

        if (! approximatelyEqual (frequency, crossoverFreq))
        {
            frequency = crossoverFreq;
            updateCoefficients();
        }
    }

    /**
        Sets the sample rate and recalculates coefficients.

        @param sampleRate  The sample rate in Hz
    */
    void setSampleRate (double sampleRate) noexcept
    {
        jassert (sampleRate > 0.0);

        if (! approximatelyEqual (this->sampleRate, sampleRate))
        {
            this->sampleRate = sampleRate;
            updateCoefficients();
        }
    }

    //==============================================================================
    /**
        Processes a single stereo sample through the crossover.

        @param inputLeft       Input sample for left channel
        @param inputRight      Input sample for right channel
        @param outputLowLeft   Output low-pass sample for left channel
        @param outputLowRight  Output low-pass sample for right channel
        @param outputHighLeft  Output high-pass sample for left channel
        @param outputHighRight Output high-pass sample for right channel
    */
    void processSample (SampleType inputLeft,
                        SampleType inputRight,
                        SampleType& outputLowLeft,
                        SampleType& outputLowRight,
                        SampleType& outputHighLeft,
                        SampleType& outputHighRight) noexcept
    {
        // Initialize outputs with input
        auto lowLeft = static_cast<CoeffType> (inputLeft);
        auto lowRight = static_cast<CoeffType> (inputRight);
        auto highLeft = static_cast<CoeffType> (inputLeft);
        auto highRight = static_cast<CoeffType> (inputRight);

        // Process through first Butterworth cascade (lowpass and highpass)
        for (int stage = 0; stage < numStages; ++stage)
            processStage (stage, lowLeft, lowRight, highLeft, highRight, lowPassStage1, highPassStage1);

        // Process through second Butterworth cascade
        for (int stage = 0; stage < numStages; ++stage)
            processStage (stage, lowLeft, lowRight, highLeft, highRight, lowPassStage2, highPassStage2);

        // Convert back to sample type
        outputLowLeft = static_cast<SampleType> (lowLeft);
        outputLowRight = static_cast<SampleType> (lowRight);
        outputHighLeft = static_cast<SampleType> (highLeft);
        outputHighRight = static_cast<SampleType> (highRight);
    }

    /**
        Processes a buffer of samples through the crossover.

        @param inputLeft       Input buffer for left channel
        @param inputRight      Input buffer for right channel
        @param outputLowLeft   Output low-pass buffer for left channel
        @param outputLowRight  Output low-pass buffer for right channel
        @param outputHighLeft  Output high-pass buffer for left channel
        @param outputHighRight Output high-pass buffer for right channel
        @param numSamples      Number of samples to process
    */
    void processBuffer (const SampleType* inputLeft,
                        const SampleType* inputRight,
                        SampleType* outputLowLeft,
                        SampleType* outputLowRight,
                        SampleType* outputHighLeft,
                        SampleType* outputHighRight,
                        int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            processSample (inputLeft[i],
                          inputRight[i],
                          outputLowLeft[i],
                          outputLowRight[i],
                          outputHighLeft[i],
                          outputHighRight[i]);
        }
    }

    //==============================================================================
    /**
        Resets the internal filter state.
    */
    void reset() noexcept
    {
        for (int stage = 0; stage < numStages; ++stage)
        {
            lowPassStage1.leftChannelStages[stage].reset();
            lowPassStage1.rightChannelStages[stage].reset();
            lowPassStage2.leftChannelStages[stage].reset();
            lowPassStage2.rightChannelStages[stage].reset();

            highPassStage1.leftChannelStages[stage].reset();
            highPassStage1.rightChannelStages[stage].reset();
            highPassStage2.leftChannelStages[stage].reset();
            highPassStage2.rightChannelStages[stage].reset();
        }
    }

    //==============================================================================
    /**
        Returns the current crossover frequency.
    */
    CoeffType getFrequency() const noexcept { return frequency; }

    /**
        Returns the current sample rate.
    */
    double getSampleRate() const noexcept { return sampleRate; }

    /**
        Returns the filter order.
    */
    static constexpr int getOrder() noexcept { return Order; }

private:
    //==============================================================================
    /** Filter stage using Biquad objects */
    struct FilterStage
    {
        std::array<Biquad<CoeffType>, numStages> leftChannelStages;
        std::array<Biquad<CoeffType>, numStages> rightChannelStages;
    };

    //==============================================================================
    void updateCoefficients() noexcept
    {
        if (sampleRate <= 0.0)
            return;

        // Use FilterDesigner to calculate Linkwitz-Riley coefficients
        const int numSections = FilterDesigner<CoeffType>::designLinkwitzRiley (Order, frequency, sampleRate, lowCoeffs, highCoeffs);

        if (numSections != numStages * 2)
            return;

        // Apply coefficients to biquad stages
        for (int stage = 0; stage < numStages; ++stage)
        {
            // Each stage gets two identical coefficients (cascade)
            const auto& lowCoeff = lowCoeffs[stage * 2];     // Both cascades use same coeffs
            const auto& highCoeff = highCoeffs[stage * 2];   // Both cascades use same coeffs

            // Set coefficients for both cascades (identical for Linkwitz-Riley)
            lowPassStage1.leftChannelStages[stage].setCoefficients (lowCoeff);
            lowPassStage1.rightChannelStages[stage].setCoefficients (lowCoeff);
            lowPassStage2.leftChannelStages[stage].setCoefficients (lowCoeff);
            lowPassStage2.rightChannelStages[stage].setCoefficients (lowCoeff);

            highPassStage1.leftChannelStages[stage].setCoefficients (highCoeff);
            highPassStage1.rightChannelStages[stage].setCoefficients (highCoeff);
            highPassStage2.leftChannelStages[stage].setCoefficients (highCoeff);
            highPassStage2.rightChannelStages[stage].setCoefficients (highCoeff);
        }
    }

    void processStage (int stage,
                       CoeffType& lowLeft,
                       CoeffType& lowRight,
                       CoeffType& highLeft,
                       CoeffType& highRight,
                       FilterStage& lowStage,
                       FilterStage& highStage) noexcept
    {
        // Process using Biquad objects
        lowLeft = lowStage.leftChannelStages[stage].processSample (lowLeft);
        lowRight = lowStage.rightChannelStages[stage].processSample (lowRight);
        highLeft = highStage.leftChannelStages[stage].processSample (highLeft);
        highRight = highStage.rightChannelStages[stage].processSample (highRight);
    }

    //==============================================================================
    CoeffType frequency = static_cast<CoeffType> (1000.0);
    double sampleRate = 44100.0;

    FilterStage lowPassStage1, lowPassStage2;
    FilterStage highPassStage1, highPassStage2;

    std::vector<BiquadCoefficients<CoeffType>> lowCoeffs, highCoeffs;

    //==============================================================================
    YUP_LEAK_DETECTOR (LinkwitzRileyFilter)
};

//==============================================================================
/** Convenience type aliases */
template <typename SampleType>
using LinkwitzRiley2Filter = LinkwitzRileyFilter<SampleType, double, 2>;

template <typename SampleType>
using LinkwitzRiley4Filter = LinkwitzRileyFilter<SampleType, double, 4>;

template <typename SampleType>
using LinkwitzRiley8Filter = LinkwitzRileyFilter<SampleType, double, 8>;

} // namespace yup
