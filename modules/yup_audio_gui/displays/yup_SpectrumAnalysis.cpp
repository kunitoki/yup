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
SpectrumAnalysis::SpectrumAnalysis (int numDisplayPoints)
{
    binMapping.setFrequencyRange (minFrequency, maxFrequency);
    binMapping.setNumDisplayPoints (numDisplayPoints);

    allocate();
}

//==============================================================================
bool SpectrumAnalysis::setState (SpectrumAnalyzerState* newState)
{
    if (state == newState)
        return false;

    state = newState;
    syncWithState();

    return true;
}

int SpectrumAnalysis::getFftSize() const noexcept
{
    return state != nullptr ? state->getFftSize() : fftSize;
}

bool SpectrumAnalysis::syncWithState()
{
    const int stateFftSize = getFftSize();

    if (stateFftSize == fftSize)
        return false;

    fftSize = stateFftSize;
    allocate();

    return true;
}

void SpectrumAnalysis::skipStaleFrames (int numFramesToKeep) noexcept
{
    if (state == nullptr)
        return;

    const int numReady = state->getNumAvailableSamples();

    if (numReady < fftSize)
        return;

    const int hopSize = state->getHopSize();
    const int numFrames = (numReady - fftSize) / hopSize + 1;

    if (numFrames > numFramesToKeep)
        state->skipSamples ((numFrames - jmax (0, numFramesToKeep)) * hopSize);
}

bool SpectrumAnalysis::processNextFrame()
{
    // The state only gives a frame of the size of the buffer, so one resized since syncWithState() is left alone
    if (state == nullptr || ! state->getFFTData (Span<float> (fftInputBuffer)))
        return false;

    FloatVectorOperations::multiply (fftInputBuffer.data(), windowBuffer.data(), fftInputBuffer.data(), fftSize);

    fftProcessor.performRealFFTForward (fftInputBuffer.data(), fftOutputBuffer.data());

    const int lastBin = fftSize / 2;
    const float scale = 1.0f / (windowCoherentGain * static_cast<float> (fftSize));

    for (int binIndex = 0; binIndex <= lastBin; ++binIndex)
    {
        const float real = fftOutputBuffer[static_cast<size_t> (binIndex * 2)];
        const float imag = fftOutputBuffer[static_cast<size_t> (binIndex * 2 + 1)];

        // Every bin but DC and Nyquist holds half of its amplitude, the other half being at the negative frequency
        const float oneSidedScale = (binIndex > 0 && binIndex < lastBin) ? 2.0f : 1.0f;
        const float amplitude = oneSidedScale * std::sqrt (real * real + imag * imag) * scale;

        binAmplitudes[static_cast<size_t> (binIndex)] = std::isfinite (amplitude) ? amplitude : 0.0f;
    }

    return true;
}

//==============================================================================
bool SpectrumAnalysis::setWindowType (WindowType type)
{
    if (windowType == type)
        return false;

    windowType = type;
    generateWindow();

    return true;
}

//==============================================================================
bool SpectrumAnalysis::setFrequencyRange (float minFreq, float maxFreq)
{
    minFreq = jmax (1.0f, minFreq);
    maxFreq = jmax (minFreq + 1.0f, maxFreq);

    if (approximatelyEqual (minFrequency, minFreq) && approximatelyEqual (maxFrequency, maxFreq))
        return false;

    minFrequency = minFreq;
    maxFrequency = maxFreq;

    binMapping.setFrequencyRange (minFrequency, maxFrequency);

    return true;
}

float SpectrumAnalysis::frequencyToProportion (float frequency) const noexcept
{
    return std::log10 (frequency / minFrequency) / std::log10 (maxFrequency / minFrequency);
}

//==============================================================================
bool SpectrumAnalysis::setDecibelRange (float minDb, float maxDb)
{
    jassert (maxDb > minDb);

    if (std::isnan (minDb) || std::isnan (maxDb))
        return false;

    maxDb = jlimit (lowestDecibels + 1.0f, highestDecibels, maxDb);
    minDb = jlimit (lowestDecibels, maxDb - 1.0f, minDb);

    if (approximatelyEqual (minDecibels, minDb) && approximatelyEqual (maxDecibels, maxDb))
        return false;

    minDecibels = minDb;
    maxDecibels = maxDb;

    return true;
}

float SpectrumAnalysis::decibelsToProportion (float decibels) const noexcept
{
    return jmap (jlimit (minDecibels, maxDecibels, decibels), minDecibels, maxDecibels, 0.0f, 1.0f);
}

//==============================================================================
bool SpectrumAnalysis::setSampleRate (double newSampleRate)
{
    newSampleRate = jmax (1.0, newSampleRate);

    if (approximatelyEqual (sampleRate, newSampleRate))
        return false;

    sampleRate = newSampleRate;

    binMapping.setFftParameters (fftSize, sampleRate);

    return true;
}

//==============================================================================
void SpectrumAnalysis::allocate()
{
    fftProcessor.setSize (fftSize);

    // The buffers are replaced, not resized: they hold the FFT size and give back what a larger one took, so
    // that they never go beyond the maximum of the state they follow
    fftInputBuffer = std::vector<float> (static_cast<size_t> (fftSize), 0.0f);
    fftOutputBuffer = std::vector<float> (static_cast<size_t> (fftSize) * 2, 0.0f);
    windowBuffer = std::vector<float> (static_cast<size_t> (fftSize), 0.0f);
    binAmplitudes = std::vector<float> (static_cast<size_t> (fftSize / 2 + 1), 0.0f);

    generateWindow();

    binMapping.setFftParameters (fftSize, sampleRate);
}

void SpectrumAnalysis::generateWindow()
{
    WindowFunctions<float>::generate (windowType, windowBuffer.data(), windowBuffer.size());

    float windowSum = 0.0f;
    float windowPowerSum = 0.0f;

    for (const float windowValue : windowBuffer)
    {
        windowSum += windowValue;
        windowPowerSum += windowValue * windowValue;
    }

    const float windowSize = static_cast<float> (fftSize);

    windowCoherentGain = windowSum > 0.0f ? windowSum / windowSize : 1.0f;

    equivalentNoiseBandwidthBins = (windowSum > 0.0f && windowPowerSum > 0.0f)
                                     ? (windowSize * windowPowerSum) / (windowSum * windowSum)
                                     : 1.0f;
}

} // namespace yup
