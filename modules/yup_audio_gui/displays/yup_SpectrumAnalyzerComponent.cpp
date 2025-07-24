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
    , fftProcessor (fftSize)
    , fftInputBuffer (fftSize, 0.0f)
    , fftOutputBuffer (fftSize * 2, 0.0f)  // Complex output needs 2x space
    , windowBuffer (fftSize, 0.0f)
    , scopeData (scopeSize, 0.0f)
{
    generateWindow();
    startTimerHz (30);  // 30 FPS updates by default
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
    stopTimer();
}

//==============================================================================
void SpectrumAnalyzerComponent::timerCallback()
{
    if (analyzerState.isFFTDataReady())
    {
        processFFT();
        repaint();
    }
}

void SpectrumAnalyzerComponent::processFFT()
{
    // Get samples from the audio thread FIFO
    if (!analyzerState.getFFTData (fftInputBuffer.data()))
        return;

    // Update window if needed
    if (needsWindowUpdate)
    {
        generateWindow();
        needsWindowUpdate = false;
    }

    // Apply window function
    for (int i = 0; i < fftSize; ++i)
        fftInputBuffer[i] *= windowBuffer[static_cast<size_t> (i)];

    // Perform FFT
    fftProcessor.performRealFFTForward (fftInputBuffer.data(), fftOutputBuffer.data());

    // Convert to magnitude spectrum and map to display
    updateDisplay();
}

void SpectrumAnalyzerComponent::updateDisplay()
{
    const int numBins = fftSize / 2 + 1;
    const float logMin = std::log10 (minFrequency);
    const float logMax = std::log10 (maxFrequency);

    // Process FFT data into logarithmically spaced frequency bins
    for (int i = 0; i < scopeSize; ++i)
    {
        // Calculate the frequency for this display bin using logarithmic spacing
        const float proportion = float (i) / float (scopeSize - 1);
        const float logFreq = logMin + proportion * (logMax - logMin);
        const float frequency = std::pow (10.0f, logFreq);

        // Find the corresponding FFT bin
        const int fftBin = getBinForFrequency (frequency);
        const int fftDataIndex = jlimit (0, numBins - 1, fftBin);

        // Calculate magnitude from complex FFT output
        const float real = fftOutputBuffer[static_cast<size_t> (fftDataIndex * 2)];
        const float imag = fftOutputBuffer[static_cast<size_t> (fftDataIndex * 2 + 1)];
        const float magnitude = std::sqrt (real * real + imag * imag);

        // Convert to decibels with proper normalization
        const float magnitudeDb = magnitude > 0.0f
            ? 20.0f * std::log10 (magnitude / float (fftSize))
            : minDecibels;

        // Map to display range [0.0, 1.0]
        const float level = jmap (jlimit (minDecibels, maxDecibels, magnitudeDb), minDecibels, maxDecibels, 0.0f, 1.0f);

        // Apply smoothing with leaky integrator
        float& currentValue = scopeData[static_cast<size_t> (i)];
        
        if (smoothingFactor <= 0.0f)
        {
            // No smoothing - use current level directly
            currentValue = level;
        }
        else if (smoothingFactor >= 1.0f)
        {
            // Maximum smoothing - pure leaky integrator
            const float alpha = 0.05f; // Low-pass cutoff for very smooth response
            currentValue = alpha * level + (1.0f - alpha) * currentValue;
        }
        else
        {
            // Blend between peak-hold and leaky integrator
            const float alpha = jmap (smoothingFactor, 0.0f, 1.0f, 1.0f, 0.05f);
            const float smoothedLevel = alpha * level + (1.0f - alpha) * currentValue;
            
            // For rising signals, use immediate response; for falling signals, use smoothing
            currentValue = jmax (level, smoothedLevel);
        }
    }
}

void SpectrumAnalyzerComponent::generateWindow()
{
    WindowFunctions<float>::generate (currentWindowType, windowBuffer.data(), windowBuffer.size());
}

//==============================================================================
void SpectrumAnalyzerComponent::paint (Graphics& g)
{
    const auto bounds = getLocalBounds();

    // Professional dark background with subtle gradient
    auto backgroundGradient = ColorGradient(
        Color (0xFF1a1a1a), bounds.getTopLeft(),
        Color (0xFF0f0f0f), bounds.getBottomLeft()
    );
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
    if (scopeSize < 3) return;

    const float firstY = binToY (0, bounds.getHeight());

    Path spectrumPath;
    spectrumPath.startNewSubPath (bounds.getX(), firstY);
    computeSpectrumPath (spectrumPath, bounds, false);

    g.setStrokeColor (Color (0xFF00ff40));
    g.setStrokeWidth (2.0f);
    g.strokePath (spectrumPath);
}

void SpectrumAnalyzerComponent::drawFilledSpectrum (Graphics& g, const Rectangle<float>& bounds)
{
    if (scopeSize < 3) return;

    const float height = bounds.getHeight();
    const float logMin = std::log10 (minFrequency);
    const float logMax = std::log10 (maxFrequency);

    const float firstX = frequencyToX (std::pow (10.0f, logMin), bounds);
    const float firstY = binToY (0, bounds.getHeight());

    // Create filled path that starts and ends properly at baseline
    Path fillPath;
    fillPath.startNewSubPath (firstX, bounds.getBottom());
    computeSpectrumPath (fillPath, bounds, true);

    auto gradient = ColorGradient (
        Color (0xc000ff40), bounds.getX(), bounds.getY(),
        Color (0x1000ff40), bounds.getX(), bounds.getBottom()
    );
    g.setFillColorGradient (gradient);
    g.fillPath (fillPath);

    // Draw the spectrum outline
    Path spectrumPath;
    spectrumPath.startNewSubPath (bounds.getX(), firstY);
    computeSpectrumPath (spectrumPath, bounds, false);

    g.setStrokeColor (Color (0xFF00ff40));
    g.setStrokeWidth (1.5f);
    g.strokePath (spectrumPath);
}

void SpectrumAnalyzerComponent::computeSpectrumPath (Path spectrumPath, const Rectangle<float>& bounds, bool closePath)
{
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();
    const float logMin = std::log10 (minFrequency);
    const float logMax = std::log10 (maxFrequency);
    float prevFrequency = 0.0f;

    // Draw the spectrum curve
    for (int i = 0; i < scopeSize; ++i)
    {
        const float proportion = float (i) / float (scopeSize - 1);
        const float frequency = std::pow (10.0f, logMin + proportion * (logMax - logMin));
        const float x = frequencyToX (frequency, bounds);
        const float y = binToY (i, height);
        
        if (i == 0)
        {
            spectrumPath.lineTo (x, y);
        }
        else
        {
            /*
            const float prevX = frequencyToX (prevFrequency, bounds);
            const float prevY = binToY (i - 1, height);
            const float controlX = (prevX + x) * 0.5f;
            const float controlY = (prevY + y) * 0.5f;
            spectrumPath.quadTo (controlX, controlY, x, y);
            */
            spectrumPath.lineTo (x, y);
        }

        prevFrequency = frequency;
    }
    
    // End at baseline at the last spectrum frequency
    if (closePath)
    {
        const float lastX = frequencyToX (std::pow (10.0f, logMax), bounds);
        spectrumPath.lineTo (lastX, bounds.getBottom());
        spectrumPath.closeSubPath();
    }
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
    minFrequency = minFreq;
    maxFrequency = maxFreq;
    repaint();
}

void SpectrumAnalyzerComponent::setDecibelRange (float minDb, float maxDb)
{
    jassert (maxDb > minDb);
    minDecibels = minDb;
    maxDecibels = maxDb;
    repaint();
}

void SpectrumAnalyzerComponent::setSampleRate (double sampleRateToUse)
{
    jassert (sampleRateToUse > 0.0);
    sampleRate = sampleRateToUse;
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
    // Use logarithmic mapping for frequency
    const float logMin = std::log10 (minFrequency);
    const float logMax = std::log10 (maxFrequency);
    const float logFreq = std::log10 (frequency);

    return jmap (logFreq, logMin, logMax, bounds.getX(), bounds.getRight());
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

void SpectrumAnalyzerComponent::setSmoothingFactor (float factor)
{
    smoothingFactor = jlimit (0.0f, 1.0f, factor);
}

} // namespace yup
