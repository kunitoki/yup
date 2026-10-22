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

#include <yup_renderer/yup_renderer.h>

using namespace yup;

namespace yup
{

/** Reads and clears the repaint request of a component: repaint() records it even without a window. */
template <>
class ComponentTestHelper<SceneComponent>
{
public:
    static bool takeRepaintRequest (Component& component)
    {
        return component.subtreeDirty.exchange (false);
    }
};

} // namespace yup

class SceneComponentMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";
    }

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;
};

TEST_F (SceneComponentMetalTests, KeepsRepaintingUntilConverged)
{
    using Helper = ComponentTestHelper<SceneComponent>;

    auto scene = Scene::Ptr (new Scene());

    SceneComponent component;
    auto settings = component.getRenderer().getRenderSettings();
    settings.tier = RenderSettings::TierPreference::full;
    settings.taaSamples = 4;
    component.getRenderer().setRenderSettings (settings);
    component.setVisible (true);
    component.setBounds (0.0f, 0.0f, 32.0f, 32.0f);
    component.setScene (scene);

    auto canvas = GpuCanvas::create (*context, 32, 32);
    ASSERT_NE (canvas, nullptr);

    // Each display frame paints, then asks for another paint until the frames have accumulated
    for (int frame = 1; frame <= 6; ++frame)
    {
        component.paint (canvas->beginDraw());
        canvas->commit();
        ASSERT_TRUE (component.getRenderer().getLastError().isEmpty()) << component.getRenderer().getLastError();

        Helper::takeRepaintRequest (component);
        component.refreshDisplay (0.0);
        EXPECT_EQ (Helper::takeRepaintRequest (component), frame < 4) << "frame " << frame;
    }
}

TEST_F (SceneComponentMetalTests, DoesNotRepaintWhenNothingWasRendered)
{
    using Helper = ComponentTestHelper<SceneComponent>;

    auto scene = Scene::Ptr (new Scene());

    SceneComponent component;
    auto settings = component.getRenderer().getRenderSettings();
    settings.tier = RenderSettings::TierPreference::full;
    settings.taaSamples = 4;
    component.getRenderer().setRenderSettings (settings);
    component.setVisible (true);
    component.setBounds (0.0f, 0.0f, 32.0f, 32.0f);
    component.setScene (scene);

    auto canvas = GpuCanvas::create (*context, 32, 32);
    ASSERT_NE (canvas, nullptr);

    component.paint (canvas->beginDraw());
    canvas->commit();
    ASSERT_FALSE (component.getRenderer().isConverged());

    // Shrunk below a pixel, paint returns before rendering: accumulating can't progress, so no repaint
    component.setBounds (0.0f, 0.0f, 0.4f, 0.4f);
    component.paint (canvas->beginDraw());
    canvas->commit();

    Helper::takeRepaintRequest (component);
    component.refreshDisplay (0.0);
    EXPECT_FALSE (Helper::takeRepaintRequest (component));
}
