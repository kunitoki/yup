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

} // namespace yup
