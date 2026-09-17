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

#include <gtest/gtest.h>

#include <yup_gui/yup_gui.h>

#include "../../modules/yup_gui/native/yup_FramePacer.h"

using namespace yup;

namespace
{

// ==============================================================================
// Minimal concrete ComponentNative for testing the constructor.
// ==============================================================================

class StubComponentNative final : public ComponentNative
{
public:
    StubComponentNative (Component& comp, const Flags& f)
        : ComponentNative (comp, f)
    {
    }

    void setTitle (const String&) override {}

    String getTitle() const override { return {}; }

    void setVisible (bool) override {}

    bool isVisible() const override { return false; }

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

    bool isContinuousRepaintingEnabled() const override { return false; }

    void enableContinuousRepainting (bool) override {}

    bool isAtomicModeEnabled() const override { return false; }

    void enableAtomicMode (bool) override {}

    bool isWireframeEnabled() const override { return false; }

    void enableWireframe (bool) override {}

    void repaint() override
    {
        repaintAreas.clearQuick();
        repaintAreas.add (component.getBounds());
    }

    void repaint (const Rectangle<float>& rect) override
    {
        repaintAreas.add (rect);
    }

    const RectangleList<float>& getRepaintAreas() const override
    {
        return repaintAreas;
    }

    void startTextInput (Component&) override {}

    void stopTextInput (Component&) override {}

    void updateTextInputRect (Component&) override {}

    float getScaleDpi() const override { return 1.0f; }

    float getCurrentFrameRate() const override { return 60.0f; }

    float getDesiredFrameRate() const override { return 60.0f; }

    void* getNativeHandle() const override { return nullptr; }

    rive::Factory* getFactory() override { return nullptr; }

    GraphicsContext* getGraphicsContext() override { return nullptr; }

    void setGlobalMouseCaptureActive (bool) override {}

    void cancelCurrentMouseGesture() override {}

    Component& getComponent() const { return component; }

    Flags getFlags() const { return flags; }

    RectangleList<float> repaintAreas;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StubComponentNative)
};

} // namespace

// ==============================================================================
// ComponentNative::Options — builder pattern tests
// ==============================================================================

class ComponentNativeOptionsTests : public ::testing::Test
{
protected:
    ComponentNative::Options opts;
};

TEST_F (ComponentNativeOptionsTests, DefaultOptionsHaveDefaultFlags)
{
    EXPECT_EQ (opts.flags, ComponentNative::defaultFlags);
    EXPECT_EQ (opts.graphicsApi, std::nullopt);
    EXPECT_EQ (opts.framerateRedraw, std::nullopt);
    EXPECT_EQ (opts.framePacingMode, ComponentNative::FramePacingMode::automatic);
    EXPECT_EQ (opts.maximumFramesInFlight, std::nullopt);
    EXPECT_EQ (opts.clearColor, std::nullopt);
    EXPECT_EQ (opts.doubleClickTime, std::nullopt);
    EXPECT_FALSE (opts.updateOnlyWhenFocused);
}

TEST_F (ComponentNativeOptionsTests, WithFlagsOverridesAllFlags)
{
    auto& result = opts.withFlags (ComponentNative::noFlags);
    EXPECT_EQ (&result, &opts);
    EXPECT_EQ (opts.flags, ComponentNative::noFlags);
}

TEST_F (ComponentNativeOptionsTests, WithDecorationTrueEnablesFlag)
{
    opts.withDecoration (true);
    EXPECT_TRUE (opts.flags.test (ComponentNative::decoratedWindow));
}

TEST_F (ComponentNativeOptionsTests, WithDecorationFalseDisablesFlag)
{
    opts.withDecoration (true);
    opts.withDecoration (false);
    EXPECT_FALSE (opts.flags.test (ComponentNative::decoratedWindow));
}

TEST_F (ComponentNativeOptionsTests, WithResizableWindowTrueEnablesFlag)
{
    opts.withResizableWindow (true);
    EXPECT_TRUE (opts.flags.test (ComponentNative::resizableWindow));
}

TEST_F (ComponentNativeOptionsTests, WithResizableWindowFalseDisablesFlag)
{
    opts.withResizableWindow (true);
    opts.withResizableWindow (false);
    EXPECT_FALSE (opts.flags.test (ComponentNative::resizableWindow));
}

TEST_F (ComponentNativeOptionsTests, WithRenderContinuousTrueEnablesFlag)
{
    opts.withRenderContinuous (true);
    EXPECT_TRUE (opts.flags.test (ComponentNative::renderContinuous));
}

TEST_F (ComponentNativeOptionsTests, WithRenderContinuousFalseDisablesFlag)
{
    opts.withRenderContinuous (true);
    opts.withRenderContinuous (false);
    EXPECT_FALSE (opts.flags.test (ComponentNative::renderContinuous));
}

TEST_F (ComponentNativeOptionsTests, DefaultOptionsHaveVSyncEnabled)
{
    EXPECT_TRUE (opts.flags.test (ComponentNative::vsync));
}

TEST_F (ComponentNativeOptionsTests, WithVSyncTrueEnablesFlag)
{
    auto& result = opts.withVSync (true);
    EXPECT_EQ (&result, &opts);
    EXPECT_TRUE (opts.flags.test (ComponentNative::vsync));
}

TEST_F (ComponentNativeOptionsTests, WithVSyncFalseDisablesFlag)
{
    opts.withVSync (true);
    opts.withVSync (false);
    EXPECT_FALSE (opts.flags.test (ComponentNative::vsync));
}

TEST_F (ComponentNativeOptionsTests, WithAllowedHighDensityDisplayTrueEnablesFlag)
{
    opts.withAllowedHighDensityDisplay (true);
    EXPECT_TRUE (opts.flags.test (ComponentNative::allowHighDensityDisplay));
}

TEST_F (ComponentNativeOptionsTests, WithAllowedHighDensityDisplayFalseDisablesFlag)
{
    opts.withAllowedHighDensityDisplay (true);
    opts.withAllowedHighDensityDisplay (false);
    EXPECT_FALSE (opts.flags.test (ComponentNative::allowHighDensityDisplay));
}

TEST_F (ComponentNativeOptionsTests, WithMouseCaptureTrueEnablesFlag)
{
    opts.withMouseCapture (true);
    EXPECT_TRUE (opts.flags.test (ComponentNative::captureMouse));
}

TEST_F (ComponentNativeOptionsTests, WithMouseCaptureFalseDisablesFlag)
{
    opts.withMouseCapture (true);
    opts.withMouseCapture (false);
    EXPECT_FALSE (opts.flags.test (ComponentNative::captureMouse));
}

TEST_F (ComponentNativeOptionsTests, WithTemporaryWindowTrueEnablesFlag)
{
    opts.withTemporaryWindow (true);
    EXPECT_TRUE (opts.flags.test (ComponentNative::temporaryWindow));
}

TEST_F (ComponentNativeOptionsTests, WithTemporaryWindowFalseDisablesFlag)
{
    opts.withTemporaryWindow (true);
    opts.withTemporaryWindow (false);
    EXPECT_FALSE (opts.flags.test (ComponentNative::temporaryWindow));
}

TEST_F (ComponentNativeOptionsTests, WithGraphicsApiSetsValue)
{
    opts.withGraphicsApi (GpuPlatform::Metal);
    ASSERT_TRUE (opts.graphicsApi.has_value());
    EXPECT_EQ (*opts.graphicsApi, GpuPlatform::Metal);
}

TEST_F (ComponentNativeOptionsTests, WithGraphicsApiNulloptClearsValue)
{
    opts.withGraphicsApi (GpuPlatform::Metal);
    opts.withGraphicsApi (std::nullopt);
    EXPECT_FALSE (opts.graphicsApi.has_value());
}

TEST_F (ComponentNativeOptionsTests, WithFramerateRedrawSetsValue)
{
    opts.withFramerateRedraw (30.0f);
    ASSERT_TRUE (opts.framerateRedraw.has_value());
    EXPECT_FLOAT_EQ (*opts.framerateRedraw, 30.0f);
}

TEST_F (ComponentNativeOptionsTests, WithFramerateRedrawNulloptClearsValue)
{
    opts.withFramerateRedraw (30.0f);
    opts.withFramerateRedraw (std::nullopt);
    EXPECT_FALSE (opts.framerateRedraw.has_value());
}

TEST_F (ComponentNativeOptionsTests, WithFramePacingModeSetsValue)
{
    opts.withFramePacingMode (ComponentNative::FramePacingMode::presentationDriven);
    EXPECT_EQ (opts.framePacingMode, ComponentNative::FramePacingMode::presentationDriven);
}

TEST_F (ComponentNativeOptionsTests, WithMaximumFramesInFlightSetsValue)
{
    opts.withMaximumFramesInFlight (3);
    ASSERT_TRUE (opts.maximumFramesInFlight.has_value());
    EXPECT_EQ (*opts.maximumFramesInFlight, 3u);
}

TEST_F (ComponentNativeOptionsTests, WithMaximumFramesInFlightNulloptClearsValue)
{
    opts.withMaximumFramesInFlight (2);
    opts.withMaximumFramesInFlight (std::nullopt);
    EXPECT_FALSE (opts.maximumFramesInFlight.has_value());
}

TEST_F (ComponentNativeOptionsTests, WithClearColorSetsValue)
{
    const Color col (0xff112233);
    opts.withClearColor (col);
    ASSERT_TRUE (opts.clearColor.has_value());
    EXPECT_EQ (*opts.clearColor, col);
}

TEST_F (ComponentNativeOptionsTests, WithClearColorNulloptClearsValue)
{
    opts.withClearColor (Color (0xff112233));
    opts.withClearColor (std::nullopt);
    EXPECT_FALSE (opts.clearColor.has_value());
}

TEST_F (ComponentNativeOptionsTests, WithDoubleClickTimeSetsValue)
{
    const auto t = RelativeTime::milliseconds (400);
    opts.withDoubleClickTime (t);
    ASSERT_TRUE (opts.doubleClickTime.has_value());
    EXPECT_EQ (*opts.doubleClickTime, t);
}

TEST_F (ComponentNativeOptionsTests, WithDoubleClickTimeNulloptClearsValue)
{
    opts.withDoubleClickTime (RelativeTime::milliseconds (400));
    opts.withDoubleClickTime (std::nullopt);
    EXPECT_FALSE (opts.doubleClickTime.has_value());
}

TEST_F (ComponentNativeOptionsTests, WithUpdateOnlyFocusedTrue)
{
    auto& result = opts.withUpdateOnlyFocused (true);
    EXPECT_EQ (&result, &opts);
    EXPECT_TRUE (opts.updateOnlyWhenFocused);
}

TEST_F (ComponentNativeOptionsTests, WithUpdateOnlyFocusedFalse)
{
    opts.withUpdateOnlyFocused (true);
    opts.withUpdateOnlyFocused (false);
    EXPECT_FALSE (opts.updateOnlyWhenFocused);
}

TEST_F (ComponentNativeOptionsTests, ChainedOptionsAllApply)
{
    opts.withFlags (ComponentNative::noFlags)
        .withDecoration (true)
        .withResizableWindow (false)
        .withRenderContinuous (true)
        .withAllowedHighDensityDisplay (true)
        .withMouseCapture (true)
        .withVSync (true)
        .withTemporaryWindow (true)
        .withGraphicsApi (GpuPlatform::Headless)
        .withFramerateRedraw (60.0f)
        .withFramePacingMode (ComponentNative::FramePacingMode::presentationDriven)
        .withMaximumFramesInFlight (2)
        .withClearColor (Color (0xff000000))
        .withDoubleClickTime (RelativeTime::milliseconds (500))
        .withUpdateOnlyFocused (true);

    EXPECT_TRUE (opts.flags.test (ComponentNative::decoratedWindow));
    EXPECT_FALSE (opts.flags.test (ComponentNative::resizableWindow));
    EXPECT_TRUE (opts.flags.test (ComponentNative::renderContinuous));
    EXPECT_TRUE (opts.flags.test (ComponentNative::allowHighDensityDisplay));
    EXPECT_TRUE (opts.flags.test (ComponentNative::captureMouse));
    EXPECT_TRUE (opts.flags.test (ComponentNative::vsync));
    EXPECT_TRUE (opts.flags.test (ComponentNative::temporaryWindow));
    ASSERT_TRUE (opts.graphicsApi.has_value());
    EXPECT_EQ (*opts.graphicsApi, GpuPlatform::Headless);
    ASSERT_TRUE (opts.framerateRedraw.has_value());
    EXPECT_FLOAT_EQ (*opts.framerateRedraw, 60.0f);
    EXPECT_EQ (opts.framePacingMode, ComponentNative::FramePacingMode::presentationDriven);
    ASSERT_TRUE (opts.maximumFramesInFlight.has_value());
    EXPECT_EQ (*opts.maximumFramesInFlight, 2u);
    ASSERT_TRUE (opts.clearColor.has_value());
    EXPECT_TRUE (opts.updateOnlyWhenFocused);
}

class FramePacerTests : public ::testing::Test
{
protected:
    using EffectiveMode = detail::FramePacer::EffectiveMode;

    static detail::FramePacer makePacer (ComponentNative::FramePacingMode mode,
                                         bool vsyncEnabled,
                                         GraphicsContext::FrameTimingCapabilities capabilities = {},
                                         std::optional<uint32_t> maximumFramesInFlight = std::nullopt,
                                         double targetFrameRate = 60.0)
    {
        return detail::FramePacer ({ mode, maximumFramesInFlight, targetFrameRate, vsyncEnabled }, capabilities);
    }
};

TEST_F (FramePacerTests, AutomaticModeFallsBackToSoftwareWithoutPresentationTiming)
{
    auto pacer = makePacer (ComponentNative::FramePacingMode::automatic, false);
    pacer.reset (1.0);

    EXPECT_EQ (pacer.planWait (1.0).effectiveMode, EffectiveMode::software);
}

TEST_F (FramePacerTests, AutomaticModeDefersToVSyncWithoutPresentationTiming)
{
    auto pacer = makePacer (ComponentNative::FramePacingMode::automatic, true);
    pacer.reset (1.0);

    EXPECT_EQ (pacer.planWait (1.0).effectiveMode, EffectiveMode::off);
}

TEST_F (FramePacerTests, SoftwareModeIsSuppressedWhenVSyncIsEnabled)
{
    auto pacer = makePacer (ComponentNative::FramePacingMode::software, true);
    pacer.reset (1.0);

    EXPECT_EQ (pacer.planWait (1.0).effectiveMode, EffectiveMode::off);
}

TEST_F (FramePacerTests, AutomaticModeUsesPresentationDrivenWhenTimingIsAvailable)
{
    GraphicsContext::FrameTimingCapabilities capabilities;
    capabilities.hasPresentationTiming = true;

    auto pacer = makePacer (ComponentNative::FramePacingMode::automatic, true, capabilities);
    pacer.reset (1.0);

    EXPECT_EQ (pacer.planWait (1.0).effectiveMode, EffectiveMode::presentationDriven);
}

TEST_F (FramePacerTests, AutomaticModeKeepsSoftwarePacingWhenOnlyTimingSupportExistsWithoutVSync)
{
    GraphicsContext::FrameTimingCapabilities capabilities;
    capabilities.hasPresentationTiming = true;

    auto pacer = makePacer (ComponentNative::FramePacingMode::automatic, false, capabilities);
    pacer.reset (1.0);

    EXPECT_EQ (pacer.planWait (1.0).effectiveMode, EffectiveMode::software);
}

TEST_F (FramePacerTests, MaximumFramesInFlightSupportAloneDoesNotEnablePresentationDrivenMode)
{
    GraphicsContext::FrameTimingCapabilities capabilities;
    capabilities.hasMaximumFramesInFlight = true;

    auto pacer = makePacer (ComponentNative::FramePacingMode::presentationDriven, false, capabilities);
    pacer.reset (1.0);

    EXPECT_EQ (pacer.planWait (1.0).effectiveMode, EffectiveMode::software);
}

TEST_F (FramePacerTests, PresentationDerivedDeltaIgnoresVariableCpuCadence)
{
    GraphicsContext::FrameTimingCapabilities capabilities;
    capabilities.hasPresentationTiming = true;

    auto pacer = makePacer (ComponentNative::FramePacingMode::presentationDriven, true, capabilities);
    pacer.reset (1.0);

    GraphicsContext::FrameTimingInfo firstSample;
    firstSample.hasPresentationTimestamp = true;
    firstSample.presentedAtSeconds = 1.0 + (1.0 / 60.0);
    firstSample.presentationCount = 1;

    auto firstDelta = pacer.makeRefreshDelta (1.02, firstSample);
    EXPECT_TRUE (firstDelta.usedPresentationTiming);
    EXPECT_NEAR (1.0 / 60.0, firstDelta.seconds, 1.0e-6);

    GraphicsContext::FrameTimingInfo secondSample = firstSample;
    secondSample.presentedAtSeconds = 1.0 + (2.0 / 60.0);
    secondSample.presentationCount = 2;

    auto secondDelta = pacer.makeRefreshDelta (1.05, secondSample);
    EXPECT_TRUE (secondDelta.usedPresentationTiming);
    EXPECT_NEAR (1.0 / 60.0, secondDelta.seconds, 1.0e-6);
}

TEST_F (FramePacerTests, AdaptiveLeadTimeTracksMoreExpensiveFrames)
{
    auto pacer = makePacer (ComponentNative::FramePacingMode::software, false);
    pacer.reset (0.0);

    pacer.recordFrame (0.010, 0.002, 0.001, 0.0);
    const auto initialLeadTime = pacer.planWait (0.010).leadTimeSeconds;

    pacer.recordFrame (0.040, 0.010, 0.005, 0.0);
    const auto adaptedLeadTime = pacer.planWait (0.040).leadTimeSeconds;

    EXPECT_GT (adaptedLeadTime, initialLeadTime);
}

TEST_F (FramePacerTests, MissedDeadlinesAdvanceToTheNextFutureSlot)
{
    auto pacer = makePacer (ComponentNative::FramePacingMode::software, false);
    pacer.reset (0.0);

    pacer.recordFrame (0.090, 0.010, 0.002, 0.0);

    const auto waitPlan = pacer.planWait (0.090);
    EXPECT_GT (pacer.getDiagnostics().missedDeadlines, 0u);
    EXPECT_LT (pacer.getDiagnostics().schedulerLatenessSeconds, waitPlan.targetIntervalSeconds);
}

TEST_F (FramePacerTests, SoftwareModeSupportsNonIntegralTargetIntervals)
{
    auto pacer = makePacer (ComponentNative::FramePacingMode::software, false, {}, std::nullopt, 40.0);
    pacer.reset (0.0);

    const auto firstPlan = pacer.planWait (0.0);
    EXPECT_NEAR (1.0 / 40.0, firstPlan.targetIntervalSeconds, 1.0e-9);

    pacer.recordFrame (1.0 / 60.0, 0.003, 0.001, 0.0);
    const auto secondPlan = pacer.planWait (1.0 / 60.0);

    EXPECT_NEAR (1.0 / 40.0, secondPlan.targetIntervalSeconds, 1.0e-9);
    ASSERT_TRUE (secondPlan.softwareWakeTimeSeconds.has_value());
    EXPECT_GT (*secondPlan.softwareWakeTimeSeconds, 1.0 / 60.0);
}

TEST_F (FramePacerTests, InvalidPresentationFeedbackFallsBackToMonotonicTime)
{
    auto pacer = makePacer (ComponentNative::FramePacingMode::presentationDriven, false);
    pacer.reset (2.0);

    const auto delta = pacer.makeRefreshDelta (2.6, {});

    EXPECT_FALSE (delta.usedPresentationTiming);
    EXPECT_DOUBLE_EQ (0.25, delta.seconds);
}

TEST_F (FramePacerTests, ResetClearsStalePresentationHistory)
{
    GraphicsContext::FrameTimingCapabilities capabilities;
    capabilities.hasPresentationTiming = true;

    auto pacer = makePacer (ComponentNative::FramePacingMode::presentationDriven, true, capabilities);
    pacer.reset (1.0);

    GraphicsContext::FrameTimingInfo sample;
    sample.hasPresentationTimestamp = true;
    sample.presentedAtSeconds = 1.0 + (1.0 / 60.0);
    sample.presentationCount = 1;

    EXPECT_TRUE (pacer.makeRefreshDelta (1.02, sample).usedPresentationTiming);

    pacer.reset (5.0);

    const auto delta = pacer.makeRefreshDelta (5.0, {});
    EXPECT_FALSE (delta.usedPresentationTiming);
    EXPECT_DOUBLE_EQ (0.0, delta.seconds);
}

TEST_F (FramePacerTests, MaximumFramesInFlightConfigurationIsRetained)
{
    auto pacer = makePacer (ComponentNative::FramePacingMode::automatic, false, {}, 3);
    ASSERT_TRUE (pacer.getMaximumFramesInFlight().has_value());
    EXPECT_EQ (*pacer.getMaximumFramesInFlight(), 3u);
}

// ==============================================================================
// ComponentNative — construction / destruction
// ==============================================================================

class ComponentNativeConstructionTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        comp.setBounds (0, 0, 100, 100);
    }

    Component comp;
};

TEST_F (ComponentNativeConstructionTests, ConstructWithDefaultFlags)
{
    StubComponentNative native (comp, ComponentNative::defaultFlags);
    EXPECT_EQ (&native.getComponent(), &comp);
    EXPECT_EQ (native.getFlags(), ComponentNative::defaultFlags);
}

TEST_F (ComponentNativeConstructionTests, ConstructWithNoFlags)
{
    StubComponentNative native (comp, ComponentNative::noFlags);
    EXPECT_EQ (&native.getComponent(), &comp);
    EXPECT_EQ (native.getFlags(), ComponentNative::noFlags);
}

TEST_F (ComponentNativeConstructionTests, ConstructWithCustomFlags)
{
    auto flags = ComponentNative::decoratedWindow | ComponentNative::renderContinuous;
    StubComponentNative native (comp, flags);
    EXPECT_EQ (&native.getComponent(), &comp);
    EXPECT_EQ (native.getFlags(), flags);
}

TEST_F (ComponentNativeConstructionTests, DestructorDoesNotCrash)
{
    {
        StubComponentNative native (comp, ComponentNative::defaultFlags);
        EXPECT_NO_THROW ({ /* destructor called here */ });
    }
    SUCCEED();
}

// ==============================================================================
// ComponentNative — repaint-area contract
//
// The render thread consumes the repaint areas produced by repaint()/repaint(rect)
// on the message thread, so the accumulation/clear semantics must be predictable.
// ==============================================================================

class ComponentNativeRepaintTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        comp.setBounds (0, 0, 100, 100);
    }

    Component comp;
};

TEST_F (ComponentNativeRepaintTests, RectRepaintAccumulatesIntoRepaintAreas)
{
    StubComponentNative native (comp, ComponentNative::defaultFlags);

    const Rectangle<float> area (10.0f, 20.0f, 30.0f, 40.0f);
    native.repaint (area);

    EXPECT_EQ (1, native.getRepaintAreas().getNumRectangles());
    EXPECT_TRUE (native.getRepaintAreas().contains (area));
}

TEST_F (ComponentNativeRepaintTests, FullRepaintResetsPendingAreas)
{
    StubComponentNative native (comp, ComponentNative::defaultFlags);

    native.repaint (Rectangle<float> (1.0f, 2.0f, 3.0f, 4.0f));
    native.repaint (Rectangle<float> (5.0f, 6.0f, 7.0f, 8.0f));
    EXPECT_EQ (2, native.getRepaintAreas().getNumRectangles());

    // A full repaint replaces the accumulated region with the component bounds.
    native.repaint();

    EXPECT_EQ (1, native.getRepaintAreas().getNumRectangles());
    EXPECT_TRUE (native.getRepaintAreas().contains (comp.getBounds()));
}

TEST_F (ComponentNativeRepaintTests, MovingAComponentMarksTheOldAreaAndTheNewOne)
{
    auto* native = new StubComponentNative (comp, ComponentNative::defaultFlags);

    yup::ComponentTestHelper<Component>::attachNative (comp, native);

    comp.setVisible (true);

    const auto oldArea = comp.getBoundsRelativeToTopLevelComponent();

    comp.setBounds (160.0f, 120.0f, 30.0f, 40.0f);

    const auto newArea = comp.getBoundsRelativeToTopLevelComponent();

    auto areas = native->getRepaintAreas();
    EXPECT_TRUE (areas.contains (oldArea));
    EXPECT_TRUE (areas.contains (newArea));
}

// ==============================================================================
// ComponentNative::Options — flags that clear a bit rather than set it
// ==============================================================================

TEST_F (ComponentNativeOptionsTests, WithTransparentFalseClearsTheFlag)
{
    opts.withTransparent (true);
    opts.withTransparent (false);
    EXPECT_FALSE (opts.flags.test (ComponentNative::transparentWindow));
}

TEST_F (ComponentNativeOptionsTests, WithFocusableRestoresFocusability)
{
    opts.withFocusable (false);
    EXPECT_TRUE (opts.flags.test (ComponentNative::nonFocusableWindow));

    opts.withFocusable (true);
    EXPECT_FALSE (opts.flags.test (ComponentNative::nonFocusableWindow));
}

TEST_F (ComponentNativeOptionsTests, WithAlwaysOnTopFalseClearsTheFlag)
{
    opts.withAlwaysOnTop (true);
    opts.withAlwaysOnTop (false);
    EXPECT_FALSE (opts.flags.test (ComponentNative::alwaysOnTopWindow));
}

// ==============================================================================
// ComponentNative — base-class contract
// ==============================================================================

TEST_F (ComponentNativeConstructionTests, BaseAccessorsReturnTheOwnedComponent)
{
    auto* native = new StubComponentNative (comp, ComponentNative::defaultFlags);

    // The stub declares its own getComponent(), which hides these; it is the base overloads that a
    // generic caller - the desktop, the drag manager - actually reaches through a ComponentNative&.
    ComponentNative& base = *native;
    const ComponentNative& constBase = *native;

    EXPECT_EQ (&comp, &base.getComponent());
    EXPECT_EQ (&comp, &constBase.getComponent());

    delete native;
}

TEST_F (ComponentNativeConstructionTests, RunWithGraphicsContextRunsTheWorkInlineByDefault)
{
    StubComponentNative native (comp, ComponentNative::defaultFlags);

    bool ran = false;
    native.runWithGraphicsContext ([&] { ran = true; });

    EXPECT_TRUE (ran);
}
