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

    /** A scene of an emissive orange quad 0.8 wide in front of the default camera, over @p background. */
    static Scene::Ptr makeQuadScene (Color background)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.4f, -0.4f, 0.0f };
        vertices[1].position = { 0.4f, -0.4f, 0.0f };
        vertices[2].position = { 0.4f, 0.4f, 0.0f };
        vertices[3].position = { -0.4f, 0.4f, 0.0f };
        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->emissiveFactor = { 1.0f, 0.5f, 0.1f };

        auto mesh = Mesh::Ptr (new Mesh ("quad"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

        auto scene = Scene::Ptr (new Scene());
        scene->createEntity ("quad").attach<MeshNode> (mesh);
        scene->getRoot().attach<BackgroundNode> (background);
        return scene;
    }

    /** Paints @p component over a 32 x 32 canvas filled with @p backdrop, as its parent would have left it. */
    Image paintOverBackdrop (SceneComponent& component, Color backdrop)
    {
        auto canvas = GpuCanvas::create (*context, 32, 32);
        if (canvas == nullptr)
            return {};

        auto& g = canvas->beginDraw();
        g.setFillColor (backdrop);
        g.fillAll();
        component.paint (g);

        EXPECT_TRUE (component.getRenderer().getLastError().isEmpty()) << component.getRenderer().getLastError();
        EXPECT_TRUE (component.getRenderer().waitForGPU().wasOk());
        return canvas->asImage();
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

TEST_F (SceneComponentMetalTests, DrawsASeeThroughSceneOverWhatLiesBehind)
{
    const Color backdrop (0xff28a0dc);

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        SCOPED_TRACE (tier == RenderSettings::TierPreference::full ? "full" : "raster");

        SceneComponent component;
        auto settings = component.getRenderer().getRenderSettings();
        settings.tier = tier;
        component.getRenderer().setRenderSettings (settings);
        component.setVisible (true);
        component.setBounds (0.0f, 0.0f, 32.0f, 32.0f);

        // A transparent background leaves what lies behind as it is, around the quad
        component.setScene (makeQuadScene (Colors::transparentBlack));
        EXPECT_FALSE (component.isOpaque());

        auto image = paintOverBackdrop (component, backdrop);
        ASSERT_TRUE (image.isValid());

        auto corner = image.getPixelColor (2, 2);
        EXPECT_NEAR (corner.getRed(), 40, 2);
        EXPECT_NEAR (corner.getGreen(), 160, 2);
        EXPECT_NEAR (corner.getBlue(), 220, 2);

        const auto center = image.getPixelColor (16, 16);
        EXPECT_GT (center.getRed(), center.getBlue() + 60);

        // An orange background of alpha 0.2 adds a fifth of its color to 0.8 of what lies behind
        component.setScene (makeQuadScene (Color (0x33ff6400)));
        EXPECT_FALSE (component.isOpaque());

        image = paintOverBackdrop (component, backdrop);
        ASSERT_TRUE (image.isValid());

        corner = image.getPixelColor (2, 2);
        EXPECT_NEAR (corner.getRed(), 83, 2);
        EXPECT_NEAR (corner.getGreen(), 148, 2);
        EXPECT_NEAR (corner.getBlue(), 176, 2);

        // An opaque background covers it, and the component is opaque again
        component.setScene (makeQuadScene (Color (0xff1a1a2e)));
        EXPECT_TRUE (component.isOpaque());

        image = paintOverBackdrop (component, backdrop);
        ASSERT_TRUE (image.isValid());

        corner = image.getPixelColor (2, 2);
        EXPECT_NEAR (corner.getRed(), 26, 2);
        EXPECT_NEAR (corner.getGreen(), 26, 2);
        EXPECT_NEAR (corner.getBlue(), 46, 2);
    }
}

TEST_F (SceneComponentMetalTests, PaintsAllItsPixelsWhileItIsStillOpaque)
{
    const Color backdrop (0xff28a0dc);

    SceneComponent component;
    component.setVisible (true);
    component.setBounds (0.0f, 0.0f, 32.0f, 32.0f);

    auto scene = makeQuadScene (Color (0xff1a1a2e));
    component.setScene (scene);
    ASSERT_TRUE (component.isOpaque());

    // The background turns translucent without the component knowing. The frame painted next took
    // it as opaque, so it covers what lies behind: with the scene over that background made opaque
    scene->getRoot().get<BackgroundNode>()->color = Color (0x33ff6400);

    auto image = paintOverBackdrop (component, backdrop);
    ASSERT_TRUE (image.isValid());

    auto corner = image.getPixelColor (2, 2);
    EXPECT_NEAR (corner.getRed(), 255, 2);
    EXPECT_NEAR (corner.getGreen(), 100, 2);
    EXPECT_NEAR (corner.getBlue(), 0, 2);

    // Its next display frame finds out, and what lies behind shows through
    component.refreshDisplay (0.0);
    EXPECT_FALSE (component.isOpaque());

    image = paintOverBackdrop (component, backdrop);
    ASSERT_TRUE (image.isValid());

    corner = image.getPixelColor (2, 2);
    EXPECT_NEAR (corner.getRed(), 83, 2);
    EXPECT_NEAR (corner.getGreen(), 148, 2);
    EXPECT_NEAR (corner.getBlue(), 176, 2);
}

TEST_F (SceneComponentMetalTests, AHiddenSceneLetsGoOfWhatItWasRenderedIntoAndIsDrawnAgainOnceItShows)
{
    const Color backdrop (0xff28a0dc);

    SceneComponent component;
    component.setVisible (true);
    component.setBounds (0.0f, 0.0f, 32.0f, 32.0f);
    component.setScene (makeQuadScene (Color (0xff1a1a2e)));

    auto image = paintOverBackdrop (component, backdrop);
    ASSERT_TRUE (image.isValid());
    ASSERT_NE (component.getRenderer().getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);

    // Hidden, nothing is left of the frames it was drawn with
    component.setVisible (false);
    EXPECT_EQ (component.getRenderer().getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);

    // Shown again, its next paint draws the scene as it was
    component.setVisible (true);

    image = paintOverBackdrop (component, backdrop);
    ASSERT_TRUE (image.isValid());
    EXPECT_NE (component.getRenderer().getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);

    const auto corner = image.getPixelColor (2, 2);
    EXPECT_NEAR (corner.getRed(), 26, 2);
    EXPECT_NEAR (corner.getGreen(), 26, 2);
    EXPECT_NEAR (corner.getBlue(), 46, 2);

    const auto center = image.getPixelColor (16, 16);
    EXPECT_GT (center.getRed(), center.getBlue() + 60);
}
