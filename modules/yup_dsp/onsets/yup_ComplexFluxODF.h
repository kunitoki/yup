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
    /** @see OnsetDetectionFunction::reset */
    void reset() override;

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

} // namespace yup
