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
    Complete onset detection pipeline orchestrator.

    Combines a Spectrogram, an OnsetDetectionFunction (SuperFluxODF or
    ComplexFluxODF), and an OnsetPeakPicker into a single convenience class.

    Usage:
    @code
    OnsetDetector detector;

    detector.prepare ({
        .spectrogram   = { .fftSize = 2048, .fps = 200 },
        .useFilterBank = true,
        .useComplexFlux = true,
        .peakPicker    = { .threshold = 0.25f }   // lower for ComplexFlux
    }, 44100.0f);

    detector.processOffline (audioBuffer);

    for (auto t : detector.getOnsetTimes())
        DBG ("Onset at " << t << "s");
    @endcode

    @see SuperFlux, ComplexFlux
*/
class OnsetDetector
{
public:
    //==============================================================================
    /** Aggregated configuration parameters for the full pipeline. */
    struct Parameters
    {
        // Sub-system parameters
        Spectrogram::Parameters spectrogram;
        SuperFluxODF::Parameters superFluxODF;
        ComplexFluxODF::Parameters complexFluxODF;
        OnsetPeakPicker::Parameters peakPicker;

        // Filter bank configuration
        bool useFilterBank = true;
        int bandsPerOctave = 24;
        float fMin = 30.0f;
        float fMax = 17000.0f;
        bool equalizeFilterArea = false;

        // ODF selection
        bool useComplexFlux = false;

        // Onset refinement
        bool refineOnsets = false;
        double refineMaxSec = 0.05;
        float refineThreshold = 0.15f;
    };

    //==============================================================================
    /** Default constructor. Call prepare() to initialize. */
    OnsetDetector() = default;

    //==============================================================================
    /**
        Prepares the onset detector with the given parameters and sample rate.

        Validates all parameters and initializes the internal pipeline components
        (FilterBank, Spectrogram, ODF, PeakPicker).

        @param p          Pipeline parameters
        @param sampleRate Audio sample rate in Hz
    */
    void prepare (const Parameters& p, float sampleRate);

    //==============================================================================
    /**
        Processes an entire audio buffer at once (offline/batch mode).

        For stereo AudioBuffer<float>, channel 0 is used directly. For
        best results, downmix to mono before calling.

        @param buffer Audio buffer to analyze (mono or multi-channel)
    */
    void processOffline (const AudioBuffer<float>& buffer);

    /**
        Processes raw audio samples at once (offline/batch mode).

        @param samples    Mono audio sample data
        @param numSamples Number of samples
    */
    void processOffline (const float* samples, int numSamples);

    //==============================================================================
    /** Resets all computed results. */
    void reset();

    //==============================================================================
    /** Returns the onset detection function activation values (one per frame). */
    const std::vector<float>& getActivationFunction() const noexcept;

    /** Returns the detected onset times in seconds. */
    const std::vector<double>& getOnsetTimes() const noexcept;

    /** Returns the number of analysis frames. */
    int getNumFrames() const noexcept;

    //==============================================================================
    /** Returns a reference to the internal Spectrogram. */
    const Spectrogram& getSpectrogram() const noexcept { return spectrogram; }

    /** Returns the current parameters. */
    const Parameters& getParameters() const noexcept { return params; }

private:
    //==============================================================================
    Spectrogram spectrogram;
    std::unique_ptr<OnsetDetectionFunction> odf;
    OnsetPeakPicker peakPicker;
    FilterBank filterBank;

    Parameters params;
    float sampleRate = 44100.0f;
};

} // namespace yup
