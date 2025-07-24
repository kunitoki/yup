/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
SpectrumAnalyzerComponent::SpectrumAnalyzerComponent (SpectrumAnalyzerState& state)
    : analyzerState (state)
    , scopeData (scopeSize, 0.0f)
{
    // Sync FFT size with the analyzer state
    fftSize = analyzerState.getFftSize();

    initializeFFTBuffers();
    generateWindow();
    startTimerHz (30); // 30 FPS updates by default
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
    stopTimer();
}

//==============================================================================
void SpectrumAnalyzerComponent::initializeFFTBuffers()
{
    fftProcessor = std::make_unique<FFTProcessor> (fftSize);
    fftInputBuffer.resize (fftSize, 0.0f);
    fftOutputBuffer.resize (fftSize * 2, 0.0f); // Complex output needs 2x space
    windowBuffer.resize (fftSize, 0.0f);

    // Pre-allocate magnitude buffer to avoid allocations during processing
    const int numBins = fftSize / 2 + 1;
    magnitudeBuffer.resize (numBins, 0.0f);
}

//==============================================================================
void SpectrumAnalyzerComponent::timerCallback()
{
    bool hasNewData = false;
    int fftCount = 0;
    const int maxFFTsPerFrame = 8; // Limit to prevent blocking UI thread

    // Process multiple FFT frames with overlap for better responsiveness
    while (analyzerState.isFFTDataReady() && fftCount < maxFFTsPerFrame)
    {
        processFFT();
        hasNewData = true;
        ++fftCount;
    }

    // Always update display to maintain smooth animation
    updateDisplay (hasNewData);
    repaint();
}

void SpectrumAnalyzerComponent::processFFT()
{
    // Get FFT frame from analyzer state
    if (! analyzerState.getFFTData (fftInputBuffer.data()))
        return;

    // Update window if needed
    if (needsWindowUpdate)
    {
        needsWindowUpdate = false;

        generateWindow();
    }

    // Apply window function
    for (int i = 0; i < fftSize; ++i)
        fftInputBuffer[static_cast<size_t> (i)] *= windowBuffer[static_cast<size_t> (i)];

    // Perform FFT
    fftProcessor->performRealFFTForward (fftInputBuffer.data(), fftOutputBuffer.data());

    // Pre-compute magnitudes with window gain compensation
    const int numBins = fftSize / 2 + 1;

    for (int binIndex = 0; binIndex < numBins; ++binIndex)
    {
        const float real = fftOutputBuffer[static_cast<size_t> (binIndex * 2)];
        const float imag = fftOutputBuffer[static_cast<size_t> (binIndex * 2 + 1)];
        const float magnitude = std::sqrt (real * real + imag * imag) * windowGain;

        magnitudeBuffer[static_cast<size_t> (binIndex)] = magnitude;
    }
}

void SpectrumAnalyzerComponent::updateDisplay (bool hasNewFFTData)
{
    // Always apply consistent smoothing to prevent pulsating
    const int numBins = fftSize / 2 + 1;

    // Process display bins
    for (int i = 0; i < scopeSize; ++i)
    {
        float targetLevel = 0.0f;

        if (hasNewFFTData)
        {
            // Calculate frequency range for this display bin
            const float proportion = float (i) / float (scopeSize - 1);
            const float logFreq = logMinFrequency + proportion * (logMaxFrequency - logMinFrequency);
            const float centerFreq = std::pow (10.0f, logFreq);

            // Calculate the frequency range that this display bin represents
            float freqRangeStart, freqRangeEnd;
            if (i == 0)
            {
                freqRangeStart = minFrequency;
                const float nextLogFreq = logMinFrequency + (float (i + 1) / float (scopeSize - 1)) * (logMaxFrequency - logMinFrequency);
                const float nextFreq = std::pow (10.0f, nextLogFreq);
                freqRangeEnd = (centerFreq + nextFreq) * 0.5f;
            }
            else if (i == scopeSize - 1)
            {
                const float prevLogFreq = logMinFrequency + (float (i - 1) / float (scopeSize - 1)) * (logMaxFrequency - logMinFrequency);
                const float prevFreq = std::pow (10.0f, prevLogFreq);
                freqRangeStart = (prevFreq + centerFreq) * 0.5f;
                freqRangeEnd = maxFrequency;
            }
            else
            {
                const float prevLogFreq = logMinFrequency + (float (i - 1) / float (scopeSize - 1)) * (logMaxFrequency - logMinFrequency);
                const float nextLogFreq = logMinFrequency + (float (i + 1) / float (scopeSize - 1)) * (logMaxFrequency - logMinFrequency);
                const float prevFreq = std::pow (10.0f, prevLogFreq);
                const float nextFreq = std::pow (10.0f, nextLogFreq);
                freqRangeStart = (prevFreq + centerFreq) * 0.5f;
                freqRangeEnd = (centerFreq + nextFreq) * 0.5f;
            }

            // Convert frequency range to bin range
            const float startBin = (freqRangeStart * float (fftSize)) / float (sampleRate);
            const float endBin = (freqRangeEnd * float (fftSize)) / float (sampleRate);
            const float binSpan = endBin - startBin;

            float magnitude = 0.0f;

            if (binSpan <= 1.5f)
            {
                // Low frequencies: Use interpolation for smooth transitions
                const float exactBin = (centerFreq * float (fftSize)) / float (sampleRate);
                const int bin1 = jlimit (0, numBins - 1, static_cast<int> (exactBin));
                const int bin2 = jlimit (0, numBins - 1, bin1 + 1);
                const float fraction = exactBin - float (bin1);

                const float mag1 = magnitudeBuffer[static_cast<size_t> (bin1)];
                const float mag2 = magnitudeBuffer[static_cast<size_t> (bin2)];

                // Linear interpolation for smooth low-frequency response
                magnitude = mag1 + fraction * (mag2 - mag1);
            }
            else
            {
                // High frequencies: Aggregate multiple bins using peak-hold
                const int binStart = jlimit (0, numBins - 1, static_cast<int> (startBin));
                const int binEnd = jlimit (0, numBins - 1, static_cast<int> (endBin + 0.5f));

                for (int binIndex = binStart; binIndex <= binEnd; ++binIndex)
                    magnitude = jmax (magnitude, magnitudeBuffer[static_cast<size_t> (binIndex)]);
            }

            // Convert to decibels with proper calibration
            // Account for window function coherent gain losses
            const float windowCompensation = WindowFunctions<float>::getCompensationGain (currentWindowType);

            const float magnitudeDb = magnitude > 0.0f
                                        ? 20.0f * std::log10 ((magnitude * windowCompensation) / float (fftSize))
                                        : minDecibels;

            // Map to display range [0.0, 1.0]
            targetLevel = jmap (jlimit (minDecibels, maxDecibels, magnitudeDb), minDecibels, maxDecibels, 0.0f, 1.0f);
        }

        // Apply peak-hold with time-based release: instant attack, controlled release
        float& currentValue = scopeData[static_cast<size_t> (i)];

        if (hasNewFFTData && targetLevel > currentValue)
        {
            // Immediately use new peak values for zero latency
            currentValue = targetLevel;
        }
        else
        {
            // Calculate release rate based on time
            if (releaseTimeSeconds <= 0.0f)
            {
                // Immediate falloff - use target directly or fast decay
                if (hasNewFFTData)
                    currentValue = targetLevel; // Use new lower value immediately
                else
                    currentValue = 0.0f; // Immediate decay when no data
            }
            else
            {
                // Calculate release rate for desired time constant
                // Rate = exp(-1 / (release_time * update_rate))
                // Use actual timer rate from getUpdateRate()
                const float updateRate = float (getUpdateRate());
                const float releaseRate = std::exp (-1.0f / (releaseTimeSeconds * updateRate));

                if (hasNewFFTData)
                {
                    // New data available but level is lower - decay toward new level
                    currentValue = releaseRate * currentValue + (1.0f - releaseRate) * targetLevel;
                }
                else
                {
                    // No new data - decay toward zero
                    currentValue *= releaseRate;
                }
            }
        }
    }
}

void SpectrumAnalyzerComponent::generateWindow()
{
    WindowFunctions<float>::generate (currentWindowType, windowBuffer.data(), windowBuffer.size());

    // Calculate window gain compensation
    float windowSum = 0.0f;
    for (int i = 0; i < fftSize; ++i)
        windowSum += windowBuffer[static_cast<size_t> (i)];

    // Gain compensation factor to restore energy after windowing
    windowGain = windowSum > 0.0f ? float (fftSize) / windowSum : 1.0f;
}

//==============================================================================
void SpectrumAnalyzerComponent::paint (Graphics& g)
{
    const auto bounds = getLocalBounds();

    // Professional dark background with subtle gradient
    auto backgroundGradient = ColorGradient (
        Color (0xFF1a1a1a), bounds.getTopLeft(), Color (0xFF0f0f0f), bounds.getBottomLeft());
    g.setFillColorGradient (backgroundGradient);
    g.fillAll();

    // Draw grid and labels first
    drawFrequencyGrid (g, bounds);
    drawDecibelGrid (g, bounds);

    // Draw spectrum based on display type
    if (displayType == DisplayType::filled)
        drawFilledSpectrum (g, bounds);
    else
        drawLinesSpectrum (g, bounds);
}

void SpectrumAnalyzerComponent::drawLinesSpectrum (Graphics& g, const Rectangle<float>& bounds)
{
    if (scopeSize < 3)
        return;

    const float firstY = binToY (0, bounds.getHeight());

    Path spectrumPath;
    spectrumPath.startNewSubPath (bounds.getX(), firstY);
    computeSpectrumPath (spectrumPath, bounds, false);

    g.setStrokeColor (Color (0xFF00ff40));
    g.setStrokeWidth (2.0f);
    g.setStrokeJoin (StrokeJoin::Round);
    g.strokePath (spectrumPath);
}

void SpectrumAnalyzerComponent::drawFilledSpectrum (Graphics& g, const Rectangle<float>& bounds)
{
    if (scopeSize < 3)
        return;

    const float firstX = frequencyToX (std::pow (10.0f, logMinFrequency), bounds);
    const float firstY = binToY (0, bounds.getHeight());

    // Create filled path that starts and ends properly at baseline
    Path fillPath;
    fillPath.startNewSubPath (firstX, bounds.getBottom());
    computeSpectrumPath (fillPath, bounds, true);

    auto gradient = ColorGradient (
        Color (0xc000ff40), bounds.getX(), bounds.getY(), Color (0x1000ff40), bounds.getX(), bounds.getBottom());
    g.setFillColorGradient (gradient);
    g.fillPath (fillPath);

    // Draw the spectrum outline
    Path spectrumPath;
    spectrumPath.startNewSubPath (bounds.getX(), firstY);
    computeSpectrumPath (spectrumPath, bounds, false);

    g.setStrokeColor (Color (0xFF00ff40));
    g.setStrokeWidth (1.5f);
    g.setStrokeJoin (StrokeJoin::Round);
    g.strokePath (spectrumPath);
}

void SpectrumAnalyzerComponent::drawFrequencyGrid (Graphics& g, const Rectangle<float>& bounds)
{
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (10.0f);

    // Generate logarithmically spaced grid lines: 1x, 2x, 5x multiples of powers of 10
    const int multipliers[] = { 1, 2, 5 };
    const int powers[] = { 1, 10, 100, 1000, 10000 }; // 10^0 to 10^4

    // Draw grid lines from darkest to brightest
    for (int brightness = 0; brightness < 3; ++brightness)
    {
        Color lineColor;
        float lineWidth;
        bool drawLabels = false;

        if (brightness == 0) // 1x multiples (brightest)
        {
            lineColor = Color (0x60ffffff);
            lineWidth = 1.0f;
            drawLabels = true;
        }
        else if (brightness == 1) // 2x multiples (medium)
        {
            lineColor = Color (0x30ffffff);
            lineWidth = 0.75f;
        }
        else // 5x multiples (darkest)
        {
            lineColor = Color (0x18ffffff);
            lineWidth = 0.5f;
        }

        g.setStrokeColor (lineColor);
        g.setStrokeWidth (lineWidth);

        for (int power = 0; power < 5; ++power)
        {
            float freq = float (multipliers[brightness] * powers[power]);

            if (freq < minFrequency || freq > maxFrequency)
                continue;

            const float x = frequencyToX (freq, bounds);
            g.strokeLine (x, bounds.getY(), x, bounds.getBottom());

            if (! drawLabels)
                continue;

            String freqText;
            if (freq >= 1000.0f)
                freqText = String (freq / 1000.0f, freq == 1000.0f ? 0 : 1) + "k";
            else
                freqText = String (static_cast<int> (freq));

            g.setFillColor (Color (0xFFcccccc));
            float labelX = jmax (x - 20.0f, bounds.getX());
            labelX = jmin (labelX, bounds.getRight() - 40.0f);
            g.fillFittedText (freqText, font, { labelX, bounds.getBottom() - 15.0f, 40.0f, 12.0f }, Justification::center);
        }
    }

    // Draw "Hz" label
    g.setFillColor (Color (0xFF999999));
    g.fillFittedText ("Hz", font, { bounds.getRight() - 25.0f, bounds.getBottom() - 15.0f, 20.0f, 12.0f }, Justification::center);
}

void SpectrumAnalyzerComponent::drawDecibelGrid (Graphics& g, const Rectangle<float>& bounds)
{
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (10.0f);

    // Draw minor dB grid lines (every 10 dB)
    g.setStrokeColor (Color (0x20ffffff));
    g.setStrokeWidth (0.5f);

    for (float db = minDecibels; db <= maxDecibels; db += 10.0f)
    {
        // Skip major grid lines (every 20 dB)
        if (static_cast<int> (db) % 20 != 0)
        {
            const float y = decibelToY (db, bounds);
            g.strokeLine (bounds.getX(), y, bounds.getRight(), y);
        }
    }

    // Draw major dB grid lines with labels (every 20 dB)
    g.setStrokeColor (Color (0x40ffffff));
    g.setStrokeWidth (1.0f);

    for (float db = minDecibels; db <= maxDecibels; db += 20.0f)
    {
        if (approximatelyEqual (db, minDecibels))
            continue;

        const float y = decibelToY (db, bounds);
        g.strokeLine (bounds.getX(), y, bounds.getRight(), y);

        // Add dB labels on the left side
        String dbText = String (static_cast<int> (db));
        g.setFillColor (Color (0xFFcccccc));
        g.fillFittedText (dbText, font, { bounds.getX() + 5.0f, y - 6.0f, 30.0f, 12.0f }, Justification::left);
    }

    // Draw "dB" label
    g.setFillColor (Color (0xFF999999));
    g.fillFittedText ("dB", font, { bounds.getX() + 5.0f, bounds.getY() + 5.0f, 20.0f, 12.0f }, Justification::centerLeft);
}

//==============================================================================
void SpectrumAnalyzerComponent::resized()
{
    // Component has been resized - no specific action needed for now
}

//==============================================================================
void SpectrumAnalyzerComponent::computeSpectrumPath (Path spectrumPath, const Rectangle<float>& bounds, bool closePath)
{
    float lastX = 0.0f;

    // Draw the spectrum curve
    for (int i = 0; i < scopeSize; ++i)
    {
        const float proportion = float (i) / float (scopeSize - 1);
        const float frequency = std::pow (10.0f, logMinFrequency + proportion * (logMaxFrequency - logMinFrequency));
        const float x = frequencyToX (frequency, bounds);
        const float y = binToY (i, bounds.getHeight());

        spectrumPath.lineTo (x, y);

        lastX = x;
    }

    // End at baseline at the last spectrum frequency
    if (closePath)
    {
        spectrumPath.lineTo (lastX, bounds.getBottom());
        spectrumPath.closeSubPath();
    }
}

//==============================================================================
void SpectrumAnalyzerComponent::setWindowType (WindowType type)
{
    if (currentWindowType != type)
    {
        currentWindowType = type;

        needsWindowUpdate = true;
    }
}

void SpectrumAnalyzerComponent::setUpdateRate (int hz)
{
    startTimerHz (jmax (1, hz));
}

int SpectrumAnalyzerComponent::getUpdateRate() const noexcept
{
    return getTimerInterval() > 0 ? 1000 / getTimerInterval() : 0;
}

void SpectrumAnalyzerComponent::setFrequencyRange (float minFreq, float maxFreq)
{
    jassert (minFreq > 0.0f && maxFreq > minFreq);

    if (! approximatelyEqual (minFrequency, minFreq)
        || ! approximatelyEqual (maxFrequency, maxFreq))
    {
        minFrequency = minFreq;
        maxFrequency = maxFreq;
        logMinFrequency = std::log10 (minFreq);
        logMaxFrequency = std::log10 (maxFreq);

        repaint();
    }
}

void SpectrumAnalyzerComponent::setDecibelRange (float minDb, float maxDb)
{
    jassert (maxDb > minDb);

    if (! approximatelyEqual (minDecibels, minDb)
        || ! approximatelyEqual (maxDecibels, maxDb))
    {
        minDecibels = minDb;
        maxDecibels = maxDb;

        repaint();
    }
}

void SpectrumAnalyzerComponent::setSampleRate (double sampleRateToUse)
{
    jassert (sampleRateToUse > 0.0);

    if (! approximatelyEqual (sampleRate, sampleRateToUse))
    {
        sampleRate = sampleRateToUse;

        repaint();
    }
}

void SpectrumAnalyzerComponent::setDisplayType (DisplayType type)
{
    if (displayType != type)
    {
        displayType = type;
        repaint();
    }
}

//==============================================================================
float SpectrumAnalyzerComponent::getFrequencyForBin (int binIndex) const noexcept
{
    return (float (binIndex) * float (sampleRate)) / float (fftSize);
}

int SpectrumAnalyzerComponent::getBinForFrequency (float frequency) const noexcept
{
    return roundToInt ((frequency * float (fftSize)) / float (sampleRate));
}

float SpectrumAnalyzerComponent::frequencyToX (float frequency, const Rectangle<float>& bounds) const noexcept
{
    return jmap (std::log10 (frequency), logMinFrequency, logMaxFrequency, bounds.getX(), bounds.getRight());
}

float SpectrumAnalyzerComponent::binToY (int binIndex, float height) const noexcept
{
    if (isPositiveAndBelow (binIndex, (int) scopeData.size()))
        return jmap (scopeData[static_cast<size_t> (binIndex)], 0.0f, 1.0f, height, 0.0f);

    return 0.0f;
}

float SpectrumAnalyzerComponent::decibelToY (float decibel, const Rectangle<float>& bounds) const noexcept
{
    return jmap (decibel, minDecibels, maxDecibels, bounds.getBottom(), bounds.getY());
}

void SpectrumAnalyzerComponent::setReleaseTimeSeconds (float timeSeconds)
{
    releaseTimeSeconds = jmax (0.1f, timeSeconds);
}

void SpectrumAnalyzerComponent::setOverlapFactor (float overlapFactor)
{
    analyzerState.setOverlapFactor (overlapFactor);
}

float SpectrumAnalyzerComponent::getOverlapFactor() const noexcept
{
    return analyzerState.getOverlapFactor();
}

void SpectrumAnalyzerComponent::setFFTSize (int size)
{
    jassert (isPowerOfTwo (size) && size >= 64 && size <= 16384);

    if (fftSize != size)
    {
        fftSize = size;

        // Update the state - this reinitializes the FIFO
        analyzerState.setFftSize (size);

        initializeFFTBuffers();
        generateWindow();

        repaint();
    }
}

} // namespace yup
