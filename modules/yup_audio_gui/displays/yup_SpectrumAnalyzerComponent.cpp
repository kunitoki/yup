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

namespace
{

struct SpectrumAnalyzerComponentIds
{
    static inline const Identifier displayType { "displayType" };
    static inline const Identifier levelMode { "levelMode" };
    static inline const Identifier releaseTime { "releaseTime" };
    static inline const Identifier updateRate { "updateRate" };
    static inline const Identifier levels { "levels" };
};

} // namespace

//==============================================================================
const Identifier SpectrumAnalyzerComponent::Style::backgroundTopColorId { "SpectrumAnalyzer_backgroundTopColorId" };
const Identifier SpectrumAnalyzerComponent::Style::backgroundBottomColorId { "SpectrumAnalyzer_backgroundBottomColorId" };
const Identifier SpectrumAnalyzerComponent::Style::outlineColorId { "SpectrumAnalyzer_outlineColorId" };
const Identifier SpectrumAnalyzerComponent::Style::fillColorId { "SpectrumAnalyzer_fillColorId" };
const Identifier SpectrumAnalyzerComponent::Style::gridColorId { "SpectrumAnalyzer_gridColorId" };
const Identifier SpectrumAnalyzerComponent::Style::textColorId { "SpectrumAnalyzer_textColorId" };

//==============================================================================
const ComponentTypeInfo& SpectrumAnalyzerComponent::getClassTypeInfo()
{
    using A = SpectrumAnalyzerComponent;

    static const auto info = []
    {
        auto builder = ComponentTypeBuilder<A> (Identifier ("SpectrumAnalyzer"));

        builder.color ("backgroundTopColor", Style::backgroundTopColorId)
            .color ("backgroundBottomColor", Style::backgroundBottomColorId)
            .color ("outlineColor", Style::outlineColorId)
            .color ("fillColor", Style::fillColorId)
            .color ("gridColor", Style::gridColorId)
            .color ("textColor", Style::textColorId)
            .enumeration<DisplayType> (
                SpectrumAnalyzerComponentIds::displayType,
                { { "filled", DisplayType::filled },
                  { "lines", DisplayType::lines } },
                &A::getDisplayType,
                &A::setDisplayType,
                "Whether the spectrum is drawn as a filled area or as a line.")
            .enumeration<LevelMode> (
                SpectrumAnalyzerComponentIds::levelMode,
                { { "peakDecibels", LevelMode::peakDecibels },
                  { "rmsDecibels", LevelMode::rmsDecibels },
                  { "powerDecibels", LevelMode::powerDecibels },
                  { "powerSpectralDensity", LevelMode::powerSpectralDensity } },
                &A::getLevelMode,
                &A::setLevelMode,
                "How the levels are measured: the peak or the RMS amplitude, the power summed across each band, or the power spectral density.");

        detail::addSpectrumDisplayProperties (builder);

        return builder.property (SpectrumAnalyzerComponentIds::releaseTime, &A::getReleaseTimeSeconds, &A::setReleaseTimeSeconds, 1.0f, "The time the spectrum takes to fall, in seconds.")
            .property (SpectrumAnalyzerComponentIds::updateRate, &A::getUpdateRate, &A::setUpdateRate, 0, "The updates a second, 0 to follow the display refresh rate.")
            .series (SpectrumAnalyzerComponentIds::levels, &A::getLevels, &A::getLevelsRevision, "The level of each display point from 0 to 1 between minDecibels and maxDecibels, the points evenly spaced along the displayed frequency axis.")
            .build();
    }();

    return info;
}

//==============================================================================
SpectrumAnalyzerComponent::SpectrumAnalyzerComponent()
    : scopeData (scopeSize, 0.0f)
    , targetData (scopeSize, 0.0f)
{
}

SpectrumAnalyzerComponent::SpectrumAnalyzerComponent (SpectrumAnalyzerState& state)
    : SpectrumAnalyzerComponent()
{
    setState (&state);
}

SpectrumAnalyzerComponent::~SpectrumAnalyzerComponent()
{
}

//==============================================================================
void SpectrumAnalyzerComponent::setState (SpectrumAnalyzerState* newState)
{
    if (! analysis.setState (newState))
        return;

    // What is shown is the spectrum of the previous state
    std::fill (scopeData.begin(), scopeData.end(), 0.0f);
    std::fill (targetData.begin(), targetData.end(), 0.0f);
    hasRecentSpectrum = false;
    levelsChanged();

    repaint();
    updateRefreshing (true);
}

//==============================================================================
void SpectrumAnalyzerComponent::updateSpectrum (double elapsedSeconds)
{
    constexpr int maxFFTsPerFrame = 4;

    // A level this close to where it is going is there, which is how the release comes to an end
    constexpr float restingDistance = 1.0e-4f;

    const auto* state = analysis.getState();

    if (state == nullptr || ! isShowing())
    {
        updateRefreshing (false);
        return;
    }

    bool levelsMoved = false;

    const float releaseRate = static_cast<float> (std::exp (-jmax (0.0, elapsedSeconds) / releaseTimeSeconds));
    for (size_t i = 0; i < scopeData.size(); ++i)
    {
        float level = targetData[i] + (scopeData[i] - targetData[i]) * releaseRate;

        if (std::abs (level - targetData[i]) < restingDistance)
            level = targetData[i];

        levelsMoved = levelsMoved || level != scopeData[i];
        scopeData[i] = level;
    }

    analysis.syncWithState();
    analysis.skipStaleFrames (maxFFTsPerFrame);

    bool hasNewSpectrum = false;

    for (int fftCount = 0; fftCount < maxFFTsPerFrame && analysis.processNextFrame(); ++fftCount)
    {
        updateBinLevels();
        levelsMoved = updateTargets() || levelsMoved;

        hasNewSpectrum = true;
    }

    if (hasNewSpectrum)
    {
        secondsSinceSpectrum = 0.0;
        hasRecentSpectrum = true;
    }
    else if (hasRecentSpectrum)
    {
        secondsSinceSpectrum += elapsedSeconds;

        // The audio stopped: the last spectrum falls to nothing
        const double hopSeconds = state->getHopSize() / analysis.getSampleRate();
        if (secondsSinceSpectrum > jmax (2.0 * hopSeconds, 0.1))
        {
            hasRecentSpectrum = false;
            std::fill (targetData.begin(), targetData.end(), 0.0f);
        }
    }

    if (levelsMoved)
    {
        levelsChanged();
        repaint();
    }

    // No spectrum to wait for, no samples to read and no level left to fall: nothing to do until samples come
    const bool isAtRest = ! hasRecentSpectrum && ! state->isFFTDataReady() && scopeData == targetData;

    updateRefreshing (! isAtRest);
}

void SpectrumAnalyzerComponent::updateRefreshing (bool hasWorkToDo)
{
    const bool canRefresh = analysis.getState() != nullptr && isShowing();

    // With nothing to do the analyzer leaves the frames of its window alone, so that the window can skip the
    // ones it has nothing to paint in
    frameAttachment.setActive (canRefresh && hasWorkToDo);

    if (! canRefresh || hasWorkToDo)
        restingTimer.stopTimer();
    else if (! restingTimer.isTimerRunning())
        restingTimer.startTimerHz (restingRefreshRate);
}

void SpectrumAnalyzerComponent::levelsChanged() noexcept
{
    // Never 0, which tells that a revision is not tracked
    if (++levelsRevision == 0)
        ++levelsRevision;
}

void SpectrumAnalyzerComponent::updateBinLevels()
{
    const auto amplitudes = analysis.getBinAmplitudes();
    const size_t lastBin = amplitudes.size() - 1;

    // As the analysis, the levels hold the bins of the FFT size of the state and no more
    if (binLevelBuffer.size() != amplitudes.size())
        binLevelBuffer = std::vector<float> (amplitudes.size());

    for (size_t binIndex = 0; binIndex < amplitudes.size(); ++binIndex)
        binLevelBuffer[binIndex] = getBinLinearLevel (amplitudes[binIndex], binIndex == 0 || binIndex == lastBin);
}

bool SpectrumAnalyzerComponent::updateTargets()
{
    const auto aggregation = getBandAggregation();
    const auto& binMapping = analysis.getBinMapping();

    bool levelsMoved = false;

    for (int i = 0; i < scopeSize; ++i)
    {
        float targetLevel = 0.0f;

        if (isPositiveAndBelow (i, binMapping.getNumDisplayPoints()))
        {
            const float bandLevel = binMapping.getBandLevel (binLevelBuffer, i, aggregation);

            targetLevel = analysis.decibelsToProportion (linearLevelToDecibels (bandLevel));
        }

        const auto index = static_cast<size_t> (i);
        targetData[index] = targetLevel;

        if (targetLevel > scopeData[index])
        {
            scopeData[index] = targetLevel;
            levelsMoved = true;
        }
    }

    return levelsMoved;
}

float SpectrumAnalyzerComponent::getBinLinearLevel (float peakAmplitude, bool isRealBin) const noexcept
{
    if (levelMode == LevelMode::peakDecibels)
        return peakAmplitude;

    // DC and Nyquist are real: their amplitude is their RMS
    const float rmsAmplitude = isRealBin ? peakAmplitude : peakAmplitude * 0.7071067811865475f;

    if (levelMode == LevelMode::rmsDecibels)
        return rmsAmplitude;

    const float power = rmsAmplitude * rmsAmplitude;

    if (levelMode == LevelMode::powerDecibels)
        return power;

    const float enbwHz = getEquivalentNoiseBandwidthHz();
    return enbwHz > 0.0f ? power / enbwHz : 0.0f;
}

float SpectrumAnalyzerComponent::linearLevelToDecibels (float level) const noexcept
{
    if (level <= 0.0f)
        return analysis.getMinDecibels();

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
void SpectrumAnalyzerComponent::refreshDisplay (double lastFrameTimeSeconds)
{
    secondsSinceUpdate += lastFrameTimeSeconds;

    if (updateRate > 0)
    {
        // The frames that come before an update is due are left out. What a frame comes late by is taken off
        // the wait for the next update, so that the rate holds over time
        secondsUntilUpdate -= lastFrameTimeSeconds;

        if (secondsUntilUpdate > 0.0)
            return;

        secondsUntilUpdate = jmax (0.0, secondsUntilUpdate + 1.0 / updateRate);
    }

    updateSpectrum (std::exchange (secondsSinceUpdate, 0.0));
}

void SpectrumAnalyzerComponent::visibilityChanged()
{
    updateRefreshing (true);
}

void SpectrumAnalyzerComponent::parentHierarchyChanged()
{
    updateRefreshing (true);
}

void SpectrumAnalyzerComponent::paint (Graphics& g)
{
    // An analyzer that is painted shows, whatever else told it: one that is neither taking frames nor at rest
    // starts again here
    if (! frameAttachment.isActive() && ! restingTimer.isTimerRunning())
        updateRefreshing (true);

    // A style set on the analyzer paints it from its levels, in place of what follows
    if (auto style = getStyle())
    {
        if (auto theme = ApplicationTheme::getGlobalTheme())
            style->paint (g, *theme, *this);

        return;
    }

    const auto bounds = getLocalBounds();

    const auto backgroundTop = ApplicationTheme::findComponentColor (*this, Style::backgroundTopColorId).value_or (Color (0xff1a1a1a));
    const auto backgroundBottom = ApplicationTheme::findComponentColor (*this, Style::backgroundBottomColorId).value_or (Color (0xff0f0f0f));

    auto backgroundGradient = ColorGradient (backgroundTop, bounds.getTopLeft(), backgroundBottom, bounds.getBottomLeft());
    g.setFillColorGradient (backgroundGradient);
    g.fillAll();

    const auto theme = ApplicationTheme::getGlobalTheme();

    if (gridNeedsUpdate || gridBounds != bounds || gridFont != theme->getDefaultFont())
        updateGrid (bounds, theme->getDefaultFont());

    drawGrid (g, bounds);

    g.setStrokeJoin (StrokeJoin::Round);

    updateSpectrumPaths (bounds);

    if (displayType == DisplayType::filled)
        drawFilledSpectrum (g, bounds);
    else
        drawLinesSpectrum (g);
}

void SpectrumAnalyzerComponent::drawLinesSpectrum (Graphics& g)
{
    const auto lineColor = ApplicationTheme::findComponentColor (*this, Style::outlineColorId).value_or (Color (0xff00ff40));

    g.setStrokeColor (lineColor.withAlpha (0.5f));
    g.setStrokeWidth (4.0f);
    g.setFeather (4.0f);
    g.strokePath (spectrumPath);
    g.setFeather (0.0f);

    g.setStrokeColor (lineColor);
    g.setStrokeWidth (1.5f);
    g.strokePath (spectrumPath);
}

void SpectrumAnalyzerComponent::drawFilledSpectrum (Graphics& g, const Rectangle<float>& bounds)
{
    const auto fillColor = ApplicationTheme::findComponentColor (*this, Style::fillColorId).value_or (Color (0xc000ff40));
    const auto lineColor = ApplicationTheme::findComponentColor (*this, Style::outlineColorId).value_or (Color (0xff00ff40));

    auto gradient = ColorGradient (
        fillColor, bounds.getX(), bounds.getY(), fillColor.withMultipliedAlpha (1.0f / 12.0f), bounds.getX(), bounds.getBottom());
    g.setFillColorGradient (gradient);
    g.fillPath (spectrumFillPath);

    g.setStrokeColor (lineColor.withAlpha (0.5f));
    g.setStrokeWidth (4.0f);
    g.setFeather (4.0f);
    g.strokePath (spectrumPath);
    g.setFeather (0.0f);

    g.setStrokeWidth (1.5f);
    g.strokePath (spectrumPath);
}

void SpectrumAnalyzerComponent::drawGrid (Graphics& g, const Rectangle<float>& bounds)
{
    const auto gridColor = ApplicationTheme::findComponentColor (*this, Style::gridColorId).value_or (Color (0x60ffffff));
    const auto textColor = ApplicationTheme::findComponentColor (*this, Style::textColorId).value_or (Color (0xffcccccc));

    const auto drawLabel = [&g] (const GridLabel& label, Color color)
    {
        if (label.text.isEmpty())
            return;

        g.setFillColor (color);
        g.fillFittedText (label.text, label.bounds);
    };

    for (const auto& line : frequencyLines)
    {
        g.setStrokeColor (gridColor.withMultipliedAlpha (line.alpha));
        g.setStrokeWidth (line.width);
        g.strokeLine (line.position, bounds.getY(), line.position, bounds.getBottom());

        drawLabel (line.label, textColor);
    }

    drawLabel (hertzLabel, textColor.withMultipliedAlpha (0.75f));

    for (const auto& line : decibelLines)
    {
        g.setStrokeColor (gridColor.withMultipliedAlpha (line.alpha));
        g.setStrokeWidth (line.width);
        g.strokeLine (bounds.getX(), line.position, bounds.getRight(), line.position);

        drawLabel (line.label, textColor);
    }

    drawLabel (decibelLabel, textColor.withMultipliedAlpha (0.75f));
}

void SpectrumAnalyzerComponent::updateGrid (const Rectangle<float>& bounds, const Font& defaultFont)
{
    gridBounds = bounds;
    gridFont = defaultFont;
    gridNeedsUpdate = false;

    const auto font = defaultFont.withHeight (10.0f);

    const auto setLabel = [&font] (GridLabel& label, const String& text, const Rectangle<float>& area, Justification justification)
    {
        label.bounds = area;

        auto modifier = label.text.startUpdate();
        modifier.clear();
        modifier.setMaxSize (area.getSize());
        modifier.appendText (text, font);
        modifier.setHorizontalAlign (StyledText::toHorizontalAlign (justification));
        modifier.setVerticalAlign (StyledText::toVerticalAlign (justification));
    };

    // The lines are kept in the order they are drawn in. Their room is taken before any is added: a label
    // that is laid out stays where it is
    constexpr int multipliers[] = { 1, 2, 5 };
    constexpr int powers[] = { 1, 10, 100, 1000, 10000 }; // 10^0 to 10^4
    constexpr float frequencyAlphas[] = { 0.8f, 0.4f, 0.2f };
    constexpr float frequencyWidths[] = { 1.0f, 0.75f, 0.5f };

    frequencyLines.clear();
    frequencyLines.reserve (static_cast<size_t> (numElementsInArray (multipliers) * numElementsInArray (powers)));

    for (int brightness = 0; brightness < numElementsInArray (multipliers); ++brightness)
    {
        for (int power = 0; power < numElementsInArray (powers); ++power)
        {
            const float freq = float (multipliers[brightness] * powers[power]);

            if (freq < analysis.getMinFrequency() || freq > analysis.getMaxFrequency())
                continue;

            auto& line = frequencyLines.emplace_back();
            line.position = frequencyToX (freq, bounds);
            line.alpha = frequencyAlphas[brightness];
            line.width = frequencyWidths[brightness];

            // Only the brightest lines have a label
            if (brightness != 0)
                continue;

            float labelX = jmax (line.position - 20.0f, bounds.getX());
            labelX = jmin (labelX, bounds.getRight() - 40.0f);

            setLabel (line.label,
                      freq >= 1000.0f ? String (freq / 1000.0f, 0) + "k" : String (static_cast<int> (freq)),
                      { labelX, bounds.getBottom() - 15.0f, 40.0f, 12.0f },
                      Justification::center);
        }
    }

    setLabel (hertzLabel, "Hz", { bounds.getRight() - 25.0f, bounds.getBottom() - 15.0f, 20.0f, 12.0f }, Justification::center);

    // The lines sit on the multiples of 10 dB and are counted as integers: a float stepped by 10 stops
    // advancing once it is large enough
    const float minDecibels = analysis.getMinDecibels();
    const int firstStep = static_cast<int> (std::ceil (minDecibels / 10.0f));
    const int lastStep = static_cast<int> (std::floor (analysis.getMaxDecibels() / 10.0f));

    decibelLines.clear();
    decibelLines.reserve (static_cast<size_t> (jmax (0, lastStep - firstStep + 1)));

    for (int step = firstStep; step <= lastStep; ++step)
    {
        if (step % 2 == 0)
            continue;

        auto& line = decibelLines.emplace_back();
        line.position = decibelToY (static_cast<float> (step * 10), bounds);
        line.alpha = 1.0f / 3.0f;
        line.width = 0.5f;
    }

    for (int step = firstStep; step <= lastStep; ++step)
    {
        const int db = step * 10;

        if (step % 2 != 0 || approximatelyEqual (static_cast<float> (db), minDecibels))
            continue;

        auto& line = decibelLines.emplace_back();
        line.position = decibelToY (static_cast<float> (db), bounds);
        line.alpha = 2.0f / 3.0f;
        line.width = 1.0f;

        setLabel (line.label, String (db), { bounds.getX() + 5.0f, line.position - 6.0f, 30.0f, 12.0f }, Justification::left);
    }

    setLabel (decibelLabel, "dB", { bounds.getX() + 5.0f, bounds.getY() + 5.0f, 20.0f, 12.0f }, Justification::centerLeft);
}

//==============================================================================
void SpectrumAnalyzerComponent::resized()
{
}

//==============================================================================
void SpectrumAnalyzerComponent::updateSpectrumPaths (const Rectangle<float>& bounds)
{
    const bool isFilled = displayType == DisplayType::filled;

    if (pathRevision == levelsRevision && pathBounds == bounds && pathIsFilled == isFilled)
        return;

    pathRevision = levelsRevision;
    pathBounds = bounds;
    pathIsFilled = isFilled;

    spectrumPath.clear();
    spectrumFillPath.clear();

    const float width = bounds.getWidth();

    if (width <= 0.0f || bounds.getHeight() <= 0.0f)
        return;

    spectrumPath.startNewSubPath (bounds.getX(), levelToY (getDisplayLevelForPosition (0.0f), bounds));

    if (isFilled)
        spectrumFillPath.startNewSubPath (bounds.getX(), bounds.getBottom());

    const int numColumns = jmax (1, roundToInt (width));

    for (int column = 0; column <= numColumns; ++column)
    {
        const float proportion = float (column) / float (numColumns);
        const float x = bounds.getX() + width * proportion;
        const float y = levelToY (getDisplayLevelForPosition (proportion * float (scopeSize - 1)), bounds);

        spectrumPath.lineTo (x, y);

        if (isFilled)
            spectrumFillPath.lineTo (x, y);
    }

    if (isFilled)
    {
        spectrumFillPath.lineTo (bounds.getRight(), bounds.getBottom());
        spectrumFillPath.closeSubPath();
    }
}

//==============================================================================
void SpectrumAnalyzerComponent::setWindowType (WindowType type)
{
    if (analysis.setWindowType (type))
        repaint();
}

void SpectrumAnalyzerComponent::setUpdateRate (int hz)
{
    updateRate = jmax (0, hz);
    secondsUntilUpdate = updateRate > 0 ? 1.0 / updateRate : 0.0;
}

void SpectrumAnalyzerComponent::setFrequencyRange (float minFreq, float maxFreq)
{
    jassert (minFreq > 0.0f && maxFreq > minFreq);

    if (! analysis.setFrequencyRange (minFreq, maxFreq))
        return;

    gridNeedsUpdate = true;
    repaint();
}

void SpectrumAnalyzerComponent::setDecibelRange (float minDb, float maxDb)
{
    if (! analysis.setDecibelRange (minDb, maxDb))
        return;

    gridNeedsUpdate = true;
    repaint();
}

void SpectrumAnalyzerComponent::setSampleRate (double sampleRateToUse)
{
    jassert (sampleRateToUse > 0.0);

    if (analysis.setSampleRate (sampleRateToUse))
        repaint();
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

        updateBinLevels();

        repaint();
    }
}

float SpectrumAnalyzerComponent::getEquivalentNoiseBandwidthHz() const noexcept
{
    return analysis.getEquivalentNoiseBandwidthBins() * (float (analysis.getSampleRate()) / float (analysis.getFftSize()));
}

//==============================================================================
float SpectrumAnalyzerComponent::getFrequencyForBin (int binIndex) const noexcept
{
    return (float (binIndex) * float (analysis.getSampleRate())) / float (analysis.getFftSize());
}

int SpectrumAnalyzerComponent::getBinForFrequency (float frequency) const noexcept
{
    return roundToInt ((frequency * float (analysis.getFftSize())) / float (analysis.getSampleRate()));
}

float SpectrumAnalyzerComponent::frequencyToX (float frequency, const Rectangle<float>& bounds) const noexcept
{
    return bounds.getX() + bounds.getWidth() * analysis.frequencyToProportion (frequency);
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
    return jmap (decibel, analysis.getMinDecibels(), analysis.getMaxDecibels(), bounds.getBottom(), bounds.getY());
}

void SpectrumAnalyzerComponent::setReleaseTimeSeconds (float timeSeconds)
{
    releaseTimeSeconds = jmax (0.1f, timeSeconds);
}

} // namespace yup
