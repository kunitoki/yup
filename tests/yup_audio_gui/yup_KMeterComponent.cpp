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

//==============================================================================
TEST (KMeterComponentTests, DefaultsAreConfigured)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);

    EXPECT_TRUE (meter.getShowPeak());
    EXPECT_TRUE (meter.getShowPeakHold());
    EXPECT_EQ (KMeterComponent::ScaleMapping::segmented, meter.getScaleMapping());
}

TEST (KMeterComponentTests, SettersUpdateState)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);

    meter.setShowPeak (false);
    meter.setShowPeakHold (false);
    meter.setScaleMapping (KMeterComponent::ScaleMapping::linear);
    meter.setAverageIntegrationTime (0.2);
    meter.setAverageFallTime (0.9);
    meter.setPeakHoldTime (1.5);
    meter.setOverCounterMode (KMeterState::OverCounterMode::total);

    EXPECT_FALSE (meter.getShowPeak());
    EXPECT_FALSE (meter.getShowPeakHold());
    EXPECT_EQ (KMeterComponent::ScaleMapping::linear, meter.getScaleMapping());
    EXPECT_DOUBLE_EQ (0.2, meterState.getIntegrationTime());
    EXPECT_DOUBLE_EQ (0.9, meterState.getAverageFallTime());
    EXPECT_DOUBLE_EQ (1.5, meterState.getPeakHoldTime());
    EXPECT_EQ (KMeterState::OverCounterMode::total, meter.getOverCounterMode());
}

TEST (KMeterComponentTests, PaintWithThemeDoesNotCrash)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    meter.setBounds (0.0f, 0.0f, 60.0f, 240.0f);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (120, 240);
    Graphics g (*context, *renderer);

    meter.paint (g);

    EXPECT_TRUE (true);
}

TEST (KMeterComponentTests, GetChannelReturnsConstructorArg)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter0 (meterState, 0);
    KMeterComponent meter1 (meterState, 1);
    KMeterComponent meterAll (meterState, -1);

    EXPECT_EQ (0, meter0.getChannel());
    EXPECT_EQ (1, meter1.getChannel());
    EXPECT_EQ (-1, meterAll.getChannel());
}

TEST (KMeterComponentTests, RefreshRateDefaultIs30)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    EXPECT_EQ (30, meter.getRefreshRate());
}

TEST (KMeterComponentTests, SetRefreshRateUpdatesValue)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    meter.setRefreshRate (60);
    EXPECT_EQ (60, meter.getRefreshRate());
}

TEST (KMeterComponentTests, KeepsItsRefreshRateWithFramesThatDoNotDivideIt)
{
    KMeterState state (48000.0, 2);
    KMeterComponent meter (state, 0);

    state.setOverThreshold (0.8f);

    // 30 updates a second are one every 33.3 ms. With frames of 25 ms the first comes too early, and the second
    // 16.7 ms late
    meter.refreshDisplay (0.025);
    meter.refreshDisplay (0.025);

    std::vector<float> samples (512, 0.9f);
    const float* channels[2] = { samples.data(), samples.data() };
    state.pushSamples (channels, 2, 512);
    state.processPendingAudio();
    ASSERT_TRUE (state.isClipping());
    ASSERT_FALSE (static_cast<bool> (meter.getProperty ("clipping")));

    // That lateness counts toward the next update, which the third frame reaches
    meter.refreshDisplay (0.025);
    EXPECT_TRUE (static_cast<bool> (meter.getProperty ("clipping")));
}

TEST (KMeterComponentTests, ItsColorIdsAreItsOwn)
{
    // Colors are looked up in the parents by these ids: a name anything could use would color the meter by accident
    for (const auto* id : { &KMeterComponent::Style::backgroundColorId,
                            &KMeterComponent::Style::greenZoneColorId,
                            &KMeterComponent::Style::amberZoneColorId,
                            &KMeterComponent::Style::redZoneColorId,
                            &KMeterComponent::Style::averageLevelColorId,
                            &KMeterComponent::Style::peakLevelColorId,
                            &KMeterComponent::Style::peakLevelClipColorId,
                            &KMeterComponent::Style::peakHoldColorId })
    {
        EXPECT_TRUE (id->toString().startsWith ("kMeter")) << id->toString();
    }

    Component parent;
    KMeterState state (48000.0, 2);
    KMeterComponent meter (state, 0);
    parent.addAndMakeVisible (meter);

    parent.setColor (Identifier ("backgroundColorId"), Colors::red);
    EXPECT_FALSE (meter.findColor (KMeterComponent::Style::backgroundColorId).has_value());
}

TEST (KMeterComponentTests, PaintLinearScaleDoesNotCrash)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    meter.setScaleMapping (KMeterComponent::ScaleMapping::linear);
    meter.setBounds (0.0f, 0.0f, 30.0f, 120.0f);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (30, 120);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({ meter.paint (g); });
}

TEST (KMeterComponentTests, PaintWithShowPeakFalseDoesNotCrash)
{
    KMeterState meterState (48000.0, 2);
    KMeterComponent meter (meterState, 0);
    meter.setShowPeak (false);
    meter.setShowPeakHold (false);
    meter.setBounds (0.0f, 0.0f, 30.0f, 120.0f);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (30, 120);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW ({ meter.paint (g); });
}

TEST (KMeterComponentTests, AMeterWithoutAStateIsConfiguredAndPainted)
{
    KMeterComponent meter;
    meter.setBounds (0.0f, 0.0f, 30.0f, 120.0f);

    EXPECT_EQ (nullptr, meter.getState());
    EXPECT_EQ (-1, meter.getChannel());
    EXPECT_EQ (KMeterState::OverCounterMode::contiguous, meter.getOverCounterMode());

    // What belongs to the state is ignored
    meter.setOverCounterMode (KMeterState::OverCounterMode::total);
    meter.setAverageIntegrationTime (0.2);
    meter.setAverageFallTime (0.9);
    meter.setPeakHoldTime (1.5);

    EXPECT_EQ (KMeterState::OverCounterMode::contiguous, meter.getOverCounterMode());

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (30, 120);
    Graphics g (*context, *renderer);

    meter.refreshDisplay (1.0);
    EXPECT_NO_THROW ({ meter.paint (g); });
}

TEST (KMeterComponentTests, SetStateMovesTheMeterToAnotherState)
{
    KMeterState first (48000.0, 2);
    KMeterState second (48000.0, 2);
    KMeterComponent meter (first, 1);

    EXPECT_EQ (&first, meter.getState());
    EXPECT_EQ (1, meter.getChannel());

    meter.setState (&second);
    meter.setOverCounterMode (KMeterState::OverCounterMode::total);

    EXPECT_EQ (&second, meter.getState());
    EXPECT_EQ (KMeterState::OverCounterMode::contiguous, first.getOverCounterMode());
    EXPECT_EQ (KMeterState::OverCounterMode::total, second.getOverCounterMode());

    meter.setChannel (0);
    EXPECT_EQ (0, meter.getChannel());

    meter.setState (nullptr);
    EXPECT_EQ (nullptr, meter.getState());
}

TEST (KMeterComponentTests, TellsItStoppedClippingWhenItsStateIsRemoved)
{
    struct Listener : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override
        {
            if (name == Identifier ("clipping"))
                ++clippingChanges;
        }

        int clippingChanges = 0;
    };

    KMeterState state (48000.0, 2);
    KMeterComponent meter (state, 0);

    Listener listener;
    meter.addComponentListener (&listener);

    state.setOverThreshold (0.8f);

    std::vector<float> samples (512, 0.9f);
    const float* channels[2] = { samples.data(), samples.data() };
    state.pushSamples (channels, 2, 512);
    state.processPendingAudio();
    ASSERT_TRUE (state.isClipping());

    meter.refreshDisplay (1.0);
    EXPECT_TRUE (static_cast<bool> (meter.getProperty ("clipping")));
    EXPECT_EQ (listener.clippingChanges, 1);

    // Without a state the meter shows silence, and what selects it by its clipping is told
    meter.setState (nullptr);
    EXPECT_FALSE (static_cast<bool> (meter.getProperty ("clipping")));
    EXPECT_EQ (listener.clippingChanges, 2);

    meter.removeComponentListener (&listener);
}

TEST (KMeterComponentTests, TellsTheLevelsThatMovedWhenItReadsThem)
{
    struct Listener : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override
        {
            changed.add (name.toString());
        }

        StringArray changed;
    };

    KMeterState state (48000.0, 2);
    KMeterComponent meter (state, 0);

    Listener listener;
    meter.addComponentListener (&listener);

    std::vector<float> samples (512, 0.25f);
    const float* channels[2] = { samples.data(), samples.data() };
    state.pushSamples (channels, 2, 512);
    state.processPendingAudio();

    meter.refreshDisplay (1.0);
    EXPECT_TRUE (listener.changed.contains ("peakDb"));
    EXPECT_TRUE (listener.changed.contains ("averageDb"));
    EXPECT_TRUE (listener.changed.contains ("peakHoldDb"));
    EXPECT_FALSE (listener.changed.contains ("rangeMinDb"));
    EXPECT_FLOAT_EQ (state.getPeakLevel (0), static_cast<float> (meter.getProperty ("peakDb")));

    // Levels that kept still are not told again
    listener.changed.clear();
    meter.refreshDisplay (1.0);
    EXPECT_TRUE (listener.changed.isEmpty());

    // The same signal again raises the average: a level is told when it is one that moved
    const auto peak = meter.getPeakLevel();
    const auto average = meter.getAverageLevel();
    const auto peakHold = meter.getPeakHoldLevel();

    state.pushSamples (channels, 2, 512);
    state.processPendingAudio();
    meter.refreshDisplay (1.0);

    ASSERT_NE (average, meter.getAverageLevel());
    EXPECT_TRUE (listener.changed.contains ("averageDb"));
    EXPECT_EQ (peak != meter.getPeakLevel(), listener.changed.contains ("peakDb"));
    EXPECT_EQ (peakHold != meter.getPeakHoldLevel(), listener.changed.contains ("peakHoldDb"));

    // The levels of another channel are told when the meter is given it
    const std::vector<float> louder (512, 0.5f);
    const float* unequalChannels[2] = { samples.data(), louder.data() };
    state.pushSamples (unequalChannels, 2, 512);
    state.processPendingAudio();
    meter.refreshDisplay (1.0);

    listener.changed.clear();
    meter.setChannel (1);
    EXPECT_TRUE (listener.changed.contains ("peakDb"));
    EXPECT_FLOAT_EQ (state.getPeakLevel (1), meter.getPeakLevel());

    meter.removeComponentListener (&listener);
}

TEST (KMeterComponentTests, TellsTheRangeOfItsScaleWhenItsStateIsGivenAnotherScale)
{
    struct Listener : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override
        {
            changed.add (name.toString());
        }

        StringArray changed;
    };

    KMeterState state (48000.0, 2);
    KMeterComponent meter (state, 0);

    Listener listener;
    meter.addComponentListener (&listener);

    state.setScale (KMeterState::Scale::k14);
    meter.refreshDisplay (1.0);

    EXPECT_TRUE (listener.changed.contains ("rangeMinDb"));
    EXPECT_TRUE (listener.changed.contains ("rangeMaxDb"));
    EXPECT_FLOAT_EQ (26.0f, static_cast<float> (meter.getProperty ("rangeMaxDb")));

    meter.removeComponentListener (&listener);
}

TEST (KMeterComponentTests, TheTypeInfoCreatesAMeterAndSetsItsChannel)
{
    const auto& type = KMeterComponent::getClassTypeInfo();
    EXPECT_EQ (type.getTypeName().toString(), "KMeter");

    const auto created = type.createInstance();
    auto* instance = dynamic_cast<KMeterComponent*> (created.get());
    ASSERT_NE (nullptr, instance);
    EXPECT_EQ (nullptr, instance->getState());
    EXPECT_EQ (&instance->getTypeInfo(), &type);
    EXPECT_EQ (instance->getProperty ("scaleMapping").toString(), "segmented");

    instance->setProperty ("channel", 1);
    instance->setProperty ("scaleMapping", "linear");
    instance->setProperty ("showPeakHold", false);
    instance->setProperty ("refreshRate", 200);

    EXPECT_EQ (1, instance->getChannel());
    EXPECT_EQ (KMeterComponent::ScaleMapping::linear, instance->getScaleMapping());
    EXPECT_FALSE (instance->getShowPeakHold());
    EXPECT_EQ (60, instance->getRefreshRate());

    EXPECT_TRUE (type.hasSignal ("onLevelChanged"));
    EXPECT_TRUE (type.hasSignal ("onClippingChanged"));
}

//==============================================================================
class KMeterComponentDisplayTests;

namespace yup
{

/** What the tests of the meter in a window do to a component that only the component does to itself. */
template <>
class ComponentTestHelper<KMeterComponentDisplayTests>
{
public:
    /** Gives a component a window the caller built, which the component then owns. */
    static void attachNative (Component& component, ComponentNative* newNative)
    {
        component.native = newNative;
    }
};

} // namespace yup

/** The meter in a window. A stub native stands in for the window, so the only frames are the ones a
    test sends, and what the meter asks to repaint is there to read.
*/
class KMeterComponentDisplayTests : public ::testing::Test
{
protected:
    class StubWindow final : public ComponentNative
    {
    public:
        explicit StubWindow (Component& component)
            : ComponentNative (component, defaultFlags)
        {
        }

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

        void setAspectRatio (std::optional<float>) override {}

        std::optional<float> getAspectRatio() const override { return std::nullopt; }

        GpuTriangulationThresholds getTriangulationThresholds() const override { return thresholds; }

        void setTriangulationThresholds (const GpuTriangulationThresholds& newThresholds) override { thresholds = newThresholds; }

        void repaint() override { repaintAreas.add (component.getBounds()); }

        void repaint (const Rectangle<float>& rect) override { repaintAreas.add (rect); }

        const RectangleList<float>& getRepaintAreas() const override { return repaintAreas; }

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

        void setGlobalMouseCaptureActive (bool) override {}

        void cancelCurrentMouseGesture() override {}

        using ComponentNative::callFrameListeners;
        using ComponentNative::hasFrameListeners;

        RectangleList<float> repaintAreas;
        GpuTriangulationThresholds thresholds;
    };

    void SetUp() override
    {
        native = new StubWindow (window);
        ComponentTestHelper<KMeterComponentDisplayTests>::attachNative (window, native);

        window.setBounds (0.0f, 0.0f, 200.0f, 300.0f);
        window.setVisible (true);

        panel.setBounds (0.0f, 0.0f, 200.0f, 300.0f);
        window.addAndMakeVisible (panel);

        meter.setBounds (meterArea);
        panel.addAndMakeVisible (meter);
    }

    /** Sends the frames of a length of time, 60 a second, as the window of the meter does. */
    void sendFrames (double seconds)
    {
        for (int frame = 0; frame < roundToInt (seconds * 60.0); ++frame)
            native->callFrameListeners (1.0 / 60.0);
    }

    /** Puts a block of samples of one value through the state, as the audio thread does. */
    void feed (float value)
    {
        const std::vector<float> samples (512, value);
        const float* channels[2] = { samples.data(), samples.data() };

        state.pushSamples (channels, 2, 512);
        state.processPendingAudio();
    }

    /** Lets the timers of the message thread run until the meter follows its levels again. */
    void waitForFrameListener()
    {
#if YUP_MODAL_LOOPS_PERMITTED
        for (int waited = 0; waited < 2000 && ! native->hasFrameListeners(); waited += 20)
            MessageManager::getInstance()->runDispatchLoopUntil (20);
#endif
    }

    String labelsOfTicks() const
    {
        StringArray labels;

        for (const auto& tick : meter.getScaleTicks())
        {
            if (tick.label.isNotEmpty())
                labels.add (tick.label);
        }

        return labels.joinIntoString (" ");
    }

    // 208 is two units for each of the 104 a K-20 scale is divided into
    const Rectangle<float> meterArea { 10.0f, 10.0f, 40.0f, 208.0f };

    Component window { "window" };
    Component panel { "panel" };
    KMeterState state { 48000.0, 2 };
    KMeterComponent meter { state, 0 };
    StubWindow* native = nullptr;
};

TEST_F (KMeterComponentDisplayTests, AsksForARepaintOnlyWhenWhatItShowsMoved)
{
    native->repaintAreas.clearQuick();
    feed (0.25f);
    sendFrames (0.1);
    ASSERT_FALSE (native->repaintAreas.isEmpty());

    // The same levels again move nothing
    native->repaintAreas.clearQuick();
    sendFrames (0.1);
    EXPECT_TRUE (native->repaintAreas.isEmpty());

    // Nor does a level that moved where it does not show: the average is still under the scale
    const auto averageLevel = meter.getAverageLevel();
    feed (0.25f);
    sendFrames (0.1);
    ASSERT_NE (averageLevel, meter.getAverageLevel());
    ASSERT_LT (meter.getAverageLevel(), KMeterState::rangeMinForScale (KMeterState::Scale::k20));
    EXPECT_TRUE (native->repaintAreas.isEmpty());

    // A peak that rose shows
    feed (0.5f);
    sendFrames (0.1);
    EXPECT_FALSE (native->repaintAreas.isEmpty());

    // And another scale moves everything
    native->repaintAreas.clearQuick();
    state.setScale (KMeterState::Scale::k14);
    sendFrames (0.1);
    EXPECT_FALSE (native->repaintAreas.isEmpty());
}

TEST_F (KMeterComponentDisplayTests, AMeterPaintedByAStyleOfItsOwnAsksForARepaintAtAnyChange)
{
    // A style puts the levels where it likes, which the meter cannot tell
    meter.setStyle (ComponentStyle::createStyle<KMeterComponent> ([] (Graphics&, const ApplicationTheme&, const KMeterComponent&) {}));

    feed (0.25f);
    sendFrames (0.1);

    native->repaintAreas.clearQuick();
    sendFrames (0.1);
    EXPECT_TRUE (native->repaintAreas.isEmpty());

    feed (0.25f);
    sendFrames (0.1);
    EXPECT_FALSE (native->repaintAreas.isEmpty());
}

TEST_F (KMeterComponentDisplayTests, AMeterAtRestLeavesItsWindowIdleAndWakesWhenLevelsArrive)
{
    feed (0.5f);
    sendFrames (0.1);
    EXPECT_TRUE (native->hasFrameListeners());

    // Levels that keep still are no longer looked at for every frame
    sendFrames (3.0);
    EXPECT_FALSE (native->hasFrameListeners());

    native->repaintAreas.clearQuick();
    feed (0.9f);
    waitForFrameListener();

#if YUP_MODAL_LOOPS_PERMITTED
    EXPECT_TRUE (native->hasFrameListeners());
    EXPECT_FALSE (native->repaintAreas.isEmpty());
    EXPECT_FLOAT_EQ (state.getPeakLevel (0), meter.getPeakLevel());
#endif
}

TEST_F (KMeterComponentDisplayTests, AHiddenMeterDoesNoWorkAndShowsTheLevelsOfNowWhenItIsBack)
{
    EXPECT_TRUE (native->hasFrameListeners());

    meter.setVisible (false);
    EXPECT_FALSE (native->hasFrameListeners());

    meter.setVisible (true);
    EXPECT_TRUE (native->hasFrameListeners());

    // Hidden through its parent, the meter is still visible itself
    panel.setVisible (false);
    EXPECT_TRUE (meter.isVisible());
    EXPECT_FALSE (native->hasFrameListeners());

    feed (0.5f);
    panel.setVisible (true);

    EXPECT_TRUE (native->hasFrameListeners());
    EXPECT_FLOAT_EQ (state.getPeakLevel (0), meter.getPeakLevel());
}

TEST_F (KMeterComponentDisplayTests, AMeterWithoutAStateFollowsNothing)
{
    meter.setState (nullptr);
    EXPECT_FALSE (native->hasFrameListeners());

    meter.setState (&state);
    EXPECT_TRUE (native->hasFrameListeners());
}

TEST_F (KMeterComponentDisplayTests, AMeterThatWasNotToldItShowsFollowsItsLevelsOnceItIsPainted)
{
    /** Handles its showing without the meter, as a subclass written before the meter did. */
    struct OwnShowingMeter : KMeterComponent
    {
        using KMeterComponent::KMeterComponent;

        void visibilityChanged() override {}

        void parentHierarchyChanged() override {}
    };

    meter.setVisible (false);

    OwnShowingMeter own (state, 0);
    own.setBounds (meterArea);
    panel.addAndMakeVisible (own);
    ASSERT_FALSE (native->hasFrameListeners());

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (200, 300);
    Graphics g (*context, *renderer);

    own.paint (g);
    EXPECT_TRUE (native->hasFrameListeners());
}

TEST_F (KMeterComponentDisplayTests, LaysOutTheSegmentsOfTheKSystem)
{
    // K-20 has 4 bars of half a dB, 42 of 1 dB twice as tall, one of 6 dB and 4 of 10 dB, the last one taller
    const auto segments = meter.getScaleSegments();
    ASSERT_EQ (51u, segments.size());

    EXPECT_FLOAT_EQ (20.0f, segments[0].topDb);
    EXPECT_FLOAT_EQ (19.5f, segments[0].bottomDb);
    EXPECT_FLOAT_EQ (0.0f, segments[0].topY);
    EXPECT_FLOAT_EQ (2.0f, segments[0].bottomY);

    EXPECT_FLOAT_EQ (18.0f, segments[4].topDb);
    EXPECT_FLOAT_EQ (17.0f, segments[4].bottomDb);
    EXPECT_FLOAT_EQ (8.0f, segments[4].topY);
    EXPECT_FLOAT_EQ (12.0f, segments[4].bottomY);

    EXPECT_FLOAT_EQ (-24.0f, segments[46].topDb);
    EXPECT_FLOAT_EQ (-30.0f, segments[46].bottomDb);
    EXPECT_FLOAT_EQ (176.0f, segments[46].topY);
    EXPECT_FLOAT_EQ (182.0f, segments[46].bottomY);

    EXPECT_FLOAT_EQ (-60.0f, segments[50].topDb);
    EXPECT_FLOAT_EQ (-70.0f, segments[50].bottomDb);
    EXPECT_FLOAT_EQ (200.0f, segments[50].topY);
    EXPECT_FLOAT_EQ (208.0f, segments[50].bottomY);

    EXPECT_FLOAT_EQ (0.0f, meter.getPositionForLevel (20.0f));
    EXPECT_FLOAT_EQ (64.0f, meter.getPositionForLevel (4.0f));
    EXPECT_FLOAT_EQ (80.0f, meter.getPositionForLevel (0.0f));
    EXPECT_FLOAT_EQ (179.0f, meter.getPositionForLevel (-27.0f));
    EXPECT_FLOAT_EQ (208.0f, meter.getPositionForLevel (-70.0f));

    // A level outside the scale is at its nearest end
    EXPECT_FLOAT_EQ (0.0f, meter.getPositionForLevel (40.0f));
    EXPECT_FLOAT_EQ (208.0f, meter.getPositionForLevel (-100.0f));

    // K-12 has 36 bars of half a dB, 40 of 1 dB and 4 below, the last one 2 dB only and the tallest
    state.setScale (KMeterState::Scale::k12);
    meter.setBounds (10.0f, 10.0f, 40.0f, 130.0f);

    const auto k12Segments = meter.getScaleSegments();
    ASSERT_EQ (80u, k12Segments.size());

    EXPECT_FLOAT_EQ (-60.0f, k12Segments[79].topDb);
    EXPECT_FLOAT_EQ (-62.0f, k12Segments[79].bottomDb);
    EXPECT_FLOAT_EQ (125.0f, k12Segments[79].topY);
    EXPECT_FLOAT_EQ (130.0f, k12Segments[79].bottomY);

    EXPECT_FLOAT_EQ (36.0f, meter.getPositionForLevel (10.0f));
    EXPECT_FLOAT_EQ (56.0f, meter.getPositionForLevel (0.0f));
    EXPECT_FLOAT_EQ (116.0f, meter.getPositionForLevel (-30.0f));

    // A linear scale is one segment
    state.setScale (KMeterState::Scale::k14);
    meter.setScaleMapping (KMeterComponent::ScaleMapping::linear);
    meter.setBounds (10.0f, 10.0f, 40.0f, 180.0f);

    const auto linearSegments = meter.getScaleSegments();
    ASSERT_EQ (1u, linearSegments.size());

    EXPECT_FLOAT_EQ (26.0f, linearSegments[0].topDb);
    EXPECT_FLOAT_EQ (-64.0f, linearSegments[0].bottomDb);
    EXPECT_FLOAT_EQ (0.0f, linearSegments[0].topY);
    EXPECT_FLOAT_EQ (180.0f, linearSegments[0].bottomY);

    EXPECT_NEAR (52.0f, meter.getPositionForLevel (0.0f), 0.001f);
    EXPECT_NEAR (32.0f, meter.getPositionForLevel (10.0f), 0.001f);
}

TEST_F (KMeterComponentDisplayTests, LaysOutTheTicksAndTheLabelsThatFit)
{
    // A tick every 5 dB, and a label every 10 dB when all of it is inside the meter and off the labels kept
    const auto ticks = meter.getScaleTicks();
    ASSERT_EQ (19u, ticks.size());

    EXPECT_EQ (20, ticks[0].db);
    EXPECT_FLOAT_EQ (0.0f, ticks[0].y);
    EXPECT_TRUE (ticks[0].label.isEmpty());

    EXPECT_EQ (0, ticks[4].db);
    EXPECT_FLOAT_EQ (80.0f, ticks[4].y);
    EXPECT_EQ (ticks[4].label, "0");
    EXPECT_EQ (ticks[4].labelBounds, Rectangle<float> (5.0f, 74.0f, 22.0f, 12.0f));

    EXPECT_EQ (-70, ticks[18].db);
    EXPECT_FLOAT_EQ (208.0f, ticks[18].y);

    EXPECT_EQ (labelsOfTicks(), "+10 0 -10 -20 -30 -60");

    // A meter under 200 high has a tick every 10 dB
    state.setScale (KMeterState::Scale::k12);
    meter.setBounds (10.0f, 10.0f, 40.0f, 130.0f);

    EXPECT_EQ (9u, meter.getScaleTicks().size());
    EXPECT_EQ (labelsOfTicks(), "+20 +10 0 -10 -20 -50");

    state.setScale (KMeterState::Scale::k14);
    meter.setScaleMapping (KMeterComponent::ScaleMapping::linear);
    meter.setBounds (10.0f, 10.0f, 40.0f, 180.0f);

    const auto linearTicks = meter.getScaleTicks();
    ASSERT_EQ (9u, linearTicks.size());
    EXPECT_NEAR (12.0f, linearTicks[0].y, 0.001f);
    EXPECT_NEAR (172.0f, linearTicks[8].y, 0.001f);
    EXPECT_EQ (labelsOfTicks(), "+20 +10 0 -10 -20 -30 -40 -50 -60");

    // A meter with no area has no scale to lay out
    meter.setBounds (10.0f, 10.0f, 40.0f, 0.0f);
    EXPECT_TRUE (meter.getScaleTicks().empty());
    EXPECT_TRUE (meter.getScaleSegments().empty());
}
