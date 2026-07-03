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

//==============================================================================
/**
    Computes a magnitude spectrogram from audio samples using the Short-Time
    Fourier Transform (STFT).

    Supports:
    - Configurable FFT size, frame rate (FPS), and window type
    - Optional logarithmic magnitude scaling: log10(mul * mag + add)
    - Optional filter bank dimensionality reduction (via FilterBank)
    - Optional Local Group Delay (LGD) computation (needed for ComplexFluxODF)

    The magnitude spectrogram is stored as a flat row-major array
    [numFrames x numBins]. When a filter bank is provided, numBins equals
    filterBank.getNumBands(); otherwise it equals fftSize / 2.

    LGD data, when enabled, is always stored at full FFT resolution
    (fftSize / 2 bins per frame), regardless of whether a filter bank is
    applied to the magnitude data.

    @see FilterBank, SuperFluxODF, ComplexFluxODF
*/
class Spectrogram
{
public:
    //==============================================================================
    /** Configuration parameters for the spectrogram computation. */
    struct Parameters
    {
        /** FFT size in samples (must be power of 2). */
        int fftSize = 2048;

        /** Frames per second. Hop size = sampleRate / fps. */
        int fps = 200;

        /** Window type applied before FFT. Hann is recommended. */
        WindowType windowType = WindowType::hann;

        /** If true, apply log10(logMul * magnitude + logAdd). */
        bool useLog = true;

        /** Multiplier applied before logarithm (must be > 0). */
        float logMul = 1.0f;

        /** Value added before logarithm (must be > 0). */
        float logAdd = 1.0f;

        /** If true, compute Local Group Delay for each frame.
            Required for ComplexFluxODF. */
        bool computeLGD = false;

        /** Optional filter bank. If non-null, magnitude is reduced to
            filterBank->getNumBands() bins per frame. */
        const FilterBank* filterBank = nullptr;
    };

    //==============================================================================
    Spectrogram() = default;
    ~Spectrogram();

    //==============================================================================
    /** Prepares the spectrogram. Must be called before processOffline(). */
    void prepare (const Parameters& params, float sampleRate);

    /** Processes all audio samples at once (offline/batch mode). */
    void processOffline (const float* samples, int numSamples);

    /** Resets all computed data. */
    void reset();

    //==============================================================================
    /** Returns the number of frames in the computed spectrogram. */
    int getNumFrames() const noexcept { return numFrames; }

    /** Returns the number of frequency bins per frame (bands if filtered, fftSize/2 otherwise). */
    int getNumBins() const noexcept { return numBins; }

    /** Returns the hop size in samples. */
    int getHopSize() const noexcept { return hopSize; }

    /** Returns the FFT size. */
    int getFFTSize() const noexcept { return fftSize; }

    /** Returns the sample rate. */
    float getSampleRate() const noexcept { return sampleRate; }

    /** Returns the number of raw FFT bins (fftSize / 2). */
    int getNumRawBins() const noexcept { return fftSize / 2; }

    /** Returns the fps used. */
    int getFps() const noexcept { return params.fps; }

    //==============================================================================
    /** Magnitude data: row-major [numFrames x numBins]. */
    const float* getMagnitudeData() const noexcept { return magnitude.data(); }

    /** LGD data (if computed): row-major [numFrames x fftSize/2]. Always raw resolution. */
    const float* getLGDData() const noexcept { return lgd.data(); }

    /** Window function buffer of length getFFTSize(). */
    const float* getWindowData() const noexcept { return window.data(); }

    /** The filter bank in use, or nullptr. */
    const FilterBank* getFilterBank() const noexcept { return filterBank; }

    /** The parameters used during preparation. */
    const Parameters& getParameters() const noexcept { return params; }

private:
    //==============================================================================
    void computeLGDForFrame (int frameIdx);

    //==============================================================================
    Parameters params;
    float sampleRate = 44100.0f;

    int fftSize = 0;
    int hopSize = 0;
    int numBins = 0;
    int numFrames = 0;

    const FilterBank* filterBank = nullptr;

    std::vector<float> window;
    std::vector<float> magnitude; // [numFrames x numBins]
    std::vector<float> lgd;       // [numFrames x (fftSize/2)]

    FFTProcessor fft;
    std::vector<float> fftInput;
    std::vector<float> fftOutput;
};

} // namespace yup
