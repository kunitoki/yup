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

class SpectrogramComponentTests;

namespace yup
{

/** What the tests of the spectrogram reach of a component to put it in a window of their own. */
template <>
class ComponentTestHelper<SpectrogramComponentTests>
{
public:
    /** Attaches a native the caller built, which the component then owns. */
    static void attachNative (Component& component, ComponentNative* newNative)
    {
        component.native = newNative;
    }
};

} // namespace yup

class SpectrogramComponentTests : public ::testing::Test
{
protected:
    /** A window with no display: its frames are the ones a test sends, and it counts the repaints it is asked for. */
    class FakeWindow final : public ComponentNative
    {
    public:
        explicit FakeWindow (Component& window)
            : ComponentNative (window, ComponentNative::defaultFlags)
        {
        }

        using ComponentNative::callFrameListeners;
        using ComponentNative::hasFrameListeners;

        void repaint() override { ++numRepaints; }

        void repaint (const Rectangle<float>&) override { ++numRepaints; }

        const RectangleList<float>& getRepaintAreas() const override { return repaintAreas; }

        void setTitle (const String&) override {}

        String getTitle() const override { return {}; }

        void setVisible (bool) override {}

        bool isVisible() const override { return false; }

        bool isResizable() const override { return false; }

        void setResizable (bool) override {}

        void toFront() override {}

        void setSize (const Size<int>&) override {}

        Size<int> getSize() const override { return {}; }

        Size<int> getContentSize() const override { return {}; }

        Point<int> getPosition() const override { return {}; }

        void setPosition (const Point<int>&) override {}

        Rectangle<int> getBounds() const override { return {}; }

        void setBounds (const Rectangle<int>&) override {}

        Rectangle<int> getSafeAreaBounds() const override { return {}; }

        void setFullScreen (bool) override {}

        bool isFullScreen() const override { return false; }

        bool isDecorated() const override { return false; }

        void setAspectRatio (std::optional<float>) override {}

        std::optional<float> getAspectRatio() const override { return std::nullopt; }

        void setOpacity (float) override {}

        float getOpacity() const override { return 1.0f; }

        void setFocusedComponent (Component*, FocusChangeType) override {}

        Component* getFocusedComponent() const override { return nullptr; }

        Component* getPointerTarget() const override { return nullptr; }

        bool isContinuousRepaintingEnabled() const override { return false; }

        void enableContinuousRepainting (bool) override {}

        bool isAtomicModeEnabled() const override { return false; }

        void enableAtomicMode (bool) override {}

        bool isWireframeEnabled() const override { return false; }

        void enableWireframe (bool) override {}

        GpuTriangulationThresholds getTriangulationThresholds() const override { return {}; }

        void setTriangulationThresholds (const GpuTriangulationThresholds&) override {}

        void setGlobalMouseCaptureActive (bool) override {}

        void cancelCurrentMouseGesture() override {}

        void startTextInput (Component&) override {}

        void stopTextInput (Component&) override {}

        void updateTextInputRect (Component&) override {}

        float getScaleDpi() const override { return 1.0f; }

        float getCurrentFrameRate() const override { return 60.0f; }

        float getDesiredFrameRate() const override { return 60.0f; }

        void setDesiredFrameRate (std::optional<float>) override {}

        bool isVsyncEnabled() const override { return false; }

        void setVsyncEnabled (bool) override {}

        void* getNativeHandle() const override { return nullptr; }

        rive::Factory* getFactory() override { return nullptr; }

        GraphicsContext* getGraphicsContext() override { return nullptr; }

        int numRepaints = 0;

    private:
        RectangleList<float> repaintAreas;
    };

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

    /** Puts a component in a window of its own, shown, and returns the native that sends its frames. */
    static FakeWindow& makeWindow (Component& window)
    {
        auto* native = new FakeWindow (window);
        ComponentTestHelper<SpectrogramComponentTests>::attachNative (window, native);
        window.setVisible (true);
        return *native;
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

TEST_F (SpectrogramComponentTests, FFTSizeIsTheOneOfTheState)
{
    state->setFftSize (4096);

    EXPECT_EQ (4096, spectrogram->getFFTSize());

    state->setFftSize (1024);

    EXPECT_EQ (1024, spectrogram->getFFTSize());
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

    state->setFftSize (512);
    state->setOverlapFactor (0.75f); // hop = fftSize / 4

    // Fill the FIFO with far more than one analysis window (a stale backlog).
    const auto testData = createSineBuffer (spectrogram->getFFTSize() * 8, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));

    spectrogram->refreshDisplay (1.0 / 60.0);

    // The stale rows are skipped: after ingestion the FIFO holds less than a
    // full window instead of accumulating the backlog (which would make the
    // display lag progressively behind the audio).
    EXPECT_LT (state->getNumAvailableSamples(), spectrogram->getFFTSize());
}

TEST_F (SpectrogramComponentTests, FollowsALargerFFTSizeSetOnTheState)
{
    // refreshDisplay() only processes FFTs while the component is showing.
    auto parent = std::make_unique<Component> ("parent");
    parent->setVisible (true);
    parent->addAndMakeVisible (*spectrogram);

    // A frame of the state is now larger than the buffers the spectrogram sized in setState()
    state->setFftSize (8192);
    state->setOverlapFactor (0.0f);

    const auto testData = createSineBuffer (8192, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));

    spectrogram->refreshDisplay (1.0 / 60.0);

    EXPECT_EQ (8192, spectrogram->getFFTSize());
    EXPECT_EQ (0, state->getNumAvailableSamples());
}

TEST_F (SpectrogramComponentTests, NeverGoesBeyondTheMaximumFFTSizeOfItsState)
{
    SpectrumAnalyzerState small (256, 512);
    small.setOverlapFactor (0.0f);

    // refreshDisplay() only processes FFTs while the component is showing.
    auto parent = std::make_unique<Component> ("parent");
    parent->setVisible (true);
    parent->addAndMakeVisible (*spectrogram);

    spectrogram->setState (&small);

    EXPECT_EQ (256, spectrogram->getFFTSize());

    // The largest size the state takes is analyzed as any other, and the spectrogram leaves the state at it
    small.setFftSize (small.getMaxFftSize());

    const auto testData = createSineBuffer (512, 100.0f);
    small.pushSamples (testData.data(), static_cast<int> (testData.size()));

    spectrogram->refreshDisplay (1.0 / 60.0);

    EXPECT_EQ (512, spectrogram->getFFTSize());
    EXPECT_EQ (512, small.getFftSize());
    EXPECT_EQ (0, small.getNumAvailableSamples());

    spectrogram->setState (state.get());
}

TEST_F (SpectrogramComponentTests, RefreshDisplayWhileHiddenDropsTheBufferedSamples)
{
    const auto testData = createSineBuffer (2048, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));

    spectrogram->refreshDisplay (1.0 / 60.0);

    // What was pushed while the spectrogram was hidden is not shown when it comes back
    EXPECT_EQ (0, state->getNumAvailableSamples());
}

TEST_F (SpectrogramComponentTests, WhatWasPushedWhileHiddenIsDroppedWhenTheSpectrogramShows)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.addChildComponent (*spectrogram);

    // A hidden spectrogram takes no frame of its window, so nothing reads its state
    EXPECT_FALSE (native.hasFrameListeners());

    const auto testData = createSineBuffer (2048, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));

    spectrogram->setVisible (true);

    EXPECT_EQ (0, state->getNumAvailableSamples());
    EXPECT_TRUE (native.hasFrameListeners());

    // Hidden by its parent, and without a state
    window.setVisible (false);
    EXPECT_FALSE (native.hasFrameListeners());

    window.setVisible (true);
    EXPECT_TRUE (native.hasFrameListeners());

    spectrogram->setState (nullptr);
    EXPECT_FALSE (native.hasFrameListeners());

    spectrogram->setState (state.get());
    EXPECT_TRUE (native.hasFrameListeners());
}

TEST_F (SpectrogramComponentTests, ComesToRestWithoutSamplesAndWakesWhenTheyArrive)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.addAndMakeVisible (*spectrogram);

    const int numRepaints = native.numRepaints;

    // No row to show and none to scroll: the frame asks for no repaint, and is the last one taken
    native.callFrameListeners (1.0 / 60.0);

    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_EQ (numRepaints, native.numRepaints);

    // At rest the spectrogram looks at its state ten times a second, which is the call made here
    spectrogram->refreshDisplay (0.1);

    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_EQ (numRepaints, native.numRepaints);

    const auto testData = createSineBuffer (2048, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));

    spectrogram->refreshDisplay (0.1);

    EXPECT_TRUE (native.hasFrameListeners());
    EXPECT_GT (native.numRepaints, numRepaints);

    // The row waits for a paint, which keeps the frames coming: here there is no GPU to write it with, so
    // the paint drops it, and with the audio stopped the spectrogram is back at rest
    native.callFrameListeners (1.0 / 60.0);
    EXPECT_TRUE (native.hasFrameListeners());

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    spectrogram->paint (g);
    native.callFrameListeners (1.0 / 60.0);

    EXPECT_FALSE (native.hasFrameListeners());
}

TEST_F (SpectrogramComponentTests, ASpectrogramAtRestLooksForSamplesByItself)
{
#if YUP_MODAL_LOOPS_PERMITTED
    Component window ("window");
    auto& native = makeWindow (window);
    window.addAndMakeVisible (*spectrogram);

    native.callFrameListeners (1.0 / 60.0);
    ASSERT_FALSE (native.hasFrameListeners());

    const auto testData = createSineBuffer (2048, 100.0f);
    state->pushSamples (testData.data(), static_cast<int> (testData.size()));

    // Nothing tells the spectrogram that samples came, the audio thread least of all
    const auto startMs = Time::getMillisecondCounterHiRes();

    while (! native.hasFrameListeners() && Time::getMillisecondCounterHiRes() - startMs < 2000.0)
        mm->runDispatchLoopUntil (10);

    EXPECT_TRUE (native.hasFrameListeners());
#else
    GTEST_SKIP() << "the spectrogram looks at its state from a timer, which needs a message loop the test can run";
#endif
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
    state->setFftSize (4096);
    spectrogram->setWindowType (WindowType::hamming);
    spectrogram->setFrequencyRange (20.0f, 20000.0f);
    spectrogram->setDecibelRange (-100.0f, 0.0f);
    spectrogram->setSampleRate (48000.0);
    spectrogram->setNumHistoryFrames (128);
    state->setOverlapFactor (0.5f);
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
}

//==============================================================================
// State Tests
//==============================================================================

TEST_F (SpectrogramComponentTests, ASpectrogramWithoutAStateIsConfiguredAndPainted)
{
    SpectrogramComponent stateless;
    stateless.setBounds (0.0f, 0.0f, 300.0f, 150.0f);

    EXPECT_EQ (nullptr, stateless.getState());
    EXPECT_EQ (4096, stateless.getFFTSize());

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (300, 150);
    Graphics g (*context, *renderer);

    stateless.refreshDisplay (1.0 / 60.0);
    stateless.paint (g);
}

TEST_F (SpectrogramComponentTests, SetStateTakesTheFFTSizeOfTheNewState)
{
    SpectrumAnalyzerState other (1024);

    spectrogram->setState (&other);

    EXPECT_EQ (&other, spectrogram->getState());
    EXPECT_EQ (1024, spectrogram->getFFTSize());

    // The size is the one of the state the spectrogram has, which is where it is set
    other.setFftSize (512);

    EXPECT_EQ (512, spectrogram->getFFTSize());
    EXPECT_EQ (2048, state->getFftSize());

    spectrogram->setState (nullptr);

    EXPECT_EQ (nullptr, spectrogram->getState());
}

TEST_F (SpectrogramComponentTests, TheTypeInfoSetsTheColorMapAndTheRanges)
{
    const auto& type = SpectrogramComponent::getClassTypeInfo();
    EXPECT_EQ (type.getTypeName().toString(), "Spectrogram");
    EXPECT_EQ (&spectrogram->getTypeInfo(), &type);

    const auto created = type.createInstance();
    auto* instance = dynamic_cast<SpectrogramComponent*> (created.get());
    ASSERT_NE (nullptr, instance);
    EXPECT_EQ (nullptr, instance->getState());
    EXPECT_EQ (instance->getProperty ("colorMap").toString(), "heatmap");

    instance->setProperty ("colorMap", "viridis");
    EXPECT_EQ (SpectrogramColorMap::Type::viridis, instance->getColorMap().getType());
    EXPECT_EQ (instance->getProperty ("colorMap").toString(), "viridis");

    instance->setProperty ("maxFrequency", 10.0);
    instance->setProperty ("minFrequency", 2.0);
    EXPECT_FLOAT_EQ (2.0f, instance->getMinFrequency());
    EXPECT_FLOAT_EQ (10.0f, instance->getMaxFrequency());

    instance->setProperty ("historyFrames", 64);
    instance->setProperty ("scrollSpeed", 0.5);
    EXPECT_EQ (64, instance->getNumHistoryFrames());
    EXPECT_FLOAT_EQ (0.5f, instance->getScrollSpeed());
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

    // Shows grays up to 3 dB below full scale: a full-scale sine saturates it, where a level read too low does not
    void showLevelsAsGraysBelowFullScale()
    {
        spectrogram->setNumHistoryFrames (8);
        spectrogram->setColorMap (SpectrogramColorMap::Type::grayscale);
        spectrogram->setWindowType (WindowType::rectangular);
        spectrogram->setDecibelRange (-100.0f, -3.0f);
        state->setOverlapFactor (0.0f); // one FFT per fftSize samples
    }

    // One FFT frame of a sine of full scale lasting a whole number of periods, which puts it on a bin
    void pushOneFullScaleSineRow()
    {
        const auto testData = createSineBuffer (spectrogram->getFFTSize(), 32.0f);
        state->pushSamples (testData.data(), static_cast<int> (testData.size()));
        spectrogram->refreshDisplay (1.0 / 60.0);
        paintComponent();
    }

    // Returns true when a pixel of the given row is white
    static bool rowHasWhitePixel (const Image& image, int row)
    {
        const auto raw = image.getRawData();
        const auto rowBytes = static_cast<size_t> (image.getWidth()) * 4u;
        const auto rowData = raw.data() + static_cast<size_t> (row) * rowBytes;

        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto pixel = rowData + static_cast<size_t> (x) * 4u;

            if (pixel[0] == 0xff && pixel[1] == 0xff && pixel[2] == 0xff && pixel[3] == 0xff)
                return true;
        }

        return false;
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
    state->setOverlapFactor (0.0f); // one FFT per fftSize samples

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

TEST_F (SpectrogramComponentGpuTests, FrameProducingMoreRowsThanOnePassKeepsEveryRow)
{
    spectrogram->setNumHistoryFrames (32);
    state->setFftSize (512);
    state->setOverlapFactor (0.75f); // hop = fftSize / 4

    paintComponent(); // first paint creates the internal ping-pong canvases

    // One analysis window per refresh, as the audio arrives, so none is skipped as stale. Four refreshes
    // before the next paint queue more rows than a single GPU pass can write.
    const int hopSize = spectrogram->getFFTSize() / 4;
    const auto testData = createSineBuffer (spectrogram->getFFTSize() + 3 * hopSize, 100.0f);

    state->pushSamples (testData.data(), spectrogram->getFFTSize());
    spectrogram->refreshDisplay (1.0 / 60.0);

    for (int i = 0; i < 3; ++i)
    {
        state->pushSamples (testData.data() + spectrogram->getFFTSize() + i * hopSize, hopSize);
        spectrogram->refreshDisplay (1.0 / 60.0);
    }

    paintComponent();

    const auto image = spectrogram->getSpectrogramImage();
    ASSERT_TRUE (image.isValid());

    // The rows written by this frame sit at the top of the history, above the cleared rows. A single
    // pass only writes defaultSpectrogramMagnitudes / defaultSpectrogramWidth of them, so the frame has
    // to drain its rows over several passes instead of dropping the surplus, which is what made how
    // much of the waterfall survived depend on the frame rate.
    int writtenRows = 0;

    while (writtenRows < image.getHeight()
           && ! spectrogramRowIsColor (image, writtenRows, 0xff0a0a0au))
        ++writtenRows;

    EXPECT_GE (writtenRows, 3);
}

TEST_F (SpectrogramComponentGpuTests, ClearHistoryResetsGpuHistoryToBackground)
{
    spectrogram->setNumHistoryFrames (8);
    state->setOverlapFactor (0.0f);

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

TEST_F (SpectrogramComponentGpuTests, AFullScaleSineReadsZeroDecibels)
{
    showLevelsAsGraysBelowFullScale();

    paintComponent(); // first paint creates the internal ping-pong canvases
    pushOneFullScaleSineRow();

    const auto image = spectrogram->getSpectrogramImage();

    ASSERT_TRUE (image.isValid());
    EXPECT_TRUE (rowHasWhitePixel (image, 0));
}

TEST_F (SpectrogramComponentGpuTests, FollowsASmallerFFTSizeSetOnTheState)
{
    showLevelsAsGraysBelowFullScale();

    paintComponent(); // first paint creates the internal ping-pong canvases

    // The spectrogram took a size of 2048 from the state in setState(): a frame of 512 samples
    // analyzed at that size would read the sine 12 dB too low
    state->setFftSize (512);
    pushOneFullScaleSineRow();

    const auto image = spectrogram->getSpectrogramImage();

    ASSERT_TRUE (image.isValid());
    EXPECT_TRUE (rowHasWhitePixel (image, 0));
}
