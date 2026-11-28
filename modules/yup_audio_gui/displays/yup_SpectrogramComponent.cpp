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
// The waterfall shader bundle, built with the module, see shaderBundles in yup_audio_gui.h
constexpr uint8_t kSpectrogramShaderBundle[] = {
#include "yup_audio_gui_shaders/yup_SpectrogramComponentShader.inc"
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

SpectrogramColorMap::SpectrogramColorMap (Type typeToUse, int numStops)
    : type (typeToUse)
    , numColorStops (jmax (2, numStops))
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

const ComponentTypeInfo& SpectrogramComponent::getClassTypeInfo()
{
    using S = SpectrogramComponent;

    static const auto info = []
    {
        auto builder = ComponentTypeBuilder<S> (Identifier ("Spectrogram"));

        detail::addSpectrumDisplayProperties (builder);

        return builder
            .enumeration<SpectrogramColorMap::Type> (
                "colorMap",
                { { "heatmap", SpectrogramColorMap::Type::heatmap },
                  { "grayscale", SpectrogramColorMap::Type::grayscale },
                  { "cool", SpectrogramColorMap::Type::cool },
                  { "warm", SpectrogramColorMap::Type::warm },
                  { "viridis", SpectrogramColorMap::Type::viridis } },
                [] (const S& s) { return s.getColorMap().getType(); },
                &S::setColorMap,
                "The colors the levels are drawn with.")
            .property ("scrollSpeed", &S::getScrollSpeed, &S::setScrollSpeed, 1.0f, "How fast the waterfall scrolls: 1 follows the audio, 0 pauses it.")
            .property ("historyFrames", &S::getNumHistoryFrames, &S::setNumHistoryFrames, 256, "The rows of spectrum the waterfall keeps.")
            .build();
    }();

    return info;
}

//==============================================================================
SpectrogramComponent::SpectrogramComponent()
    : colorMap (SpectrogramColorMap::Type::heatmap)
{
    rowDataCache.resize (defaultSpectrogramMagnitudes, 0.0f);
    lutDataCache.resize (defaultColorLutSize, 0u);
}

SpectrogramComponent::SpectrogramComponent (SpectrumAnalyzerState& state)
    : SpectrogramComponent()
{
    setState (&state);
}

SpectrogramComponent::~SpectrogramComponent()
{
}

//==============================================================================
void SpectrogramComponent::setState (SpectrumAnalyzerState* newState)
{
    if (! analysis.setState (newState))
        return;

    // The rows shown are the ones of the previous state
    clearHistory();
    updateRefreshing (true);
}

//==============================================================================
void SpectrogramComponent::applyPendingRows()
{
    if (numPendingRows == 0)
        return;

    // A paused waterfall holds its last frame: new rows are discarded rather than written, otherwise
    // the content would keep sliding down even though the scroll is stopped.
    if (scrollSpeedMultiplier <= 0.0f)
    {
        numPendingRows = 0;
        return;
    }

    if (waterfallPipeline == nullptr || gpuTargets[0] == nullptr || gpuTargets[1] == nullptr)
    {
        // Without a GPU this is every frame: it is told once
        if (! droppedRowsWereLogged)
        {
            droppedRowsWereLogged = true;
            Logger::outputDebugString ("SpectrogramComponent: dropping the FFT rows - the waterfall needs a GPU, and its pipeline or its targets are not ready");
        }

        numPendingRows = 0;
        return;
    }

    const int maxShaderRows = jmax (1, static_cast<int> (rowDataCache.size()) / spectrogramWidth);

    // A single pass can only shift and write maxShaderRows rows, so a frame that produced more FFT rows
    // than that - which is what happens whenever the display runs below the FFT row rate - needs several
    // passes. Without them the surplus rows would be dropped and the history would silently lose data, so
    // how much of the waterfall survives would depend on the frame rate. The pass count stays bounded so
    // that a large backlog cannot stall the frame.
    constexpr int maxPassesPerFrame = 4;

    for (int pass = 0; pass < maxPassesPerFrame && numPendingRows > 0; ++pass)
    {
        const int appliedRows = jmin (numPendingRows, maxShaderRows);

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
            std::copy_n (getPendingRow (row), spectrogramWidth, rowDataCache.begin() + static_cast<std::ptrdiff_t> (row) * spectrogramWidth);

        if (lutNeedsRefresh)
        {
            const auto colorTable = colorMap.getColorTable();
            std::copy (colorTable.begin(),
                       colorTable.begin() + static_cast<std::ptrdiff_t> (jmin (colorTable.size(), lutDataCache.size())),
                       lutDataCache.begin());
            lutNeedsRefresh = false;
        }

        bool rendered = false;

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
                    rendered = true;

                    lastRowTimeMs = Time::getMillisecondCounter();
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

        // The rows are always consumed, rendered or not, so the update queue can never accumulate. The
        // scroll offset moves down with them, which keeps the history below them aligned with the audio
        // timeline even when rows had to be skipped.
        dropPendingRows (appliedRows);
        scrollOffset -= static_cast<float> (appliedRows);

        if (! rendered)
            break;
    }
}

void SpectrogramComponent::advanceScroll()
{
    const auto now = Time::getMillisecondCounter();
    const float elapsedSeconds = lastPaintTimeMs == 0 ? 0.0f : static_cast<float> (now - lastPaintTimeMs) / 1000.0f;
    lastPaintTimeMs = now;

    const float rowRate = getRowRate();

    if (rowRate <= 0.0f || scrollSpeedMultiplier <= 0.0f)
        return; // Paused: the offset is held where it is.

    scrollOffset += rowRate * scrollSpeedMultiplier * jlimit (0.0f, 0.25f, elapsedSeconds);

    // The offset is anchored to the newest row written into the history: it starts one row above the top
    // edge when that row is written and slides into place over one row period. Clamping it at the ends of
    // that window would stop the motion dead until the next row arrived - which is what used to make the
    // waterfall advance exactly one row per repaint - so the excess is pulled back with a fixed time
    // constant instead. The scroll therefore stays continuous and independent of the frame rate and of
    // the jitter of the row arrivals; the absolute bound below only matters when the analysis stalls.
    constexpr float pullBackTimeConstantSeconds = 0.1f;
    const float pullBackGain = elapsedSeconds > 0.0f
                                 ? 1.0f - std::exp (-elapsedSeconds / pullBackTimeConstantSeconds)
                                 : 0.0f;

    if (scrollOffset > 0.0f)
        scrollOffset -= pullBackGain * scrollOffset;
    else if (scrollOffset < -1.0f)
        scrollOffset += pullBackGain * (-1.0f - scrollOffset);

    scrollOffset = jlimit (-1.5f, 0.5f, scrollOffset);
}

void SpectrogramComponent::setScrollSpeed (float scrollSpeedMultiplier)
{
    this->scrollSpeedMultiplier = jmax (0.0f, scrollSpeedMultiplier);

    // Start animating immediately when the scroll is (re)enabled.
    if (this->scrollSpeedMultiplier > 0.0f)
        repaint();
}

bool SpectrogramComponent::isAnimationRunning() const noexcept
{
    const float rowRate = getRowRate();

    if (scrollSpeedMultiplier <= 0.0f || rowRate <= 0.0f)
        return false;

    if (numPendingRows > 0)
        return true;

    // Once the analysis stalls the offset settles at the end of its window, so only a recent row needs
    // further repaints.
    if (lastRowTimeMs == 0)
        return false;

    const float secondsSinceLastRow = static_cast<float> (Time::getMillisecondCounter() - lastRowTimeMs) / 1000.0f;

    return secondsSinceLastRow * rowRate < 2.0f;
}

void SpectrogramComponent::refreshDisplay (double lastFrameTimeSeconds)
{
    auto* state = analysis.getState();

    if (state == nullptr || ! isShowing())
    {
        // What the audio thread pushed while the spectrogram was hidden is not shown when it comes back
        if (state != nullptr)
            state->reset();

        updateRefreshing (false);
        return;
    }

    if (analysis.syncWithState())
        clearHistory();

    // If the analysis fell behind the audio (e.g. slow frames), skip the stale
    // rows so the display keeps tracking the most recent audio instead of the
    // latency accumulating over time.
    analysis.skipStaleFrames (1);

    bool hasNewData = false;
    int fftCount = 0;

    constexpr int maxFFTsPerFrame = 4;

    while (fftCount < maxFFTsPerFrame && analysis.processNextFrame())
    {
        queueRow();

        hasNewData = true;
        ++fftCount;
    }

    // Repaint on every frame while the waterfall is live rather than only when a row arrives: the
    // sub-row scroll offset has to be rendered at the display refresh rate, otherwise the waterfall
    // only ever moves the whole rows written since the previous repaint.
    const bool isMoving = hasNewData || isAnimationRunning();

    if (isMoving)
        repaint();

    // With no row to show, none to read and the last one in place, there is nothing to do until samples come
    updateRefreshing (isMoving || state->isFFTDataReady());
}

void SpectrogramComponent::updateRefreshing (bool hasWorkToDo)
{
    const bool canRefresh = analysis.getState() != nullptr && isShowing();

    // With nothing to do the spectrogram leaves the frames of its window alone, so that the window can skip
    // the ones it has nothing to paint in
    frameAttachment.setActive (canRefresh && hasWorkToDo);

    if (! canRefresh || hasWorkToDo)
        restingTimer.stopTimer();
    else if (! restingTimer.isTimerRunning())
        restingTimer.startTimerHz (restingRefreshRate);
}

void SpectrogramComponent::visibilityChanged()
{
    // What the audio thread pushed while the spectrogram was hidden is not shown when it comes back
    if (auto* state = analysis.getState(); state != nullptr && isShowing())
        state->reset();

    updateRefreshing (true);
}

void SpectrogramComponent::parentHierarchyChanged()
{
    updateRefreshing (true);
}

float SpectrogramComponent::getRowRate() const noexcept
{
    const auto* state = analysis.getState();

    return state != nullptr ? static_cast<float> (analysis.getSampleRate()) / static_cast<float> (state->getHopSize()) : 0.0f;
}

void SpectrogramComponent::queueRow()
{
    if (pendingRows.empty())
        pendingRows.resize (static_cast<size_t> (maxPendingRows) * static_cast<size_t> (spectrogramWidth));

    // The paint may be held back: the queue keeps the newest rows
    if (numPendingRows == maxPendingRows)
        dropPendingRows (1);

    float* const row = getPendingRow (numPendingRows++);

    const auto& binMapping = analysis.getBinMapping();
    const auto amplitudes = analysis.getBinAmplitudes();

    // Map FFT bins to display bins using the precomputed logarithmic mapping. The bands are resolved
    // across the fractional FFT bin domain, so neighbouring columns never snap to the same bin.
    for (int i = 0; i < spectrogramWidth; ++i)
    {
        const float amplitude = binMapping.getBandLevel (amplitudes, i, SpectrumBinMapping::BandAggregation::peak);
        const float decibels = amplitude > 0.0f ? 20.0f * std::log10 (amplitude) : analysis.getMinDecibels();

        row[i] = analysis.decibelsToProportion (decibels);
    }
}

float* SpectrogramComponent::getPendingRow (int index) noexcept
{
    return pendingRows.data() + static_cast<size_t> ((firstPendingRow + index) % maxPendingRows) * static_cast<size_t> (spectrogramWidth);
}

void SpectrogramComponent::dropPendingRows (int numRows) noexcept
{
    firstPendingRow = (firstPendingRow + numRows) % maxPendingRows;
    numPendingRows -= numRows;
}

bool SpectrogramComponent::ensureGpuTargets (GraphicsContext& context)
{
    if (gpuTargets[0] == nullptr || gpuTargets[1] == nullptr)
    {
        if (! context.isGpuAvailable())
            return false;

        gpuDevice = context.getGpuDevice();
        if (gpuDevice == nullptr)
            return false;

        // The waterfall texture is rendered at a higher horizontal resolution than
        // the bin count and interpolated in the shader for a smooth (antialiased)
        // frequency axis.
        const int renderWidth = defaultSpectrogramRenderWidth;

        gpuTargets[0] = GpuTarget::create (gpuDevice, renderWidth, numHistoryFrames);
        gpuTargets[1] = GpuTarget::create (gpuDevice, renderWidth, numHistoryFrames);

        if (gpuTargets[0] == nullptr || gpuTargets[1] == nullptr)
        {
            gpuTargets[0] = nullptr;
            gpuTargets[1] = nullptr;
            return false;
        }

        ensureWaterfallPipeline();

        scrollOffset = 0.0f;
        lastPaintTimeMs = 0;
        lastRowTimeMs = 0;

        historyNeedsClearing = true;
    }

    if (historyNeedsClearing)
        clearGpuTargets();

    return true;
}

void SpectrogramComponent::clearGpuTargets()
{
    const auto backgroundColor = Color (0xFF0a0a0a);

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

    pingPongIndex = 0;
    displayTexture = gpuTargets[0]->asTexture();

    historyNeedsClearing = false;
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
    auto& colorTarget = options.colorTargets.emplace_back();
    colorTarget.format = GpuTextureFormat::rgba8unorm;
    colorTarget.blendEnabled = false;

    auto result = GpuPipeline::compileFromBundle (gpuDevice, loaded.getReference(), options);

    if (result.wasOk())
        waterfallPipeline = result.getValue();
    else
    {
        Logger::outputDebugString ("SpectrogramComponent: waterfall shader failed to compile: " + result.getErrorMessage());
        jassertfalse; // Waterfall shader failed to compile - no waterfall will be rendered.
    }
}

//==============================================================================
void SpectrogramComponent::paint (Graphics& g)
{
    // A spectrogram that is painted shows, whatever else told it: one that is neither taking frames nor at
    // rest starts again here
    if (! frameAttachment.isActive() && ! restingTimer.isTimerRunning())
        updateRefreshing (true);

    // Move to the GPU path on the first paint that has a live context, and clear the history when asked.
    if (g.getGraphicsContext().isGpuAvailable())
        ensureGpuTargets (g.getGraphicsContext());

    // Apply the FFT rows queued by refreshDisplay() since the last paint.
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

            if (freq < analysis.getMinFrequency() || freq > analysis.getMaxFrequency())
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
    return width * analysis.frequencyToProportion (frequency);
}

void SpectrogramComponent::drawFrequencyGridCached (Graphics& g, const Rectangle<float>& bounds)
{
    const Rectangle<float> cacheBounds (0.0f, 0.0f, static_cast<float> (static_cast<int> (bounds.getWidth())), static_cast<float> (static_cast<int> (bounds.getHeight())));

    // The canvas is sized in pixels: it has as many as the display shows the bounds with, so the grid is not stretched
    const float cacheScale = g.getContextScale();

    if (gridCanvas == nullptr || gridNeedsRedraw || gridCacheBounds != cacheBounds || ! approximatelyEqual (gridCacheScale, cacheScale))
    {
        gridCanvas = GpuCanvas::create (g.getGraphicsContext(),
                                        roundToInt (cacheBounds.getWidth() * cacheScale),
                                        roundToInt (cacheBounds.getHeight() * cacheScale),
                                        Colors::transparentBlack);

        if (gridCanvas != nullptr)
        {
            auto& gridGraphics = gridCanvas->beginDraw ({}, cacheScale);
            drawFrequencyGrid (gridGraphics, cacheBounds);
            gridCanvas->commit();

            gridCacheBounds = cacheBounds;
            gridCacheScale = cacheScale;
            gridNeedsRedraw = false;
        }
    }

    if (gridCanvas != nullptr)
        g.drawTexture (gridCanvas->asTexture(), bounds);
    else
        drawFrequencyGrid (g, bounds);
}

//==============================================================================
void SpectrogramComponent::setWindowType (WindowType type)
{
    analysis.setWindowType (type);
}

void SpectrogramComponent::setFrequencyRange (float minFreq, float maxFreq)
{
    if (! analysis.setFrequencyRange (minFreq, maxFreq))
        return;

    gridNeedsRedraw = true;

    clearHistory();
}

void SpectrogramComponent::setDecibelRange (float minDb, float maxDb)
{
    if (analysis.setDecibelRange (minDb, maxDb))
        clearHistory();
}

void SpectrogramComponent::setSampleRate (double rate)
{
    if (analysis.setSampleRate (rate))
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

    // The targets have a row for each frame: they are made again, where the rest only clears them
    gpuTargets[0] = nullptr;
    gpuTargets[1] = nullptr;

    clearHistory();
}

void SpectrogramComponent::clearHistory()
{
    numPendingRows = 0;
    scrollOffset = 0.0f;
    lastPaintTimeMs = 0;
    lastRowTimeMs = 0;

    // The targets are kept, and cleared by the next paint: nothing of them is shown until then
    historyNeedsClearing = true;
    displayTexture = nullptr;

    repaint();
}

Image SpectrogramComponent::getSpectrogramImage()
{
    // A history that is to be cleared has no image until it is, at the next paint
    if (gpuTargets[pingPongIndex] != nullptr && ! historyNeedsClearing)
        return Image::fromTarget (*gpuTargets[pingPongIndex]);

    return {};
}

} // namespace yup
