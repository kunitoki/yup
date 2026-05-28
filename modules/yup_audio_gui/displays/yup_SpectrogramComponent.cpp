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
// SpectrogramColorMap
//==============================================================================

SpectrogramColorMap::SpectrogramColorMap (Type type, int numStops)
    : numColorStops (jmax (2, numStops))
{
    switch (type)
    {
        case Type::heatmap:
            buildHeatmap();
            break;
        case Type::grayscale:
            buildGrayscale();
            break;
        case Type::cool:
            buildCool();
            break;
        case Type::warm:
            buildWarm();
            break;
        case Type::viridis:
            buildViridis();
            break;
    }
}

uint32 SpectrogramColorMap::map (float normalizedMagnitude) const noexcept
{
    const float clamped = jlimit (0.0f, 1.0f, normalizedMagnitude);
    const float index = clamped * static_cast<float> (numColorStops - 1);
    const int i0 = static_cast<int> (index);
    const int i1 = jmin (i0 + 1, numColorStops - 1);
    const float frac = index - static_cast<float> (i0);

    const uint32 c0 = colorTable[static_cast<size_t> (i0)];
    const uint32 c1 = colorTable[static_cast<size_t> (i1)];

    const uint8 a = static_cast<uint8> (
        static_cast<float> ((c0 >> 24) & 0xFF) * (1.0f - frac) + static_cast<float> ((c1 >> 24) & 0xFF) * frac);
    const uint8 r = static_cast<uint8> (
        static_cast<float> ((c0 >> 16) & 0xFF) * (1.0f - frac) + static_cast<float> ((c1 >> 16) & 0xFF) * frac);
    const uint8 g = static_cast<uint8> (
        static_cast<float> ((c0 >> 8) & 0xFF) * (1.0f - frac) + static_cast<float> ((c1 >> 8) & 0xFF) * frac);
    const uint8 b = static_cast<uint8> (
        static_cast<float> (c0 & 0xFF) * (1.0f - frac) + static_cast<float> (c1 & 0xFF) * frac);

    return (static_cast<uint32> (a) << 24)
         | (static_cast<uint32> (r) << 16)
         | (static_cast<uint32> (g) << 8)
         | static_cast<uint32> (b);
}

static ColorGradient::ColorStop makeColorStop (float delta, uint8 r, uint8 g, uint8 b)
{
    return { Color (r, g, b), 0.0f, 0.0f, delta };
}

void SpectrogramColorMap::buildFromGradient (ColorGradient gradient)
{
    colorTable.resize (static_cast<size_t> (numColorStops));
    gradient.fillGradient (colorTable);
}

void SpectrogramColorMap::buildHeatmap()
{
    buildFromGradient ({ ColorGradient::Linear,
                         {
                             makeColorStop (0.00f, 0, 0, 0),
                             makeColorStop (0.15f, 0, 0, 64),
                             makeColorStop (0.30f, 0, 0, 255),
                             makeColorStop (0.45f, 0, 255, 255),
                             makeColorStop (0.60f, 0, 255, 0),
                             makeColorStop (0.75f, 255, 255, 0),
                             makeColorStop (0.90f, 255, 0, 0),
                             makeColorStop (1.00f, 255, 255, 255),
                         } });
}

void SpectrogramColorMap::buildGrayscale()
{
    buildFromGradient ({ ColorGradient::Linear,
                         {
                             makeColorStop (0.00f, 0, 0, 0),
                             makeColorStop (1.00f, 255, 255, 255),
                         } });
}

void SpectrogramColorMap::buildCool()
{
    buildFromGradient ({ ColorGradient::Linear,
                         {
                             makeColorStop (0.00f, 0, 0, 0),
                             makeColorStop (0.33f, 0, 0, 255),
                             makeColorStop (0.66f, 0, 255, 255),
                             makeColorStop (1.00f, 255, 255, 255),
                         } });
}

void SpectrogramColorMap::buildWarm()
{
    buildFromGradient ({ ColorGradient::Linear,
                         {
                             makeColorStop (0.00f, 0, 0, 0),
                             makeColorStop (0.33f, 255, 0, 0),
                             makeColorStop (0.66f, 255, 165, 0),
                             makeColorStop (1.00f, 255, 255, 255),
                         } });
}

void SpectrogramColorMap::buildViridis()
{
    buildFromGradient ({ ColorGradient::Linear,
                         {
                             makeColorStop (0.00f, 68, 1, 84),
                             makeColorStop (0.20f, 59, 82, 139),
                             makeColorStop (0.40f, 33, 145, 140),
                             makeColorStop (0.60f, 94, 201, 98),
                             makeColorStop (0.80f, 181, 222, 43),
                             makeColorStop (1.00f, 253, 231, 37),
                         } });
}

//==============================================================================
// SpectrogramComponent
//==============================================================================

SpectrogramComponent::SpectrogramComponent (SpectrumAnalyzerState& state)
    : analyzerState (state)
    , displayMagnitudes (defaultSpectrogramWidth, 0.0f)
    , colorMap (SpectrogramColorMap::Type::heatmap)
{
    fftSize = analyzerState.getFftSize();

    initializeFFTBuffers();
    generateWindow();

    startTimerHz (25); // 25 FPS updates
}

SpectrogramComponent::~SpectrogramComponent()
{
    stopTimer();
}

//==============================================================================
void SpectrogramComponent::initializeFFTBuffers()
{
    fftProcessor = std::make_unique<FFTProcessor> (fftSize);
    fftInputBuffer.resize (static_cast<size_t> (fftSize), 0.0f);
    fftOutputBuffer.resize (static_cast<size_t> (fftSize * 2), 0.0f);
    windowBuffer.resize (static_cast<size_t> (fftSize), 0.0f);

    const int numBins = fftSize / 2 + 1;
    magnitudeBuffer.resize (static_cast<size_t> (numBins), 0.0f);
}

void SpectrogramComponent::ensureImageSize()
{
    const int currentWidth = spectrogramImage.isValid() ? spectrogramImage.getWidth() : 0;
    const int currentHeight = spectrogramImage.isValid() ? spectrogramImage.getHeight() : 0;

    if (currentWidth != spectrogramWidth || currentHeight != numHistoryFrames)
    {
        spectrogramImage = Image (spectrogramWidth, numHistoryFrames, PixelFormat::RGBA);
        spectrogramImage.fill (0xFF0a0a0a);
    }
}

//==============================================================================
void SpectrogramComponent::timerCallback()
{
    if (! isShowing())
        return;

    bool hasNewData = false;
    int fftCount = 0;

    constexpr int maxFFTsPerFrame = 4;

    while (analyzerState.isFFTDataReady() && fftCount < maxFFTsPerFrame)
    {
        processFFT();
        hasNewData = true;
        ++fftCount;
    }

    if (hasNewData)
        updateSpectrogramImage();

    repaint();
}

void SpectrogramComponent::processFFT()
{
    if (! analyzerState.getFFTData (fftInputBuffer.data()))
        return;

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

    // Compute magnitudes with window gain compensation
    const int numBins = fftSize / 2 + 1;

    for (int binIndex = 0; binIndex < numBins; ++binIndex)
    {
        const float real = fftOutputBuffer[static_cast<size_t> (binIndex * 2)];
        const float imag = fftOutputBuffer[static_cast<size_t> (binIndex * 2 + 1)];
        magnitudeBuffer[static_cast<size_t> (binIndex)] = std::sqrt (real * real + imag * imag) * windowGain;
    }

    // Map FFT bins to display bins using logarithmic frequency scaling
    const int numDisplayBins = spectrogramWidth;

    for (int i = 0; i < numDisplayBins; ++i)
    {
        const float proportion = static_cast<float> (i) / static_cast<float> (numDisplayBins - 1);
        const float logFreq = logMinFrequency + proportion * (logMaxFrequency - logMinFrequency);
        const float centerFreq = std::pow (10.0f, logFreq);

        float freqRangeStart, freqRangeEnd;

        if (i == 0)
        {
            freqRangeStart = minFrequency;
            const float nextLogFreq = logMinFrequency + (static_cast<float> (i + 1) / static_cast<float> (numDisplayBins - 1)) * (logMaxFrequency - logMinFrequency);
            const float nextFreq = std::pow (10.0f, nextLogFreq);
            freqRangeEnd = (centerFreq + nextFreq) * 0.5f;
        }
        else if (i == numDisplayBins - 1)
        {
            const float prevLogFreq = logMinFrequency + (static_cast<float> (i - 1) / static_cast<float> (numDisplayBins - 1)) * (logMaxFrequency - logMinFrequency);
            const float prevFreq = std::pow (10.0f, prevLogFreq);
            freqRangeStart = (prevFreq + centerFreq) * 0.5f;
            freqRangeEnd = maxFrequency;
        }
        else
        {
            const float prevLogFreq = logMinFrequency + (static_cast<float> (i - 1) / static_cast<float> (numDisplayBins - 1)) * (logMaxFrequency - logMinFrequency);
            const float nextLogFreq = logMinFrequency + (static_cast<float> (i + 1) / static_cast<float> (numDisplayBins - 1)) * (logMaxFrequency - logMinFrequency);
            const float prevFreq = std::pow (10.0f, prevLogFreq);
            const float nextFreq = std::pow (10.0f, nextLogFreq);
            freqRangeStart = (prevFreq + centerFreq) * 0.5f;
            freqRangeEnd = (centerFreq + nextFreq) * 0.5f;
        }

        const float startBin = (freqRangeStart * static_cast<float> (fftSize)) / static_cast<float> (sampleRate);
        const float endBin = (freqRangeEnd * static_cast<float> (fftSize)) / static_cast<float> (sampleRate);
        const float binSpan = endBin - startBin;

        float magnitude = 0.0f;

        if (binSpan <= 1.5f)
        {
            const float exactBin = (centerFreq * static_cast<float> (fftSize)) / static_cast<float> (sampleRate);
            const int bin1 = jlimit (0, numBins - 1, static_cast<int> (exactBin));
            const int bin2 = jlimit (0, numBins - 1, bin1 + 1);
            const float fraction = exactBin - static_cast<float> (bin1);

            const float mag1 = magnitudeBuffer[static_cast<size_t> (bin1)];
            const float mag2 = magnitudeBuffer[static_cast<size_t> (bin2)];
            magnitude = mag1 + fraction * (mag2 - mag1);
        }
        else
        {
            const int binStart = jlimit (0, numBins - 1, static_cast<int> (startBin));
            const int binEnd = jlimit (0, numBins - 1, static_cast<int> (endBin + 0.5f));

            for (int binIndex = binStart; binIndex <= binEnd; ++binIndex)
                magnitude = jmax (magnitude, magnitudeBuffer[static_cast<size_t> (binIndex)]);
        }

        // Convert to decibels and normalize to [0, 1]
        float magnitudeDb = magnitude > 0.0f
                              ? 20.0f * std::log10 (magnitude / static_cast<float> (fftSize))
                              : minDecibels;

        displayMagnitudes[static_cast<size_t> (i)] = jmap (
            jlimit (minDecibels, maxDecibels, magnitudeDb),
            minDecibels,
            maxDecibels,
            0.0f,
            1.0f);
    }
}

void SpectrogramComponent::updateSpectrogramImage()
{
    ensureImageSize();

    scrollSpectrogram();
    writeMagnitudeRow();

    // Invalidate so the GPU texture is recreated from updated pixel data
    spectrogramImage.invalidateTexture();
}

void SpectrogramComponent::scrollSpectrogram()
{
    auto raw = spectrogramImage.getRawData();
    const int width = spectrogramImage.getWidth();
    const int height = spectrogramImage.getHeight();
    const int stride = spectrogramImage.getPixelStride();
    const int rowBytes = width * stride;

    // Shift all rows down by one (row 0 stays at top, row height-1 falls off)
    if (height > 1)
    {
        std::memmove (
            raw.data() + rowBytes,
            raw.data(),
            static_cast<size_t> ((height - 1)) * static_cast<size_t> (rowBytes));
    }
}

void SpectrogramComponent::writeMagnitudeRow()
{
    auto& bitmap = spectrogramImage.getBitmapData();
    const int width = spectrogramImage.getWidth();

    for (int x = 0; x < width; ++x)
    {
        const float magnitude = displayMagnitudes[static_cast<size_t> (x)];
        const uint32 color = colorMap.map (magnitude);
        bitmap.setPixel (x, 0, color);
    }
}

void SpectrogramComponent::generateWindow()
{
    WindowFunctions<float>::generate (currentWindowType, windowBuffer.data(), windowBuffer.size());

    float windowSum = 0.0f;
    for (int i = 0; i < fftSize; ++i)
        windowSum += windowBuffer[static_cast<size_t> (i)];

    windowGain = windowSum > 0.0f ? static_cast<float> (fftSize) / windowSum : 1.0f;
}

//==============================================================================
void SpectrogramComponent::paint (Graphics& g)
{
    const auto bounds = getLocalBounds();

    // Background
    g.setFillColor (Color (0xFF0a0a0a));
    g.fillAll();

    // Draw the spectrogram image (stretched to fill the component)
    if (spectrogramImage.isValid())
        g.drawImage (spectrogramImage, bounds);

    // Draw grid overlays
    drawFrequencyGrid (g, bounds);
}

void SpectrogramComponent::resized()
{
    // Update history frames to match the component height if not explicitly set
}

//==============================================================================
void SpectrogramComponent::drawFrequencyGrid (Graphics& g, const Rectangle<float>& bounds)
{
    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (9.0f);

    const int multipliers[] = { 1, 2, 5 };
    const int powers[] = { 1, 10, 100, 1000, 10000 };

    for (int brightness = 0; brightness < 3; ++brightness)
    {
        Color lineColor;
        float lineWidth;
        bool drawLabels = false;

        if (brightness == 0)
        {
            lineColor = Color (0x50ffffff);
            lineWidth = 1.0f;
            drawLabels = true;
        }
        else if (brightness == 1)
        {
            lineColor = Color (0x28ffffff);
            lineWidth = 0.75f;
        }
        else
        {
            lineColor = Color (0x12ffffff);
            lineWidth = 0.5f;
        }

        g.setStrokeColor (lineColor);
        g.setStrokeWidth (lineWidth);

        for (int power = 0; power < 5; ++power)
        {
            float freq = static_cast<float> (multipliers[brightness] * powers[power]);

            if (freq < minFrequency || freq > maxFrequency)
                continue;

            const float x = frequencyToX (freq, bounds.getWidth());

            // Vertical grid line
            g.strokeLine (
                bounds.getX() + x,
                bounds.getY(),
                bounds.getX() + x,
                bounds.getBottom());

            if (! drawLabels)
                continue;

            String freqText;
            if (freq >= 1000.0f)
                freqText = String (freq / 1000.0f, freq == 1000.0f ? 0 : 1) + "k";
            else
                freqText = String (static_cast<int> (freq));

            g.setFillColor (Color (0xFFaaaaaa));
            float labelX = jmax (bounds.getX() + x - 20.0f, bounds.getX());
            labelX = jmin (labelX, bounds.getRight() - 40.0f);
            g.fillFittedText (freqText, font, { labelX, bounds.getBottom() - 12.0f, 40.0f, 10.0f }, Justification::center);
        }
    }
}

//==============================================================================
float SpectrogramComponent::frequencyToX (float frequency, float width) const noexcept
{
    return jmap (std::log10 (frequency), logMinFrequency, logMaxFrequency, 0.0f, width);
}

//==============================================================================
void SpectrogramComponent::setFFTSize (int size)
{
    jassert (isPowerOfTwo (size) && size >= 64 && size <= 65536);

    if (fftSize != size)
    {
        fftSize = size;
        analyzerState.setFftSize (size);

        initializeFFTBuffers();
        generateWindow();

        clearHistory();
        repaint();
    }
}

void SpectrogramComponent::setWindowType (WindowType type)
{
    if (currentWindowType != type)
    {
        currentWindowType = type;
        needsWindowUpdate = true;
    }
}

void SpectrogramComponent::setUpdateRate (int hz)
{
    stopTimer();
    startTimerHz (jmax (1, jmin (60, hz)));
}

int SpectrogramComponent::getUpdateRate() const noexcept
{
    const int intervalMs = getTimerInterval();
    return isTimerRunning() ? 1000 / intervalMs : 0;
}

void SpectrogramComponent::setFrequencyRange (float minFreq, float maxFreq)
{
    const float newMinFrequency = jmax (1.0f, minFreq);
    const float newMaxFrequency = jmax (newMinFrequency + 1.0f, maxFreq);

    if (approximatelyEqual (minFrequency, newMinFrequency)
        && approximatelyEqual (maxFrequency, newMaxFrequency))
    {
        return;
    }

    minFrequency = newMinFrequency;
    maxFrequency = newMaxFrequency;
    logMinFrequency = std::log10 (minFrequency);
    logMaxFrequency = std::log10 (maxFrequency);

    clearHistory();
}

void SpectrogramComponent::setDecibelRange (float minDb, float maxDb)
{
    jassert (maxDb > minDb);

    minDb = jmin (minDb, maxDb - 1.0f);

    if (approximatelyEqual (minDecibels, minDb)
        && approximatelyEqual (maxDecibels, maxDb))
    {
        return;
    }

    minDecibels = minDb;
    maxDecibels = maxDb;

    clearHistory();
}

void SpectrogramComponent::setSampleRate (double rate)
{
    const double newSampleRate = jmax (1.0, rate);

    if (approximatelyEqual (sampleRate, newSampleRate))
        return;

    sampleRate = newSampleRate;
    clearHistory();
}

void SpectrogramComponent::setColorMap (SpectrogramColorMap::Type type)
{
    colorMap = SpectrogramColorMap (type);
    clearHistory();
}

void SpectrogramComponent::setNumHistoryFrames (int numFrames)
{
    numHistoryFrames = jmax (4, numFrames);

    // Recreate the image with the new height
    spectrogramImage = Image (spectrogramWidth, numHistoryFrames, PixelFormat::RGBA);
    spectrogramImage.fill (Color (0xFF0a0a0a).getARGB());

    repaint();
}

void SpectrogramComponent::setOverlapFactor (float overlapFactor)
{
    analyzerState.setOverlapFactor (overlapFactor);
}

float SpectrogramComponent::getOverlapFactor() const noexcept
{
    return analyzerState.getOverlapFactor();
}

void SpectrogramComponent::clearHistory()
{
    if (spectrogramImage.isValid())
        spectrogramImage.fill (Color (0xFF0a0a0a).getARGB());

    repaint();
}

} // namespace yup
