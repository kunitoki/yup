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

class Spectrogram;
class FilterBank;

//==============================================================================
/**
    Abstract base class for onset detection functions.

    An onset detection function (ODF) takes a spectrogram and produces a
    one-dimensional activation signal where peaks indicate note onsets.

    @see SuperFluxODF, ComplexFluxODF, OnsetPeakPicker
*/
class OnsetDetectionFunction
{
public:
    //==============================================================================
    /** Destructor. */
    virtual ~OnsetDetectionFunction() = default;

    //==============================================================================
    /**
        Computes the onset detection function from the given spectrogram.

        @param spec  The spectrogram to analyze.
    */
    virtual void compute (const Spectrogram& spec) = 0;

    //==============================================================================
    /** Returns the activation function values (one per frame). */
    virtual const std::vector<float>& getActivations() const noexcept = 0;
};

//==============================================================================
/**
    SuperFlux onset detection function with maximum filter vibrato suppression.

    Implementation of the algorithm described in:

    "Maximum Filter Vibrato Suppression for Onset Detection"
    Sebastian Böck and Gerhard Widmer.
    Proceedings of the 16th International Conference on Digital Audio Effects
    (DAFx-13), Maynooth, Ireland, September 2013.

    The algorithm computes the positive first-order difference of a maximum-filtered
    magnitude spectrogram and sums across all frequency bins per frame.

    @see ComplexFluxODF, OnsetPeakPicker
*/
class SuperFluxODF : public OnsetDetectionFunction
{
public:
    //==============================================================================
    /** Configuration parameters for SuperFluxODF. */
    struct Parameters
    {
        /** Number of frames to look back for difference. 0 = auto-compute from
            the window magnitude ratio. */
        int diffFrames = 0;

        /** Window magnitude ratio threshold for automatic diffFrames computation.
            Used only when diffFrames == 0. */
        float windowMagRatio = 0.5f;

        /** Width of the maximum filter in frequency bins.
            Must be odd. Higher values provide more vibrato suppression. */
        int maxFilterBins = 3;
    };

    //==============================================================================
    /** Default constructor. Call prepare() to initialize. */
    SuperFluxODF() = default;

    //==============================================================================
    /**
        Prepares the ODF with the given parameters.

        @param p          Configuration parameters
        @param window     Window function data (for auto diffFrames computation)
        @param windowSize Length of the window
        @param hopSize    STFT hop size in samples
    */
    void prepare (const Parameters& p, const float* window, int windowSize, int hopSize);

    //==============================================================================
    /** @see OnsetDetectionFunction::compute */
    void compute (const Spectrogram& spec) override;

    //==============================================================================
    /** @see OnsetDetectionFunction::getActivations */
    const std::vector<float>& getActivations() const noexcept override { return activations; }

    //==============================================================================
    /**
        Derives the optimal number of diff frames from a window function.

        @param window     Window function data
        @param windowSize Length of the window
        @param ratio      Window magnitude ratio threshold
        @param hopSize    STFT hop size in samples
    */
    static int deriveDiffFrames (const float* window, int windowSize, float ratio, int hopSize);

private:
    //==============================================================================
    std::vector<float> activations;
    int diffFrames = 1;
    int maxFilterBins = 3;
    int maxFilterHalf = 1;
};

//==============================================================================
// Inline implementations: SuperFluxODF
//==============================================================================

inline void SuperFluxODF::prepare (const Parameters& p, const float* window, int windowSize, int hopSize)
{
    jassert (window != nullptr);
    jassert (windowSize > 0 && hopSize > 0);
    jassert (p.maxFilterBins > 0);

    maxFilterBins = p.maxFilterBins;
    maxFilterHalf = maxFilterBins / 2;

    if (p.diffFrames > 0)
        diffFrames = p.diffFrames;
    else
        diffFrames = deriveDiffFrames (window, windowSize, p.windowMagRatio, hopSize);

    jassert (diffFrames >= 1);

    activations.clear();
}

inline void SuperFluxODF::compute (const Spectrogram& spec)
{
    const int numFrames = spec.getNumFrames();
    const int numBins = spec.getNumBins();

    activations.assign (static_cast<std::size_t> (numFrames), 0.0f);

    if (numFrames <= diffFrames || numBins <= 0)
        return;

    const float* magnitude = spec.getMagnitudeData();

    for (int frame = diffFrames; frame < numFrames; ++frame)
    {
        const int prevFrame = frame - diffFrames;
        float sum = 0.0f;

        for (int band = 0; band < numBins; ++band)
        {
            // Maximum filter with shifting window (from reference implementation)
            int first = band - maxFilterHalf;
            int last = first + maxFilterBins - 1;

            if (first < 0)
            {
                last -= first;
                first = 0;
            }

            if (last >= numBins)
            {
                first = jmax (0, first - (last - numBins + 1));
                last = numBins - 1;
            }

            float prevMax = magnitude[static_cast<std::size_t> (prevFrame) * static_cast<std::size_t> (numBins)
                                      + static_cast<std::size_t> (band)];

            for (int k = first; k <= last; ++k)
            {
                const float val = magnitude[static_cast<std::size_t> (prevFrame) * static_cast<std::size_t> (numBins)
                                            + static_cast<std::size_t> (k)];

                if (val > prevMax)
                    prevMax = val;
            }

            const float diff = magnitude[static_cast<std::size_t> (frame) * static_cast<std::size_t> (numBins)
                                         + static_cast<std::size_t> (band)]
                             - prevMax;

            if (diff > 0.0f)
                sum += diff;
        }

        activations[static_cast<std::size_t> (frame)] = sum;
    }
}

inline int SuperFluxODF::deriveDiffFrames (const float* window, int windowSize, float ratio, int hopSize)
{
    jassert (window != nullptr);
    jassert (ratio >= 0.0f && ratio <= 1.0f);

    int sample = 0;
    while (sample < windowSize / 2 && window[sample] <= ratio)
        ++sample;

    const int diffSamples = windowSize / 2 - sample;
    return jmax (1, static_cast<int> (std::round (static_cast<float> (diffSamples) / static_cast<float> (hopSize))));
}

//==============================================================================
/**
    Complex Flux onset detection function with local group delay (LGD)
    based tremolo suppression.

    Implementation based on:

    "Local group delay based vibrato and tremolo suppression for onset
     detection"
    Sebastian Böck and Gerhard Widmer.
    Proceedings of the 13th International Society for Music Information
    Retrieval Conference (ISMIR), 2013.

    Extends SuperFlux by weighting the difference spectrogram with a mask
    derived from the Local Group Delay of the STFT.

    @see SuperFluxODF, Spectrogram
*/
class ComplexFluxODF : public OnsetDetectionFunction
{
public:
    //==============================================================================
    /** Configuration parameters for ComplexFluxODF. */
    struct Parameters
    {
        /** Number of frames to look back for difference. 0 = auto-compute. */
        int diffFrames = 0;

        /** Window magnitude ratio for auto diffFrames. */
        float windowMagRatio = 0.5f;

        /** Maximum filter width in frequency bins. */
        int maxFilterBins = 3;

        /** Temporal filter size for LGD smoothing.
            0 disables temporal filtering. */
        int temporalFilter = 3;

        /** Origin for the temporal filter (center offset). */
        int temporalOrigin = 0;
    };

    //==============================================================================
    /** Default constructor. Call prepare() to initialize. */
    ComplexFluxODF() = default;

    //==============================================================================
    /**
        Prepares the ODF.

        @param p          Configuration parameters
        @param window     Window function data
        @param windowSize Length of the window
        @param hopSize    STFT hop size
    */
    void prepare (const Parameters& p, const float* window, int windowSize, int hopSize);

    //==============================================================================
    /** @see OnsetDetectionFunction::compute
        Requires that the Spectrogram has computeLGD enabled and
        provides valid LGD data. */
    void compute (const Spectrogram& spec) override;

    //==============================================================================
    const std::vector<float>& getActivations() const noexcept override { return activations; }

private:
    //==============================================================================
    std::vector<float> activations;
    int diffFrames = 1;
    int maxFilterBins = 3;
    int maxFilterHalf = 1;
    int temporalFilter = 3;
    int temporalOrigin = 0;
};

//==============================================================================
// Inline: ComplexFluxODF::prepare
//==============================================================================

inline void ComplexFluxODF::prepare (const Parameters& p, const float* window, int windowSize, int hopSize)
{
    jassert (window != nullptr);
    jassert (windowSize > 0 && hopSize > 0);
    jassert (p.maxFilterBins > 0);
    jassert (p.temporalFilter >= 0);

    maxFilterBins = p.maxFilterBins;
    maxFilterHalf = maxFilterBins / 2;
    temporalFilter = p.temporalFilter;
    temporalOrigin = p.temporalOrigin;

    if (p.diffFrames > 0)
        diffFrames = p.diffFrames;
    else
        diffFrames = SuperFluxODF::deriveDiffFrames (window, windowSize, p.windowMagRatio, hopSize);

    jassert (diffFrames >= 1);
    activations.clear();
}

} // namespace yup
