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

class FilterBank;
class Spectrogram;

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
    /** Clears the computed activation function values. */
    virtual void reset() = 0;

    //==============================================================================
    /** Returns the activation function values (one per frame). */
    virtual const std::vector<float>& getActivations() const noexcept = 0;

    //==============================================================================
    /** True if this ODF implements the incremental streaming API. */
    virtual bool supportsStreaming() const noexcept { return false; }

    /** Prepares the streaming path for frames of @p numBins bins. Allocates. */
    virtual void prepareStreaming (int numBins) { (void) numBins; }

    /** Computes the activation for one streamed frame. Real-time safe after
        prepareStreaming(). Produces the same values as compute() would for the
        same frame sequence. */
    virtual float computeStreamingFrame (const float* frameMagnitudes, int numBins) noexcept
    {
        (void) frameMagnitudes;
        (void) numBins;
        return 0.0f;
    }

    /** Resets the streaming state. */
    virtual void resetStreaming() noexcept {}
};

} // namespace yup
