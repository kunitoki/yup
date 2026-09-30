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
const Identifier SpectrumAnalyzerComponent::Style::backgroundTopColorId { "SpectrumAnalyzer_backgroundTopColorId" };
const Identifier SpectrumAnalyzerComponent::Style::backgroundBottomColorId { "SpectrumAnalyzer_backgroundBottomColorId" };
const Identifier SpectrumAnalyzerComponent::Style::outlineColorId { "SpectrumAnalyzer_outlineColorId" };
const Identifier SpectrumAnalyzerComponent::Style::fillColorId { "SpectrumAnalyzer_fillColorId" };
const Identifier SpectrumAnalyzerComponent::Style::gridColorId { "SpectrumAnalyzer_gridColorId" };
const Identifier SpectrumAnalyzerComponent::Style::textColorId { "SpectrumAnalyzer_textColorId" };

//==============================================================================
SpectrumAnalyzerComponent::SpectrumAnalyzerComponent (SpectrumAnalyzerState& state)
    : analyzerState (state)
    , scopeData (scopeSize, 0.0f)
    , targetData (scopeSize, 0.0f)
    , fftSize (analyzerState.getFftSize())
{
    initializeFFTBuffers();
    generateWindow();
    updateBinMapping();
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
    stopTimer();
}

//==============================================================================
void SpectrumAnalyzerComponent::initializeFFTBuffers()
{
    fftProcessor = std::make_unique<FFTProcessor<float>> (fftSize);
    fftInputBuffer.resize (fftSize, 0.0f);
    fftOutputBuffer.resize (fftSize * 2, 0.0f);
    windowBuffer.resize (fftSize, 0.0f);

    const int numBins = fftSize / 2 + 1;
    magnitudeBuffer.resize (numBins, 0.0f);
    binLevelBuffer.resize (numBins, 0.0f);
}

void SpectrumAnalyzerComponent::updateBinMapping()
{
    binMapping.setFftParameters (fftSize, sampleRate);
    binMapping.setFrequencyRange (minFrequency, maxFrequency);
    binMapping.setNumDisplayPoints (scopeSize);
}

//==============================================================================
void SpectrumAnalyzerComponent::timerCallback()
{
    updateSpectrum();
}

void SpectrumAnalyzerComponent::updateSpectrum()
{
    if (! isShowing())
        return;

    const double nowSeconds = Time::getMillisecondCounterHiRes() / 1000.0;
    const double elapsedSeconds = lastUpdateSeconds > 0.0 ? jmax (0.0, nowSeconds - lastUpdateSeconds) : 0.0;
    lastUpdateSeconds = nowSeconds;

    // Exponentials compose, so releasing towards the held targets by the elapsed time gives the same
    // falloff at any update rate, whether or not an FFT frame arrived since the previous update.
    const float releaseRate = static_cast<float> (std::exp (-elapsedSeconds / releaseTimeSeconds));
    for (size_t i = 0; i < scopeData.size(); ++i)
        scopeData[i] = targetData[i] + (scopeData[i] - targetData[i]) * releaseRate;

    constexpr int maxFFTsPerFrame = 4;

    // Drop the hops that won't be processed in this update, so the latency never accumulates.
    const int hopSize = analyzerState.getHopSize();
    const int numReady = analyzerState.getNumAvailableSamples();
    if (hopSize > 0 && numReady >= fftSize)
    {
        const int numFrames = (numReady - fftSize) / hopSize + 1;
        for (int i = numFrames - maxFFTsPerFrame; i > 0 && analyzerState.isFFTDataReady(); --i)
            analyzerState.getFFTData (fftInputBuffer.data());
    }

    for (int fftCount = 0; fftCount < maxFFTsPerFrame && analyzerState.isFFTDataReady(); ++fftCount)
    {
        processFFT();
        updateTargets();

        lastFFTSeconds = nowSeconds;
    }

    // Once the audio stops feeding the analyzer, release towards silence.
    const double hopSeconds = sampleRate > 0.0 ? hopSize / sampleRate : 0.0;
    if (nowSeconds - lastFFTSeconds > jmax (2.0 * hopSeconds, 0.1))
        std::fill (targetData.begin(), targetData.end(), 0.0f);

    repaint();
}

void SpectrumAnalyzerComponent::processFFT()
{
    if (! analyzerState.getFFTData (fftInputBuffer.data()))
        return;

    if (needsWindowUpdate)
    {
        needsWindowUpdate = false;

        generateWindow();
    }

    FloatVectorOperations::multiply (fftInputBuffer.data(), windowBuffer.data(), fftInputBuffer.data(), fftSize);

    fftProcessor->performRealFFTForward (fftInputBuffer.data(), fftOutputBuffer.data());

    const int numBins = fftSize / 2 + 1;

    for (int binIndex = 0; binIndex < numBins; ++binIndex)
    {
        const float real = fftOutputBuffer[static_cast<size_t> (binIndex * 2)];
        const float imag = fftOutputBuffer[static_cast<size_t> (binIndex * 2 + 1)];
        const float magnitude = std::sqrt (real * real + imag * imag);

        magnitudeBuffer[static_cast<size_t> (binIndex)] = magnitude;
    }

    for (int binIndex = 0; binIndex < numBins; ++binIndex)
        binLevelBuffer[static_cast<size_t> (binIndex)] = getBinLinearLevel (binIndex);
}

void SpectrumAnalyzerComponent::updateTargets()
{
    const auto aggregation = getBandAggregation();

    for (int i = 0; i < scopeSize; ++i)
    {
        float targetLevel = 0.0f;

        if (isPositiveAndBelow (i, binMapping.getNumDisplayPoints()))
        {
            const float bandLevel = binMapping.getBandLevel (binLevelBuffer, i, aggregation);

            targetLevel = jmap (jlimit (minDecibels, maxDecibels, linearLevelToDecibels (bandLevel)),
                                minDecibels,
                                maxDecibels,
                                0.0f,
                                1.0f);
        }

        const auto index = static_cast<size_t> (i);
        targetData[index] = targetLevel;
        scopeData[index] = jmax (scopeData[index], targetLevel);
    }
}

void SpectrumAnalyzerComponent::generateWindow()
{
    WindowFunctions<float>::generate (currentWindowType, windowBuffer.data(), windowBuffer.size());

    float windowSum = 0.0f;
    float windowPowerSum = 0.0f;
    for (int i = 0; i < fftSize; ++i)
    {
        const float windowValue = windowBuffer[static_cast<size_t> (i)];
        windowSum += windowValue;
        windowPowerSum += windowValue * windowValue;
    }

    windowCoherentGain = windowSum > 0.0f ? windowSum / float (fftSize) : 1.0f;

    equivalentNoiseBandwidthBins = (windowSum > 0.0f && windowPowerSum > 0.0f)
                                     ? (float (fftSize) * windowPowerSum) / (windowSum * windowSum)
                                     : 1.0f;
}

float SpectrumAnalyzerComponent::getBinPeakAmplitude (int binIndex) const noexcept
{
    const int lastBin = fftSize / 2;

    if (! isPositiveAndBelow (binIndex, lastBin + 1))
        return 0.0f;

    const float coherentGain = windowCoherentGain > 0.0f ? windowCoherentGain : 1.0f;
    const float oneSidedScale = (binIndex > 0 && binIndex < lastBin) ? 2.0f : 1.0f;
    const float rawMagnitude = magnitudeBuffer[static_cast<size_t> (binIndex)];

    return (oneSidedScale * rawMagnitude) / (coherentGain * float (fftSize));
}

float SpectrumAnalyzerComponent::getBinRMSAmplitude (int binIndex) const noexcept
{
    const int lastBin = fftSize / 2;
    const float peakAmplitude = getBinPeakAmplitude (binIndex);

    if (binIndex <= 0 || binIndex >= lastBin)
        return peakAmplitude;

    return peakAmplitude * 0.7071067811865475f;
}

float SpectrumAnalyzerComponent::getBinPower (int binIndex) const noexcept
{
    const float rmsAmplitude = getBinRMSAmplitude (binIndex);
    return rmsAmplitude * rmsAmplitude;
}

float SpectrumAnalyzerComponent::getBinPowerSpectralDensity (int binIndex) const noexcept
{
    const float enbwHz = getEquivalentNoiseBandwidthHz();
    return enbwHz > 0.0f ? getBinPower (binIndex) / enbwHz : 0.0f;
}

float SpectrumAnalyzerComponent::getBinLinearLevel (int binIndex) const noexcept
{
    switch (levelMode)
    {
        case LevelMode::peakDecibels:
            return getBinPeakAmplitude (binIndex);

        case LevelMode::rmsDecibels:
            return getBinRMSAmplitude (binIndex);

        case LevelMode::powerDecibels:
            return getBinPower (binIndex);

        case LevelMode::powerSpectralDensity:
            return getBinPowerSpectralDensity (binIndex);
    }

    return getBinPeakAmplitude (binIndex);
}

float SpectrumAnalyzerComponent::linearLevelToDecibels (float level) const noexcept
{
    if (level <= 0.0f)
        return minDecibels;

    return (isPowerMode() ? 10.0f : 20.0f) * std::log10 (level);
}

SpectrumBinMapping::BandAggregation SpectrumAnalyzerComponent::getBandAggregation() const noexcept
{
    switch (levelMode)
    {
        case LevelMode::powerDecibels:
            return SpectrumBinMapping::BandAggregation::sum;

        case LevelMode::powerSpectralDensity:
            return SpectrumBinMapping::BandAggregation::mean;

        case LevelMode::peakDecibels:
        case LevelMode::rmsDecibels:
            break;
    }

    return SpectrumBinMapping::BandAggregation::peak;
}

bool SpectrumAnalyzerComponent::isPowerMode() const noexcept
{
    return levelMode == LevelMode::powerDecibels
        || levelMode == LevelMode::powerSpectralDensity;
}

//==============================================================================
void SpectrumAnalyzerComponent::refreshDisplay ([[maybe_unused]] double lastFrameTimeSeconds)
{
    if (! isTimerRunning())
        updateSpectrum();
}

void SpectrumAnalyzerComponent::paint (Graphics& g)
{
    const auto bounds = getLocalBounds();

    const auto backgroundTop = ApplicationTheme::findComponentColor (*this, Style::backgroundTopColorId).value_or (Color (0xFF1a1a1a));
    const auto backgroundBottom = ApplicationTheme::findComponentColor (*this, Style::backgroundBottomColorId).value_or (Color (0xFF0f0f0f));

    auto backgroundGradient = ColorGradient (backgroundTop, bounds.getTopLeft(), backgroundBottom, bounds.getBottomLeft());
    g.setFillColorGradient (backgroundGradient);
    g.fillAll();

    drawFrequencyGrid (g, bounds);
    drawDecibelGrid (g, bounds);

    g.setStrokeJoin (StrokeJoin::Round);

    if (displayType == DisplayType::filled)
        drawFilledSpectrum (g, bounds);
    else
        drawLinesSpectrum (g, bounds);
}

void SpectrumAnalyzerComponent::drawLinesSpectrum (Graphics& g, const Rectangle<float>& bounds)
{
    if (scopeSize < 3)
        return;

    auto spectrumPath = createSpectrumPath (bounds, false);
    auto filledPath = spectrumPath.createStrokePolygon (4.0f);
    const auto lineColor = ApplicationTheme::findComponentColor (*this, Style::outlineColorId).value_or (Color (0xFF00ff40));

    /*
    g.setFillColor (lineColor.brighter (0.2f));
    g.setFeather (4.0f);
    g.fillPath (filledPath);

    g.setFillColor (lineColor);
    g.setFeather (8.0f);
    g.fillPath (filledPath);

    g.setFillColor (lineColor.brighter (0.2f));
    g.setFeather (4.0f);
    g.fillPath (filledPath);

    g.setStrokeColor (lineColor.withAlpha (0.8f));
    g.setStrokeWidth (2.0f);
    g.strokePath (spectrumPath);

    g.setStrokeColor (lineColor.brighter (0.3f));
    g.setStrokeWidth (1.0f);
    g.strokePath (spectrumPath);
    */

    g.setStrokeColor (lineColor);
    g.setStrokeWidth (1.5f);
    g.strokePath (spectrumPath);
}

void SpectrumAnalyzerComponent::drawFilledSpectrum (Graphics& g, const Rectangle<float>& bounds)
{
    if (scopeSize < 3)
        return;

    // Create filled path that starts and ends properly at baseline
    auto fillPath = createSpectrumPath (bounds, true);

    const auto fillColor = ApplicationTheme::findComponentColor (*this, Style::fillColorId).value_or (Color (0xc000ff40));

    auto gradient = ColorGradient (
        fillColor, bounds.getX(), bounds.getY(), fillColor.withMultipliedAlpha (1.0f / 12.0f), bounds.getX(), bounds.getBottom());
    g.setFillColorGradient (gradient);
    g.fillPath (fillPath);

    // Draw the spectrum outline
    auto spectrumPath = createSpectrumPath (bounds, false);

    g.setStrokeColor (ApplicationTheme::findComponentColor (*this, Style::outlineColorId).value_or (Color (0xFF00ff40)));
    g.setStrokeWidth (1.5f);
    g.strokePath (spectrumPath);
}

void SpectrumAnalyzerComponent::drawFrequencyGrid (Graphics& g, const Rectangle<float>& bounds)
{
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (10.0f);
    const auto gridColor = ApplicationTheme::findComponentColor (*this, Style::gridColorId).value_or (Color (0x60ffffff));
    const auto textColor = ApplicationTheme::findComponentColor (*this, Style::textColorId).value_or (Color (0xFFcccccc));

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
            lineColor = gridColor;
            lineWidth = 1.0f;
            drawLabels = true;
        }
        else if (brightness == 1) // 2x multiples (medium)
        {
            lineColor = gridColor.withMultipliedAlpha (0.5f);
            lineWidth = 0.75f;
        }
        else // 5x multiples (darkest)
        {
            lineColor = gridColor.withMultipliedAlpha (0.25f);
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

            g.setFillColor (textColor);
            float labelX = jmax (x - 20.0f, bounds.getX());
            labelX = jmin (labelX, bounds.getRight() - 40.0f);
            g.fillFittedText (freqText, font, { labelX, bounds.getBottom() - 15.0f, 40.0f, 12.0f }, Justification::center);
        }
    }

    // Draw "Hz" label
    g.setFillColor (textColor.withMultipliedAlpha (0.75f));
    g.fillFittedText ("Hz", font, { bounds.getRight() - 25.0f, bounds.getBottom() - 15.0f, 20.0f, 12.0f }, Justification::center);
}

void SpectrumAnalyzerComponent::drawDecibelGrid (Graphics& g, const Rectangle<float>& bounds)
{
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (10.0f);
    const auto gridColor = ApplicationTheme::findComponentColor (*this, Style::gridColorId).value_or (Color (0x60ffffff));
    const auto textColor = ApplicationTheme::findComponentColor (*this, Style::textColorId).value_or (Color (0xFFcccccc));

    // Draw minor dB grid lines (every 10 dB)
    g.setStrokeColor (gridColor.withMultipliedAlpha (1.0f / 3.0f));
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
    g.setStrokeColor (gridColor.withMultipliedAlpha (2.0f / 3.0f));
    g.setStrokeWidth (1.0f);

    for (float db = minDecibels; db <= maxDecibels; db += 20.0f)
    {
        if (approximatelyEqual (db, minDecibels))
            continue;

        const float y = decibelToY (db, bounds);
        g.strokeLine (bounds.getX(), y, bounds.getRight(), y);

        // Add dB labels on the left side
        String dbText = String (static_cast<int> (db));
        g.setFillColor (textColor);
        g.fillFittedText (dbText, font, { bounds.getX() + 5.0f, y - 6.0f, 30.0f, 12.0f }, Justification::left);
    }

    // Draw "dB" label
    g.setFillColor (textColor.withMultipliedAlpha (0.75f));
    g.fillFittedText ("dB", font, { bounds.getX() + 5.0f, bounds.getY() + 5.0f, 20.0f, 12.0f }, Justification::centerLeft);
}

//==============================================================================
void SpectrumAnalyzerComponent::resized()
{
    // Component has been resized - no specific action needed for now
}

//==============================================================================
Path SpectrumAnalyzerComponent::createSpectrumPath (const Rectangle<float>& bounds, bool closePath) const
{
    Path path;

    const float width = bounds.getWidth();

    if (scopeSize < 2 || width <= 0.0f || bounds.getHeight() <= 0.0f)
        return path;

    // A closed path starts and ends on the baseline, so that it can be filled directly.
    path.startNewSubPath (bounds.getX(),
                          closePath ? bounds.getBottom() : levelToY (getDisplayLevelForPosition (0.0f), bounds));

    // Sample one point per pixel column and interpolate between the smoothed display points, so the
    // outline stays continuous at any component width.
    const int numColumns = jmax (1, roundToInt (width));

    for (int column = 0; column <= numColumns; ++column)
    {
        const float proportion = float (column) / float (numColumns);
        const float level = getDisplayLevelForPosition (proportion * float (scopeSize - 1));

        path.lineTo (bounds.getX() + width * proportion, levelToY (level, bounds));
    }

    if (closePath)
    {
        path.lineTo (bounds.getRight(), bounds.getBottom());
        path.closeSubPath();
    }

    return path;
}

//==============================================================================
void SpectrumAnalyzerComponent::setWindowType (WindowType type)
{
    if (currentWindowType != type)
    {
        currentWindowType = type;
        generateWindow();
        needsWindowUpdate = false;

        repaint();
    }
}

void SpectrumAnalyzerComponent::setUpdateRate (int hz)
{
    if (hz <= 0)
    {
        stopTimer();
        return;
    }

    startTimerHz (hz);
}

int SpectrumAnalyzerComponent::getUpdateRate() const noexcept
{
    return roundToInt (getTimerFrequencyHz());
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

        updateBinMapping();

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

        updateBinMapping();

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

void SpectrumAnalyzerComponent::setLevelMode (LevelMode mode)
{
    if (levelMode != mode)
    {
        levelMode = mode;

        if (! binLevelBuffer.empty())
            for (int binIndex = 0; binIndex < fftSize / 2 + 1; ++binIndex)
                binLevelBuffer[static_cast<size_t> (binIndex)] = getBinLinearLevel (binIndex);

        repaint();
    }
}

float SpectrumAnalyzerComponent::getEquivalentNoiseBandwidthHz() const noexcept
{
    return equivalentNoiseBandwidthBins * (float (sampleRate) / float (fftSize));
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

float SpectrumAnalyzerComponent::levelToY (float level, const Rectangle<float>& bounds) const noexcept
{
    return jmap (jlimit (0.0f, 1.0f, level), 0.0f, 1.0f, bounds.getBottom(), bounds.getY());
}

float SpectrumAnalyzerComponent::getDisplayLevelForPosition (float displayPoint) const noexcept
{
    const float position = jlimit (0.0f, float (scopeSize - 1), displayPoint);
    const int lowerPoint = jlimit (0, scopeSize - 2, (int) std::floor (position));
    const float fraction = position - float (lowerPoint);

    const float lowerLevel = scopeData[static_cast<size_t> (lowerPoint)];
    const float upperLevel = scopeData[static_cast<size_t> (lowerPoint + 1)];

    return lowerLevel + fraction * (upperLevel - lowerLevel);
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
    jassert (isPowerOfTwo (size) && size >= 64 && size <= 65536);

    if (fftSize != size)
    {
        fftSize = size;

        analyzerState.setFftSize (size);

        initializeFFTBuffers();
        generateWindow();
        updateBinMapping();

        repaint();
    }
}

} // namespace yup
