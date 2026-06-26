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
    A log-spaced triangular filter bank for dimensionality reduction of magnitude
    spectrograms.

    Maps FFT frequency bins to a smaller set of perceptually-motivated filter bands
    (e.g. quarter-tone resolution = 24 bands per octave). Each band uses a triangular
    filter with rising edge from the previous center frequency, peak at the current
    center, and falling edge to the next center.

    The filter bank matrix has dimensions [numFFTBins x numBands] stored row-major,
    where matrix[fftBin * numBands + band] gives the contribution of that FFT bin
    to that filter band.

    @see Spectrogram, SuperFluxODF
*/
class FilterBank
{
public:
    //==============================================================================
    /** Default constructor. Call build() to initialize the matrix. */
    FilterBank() = default;

    //==============================================================================
    /**
        Builds the filter bank matrix for the given parameters.

        @param bandsPerOctave  Number of bands per octave (e.g. 24 for quarter-tone)
        @param fMin            Minimum frequency in Hz (default 30)
        @param fMax            Maximum frequency in Hz (default 17000, clamped to Nyquist)
        @param numFFTBins      Number of FFT frequency bins (typically fftSize / 2)
        @param sampleRate      Audio sample rate in Hz
        @param equalizeArea    If true, normalize each triangular filter to have area = 1
    */
    void build (int bandsPerOctave, float fMin, float fMax, int numFFTBins, float sampleRate, bool equalizeArea = false);

    //==============================================================================
    /** Returns the number of filter bands. */
    int getNumBands() const noexcept { return numBands; }

    /** Returns the number of FFT bins the filter bank was built for. */
    int getNumFFTBins() const noexcept { return numFFTBins; }

    //==============================================================================
    /**
        Applies the filter bank to a single frame of magnitude data.

        @param magnitudeIn   Input array of length getNumFFTBins()
        @param magnitudeOut  Output array of length getNumBands()
    */
    void applySingleFrame (const float* magnitudeIn, float* magnitudeOut) const noexcept;

    /**
        Applies the filter bank to multiple frames.

        @param spectrogram   Input row-major array [numFrames x numFFTBins]
        @param filtered      Output row-major array [numFrames x numBands]
        @param numFrames     Number of frames
    */
    void applyMultipleFrames (const float* spectrogram, float* filtered, int numFrames) const noexcept;

    //==============================================================================
    /** Returns raw matrix data. Row-major: matrix[fftBin * numBands + band]. */
    const float* getMatrixData() const noexcept { return matrix.data(); }

private:
    //==============================================================================
    static std::vector<float> generateFrequencies (int bandsPerOctave, float fMin, float fMax);

    //==============================================================================
    std::vector<float> matrix;
    int numFFTBins = 0;
    int numBands = 0;
};

} // namespace yup
