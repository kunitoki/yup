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
LevelProcessor::LevelProcessor()
{
    updateRMSBufferSize();
}

LevelProcessor::~LevelProcessor()
{
}

//==============================================================================
void LevelProcessor::setSampleRate (double newSampleRate)
{
    if (newSampleRate > 0.0 && newSampleRate != sampleRate)
    {
        sampleRate = newSampleRate;
        updateRMSBufferSize();
    }
}

void LevelProcessor::setIntegrationTime (double seconds)
{
    if (seconds > 0.0 && seconds != integrationTime)
    {
        integrationTime = seconds;
        updateRMSBufferSize();
    }
}

void LevelProcessor::setFallTime (double seconds)
{
    if (seconds > 0.0)
        fallTime = seconds;
}

//==============================================================================
void LevelProcessor::reset() noexcept
{
    rmsBufferPos = 0;
    rmsSumSquares = 0.0;

    if (! rmsBuffer.empty())
        std::fill (rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
}

//==============================================================================
void LevelProcessor::processPeak (const float* samples, int numSamples, float& peakOut) noexcept
{
    jassert (samples != nullptr);
    jassert (numSamples >= 0);

    float peak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float absSample = std::abs (samples[i]);
        if (absSample > peak)
            peak = absSample;
    }

    peakOut = peak;
}

void LevelProcessor::processRMS (const float* samples, int numSamples, float& rmsOut) noexcept
{
    jassert (samples != nullptr);
    jassert (numSamples >= 0);

    if (rmsBuffer.empty() || rmsBufferSize == 0)
    {
        // Fallback: simple RMS if buffer not initialized
        double sumSquares = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float sample = samples[i];
            sumSquares += sample * sample;
        }
        rmsOut = numSamples > 0 ? static_cast<float> (std::sqrt (sumSquares / numSamples)) : 0.0f;
        return;
    }

    // Process each sample through circular buffer
    for (int i = 0; i < numSamples; ++i)
    {
        const float sample = samples[i];
        const float sampleSquared = sample * sample;

        // Remove oldest sample from sum
        const float oldSample = rmsBuffer[rmsBufferPos];
        rmsSumSquares -= oldSample;

        // Add new sample to buffer and sum
        rmsBuffer[rmsBufferPos] = sampleSquared;
        rmsSumSquares += sampleSquared;

        // Advance circular buffer position
        ++rmsBufferPos;
        if (rmsBufferPos >= rmsBufferSize)
            rmsBufferPos = 0;
    }

    // Calculate RMS from mean square
    const double meanSquare = rmsSumSquares / rmsBufferSize;
    rmsOut = static_cast<float> (std::sqrt (jmax (0.0, meanSquare)));
}

void LevelProcessor::processPeakWithFall (float currentPeak, double timeDelta, float& peakOut) noexcept
{
    // If new peak is higher, instantly jump to it
    if (currentPeak > peakOut)
    {
        peakOut = currentPeak;
    }
    else
    {
        // Apply linear fall in dB space: 26 dB in fallTime seconds (K-Meter spec)
        const double effectiveFallTime = fallTime > 0.0 ? fallTime : 3.0;
        const float fallRateDbPerSecond = 26.0f / static_cast<float> (effectiveFallTime);
        const float fallAmountDb = static_cast<float> (fallRateDbPerSecond * timeDelta);

        // Convert current peak to dB
        const float peakDb = peakOut > 0.0f ? Decibels::gainToDecibels (peakOut) : -100.0f;

        // Apply linear fall
        const float newPeakDb = peakDb - fallAmountDb;

        // Convert back to linear
        peakOut = Decibels::decibelsToGain (newPeakDb);

        // Never fall below the current peak
        if (currentPeak > peakOut)
            peakOut = currentPeak;
    }
}

//==============================================================================
float LevelProcessor::calculateBallistics (float current, float target, double timeConstant, double timeDelta) noexcept
{
    if (timeConstant <= 0.0 || timeDelta <= 0.0)
        return target;

    // First-order exponential smoothing
    // alpha = 1 - exp(-timeDelta / timeConstant)
    const double alpha = 1.0 - std::exp (-timeDelta / timeConstant);
    return static_cast<float> (current + alpha * (target - current));
}

//==============================================================================
void LevelProcessor::updateRMSBufferSize()
{
    // Calculate buffer size based on integration time
    const int newSize = static_cast<int> (std::ceil (integrationTime * sampleRate));

    if (newSize != rmsBufferSize && newSize > 0)
    {
        rmsBufferSize = newSize;
        rmsBuffer.resize (rmsBufferSize);
        reset();
    }
}

} // namespace yup
