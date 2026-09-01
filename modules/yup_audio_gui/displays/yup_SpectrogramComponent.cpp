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

namespace
{

//==============================================================================
/*

// The waterfall shader is precompiled into a .ysl bundle embedded in
// yup_SpectrogramComponentShader.inc (do not edit by hand). Regenerate it after
// changing yup_SpectrogramComponentShader.vert / .frag with:

    build/mac/_host_tools/shader_bundler/yup_shader_bundler \
       --vert  modules/yup_audio_gui/displays/yup_SpectrogramComponentShader.vert \
       --frag  modules/yup_audio_gui/displays/yup_SpectrogramComponentShader.frag \
       --output /tmp/yup_SpectrogramComponentShader.ysl \
       --target-langs glsl,essl,hlsl,msl,wgsl

// then embed the bundle bytes into the .inc (keep the two-line comment header):

   xxd -i /tmp/yup_SpectrogramComponentShader.ysl \
       | sed -e '1d' -e '/^};/d' -e '/_len =/d' -e '/^[[:space:]]*$/d' \
       > /tmp/yup_SpectrogramComponentShader.inc.body
   { echo '// Generated shader bundle (yup_SpectrogramComponentShader.ysl) - do not edit by hand.'; \
     echo '// Regenerate with the command at the top of yup_SpectrogramComponent.cpp.'; \
     cat /tmp/yup_SpectrogramComponentShader.inc.body; } \
       > modules/yup_audio_gui/displays/yup_SpectrogramComponentShader.inc

*/

// Embedded precompiled shader bundle (.ysl), consumed by ShaderBundle::loadFromData().
constexpr uint8_t kSpectrogramShaderBundle[] = {
#include "yup_SpectrogramComponentShader.inc"
};

// Uniforms for the waterfall shader (std140: eight floats).
struct WaterfallParams
{
    float numRows;
    float width; // Waterfall texture width in texels
    float height;
    float bins; // Number of magnitude bins per row
    float pad0;
    float pad1;
    float pad2;
    float pad3;
};
} // namespace

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
    updateFrequencyMapping();

    rowDataCache.resize (defaultSpectrogramMagnitudes, 0.0f);
    lutDataCache.resize (defaultColorLutSize, 0u);
}

SpectrogramComponent::~SpectrogramComponent()
{
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

//==============================================================================
void SpectrogramComponent::applyPendingRows()
{
    if (pendingRows.empty())
        return;

    if (waterfallPipeline == nullptr || gpuTargets[0] == nullptr || gpuTargets[1] == nullptr)
    {
        Logger::outputDebugString ("SpectrogramComponent: dropping " + String (static_cast<int> (pendingRows.size()))
                                   + " pending FFT rows - waterfall pipeline or GPU targets not ready");
        pendingRows.clear();
        return;
    }

    const int numRows = static_cast<int> (pendingRows.size());
    const int maxShaderRows = jmax (1, static_cast<int> (rowDataCache.size()) / spectrogramWidth);
    const int appliedRows = jmin (numRows, maxShaderRows);

    auto& previous = gpuTargets[pingPongIndex];
    auto& current = gpuTargets[pingPongIndex ^ 1];

    const WaterfallParams params { static_cast<float> (appliedRows),
                                   static_cast<float> (defaultSpectrogramRenderWidth), // render width
                                   static_cast<float> (numHistoryFrames),
                                   static_cast<float> (spectrogramWidth), // bins
                                   0.0f,
                                   0.0f,
                                   0.0f,
                                   0.0f };

    for (int row = 0; row < appliedRows; ++row)
    {
        const auto& magnitudes = pendingRows[static_cast<size_t> (row)];
        std::copy (magnitudes.begin(),
                   magnitudes.begin() + spectrogramWidth,
                   rowDataCache.begin() + static_cast<std::ptrdiff_t> (row) * spectrogramWidth);
    }

    if (lutNeedsRefresh)
    {
        const auto colorTable = colorMap.getColorTable();
        std::copy (colorTable.begin(),
                   colorTable.begin() + static_cast<std::ptrdiff_t> (jmin (colorTable.size(), lutDataCache.size())),
                   lutDataCache.begin());
        lutNeedsRefresh = false;
    }

    auto frame = GpuFrame::begin (gpuDevice);
    if (frame.isValid())
    {
        auto pass = current->beginRenderPass (frame, { false, GpuColor::transparentBlack() });
        if (pass.isValid())
        {
            pass.setPipeline (waterfallPipeline);
            pass.setTexture (0, 0, previous->asTexture());
            pass.setUniformBuffer (0, 2, &params, sizeof (params));
            pass.setUniformBuffer (0, 3, rowDataCache.data(), rowDataCache.size() * sizeof (float));
            pass.setUniformBuffer (0, 4, lutDataCache.data(), lutDataCache.size() * sizeof (uint32));

            if (pass.draw (3) && pass.finish() && frame.submit())
            {
                displayTexture = current->asTexture();

                pingPongIndex ^= 1;
                scrollOffset -= static_cast<float> (appliedRows);
            }
            else
            {
                Logger::outputDebugString ("SpectrogramComponent: waterfall draw/finish/submit failed - rows not applied");
            }
        }
        else
        {
            Logger::outputDebugString ("SpectrogramComponent: beginRenderPass failed - rows not applied");
        }
    }
    else
    {
        Logger::outputDebugString ("SpectrogramComponent: GpuFrame::begin failed - rows not applied");
    }

    pendingRows.erase (pendingRows.begin(), pendingRows.begin() + appliedRows);
}

void SpectrogramComponent::advanceScroll()
{
    const auto now = Time::getMillisecondCounter();
    const float elapsedSeconds = lastPaintTimeMs == 0 ? 0.0f : static_cast<float> (now - lastPaintTimeMs) / 1000.0f;
    lastPaintTimeMs = now;

    const float rowRate = getRowRate();
    const float advance = rowRate > 0.0f ? rowRate * scrollSpeedMultiplier * jlimit (0.0f, 0.25f, elapsedSeconds) : 0.0f;
    scrollOffset = jlimit (-1.0f, 0.0f, scrollOffset + advance);
}

void SpectrogramComponent::setScrollSpeed (float scrollSpeedMultiplier)
{
    this->scrollSpeedMultiplier = jmax (0.0f, scrollSpeedMultiplier);
}

void SpectrogramComponent::refreshDisplay (double lastFrameTimeSeconds)
{
    if (! isShowing())
    {
        analyzerState.reset();
        return;
    }

    // If the analysis fell behind the audio (e.g. slow frames), skip the stale
    // rows so the display keeps tracking the most recent audio instead of the
    // latency accumulating over time.
    const int numReady = analyzerState.getNumAvailableSamples();
    const int hopSize = analyzerState.getHopSize();
    if (hopSize > 0 && numReady > fftSize)
    {
        const int skipRows = (numReady - fftSize) / hopSize;
        for (int i = 0; i < skipRows && analyzerState.isFFTDataReady(); ++i)
            analyzerState.getFFTData (fftInputBuffer.data());
    }

    bool hasNewData = false;
    int fftCount = 0;

    constexpr int maxFFTsPerFrame = 4;

    while (analyzerState.isFFTDataReady() && fftCount < maxFFTsPerFrame)
    {
        processFFT();

        pendingRows.push_back (displayMagnitudes);

        hasNewData = true;
        ++fftCount;
    }

    // Defensive bound: never let the pending queue grow unboundedly if paint()
    // is ever throttled - keep only the newest rows.
    if (pendingRows.size() > 16)
        pendingRows.erase (pendingRows.begin(), pendingRows.begin() + static_cast<std::ptrdiff_t> (pendingRows.size() - 16));

    if (hasNewData)
        repaint();
}

float SpectrogramComponent::getRowRate() const noexcept
{
    const int hopSize = analyzerState.getHopSize();
    return hopSize > 0 ? static_cast<float> (sampleRate) / static_cast<float> (hopSize) : 0.0f;
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
    FloatVectorOperations::multiply (fftInputBuffer.data(), windowBuffer.data(), fftInputBuffer.data(), fftSize);

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

    // Map FFT bins to display bins using the precomputed logarithmic mapping.
    for (int i = 0; i < spectrogramWidth; ++i)
    {
        const auto& mapping = displayBinMapping[static_cast<size_t> (i)];
        const float binSpan = mapping.endBin - mapping.startBin;

        float magnitude = 0.0f;

        if (binSpan <= 1.5f)
        {
            const int bin1 = jlimit (0, numBins - 1, static_cast<int> (mapping.exactBin));
            const int bin2 = jlimit (0, numBins - 1, bin1 + 1);
            const float fraction = mapping.exactBin - static_cast<float> (bin1);

            const float mag1 = magnitudeBuffer[static_cast<size_t> (bin1)];
            const float mag2 = magnitudeBuffer[static_cast<size_t> (bin2)];
            magnitude = mag1 + fraction * (mag2 - mag1);
        }
        else
        {
            const int binStart = jlimit (0, numBins - 1, static_cast<int> (mapping.startBin));
            const int binEnd = jlimit (0, numBins - 1, static_cast<int> (mapping.endBin + 0.5f));

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

void SpectrogramComponent::updateFrequencyMapping()
{
    displayBinMapping.resize (static_cast<size_t> (spectrogramWidth));

    const int numDisplayBins = spectrogramWidth;
    const float invLastBin = 1.0f / static_cast<float> (numDisplayBins - 1);

    for (int i = 0; i < numDisplayBins; ++i)
    {
        const float proportion = static_cast<float> (i) * invLastBin;
        const float logFreq = logMinFrequency + proportion * (logMaxFrequency - logMinFrequency);
        const float centerFreq = std::pow (10.0f, logFreq);

        const float prevProportion = static_cast<float> (i - 1) * invLastBin;
        const float nextProportion = static_cast<float> (i + 1) * invLastBin;

        float freqRangeStart, freqRangeEnd;

        if (i == 0)
        {
            freqRangeStart = minFrequency;
            freqRangeEnd = (centerFreq + std::pow (10.0f, logMinFrequency + nextProportion * (logMaxFrequency - logMinFrequency))) * 0.5f;
        }
        else if (i == numDisplayBins - 1)
        {
            freqRangeStart = (std::pow (10.0f, logMinFrequency + prevProportion * (logMaxFrequency - logMinFrequency)) + centerFreq) * 0.5f;
            freqRangeEnd = maxFrequency;
        }
        else
        {
            freqRangeStart = (std::pow (10.0f, logMinFrequency + prevProportion * (logMaxFrequency - logMinFrequency)) + centerFreq) * 0.5f;
            freqRangeEnd = (centerFreq + std::pow (10.0f, logMinFrequency + nextProportion * (logMaxFrequency - logMinFrequency))) * 0.5f;
        }

        auto& mapping = displayBinMapping[static_cast<size_t> (i)];
        mapping.startBin = (freqRangeStart * static_cast<float> (fftSize)) / static_cast<float> (sampleRate);
        mapping.endBin = (freqRangeEnd * static_cast<float> (fftSize)) / static_cast<float> (sampleRate);
        mapping.exactBin = (centerFreq * static_cast<float> (fftSize)) / static_cast<float> (sampleRate);
    }
}

bool SpectrogramComponent::ensureGpuTargets (GraphicsContext& context)
{
    if (gpuTargets[0] != nullptr && gpuTargets[1] != nullptr)
        return true;

    if (! context.isGpuAvailable())
        return false;

    gpuDevice = context.getGpuDevice();
    if (gpuDevice == nullptr)
        return false;

    // The waterfall texture is rendered at a higher horizontal resolution than
    // the bin count and interpolated in the shader for a smooth (antialiased)
    // frequency axis.
    const int renderWidth = defaultSpectrogramRenderWidth;

    const auto backgroundColor = Color (0xFF0a0a0a);

    gpuTargets[0] = GpuTarget::create (gpuDevice, renderWidth, numHistoryFrames);
    gpuTargets[1] = GpuTarget::create (gpuDevice, renderWidth, numHistoryFrames);

    if (gpuTargets[0] == nullptr || gpuTargets[1] == nullptr)
    {
        gpuTargets[0] = nullptr;
        gpuTargets[1] = nullptr;
        return false;
    }

    const GpuRenderOptions clearOptions { true, backgroundColor };
    for (auto& target : gpuTargets)
    {
        auto frame = GpuFrame::begin (gpuDevice);
        if (frame.isValid())
        {
            auto pass = target->beginRenderPass (frame, clearOptions);
            pass.finish();
            frame.submit();
        }
    }

    ensureWaterfallPipeline();

    pingPongIndex = 0;
    displayTexture = gpuTargets[0]->asTexture();

    scrollOffset = 0.0f;
    lastPaintTimeMs = 0;

    return true;
}

void SpectrogramComponent::ensureWaterfallPipeline()
{
    if (waterfallPipeline != nullptr || gpuDevice == nullptr)
        return;

    auto loaded = ShaderBundle::loadFromData (kSpectrogramShaderBundle, sizeof (kSpectrogramShaderBundle));
    if (loaded.failed())
    {
        Logger::outputDebugString ("SpectrogramComponent: failed to load waterfall shader bundle: " + loaded.getErrorMessage());

        jassertfalse; // Waterfall shader bundle failed to load - no waterfall will be rendered.
        return;
    }

    GpuPipelineOptions options;
    options.colorTargetCount = 1;
    options.colorTargets[0].format = GpuTextureFormat::rgba8unorm;
    options.colorTargets[0].blendEnabled = false;

    auto result = GpuPipeline::compileFromBundle (gpuDevice, loaded.getReference(), options);

    if (result.wasOk())
        waterfallPipeline = result.getValue();
    else
    {
        Logger::outputDebugString ("SpectrogramComponent: waterfall shader failed to compile: " + result.getErrorMessage());
        jassertfalse; // Waterfall shader failed to compile - no waterfall will be rendered.
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
    // Lazily move to the GPU path on the first paint that has a live context.
    if (gpuTargets[0] == nullptr && g.getGraphicsContext().isGpuAvailable())
        ensureGpuTargets (g.getGraphicsContext());

    // Apply any FFT rows queued since the last timer tick.
    applyPendingRows();

    // Advance the fractional scroll offset on the GPU path.
    advanceScroll();

    const auto bounds = getLocalBounds();

    // Background
    g.setFillColor (Color (0xFF0a0a0a));
    g.fillAll();

    // Draw the spectrogram, stretched to fill the component.
    if (displayTexture != nullptr)
    {
        const float scaleY = bounds.getHeight() / static_cast<float> (displayTexture->getHeight());
        g.drawTexture (displayTexture, Rectangle<float> (bounds.getX(), bounds.getY() + scrollOffset * scaleY, bounds.getWidth(), bounds.getHeight()));
    }

    // Draw grid overlays (cached offscreen, only re-rendered on changes)
    drawFrequencyGridCached (g, bounds);
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
    const int powers[] = { 1, 10, 100, 200, 1000, 2000, 10000 };

    for (int brightness = 0; brightness < numElementsInArray (multipliers); ++brightness)
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

        for (int power = 0; power < numElementsInArray (powers); ++power)
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

void SpectrogramComponent::drawFrequencyGridCached (Graphics& g, const Rectangle<float>& bounds)
{
    const Rectangle<float> cacheBounds (0.0f, 0.0f, static_cast<float> (static_cast<int> (bounds.getWidth())), static_cast<float> (static_cast<int> (bounds.getHeight())));

    if (gridCanvas == nullptr || gridNeedsRedraw || gridCacheBounds != cacheBounds)
    {
        gridCanvas = GpuCanvas::create (g.getGraphicsContext(),
                                        static_cast<int> (cacheBounds.getWidth()),
                                        static_cast<int> (cacheBounds.getHeight()),
                                        Colors::transparentBlack);

        if (gridCanvas != nullptr)
        {
            auto& gridGraphics = gridCanvas->beginDraw();
            drawFrequencyGrid (gridGraphics, cacheBounds);
            gridCanvas->commit();

            gridCacheBounds = cacheBounds;
            gridNeedsRedraw = false;
        }
    }

    if (gridCanvas != nullptr)
        g.drawTexture (gridCanvas->asTexture(), bounds);
    else
        drawFrequencyGrid (g, bounds);
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
        updateFrequencyMapping();

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

    updateFrequencyMapping();
    gridNeedsRedraw = true;

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
    updateFrequencyMapping();
    clearHistory();
}

void SpectrogramComponent::setColorMap (SpectrogramColorMap::Type type)
{
    colorMap = SpectrogramColorMap (type);
    lutNeedsRefresh = true; // Re-fill the cached LUT on the next apply
    clearHistory();
}

void SpectrogramComponent::setNumHistoryFrames (int numFrames)
{
    numHistoryFrames = jmax (4, numFrames);

    pendingRows.clear();
    scrollOffset = 0.0f;
    lastPaintTimeMs = 0;

    gpuTargets[0] = nullptr;
    gpuTargets[1] = nullptr;
    displayTexture = nullptr;

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
    pendingRows.clear();
    scrollOffset = 0.0f;
    lastPaintTimeMs = 0;

    gpuTargets[0] = nullptr;
    gpuTargets[1] = nullptr;
    displayTexture = nullptr;

    repaint();
}

Image SpectrogramComponent::getSpectrogramImage()
{
    if (gpuTargets[pingPongIndex] != nullptr)
        return Image::fromTarget (*gpuTargets[pingPongIndex]);

    return {};
}

} // namespace yup
