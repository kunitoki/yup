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
    EXPECT_EQ (25, spectrogram->getUpdateRate());
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

TEST_F (SpectrogramComponentTests, SetUpdateRateClampsToSupportedRange)
{
    spectrogram->setUpdateRate (30);
    EXPECT_EQ (30, spectrogram->getUpdateRate());

    spectrogram->setUpdateRate (0);
    EXPECT_EQ (1, spectrogram->getUpdateRate());

    spectrogram->setUpdateRate (-10);
    EXPECT_EQ (1, spectrogram->getUpdateRate());

    spectrogram->setUpdateRate (1000);
    EXPECT_GE (spectrogram->getUpdateRate(), 60);
    EXPECT_LE (spectrogram->getUpdateRate(), 63);
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
    ASSERT_TRUE (spectrogram->getSpectrogramImage().isValid());
    EXPECT_EQ (SpectrogramComponent::defaultSpectrogramWidth, spectrogram->getSpectrogramImage().getWidth());
    EXPECT_EQ (64, spectrogram->getSpectrogramImage().getHeight());

    spectrogram->setNumHistoryFrames (1);

    EXPECT_EQ (4, spectrogram->getNumHistoryFrames());
    ASSERT_TRUE (spectrogram->getSpectrogramImage().isValid());
    EXPECT_EQ (SpectrogramComponent::defaultSpectrogramWidth, spectrogram->getSpectrogramImage().getWidth());
    EXPECT_EQ (4, spectrogram->getSpectrogramImage().getHeight());
}

TEST_F (SpectrogramComponentTests, GetSpectrogramImageReturnsCurrentHistoryImage)
{
    spectrogram->setNumHistoryFrames (8);

    const auto& image = spectrogram->getSpectrogramImage();

    ASSERT_TRUE (image.isValid());
    EXPECT_EQ (SpectrogramComponent::defaultSpectrogramWidth, image.getWidth());
    EXPECT_EQ (8, image.getHeight());
    EXPECT_EQ (PixelFormat::RGBA, image.getPixelFormat());
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

//==============================================================================
// Runtime Tests
//==============================================================================

TEST_F (SpectrogramComponentTests, TimerCallbackWithoutAudioDataDoesNotCrash)
{
    spectrogram->timerCallback();

    EXPECT_TRUE (true);
}

TEST_F (SpectrogramComponentTests, TimerCallbackWithAudioDataDoesNotCrash)
{
    const auto testData = createSineBuffer (2048, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));

    spectrogram->timerCallback();

    EXPECT_TRUE (true);
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
    ASSERT_TRUE (spectrogram->getSpectrogramImage().isValid());
    EXPECT_EQ (SpectrogramComponent::defaultSpectrogramWidth, spectrogram->getSpectrogramImage().getWidth());
    EXPECT_EQ (32, spectrogram->getSpectrogramImage().getHeight());
}

TEST_F (SpectrogramComponentTests, PaintWithoutAudioDataDoesNotCrash)
{
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    spectrogram->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrogramComponentTests, PaintAfterTimerCallbackDoesNotCrash)
{
    const auto testData = createSineBuffer (2048, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));
    spectrogram->timerCallback();

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
    spectrogram->setUpdateRate (30);
    spectrogram->setFrequencyRange (20.0f, 20000.0f);
    spectrogram->setDecibelRange (-100.0f, 0.0f);
    spectrogram->setSampleRate (48000.0);
    spectrogram->setNumHistoryFrames (128);
    spectrogram->setOverlapFactor (0.5f);
    spectrogram->setColorMap (SpectrogramColorMap::Type::warm);

    const auto testData = createSineBuffer (4096, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));
    spectrogram->timerCallback();

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    spectrogram->paint (g);

    EXPECT_EQ (4096, spectrogram->getFFTSize());
    EXPECT_EQ (WindowType::hamming, spectrogram->getWindowType());
    EXPECT_EQ (30, spectrogram->getUpdateRate());
    EXPECT_EQ (128, spectrogram->getNumHistoryFrames());
    EXPECT_FLOAT_EQ (0.5f, spectrogram->getOverlapFactor());
}
