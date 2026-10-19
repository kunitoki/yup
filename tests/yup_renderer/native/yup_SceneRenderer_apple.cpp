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

TEST_F (SceneRendererMetalTests, CapturesBaselineScenes)
{
    captureBaselineScenes ("metal");
}

TEST_F (SceneRendererMetalTests, MatchesPhaseZeroBaselineOnRaster)
{
    checkMatchesPhaseZeroBaseline ("metal");
}

TEST_F (SceneRendererMetalTests, RendersWithoutMultisamplingIntoTheReturnedTexture)
{
    // An emissive quad above and left of the center makes the frame asymmetric, so a flip shows
    const auto buildScene = [] (Scene& scene)
    {
        buildShadowsScene (scene);

        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.7f, 0.3f, 0.6f };
        vertices[1].position = { -0.3f, 0.3f, 0.6f };
        vertices[2].position = { -0.3f, 0.7f, 0.6f };
        vertices[3].position = { -0.7f, 0.7f, 0.6f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto emissive = Material::Ptr (new Material());
        emissive->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        emissive->emissiveFactor = { 1.0f, 0.5f, 0.1f };

        auto mesh = Mesh::Ptr (new Mesh ("marker"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, emissive);

        auto marker = Entity::Ptr (new Entity ("marker"));
        marker->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (marker);
    };

    // Orange marker pixels in the top-left and bottom-left quadrants
    const auto countMarker = [] (const Image& image, bool top)
    {
        int count = 0;
        for (int y = top ? 0 : 32; y < (top ? 32 : 64); ++y)
        {
            for (int x = 0; x < 32; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                if (static_cast<int> (color.getRed()) - static_cast<int> (color.getBlue()) > 80)
                    ++count;
            }
        }

        return count;
    };

    // The sample count applies to the raster tier only
    Scene scene;
    buildScene (scene);

    SceneRenderer single;
    setTier (single, RenderSettings::TierPreference::raster);
    single.setSampleCount (1);
    const auto image = renderToImage (single, scene, 64);
    ASSERT_TRUE (single.getLastError().isEmpty()) << single.getLastError();
    ASSERT_TRUE (image.isValid());

    SceneRenderer multisampled;
    setTier (multisampled, RenderSettings::TierPreference::raster);
    const auto reference = renderToImage (multisampled, scene, 64);
    ASSERT_TRUE (reference.isValid());

    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    EXPECT_NEAR (image.getPixelColor (1, 1).getRed(), 206, 3);

    // The marker lands at the top on both paths, never at the bottom
    EXPECT_GT (countMarker (reference, true), 4);
    EXPECT_EQ (countMarker (reference, false), 0);
    EXPECT_GT (countMarker (image, true), 4);
    EXPECT_EQ (countMarker (image, false), 0);
}

TEST_F (SceneRendererMetalTests, GpuFailureOfOneRendererDoesNotFailAnother)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer first, second;
    ASSERT_NE (first.render (device, scene, 32, 32), nullptr) << first.getLastError();
    ASSERT_NE (second.render (device, scene, 32, 32), nullptr) << second.getLastError();
    ASSERT_TRUE (first.waitForGPU().wasOk());
    ASSERT_TRUE (second.waitForGPU().wasOk());

    // Only the first renderer's next frame fails
    device->failNextFrameForTesting ("injected failure");
    EXPECT_NE (first.render (device, scene, 32, 32), nullptr) << first.getLastError();
    EXPECT_NE (second.render (device, scene, 32, 32), nullptr) << second.getLastError();
    EXPECT_TRUE (second.waitForGPU().wasOk());
    EXPECT_NE (second.render (device, scene, 32, 32), nullptr) << second.getLastError();

    EXPECT_TRUE (second.getLastGpuFailure().isEmpty());

    // The failure is found by the next render of its own renderer, which still renders, and by nobody else
    EXPECT_NE (first.render (device, scene, 32, 32), nullptr) << first.getLastError();
    EXPECT_EQ (first.getLastGpuFailure(), "injected failure");
    EXPECT_TRUE (device->takeGpuFailure().wasOk());

    EXPECT_NE (first.render (device, scene, 32, 32), nullptr) << first.getLastError();
    EXPECT_TRUE (first.getLastGpuFailure().isEmpty());
    EXPECT_TRUE (first.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, GpuFailureStillRendersTheNextFrame)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    // Frames complete in order, so once a later frame did the failed one has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    auto texture = renderer.render (device, scene, 64, 64);
    ASSERT_NE (texture, nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
    EXPECT_TRUE (renderer.getLastError().isEmpty());

    // The history starts over, so the renderer keeps refining (and a component repainting)
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    const auto image = readBack (texture);
    ASSERT_TRUE (image.isValid());

    const auto background = SceneSettings::resolve (scene).backgroundColor;
    bool drewSomething = false;
    for (int y = 0; y < 64 && ! drewSomething; y += 2)
        for (int x = 0; x < 64 && ! drewSomething; x += 2)
            drewSomething = image.getPixelColor (x, y) != background;

    EXPECT_TRUE (drewSomething);
}

TEST_F (SceneRendererMetalTests, WaitForGpuReportsTheRenderersFailureOnce)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 32, 32), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.waitForGPU().wasOk());

    device->failNextFrameForTesting ("injected failure");
    EXPECT_NE (renderer.render (device, scene, 32, 32), nullptr);

    EXPECT_EQ (renderer.waitForGPU().getErrorMessage(), "injected failure");
    EXPECT_NE (renderer.render (device, scene, 32, 32), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.getLastGpuFailure().isEmpty());
    EXPECT_TRUE (device->takeGpuFailure().wasOk());
}

TEST_F (SceneRendererMetalTests, SwappingTransformsOfTwoMeshesIsAChange)
{
    Scene scene;

    // Two emissive quads, orange on the left and blue on the right
    const auto addQuad = [&] (const char* name, std::array<float, 3> emissive, float x)
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
        material->emissiveFactor = emissive;

        auto mesh = Mesh::Ptr (new Mesh (name));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);
        auto entity = Entity::Ptr (new Entity (name));
        entity->attach<MeshNode> (mesh);
        entity->setPosition ({ x, 0.0f, 0.0f });
        scene.getRoot()->addChild (entity);
        return entity;
    };

    auto orange = addQuad ("orange", { 1.0f, 0.5f, 0.1f }, -0.6f);
    auto blue = addQuad ("blue", { 0.1f, 0.2f, 1.0f }, 0.6f);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    Image image;
    for (int i = 0; i < 4; ++i)
        image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (renderer.isConverged()) << renderer.getLastError();

    const auto isBlue = [] (Color color) { return color.getBlue() > color.getRed() + 60; };
    ASSERT_FALSE (isBlue (image.getPixelColor (20, 32)));
    ASSERT_TRUE (isBlue (image.getPixelColor (44, 32)));

    // The same transforms, the same meshes and materials, but paired the other way round
    orange->setPosition ({ 0.6f, 0.0f, 0.0f });
    blue->setPosition ({ -0.6f, 0.0f, 0.0f });

    image = renderToImage (renderer, scene, 64);
    EXPECT_FALSE (renderer.isConverged());

    for (int i = 0; i < 4 && ! renderer.isConverged(); ++i)
        image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (isBlue (image.getPixelColor (20, 32)));
    EXPECT_FALSE (isBlue (image.getPixelColor (44, 32)));
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, FailedRenderClearsThePassOutputs)
{
    Scene scene;
    buildShadowsScene (scene);

    SceneRenderer renderer;
    setTier (renderer, RenderSettings::TierPreference::full);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), nullptr);
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::gbufferNormal), nullptr);

    EXPECT_EQ (renderer.render (device, scene, 0, 64), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::gbufferNormal), nullptr);

    // Rendering again would fail again: nothing to wait for
    EXPECT_TRUE (renderer.isConverged());
}
#endif

TEST_F (SceneRendererMetalTests, DeferredMatchesForwardForOpaque)
{
    // The lighting pass shades the G-buffer with the BSDF and the light code of the forward pass:
    // the interior of each baseline scene matches a forced raster frame, away from the edges where
    // MSAA and TAA differ. The mean over the interior catches global drift, the 8x8 blocks of
    // opaque texels catch a local one. The blended quad of the materials scene is drawn forward on
    // both tiers but blended in linear light on full only, so its blocks are left out. The baseline
    // scenes are compact enough for one scene-fitted sun map; the long ground keeps two view-fitted
    // cascades, so the raster blend and the full dither between them are compared too
    const struct
    {
        const char* name;
        void (*build) (Scene&);
        Rectangle<int> blended;
        bool viewFitted = false; // keeps the view-fitted cascades, so the tiers compare their handover too
    } scenes[] = {
        { "shadows", buildShadowsScene, {} },
        { "materials", buildMaterialsScene, { 74, 53, 24, 24 } },
        { "areaLights", buildAreaLightsScene, {} },
        { "longGround", [] (Scene& scene) { buildLongGroundScene (scene, 2.0f); }, {}, true },
    };

    for (const auto& [name, build, blended, viewFitted] : scenes)
    {
        Image images[2];

        for (const auto tier : { RenderSettings::TierPreference::full, RenderSettings::TierPreference::raster })
        {
            // Both tiers shadow alike: the raster cascades, no soft shadows, the full frame converged
            Scene scene;
            build (scene);

            SceneRenderer renderer;
            setTier (renderer, tier);
            pinCrossTierShadows (renderer);
            auto& image = images[tier == RenderSettings::TierPreference::full ? 0 : 1];
            image = renderConverged (renderer, scene, 128);
            ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();
            ASSERT_EQ (renderer.getCapabilities()->getTier() == RenderCapabilities::Tier::full, tier == RenderSettings::TierPreference::full) << name;

            if (viewFitted)
                EXPECT_EQ (shadows (renderer).getShadowData().sunInfo[0], 2.0f) << name;
        }

        const auto full = meanLuminance (images[0], { 32, 32, 64, 64 });
        const auto raster = meanLuminance (images[1], { 32, 32, 64, 64 });
        const auto difference = std::abs (full - raster) / raster;
        RecordProperty ((String (name) + "Difference").toStdString(), String (difference * 100.0, 2).toStdString());
        EXPECT_LE (difference, 0.03) << name << ": full " << full << ", raster " << raster;

        auto largestBlockDifference = 0.0;
        for (int y = 32; y < 96; y += 8)
        {
            for (int x = 32; x < 96; x += 8)
            {
                const Rectangle<int> block (x, y, 8, 8);
                if (block.intersects (blended))
                    continue;

                largestBlockDifference = jmax (largestBlockDifference, std::abs (meanLuminance (images[0], block) - meanLuminance (images[1], block)));
            }
        }

        RecordProperty ((String (name) + "LargestBlockDifference").toStdString(), String (largestBlockDifference, 2).toStdString());
        // Measured at most 1.7 levels of luma (half float roughness, 8 bit base color, the converged
        // TAA average); a local shading error moves a block by tens of levels
        EXPECT_LE (largestBlockDifference, 4.0) << name;
    }
}

TEST_F (SceneRendererMetalTests, BackgroundIsKeptBehindLitPixels)
{
    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    {
        Scene scene;
        buildShadowsScene (scene);

        SceneRenderer renderer;
        setTier (renderer, RenderSettings::TierPreference::full);
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto corner = image.getPixelColor (1, 1);
        EXPECT_NEAR (corner.getRed(), 206, 3);
        EXPECT_NEAR (corner.getGreen(), 206, 3);
        EXPECT_NEAR (corner.getBlue(), 206, 3);
    }

    // A red sky over a blue ground: the full tier shows each where the raster tier does
    Image images[2];

    for (const auto tier : { RenderSettings::TierPreference::full, RenderSettings::TierPreference::raster })
    {
        Scene scene;
        buildShadowsScene (scene);
        setEnvironmentMap (scene, new EnvironmentMap ([] (const Vector3<float>& d)
        {
            return d.getY() > 0.0f ? Vector3<float> (0.8f, 0.1f, 0.05f) : Vector3<float> (0.05f, 0.1f, 0.8f);
        }, 32, 3));

        SceneRenderer renderer;
        setTier (renderer, tier);
        images[tier == RenderSettings::TierPreference::full ? 0 : 1] = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (images[tier == RenderSettings::TierPreference::full ? 0 : 1].isValid()) << renderer.getLastError();
    }

    const auto top = images[0].getPixelColor (1, 1);
    const auto bottom = images[0].getPixelColor (1, 62);
    EXPECT_GT (top.getRed(), top.getBlue() + 50);
    EXPECT_GT (bottom.getBlue(), bottom.getRed() + 50);

    for (const auto& [x, y] : { std::pair { 1, 1 }, std::pair { 1, 62 }, std::pair { 62, 1 }, std::pair { 62, 62 } })
    {
        const auto full = images[0].getPixelColor (x, y);
        const auto raster = images[1].getPixelColor (x, y);
        EXPECT_NEAR (full.getRed(), raster.getRed(), 3) << x << ", " << y;
        EXPECT_NEAR (full.getGreen(), raster.getGreen(), 3) << x << ", " << y;
        EXPECT_NEAR (full.getBlue(), raster.getBlue(), 3) << x << ", " << y;
    }
}

TEST_P (SceneRendererTierTests, ReportsTheRequestedTier)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    EXPECT_EQ (renderer.getCapabilities(), nullptr);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto* capabilities = renderer.getCapabilities();
    ASSERT_NE (capabilities, nullptr);
    EXPECT_EQ (capabilities->getTier(), GetParam() == RenderSettings::TierPreference::full ? RenderCapabilities::Tier::full : RenderCapabilities::Tier::raster);
}

TEST_P (SceneRendererTierTests, BlendItemsDrawOverDeferred)
{
    // A half transparent blue quad in front of a white plate tints it, and one behind the plate is
    // hidden by the depth the plate wrote
    const auto addQuad = [] (Scene& scene, float x, float z)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { x - 0.3f, -0.3f, z };
        vertices[1].position = { x + 0.3f, -0.3f, z };
        vertices[2].position = { x + 0.3f, 0.3f, z };
        vertices[3].position = { x - 0.3f, 0.3f, z };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 1.0f, 0.5f };
        material->alphaMode = Material::AlphaMode::blend;

        auto mesh = Mesh::Ptr (new Mesh ("blended"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

        auto entity = Entity::Ptr (new Entity ("blended"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
    };

    auto white = Material::Ptr (new Material());
    white->metallicFactor = 0.0f;

    Scene scene;
    addPlate (scene, white);
    addQuad (scene, 0.0f, 0.5f);
    addQuad (scene, -0.6f, -0.5f);

    auto& renderer = sharedRenderer();
    setTier (renderer, GetParam());
    const auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    // The plate alone is gray. The full tier blends in linear light and the raster tier after tone
    // mapping, so the mix differs between them, but both darken the red and keep more blue
    const auto plate = image.getPixelColor (44, 32);
    const auto tinted = image.getPixelColor (32, 32);
    EXPECT_NEAR (plate.getRed(), plate.getBlue(), 4);
    EXPECT_GT (static_cast<int> (tinted.getBlue()), static_cast<int> (tinted.getRed()) + 10);
    EXPECT_LT (static_cast<int> (tinted.getRed()), static_cast<int> (plate.getRed()) - 20);
    EXPECT_GT (tinted.getRed(), 0);

    // The hidden quad would cover about 5 px around (24, 32)
    const auto hidden = image.getPixelColor (24, 32);
    EXPECT_NEAR (hidden.getRed(), plate.getRed(), 4);
    EXPECT_NEAR (hidden.getBlue(), plate.getBlue(), 4);
}

TEST_P (SceneRendererTierTests, DefaultSettingsAddNoOptInPass)
{
    checkDefaultSettingsAddNoOptInPass (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, PrepassMatchesTheGBufferEncoding)
{
    checkPrepassMatchesTheGBufferEncoding (GetParam());
}
#endif

TEST_P (SceneRendererTierTests, PrepassIsNotCreatedByDefault)
{
    checkPrepassIsNotCreatedByDefault (GetParam());
}

INSTANTIATE_TEST_SUITE_P (SceneRendererTiers,
                          SceneRendererTierTests,
                          ::testing::Values (RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full),
                          [] (const auto& info)
                          {
                              return info.param == RenderSettings::TierPreference::full ? "full" : "raster";
                          });
