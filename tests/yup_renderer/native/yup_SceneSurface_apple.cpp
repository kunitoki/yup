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

/** The components inside a node of a scene, rendered and shown on its mesh by a Metal device.

    The scenes are 32 pixels wide, seen through the camera the renderer draws a scene without one
    with: a quad 2 wide around the origin is shown from 8.4 to 23.6. They have no light, so a mesh
    only shows what it emits.
*/
class SceneSurfaceMetalTests : public ::testing::Test
{
protected:
    /** A component filled with a color, or with one for its top half and another for its bottom half. */
    class Fill final : public Component
    {
    public:
        void setColors (Color newTop, Color newBottom)
        {
            top = newTop;
            bottom = newBottom;
            repaint();
        }

        void paint (Graphics& g) override
        {
            const auto bounds = getLocalBounds();
            const auto half = bounds.getHeight() * 0.5f;

            g.setFillColor (top);
            g.fillRect (bounds.withHeight (half));

            g.setFillColor (bottom);
            g.fillRect (bounds.withTrimmedTop (half));
        }

    private:
        Color top { 0xff000000 };
        Color bottom { 0xff000000 };
    };

    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";
    }

    /** Makes a scene the one a window would show, 32 pixels wide, without any light over a dark blue background. */
    static void prepare (SceneComponent& view, RenderSettings::TierPreference tier = RenderSettings::TierPreference::raster)
    {
        auto settings = view.getRenderer().getRenderSettings();
        settings.tier = tier;
        view.getRenderer().setRenderSettings (settings);

        view.setVisible (true);
        view.setBounds (0.0f, 0.0f, 32.0f, 32.0f);
        view.setBackgroundColor (Color (0xff204060));
        view.setAmbientColor (Color (0xff000000));
        view.setDefaultLightEnabled (false);
    }

    /** Gives a mesh a quad 2 wide facing the camera, with the texture coordinates 0, 0 at its top left and 1, 1 at its bottom right. */
    static void makeQuad (SceneMeshComponent& mesh)
    {
        const auto numbers = [] (std::initializer_list<double> values)
        {
            Array<var> array;

            for (const auto value : values)
                array.add (value);

            return var (std::move (array));
        };

        mesh.setProperty (Identifier ("vertices"), numbers ({ -1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 1.0, 1.0, 0.0, -1.0, 1.0, 0.0 }));
        mesh.setProperty (Identifier ("uvs"), numbers ({ 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 }));
        mesh.setProperty (Identifier ("indices"), numbers ({ 0.0, 1.0, 2.0, 0.0, 2.0, 3.0 }));
    }

    /** Runs the display frame of a scene, then paints it over a canvas of its size and reads the canvas back. */
    Image paintFrame (SceneComponent& view)
    {
        view.refreshDisplay (0.0);

        auto canvas = GpuCanvas::create (*context, 32, 32);
        if (canvas == nullptr)
            return {};

        view.paint (canvas->beginDraw());

        EXPECT_TRUE (view.getRenderer().getLastError().isEmpty()) << view.getRenderer().getLastError();
        EXPECT_TRUE (view.getRenderer().waitForGPU().wasOk());
        return canvas->asImage();
    }

    /** The material the entity of a node is drawn with, or nullptr when it has none of its own. */
    static Material::Ptr materialOf (const SceneNodeComponent& node)
    {
        const auto entity = node.getEntity();
        const auto* materialNode = entity.isValid() ? entity.get<MaterialNode>() : nullptr;

        return materialNode != nullptr ? materialNode->material : Material::Ptr();
    }

    static void expectBackground (Color pixel)
    {
        EXPECT_NEAR (pixel.getRed(), 0x20, 2);
        EXPECT_NEAR (pixel.getGreen(), 0x40, 2);
        EXPECT_NEAR (pixel.getBlue(), 0x60, 2);
    }

    /** What a red component looks like once emitted and mapped to the display: red above all, whatever the operator. */
    static void expectRed (Color pixel)
    {
        EXPECT_GT (pixel.getRed(), pixel.getGreen() + 60) << pixel.toString();
        EXPECT_GT (pixel.getRed(), pixel.getBlue() + 60) << pixel.toString();
    }

    static void expectBlue (Color pixel)
    {
        EXPECT_GT (pixel.getBlue(), pixel.getRed() + 60) << pixel.toString();
        EXPECT_GT (pixel.getBlue(), pixel.getGreen() + 60) << pixel.toString();
    }

    /** What a mesh that emits nothing looks like: without a color of its own, whatever is left to light it. */
    static void expectNothingEmitted (Color pixel)
    {
        EXPECT_NEAR (pixel.getRed(), pixel.getGreen(), 24) << pixel.toString();
        EXPECT_NEAR (pixel.getRed(), pixel.getBlue(), 24) << pixel.toString();
    }

    const Color red { 0xffff0000 };
    const Color blue { 0xff0000ff };

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;
};

TEST_F (SceneSurfaceMetalTests, ShowsAComponentOnTheMeshOfItsNodeAndFollowsItsRepaints)
{
    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        SCOPED_TRACE (tier == RenderSettings::TierPreference::full ? "full" : "raster");

        SceneComponent view;
        SceneMeshComponent quad;
        Fill panel;

        prepare (view, tier);
        makeQuad (quad);
        view.addAndMakeVisible (quad);

        panel.setColors (red, red);
        panel.setBounds (0.0f, 0.0f, 64.0f, 64.0f);
        quad.addAndMakeVisible (panel);

        auto image = paintFrame (view);
        ASSERT_TRUE (image.isValid());

        // The component where the quad is, emitted by it as there is no light, and the background around it
        expectRed (image.getPixelColor (16, 16));
        expectRed (image.getPixelColor (11, 21));
        expectBackground (image.getPixelColor (2, 2));

        // The material the surface gave the node holds what was rendered, at full strength
        const auto material = materialOf (quad);
        ASSERT_NE (material, nullptr);

        const auto texture = material->emissiveTexture;
        ASSERT_NE (texture, nullptr);
        EXPECT_TRUE (texture->isGpuOnly());
        EXPECT_FLOAT_EQ (material->emissiveFactor[0], 1.0f);
        EXPECT_FLOAT_EQ (material->emissiveFactor[1], 1.0f);
        EXPECT_FLOAT_EQ (material->emissiveFactor[2], 1.0f);
        EXPECT_EQ (material->baseColorTexture, nullptr);

        // A scene painted again shows a component that did not repaint as it was rendered
        const auto changes = texture->getChangeCount();

        image = paintFrame (view);
        ASSERT_TRUE (image.isValid());

        expectRed (image.getPixelColor (16, 16));
        EXPECT_EQ (material->emissiveTexture, texture);
        EXPECT_EQ (texture->getChangeCount(), changes);

        // And what it paints now once it did
        panel.setColors (blue, blue);

        image = paintFrame (view);
        ASSERT_TRUE (image.isValid());

        expectBlue (image.getPixelColor (16, 16));
        expectBackground (image.getPixelColor (2, 2));
        EXPECT_EQ (material->emissiveTexture, texture);
        EXPECT_GT (texture->getChangeCount(), changes);

        // Without the component the mesh emits nothing, and has no material of its own anymore
        quad.removeChildComponent (panel);
        EXPECT_EQ (materialOf (quad), nullptr);
        EXPECT_EQ (material->emissiveTexture, nullptr);

        image = paintFrame (view);
        ASSERT_TRUE (image.isValid());

        expectNothingEmitted (image.getPixelColor (16, 16));
    }
}

TEST_F (SceneSurfaceMetalTests, ShowsWhatAComponentPaintsOnceItsNodeIsShownAgain)
{
    SceneComponent view;
    SceneMeshComponent quad;
    Fill panel;

    prepare (view);
    makeQuad (quad);
    view.addAndMakeVisible (quad);

    panel.setColors (red, red);
    panel.setBounds (0.0f, 0.0f, 64.0f, 64.0f);
    quad.addAndMakeVisible (panel);

    auto image = paintFrame (view);
    ASSERT_TRUE (image.isValid());

    expectRed (image.getPixelColor (16, 16));

    // A node that is hidden shows nothing, and what its component asks to be painted meanwhile is dropped
    quad.setVisible (false);
    panel.setColors (blue, blue);

    image = paintFrame (view);
    ASSERT_TRUE (image.isValid());

    expectBackground (image.getPixelColor (16, 16));

    // Shown again, it shows the component as it paints itself by then
    quad.setVisible (true);

    image = paintFrame (view);
    ASSERT_TRUE (image.isValid());

    expectBlue (image.getPixelColor (16, 16));
    expectBackground (image.getPixelColor (2, 2));
}

TEST_F (SceneSurfaceMetalTests, ShowsTheComponentsTheRightWayUpAloneOrSeveralTogether)
{
    SceneComponent view;
    SceneMeshComponent quad;
    Fill panel;
    Fill lower;

    prepare (view);
    makeQuad (quad);
    view.addAndMakeVisible (quad);

    // A component that is the whole surface is shown as it is rendered: its top at the top of the quad
    panel.setColors (red, blue);
    panel.setBounds (0.0f, 0.0f, 64.0f, 64.0f);
    quad.addAndMakeVisible (panel);

    auto image = paintFrame (view);
    ASSERT_TRUE (image.isValid());

    expectRed (image.getPixelColor (16, 11));
    expectBlue (image.getPixelColor (16, 21));
    expectBackground (image.getPixelColor (2, 2));

    // Two of them are put together where they are in the surface: one over the top half, the other under it
    panel.setColors (red, red);
    panel.setBounds (0.0f, 0.0f, 64.0f, 32.0f);
    lower.setColors (blue, blue);
    lower.setBounds (0.0f, 32.0f, 64.0f, 32.0f);
    quad.addAndMakeVisible (lower);

    image = paintFrame (view);
    ASSERT_TRUE (image.isValid());

    expectRed (image.getPixelColor (16, 11));
    expectBlue (image.getPixelColor (16, 21));
    expectBackground (image.getPixelColor (2, 2));

    // One that repaints is put together with the others again
    lower.setColors (red, red);

    image = paintFrame (view);
    ASSERT_TRUE (image.isValid());

    expectRed (image.getPixelColor (16, 11));
    expectRed (image.getPixelColor (16, 21));

    // A component that is hidden leaves its part of the surface empty: nothing is emitted there
    lower.setVisible (false);

    image = paintFrame (view);
    ASSERT_TRUE (image.isValid());

    expectRed (image.getPixelColor (16, 11));
    expectNothingEmitted (image.getPixelColor (16, 21));
}

TEST_F (SceneSurfaceMetalTests, GivesTheMaterial3DOfTheNodeItsTextureAndTakesItBack)
{
    SceneComponent view;
    SceneMeshComponent quad;
    SceneMaterialComponent paint;
    Fill panel;

    prepare (view);
    makeQuad (quad);
    view.addAndMakeVisible (quad);

    // A material with colors of its own, which the surface is not to be tinted with
    const auto material = paint.getMaterial();
    ASSERT_NE (material, nullptr);
    material->emissiveFactor = { 0.25f, 0.5f, 0.75f };
    material->baseColorFactor = { 0.5f, 0.25f, 0.125f, 0.5f };

    quad.addChildComponent (paint);
    ASSERT_EQ (materialOf (quad), material);

    panel.setColors (red, red);
    panel.setBounds (0.0f, 0.0f, 64.0f, 64.0f);
    quad.addAndMakeVisible (panel);

    auto image = paintFrame (view);
    ASSERT_TRUE (image.isValid());

    // The node keeps its material, which shows the component as it is painted, and its base color
    EXPECT_EQ (materialOf (quad), material);
    EXPECT_NE (material->emissiveTexture, nullptr);
    EXPECT_FLOAT_EQ (material->emissiveFactor[0], 1.0f);
    EXPECT_FLOAT_EQ (material->emissiveFactor[1], 1.0f);
    EXPECT_FLOAT_EQ (material->emissiveFactor[2], 1.0f);
    EXPECT_FLOAT_EQ (material->baseColorFactor[0], 0.5f);
    expectRed (image.getPixelColor (16, 16));

    // Shown as the base color instead, the material emits what it did again
    quad.setSurfaceSlot (SceneNodeComponent::SurfaceSlot::baseColor);

    EXPECT_EQ (material->emissiveTexture, nullptr);
    EXPECT_FLOAT_EQ (material->emissiveFactor[0], 0.25f);
    EXPECT_FLOAT_EQ (material->emissiveFactor[1], 0.5f);
    EXPECT_FLOAT_EQ (material->emissiveFactor[2], 0.75f);

    image = paintFrame (view);
    ASSERT_TRUE (image.isValid());

    // With a base color of 1 that keeps its alpha, for the component to be lit as it is painted
    EXPECT_NE (material->baseColorTexture, nullptr);
    EXPECT_EQ (material->emissiveTexture, nullptr);
    EXPECT_FLOAT_EQ (material->baseColorFactor[0], 1.0f);
    EXPECT_FLOAT_EQ (material->baseColorFactor[1], 1.0f);
    EXPECT_FLOAT_EQ (material->baseColorFactor[2], 1.0f);
    EXPECT_FLOAT_EQ (material->baseColorFactor[3], 0.5f);

    // And is as its owner made it once the components are gone
    quad.removeChildComponent (panel);

    EXPECT_EQ (materialOf (quad), material);
    EXPECT_EQ (material->baseColorTexture, nullptr);
    EXPECT_EQ (material->emissiveTexture, nullptr);
    EXPECT_FLOAT_EQ (material->baseColorFactor[0], 0.5f);
    EXPECT_FLOAT_EQ (material->baseColorFactor[1], 0.25f);
    EXPECT_FLOAT_EQ (material->baseColorFactor[2], 0.125f);
    EXPECT_FLOAT_EQ (material->baseColorFactor[3], 0.5f);
    EXPECT_FLOAT_EQ (material->emissiveFactor[0], 0.25f);
    EXPECT_FLOAT_EQ (material->emissiveFactor[2], 0.75f);
}
