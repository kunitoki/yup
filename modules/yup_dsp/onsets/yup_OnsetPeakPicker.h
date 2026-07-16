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
    Generic peak-picking onset detector.

    Takes an activation function (one value per frame) and applies
    moving maximum, moving average, and threshold to detect peaks.
    Implements the algorithm from:

    "Evaluating the Online Capabilities of Onset Detection Methods"
    Sebastian Böck, Florian Krebs and Markus Schedl
    Proceedings of ISMIR, 2012.

    This class is algorithm-agnostic - it operates on any float activation
    array regardless of which ODF produced it.

    @see SuperFluxODF, ComplexFluxODF, OnsetDetector
*/
class OnsetPeakPicker
{
public:
    //==============================================================================
    /** Configuration parameters for peak picking. */
    struct Parameters
    {
        /** Detection threshold. Higher = fewer detections. */
        float threshold = 1.1f;

        /** Minimum time between adjacent onsets (seconds). */
        float combineSec = 0.03f;

        /** Past window size for moving average (seconds). */
        float preAvgSec = 0.15f;

        /** Past window size for moving maximum (seconds). */
        float preMaxSec = 0.01f;

        /** Future window size for moving average (seconds).
            Set to 0 for online mode. */
        float postAvgSec = 0.0f;

        /** Future window size for moving maximum (seconds).
            Set to 0 for online mode. */
        float postMaxSec = 0.05f;

        /** Report onsets delayed by this amount (seconds). */
        float delaySec = 0.0f;

        /** If true, postAvgSec and postMaxSec are forced to 0
            (no future information used). */
        bool onlineMode = false;
    };

    //==============================================================================
    /** Default constructor. Call prepare() to initialize. */
    OnsetPeakPicker() = default;

    //==============================================================================
    /**
        Prepares the peak picker.

        @param p   Configuration parameters
        @param fps Frames per second (sample rate of the activation function)
    */
    void prepare (const Parameters& p, float fps);

    //==============================================================================
    /**
        Detects onsets from the activation function.

        @param activations Activation function values (array of numFrames)
        @param numFrames   Number of frames in the activation function
    */
    void detect (const float* activations, int numFrames);

    //==============================================================================
    /**
        Refines onset times to sample-accurate positions using RMS envelope
        analysis and zero-crossing alignment.

        Algorithm (per onset):
         1. Computes a prefix-sum of squared samples for O(1) RMS queries.
         2. Builds a causal RMS envelope (only samples up to each frame are
            used, avoiding early-onset bias from centered windows).
         3. Estimates a noise floor from the 10th-percentile RMS and computes
            a dynamic trigger threshold (noise + threshold × dynamic range).
         4. Walks backward from the peak-RMS frame to find the first frame in
            the above-threshold region leading into the peak.
         5. Interpolates between envelope frames for sub-hop precision.
         6. Searches backward from the candidate for the nearest zero
            crossing (or falls back to the lowest-amplitude sample).

        @param samples       Mono audio sample data
        @param numSamples    Total number of samples
        @param sampleRate    Audio sample rate in Hz
        @param maxRefineSec  Maximum seconds to search forward/backward for
                             the transient (default 0.008s).
        @param threshold     Ratio of local peak RMS below which the signal
                             is considered "silence" (0.0 to 1.0).
                             Higher values = tighter alignment to transient.
                             (default 0.05).
    */
    void refineOnsetTimes (const float* samples, int numSamples, float sampleRate, double maxRefineSec = 0.008, float threshold = 0.05f);

    //==============================================================================
    /** Resets the detected onsets. */
    void reset();

    //==============================================================================
    /** Returns the detected onset times in seconds. */
    const std::vector<double>& getOnsetTimes() const noexcept { return onsetTimes; }

private:
    //==============================================================================
    Parameters params;
    float fps = 200.0f;
    int preAvgLen = 0;
    int preMaxLen = 0;
    int postAvgLen = 0;
    int postMaxLen = 0;
    std::vector<double> onsetTimes;
};

} // namespace yup
