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

namespace yup
{

//==============================================================================
Spectrogram::~Spectrogram() = default;

//==============================================================================
void Spectrogram::prepare (const Parameters& p, float sr)
{
    jassert (sr > 0.0f);
    jassert ((p.fftSize & (p.fftSize - 1)) == 0 && p.fftSize >= 64);
    jassert (p.fps > 0);
    jassert (p.logAdd > 0.0f);
    jassert (p.logMul > 0.0f);

    params = p;
    sampleRate = sr;

    fftSize = p.fftSize;
    hopSize = jmax (1, static_cast<int> (std::round (sampleRate / static_cast<float> (p.fps))));
    filterBank = p.filterBank;

    numBins = (filterBank != nullptr) ? filterBank->getNumBands() : fftSize / 2;

    fft.setSize (fftSize);

    fftInput.resize (static_cast<std::size_t> (fftSize));
    fftOutput.resize (static_cast<std::size_t> (fftSize) * 2);

    window.resize (static_cast<std::size_t> (fftSize));
    WindowFunctions<float>::generate (p.windowType, window.data(), window.size());

    numFrames = 0;
    magnitude.clear();
    lgd.clear();
}

//==============================================================================
void Spectrogram::reset()
{
    numFrames = 0;
    magnitude.clear();
    lgd.clear();
}

//==============================================================================
void Spectrogram::processOffline (const float* samples, int numSamples)
{
    jassert (samples != nullptr);
    jassert (fftSize > 0 && hopSize > 0);

    numFrames = jmax (1, (numSamples - fftSize / 2 + hopSize - 1) / hopSize);

    const std::size_t magElements = static_cast<std::size_t> (numFrames) * static_cast<std::size_t> (numBins);
    magnitude.assign (magElements, 0.0f);

    const int numRawBins = fftSize / 2;

    if (params.computeLGD)
        lgd.assign (static_cast<std::size_t> (numFrames) * static_cast<std::size_t> (numRawBins), 0.0f);
    else
        lgd.clear();

    std::vector<float> rawMag (static_cast<std::size_t> (numRawBins));
    std::vector<float> filteredFrame;

    if (filterBank != nullptr)
        filteredFrame.resize (static_cast<std::size_t> (numBins));

    for (int frame = 0; frame < numFrames; ++frame)
    {
        const int seek = static_cast<int> (std::round (
            static_cast<double> (frame) * static_cast<double> (hopSize)
            - static_cast<double> (fftSize) * 0.5));

        // --- Extract and window audio frame ---
        std::fill (fftInput.begin(), fftInput.end(), 0.0f);

        for (int i = 0; i < fftSize; ++i)
        {
            const int srcIdx = seek + i;

            if (srcIdx >= 0 && srcIdx < numSamples)
                fftInput[static_cast<std::size_t> (i)] = samples[srcIdx] * window[static_cast<std::size_t> (i)];
        }

        // --- FFT ---
        fft.performRealFFTForward (fftInput.data(), fftOutput.data());

        // --- Magnitude ---
        for (int i = 0; i < numRawBins; ++i)
        {
            const float real = fftOutput[static_cast<std::size_t> (i * 2)];
            const float imag = fftOutput[static_cast<std::size_t> (i * 2 + 1)];
            rawMag[static_cast<std::size_t> (i)] = std::sqrt (real * real + imag * imag);
        }

        // --- LGD ---
        if (params.computeLGD)
            computeLGDForFrame (frame);

        // --- Filter bank + log ---
        const float* frameMag = rawMag.data();

        if (filterBank != nullptr)
        {
            filterBank->applySingleFrame (rawMag.data(), filteredFrame.data());
            frameMag = filteredFrame.data();
        }

        for (int bin = 0; bin < numBins; ++bin)
        {
            float val = frameMag[bin];

            if (params.useLog)
                val = std::log10 (params.logMul * val + params.logAdd);

            magnitude[static_cast<std::size_t> (frame) * static_cast<std::size_t> (numBins)
                      + static_cast<std::size_t> (bin)] = val;
        }
    }
}

//==============================================================================
void Spectrogram::computeLGDForFrame (int frameIdx)
{
    const int numRawBins = fftSize / 2;

    // Compute phase
    std::vector<float> phase (static_cast<std::size_t> (numRawBins));

    for (int i = 0; i < numRawBins; ++i)
    {
        const float real = fftOutput[static_cast<std::size_t> (i * 2)];
        const float imag = fftOutput[static_cast<std::size_t> (i * 2 + 1)];
        phase[static_cast<std::size_t> (i)] = std::atan2 (imag, real);
    }

    // Phase unwrapping along frequency axis
    std::vector<float> unwrapped (static_cast<std::size_t> (numRawBins));
    unwrapped[0] = phase[0];

    for (int i = 1; i < numRawBins; ++i)
    {
        float diff = phase[static_cast<std::size_t> (i)] - phase[static_cast<std::size_t> (i - 1)];
        diff -= std::round (diff / MathConstants<float>::twoPi) * MathConstants<float>::twoPi;
        unwrapped[static_cast<std::size_t> (i)] = unwrapped[static_cast<std::size_t> (i - 1)] + diff;
    }

    // LGD = derivative of unwrapped phase over frequency
    const auto frameOffset = static_cast<std::size_t> (frameIdx) * static_cast<std::size_t> (numRawBins);

    for (int i = 0; i < numRawBins - 1; ++i)
        lgd[frameOffset + static_cast<std::size_t> (i)] = unwrapped[static_cast<std::size_t> (i)]
                                                        - unwrapped[static_cast<std::size_t> (i + 1)];

    lgd[frameOffset + static_cast<std::size_t> (numRawBins - 1)] = 0.0f;
}

} // namespace yup
