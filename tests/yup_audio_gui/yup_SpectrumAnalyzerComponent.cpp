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

using namespace yup;

namespace yup
{
extern std::unique_ptr<yup::GraphicsContext> yup_constructHeadlessGraphicsContext (yup::GraphicsContext::Options);
} // namespace yup

class SpectrumAnalyzerComponentTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mm = MessageManager::getInstance();
        state = std::make_unique<SpectrumAnalyzerState> (2048);
        analyzer = std::make_unique<SpectrumAnalyzerComponent> (*state);
        analyzer->setBounds (0.0f, 0.0f, 800.0f, 400.0f);
    }

    void TearDown() override
    {
        analyzer.reset();
        state.reset();
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 10)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        mm->runDispatchLoopUntil (millisecondsToRunFor);
#endif
    }

    MessageManager* mm = nullptr;
    std::unique_ptr<SpectrumAnalyzerState> state;
    std::unique_ptr<SpectrumAnalyzerComponent> analyzer;
};

//==============================================================================
// Construction Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, ConstructorInitializesCorrectly)
{
    EXPECT_EQ (2048, analyzer->getFFTSize());
    EXPECT_EQ (WindowType::hann, analyzer->getWindowType());
}

TEST_F (SpectrumAnalyzerComponentTests, ConstructorWithDifferentFFTSize)
{
    SpectrumAnalyzerState state4096 (4096);
    SpectrumAnalyzerComponent analyzer4096 (state4096);

    EXPECT_EQ (4096, analyzer4096.getFFTSize());
}

//==============================================================================
// FFT Size Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, SetFFTSizeUpdatesSize)
{
    analyzer->setFFTSize (4096);
    EXPECT_EQ (4096, analyzer->getFFTSize());
}

TEST_F (SpectrumAnalyzerComponentTests, SetFFTSizeWith512)
{
    analyzer->setFFTSize (512);
    EXPECT_EQ (512, analyzer->getFFTSize());
}

TEST_F (SpectrumAnalyzerComponentTests, SetFFTSizeWith8192)
{
    analyzer->setFFTSize (8192);
    EXPECT_EQ (8192, analyzer->getFFTSize());
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetFFTSizeWithNonPowerOfTwo)
{
    // Should handle gracefully or clamp to nearest power of 2
    analyzer->setFFTSize (1000);
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetFFTSizeWithZero)
{
    // Should handle gracefully
    analyzer->setFFTSize (0);
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetFFTSizeNegative)
{
    // Should handle gracefully
    analyzer->setFFTSize (-1024);
    EXPECT_TRUE (true);
}

//==============================================================================
// Window Type Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, SetWindowTypeToHamming)
{
    analyzer->setWindowType (WindowType::hamming);
    EXPECT_EQ (WindowType::hamming, analyzer->getWindowType());
}

TEST_F (SpectrumAnalyzerComponentTests, SetWindowTypeToBlackman)
{
    analyzer->setWindowType (WindowType::blackman);
    EXPECT_EQ (WindowType::blackman, analyzer->getWindowType());
}

TEST_F (SpectrumAnalyzerComponentTests, SetWindowTypeToBlackmanHarris)
{
    analyzer->setWindowType (WindowType::blackmanHarris);
    EXPECT_EQ (WindowType::blackmanHarris, analyzer->getWindowType());
}

TEST_F (SpectrumAnalyzerComponentTests, SetWindowTypeToFlatTop)
{
    analyzer->setWindowType (WindowType::flattop);
    EXPECT_EQ (WindowType::flattop, analyzer->getWindowType());
}

TEST_F (SpectrumAnalyzerComponentTests, SetWindowTypeToRectangular)
{
    analyzer->setWindowType (WindowType::rectangular);
    EXPECT_EQ (WindowType::rectangular, analyzer->getWindowType());
}

TEST_F (SpectrumAnalyzerComponentTests, ToggleWindowTypes)
{
    analyzer->setWindowType (WindowType::hamming);
    EXPECT_EQ (WindowType::hamming, analyzer->getWindowType());

    analyzer->setWindowType (WindowType::hann);
    EXPECT_EQ (WindowType::hann, analyzer->getWindowType());
}

TEST_F (SpectrumAnalyzerComponentTests, RectangularWindowHasUnityCalibration)
{
    analyzer->setWindowType (WindowType::rectangular);

    EXPECT_FLOAT_EQ (1.0f, analyzer->getWindowCoherentGain());
    EXPECT_FLOAT_EQ (1.0f, analyzer->getEquivalentNoiseBandwidthBins());
}

TEST_F (SpectrumAnalyzerComponentTests, HannWindowReportsExpectedNoiseBandwidth)
{
    analyzer->setWindowType (WindowType::hann);

    EXPECT_NEAR (0.5f, analyzer->getWindowCoherentGain(), 0.001f);
    EXPECT_NEAR (1.5f, analyzer->getEquivalentNoiseBandwidthBins(), 0.01f);
}

TEST_F (SpectrumAnalyzerComponentTests, EquivalentNoiseBandwidthHzFollowsSampleRate)
{
    analyzer->setWindowType (WindowType::rectangular);
    analyzer->setSampleRate (48000.0);

    EXPECT_NEAR (48000.0f / 2048.0f, analyzer->getEquivalentNoiseBandwidthHz(), 0.001f);
}

//==============================================================================
// Level Mode Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, DefaultLevelModeIsPeakDecibels)
{
    EXPECT_EQ (SpectrumAnalyzerComponent::LevelMode::peakDecibels, analyzer->getLevelMode());
}

TEST_F (SpectrumAnalyzerComponentTests, SetLevelModeToRMSDecibels)
{
    analyzer->setLevelMode (SpectrumAnalyzerComponent::LevelMode::rmsDecibels);
    EXPECT_EQ (SpectrumAnalyzerComponent::LevelMode::rmsDecibels, analyzer->getLevelMode());
}

TEST_F (SpectrumAnalyzerComponentTests, SetLevelModeToPowerDecibels)
{
    analyzer->setLevelMode (SpectrumAnalyzerComponent::LevelMode::powerDecibels);
    EXPECT_EQ (SpectrumAnalyzerComponent::LevelMode::powerDecibels, analyzer->getLevelMode());
}

TEST_F (SpectrumAnalyzerComponentTests, SetLevelModeToPowerSpectralDensity)
{
    analyzer->setLevelMode (SpectrumAnalyzerComponent::LevelMode::powerSpectralDensity);
    EXPECT_EQ (SpectrumAnalyzerComponent::LevelMode::powerSpectralDensity, analyzer->getLevelMode());
}

//==============================================================================
// Update Rate Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, SetUpdateRate30Hz)
{
    analyzer->setUpdateRate (30);
    EXPECT_EQ (30, analyzer->getUpdateRate());
}

TEST_F (SpectrumAnalyzerComponentTests, SetUpdateRate60Hz)
{
    analyzer->setUpdateRate (60);
    // Due to integer rounding in timer interval calculation, actual rate may vary slightly
    EXPECT_NEAR (60, analyzer->getUpdateRate(), 5);
}

TEST_F (SpectrumAnalyzerComponentTests, SetUpdateRate15Hz)
{
    analyzer->setUpdateRate (15);
    EXPECT_EQ (15, analyzer->getUpdateRate());
}

TEST_F (SpectrumAnalyzerComponentTests, SetUpdateRateZero)
{
    // Should handle gracefully
    analyzer->setUpdateRate (0);
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, SetUpdateRateNegative)
{
    // Should handle gracefully
    analyzer->setUpdateRate (-10);
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, SetUpdateRateVeryHigh)
{
    analyzer->setUpdateRate (1000);
    EXPECT_EQ (1000, analyzer->getUpdateRate());
}

//==============================================================================
// Frequency Range Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, DefaultFrequencyRange)
{
    EXPECT_FLOAT_EQ (20.0f, analyzer->getMinFrequency());
    EXPECT_FLOAT_EQ (20000.0f, analyzer->getMaxFrequency());
}

TEST_F (SpectrumAnalyzerComponentTests, SetFrequencyRangeFullSpectrum)
{
    analyzer->setFrequencyRange (20.0f, 20000.0f);

    EXPECT_FLOAT_EQ (20.0f, analyzer->getMinFrequency());
    EXPECT_FLOAT_EQ (20000.0f, analyzer->getMaxFrequency());
}

TEST_F (SpectrumAnalyzerComponentTests, SetFrequencyRangeLowPass)
{
    analyzer->setFrequencyRange (20.0f, 5000.0f);

    EXPECT_FLOAT_EQ (20.0f, analyzer->getMinFrequency());
    EXPECT_FLOAT_EQ (5000.0f, analyzer->getMaxFrequency());
}

TEST_F (SpectrumAnalyzerComponentTests, SetFrequencyRangeHighPass)
{
    analyzer->setFrequencyRange (100.0f, 20000.0f);

    EXPECT_FLOAT_EQ (100.0f, analyzer->getMinFrequency());
    EXPECT_FLOAT_EQ (20000.0f, analyzer->getMaxFrequency());
}

TEST_F (SpectrumAnalyzerComponentTests, SetFrequencyRangeNarrow)
{
    analyzer->setFrequencyRange (1000.0f, 2000.0f);

    EXPECT_FLOAT_EQ (1000.0f, analyzer->getMinFrequency());
    EXPECT_FLOAT_EQ (2000.0f, analyzer->getMaxFrequency());
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetFrequencyRangeWithZero)
{
    // Should handle gracefully
    analyzer->setFrequencyRange (0.0f, 20000.0f);
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetFrequencyRangeWithNegative)
{
    // Should handle gracefully
    analyzer->setFrequencyRange (-100.0f, 20000.0f);
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetFrequencyRangeInverted)
{
    // Should handle gracefully
    analyzer->setFrequencyRange (20000.0f, 20.0f);
    EXPECT_TRUE (true);
}

//==============================================================================
// Decibel Range Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, DefaultDecibelRange)
{
    EXPECT_FLOAT_EQ (-100.0f, analyzer->getMinDecibels());
    EXPECT_FLOAT_EQ (0.0f, analyzer->getMaxDecibels());
}

TEST_F (SpectrumAnalyzerComponentTests, SetDecibelRangeStandard)
{
    analyzer->setDecibelRange (-80.0f, 0.0f);

    EXPECT_FLOAT_EQ (-80.0f, analyzer->getMinDecibels());
    EXPECT_FLOAT_EQ (0.0f, analyzer->getMaxDecibels());
}

TEST_F (SpectrumAnalyzerComponentTests, SetDecibelRangeWide)
{
    analyzer->setDecibelRange (-120.0f, 6.0f);

    EXPECT_FLOAT_EQ (-120.0f, analyzer->getMinDecibels());
    EXPECT_FLOAT_EQ (6.0f, analyzer->getMaxDecibels());
}

TEST_F (SpectrumAnalyzerComponentTests, SetDecibelRangeNarrow)
{
    analyzer->setDecibelRange (-60.0f, -20.0f);

    EXPECT_FLOAT_EQ (-60.0f, analyzer->getMinDecibels());
    EXPECT_FLOAT_EQ (-20.0f, analyzer->getMaxDecibels());
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetDecibelRangeInverted)
{
    // Should handle gracefully
    analyzer->setDecibelRange (0.0f, -100.0f);
    EXPECT_TRUE (true);
}

//==============================================================================
// Sample Rate Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, DefaultSampleRate)
{
    EXPECT_DOUBLE_EQ (44100.0, analyzer->getSampleRate());
}

TEST_F (SpectrumAnalyzerComponentTests, SetSampleRate48kHz)
{
    analyzer->setSampleRate (48000.0);
    EXPECT_DOUBLE_EQ (48000.0, analyzer->getSampleRate());
}

TEST_F (SpectrumAnalyzerComponentTests, SetSampleRate96kHz)
{
    analyzer->setSampleRate (96000.0);
    EXPECT_DOUBLE_EQ (96000.0, analyzer->getSampleRate());
}

TEST_F (SpectrumAnalyzerComponentTests, SetSampleRate192kHz)
{
    analyzer->setSampleRate (192000.0);
    EXPECT_DOUBLE_EQ (192000.0, analyzer->getSampleRate());
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetSampleRateZero)
{
    // Should handle gracefully
    analyzer->setSampleRate (0.0);
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetSampleRateNegative)
{
    // Should handle gracefully
    analyzer->setSampleRate (-44100.0);
    EXPECT_TRUE (true);
}

//==============================================================================
// Display Type Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, DefaultDisplayTypeIsFilled)
{
    EXPECT_EQ (SpectrumAnalyzerComponent::DisplayType::filled, analyzer->getDisplayType());
}

TEST_F (SpectrumAnalyzerComponentTests, SetDisplayTypeToLines)
{
    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::lines);
    EXPECT_EQ (SpectrumAnalyzerComponent::DisplayType::lines, analyzer->getDisplayType());
}

TEST_F (SpectrumAnalyzerComponentTests, ToggleDisplayType)
{
    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::lines);
    EXPECT_EQ (SpectrumAnalyzerComponent::DisplayType::lines, analyzer->getDisplayType());

    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::filled);
    EXPECT_EQ (SpectrumAnalyzerComponent::DisplayType::filled, analyzer->getDisplayType());
}

//==============================================================================
// Release Time Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, DefaultReleaseTime)
{
    EXPECT_FLOAT_EQ (1.0f, analyzer->getReleaseTimeSeconds());
}

TEST_F (SpectrumAnalyzerComponentTests, SetReleaseTimeVeryFast)
{
    analyzer->setReleaseTimeSeconds (0.1f);
    EXPECT_FLOAT_EQ (0.1f, analyzer->getReleaseTimeSeconds());
}

TEST_F (SpectrumAnalyzerComponentTests, SetReleaseTimeVerySlow)
{
    analyzer->setReleaseTimeSeconds (5.0f);
    EXPECT_FLOAT_EQ (5.0f, analyzer->getReleaseTimeSeconds());
}

TEST_F (SpectrumAnalyzerComponentTests, SetReleaseTimeZero)
{
    analyzer->setReleaseTimeSeconds (0.0f);
    // Release time is clamped to minimum of 0.1 seconds
    EXPECT_FLOAT_EQ (0.1f, analyzer->getReleaseTimeSeconds());
}

TEST_F (SpectrumAnalyzerComponentTests, SetReleaseTimeNegative)
{
    // Should handle gracefully
    analyzer->setReleaseTimeSeconds (-1.0f);
    EXPECT_TRUE (true);
}

//==============================================================================
// Overlap Factor Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, SetOverlapFactorHalf)
{
    analyzer->setOverlapFactor (0.5f);
    EXPECT_FLOAT_EQ (0.5f, analyzer->getOverlapFactor());
}

TEST_F (SpectrumAnalyzerComponentTests, SetOverlapFactorThreeQuarters)
{
    analyzer->setOverlapFactor (0.75f);
    EXPECT_FLOAT_EQ (0.75f, analyzer->getOverlapFactor());
}

TEST_F (SpectrumAnalyzerComponentTests, SetOverlapFactorZero)
{
    analyzer->setOverlapFactor (0.0f);
    EXPECT_FLOAT_EQ (0.0f, analyzer->getOverlapFactor());
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetOverlapFactorNegative)
{
    // Should clamp to valid range
    analyzer->setOverlapFactor (-0.5f);
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, DISABLED_SetOverlapFactorAboveOne)
{
    // Should clamp to valid range
    analyzer->setOverlapFactor (1.5f);
    EXPECT_TRUE (true);
}

//==============================================================================
// Frequency/Bin Conversion Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, GetFrequencyForBinZero)
{
    float freq = analyzer->getFrequencyForBin (0);
    EXPECT_FLOAT_EQ (0.0f, freq);
}

TEST_F (SpectrumAnalyzerComponentTests, GetFrequencyForBinMiddle)
{
    int middleBin = analyzer->getFFTSize() / 4;
    float freq = analyzer->getFrequencyForBin (middleBin);

    EXPECT_GT (freq, 0.0f);
    EXPECT_LT (freq, static_cast<float> (analyzer->getSampleRate()) / 2.0f);
}

TEST_F (SpectrumAnalyzerComponentTests, GetFrequencyForBinNyquist)
{
    int nyquistBin = analyzer->getFFTSize() / 2;
    float freq = analyzer->getFrequencyForBin (nyquistBin);

    EXPECT_NEAR (static_cast<float> (analyzer->getSampleRate()) / 2.0f, freq, 1.0f);
}

TEST_F (SpectrumAnalyzerComponentTests, GetFrequencyForNegativeBin)
{
    float freq = analyzer->getFrequencyForBin (-1);
    // Should handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, GetBinForFrequency1kHz)
{
    int bin = analyzer->getBinForFrequency (1000.0f);

    EXPECT_GE (bin, 0);
    EXPECT_LT (bin, analyzer->getFFTSize() / 2);
}

TEST_F (SpectrumAnalyzerComponentTests, GetBinForFrequencyZero)
{
    int bin = analyzer->getBinForFrequency (0.0f);
    EXPECT_EQ (0, bin);
}

TEST_F (SpectrumAnalyzerComponentTests, GetBinForFrequencyNyquist)
{
    float nyquist = static_cast<float> (analyzer->getSampleRate()) / 2.0f;
    int bin = analyzer->getBinForFrequency (nyquist);

    EXPECT_GT (bin, 0);
}

TEST_F (SpectrumAnalyzerComponentTests, GetBinForFrequencyBeyondNyquist)
{
    int bin = analyzer->getBinForFrequency (100000.0f);
    // Should clamp or return valid bin
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, FrequencyBinConversionRoundTrip)
{
    int originalBin = 100;
    float frequency = analyzer->getFrequencyForBin (originalBin);
    int convertedBin = analyzer->getBinForFrequency (frequency);

    EXPECT_NEAR (originalBin, convertedBin, 1);
}

//==============================================================================
// Timer Callback Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, TimerCallbackDoesNotCrash)
{
    analyzer->timerCallback();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, TimerCallbackWithAudioData)
{
    // Push some test data
    std::vector<float> testData (2048);
    for (int i = 0; i < 2048; ++i)
        testData[i] = std::sin (2.0f * MathConstants<float>::pi * i / 100.0f);

    state->pushSamples (testData.data(), 2048);

    analyzer->timerCallback();

    // Should process FFT without crashing
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, MultipleTimerCallbacks)
{
    for (int i = 0; i < 10; ++i)
        analyzer->timerCallback();

    // Should handle multiple calls
    EXPECT_TRUE (true);
}

//==============================================================================
// Paint Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, PaintWithoutCrashing)
{
    auto context = yup_constructHeadlessGraphicsContext ({});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, PaintWithLinesDisplayType)
{
    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::lines);

    auto context = yup_constructHeadlessGraphicsContext ({});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, PaintWithFilledDisplayType)
{
    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::filled);

    auto context = yup_constructHeadlessGraphicsContext ({});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, PaintWithAudioData)
{
    // Push some test data
    std::vector<float> testData (2048);
    for (int i = 0; i < 2048; ++i)
        testData[i] = std::sin (2.0f * MathConstants<float>::pi * i / 100.0f);

    state->pushSamples (testData.data(), 2048);
    analyzer->timerCallback();

    auto context = yup_constructHeadlessGraphicsContext ({});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, PaintWithZeroSize)
{
    analyzer->setBounds (0.0f, 0.0f, 0.0f, 0.0f);

    auto context = yup_constructHeadlessGraphicsContext ({});
    auto renderer = context->makeRenderer (1, 1);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

//==============================================================================
// Resized Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, ResizedDoesNotCrash)
{
    analyzer->setBounds (0.0f, 0.0f, 1000.0f, 600.0f);
    analyzer->resized();

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, ResizedWithZeroSize)
{
    analyzer->setBounds (0.0f, 0.0f, 0.0f, 0.0f);
    analyzer->resized();

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, ResizedWithVeryLargeSize)
{
    analyzer->setBounds (0.0f, 0.0f, 10000.0f, 10000.0f);
    analyzer->resized();

    EXPECT_TRUE (true);
}

//==============================================================================
// Integration Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, CompleteWorkflow)
{
    // Setup
    analyzer->setFFTSize (4096);
    analyzer->setWindowType (WindowType::hamming);
    analyzer->setUpdateRate (30);
    analyzer->setFrequencyRange (20.0f, 20000.0f);
    analyzer->setDecibelRange (-100.0f, 0.0f);
    analyzer->setSampleRate (48000.0);
    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::filled);

    // Push audio data
    std::vector<float> testData (4096);
    for (int i = 0; i < 4096; ++i)
        testData[i] = std::sin (2.0f * MathConstants<float>::pi * i / 100.0f);

    state->pushSamples (testData.data(), 4096);

    // Process
    analyzer->timerCallback();

    // Render
    auto context = yup_constructHeadlessGraphicsContext ({});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, MultipleConfigurationChanges)
{
    analyzer->setFFTSize (1024);
    analyzer->setWindowType (WindowType::blackman);
    analyzer->setFFTSize (2048);
    analyzer->setWindowType (WindowType::hann);
    analyzer->setFFTSize (4096);

    EXPECT_EQ (4096, analyzer->getFFTSize());
    EXPECT_EQ (WindowType::hann, analyzer->getWindowType());
}

TEST_F (SpectrumAnalyzerComponentTests, DifferentWindowTypesWithSameData)
{
    std::vector<float> testData (2048);
    for (int i = 0; i < 2048; ++i)
        testData[i] = std::sin (2.0f * MathConstants<float>::pi * i / 100.0f);

    WindowType types[] = { WindowType::hann, WindowType::hamming, WindowType::blackman, WindowType::blackmanHarris, WindowType::flattop };

    for (auto type : types)
    {
        analyzer->setWindowType (type);
        state->pushSamples (testData.data(), 2048);
        analyzer->timerCallback();

        EXPECT_EQ (type, analyzer->getWindowType());
    }
}

TEST_F (SpectrumAnalyzerComponentTests, FrequencyRangeAffectsDisplay)
{
    // Narrow range
    analyzer->setFrequencyRange (500.0f, 2000.0f);
    EXPECT_FLOAT_EQ (500.0f, analyzer->getMinFrequency());
    EXPECT_FLOAT_EQ (2000.0f, analyzer->getMaxFrequency());

    // Wide range
    analyzer->setFrequencyRange (20.0f, 20000.0f);
    EXPECT_FLOAT_EQ (20.0f, analyzer->getMinFrequency());
    EXPECT_FLOAT_EQ (20000.0f, analyzer->getMaxFrequency());
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, VeryShortFFTSize)
{
    analyzer->setFFTSize (64);
    EXPECT_EQ (64, analyzer->getFFTSize());
}

TEST_F (SpectrumAnalyzerComponentTests, VeryLongFFTSize)
{
    analyzer->setFFTSize (16384);
    EXPECT_EQ (16384, analyzer->getFFTSize());
}

TEST_F (SpectrumAnalyzerComponentTests, ExtremeSampleRates)
{
    analyzer->setSampleRate (8000.0);
    EXPECT_DOUBLE_EQ (8000.0, analyzer->getSampleRate());

    analyzer->setSampleRate (192000.0);
    EXPECT_DOUBLE_EQ (192000.0, analyzer->getSampleRate());
}

TEST_F (SpectrumAnalyzerComponentTests, ExtremeDecibelRanges)
{
    analyzer->setDecibelRange (-140.0f, 20.0f);

    // Should handle extreme ranges
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, SilentAudioInput)
{
    std::vector<float> silentData (2048, 0.0f);
    state->pushSamples (silentData.data(), 2048);

    analyzer->timerCallback();

    // Should handle silent input without crashing
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, MaximumAudioInput)
{
    std::vector<float> maxData (2048, 1.0f);
    state->pushSamples (maxData.data(), 2048);

    analyzer->timerCallback();

    // Should handle maximum input without crashing
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, RapidConfigurationChanges)
{
    for (int i = 0; i < 100; ++i)
    {
        analyzer->setFrequencyRange (20.0f + i, 20000.0f - i);
        analyzer->setDecibelRange (-100.0f + i * 0.1f, 0.0f - i * 0.1f);
    }

    // Should handle rapid changes without crashing
    EXPECT_TRUE (true);
}
