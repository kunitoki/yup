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

#include <yup_audio_gui/yup_audio_gui.h>

#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <vector>

using namespace yup;

namespace yup
{
extern std::unique_ptr<yup::GraphicsContext> yup_constructHeadlessGraphicsContext (yup::GpuDevice::Options, yup::GpuDevice::Ptr);
} // namespace yup

namespace
{

uint8 getAlpha (uint32 color) noexcept
{
    return static_cast<uint8> ((color >> 24) & 0xff);
}

std::vector<float> createSineBuffer (int numSamples, float period)
{
    std::vector<float> buffer (static_cast<size_t> (numSamples));

    for (int i = 0; i < numSamples; ++i)
        buffer[static_cast<size_t> (i)] = std::sin (2.0f * MathConstants<float>::pi * static_cast<float> (i) / period);

    return buffer;
}

// Returns true when every pixel of the given row equals the expected RGBA color
// (bytes are compared as R, G, B, A, matching the readback pixel layout).
bool spectrogramRowIsColor (const Image& image, int row, uint32 expectedColor)
{
    const auto raw = image.getRawData();
    const auto rowBytes = static_cast<size_t> (image.getWidth()) * 4u;
    const auto rowData = raw.data() + static_cast<size_t> (row) * rowBytes;

    const uint8 expectedBytes[4] = {
        static_cast<uint8> ((expectedColor >> 16) & 0xff), // R
        static_cast<uint8> ((expectedColor >> 8) & 0xff),  // G
        static_cast<uint8> (expectedColor & 0xff),         // B
        static_cast<uint8> ((expectedColor >> 24) & 0xff), // A
    };

    for (int x = 0; x < image.getWidth(); ++x)
    {
        if (std::memcmp (rowData + static_cast<size_t> (x) * 4u, expectedBytes, 4) != 0)
            return false;
    }

    return true;
}

// Returns true when two rows of two images hold identical bytes.
bool spectrogramRowsEqual (const Image& a, int rowA, const Image& b, int rowB)
{
    if (a.getWidth() != b.getWidth())
        return false;

    const auto rawA = a.getRawData();
    const auto rawB = b.getRawData();
    const auto rowBytes = static_cast<size_t> (a.getWidth()) * 4u;

    return std::memcmp (rawA.data() + static_cast<size_t> (rowA) * rowBytes,
                        rawB.data() + static_cast<size_t> (rowB) * rowBytes,
                        rowBytes)
        == 0;
}

} // namespace

class SpectrogramComponentTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mm = MessageManager::getInstance();
        state = std::make_unique<SpectrumAnalyzerState> (2048);
        spectrogram = std::make_unique<SpectrogramComponent> (*state);
        spectrogram->setBounds (0.0f, 0.0f, 800.0f, 400.0f);
    }

    void TearDown() override
    {
        spectrogram.reset();
        state.reset();
    }

    MessageManager* mm = nullptr;
    std::unique_ptr<SpectrumAnalyzerState> state;
    std::unique_ptr<SpectrogramComponent> spectrogram;
};

//==============================================================================
// Color Map Tests
//==============================================================================

TEST (SpectrogramColorMapTests, GrayscaleMapsEndpointsAndClampsInput)
{
    SpectrogramColorMap colorMap (SpectrogramColorMap::Type::grayscale, 16);

    EXPECT_EQ (16, colorMap.getNumColorStops());
    EXPECT_EQ (0xff000000u, colorMap.map (-1.0f));
    EXPECT_EQ (0xff000000u, colorMap.map (0.0f));
    EXPECT_EQ (0xffffffffu, colorMap.map (1.0f));
    EXPECT_EQ (0xffffffffu, colorMap.map (2.0f));
    EXPECT_NE (colorMap.map (0.25f), colorMap.map (0.75f));
}

TEST (SpectrogramColorMapTests, ConstructorUsesAtLeastTwoColorStops)
{
    SpectrogramColorMap colorMap (SpectrogramColorMap::Type::heatmap, 1);

    EXPECT_EQ (2, colorMap.getNumColorStops());
    EXPECT_NE (colorMap.map (0.0f), colorMap.map (1.0f));
}

TEST (SpectrogramColorMapTests, PredefinedColorMapsReturnOpaqueColors)
{
    const SpectrogramColorMap::Type types[] = {
        SpectrogramColorMap::Type::heatmap,
        SpectrogramColorMap::Type::grayscale,
        SpectrogramColorMap::Type::cool,
        SpectrogramColorMap::Type::warm,
        SpectrogramColorMap::Type::viridis,
    };

    for (const auto type : types)
    {
        SpectrogramColorMap colorMap (type);

        EXPECT_EQ (256, colorMap.getNumColorStops());
        EXPECT_EQ (0xff, getAlpha (colorMap.map (0.0f)));
        EXPECT_EQ (0xff, getAlpha (colorMap.map (0.5f)));
        EXPECT_EQ (0xff, getAlpha (colorMap.map (1.0f)));
        EXPECT_NE (colorMap.map (0.0f), colorMap.map (1.0f));
    }
}

//==============================================================================
// Construction Tests
//==============================================================================

TEST_F (SpectrogramComponentTests, ConstructorInitializesDefaults)
{
    EXPECT_EQ (2048, spectrogram->getFFTSize());
    EXPECT_EQ (WindowType::hann, spectrogram->getWindowType());
    EXPECT_FLOAT_EQ (20.0f, spectrogram->getMinFrequency());
    EXPECT_FLOAT_EQ (20000.0f, spectrogram->getMaxFrequency());
    EXPECT_FLOAT_EQ (-100.0f, spectrogram->getMinDecibels());
    EXPECT_FLOAT_EQ (0.0f, spectrogram->getMaxDecibels());
    EXPECT_DOUBLE_EQ (44100.0, spectrogram->getSampleRate());
    EXPECT_EQ (256, spectrogram->getNumHistoryFrames());
    EXPECT_FLOAT_EQ (0.75f, spectrogram->getOverlapFactor());
    EXPECT_EQ (256, spectrogram->getColorMap().getNumColorStops());
    EXPECT_FALSE (spectrogram->getSpectrogramImage().isValid());
}

TEST_F (SpectrogramComponentTests, ConstructorUsesAnalyzerStateFFTSize)
{
    SpectrumAnalyzerState state4096 (4096);
    SpectrogramComponent spectrogram4096 (state4096);

    EXPECT_EQ (4096, spectrogram4096.getFFTSize());
}

//==============================================================================
// Configuration Tests
//==============================================================================

TEST_F (SpectrogramComponentTests, SetFFTSizeUpdatesComponentAndState)
{
    spectrogram->setFFTSize (4096);

    EXPECT_EQ (4096, spectrogram->getFFTSize());
    EXPECT_EQ (4096, state->getFftSize());

    spectrogram->setFFTSize (1024);

    EXPECT_EQ (1024, spectrogram->getFFTSize());
    EXPECT_EQ (1024, state->getFftSize());
}

TEST_F (SpectrogramComponentTests, SetWindowTypeUpdatesCurrentWindow)
{
    spectrogram->setWindowType (WindowType::blackmanHarris);
    EXPECT_EQ (WindowType::blackmanHarris, spectrogram->getWindowType());

    spectrogram->setWindowType (WindowType::rectangular);
    EXPECT_EQ (WindowType::rectangular, spectrogram->getWindowType());
}

TEST_F (SpectrogramComponentTests, SetFrequencyRangeUpdatesAndClampsValues)
{
    spectrogram->setFrequencyRange (100.0f, 5000.0f);

    EXPECT_FLOAT_EQ (100.0f, spectrogram->getMinFrequency());
    EXPECT_FLOAT_EQ (5000.0f, spectrogram->getMaxFrequency());

    spectrogram->setFrequencyRange (-100.0f, 500.0f);

    EXPECT_FLOAT_EQ (1.0f, spectrogram->getMinFrequency());
    EXPECT_FLOAT_EQ (500.0f, spectrogram->getMaxFrequency());

    spectrogram->setFrequencyRange (2000.0f, 100.0f);

    EXPECT_FLOAT_EQ (2000.0f, spectrogram->getMinFrequency());
    EXPECT_FLOAT_EQ (2001.0f, spectrogram->getMaxFrequency());
}

TEST_F (SpectrogramComponentTests, SetDecibelRangeUpdatesValues)
{
    spectrogram->setDecibelRange (-80.0f, 6.0f);

    EXPECT_FLOAT_EQ (-80.0f, spectrogram->getMinDecibels());
    EXPECT_FLOAT_EQ (6.0f, spectrogram->getMaxDecibels());
}

TEST_F (SpectrogramComponentTests, SetSampleRateUpdatesAndClampsValues)
{
    spectrogram->setSampleRate (48000.0);
    EXPECT_DOUBLE_EQ (48000.0, spectrogram->getSampleRate());

    spectrogram->setSampleRate (0.0);
    EXPECT_DOUBLE_EQ (1.0, spectrogram->getSampleRate());

    spectrogram->setSampleRate (-44100.0);
    EXPECT_DOUBLE_EQ (1.0, spectrogram->getSampleRate());
}

TEST_F (SpectrogramComponentTests, SetColorMapReplacesCurrentMap)
{
    spectrogram->setColorMap (SpectrogramColorMap::Type::viridis);

    SpectrogramColorMap expected (SpectrogramColorMap::Type::viridis);
    EXPECT_EQ (expected.map (0.0f), spectrogram->getColorMap().map (0.0f));
    EXPECT_EQ (expected.map (0.5f), spectrogram->getColorMap().map (0.5f));
    EXPECT_EQ (expected.map (1.0f), spectrogram->getColorMap().map (1.0f));
}

TEST_F (SpectrogramComponentTests, SetNumHistoryFramesUpdatesAndClampsToMinimum)
{
    spectrogram->setNumHistoryFrames (64);

    EXPECT_EQ (64, spectrogram->getNumHistoryFrames());

    spectrogram->setNumHistoryFrames (1);

    EXPECT_EQ (4, spectrogram->getNumHistoryFrames());
}

TEST_F (SpectrogramComponentTests, SetOverlapFactorDelegatesToAnalyzerState)
{
    spectrogram->setOverlapFactor (0.5f);

    EXPECT_FLOAT_EQ (0.5f, spectrogram->getOverlapFactor());
    EXPECT_FLOAT_EQ (0.5f, state->getOverlapFactor());
    EXPECT_EQ (spectrogram->getFFTSize() / 2, state->getHopSize());

    spectrogram->setOverlapFactor (0.0f);

    EXPECT_FLOAT_EQ (0.0f, spectrogram->getOverlapFactor());
    EXPECT_EQ (spectrogram->getFFTSize(), state->getHopSize());
}

TEST_F (SpectrogramComponentTests, SetScrollSpeedClampsToNonNegative)
{
    EXPECT_FLOAT_EQ (1.0f, spectrogram->getScrollSpeed());

    spectrogram->setScrollSpeed (2.0f);
    EXPECT_FLOAT_EQ (2.0f, spectrogram->getScrollSpeed());

    spectrogram->setScrollSpeed (0.0f);
    EXPECT_FLOAT_EQ (0.0f, spectrogram->getScrollSpeed());

    spectrogram->setScrollSpeed (-1.0f);
    EXPECT_FLOAT_EQ (0.0f, spectrogram->getScrollSpeed());
}

//==============================================================================
// Runtime Tests
//==============================================================================

TEST_F (SpectrogramComponentTests, RefreshDisplayWithoutAudioDataDoesNotCrash)
{
    spectrogram->refreshDisplay (1.0 / 60.0);

    EXPECT_TRUE (true);
}

TEST_F (SpectrogramComponentTests, RefreshDisplayWithAudioDataDoesNotCrash)
{
    const auto testData = createSineBuffer (2048, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));

    spectrogram->refreshDisplay (1.0 / 60.0);

    EXPECT_TRUE (true);
}

TEST_F (SpectrogramComponentTests, RefreshDisplaySkipsStaleBacklog)
{
    // refreshDisplay() only processes FFTs while the component is showing.
    auto parent = std::make_unique<Component> ("parent");
    parent->setVisible (true);
    parent->addAndMakeVisible (*spectrogram);

    spectrogram->setFFTSize (512);
    spectrogram->setOverlapFactor (0.75f); // hop = fftSize / 4

    // Fill the FIFO with far more than one analysis window (a stale backlog).
    const auto testData = createSineBuffer (spectrogram->getFFTSize() * 8, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));

    spectrogram->refreshDisplay (1.0 / 60.0);

    // The stale rows are skipped: after ingestion the FIFO holds less than a
    // full window instead of accumulating the backlog (which would make the
    // display lag progressively behind the audio).
    EXPECT_LT (state->getNumAvailableSamples(), spectrogram->getFFTSize());
}

TEST_F (SpectrogramComponentTests, ClearHistoryDoesNotChangeConfiguration)
{
    spectrogram->setFrequencyRange (100.0f, 10000.0f);
    spectrogram->setDecibelRange (-90.0f, -6.0f);
    spectrogram->setSampleRate (48000.0);
    spectrogram->setNumHistoryFrames (32);

    spectrogram->clearHistory();

    EXPECT_FLOAT_EQ (100.0f, spectrogram->getMinFrequency());
    EXPECT_FLOAT_EQ (10000.0f, spectrogram->getMaxFrequency());
    EXPECT_FLOAT_EQ (-90.0f, spectrogram->getMinDecibels());
    EXPECT_FLOAT_EQ (-6.0f, spectrogram->getMaxDecibels());
    EXPECT_DOUBLE_EQ (48000.0, spectrogram->getSampleRate());
    EXPECT_EQ (32, spectrogram->getNumHistoryFrames());
}

TEST_F (SpectrogramComponentTests, PaintWithoutAudioDataDoesNotCrash)
{
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    spectrogram->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrogramComponentTests, PaintAfterRefreshDisplayDoesNotCrash)
{
    const auto testData = createSineBuffer (2048, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));
    spectrogram->refreshDisplay (1.0 / 60.0);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    spectrogram->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrogramComponentTests, ResizedDoesNotCrash)
{
    spectrogram->setBounds (0.0f, 0.0f, 1000.0f, 600.0f);
    spectrogram->resized();

    spectrogram->setBounds (0.0f, 0.0f, 0.0f, 0.0f);
    spectrogram->resized();

    EXPECT_TRUE (true);
}

//==============================================================================
// Integration Tests
//==============================================================================

TEST_F (SpectrogramComponentTests, CompleteWorkflow)
{
    spectrogram->setFFTSize (4096);
    spectrogram->setWindowType (WindowType::hamming);
    spectrogram->setFrequencyRange (20.0f, 20000.0f);
    spectrogram->setDecibelRange (-100.0f, 0.0f);
    spectrogram->setSampleRate (48000.0);
    spectrogram->setNumHistoryFrames (128);
    spectrogram->setOverlapFactor (0.5f);
    spectrogram->setColorMap (SpectrogramColorMap::Type::warm);

    const auto testData = createSineBuffer (4096, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));
    spectrogram->refreshDisplay (1.0 / 60.0);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    spectrogram->paint (g);

    EXPECT_EQ (4096, spectrogram->getFFTSize());
    EXPECT_EQ (WindowType::hamming, spectrogram->getWindowType());
    EXPECT_EQ (128, spectrogram->getNumHistoryFrames());
    EXPECT_FLOAT_EQ (0.5f, spectrogram->getOverlapFactor());
}

//==============================================================================
// GPU Path Tests (skipped when no GPU backend is available)
//==============================================================================

class SpectrogramComponentGpuTests : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});
        if (gpuContext == nullptr)
            return;

        auto probe = GpuCanvas::create (*gpuContext, 64, 64);
        if (probe == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";

        paintCanvas = GpuCanvas::create (*gpuContext, 800, 400);
        if (paintCanvas == nullptr)
            GTEST_SKIP() << "Unable to create paint GpuCanvas";

        mm = MessageManager::getInstance();
        state = std::make_unique<SpectrumAnalyzerState> (2048);
        spectrogram = std::make_unique<SpectrogramComponent> (*state);
        spectrogram->setBounds (0.0f, 0.0f, 800.0f, 400.0f);

        // refreshDisplay() only processes FFTs while the component is showing.
        parent = std::make_unique<Component> ("parent");
        parent->setVisible (true);
        parent->addAndMakeVisible (*spectrogram);
    }

    void TearDown() override
    {
        spectrogram.reset();
        state.reset();
        paintCanvas.reset();
        parent.reset();
    }

    void paintComponent()
    {
        auto& g = paintCanvas->beginDraw();
        spectrogram->paint (g);
        paintCanvas->commit();
    }

    void pushOneFftRow()
    {
        const auto testData = createSineBuffer (spectrogram->getFFTSize(), 100.0f);
        state->pushSamples (testData.data(), static_cast<int> (testData.size()));
        spectrogram->refreshDisplay (1.0 / 60.0);
        paintComponent();
    }

    static std::unique_ptr<GraphicsContext> gpuContext;

    MessageManager* mm = nullptr;
    GpuCanvas::Ptr paintCanvas;
    std::unique_ptr<Component> parent;
    std::unique_ptr<SpectrumAnalyzerState> state;
    std::unique_ptr<SpectrogramComponent> spectrogram;
};

std::unique_ptr<GraphicsContext> SpectrogramComponentGpuTests::gpuContext;

TEST_F (SpectrogramComponentGpuTests, AppliesRowsAndScrollsHistoryOnGpu)
{
    spectrogram->setNumHistoryFrames (16);
    spectrogram->setOverlapFactor (0.0f); // one FFT per fftSize samples

    paintComponent(); // first paint creates the internal ping-pong canvases

    // First row: only the top row is non-background.
    pushOneFftRow();

    auto snapshot1 = spectrogram->getSpectrogramImage();

    ASSERT_TRUE (snapshot1.isValid());
    EXPECT_EQ (SpectrogramComponent::defaultSpectrogramRenderWidth, snapshot1.getWidth());
    EXPECT_EQ (16, snapshot1.getHeight());
    EXPECT_FALSE (spectrogramRowIsColor (snapshot1, 0, 0xFF0a0a0a));

    for (int row = 1; row < 16; ++row)
        EXPECT_TRUE (spectrogramRowIsColor (snapshot1, row, 0xFF0a0a0a));

    // Second row: the previous top row must have scrolled down by exactly one
    // row, and the new top row must be non-background.
    pushOneFftRow();

    auto snapshot2 = spectrogram->getSpectrogramImage();

    ASSERT_TRUE (snapshot2.isValid());
    EXPECT_TRUE (spectrogramRowsEqual (snapshot2, 1, snapshot1, 0));
    EXPECT_FALSE (spectrogramRowIsColor (snapshot2, 0, 0xFF0a0a0a));

    for (int row = 2; row < 16; ++row)
        EXPECT_TRUE (spectrogramRowIsColor (snapshot2, row, 0xFF0a0a0a));
}

TEST_F (SpectrogramComponentGpuTests, ClearHistoryResetsGpuHistoryToBackground)
{
    spectrogram->setNumHistoryFrames (8);
    spectrogram->setOverlapFactor (0.0f);

    paintComponent();
    pushOneFftRow();

    auto before = spectrogram->getSpectrogramImage();
    ASSERT_TRUE (before.isValid());
    EXPECT_FALSE (spectrogramRowIsColor (before, 0, 0xFF0a0a0a));

    spectrogram->clearHistory();
    paintComponent(); // clearHistory() destroys the history canvases; repaint to recreate them

    auto after = spectrogram->getSpectrogramImage();
    ASSERT_TRUE (after.isValid());

    for (int row = 0; row < 8; ++row)
        EXPECT_TRUE (spectrogramRowIsColor (after, row, 0xFF0a0a0a));
}

TEST_F (SpectrogramComponentGpuTests, GetSpectrogramImageReturnsCurrentHistoryImage)
{
    spectrogram->setNumHistoryFrames (8);

    paintComponent(); // first paint creates the internal ping-pong canvases

    const auto image = spectrogram->getSpectrogramImage();

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (SpectrogramComponent::defaultSpectrogramRenderWidth, image.getWidth());
    EXPECT_EQ (8, image.getHeight());
    EXPECT_EQ (PixelFormat::RGBA, image.getPixelFormat());
}
