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
extern std::unique_ptr<yup::GraphicsContext> yup_constructHeadlessGraphicsContext (yup::GpuDevice::Options, yup::GpuDevice::Ptr);
} // namespace yup

class SpectrumAnalyzerComponentTests;

namespace yup
{

/** What the tests of the analyzer reach of a component to put it in a window of their own. */
template <>
class ComponentTestHelper<SpectrumAnalyzerComponentTests>
{
public:
    /** Attaches a native the caller built, which the component then owns. */
    static void attachNative (Component& component, ComponentNative* newNative)
    {
        component.native = newNative;
    }
};

} // namespace yup

class SpectrumAnalyzerComponentTests : public ::testing::Test
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

    /** Puts a component in a window of its own, shown, and returns the native that sends its frames. */
    static FakeWindow& makeWindow (Component& window)
    {
        auto* native = new FakeWindow (window);
        ComponentTestHelper<SpectrumAnalyzerComponentTests>::attachNative (window, native);
        window.setVisible (true);
        return *native;
    }

    // A sine of full scale lasting a whole number of periods, which puts it on a bin of any FFT size
    static void pushFullScaleSine (SpectrumAnalyzerState& target, int numSamples)
    {
        constexpr int period = 32;

        std::vector<float> sine (static_cast<size_t> (numSamples));

        for (int i = 0; i < numSamples; ++i)
            sine[static_cast<size_t> (i)] = std::sin (MathConstants<float>::twoPi * static_cast<float> (i % period) / static_cast<float> (period));

        target.pushSamples (sine.data(), numSamples);
    }

    static float getHighestLevel (const SpectrumAnalyzerComponent& component)
    {
        const auto levels = component.getLevels();

        return *std::max_element (levels.begin(), levels.end());
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

TEST_F (SpectrumAnalyzerComponentTests, FFTSizeIsTheOneOfTheState)
{
    state->setFftSize (4096);
    EXPECT_EQ (4096, analyzer->getFFTSize());

    state->setFftSize (512);
    EXPECT_EQ (512, analyzer->getFFTSize());

    state->setFftSize (8192);
    EXPECT_EQ (8192, analyzer->getFFTSize());
}

TEST_F (SpectrumAnalyzerComponentTests, FollowsASmallerFFTSizeSetOnTheState)
{
    analyzer->setVisible (true);
    analyzer->setWindowType (WindowType::rectangular);

    // The analyzer took a size of 2048 from the state in setState()
    state->setFftSize (512);
    pushFullScaleSine (*state, 512);
    analyzer->refreshDisplay (1.0 / 60.0);

    EXPECT_EQ (512, analyzer->getFFTSize());
    EXPECT_LT (state->getNumAvailableSamples(), 512);
    EXPECT_NEAR (1.0f, getHighestLevel (*analyzer), 0.01f);
}

TEST_F (SpectrumAnalyzerComponentTests, FollowsALargerFFTSizeSetOnTheState)
{
    analyzer->setVisible (true);
    analyzer->setWindowType (WindowType::rectangular);

    // A frame of the state is now larger than the buffers the analyzer sized in setState()
    state->setFftSize (8192);
    pushFullScaleSine (*state, 8192);
    analyzer->refreshDisplay (1.0 / 60.0);

    EXPECT_EQ (8192, analyzer->getFFTSize());
    EXPECT_LT (state->getNumAvailableSamples(), 8192);
    EXPECT_NEAR (1.0f, getHighestLevel (*analyzer), 0.01f);
}

TEST_F (SpectrumAnalyzerComponentTests, NeverGoesBeyondTheMaximumFFTSizeOfItsState)
{
    SpectrumAnalyzerState small (256, 512);

    analyzer->setVisible (true);
    analyzer->setWindowType (WindowType::rectangular);
    analyzer->setState (&small);

    EXPECT_EQ (256, analyzer->getFFTSize());

    // The largest size the state takes is analyzed as any other, and the analyzer leaves the state at it
    small.setFftSize (small.getMaxFftSize());
    pushFullScaleSine (small, 512);
    analyzer->refreshDisplay (1.0 / 60.0);

    EXPECT_EQ (512, analyzer->getFFTSize());
    EXPECT_EQ (512, small.getFftSize());
    EXPECT_LT (small.getNumAvailableSamples(), 512);
    EXPECT_NEAR (1.0f, getHighestLevel (*analyzer), 0.01f);

    analyzer->setState (state.get());
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
    EXPECT_EQ (60, analyzer->getUpdateRate());
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

TEST_F (SpectrumAnalyzerComponentTests, DefaultUpdateRateFollowsDisplay)
{
    EXPECT_EQ (0, analyzer->getUpdateRate());
}

TEST_F (SpectrumAnalyzerComponentTests, SetUpdateRateZeroFollowsDisplay)
{
    analyzer->setUpdateRate (30);
    analyzer->setUpdateRate (0);
    EXPECT_EQ (0, analyzer->getUpdateRate());
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

TEST_F (SpectrumAnalyzerComponentTests, TellsItsLevelsAsASeriesWithARevision)
{
    // One level for each display point, at rest until a spectrum is analyzed
    const auto levels = analyzer->getLevels();
    ASSERT_EQ (levels.size(), static_cast<size_t> (SpectrumAnalyzerComponent::scopeSize));
    EXPECT_TRUE (std::all_of (levels.begin(), levels.end(), [] (float level) { return level == 0.0f; }));

    const auto* property = analyzer->getTypeInfo().findProperty (Identifier ("levels"));
    ASSERT_NE (property, nullptr);
    EXPECT_TRUE (property->isSeries());
    EXPECT_TRUE (property->isReadOnly());
    EXPECT_EQ (property->elementType, ComponentProperty::Type::number);

    // A drawing reads the levels where the analyzer keeps them
    const auto view = property->arrayGetter (*analyzer);
    EXPECT_EQ (view.data, levels.data());
    EXPECT_EQ (view.size, static_cast<int> (SpectrumAnalyzerComponent::scopeSize));
    EXPECT_EQ (view.revision, analyzer->getLevelsRevision());
    EXPECT_NE (view.revision, 0u);

    // The revision tells when they change: another state starts from silence
    const auto revision = analyzer->getLevelsRevision();
    SpectrumAnalyzerState other (2048);

    analyzer->setState (&other);
    EXPECT_NE (analyzer->getLevelsRevision(), revision);
    EXPECT_EQ (analyzer->getLevels().data(), levels.data());

    analyzer->setState (state.get());
}

TEST_F (SpectrumAnalyzerComponentTests, AStyleSetOnTheAnalyzerPaintsItInPlaceOfItsOwnPaint)
{
    struct CountingStyle final : ComponentStyle
    {
        void paint (Graphics&, const ApplicationTheme&, const Component&) override { ++numPaints; }

        int numPaints = 0;
    };

    const auto oldTheme = ApplicationTheme::getGlobalTheme();
    ApplicationTheme::setGlobalTheme (new ApplicationTheme());

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (800, 400);

    {
        Graphics g (*context, *renderer);

        ReferenceCountedObjectPtr<CountingStyle> style (new CountingStyle());
        analyzer->setStyle (style);
        analyzer->paint (g);
        EXPECT_EQ (style->numPaints, 1);

        // Without one the analyzer paints itself
        analyzer->setStyle (nullptr);
        analyzer->paint (g);
        EXPECT_EQ (style->numPaints, 1);
    }

    ApplicationTheme::setGlobalTheme (oldTheme.get());
}

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
// Refresh Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, RefreshDisplayDoesNotCrash)
{
    analyzer->refreshDisplay (1.0 / 60.0);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, RefreshDisplayWithAudioData)
{
    // Push some test data
    std::vector<float> testData (2048);
    for (int i = 0; i < 2048; ++i)
        testData[i] = std::sin (2.0f * MathConstants<float>::pi * i / 100.0f);

    state->pushSamples (testData.data(), 2048);

    analyzer->refreshDisplay (1.0 / 60.0);

    // Should process FFT without crashing
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, MultipleRefreshDisplays)
{
    for (int i = 0; i < 10; ++i)
        analyzer->refreshDisplay (1.0 / 60.0);

    // Should handle multiple calls
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, AFixedUpdateRateTakesTheFramesItNeeds)
{
    analyzer->setVisible (true);
    analyzer->setUpdateRate (30);

    pushFullScaleSine (*state, 2048);

    // The frames come every 20 ms and an update is due every 33: the first frame is left out
    analyzer->refreshDisplay (0.02);
    EXPECT_FLOAT_EQ (0.0f, getHighestLevel (*analyzer));

    analyzer->refreshDisplay (0.02);
    EXPECT_GT (getHighestLevel (*analyzer), 0.5f);
}

//==============================================================================
// Idle Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, AFrameWithNothingNewAsksForNoRepaintAndLeavesTheRevisionAlone)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.addAndMakeVisible (*analyzer);

    const auto initialRevision = analyzer->getLevelsRevision();
    const int initialRepaints = native.numRepaints;

    pushFullScaleSine (*state, 2048);
    native.callFrameListeners (1.0 / 60.0);

    // The spectrum of the sine came up
    const auto revision = analyzer->getLevelsRevision();
    const int numRepaints = native.numRepaints;

    EXPECT_NE (initialRevision, revision);
    EXPECT_GT (numRepaints, initialRepaints);
    EXPECT_GT (getHighestLevel (*analyzer), 0.5f);

    // The next frame comes before the next spectrum, with the levels where the last one put them
    native.callFrameListeners (1.0 / 60.0);

    EXPECT_EQ (revision, analyzer->getLevelsRevision());
    EXPECT_EQ (numRepaints, native.numRepaints);

    // The audio may well be going on: the analyzer keeps looking at every frame
    EXPECT_TRUE (native.hasFrameListeners());
}

TEST_F (SpectrumAnalyzerComponentTests, ComesToRestWhenTheAudioStopsAndWakesWhenItComesBack)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.addAndMakeVisible (*analyzer);

    analyzer->setReleaseTimeSeconds (0.1f);

    pushFullScaleSine (*state, 2048);
    native.callFrameListeners (1.0 / 60.0);

    EXPECT_GT (getHighestLevel (*analyzer), 0.5f);

    // No more samples: the spectrum falls, and the analyzer takes the frames it needs to show it falling
    native.callFrameListeners (0.25);
    EXPECT_TRUE (native.hasFrameListeners());

    for (int frame = 0; frame < 100 && native.hasFrameListeners(); ++frame)
        native.callFrameListeners (0.25);

    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_FLOAT_EQ (0.0f, getHighestLevel (*analyzer));

    // At rest the analyzer looks at its state ten times a second, which is the call made here: with no
    // samples it asks nothing of its window
    const auto revision = analyzer->getLevelsRevision();
    const int numRepaints = native.numRepaints;

    analyzer->refreshDisplay (0.1);

    EXPECT_FALSE (native.hasFrameListeners());
    EXPECT_EQ (revision, analyzer->getLevelsRevision());
    EXPECT_EQ (numRepaints, native.numRepaints);

    // The audio is back
    pushFullScaleSine (*state, 2048);
    analyzer->refreshDisplay (0.1);

    EXPECT_TRUE (native.hasFrameListeners());
    EXPECT_NE (revision, analyzer->getLevelsRevision());
    EXPECT_GT (native.numRepaints, numRepaints);
    EXPECT_GT (getHighestLevel (*analyzer), 0.5f);
}

TEST_F (SpectrumAnalyzerComponentTests, AnAnalyzerAtRestLooksForSamplesByItself)
{
#if YUP_MODAL_LOOPS_PERMITTED
    Component window ("window");
    auto& native = makeWindow (window);
    window.addAndMakeVisible (*analyzer);

    // Nothing was ever pushed: the first frame finds nothing to do
    native.callFrameListeners (1.0 / 60.0);
    ASSERT_FALSE (native.hasFrameListeners());

    pushFullScaleSine (*state, 2048);

    // Nothing tells the analyzer that samples came, the audio thread least of all
    const auto startMs = Time::getMillisecondCounterHiRes();

    while (! native.hasFrameListeners() && Time::getMillisecondCounterHiRes() - startMs < 2000.0)
        runDispatchLoopUntil (10);

    EXPECT_TRUE (native.hasFrameListeners());
    EXPECT_GT (getHighestLevel (*analyzer), 0.5f);
#else
    GTEST_SKIP() << "the analyzer looks at its state from a timer, which needs a message loop the test can run";
#endif
}

TEST_F (SpectrumAnalyzerComponentTests, AHiddenAnalyzerOrOneWithoutAStateHasNoFrameListener)
{
    Component window ("window");
    auto& native = makeWindow (window);

    // A component is hidden until it is made visible
    window.addChildComponent (*analyzer);
    EXPECT_FALSE (native.hasFrameListeners());

    analyzer->setVisible (true);
    EXPECT_TRUE (native.hasFrameListeners());

    // Hidden by its parent
    window.setVisible (false);
    EXPECT_FALSE (native.hasFrameListeners());

    window.setVisible (true);
    EXPECT_TRUE (native.hasFrameListeners());

    analyzer->setState (nullptr);
    EXPECT_FALSE (native.hasFrameListeners());

    analyzer->setState (state.get());
    EXPECT_TRUE (native.hasFrameListeners());
}

TEST_F (SpectrumAnalyzerComponentTests, AnAnalyzerThatWasNotToldItShowsWakesWhenPainted)
{
    // A subclass can keep the notifications of the analyzer for itself: being painted still tells it that it shows
    struct QuietAnalyzer final : SpectrumAnalyzerComponent
    {
        void visibilityChanged() override {}

        void parentHierarchyChanged() override {}
    };

    QuietAnalyzer quiet;
    quiet.setBounds (0.0f, 0.0f, 300.0f, 150.0f);
    quiet.setState (state.get());

    Component window ("window");
    auto& native = makeWindow (window);
    window.addAndMakeVisible (quiet);

    EXPECT_FALSE (native.hasFrameListeners());

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (300, 150);
    Graphics g (*context, *renderer);

    quiet.paint (g);

    EXPECT_TRUE (native.hasFrameListeners());
}

//==============================================================================
// Style Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, StyleColorsResolveFromComponent)
{
    analyzer->setColor (SpectrumAnalyzerComponent::Style::outlineColorId, Colors::red);
    analyzer->setColor (SpectrumAnalyzerComponent::Style::fillColorId, Colors::blue);

    EXPECT_EQ (Colors::red, ApplicationTheme::findComponentColor (*analyzer, SpectrumAnalyzerComponent::Style::outlineColorId));
    EXPECT_EQ (Colors::blue, ApplicationTheme::findComponentColor (*analyzer, SpectrumAnalyzerComponent::Style::fillColorId));
}

TEST_F (SpectrumAnalyzerComponentTests, PaintWithStyleColors)
{
    analyzer->setColor (SpectrumAnalyzerComponent::Style::backgroundTopColorId, Colors::white);
    analyzer->setColor (SpectrumAnalyzerComponent::Style::backgroundBottomColorId, Colors::gray);
    analyzer->setColor (SpectrumAnalyzerComponent::Style::gridColorId, Colors::black);
    analyzer->setColor (SpectrumAnalyzerComponent::Style::textColorId, Colors::black);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

//==============================================================================
// Paint Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, PaintWithoutCrashing)
{
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, PaintWithLinesDisplayType)
{
    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::lines);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, PaintWithFilledDisplayType)
{
    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::filled);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
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
    analyzer->refreshDisplay (1.0 / 60.0);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, PaintWithZeroSize)
{
    analyzer->setBounds (0.0f, 0.0f, 0.0f, 0.0f);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
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
    state->setFftSize (4096);
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

    // Process: at 30 updates a second the frame of an update is one that comes 1/30 s after the last
    analyzer->refreshDisplay (1.0 / 30.0);

    // Render
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    analyzer->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, MultipleConfigurationChanges)
{
    state->setFftSize (1024);
    analyzer->setWindowType (WindowType::blackman);
    state->setFftSize (2048);
    analyzer->setWindowType (WindowType::hann);
    state->setFftSize (4096);

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
        analyzer->refreshDisplay (1.0 / 60.0);

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

TEST_F (SpectrumAnalyzerComponentTests, AnalyzesAtTheSmallestAndTheLargestFFTSize)
{
    analyzer->setVisible (true);

    for (const int fftSize : { SpectrumAnalyzerState::minFftSize, SpectrumAnalyzerState::maxFftSize })
    {
        state->setFftSize (fftSize);
        pushFullScaleSine (*state, fftSize);
        analyzer->refreshDisplay (1.0 / 60.0);

        EXPECT_EQ (fftSize, analyzer->getFFTSize());
        EXPECT_LT (state->getNumAvailableSamples(), fftSize);
    }
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

    analyzer->refreshDisplay (1.0 / 60.0);

    // Should handle silent input without crashing
    EXPECT_TRUE (true);
}

TEST_F (SpectrumAnalyzerComponentTests, MaximumAudioInput)
{
    std::vector<float> maxData (2048, 1.0f);
    state->pushSamples (maxData.data(), 2048);

    analyzer->refreshDisplay (1.0 / 60.0);

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

TEST_F (SpectrumAnalyzerComponentTests, PaintWithEveryLevelModeAndDisplayType)
{
    const SpectrumAnalyzerComponent::LevelMode levelModes[] = {
        SpectrumAnalyzerComponent::LevelMode::peakDecibels,
        SpectrumAnalyzerComponent::LevelMode::rmsDecibels,
        SpectrumAnalyzerComponent::LevelMode::powerDecibels,
        SpectrumAnalyzerComponent::LevelMode::powerSpectralDensity
    };

    const SpectrumAnalyzerComponent::DisplayType displayTypes[] = {
        SpectrumAnalyzerComponent::DisplayType::filled,
        SpectrumAnalyzerComponent::DisplayType::lines
    };

    std::vector<float> testData (2048);

    for (int i = 0; i < 2048; ++i)
        testData[static_cast<size_t> (i)] = std::sin (2.0f * MathConstants<float>::pi * static_cast<float> (i) / 100.0f);

    state->pushSamples (testData.data(), 2048);
    analyzer->refreshDisplay (1.0 / 60.0);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    for (auto levelMode : levelModes)
    {
        analyzer->setLevelMode (levelMode);

        for (auto displayType : displayTypes)
        {
            analyzer->setDisplayType (displayType);
            analyzer->paint (g);

            EXPECT_EQ (levelMode, analyzer->getLevelMode());
            EXPECT_EQ (displayType, analyzer->getDisplayType());
        }
    }
}

TEST_F (SpectrumAnalyzerComponentTests, PaintWithBandsNarrowerThanOneBin)
{
    // 20 Hz > 25 Hz at 44.1 kHz makes every display band narrower than an FFT bin, so the rendered
    // levels come from the interpolation between the surrounding bins.
    state->setFftSize (2048);
    analyzer->setSampleRate (44100.0);
    analyzer->setFrequencyRange (20.0f, 25.0f);

    std::vector<float> testData (2048);

    for (int i = 0; i < 2048; ++i)
        testData[static_cast<size_t> (i)] = std::sin (2.0f * MathConstants<float>::pi * static_cast<float> (i) / 400.0f);

    state->pushSamples (testData.data(), 2048);
    analyzer->refreshDisplay (1.0 / 60.0);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (400, 200);
    Graphics g (*context, *renderer);

    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::filled);
    analyzer->paint (g);

    analyzer->setDisplayType (SpectrumAnalyzerComponent::DisplayType::lines);
    analyzer->paint (g);

    EXPECT_FLOAT_EQ (20.0f, analyzer->getMinFrequency());
    EXPECT_FLOAT_EQ (25.0f, analyzer->getMaxFrequency());
}

TEST_F (SpectrumAnalyzerComponentTests, PaintAfterEveryMappingChangingConfiguration)
{
    // The bin mapping is rebuilt from the FFT size, the sample rate and the frequency range, so
    // every combination has to keep rendering. The FFT size is taken from the state at each update,
    // which only a showing analyzer runs.
    const double sampleRates[] = { 44100.0, 48000.0, 96000.0 };
    const int fftSizes[] = { 512, 2048, 8192 };

    analyzer->setVisible (true);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (300, 150);
    Graphics g (*context, *renderer);

    for (auto sampleRate : sampleRates)
    {
        for (auto fftSize : fftSizes)
        {
            analyzer->setSampleRate (sampleRate);
            state->setFftSize (fftSize);
            analyzer->setFrequencyRange (30.0f, static_cast<float> (sampleRate) / 3.0f);

            std::vector<float> testData (static_cast<size_t> (fftSize), 0.5f);

            state->pushSamples (testData.data(), fftSize);
            analyzer->refreshDisplay (1.0 / 60.0);
            analyzer->paint (g);

            EXPECT_EQ (fftSize, analyzer->getFFTSize());
        }
    }
}

//==============================================================================
// State Tests
//==============================================================================

TEST_F (SpectrumAnalyzerComponentTests, AnAnalyzerWithoutAStateIsConfiguredAndPainted)
{
    SpectrumAnalyzerComponent stateless;
    stateless.setBounds (0.0f, 0.0f, 300.0f, 150.0f);

    EXPECT_EQ (nullptr, stateless.getState());
    EXPECT_EQ (4096, stateless.getFFTSize());

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (300, 150);
    Graphics g (*context, *renderer);

    stateless.refreshDisplay (1.0 / 60.0);
    stateless.paint (g);
}

TEST_F (SpectrumAnalyzerComponentTests, SetStateTakesTheFFTSizeOfTheNewState)
{
    SpectrumAnalyzerState other (1024);

    analyzer->setVisible (true);
    analyzer->setState (&other);

    EXPECT_EQ (&other, analyzer->getState());
    EXPECT_EQ (1024, analyzer->getFFTSize());

    // The size is the one of the state the analyzer has, which is where it is set
    other.setFftSize (512);
    analyzer->refreshDisplay (1.0 / 60.0);

    EXPECT_EQ (512, analyzer->getFFTSize());
    EXPECT_EQ (2048, state->getFftSize());

    // Without a state the analyzer keeps the size it last analyzed at
    analyzer->setState (nullptr);

    EXPECT_EQ (nullptr, analyzer->getState());
    EXPECT_EQ (512, analyzer->getFFTSize());
}

TEST_F (SpectrumAnalyzerComponentTests, AFullScaleSineReadsZeroDecibels)
{
    analyzer->setVisible (true);
    analyzer->setWindowType (WindowType::rectangular);
    analyzer->setDecibelRange (-100.0f, 20.0f);

    pushFullScaleSine (*state, 2048);
    analyzer->refreshDisplay (1.0 / 60.0);

    // 0 dB sits at 100 of the 120 dB of the range, with room above it to tell a level that is too high:
    // 6 dB, the difference between a one-sided and a two-sided spectrum, are 0.05
    EXPECT_NEAR (100.0f / 120.0f, getHighestLevel (*analyzer), 0.02f);
}

TEST_F (SpectrumAnalyzerComponentTests, SamplesThatAreNotFiniteLeaveNoTraceInTheLevels)
{
    analyzer->setVisible (true);
    analyzer->setLevelMode (SpectrumAnalyzerComponent::LevelMode::powerDecibels);

    // A block that is not a number, then one whose magnitude overflows, as an unstable filter gives
    for (const float value : { std::numeric_limits<float>::quiet_NaN(), 1.0e30f })
    {
        const std::vector<float> block (2048, value);

        state->pushSamples (block.data(), 2048);
        analyzer->refreshDisplay (1.0 / 60.0);
    }

    // The frames of the next update only hold the sine
    pushFullScaleSine (*state, 4096);
    analyzer->refreshDisplay (1.0 / 60.0);

    const auto levels = analyzer->getLevels();

    EXPECT_TRUE (std::all_of (levels.begin(), levels.end(), [] (float level) { return std::isfinite (level); }));
    EXPECT_GT (getHighestLevel (*analyzer), 0.8f);
}

TEST_F (SpectrumAnalyzerComponentTests, DecibelRangeStaysWithinItsLimits)
{
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 400);
    Graphics g (*context, *renderer);

    // The grid has a line every 10 dB: it has to stay a few lines to paint, whatever the range asked for
    analyzer->setDecibelRange (-1.0e9f, 1.0e9f);
    analyzer->paint (g);

    EXPECT_FLOAT_EQ (SpectrumAnalysis::lowestDecibels, analyzer->getMinDecibels());
    EXPECT_FLOAT_EQ (SpectrumAnalysis::highestDecibels, analyzer->getMaxDecibels());

    // A document writes any number, one end at a time
    analyzer->setProperty ("minDecibels", 1.0e30);
    analyzer->paint (g);

    EXPECT_FLOAT_EQ (SpectrumAnalysis::highestDecibels - 1.0f, analyzer->getMinDecibels());
    EXPECT_FLOAT_EQ (SpectrumAnalysis::highestDecibels, analyzer->getMaxDecibels());

    analyzer->setProperty ("maxDecibels", -1.0e30);
    analyzer->paint (g);

    EXPECT_FLOAT_EQ (SpectrumAnalysis::lowestDecibels, analyzer->getMinDecibels());
    EXPECT_FLOAT_EQ (SpectrumAnalysis::lowestDecibels + 1.0f, analyzer->getMaxDecibels());
}

TEST_F (SpectrumAnalyzerComponentTests, TheTypeInfoSetsTheRangesInAnyOrder)
{
    const auto& type = SpectrumAnalyzerComponent::getClassTypeInfo();
    EXPECT_EQ (type.getTypeName().toString(), "SpectrumAnalyzer");
    EXPECT_EQ (&analyzer->getTypeInfo(), &type);

    const auto created = type.createInstance();
    auto* instance = dynamic_cast<SpectrumAnalyzerComponent*> (created.get());
    ASSERT_NE (nullptr, instance);
    EXPECT_EQ (nullptr, instance->getState());
    EXPECT_EQ (instance->getProperty ("displayType").toString(), "filled");
    EXPECT_EQ (instance->getProperty ("windowType").toString(), "hann");

    // An end written past the other one moves it out of its way, one written before it leaves it alone
    instance->setProperty ("minFrequency", 30000.0);
    instance->setProperty ("maxFrequency", 40000.0);
    EXPECT_FLOAT_EQ (30000.0f, instance->getMinFrequency());
    EXPECT_FLOAT_EQ (40000.0f, instance->getMaxFrequency());

    instance->setProperty ("maxDecibels", -120.0);
    instance->setProperty ("minDecibels", -140.0);
    EXPECT_FLOAT_EQ (-140.0f, instance->getMinDecibels());
    EXPECT_FLOAT_EQ (-120.0f, instance->getMaxDecibels());

    // A value a range can't take is ignored
    instance->setProperty ("minFrequency", -5.0);
    instance->setProperty ("sampleRate", 0.0);
    EXPECT_FLOAT_EQ (30000.0f, instance->getMinFrequency());
    EXPECT_DOUBLE_EQ (44100.0, instance->getSampleRate());

    instance->setProperty ("windowType", "blackmanHarris");
    instance->setProperty ("levelMode", "rmsDecibels");
    instance->setProperty ("displayType", "lines");
    EXPECT_EQ (WindowType::blackmanHarris, instance->getWindowType());
    EXPECT_EQ (SpectrumAnalyzerComponent::LevelMode::rmsDecibels, instance->getLevelMode());
    EXPECT_EQ (SpectrumAnalyzerComponent::DisplayType::lines, instance->getDisplayType());
}
