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

    void setFocusedComponent (Component*) override {}

    Component* getFocusedComponent() const override { return nullptr; }

    bool isContinuousRepaintingEnabled() const override { return false; }

    void enableContinuousRepainting (bool) override {}

    bool isAtomicModeEnabled() const override { return false; }

    void enableAtomicMode (bool) override {}

    bool isWireframeEnabled() const override { return false; }

    void enableWireframe (bool) override {}

    void repaint() override {}

    void repaint (const Rectangle<float>&) override {}

    const RectangleList<float>& getRepaintAreas() const override
    {
        static RectangleList<float> r;
        return r;
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

    Component& getComponent() const { return component; }

    Flags getFlags() const { return flags; }

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
        .withTemporaryWindow (true)
        .withGraphicsApi (GpuPlatform::Headless)
        .withFramerateRedraw (60.0f)
        .withClearColor (Color (0xff000000))
        .withDoubleClickTime (RelativeTime::milliseconds (500))
        .withUpdateOnlyFocused (true);

    EXPECT_TRUE (opts.flags.test (ComponentNative::decoratedWindow));
    EXPECT_FALSE (opts.flags.test (ComponentNative::resizableWindow));
    EXPECT_TRUE (opts.flags.test (ComponentNative::renderContinuous));
    EXPECT_TRUE (opts.flags.test (ComponentNative::allowHighDensityDisplay));
    EXPECT_TRUE (opts.flags.test (ComponentNative::captureMouse));
    EXPECT_TRUE (opts.flags.test (ComponentNative::temporaryWindow));
    ASSERT_TRUE (opts.graphicsApi.has_value());
    EXPECT_EQ (*opts.graphicsApi, GpuPlatform::Headless);
    ASSERT_TRUE (opts.framerateRedraw.has_value());
    EXPECT_FLOAT_EQ (*opts.framerateRedraw, 60.0f);
    ASSERT_TRUE (opts.clearColor.has_value());
    EXPECT_TRUE (opts.updateOnlyWhenFocused);
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
