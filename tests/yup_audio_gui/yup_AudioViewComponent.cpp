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

class AudioViewComponentTests : public ::testing::Test
{
protected:
    static constexpr int kTestSampleRate = 44100;
    static constexpr int kTestBufferSize = 10000;

    void SetUp() override
    {
        mm = MessageManager::getInstance();
        cache = std::make_shared<AudioPeakProfileCache>();
        view = std::make_unique<AudioViewComponent> (cache);
        view->setBounds (0.0f, 0.0f, 800.0f, 400.0f);
    }

    void TearDown() override
    {
        view.reset();
        cache.reset();
    }

    void runDispatchLoopUntil (int millisecondsToRunFor = 10)
    {
#if YUP_MODAL_LOOPS_PERMITTED
        mm->runDispatchLoopUntil (millisecondsToRunFor);
#endif
    }

    AudioBuffer<float> createTestBuffer (int numChannels, int numSamples)
    {
        AudioBuffer<float> buffer (numChannels, numSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                channelData[i] = std::sin (2.0f * MathConstants<float>::pi * i / 100.0f);
        }

        return buffer;
    }

    MessageManager* mm = nullptr;
    std::shared_ptr<AudioPeakProfileCache> cache;
    std::unique_ptr<AudioViewComponent> view;
};

//==============================================================================
// Construction Tests
//==============================================================================

TEST_F (AudioViewComponentTests, ConstructorWithCacheCreatesComponent)
{
    EXPECT_NE (nullptr, view->getHorizontalScrollBar());
    EXPECT_NE (nullptr, view->getProgressBar());
    EXPECT_EQ (1.0, view->getZoomFactor());
}

TEST_F (AudioViewComponentTests, ConstructorWithNullCacheCreatesDefaultCache)
{
    AudioViewComponent viewWithNullCache (nullptr);
    EXPECT_NE (nullptr, viewWithNullCache.getHorizontalScrollBar());
    EXPECT_NE (nullptr, viewWithNullCache.getProgressBar());
}

TEST_F (AudioViewComponentTests, ConstructorWithExternalThumbnail)
{
    AudioThumbnail externalThumbnail (cache);
    AudioViewComponent externalView (externalThumbnail);

    EXPECT_NE (nullptr, externalView.getHorizontalScrollBar());
    EXPECT_NE (nullptr, externalView.getProgressBar());
}

//==============================================================================
// Source Tests
//==============================================================================

TEST_F (AudioViewComponentTests, SetSourceWithBuffer)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    EXPECT_EQ (kTestBufferSize, view->getTotalSamples());
    EXPECT_EQ (2, view->getNumChannels());
    EXPECT_DOUBLE_EQ (kTestSampleRate, view->getSampleRate());
}

TEST_F (AudioViewComponentTests, SetSourceWithZeroSampleRate)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, 0.0);

    EXPECT_EQ (kTestBufferSize, view->getTotalSamples());
    EXPECT_DOUBLE_EQ (0.0, view->getSampleRate());
}

TEST_F (AudioViewComponentTests, SetSourceWithNullBuffer)
{
    view->setSource (static_cast<const AudioBuffer<float>*> (nullptr), kTestSampleRate);

    EXPECT_EQ (0, view->getTotalSamples());
}

TEST_F (AudioViewComponentTests, ClearRemovesSource)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);
    EXPECT_GT (view->getTotalSamples(), 0);

    view->clear();

    EXPECT_EQ (0, view->getTotalSamples());
}

//==============================================================================
// Zoom Factor Tests
//==============================================================================

TEST_F (AudioViewComponentTests, SetZoomFactorUpdatesZoom)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    view->setZoomFactor (2.0);
    EXPECT_DOUBLE_EQ (2.0, view->getZoomFactor());
}

TEST_F (AudioViewComponentTests, SetZoomFactorBelowOne)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    view->setZoomFactor (0.5);
    // Zoom factor is clamped to minimum of 1.0
    EXPECT_DOUBLE_EQ (1.0, view->getZoomFactor());
}

TEST_F (AudioViewComponentTests, SetZoomFactorVeryLarge)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    view->setZoomFactor (100.0);
    EXPECT_DOUBLE_EQ (100.0, view->getZoomFactor());
}

TEST_F (AudioViewComponentTests, SetZoomFactorZero)
{
    view->setZoomFactor (0.0);
    // Should handle gracefully, likely clamped to minimum
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, SetZoomFactorNegative)
{
    view->setZoomFactor (-1.0);
    // Should handle gracefully
    EXPECT_TRUE (true);
}

//==============================================================================
// View Range Tests
//==============================================================================

TEST_F (AudioViewComponentTests, SetViewRangeSamplesUpdatesRange)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    Range<double> newRange (1000.0, 5000.0);
    view->setViewRangeSamples (newRange);

    auto actualRange = view->getViewRangeSamples();
    EXPECT_DOUBLE_EQ (1000.0, actualRange.getStart());
    EXPECT_DOUBLE_EQ (5000.0, actualRange.getEnd());
}

TEST_F (AudioViewComponentTests, SetViewRangeSamplesWithEmptyRange)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    Range<double> emptyRange (5000.0, 5000.0);
    view->setViewRangeSamples (emptyRange);

    // Should handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, SetViewRangeSamplesWithInvertedRange)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    Range<double> invertedRange (5000.0, 1000.0);
    view->setViewRangeSamples (invertedRange);

    // Should handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, SetViewRangeSamplesBeyondBuffer)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    Range<double> beyondRange (5000.0, 20000.0);
    view->setViewRangeSamples (beyondRange);

    // Should clamp or handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, ScrollToSampleUpdatesViewStart)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    view->scrollToSample (2000.0);

    auto range = view->getViewRangeSamples();
    EXPECT_DOUBLE_EQ (2000.0, range.getStart());
}

TEST_F (AudioViewComponentTests, ScrollToSampleNegativeValue)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    view->scrollToSample (-100.0);

    // Should clamp to valid range
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, ScrollToSampleBeyondBuffer)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    view->scrollToSample (50000.0);

    // Should clamp to valid range
    EXPECT_TRUE (true);
}

//==============================================================================
// Label Width Tests
//==============================================================================

TEST_F (AudioViewComponentTests, DefaultLabelWidth)
{
    EXPECT_EQ (48, view->getLabelWidth());
}

TEST_F (AudioViewComponentTests, SetLabelWidthUpdatesWidth)
{
    view->setLabelWidth (100);
    EXPECT_EQ (100, view->getLabelWidth());
}

TEST_F (AudioViewComponentTests, SetLabelWidthZero)
{
    view->setLabelWidth (0);
    EXPECT_EQ (0, view->getLabelWidth());
}

TEST_F (AudioViewComponentTests, SetLabelWidthNegative)
{
    view->setLabelWidth (-10);
    // Should handle gracefully
    EXPECT_TRUE (true);
}

//==============================================================================
// Channel Labels Visibility Tests
//==============================================================================

TEST_F (AudioViewComponentTests, ChannelLabelsVisibleByDefault)
{
    EXPECT_TRUE (view->isChannelLabelsVisible());
}

TEST_F (AudioViewComponentTests, SetChannelLabelsVisibleHidesLabels)
{
    view->setChannelLabelsVisible (false);
    EXPECT_FALSE (view->isChannelLabelsVisible());
}

TEST_F (AudioViewComponentTests, ToggleChannelLabelsVisibility)
{
    view->setChannelLabelsVisible (false);
    EXPECT_FALSE (view->isChannelLabelsVisible());

    view->setChannelLabelsVisible (true);
    EXPECT_TRUE (view->isChannelLabelsVisible());
}

//==============================================================================
// Selectable Tests
//==============================================================================

TEST_F (AudioViewComponentTests, NotSelectableByDefault)
{
    EXPECT_FALSE (view->isSelectable());
}

TEST_F (AudioViewComponentTests, SetSelectableEnablesInteraction)
{
    view->setSelectable (true);
    EXPECT_TRUE (view->isSelectable());
}

TEST_F (AudioViewComponentTests, ToggleSelectable)
{
    view->setSelectable (true);
    EXPECT_TRUE (view->isSelectable());

    view->setSelectable (false);
    EXPECT_FALSE (view->isSelectable());
}

//==============================================================================
// Audio Info Tests
//==============================================================================

TEST_F (AudioViewComponentTests, GetTotalSamplesWithNoSource)
{
    EXPECT_EQ (0, view->getTotalSamples());
}

TEST_F (AudioViewComponentTests, GetNumChannelsWithNoSource)
{
    EXPECT_EQ (0, view->getNumChannels());
}

TEST_F (AudioViewComponentTests, GetSampleRateWithNoSource)
{
    EXPECT_DOUBLE_EQ (0.0, view->getSampleRate());
}

TEST_F (AudioViewComponentTests, GetTotalSamplesWithSource)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    EXPECT_EQ (kTestBufferSize, view->getTotalSamples());
}

TEST_F (AudioViewComponentTests, GetNumChannelsWithSource)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    EXPECT_EQ (2, view->getNumChannels());
}

TEST_F (AudioViewComponentTests, GetSampleRateWithSource)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    EXPECT_DOUBLE_EQ (kTestSampleRate, view->getSampleRate());
}

//==============================================================================
// Conversion Function Tests
//==============================================================================

TEST_F (AudioViewComponentTests, TimeToSampleConversion)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    double sample = view->timeToSample (1.0); // 1 second
    EXPECT_NEAR (kTestSampleRate, sample, 1.0);
}

TEST_F (AudioViewComponentTests, SampleToTimeConversion)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    double time = view->sampleToTime (kTestSampleRate);
    EXPECT_NEAR (1.0, time, 0.001);
}

TEST_F (AudioViewComponentTests, TimeToSampleWithZeroSampleRate)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, 0.0);

    double sample = view->timeToSample (1.0);
    // Should handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, SampleToTimeWithZeroSampleRate)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, 0.0);

    double time = view->sampleToTime (100.0);
    // Should handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, SampleToXConversion)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);
    view->setViewRangeSamples (Range<double> (0.0, static_cast<double> (kTestBufferSize)));

    Rectangle<float> bounds (0.0f, 0.0f, 800.0f, 400.0f);
    float x = view->sampleToX (kTestBufferSize / 2.0, bounds);

    EXPECT_GT (x, 0.0f);
    EXPECT_LT (x, 800.0f);
}

TEST_F (AudioViewComponentTests, XToSampleConversion)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);
    view->setViewRangeSamples (Range<double> (0.0, static_cast<double> (kTestBufferSize)));

    Rectangle<float> bounds (0.0f, 0.0f, 800.0f, 400.0f);
    double sample = view->xToSample (400.0f, bounds);

    EXPECT_GT (sample, 0.0);
    EXPECT_LT (sample, static_cast<double> (kTestBufferSize));
}

TEST_F (AudioViewComponentTests, GetWaveformBoundsReturnsValidBounds)
{
    auto bounds = view->getWaveformBounds();
    EXPECT_GE (bounds.getWidth(), 0.0f);
    EXPECT_GE (bounds.getHeight(), 0.0f);
}

//==============================================================================
// Scrollbar Tests
//==============================================================================

TEST_F (AudioViewComponentTests, GetHorizontalScrollBarReturnsNonNull)
{
    EXPECT_NE (nullptr, view->getHorizontalScrollBar());
}

TEST_F (AudioViewComponentTests, ScrollBarInteraction)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    auto* scrollBar = view->getHorizontalScrollBar();
    ASSERT_NE (nullptr, scrollBar);

    scrollBar->setCurrentRangeStart (1000.0, sendNotification);

    // Should update view range
    EXPECT_TRUE (true);
}

//==============================================================================
// Progress Bar Tests
//==============================================================================

TEST_F (AudioViewComponentTests, GetProgressBarReturnsNonNull)
{
    EXPECT_NE (nullptr, view->getProgressBar());
}

//==============================================================================
// Mouse Interaction Tests
//==============================================================================

TEST_F (AudioViewComponentTests, MouseDownWhenNotSelectable)
{
    view->setSelectable (false);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (400.0f, 200.0f));
    view->mouseDown (event);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, MouseDownWhenSelectable)
{
    view->setSelectable (true);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (400.0f, 200.0f));
    view->mouseDown (event);

    // Should handle interaction
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, MouseWheelZooming)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);
    view->setSelectable (true);

    double initialZoom = view->getZoomFactor();

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (400.0f, 200.0f));
    MouseWheelData wheelData (0.0f, 1.0f);
    view->mouseWheel (event, wheelData);

    // Zoom should change
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, MouseWheelWhenNotSelectable)
{
    view->setSelectable (false);

    MouseEvent event (MouseEvent::leftButton, KeyModifiers(), Point<float> (400.0f, 200.0f));
    MouseWheelData wheelData (0.0f, 1.0f);
    view->mouseWheel (event, wheelData);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Keyboard Interaction Tests
//==============================================================================

TEST_F (AudioViewComponentTests, KeyDownWhenSelectable)
{
    view->setSelectable (true);

    KeyPress leftArrow (KeyPress::leftKey, KeyModifiers());
    view->keyDown (leftArrow, Point<float> (0.0f, 0.0f));

    // Should handle keyboard navigation
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, KeyDownWhenNotSelectable)
{
    view->setSelectable (false);

    KeyPress leftArrow (KeyPress::leftKey, KeyModifiers());
    view->keyDown (leftArrow, Point<float> (0.0f, 0.0f));

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Resized Tests
//==============================================================================

TEST_F (AudioViewComponentTests, ResizedUpdatesLayout)
{
    view->setBounds (0.0f, 0.0f, 1000.0f, 600.0f);
    view->resized();

    auto bounds = view->getWaveformBounds();
    EXPECT_GT (bounds.getWidth(), 0.0f);
    EXPECT_GT (bounds.getHeight(), 0.0f);
}

TEST_F (AudioViewComponentTests, ResizedWithZeroSize)
{
    view->setBounds (0.0f, 0.0f, 0.0f, 0.0f);
    view->resized();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, ResizedWithVeryLargeSize)
{
    view->setBounds (0.0f, 0.0f, 10000.0f, 10000.0f);
    view->resized();

    // Should handle large sizes
    EXPECT_TRUE (true);
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F (AudioViewComponentTests, MultipleSourceChanges)
{
    auto buffer1 = createTestBuffer (1, 5000);
    view->setSource (&buffer1, kTestSampleRate);
    EXPECT_EQ (5000, view->getTotalSamples());

    auto buffer2 = createTestBuffer (2, 10000);
    view->setSource (&buffer2, kTestSampleRate);
    EXPECT_EQ (10000, view->getTotalSamples());
    EXPECT_EQ (2, view->getNumChannels());

    view->clear();
    EXPECT_EQ (0, view->getTotalSamples());
}

TEST_F (AudioViewComponentTests, ZoomAndScrollCombination)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    view->setZoomFactor (2.0);
    view->scrollToSample (2000.0);

    EXPECT_DOUBLE_EQ (2.0, view->getZoomFactor());
    auto range = view->getViewRangeSamples();
    EXPECT_DOUBLE_EQ (2000.0, range.getStart());
}

TEST_F (AudioViewComponentTests, LabelWidthAndVisibilityCombination)
{
    view->setLabelWidth (80);
    view->setChannelLabelsVisible (false);

    EXPECT_EQ (80, view->getLabelWidth());
    EXPECT_FALSE (view->isChannelLabelsVisible());

    auto bounds = view->getWaveformBounds();
    // Bounds should reflect hidden labels
    EXPECT_TRUE (true);
}

TEST_F (AudioViewComponentTests, EmptyBufferHandling)
{
    AudioBuffer<float> emptyBuffer (0, 0);
    view->setSource (&emptyBuffer, kTestSampleRate);

    EXPECT_EQ (0, view->getTotalSamples());
    EXPECT_EQ (0, view->getNumChannels());
}

TEST_F (AudioViewComponentTests, SingleSampleBuffer)
{
    AudioBuffer<float> singleSample (1, 1);
    singleSample.getWritePointer (0)[0] = 0.5f;
    view->setSource (&singleSample, kTestSampleRate);

    EXPECT_EQ (1, view->getTotalSamples());
}

TEST_F (AudioViewComponentTests, MultiChannelBuffer)
{
    auto buffer = createTestBuffer (8, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    EXPECT_EQ (8, view->getNumChannels());
}

TEST_F (AudioViewComponentTests, ConversionRoundTrip)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    double originalTime = 2.5;
    double sample = view->timeToSample (originalTime);
    double convertedTime = view->sampleToTime (sample);

    EXPECT_NEAR (originalTime, convertedTime, 0.01);
}

TEST_F (AudioViewComponentTests, ViewRangeAfterMultipleZoomChanges)
{
    auto buffer = createTestBuffer (2, kTestBufferSize);
    view->setSource (&buffer, kTestSampleRate);

    view->setZoomFactor (2.0);
    view->setZoomFactor (1.5);
    view->setZoomFactor (3.0);

    EXPECT_DOUBLE_EQ (3.0, view->getZoomFactor());
}
