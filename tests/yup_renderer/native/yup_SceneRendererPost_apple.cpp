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

TEST_F (SceneRendererMetalTests, AgxToneMapsTheEnvironment)
{
    const auto mid = renderEnvironmentCorner (0.5f, ToneMappingOperator::agx);
    const auto expected = SceneRendererAgxMirrorTests::toneMapAgx ({ 0.5f, 0.5f, 0.5f });

    // 0.5 through AgX lands inside the displayable range, neither crushed nor clipped
    EXPECT_GT (mid.getRed(), 0);
    EXPECT_LT (mid.getRed(), 255);
    EXPECT_NEAR (mid.getRed(), expected[0], 3);
    EXPECT_NEAR (mid.getGreen(), expected[1], 3);
    EXPECT_NEAR (mid.getBlue(), expected[2], 3);

    const auto aces = renderEnvironmentCorner (0.5f, ToneMappingOperator::aces);
    EXPECT_GE (std::abs (static_cast<int> (aces.getRed()) - static_cast<int> (mid.getRed())), 3);

    const auto dim = renderEnvironmentCorner (0.25f, ToneMappingOperator::agx);
    const auto bright = renderEnvironmentCorner (1.0f, ToneMappingOperator::agx);
    EXPECT_NEAR (dim.getRed(), SceneRendererAgxMirrorTests::toneMapAgx ({ 0.25f, 0.25f, 0.25f })[0], 3);
    EXPECT_NEAR (bright.getRed(), SceneRendererAgxMirrorTests::toneMapAgx ({ 1.0f, 1.0f, 1.0f })[0], 3);
    EXPECT_GT (bright.getRed(), mid.getRed());
    EXPECT_GT (mid.getRed(), dim.getRed());
}

TEST_F (SceneRendererMetalTests, RasterBloomOffStaysOnTheBaseline)
{
    const std::pair<const char*, void (*) (Scene&)> scenes[] = {
        { "shadows", buildShadowsScene },
        { "materials", buildMaterialsScene },
        { "edges", buildEdgesScene },
    };

    for (const auto& [name, build] : scenes)
    {
        Scene scene;
        build (scene);

        // Bloom explicitly off, its other settings changed: the frame stays on the rgba8 inline path
        RenderSettings settings;
        settings.tier = RenderSettings::TierPreference::raster;
        settings.bloom = false;
        settings.bloomThreshold = 0.1f;
        settings.bloomIntensity = 1.0f;
        settings.bloomRadius = 1.0f;

        SceneRenderer renderer;
        renderer.setRenderSettings (settings);
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr) << name;
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba8unorm) << name;
        EXPECT_FALSE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::hdrSceneColor, settings)) << name;

        // The cascades replaced the single map of the Phase 0 shadows
        if (String (name) == "shadows")
        {
            checkShadowsAgainstBaseline (image, "metal");
            continue;
        }

        const auto differing = countBaselineDifferences (image, String ("metal_") + name + ".png");
        ASSERT_GE (differing, 0) << "capture the Phase 0 baselines first";

        RecordProperty ((String (name) + "Differing").toStdString(), differing);
        EXPECT_EQ (differing, 0) << name;
    }
}

TEST_F (SceneRendererMetalTests, RasterBloomIsUnavailableWhenHalfFloatCannotMultisample)
{
    // Metal multisamples rgba16float at 4: pretend it allows only 2, below the default 4 samples
    const auto halfFloatTakesTwoSamples = [] (RenderCapabilities::DeviceFacts& facts)
    {
        facts.rgba16floatSamples = 2;
    };

    Scene scene;
    buildGlowScene (scene);

    SceneRenderer withoutBloom;
    setTier (withoutBloom, RenderSettings::TierPreference::raster);
    withoutBloom.setDeviceFactsOverride (halfFloatTakesTwoSamples);
    const auto reference = renderToImage (withoutBloom, scene, 64);
    ASSERT_TRUE (reference.isValid()) << withoutBloom.getLastError();

    SceneRenderer withBloom;
    withBloom.setRenderSettings (withoutBloom.getRenderSettings());
    setBloom (withBloom, true);
    withBloom.setDeviceFactsOverride (halfFloatTakesTwoSamples);
    const auto image = renderToImage (withBloom, scene, 64);
    ASSERT_TRUE (image.isValid()) << withBloom.getLastError();

    const auto* capabilities = withBloom.getCapabilities();
    ASSERT_NE (capabilities, nullptr);
    EXPECT_EQ (capabilities->getTier(), RenderCapabilities::Tier::raster);
    EXPECT_FALSE (capabilities->isAvailable (RenderCapabilities::Feature::bloom));
    EXPECT_FALSE (capabilities->isEnabled (RenderCapabilities::Feature::hdrSceneColor, withBloom.getRenderSettings()));

    // Still the multisampled rgba8 inline frame: not black, not one sample, identical to bloom off
    const auto sceneColor = withBloom.getPassOutput (SceneRenderer::PassOutput::sceneColor);
    ASSERT_NE (sceneColor, nullptr);
    EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba8unorm);

    // Two renders of one raster frame can differ by a level here and there
    EXPECT_EQ (countDifferences (image, reference), 0);
    EXPECT_GT (meanLuminance (image, findBrightBounds (image)), 200.0);
}

TEST_F (SceneRendererMetalTests, BloomStaysFiniteOverSaturatedHighlights)
{
    // Exposed radiance far above the half float range: the scene color and the TAA history hold
    // the largest half float, and the glow added over it must not overflow to infinity
    Scene scene;
    buildGlowScene (scene, 1.0e5f);

    SceneRenderer withoutBloom;
    setFullTierTaa (withoutBloom, 4);
    const auto off = renderToImage (withoutBloom, scene, 128);
    ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

    const auto area = findBrightBounds (off).enlarged (8);
    ASSERT_FALSE (area.isEmpty());

    SceneRenderer withBloom;
    withBloom.setRenderSettings (withoutBloom.getRenderSettings());
    setBloom (withBloom, true);
    for (int frame = 1; frame <= 4; ++frame)
    {
        const auto image = renderToImage (withBloom, scene, 128);
        ASSERT_TRUE (image.isValid()) << withBloom.getLastError();

        int dark = 0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
            for (int x = area.getX(); x < area.getRight(); ++x)
                dark += image.getPixelColor (x, y).getRed() < 100 ? 1 : 0;

        EXPECT_EQ (dark, 0) << "frame " << frame;
    }
}

TEST_F (SceneRendererMetalTests, RasterBloomRingMatchesTheFullTier)
{
    std::map<RenderSettings::TierPreference, double> rings;

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        Scene scene;
        buildGlowScene (scene);

        SceneRenderer withoutBloom;
        setTier (withoutBloom, tier);
        const auto off = renderToImage (withoutBloom, scene, 128);
        ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

        SceneRenderer withBloom;
        withBloom.setRenderSettings (withoutBloom.getRenderSettings());
        setBloom (withBloom, true);
        const auto on = renderToImage (withBloom, scene, 128);
        ASSERT_TRUE (on.isValid()) << withBloom.getLastError();

        rings[tier] = ringLuminance (on, findBrightBounds (off));
    }

    const auto raster = rings[RenderSettings::TierPreference::raster];
    const auto full = rings[RenderSettings::TierPreference::full];
    RecordProperty ("rasterRing", String (raster, 2).toStdString());
    RecordProperty ("fullRing", String (full, 2).toStdString());

    EXPECT_GT (full, 5.0);
    EXPECT_NEAR (raster, full, full * 0.1);
}

TEST_F (SceneRendererMetalTests, RasterRendererTogglesBloomOnOffOn)
{
    Scene scene;
    buildGlowScene (scene);

    SceneRenderer reference;
    setTier (reference, RenderSettings::TierPreference::raster);
    const auto inline8 = renderToImage (reference, scene, 128);
    ASSERT_TRUE (inline8.isValid()) << reference.getLastError();
    const auto quad = findBrightBounds (inline8);

    SceneRenderer renderer;
    renderer.setRenderSettings (reference.getRenderSettings());
    GpuTexture::Ptr halfFloatSceneColor;
    for (const auto bloom : { true, false, true })
    {
        setBloom (renderer, bloom);

        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        ASSERT_TRUE (renderer.waitForGPU().wasOk());

        // Bloom renders the reversible half-float scene color, without it the rgba8 inline frame
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), bloom ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm) << "bloom " << bloom;

        // The frame without bloom keeps nothing of the half-float scene color of the frame before it
        if (bloom)
            halfFloatSceneColor = sceneColor;
        else if (halfFloatSceneColor != nullptr)
            EXPECT_EQ (std::exchange (halfFloatSceneColor, nullptr)->getReferenceCount(), 1);
        EXPECT_EQ (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::hdrSceneColor, renderer.getRenderSettings()), bloom);

        const auto ring = ringLuminance (image, quad);
        RecordProperty ((String ("ring") + (bloom ? "On" : "Off")).toStdString(), String (ring, 2).toStdString());

        if (bloom)
            EXPECT_GE (ring, 5.0);
        else
            EXPECT_EQ (countDifferences (image, inline8), 0);

        EXPECT_GE (meanLuminance (image, quad.reduced (2)), 245.0) << "bloom " << bloom;
    }
}

TEST_F (SceneRendererMetalTests, BloomChangesKeepTheTaaHistory)
{
    Scene scene;
    buildGlowScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    setBloom (renderer, true);
    Image converged;
    for (int i = 0; i < 4; ++i)
        converged = renderToImage (renderer, scene, 128);

    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Bloom runs after temporal anti-aliasing, so its settings apply to the kept history
    auto settings = renderer.getRenderSettings();
    settings.bloomIntensity = 0.5f;
    renderer.setRenderSettings (settings);

    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    const auto quad = findBrightBounds (converged);
    const auto before = ringLuminance (converged, quad);
    const auto after = ringLuminance (image, quad);
    RecordProperty ("ringBefore", String (before, 2).toStdString());
    RecordProperty ("ringAfter", String (after, 2).toStdString());
    EXPECT_GT (after, before + 5.0);
}

TEST_F (SceneRendererMetalTests, FullTierMatchesRasterExposureAndToneMapping)
{
    std::map<RenderSettings::TierPreference, double> luminance;

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        // Both tiers shadow alike: the raster cascades, no soft shadows, the full frame converged
        Scene scene;
        buildShadowsScene (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        pinCrossTierShadows (renderer);
        const auto image = renderConverged (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto* capabilities = renderer.getCapabilities();
        ASSERT_NE (capabilities, nullptr);
        ASSERT_EQ (capabilities->getTier() == RenderCapabilities::Tier::full, tier == RenderSettings::TierPreference::full);

        // The full frame really went through the HDR scene color, not a raster fallback
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), tier == RenderSettings::TierPreference::full ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm);

        // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
        const auto corner = image.getPixelColor (1, 1);
        EXPECT_NEAR (corner.getRed(), 206, 3);
        EXPECT_NEAR (corner.getGreen(), 206, 3);
        EXPECT_NEAR (corner.getBlue(), 206, 3);
        RecordProperty (tier == RenderSettings::TierPreference::full ? "fullCorner" : "rasterCorner", static_cast<int> (corner.getRed()));

        luminance[tier] = meanLuminance (image, { 32, 32, 64, 64 });
    }

    const auto raster = luminance[RenderSettings::TierPreference::raster];
    const auto full = luminance[RenderSettings::TierPreference::full];
    RecordProperty ("rasterLuminance", String (raster, 3).toStdString());
    RecordProperty ("fullLuminance", String (full, 3).toStdString());

    // Same shading, different antialiasing
    EXPECT_NEAR (full, raster, raster * 0.02);
}

TEST_F (SceneRendererMetalTests, FullTierBackgroundColorMatchesRaster)
{
    // No environment: the background color shows around the plates, display-referred on both tiers
    const auto background = Color (static_cast<uint8> (200), static_cast<uint8> (80), static_cast<uint8> (40));

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        const auto tierName = tier == RenderSettings::TierPreference::full ? String ("full") : String ("raster");

        Scene scene;
        makePlates (scene);
        auto sun = scene.createEntity ("sun");
        sun.attach<LightNode>();
        scene.getRoot().attach<BackgroundNode> (background);

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << tierName << ": " << renderer.getLastError();
        ASSERT_EQ (renderer.getCapabilities()->getTier() == RenderCapabilities::Tier::full, tier == RenderSettings::TierPreference::full);

        const auto corner = image.getPixelColor (1, 1);
        RecordProperty ((tierName + "Corner").toStdString(), (String (static_cast<int> (corner.getRed())) + "," + String (static_cast<int> (corner.getGreen())) + "," + String (static_cast<int> (corner.getBlue()))).toStdString());
        EXPECT_NEAR (corner.getRed(), background.getRed(), 1) << tierName;
        EXPECT_NEAR (corner.getGreen(), background.getGreen(), 1) << tierName;
        EXPECT_NEAR (corner.getBlue(), background.getBlue(), 1) << tierName;

        // The lit white plate covers the center
        const auto center = image.getPixelColor (32, 32);
        EXPECT_GT (std::abs (static_cast<int> (center.getBlue()) - static_cast<int> (background.getBlue())), 40) << tierName;
    }
}

TEST_F (SceneRendererMetalTests, FullTierBackgroundColorSurvivesTaaConvergence)
{
    // Temporal anti-aliasing resolves the coverage alpha too, so the background still shows exactly
    const auto background = Color (static_cast<uint8> (200), static_cast<uint8> (80), static_cast<uint8> (40));

    Scene scene;
    buildEdgesScene (scene);
    scene.getRoot().attach<BackgroundNode> (background);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    for (int frame = 1; frame <= 6; ++frame)
    {
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto corner = image.getPixelColor (1, 1);
        EXPECT_NEAR (corner.getRed(), background.getRed(), 1) << "frame " << frame;
        EXPECT_NEAR (corner.getGreen(), background.getGreen(), 1) << "frame " << frame;
        EXPECT_NEAR (corner.getBlue(), background.getBlue(), 1) << "frame " << frame;
    }

    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, RasterIsAlwaysConverged)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setTier (renderer, RenderSettings::TierPreference::raster);
    EXPECT_TRUE (renderer.isConverged());

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    // Changing the scene changes nothing on raster
    scene.getRoot().attach<ExposureNode> (2.0f);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, ConvergesThenRestartsOnChange)
{
    Scene scene;
    buildEdgesScene (scene);

    auto material = Material::Ptr (new Material());
    auto plates = scene.getRoot().findChild ("plates");
    auto sun = scene.getRoot().findChild ("sun");
    ASSERT_TRUE (plates.isValid());
    ASSERT_TRUE (sun.isValid());
    plates.attach<MaterialNode> (material);

    auto camera = scene.createEntity ("camera");
    camera.attach<CameraNode>();
    camera.getOrAttach<TransformNode>().setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.setActiveCamera (camera);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    const auto expectConvergesAfterFourRenders = [&] (const char* step)
    {
        for (int i = 1; i <= 4; ++i)
        {
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << step << ": " << renderer.getLastError();
            EXPECT_EQ (renderer.isConverged(), i == 4) << step << ", render " << i;
        }

        // Nothing changed: still converged
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << step << ": " << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged()) << step;
    };

    expectConvergesAfterFourRenders ("first");

    scene.getRoot().attach<ExposureNode> (2.0f);
    expectConvergesAfterFourRenders ("exposure");

    material->roughnessFactor = 0.5f;
    expectConvergesAfterFourRenders ("material");

    sun.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 0.4f));
    expectConvergesAfterFourRenders ("light");

    camera.getOrAttach<TransformNode>().setPosition ({ 0.1f, 0.0f, 5.0f });
    expectConvergesAfterFourRenders ("camera");
}

TEST_F (SceneRendererMetalTests, ConvergedFrameStopsChanging)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    Image converged;
    for (int i = 0; i < 4; ++i)
        converged = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    const auto countChanged = [] (const Image& a, const Image& b)
    {
        int differing = 0;
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                differing += a.getPixelColor (x, y) != b.getPixelColor (x, y) ? 1 : 0;

        return differing;
    };

    // Further frames keep the history as it is, whatever the jitter. Each frame is checked against
    // the converged one and the one before, so a failure tells a restart from a one-frame glitch
    auto previous = converged;
    for (int i = 0; i < 3; ++i)
    {
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        EXPECT_TRUE (renderer.isConverged()) << "frame " << i << ": the history restarted";
        EXPECT_EQ (countChanged (image, converged), 0) << "frame " << i << ": pixels differing from the converged frame";
        EXPECT_EQ (countChanged (image, previous), 0) << "frame " << i << ": pixels differing from the previous frame";

        previous = image;
    }
}

TEST_F (SceneRendererMetalTests, SignedZeroAndNanPayloadsAreNoChange)
{
    Scene scene;
    buildEdgesScene (scene);

    // No normal texture: the normal scale is never read, so a NaN there renders nothing wrong
    auto material = Material::Ptr (new Material());
    material->metallicFactor = 0.0f;
    material->normalScale = std::bit_cast<float> (0x7fc00000u);
    scene.getRoot().findChild ("plates").attach<MaterialNode> (material);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 2);
    for (int i = 0; i < 2; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    material->metallicFactor = -0.0f;
    material->normalScale = std::bit_cast<float> (0xffc00001u);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, SampleCountChangesKeepTheFullTierConverged)
{
    Scene scene;
    buildEdgesScene (scene);

    // The full tier renders one sample per pixel whatever the count, so its accumulation is kept,
    // and so is the texture it renders into
    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    GpuTexture::Ptr output;
    for (int i = 0; i < 4; ++i)
        ASSERT_NE (output = renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    renderer.setSampleCount (1);
    EXPECT_EQ (renderer.render (device, scene, 64, 64), output) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    // The raster tier recreates its multisampled targets for the new count
    setTier (renderer, RenderSettings::TierPreference::raster);
    renderer.setSampleCount (4);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    renderer.setSampleCount (1);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    renderer.setSampleCount (4);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
}

TEST_F (SceneRendererMetalTests, TaaSmoothsEdges)
{
    const auto edgeLevels = [this] (int taaSamples)
    {
        Scene scene;
        buildEdgesScene (scene);

        SceneRenderer renderer;
        setFullTierTaa (renderer, taaSamples);
        Image image;
        for (int i = 0; i < jmax (1, taaSamples); ++i)
            image = renderToImage (renderer, scene, 128);

        EXPECT_TRUE (renderer.isConverged()) << taaSamples;
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return countEdgeLevels (image);
    };

    const auto antialiased = edgeLevels (16);
    const auto aliased = edgeLevels (1);
    RecordProperty ("antialiasedLevels", static_cast<int> (antialiased));
    RecordProperty ("aliasedLevels", static_cast<int> (aliased));

    EXPECT_GE (antialiased, 4u);
    EXPECT_LE (aliased, 2u);
}

TEST_F (SceneRendererMetalTests, MovingObjectKeepsStillPartsAntialiased)
{
    Scene scene;
    buildEdgesScene (scene);

    // A small quad spinning far below row 40, whose transform changes on every frame
    std::vector<Mesh::Vertex> vertices (4);
    vertices[0].position = { -0.15f, -0.15f, 0.0f };
    vertices[1].position = { 0.15f, -0.15f, 0.0f };
    vertices[2].position = { 0.15f, 0.15f, 0.0f };
    vertices[3].position = { -0.15f, 0.15f, 0.0f };
    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    auto mesh = Mesh::Ptr (new Mesh ("spinner"));
    mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 });
    auto spinner = scene.createEntity ("spinner");
    spinner.attach<MeshNode> (mesh);
    spinner.getOrAttach<TransformNode>().setPosition ({ -1.6f, -1.6f, 0.0f });

    SceneRenderer renderer;
    setFullTierTaa (renderer, 16);
    Image image;
    for (int i = 0; i < 24; ++i)
    {
        spinner.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.2f * static_cast<float> (i)));
        image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    }

    // Moving things never converge, but the still plate keeps accumulating
    EXPECT_FALSE (renderer.isConverged());

    const auto levels = countEdgeLevels (image);
    RecordProperty ("levelsWithMovingObject", static_cast<int> (levels));
    EXPECT_GE (levels, 4u);
}

TEST_F (SceneRendererMetalTests, RedrawnTextureRestartsAndShows)
{
    Scene scene;
    buildEdgesScene (scene);

    const auto upload = [] (const GpuTexture::Ptr& texture, uint8 r, uint8 g, uint8 b)
    {
        std::vector<uint8> texels;
        for (int i = 0; i < 4; ++i)
            texels.insert (texels.end(), { r, g, b, 255 });

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data);
    };

    auto gpuTexture = GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm));
    ASSERT_NE (gpuTexture, nullptr);
    ASSERT_TRUE (upload (gpuTexture, 255, 255, 255));

    auto texture = Texture::Ptr (new Texture (gpuTexture));
    auto material = Material::Ptr (new Material());
    material->baseColorTexture = texture;
    scene.getRoot().findChild ("plates").attach<MaterialNode> (material);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    Image image;
    for (int i = 0; i < 4; ++i)
        image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (renderer.isConverged()) << renderer.getLastError();
    EXPECT_NEAR (image.getPixelColor (32, 32).getRed(), image.getPixelColor (32, 32).getGreen(), 4);

    // Redrawn in place: the same GpuTexture handed over again
    ASSERT_TRUE (upload (gpuTexture, 255, 0, 0));
    texture->setGpuTexture (gpuTexture);

    image = renderToImage (renderer, scene, 64);
    EXPECT_FALSE (renderer.isConverged());

    for (int i = 0; i < 4 && ! renderer.isConverged(); ++i)
        image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (image.getPixelColor (32, 32).getRed(), image.getPixelColor (32, 32).getGreen() + 60);
}

TEST_F (SceneRendererMetalTests, RestartAccumulationStartsOver)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    renderer.restartAccumulation(); // Nothing rendered yet: nothing to restart

    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    renderer.restartAccumulation();
    EXPECT_FALSE (renderer.isConverged());

    for (int i = 1; i <= 4; ++i)
    {
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.isConverged(), i == 4) << "render " << i;
    }
}

// RenderSettings::debugView on 64 x 64 frames of the full tier
class SceneRendererDebugViewTests : public SceneRendererMetalTests
{
protected:
    static void setDebugView (SceneRenderer& renderer, RenderSettings::DebugView debugView)
    {
        auto settings = renderer.getRenderSettings();
        settings.debugView = debugView;
        renderer.setRenderSettings (settings);
    }

    /** The pixels of @p image with any color channel above 0. */
    static int countNonBlackPixels (const Image& image)
    {
        int count = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto color = image.getPixelColor (x, y);
                count += color.getRed() > 0 || color.getGreen() > 0 || color.getBlue() > 0 ? 1 : 0;
            }
        }

        return count;
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Shows @p view at an exposure of 0.25, which keeps it below saturation, and checks that each
        pixel shows the texel of @p output covering it as clamp (rgb * @p weight * 0.25). Both the
        render and the expected image round the same float expression to 8 bits, so they agree
        within one level. At least @p minLit pixels must be lit, and @p minPartial texels must have
        a confidence or share below 0.9 with some light, so a wrong weight shows.
    */
    void checkExposedPerTexel (Scene& scene, SceneRenderer& renderer, RenderSettings::DebugView view, SceneRenderer::PassOutput output, const String& weight, int minLit, int minPartial)
    {
        scene.getRoot().attach<ExposureNode> (0.25f);
        setDebugView (renderer, view);

        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        const auto image = renderToImage (renderer, scene, 64);
        const auto source = renderer.getPassOutput (output);
        ASSERT_TRUE (image.isValid());
        ASSERT_NE (source, nullptr);

        const auto expected = mapTexels (source, "vec4(clamp(t.rgb * " + weight + " * 0.25, 0.0, 1.0), 1.0)");
        const auto partial = mapTexels (source, "vec4(t.a < 0.9 && max(t.r, max(t.g, t.b)) * 0.25 > 0.04 ? 1.0 : 0.0, 0.0, 0.0, 1.0)");
        ASSERT_TRUE (expected.isValid() && partial.isValid());
        const auto scale = image.getWidth() / expected.getWidth();

        auto differing = 0, lit = 0, partialTexels = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto shown = image.getPixelColor (x, y);
                const auto wanted = expected.getPixelColor (x / scale, y / scale);
                differing += std::abs (shown.getRed() - wanted.getRed()) > 1 || std::abs (shown.getGreen() - wanted.getGreen()) > 1
                                     || std::abs (shown.getBlue() - wanted.getBlue()) > 1 ? 1 : 0;
                lit += wanted.getRed() > 16 ? 1 : 0;
            }
        }

        for (int y = 0; y < partial.getHeight(); ++y)
            for (int x = 0; x < partial.getWidth(); ++x)
                partialTexels += partial.getPixelColor (x, y).getRed() > 127 ? 1 : 0;

        RecordProperty ("litPixels", lit);
        RecordProperty ("partialTexels", partialTexels);
        EXPECT_EQ (differing, 0) << weight;
        EXPECT_GT (lit, minLit) << weight;
        EXPECT_GT (partialTexels, minPartial) << weight;
    }
#endif
};

TEST_F (SceneRendererDebugViewTests, NoneCreatesNoPassAndReleasesItsTargets)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    for (int i = 0; i < 2; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (renderer.getDebugViewPass(), nullptr);

    setDebugView (renderer, RenderSettings::DebugView::reflections);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    const auto* pass = renderer.getDebugViewPass();
    ASSERT_NE (pass, nullptr);
    EXPECT_GT (pass->getNumCachedTargets(), 0u);

    // Kept, with no target, so showing a view again compiles nothing
    setDebugView (renderer, RenderSettings::DebugView::none);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getDebugViewPass(), pass);
    EXPECT_EQ (pass->getNumCachedTargets(), 0u);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererDebugViewTests, ChangingTheViewKeepsTheAccumulation)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    ASSERT_TRUE (renderConverged (renderer, scene, 64).isValid());
    ASSERT_TRUE (renderer.isConverged());

    setDebugView (renderer, RenderSettings::DebugView::reflections);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    setDebugView (renderer, RenderSettings::DebugView::none);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererDebugViewTests, ReflectionsShowBlackWithoutThemAndNeverStale)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);

    // The first frame has no history to trace the reflections from
    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    setDebugView (renderer, RenderSettings::DebugView::reflections);
    const auto first = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (first.isValid());
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    EXPECT_EQ (countNonBlackPixels (first), 0);

    // The floor reflects the red emitter
    const auto traced = renderToImage (renderer, scene, 64);
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    const auto floor = meanColor (traced, reflectedEmitterRegion());
    RecordProperty ("floorRed", static_cast<int> (floor.getRed()));
    EXPECT_GT (floor.getRed(), 200); // Measured 255
    EXPECT_LE (floor.getGreen(), 2);

    // Turned off, the view shows black, not the reflections of the frame before
    setScreenSpaceReflections (renderer, false);
    const auto off = renderToImage (renderer, scene, 64);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    EXPECT_EQ (countNonBlackPixels (off), 0);
}

TEST_F (SceneRendererDebugViewTests, IndirectDiffuseShowsTheGatheredLight)
{
    Scene scene;
    buildGIRedRoomScene (scene);

    SceneRenderer renderer;
    setScreenSpaceGI (renderer, true);
    setDebugView (renderer, RenderSettings::DebugView::indirectDiffuse);
    const auto first = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (first.isValid());
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::indirectDiffuse), nullptr);
    EXPECT_EQ (countNonBlackPixels (first), 0);

    // The red wall bounces red onto the floor at its foot
    const auto gathered = renderToImage (renderer, scene, 64);
    const auto indirect = renderer.getPassOutput (SceneRenderer::PassOutput::indirectDiffuse);
    ASSERT_NE (indirect, nullptr);
    EXPECT_EQ (indirect, renderer.getAmbientOcclusionPass()->getIndirectDiffuse());
    EXPECT_EQ (indirect->getWidth(), 32u);

    auto redder = 0;
    for (int y = 0; y < gathered.getHeight(); ++y)
        for (int x = 0; x < gathered.getWidth(); ++x)
            redder += gathered.getPixelColor (x, y).getRed() > gathered.getPixelColor (x, y).getGreen() + 8 ? 1 : 0;

    RecordProperty ("redderPixels", redder);
    EXPECT_GT (redder, 400); // Measured 572 of 4096

    auto settings = renderer.getRenderSettings();
    settings.screenSpaceGI = false;
    renderer.setRenderSettings (settings);
    const auto off = renderToImage (renderer, scene, 64);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::indirectDiffuse), nullptr);
    EXPECT_EQ (countNonBlackPixels (off), 0);
}

TEST_F (SceneRendererDebugViewTests, RasterIgnoresIt)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);

    SceneRenderer renderer;
    auto settings = renderer.getRenderSettings();
    settings.tier = RenderSettings::TierPreference::raster;
    settings.ambientOcclusion = true;
    renderer.setRenderSettings (settings);
    const auto finalImage = renderToImage (renderer, scene, 64);

    setDebugView (renderer, RenderSettings::DebugView::ambientOcclusion);
    const auto withView = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (finalImage.isValid() && withView.isValid());
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion), nullptr);
    EXPECT_EQ (renderer.getDebugViewPass(), nullptr);
    EXPECT_EQ (countDifferencesAboveOneLevel (withView, finalImage), 0); // Two raster frames of this scene differ by up to one level
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererDebugViewTests, AmbientOcclusionShowsTheVisibilityAsGray)
{
    // Each pixel shows the visibility of the half resolution texel covering it, unfiltered
    Scene scene;
    buildGIRedRoomScene (scene);

    SceneRenderer renderer;
    setScreenSpaceGI (renderer, false);
    setDebugView (renderer, RenderSettings::DebugView::ambientOcclusion);
    const auto image = renderToImage (renderer, scene, 64);
    const auto ambientOcclusion = renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion);
    ASSERT_TRUE (image.isValid());
    ASSERT_NE (ambientOcclusion, nullptr);
    ASSERT_EQ (ambientOcclusion->getWidth(), 32u);

    const auto visibility = mapTexels (ambientOcclusion, "vec4(t.aaa, 1.0)");
    ASSERT_TRUE (visibility.isValid());

    auto differing = 0, occluded = 0;
    for (int y = 0; y < image.getHeight(); ++y)
    {
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto shown = image.getPixelColor (x, y);
            const auto expected = visibility.getPixelColor (x / 2, y / 2).getRed();
            differing += std::abs (shown.getRed() - expected) > 1 || shown.getGreen() != shown.getRed() || shown.getBlue() != shown.getRed() ? 1 : 0;
            occluded += expected < 200 ? 1 : 0;
        }
    }

    RecordProperty ("occludedPixels", occluded);
    EXPECT_EQ (differing, 0);
    EXPECT_GT (occluded, 80); // Measured 120: the comparison is not of a white image
}

TEST_F (SceneRendererDebugViewTests, UpscaledFrameShowsTheWholeSmallerTexture)
{
    // The ambient occlusion of a frame shaded at half size is a quarter of the view's: each of its
    // texels covers 4 x 4 pixels of the view, all the way to its right and bottom edges
    Scene scene;
    buildGIRedRoomScene (scene);

    SceneRenderer renderer;
    setScreenSpaceGI (renderer, false);
    setDebugView (renderer, RenderSettings::DebugView::ambientOcclusion);

    auto settings = renderer.getRenderSettings();
    settings.upscaling = RenderSettings::Upscaling::temporal;
    settings.renderScale = 0.5f;
    renderer.setRenderSettings (settings);

    const auto image = renderToImage (renderer, scene, 64);
    const auto ambientOcclusion = renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion);
    ASSERT_TRUE (image.isValid());
    ASSERT_NE (ambientOcclusion, nullptr);
    ASSERT_EQ (ambientOcclusion->getWidth(), 16u);

    const auto visibility = mapTexels (ambientOcclusion, "vec4(t.aaa, 1.0)");
    ASSERT_TRUE (visibility.isValid());

    auto differing = 0, occludedRight = 0;
    for (int y = 0; y < image.getHeight(); ++y)
    {
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto expected = visibility.getPixelColor (x / 4, y / 4).getRed();
            differing += std::abs (image.getPixelColor (x, y).getRed() - expected) > 1 ? 1 : 0;
            occludedRight += x >= 32 && expected < 200 ? 1 : 0;
        }
    }

    RecordProperty ("occludedRightPixels", occludedRight);
    EXPECT_EQ (differing, 0);
    EXPECT_GT (occludedRight, 16); // The right half is not white, so it shows where the texels land
}

TEST_F (SceneRendererDebugViewTests, ReflectionsAreExposedAndWeightedByTheirConfidence)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    checkExposedPerTexel (scene, renderer, RenderSettings::DebugView::reflections, SceneRenderer::PassOutput::reflections, "t.a", 250, 30); // Measured 382 and 58
}

TEST_F (SceneRendererDebugViewTests, IndirectDiffuseIsExposedAsGathered)
{
    // Not weighted by a: the gathered rgb already carries the share, as the composite adds it
    Scene scene;
    buildGIRedRoomScene (scene);

    SceneRenderer renderer;
    setScreenSpaceGI (renderer, true);
    checkExposedPerTexel (scene, renderer, RenderSettings::DebugView::indirectDiffuse, SceneRenderer::PassOutput::indirectDiffuse, "1.0", 250, 50); // Measured 384 and 96
}
#endif

TEST_F (SceneRendererDebugViewTests, CircleOfConfusionShowsNearAndFarInTwoColorsAndFocusBlack)
{
    // Focused on the bars at 0.5, the square at 0.25 in front blurs as much as the background
    // behind: both reach 0.8 of the largest CoC (8 px), the bars 0
    constexpr auto size = depthOfFieldSize;

    Scene scene;
    setUpDepthOfFieldScene (scene, 0.5f);
    buildBarsScene (scene, 0.5f, 0.25f);
    ASSERT_NEAR (circleOfConfusion (scene, 0.25f), -6.5f, 0.1f);
    ASSERT_NEAR (circleOfConfusion (scene, 1000.0f), 6.5f, 0.1f);

    SceneRenderer renderer;
    setDepthOfFieldTier (renderer, RenderSettings::TierPreference::full);
    setDepthOfField (renderer, true, 8.0f);
    setDebugView (renderer, RenderSettings::DebugView::circleOfConfusion);
    const auto image = renderToImage (renderer, scene, size);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion), nullptr);

    const auto near = meanColor (image, { 56, 56, 16, 16 });
    const auto far = meanColor (image, { 36, 12, 10, 30 });
    RecordProperty ("nearRed", static_cast<int> (near.getRed()));
    RecordProperty ("farBlue", static_cast<int> (far.getBlue()));
    // The square root of 0.81 (1.625 of 2 texels), 0.9 of 255: without the scale both would saturate at 255
    EXPECT_NEAR (near.getRed(), 230, 12);
    EXPECT_NEAR (near.getGreen(), 115, 12);
    EXPECT_LE (near.getBlue(), 2);
    EXPECT_NEAR (far.getBlue(), 230, 12);
    EXPECT_NEAR (far.getGreen(), 115, 12);
    EXPECT_LE (far.getRed(), 2);

    auto focusedLevel = 0;
    for (int y = 12; y < 116; ++y)
    {
        for (int x = 12; x < 24; ++x)
        {
            const auto color = image.getPixelColor (x, y);
            focusedLevel = jmax (focusedLevel, static_cast<int> (color.getRed()), static_cast<int> (color.getGreen()), static_cast<int> (color.getBlue()));
        }
    }

    RecordProperty ("focusedLevel", focusedLevel);
    EXPECT_LE (focusedLevel, 1);

#if YUP_ENABLE_SHADER_TRANSPILER
    // Each pixel shows the CoC texel covering it as the square root of its share of the largest CoC, 2 texels of the half size
    const auto* depthOfField = renderer.getDepthOfField();
    ASSERT_NE (depthOfField, nullptr);
    ASSERT_FLOAT_EQ (depthOfField->getMaxCircleOfConfusion(), 2.0f);

    const auto expected = mapTexels (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion),
                                     "vec4((t.r < 0.0 ? vec3(1.0, 0.5, 0.0) : vec3(0.0, 0.5, 1.0)) * sqrt(clamp(abs(t.r) * 0.5, 0.0, 1.0)), 1.0)");
    ASSERT_TRUE (expected.isValid());
    ASSERT_EQ (expected.getWidth(), size / 2);

    auto differing = 0;
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const auto shown = image.getPixelColor (x, y);
            const auto wanted = expected.getPixelColor (x / 2, y / 2);
            differing += std::abs (shown.getRed() - wanted.getRed()) > 1 || std::abs (shown.getGreen() - wanted.getGreen()) > 1
                                 || std::abs (shown.getBlue() - wanted.getBlue()) > 1 ? 1 : 0;
        }
    }

    EXPECT_EQ (differing, 0);
#endif
}

TEST_F (SceneRendererDebugViewTests, CircleOfConfusionShowsBlackWithoutDepthOfFieldAndNeverStale)
{
    Scene scene;
    auto& camera = setUpDepthOfFieldScene (scene, 0.5f);
    buildBarsScene (scene, 0.5f, 0.25f);

    SceneRenderer renderer;
    setDepthOfFieldTier (renderer, RenderSettings::TierPreference::full);
    setDepthOfField (renderer, false);
    setDebugView (renderer, RenderSettings::DebugView::circleOfConfusion);
    const auto off = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (off.isValid()) << renderer.getLastError();
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion), nullptr);
    EXPECT_EQ (countNonBlackPixels (off), 0);

    setDepthOfField (renderer, true);
    const auto on = renderToImage (renderer, scene, 64);
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion), nullptr);
    EXPECT_GT (countNonBlackPixels (on), 1000);

    // A camera that cannot blur draws no depth of field: black, not the CoC of the frame before
    camera.fStop = 0.0f;
    const auto cannotBlur = renderToImage (renderer, scene, 64);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion), nullptr);
    EXPECT_EQ (countNonBlackPixels (cannotBlur), 0);
}

TEST_F (SceneRendererMetalTests, SilhouetteEdgesMatchRasterOnBothTiers)
{
    Scene scene;
    buildSilhouettesScene (scene);

    // The reference: raster with bloom off, tone mapped per sample and resolved in display space
    SceneRenderer rasterRenderer;
    setTier (rasterRenderer, RenderSettings::TierPreference::raster);
    const auto raster = renderToImage (rasterRenderer, scene, 128);
    ASSERT_TRUE (raster.isValid()) << rasterRenderer.getLastError();

    // Raster with bloom at zero intensity: the half-float path through the same samples
    SceneRenderer bloomRenderer;
    auto settings = rasterRenderer.getRenderSettings();
    settings.bloom = true;
    settings.bloomIntensity = 0.0f;
    bloomRenderer.setRenderSettings (settings);
    const auto bloom = renderToImage (bloomRenderer, scene, 128);
    ASSERT_TRUE (bloom.isValid()) << bloomRenderer.getLastError();
    ASSERT_EQ (bloomRenderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float);

    // The full tier, converged: its jittered frames estimate each pixel's coverage differently

    SceneRenderer fullRenderer;
    fullRenderer.setRenderSettings (bloomRenderer.getRenderSettings());
    setBloom (fullRenderer, false);
    setFullTierTaa (fullRenderer, 16);
    Image full;
    for (int i = 0; i < 16; ++i)
        full = renderToImage (fullRenderer, scene, 128);
    ASSERT_TRUE (full.isValid()) << fullRenderer.getLastError();
    ASSERT_TRUE (fullRenderer.isConverged());

    // The same samples as the reference: every pixel matches
    const auto bloomDiffering = countDifferences (bloom, raster);
    RecordProperty ("bloomDiffering", bloomDiffering);
    EXPECT_EQ (bloomDiffering, 0);

    // Other samples: the mean over the silhouettes, where the coverage averages out. Each pixel
    // rounds to its own 8 bit level on each path, so the means may differ by a fraction of a level;
    // tone mapping the premultiplied color instead moved them by 2.3 to 10.8 levels
    for (const auto& [name, left, right] : { std::tuple { "bright", 0, 64 }, std::tuple { "gray", 64, 128 } })
    {
        const auto reference = silhouetteBandLuminance (raster, raster, left, right);
        const auto fullBand = silhouetteBandLuminance (full, raster, left, right);
        const auto bloomBand = silhouetteBandLuminance (bloom, raster, left, right);
        RecordProperty ((String (name) + "Raster").toStdString(), String (reference, 2).toStdString());
        RecordProperty ((String (name) + "Full").toStdString(), String (fullBand, 2).toStdString());
        RecordProperty ((String (name) + "RasterBloom").toStdString(), String (bloomBand, 2).toStdString());

        EXPECT_NEAR (fullBand, reference, 1.0) << name;
        EXPECT_NEAR (bloomBand, reference, 1.0) << name;
    }
}

TEST_F (SceneRendererMetalTests, FullRasterFullRoundTripStartsOver)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer reference;
    setFullTierTaa (reference, 8);
    Image converged;
    for (int i = 0; i < 8; ++i)
        converged = renderToImage (reference, scene, 128);
    ASSERT_TRUE (converged.isValid()) << reference.getLastError();
    ASSERT_TRUE (reference.isConverged());

    SceneRenderer renderer;
    renderer.setRenderSettings (reference.getRenderSettings());
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    setTier (renderer, RenderSettings::TierPreference::raster);
    ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_EQ (renderer.getCapabilities()->getTier(), RenderCapabilities::Tier::raster);

    // Back on the full tier the history is gone: the first frame stands alone, so no pixel of its
    // silhouettes is partially covered yet
    setTier (renderer, RenderSettings::TierPreference::full);
    auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_EQ (countIntermediatePixels (image, 40), 0);

    for (int i = 2; i <= 8; ++i)
    {
        image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.isConverged(), i == 8) << "render " << i;
    }

    // The jitter sequence continued through the raster frame, so edges may differ by a level or two
    const auto differing = countDifferences (image, converged);
    RecordProperty ("differingFromAFreshRenderer", differing);
    EXPECT_LE (differing, 16);
    EXPECT_GE (countIntermediatePixels (image, 40), 4);
}

TEST_F (SceneRendererMetalTests, ToneMappingChangeKeepsTheFullTierConverged)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    Image converged;
    for (int i = 0; i < 8; ++i)
        converged = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Tone mapping runs after the accumulation: the converged history shows through the new operator
    scene.getRoot().attach<ToneMappingNode> (ToneMappingOperator::aces);
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (countDifferences (image, converged), 1000);
    EXPECT_GE (countIntermediatePixels (image, 40), 4);
}

TEST_F (SceneRendererMetalTests, ExposureAndLightChangesRestartButKeepTheHistory)
{
    // Each change sets a value, applied to the edges scene converged with 8 TAA samples
    const std::pair<const char*, std::function<void (Scene&, float)>> changes[] = {
        { "exposure", [] (Scene& scene, float value) { scene.getRoot().attach<ExposureNode> (value); } },
        { "light", [] (Scene& scene, float value) { scene.getRoot().findChild ("sun").getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, value)); } },
    };

    const std::map<String, std::pair<float, float>> values { { "exposure", { 1.0f, 1.5f } }, { "light", { 0.0f, 0.4f } } };

    // Converges at @p before, changes to @p after and renders once, then until converged again
    const auto renderChange = [&] (const std::function<void (Scene&, float)>& change, float before, float after, bool restart, Image& first)
    {
        Scene scene;
        buildEdgesScene (scene);
        change (scene, before);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 8);
        for (int i = 0; i < 8; ++i)
            EXPECT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged());

        change (scene, after);
        if (restart)
            renderer.restartAccumulation();

        first = renderToImage (renderer, scene, 128);
        EXPECT_FALSE (renderer.isConverged());

        Image converged;
        for (int i = 0; i < 8 && ! renderer.isConverged(); ++i)
            converged = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (renderer.isConverged());
        return converged;
    };

    for (const auto& [name, change] : changes)
    {
        const auto [before, after] = values.at (name);

        // The signal is the first frame after the change. Blended into the kept history, its
        // silhouettes still have partially covered pixels. Started over from that frame alone, as
        // restartAccumulation() does, every pixel is either covered or not
        Image keptFirst, restartedFirst;
        const auto converged = renderChange (change, before, after, false, keptFirst);
        renderChange (change, before, after, true, restartedFirst);
        ASSERT_TRUE (keptFirst.isValid() && restartedFirst.isValid() && converged.isValid()) << name;

        RecordProperty ((name + String ("Kept")).toStdString(), countIntermediatePixels (keptFirst, 40));
        RecordProperty ((name + String ("Restarted")).toStdString(), countIntermediatePixels (restartedFirst, 40));
        EXPECT_GE (countIntermediatePixels (keptFirst, 40), 4) << name;
        EXPECT_EQ (countIntermediatePixels (restartedFirst, 40), 0) << name;

        // Converged again, what remains of the image before the change. The control goes through
        // the same frames and the same restart, from a value too close to tell apart, so only the
        // remains of the change differ. Counting the history as one sample once the scene is still
        // leaves about 1/8 of the change on edge pixels, where the clip box is wide
        Image controlFirst;
        const auto control = renderChange (change, after + 1.0e-4f, after, false, controlFirst);
        ASSERT_TRUE (control.isValid()) << name;

        int largest = 0;
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 128; ++x)
                largest = jmax (largest, std::abs (static_cast<int> (converged.getPixelColor (x, y).getRed()) - static_cast<int> (control.getPixelColor (x, y).getRed())));

        // Keeping the history's blend weight instead left 236 pixels beyond 2 levels, up to 7
        const auto differing = countDifferences (converged, control);
        RecordProperty ((name + String ("DifferingFromControl")).toStdString(), differing);
        RecordProperty ((name + String ("LargestDifference")).toStdString(), largest);
        EXPECT_LE (differing, 16) << name;
        EXPECT_LE (largest, 4) << name;
    }
}

TEST_F (SceneRendererMetalTests, UncoveredPixelsShowTheBackgroundExactly)
{
    // The plate moves right: the pixels it leaves keep no trace of it, because the history
    // coverage is clamped to the coverage around them
    const auto background = Color (static_cast<uint8> (200), static_cast<uint8> (80), static_cast<uint8> (40));
    const auto isBackground = [&] (Color color)
    {
        return std::abs (color.getRed() - background.getRed()) <= 1
            && std::abs (color.getGreen() - background.getGreen()) <= 1
            && std::abs (color.getBlue() - background.getBlue()) <= 1;
    };

    Scene scene;
    buildEdgesScene (scene);
    scene.getRoot().attach<BackgroundNode> (background);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    Image image;
    for (int i = 0; i < 4; ++i)
        image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (renderer.isConverged()) << renderer.getLastError();

    // Just inside the left silhouette on row 64
    EXPECT_FALSE (isBackground (image.getPixelColor (34, 64)));

    scene.getRoot().findChild ("plates").getOrAttach<TransformNode>().setPosition ({ 0.5f, 0.0f, 0.0f });

    for (int frame = 1; frame <= 6; ++frame)
    {
        image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto color = image.getPixelColor (34, 64);
        EXPECT_TRUE (isBackground (color)) << "frame " << frame << ": " << static_cast<int> (color.getRed()) << ","
                                           << static_cast<int> (color.getGreen()) << "," << static_cast<int> (color.getBlue());
    }

    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, CameraMotionReorderingBlendedItemsKeepsTheHistory)
{
    Scene scene;
    buildEdgesScene (scene);

    // Two blended quads below row 40, 0.005 apart in depth: turning the camera by 0.01 swaps
    // which one is farther, and so their drawing order
    auto blended = Material::Ptr (new Material());
    blended->baseColorFactor = { 0.2f, 0.2f, 0.9f, 0.5f };
    blended->alphaMode = Material::AlphaMode::blend;

    for (const auto& [x, z] : { std::pair { -0.5f, 0.0f }, std::pair { 0.5f, -0.005f } })
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.2f, -0.2f, 0.0f };
        vertices[1].position = { 0.2f, -0.2f, 0.0f };
        vertices[2].position = { 0.2f, 0.2f, 0.0f };
        vertices[3].position = { -0.2f, 0.2f, 0.0f };
        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("blended"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, blended);
        auto quad = scene.createEntity ("blended");
        quad.attach<MeshNode> (mesh);
        quad.getOrAttach<TransformNode>().setPosition ({ x, -0.7f, 0.6f + z });
    }

    auto camera = scene.createEntity ("camera");
    camera.attach<CameraNode>();
    camera.getOrAttach<TransformNode>().setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.setActiveCamera (camera);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 16);
    for (int i = 0; i < 16; ++i)
        ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    camera.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.01f));
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    const auto levels = countEdgeLevels (image);
    RecordProperty ("levelsAfterReorder", static_cast<int> (levels));
    EXPECT_GE (levels, 4u);
}

TEST_F (SceneRendererMetalTests, CameraMotionKeepsTheHistory)
{
    Scene scene;
    buildEdgesScene (scene);

    auto camera = scene.createEntity ("camera");
    camera.attach<CameraNode>();
    camera.getOrAttach<TransformNode>().setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.setActiveCamera (camera);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 16);
    for (int i = 0; i < 16; ++i)
        ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // The first frame after a small camera move blends into the reprojected history, so the edges
    // stay antialiased; starting over from that frame alone would alias them
    camera.getOrAttach<TransformNode>().setPosition ({ 0.01f, 0.0f, 5.0f });
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());

    const auto levels = countEdgeLevels (image);
    RecordProperty ("levelsAfterCameraMove", static_cast<int> (levels));
    EXPECT_GE (levels, 4u);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, NonFiniteCheckFindsNanAndInfinity)
{
    // Half floats: 1.0, NaN and +infinity
    const auto check = [this] (uint16 bits)
    {
        std::vector<uint16> texels (4 * 4 * 4, 0x3c00);
        texels[5 * 4 + 1] = bits;

        auto texture = GpuTexture::create (device, GpuTextureDesc (4, 4, GpuTextureFormat::rgba16float));
        if (texture == nullptr)
            return false;

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data) && isFiniteTexture (texture);
    };

    EXPECT_TRUE (check (0x3c00));
    EXPECT_FALSE (check (0x7e00));
    EXPECT_FALSE (check (0x7c00));
}

TEST_F (SceneRendererMetalTests, VelocityIsZeroForAStaticSceneAndCameraMotionOtherwise)
{
    Scene scene;
    buildShadowsScene (scene);

    auto camera = scene.createEntity ("camera");
    camera.attach<CameraNode>();
    camera.getOrAttach<TransformNode>().setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.setActiveCamera (camera);

    SceneRenderer renderer;
    setTier (renderer, RenderSettings::TierPreference::full);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto still = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (still.isValid());
    EXPECT_EQ (maxVelocityDeviation (still), 0);

    // Velocity is current uv minus previous uv, with uv from the top-left. Moving the camera right
    // moves the scene left on screen: x decreases, y is unchanged
    camera.getOrAttach<TransformNode>().setPosition ({ 0.1f, 0.0f, 5.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto right = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (right.isValid());
    EXPECT_LT (right.getPixelColor (32, 32).getRed(), 64);
    EXPECT_EQ (velocityDeviation (right.getPixelColor (32, 32).getGreen()), 0);

    // Moving the camera up moves the scene down on screen, towards larger uv.y: a missing y flip fails here
    camera.getOrAttach<TransformNode>().setPosition ({ 0.1f, 0.1f, 5.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto up = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (up.isValid());
    EXPECT_GT (up.getPixelColor (32, 32).getGreen(), 192);
    EXPECT_EQ (velocityDeviation (up.getPixelColor (32, 32).getRed()), 0);
}

TEST_F (SceneRendererMetalTests, InfiniteFarPlaneSkyMovesWithTheCameraRotation)
{
    // Turning a glTF camera without zfar (an infinite far plane) moves the sky on screen exactly as
    // it does through the same camera with a distant far plane
    const auto renderTurning = [this] (float zFar)
    {
        Scene scene;
        setEnvironmentMap (scene, new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.getRoot().get<EnvironmentNode>()->visible = true;

        auto camera = scene.createEntity ("camera");
        auto& node = camera.attach<CameraNode>();
        node.zNear = 0.05f;
        node.zFar = zFar;
        scene.setActiveCamera (camera);

        SceneRenderer renderer;
        setTier (renderer, RenderSettings::TierPreference::full);
        EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        camera.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle (Vector3<float> (0.3f, 1.0f, 0.0f).normalized(), 0.05f));
        EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        return readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 4.0f);
    };

    const auto infinite = renderTurning (0.0f);
    const auto finite = renderTurning (1.0e5f);
    ASSERT_TRUE (infinite.isValid());
    ASSERT_TRUE (finite.isValid());

    // A turn of 0.05 rad moves the sky by about 0.06 of the frame: not vacuous
    EXPECT_GT (velocityDeviation (finite.getPixelColor (32, 32).getRed()), 20);
    EXPECT_GT (velocityDeviation (infinite.getPixelColor (32, 32).getRed()), 20);
    EXPECT_EQ (countDifferences (infinite, finite), 0);
}

TEST_F (SceneRendererMetalTests, BlendedItemsKeepTheDeferredVelocityAroundThem)
{
    // The blended items draw over the G-buffer velocity: texels outside them keep the motion the
    // G-buffer wrote, which a cleared velocity attachment would zero
    Scene scene;
    buildShadowsScene (scene);

    std::vector<Mesh::Vertex> vertices (4);
    vertices[0].position = { -0.9f, 0.5f, 0.6f };
    vertices[1].position = { -0.5f, 0.5f, 0.6f };
    vertices[2].position = { -0.5f, 0.9f, 0.6f };
    vertices[3].position = { -0.9f, 0.9f, 0.6f };
    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 0.0f, 0.0f, 1.0f, 0.5f };
    material->alphaMode = Material::AlphaMode::blend;

    auto mesh = Mesh::Ptr (new Mesh ("blended"));
    mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);
    auto blended = scene.createEntity ("blended");
    blended.attach<MeshNode> (mesh);

    auto camera = scene.createEntity ("camera");
    camera.attach<CameraNode>();
    camera.getOrAttach<TransformNode>().setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.setActiveCamera (camera);

    SceneRenderer renderer;
    setTier (renderer, RenderSettings::TierPreference::full);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    // Moving the camera right moves the scene left on screen: x decreases everywhere
    camera.getOrAttach<TransformNode>().setPosition ({ 0.1f, 0.0f, 5.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto moved = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (moved.isValid());

    // The opaque plate at the center, away from the blended quad in the top-left corner, then the quad
    EXPECT_LT (moved.getPixelColor (40, 40).getRed(), 64);
    EXPECT_LT (moved.getPixelColor (20, 20).getRed(), 64);
}

TEST_F (SceneRendererMetalTests, TwoRenderersKeepIndependentHistory)
{
    Scene scene;
    buildShadowsScene (scene);

    auto plates = scene.getRoot().findChild ("plates");
    ASSERT_TRUE (plates.isValid());

    const auto velocityAfterMove = [&] (SceneRenderer& renderer, float x)
    {
        plates.getOrAttach<TransformNode>().setPosition ({ x, 0.0f, 0.0f });
        if (renderer.render (device, scene, 64, 64) == nullptr)
            return -1;

        const auto velocity = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
        return velocity.isValid() ? maxVelocityDeviation (velocity) : -1;
    };

    // A single renderer sees the plates move between its frames
    SceneRenderer single;
    setTier (single, RenderSettings::TierPreference::full);
    EXPECT_EQ (velocityAfterMove (single, 0.0f), 0);
    EXPECT_GT (velocityAfterMove (single, 0.1f), 64);

    // Alternating renderers never take the other's transform as their previous frame
    SceneRenderer a;
    SceneRenderer b;
    setTier (a, RenderSettings::TierPreference::full);
    setTier (b, RenderSettings::TierPreference::full);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ (velocityAfterMove (a, 0.1f * static_cast<float> (2 * i)), 0) << "a, frame " << i;
        EXPECT_EQ (velocityAfterMove (b, 0.1f * static_cast<float> (2 * i + 1)), 0) << "b, frame " << i;
    }
}

TEST_F (SceneRendererMetalTests, FirstFrameHasNoObjectMotion)
{
    // A transform set before the first render is not motion, whatever the node held before
    Scene scene;
    buildShadowsScene (scene);
    scene.getRoot().findChild ("plates").getOrAttach<TransformNode>().setPosition ({ 0.3f, 0.2f, 0.0f });

    SceneRenderer renderer;
    setTier (renderer, RenderSettings::TierPreference::full);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto velocity = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (velocity.isValid());
    EXPECT_EQ (maxVelocityDeviation (velocity), 0);
}

TEST_F (SceneRendererMetalTests, NodeReappearingMovedHasNoMotion)
{
    Scene scene;
    buildShadowsScene (scene);

    auto plates = scene.getRoot().findChild ("plates");
    ASSERT_TRUE (plates.isValid());

    SceneRenderer renderer;
    setTier (renderer, RenderSettings::TierPreference::full);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    // Hidden for one frame, then shown somewhere else: its last drawn transform is two frames old
    plates.setVisible (false);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    plates.setVisible (true);
    plates.getOrAttach<TransformNode>().setPosition ({ 0.1f, 0.0f, 0.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto velocity = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (velocity.isValid());
    EXPECT_EQ (maxVelocityDeviation (velocity), 0);
}

TEST_F (SceneRendererMetalTests, ResizeResetsAccumulation)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    ASSERT_NE (renderer.render (device, scene, 80, 80), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());

    const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
    ASSERT_NE (sceneColor, nullptr);
    EXPECT_EQ (sceneColor->getWidth(), 80);
    EXPECT_TRUE (isFiniteTexture (sceneColor));
}

/** Temporal upscaling: the scene shaded at RenderSettings::renderScale of the frame, reconstructed by TAA. */
class UpscalingMetalTests : public SceneRendererMetalTests
{
protected:
    static void setUpscaling (SceneRenderer& renderer, RenderSettings::Upscaling upscaling, float renderScale)
    {
        auto settings = renderer.getRenderSettings();
        settings.upscaling = upscaling;
        settings.renderScale = renderScale;
        renderer.setRenderSettings (settings);
    }

    /** Thin emissive bars on black, a little rotated, 1.5 to 2 pixels wide at 128 x 128: half
        their width at half that size, so one frame shaded at a scale of 0.5 cannot show them. */
    static void buildBarsScene (Scene& scene)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->metallicFactor = 0.0f;
        material->emissiveFactor = { 0.8f, 0.8f, 0.8f };

        auto mesh = Mesh::Ptr (new Mesh ("bars"));
        for (int i = 0; i < 12; ++i)
        {
            const auto x = -1.4f + 0.23f * static_cast<float> (i);
            const auto halfWidth = 0.022f + 0.004f * static_cast<float> (i % 3);

            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = { x - halfWidth, -1.5f, 0.0f };
            vertices[1].position = { x + halfWidth, -1.5f, 0.0f };
            vertices[2].position = { x + halfWidth, 1.5f, 0.0f };
            vertices[3].position = { x - halfWidth, 1.5f, 0.0f };

            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);
        }

        auto bars = scene.createEntity ("bars");
        bars.attach<MeshNode> (mesh);
        bars.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.12f));
    }

    /** The mean absolute difference of the red channels of two images of the same size, in levels. */
    static double meanRedDifference (const Image& a, const Image& b)
    {
        double sum = 0.0;
        for (int y = 0; y < a.getHeight(); ++y)
            for (int x = 0; x < a.getWidth(); ++x)
                sum += std::abs (static_cast<int> (a.getPixelColor (x, y).getRed()) - static_cast<int> (b.getPixelColor (x, y).getRed()));

        return sum / static_cast<double> (a.getWidth() * a.getHeight());
    }

    /** The mean red level of an image. */
    static double meanRed (const Image& image)
    {
        double sum = 0.0;
        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                sum += image.getPixelColor (x, y).getRed();

        return sum / static_cast<double> (image.getWidth() * image.getHeight());
    }

    /** The red channel of @p image bilinearly scaled up by 2, as a plain upscale of a frame shaded at half size would show it. */
    static std::vector<double> upscaleRedTwice (const Image& image)
    {
        const auto width = image.getWidth();
        const auto height = image.getHeight();
        const auto red = [&] (int x, int y) { return static_cast<double> (image.getPixelColor (jlimit (0, width - 1, x), jlimit (0, height - 1, y)).getRed()); };

        std::vector<double> result (static_cast<size_t> (width * height * 4));
        for (int y = 0; y < height * 2; ++y)
        {
            for (int x = 0; x < width * 2; ++x)
            {
                const auto sx = (x + 0.5) * 0.5 - 0.5;
                const auto sy = (y + 0.5) * 0.5 - 0.5;
                const auto x0 = static_cast<int> (std::floor (sx));
                const auto y0 = static_cast<int> (std::floor (sy));
                const auto fx = sx - x0;
                const auto fy = sy - y0;

                result[static_cast<size_t> (y * width * 2 + x)] = (red (x0, y0) * (1.0 - fx) + red (x0 + 1, y0) * fx) * (1.0 - fy)
                                                                + (red (x0, y0 + 1) * (1.0 - fx) + red (x0 + 1, y0 + 1) * fx) * fy;
            }
        }

        return result;
    }
};

TEST_F (UpscalingMetalTests, OffAndScaleOneAreUnchanged)
{
    const auto renderFrames = [this] (RenderSettings::Upscaling upscaling, float renderScale)
    {
        Scene scene;
        buildEdgesScene (scene);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 4);
        setUpscaling (renderer, upscaling, renderScale);
        std::vector<Image> images;
        for (int i = 0; i < 5; ++i)
            images.push_back (renderToImage (renderer, scene, 64));

        EXPECT_FALSE (renderer.getTemporalAA()->isUpscalingPrepared());
        return images;
    };

    // The first renderer of the device differs in its first frame, so it renders none of the compared ones
    renderFrames (RenderSettings::Upscaling::off, 0.5f);
    const auto today = renderFrames (RenderSettings::Upscaling::off, 0.5f);

    for (const auto renderScale : { 1.0f, 2.0f, std::numeric_limits<float>::quiet_NaN() })
    {
        const auto temporal = renderFrames (RenderSettings::Upscaling::temporal, renderScale);
        ASSERT_EQ (temporal.size(), today.size());

        for (size_t i = 0; i < today.size(); ++i)
        {
            ASSERT_TRUE (today[i].isValid() && temporal[i].isValid());
            EXPECT_EQ (meanRedDifference (today[i], temporal[i]), 0.0) << "scale " << renderScale << ", frame " << i;
        }
    }
}

TEST_F (UpscalingMetalTests, ShadesTheSceneSmallerAndReturnsTheFullSize)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    setUpscaling (renderer, RenderSettings::Upscaling::temporal, 0.5f);
    renderer.setFrameStatsEnabled (true);

    const auto output = renderer.render (device, scene, 100, 60);
    ASSERT_NE (output, nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.waitForGPU().wasOk());

    EXPECT_EQ (output->getWidth(), 100u);
    EXPECT_EQ (output->getHeight(), 60u);
    EXPECT_EQ (renderer.getLastFrameStats().internalWidth, 50);
    EXPECT_EQ (renderer.getLastFrameStats().internalHeight, 30);

    // Shaded at the internal size, reconstructed at the frame's
    for (const auto pass : { SceneRenderer::PassOutput::velocity, SceneRenderer::PassOutput::gbufferNormal })
    {
        const auto texture = renderer.getPassOutput (pass);
        ASSERT_NE (texture, nullptr);
        EXPECT_EQ (texture->getWidth(), 50u);
        EXPECT_EQ (texture->getHeight(), 30u);
    }

    const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
    ASSERT_NE (sceneColor, nullptr);
    EXPECT_EQ (sceneColor->getWidth(), 100u);
    EXPECT_EQ (sceneColor->getHeight(), 60u);
    EXPECT_TRUE (isFiniteTexture (sceneColor));
    EXPECT_TRUE (renderer.getTemporalAA()->isUpscalingPrepared());

    // The internal size is rounded, and never 0
    ASSERT_NE (renderer.render (device, scene, 75, 1), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.waitForGPU().wasOk());
    EXPECT_EQ (renderer.getLastFrameStats().internalWidth, 38);
    EXPECT_EQ (renderer.getLastFrameStats().internalHeight, 1);
}

TEST_F (UpscalingMetalTests, ConvergedImageApproachesTheFullSizeOne)
{
    const auto render = [this] (int size, bool upscaled)
    {
        Scene scene;
        buildBarsScene (scene);

        // 16 samples, over 64 frames at a quarter of the pixels
        SceneRenderer renderer;
        setFullTierTaa (renderer, 16);
        setUpscaling (renderer, upscaled ? RenderSettings::Upscaling::temporal : RenderSettings::Upscaling::off, 0.5f);
        auto image = renderConverged (renderer, scene, size);
        EXPECT_TRUE (renderer.isConverged()) << size << (upscaled ? " upscaled" : "");
        return image;
    };

    const auto native = render (128, false);
    const auto upscaled = render (128, true);
    const auto half = render (64, false);
    ASSERT_TRUE (native.isValid() && upscaled.isValid() && half.isValid());

    // A plain upscale of the frame shaded at half size: what the reconstruction must beat
    const auto bilinear = upscaleRedTwice (half);
    double bilinearDifference = 0.0;
    for (int y = 0; y < 128; ++y)
        for (int x = 0; x < 128; ++x)
            bilinearDifference += std::abs (bilinear[static_cast<size_t> (y * 128 + x)] - native.getPixelColor (x, y).getRed());

    bilinearDifference /= 128.0 * 128.0;
    const auto upscaledDifference = meanRedDifference (upscaled, native);

    RecordProperty ("bilinearDifference", String (bilinearDifference, 3).toStdString());
    RecordProperty ("upscaledDifference", String (upscaledDifference, 3).toStdString());
    RecordProperty ("nativeMean", String (meanRed (native), 3).toStdString());
    RecordProperty ("upscaledMean", String (meanRed (upscaled), 3).toStdString());

    // On Apple M2 Max: 1.7 levels against 11.9 for the plain upscale. A reconstruction filter a
    // little too wide (13.0) or a jitter of the wrong sign (18.6) cannot come near 0.3 of it
    EXPECT_LT (upscaledDifference, bilinearDifference * 0.3);
    EXPECT_NEAR (meanRed (upscaled), meanRed (native), meanRed (native) * 0.01);
}

TEST_F (UpscalingMetalTests, ScaleOrModeChangesRestartCleanly)
{
    Scene scene;
    buildEdgesScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    setUpscaling (renderer, RenderSettings::Upscaling::temporal, 0.5f);
    ASSERT_TRUE (renderConverged (renderer, scene, 64).isValid());
    ASSERT_TRUE (renderer.isConverged());

    const auto expectRestarted = [&] (const char* change)
    {
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << change;

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr) << change;
        EXPECT_EQ (sceneColor->getWidth(), 64u) << change;
        EXPECT_TRUE (isFiniteTexture (sceneColor)) << change;

        EXPECT_TRUE (renderConverged (renderer, scene, 64).isValid()) << change;
        EXPECT_TRUE (renderer.isConverged()) << change;
    };

    setUpscaling (renderer, RenderSettings::Upscaling::temporal, 0.75f);
    expectRestarted ("scale");

    setUpscaling (renderer, RenderSettings::Upscaling::off, 0.75f);
    expectRestarted ("off");

    setUpscaling (renderer, RenderSettings::Upscaling::temporal, 0.75f);
    expectRestarted ("on");
}

TEST_F (UpscalingMetalTests, ScreenSpaceReflectionsReadTheFullSizeHistory)
{
    // Traced at the shaded size, they read their radiance from the TAA history at the view's
    const auto render = [&] (bool screenSpaceReflections, SceneRenderer& renderer)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        setFullTierTaa (renderer, 8);
        setScreenSpaceReflections (renderer, screenSpaceReflections);
        setUpscaling (renderer, RenderSettings::Upscaling::temporal, 0.5f);

        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged());
        return image;
    };

    SceneRenderer on, off;
    const auto withReflections = render (true, on);
    const auto without = render (false, off);
    ASSERT_TRUE (withReflections.isValid() && without.isValid());

    const auto region = reflectedEmitterRegion();
    const auto floorOn = meanColor (withReflections, region);
    const auto floorOff = meanColor (without, region);
    RecordProperty ("floorRedOn", static_cast<int> (floorOn.getRed()));
    RecordProperty ("floorRedOff", static_cast<int> (floorOff.getRed()));
    // Measured 189 against 85, as 191 against 85 without upscaling
    EXPECT_GT (floorOn.getRed(), floorOff.getRed() + 80);

    const auto reflections = on.getPassOutput (SceneRenderer::PassOutput::reflections);
    ASSERT_NE (reflections, nullptr);
    EXPECT_EQ (reflections->getWidth(), 32u);
    EXPECT_TRUE (isFiniteTexture (reflections));
}

TEST_F (UpscalingMetalTests, RasterAndFramesWithoutTaaShadeAtFullSize)
{
    for (const auto& [tier, taaSamples] : { std::pair { RenderSettings::TierPreference::raster, 16 }, std::pair { RenderSettings::TierPreference::full, 1 } })
    {
        Scene scene;
        buildEdgesScene (scene);

        SceneRenderer renderer;
        setFullTierTaa (renderer, taaSamples);
        setTier (renderer, tier);
        setUpscaling (renderer, RenderSettings::Upscaling::temporal, 0.5f);
        renderer.setFrameStatsEnabled (true);
        ASSERT_NE (renderer.render (device, scene, 64, 48), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.waitForGPU().wasOk());

        EXPECT_EQ (renderer.getLastFrameStats().internalWidth, 64) << taaSamples;
        EXPECT_EQ (renderer.getLastFrameStats().internalHeight, 48) << taaSamples;
        EXPECT_FALSE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::upscaling, renderer.getRenderSettings())) << taaSamples;
    }
}

TEST_F (SceneRendererMetalTests, NonFiniteInputDoesNotPoisonHistory)
{
    // A huge point light just above a plate corner, a zero-area triangle, and a blended quad with
    // zero normals, a NaN emissive factor and a NaN alpha. Metal saturates overflowing half floats,
    // and max() and the clamp of the shaded color drop NaN, so the alpha is what reliably reaches
    // the scene color, through the blending
    const auto buildScene = [] (Scene& scene)
    {
        makePlates (scene);

        std::vector<Mesh::Vertex> vertices (7);
        for (auto& vertex : vertices)
        {
            vertex.position = { 0.2f, 0.2f, 0.6f };
            vertex.normal = {};
        }

        vertices[3].position = { -0.6f, -0.6f, 0.6f };
        vertices[4].position = { -0.4f, -0.6f, 0.6f };
        vertices[5].position = { -0.4f, -0.4f, 0.6f };
        vertices[6].position = { -0.6f, -0.4f, 0.6f };

        auto material = Material::Ptr (new Material());
        material->emissiveFactor = { std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f };
        material->baseColorFactor[3] = std::numeric_limits<float>::quiet_NaN();
        material->alphaMode = Material::AlphaMode::blend;

        auto mesh = Mesh::Ptr (new Mesh ("degenerate"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 3, 4, 5, 3, 5, 6 }, material);
        auto degenerate = scene.createEntity ("degenerate");
        degenerate.attach<MeshNode> (mesh);

        auto light = scene.createEntity ("point");
        auto& node = light.attach<LightNode>();
        node.type = LightNode::Type::point;
        node.range = 0.0f;
        node.intensity = 1.0e8f;
        light.getOrAttach<TransformNode>().setPosition ({ 1.0f, 1.0f, 0.01f });
    };

    // Without temporal anti-aliasing the scene color really holds non-finite values
    {
        Scene scene;
        buildScene (scene);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 1);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_FALSE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));
    }

    Scene scene;
    buildScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 16);
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));
}

TEST_F (SceneRendererMetalTests, FullTierWithoutTaaKeepsOverflowingHighlightsFinite)
{
    // Without TAA or bloom nothing sanitizes the scene color before tone mapping. Metal saturates
    // half floats written above their range, so radiance just past 65504 can't show the overflow
    // here; a light this strong overflows the 32 bit shading math itself to infinity
    Scene scene;
    makePlates (scene);

    auto light = scene.createEntity ("point");
    auto& node = light.attach<LightNode>();
    node.type = LightNode::Type::point;
    node.range = 0.0f;
    node.intensity = 1.0e38f;
    light.getOrAttach<TransformNode>().setPosition ({ 0.5f, 0.5f, 0.05f });

    auto& renderer = sharedRenderer();
    setFullTierTaa (renderer, 1);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_FALSE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, renderer.getRenderSettings()));
    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));
}
#endif

TEST_P (SceneRendererTierTests, BloomBrightensAroundAnEmissiveQuad)
{
    const auto tierName = GetParam() == RenderSettings::TierPreference::full ? String ("full") : String ("raster");

    Scene scene;
    buildGlowScene (scene);

    SceneRenderer withoutBloom;
    setTier (withoutBloom, GetParam());
    const auto off = renderToImage (withoutBloom, scene, 128);
    ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

    const auto quad = findBrightBounds (off);
    ASSERT_GE (quad.getWidth(), 12);
    ASSERT_GE (quad.getHeight(), 12);

    SceneRenderer withBloom;
    withBloom.setRenderSettings (withoutBloom.getRenderSettings());
    setBloom (withBloom, true);
    const auto on = renderToImage (withBloom, scene, 128);
    ASSERT_TRUE (on.isValid()) << withBloom.getLastError();
    EXPECT_TRUE (withBloom.getCapabilities()->isEnabled (RenderCapabilities::Feature::bloom, withBloom.getRenderSettings()));

    const auto ringOff = ringLuminance (off, quad);
    const auto ringOn = ringLuminance (on, quad);
    const auto inside = meanLuminance (on, quad.reduced (2));
    RecordProperty ((tierName + "RingOff").toStdString(), String (ringOff, 2).toStdString());
    RecordProperty ((tierName + "RingOn").toStdString(), String (ringOn, 2).toStdString());
    RecordProperty ((tierName + "Inside").toStdString(), String (inside, 2).toStdString());

    EXPECT_LT (ringOff, 1.0);
    EXPECT_GE (ringOn, 5.0);
    EXPECT_GE (inside, 245.0);
}

TEST_P (SceneRendererTierTests, BloomThresholdAboveTheHighlightAddsNoGlow)
{
    Scene scene;
    buildGlowScene (scene);

    SceneRenderer withoutBloom;
    setTier (withoutBloom, GetParam());
    const auto off = renderToImage (withoutBloom, scene, 128);
    ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

    // The quad emits 20: a threshold of 50 keeps nothing, even with the soft knee from 25
    auto settings = withoutBloom.getRenderSettings();
    settings.bloom = true;
    settings.bloomThreshold = 50.0f;
    settings.bloomIntensity = 1.0f;

    SceneRenderer withBloom;
    withBloom.setRenderSettings (settings);
    const auto on = renderToImage (withBloom, scene, 128);
    ASSERT_TRUE (on.isValid()) << withBloom.getLastError();
    ASSERT_TRUE (withBloom.getCapabilities()->isEnabled (RenderCapabilities::Feature::bloom, settings));

    const auto ring = ringLuminance (on, findBrightBounds (off));
    RecordProperty ("ring", String (ring, 2).toStdString());
    EXPECT_LT (ring, 1.0);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, SceneColorHasNoNaN)
{
    checkSceneColorHasNoNaN (GetParam());
}

TEST_P (SceneRendererTierTests, RendersEveryBaselineSceneWithoutNonFiniteValues)
{
    const std::pair<const char*, void (*) (Scene&)> scenes[] = {
        { "shadows", buildShadowsScene },
        { "materials", buildMaterialsScene },
        { "edges", buildEdgesScene },
    };

    const auto full = GetParam() == RenderSettings::TierPreference::full;

    for (const auto& [name, build] : scenes)
    {
        Scene scene;
        build (scene);

        SceneRenderer renderer;
        setTier (renderer, GetParam());
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << name << ": " << renderer.getLastError();
        ASSERT_TRUE (image.isValid()) << name;

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        const auto velocity = renderer.getPassOutput (SceneRenderer::PassOutput::velocity);
        ASSERT_NE (sceneColor, nullptr) << name;
        EXPECT_EQ (sceneColor->getFormat(), full ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm) << name;
        EXPECT_EQ (velocity != nullptr, full) << name;

        if (full)
            EXPECT_TRUE (isFiniteTexture (sceneColor)) << name;
    }
}
#endif

TEST_P (SceneRendererTierTests, DepthOfFieldBlursOutOfFocusBarsOnly)
{
    checkDepthOfFieldBlursOutOfFocusBarsOnly (GetParam());
}

TEST_P (SceneRendererTierTests, NearFieldBleedsOverTheFocusedBackground)
{
    checkNearFieldBleedsOverTheFocusedBackground (GetParam());
}

TEST_P (SceneRendererTierTests, InFocusEdgesMatchWithoutDepthOfField)
{
    checkInFocusEdgesMatchWithoutDepthOfField (GetParam());
}

TEST_P (SceneRendererTierTests, DepthOfFieldOffChangesNothing)
{
    checkDepthOfFieldOffChangesNothing (GetParam());
}

TEST_P (SceneRendererTierTests, DepthOfFieldKeepsTheSilhouetteBlendOverTheBackground)
{
    checkDepthOfFieldKeepsTheSilhouetteBlendOverTheBackground (GetParam());
}

TEST_P (SceneRendererTierTests, BlurredHighlightsKeepTheirBrightnessOverTheBackground)
{
    checkBlurredHighlightsKeepTheirBrightnessOverTheBackground (GetParam());
}

TEST_P (SceneRendererTierTests, MotionBlurOnRasterRendersTheUnblurredFallback)
{
    if (GetParam() != RenderSettings::TierPreference::raster)
        GTEST_SKIP() << "The full tier smears, see MotionBlurMetalTests";

    checkMotionBlurOnRasterRendersTheUnblurredFallback();
}

TEST_P (SceneRendererTierTests, FogAttenuatesWithDistance)
{
    checkFogAttenuatesWithDistance (GetParam());
}

TEST_P (SceneRendererTierTests, FogTransmittanceMatchesTheCpu)
{
    checkFogTransmittanceMatchesTheCpu (GetParam());
}

TEST_P (SceneRendererTierTests, InfiniteFarPlaneFogStaysFinite)
{
    checkInfiniteFarPlaneFogStaysFinite (GetParam());
}

TEST_P (SceneRendererTierTests, FogIsLitByTheAmbientLightAndTheEnvironment)
{
    checkFogIsLitByTheAmbientLightAndTheEnvironment (GetParam());
}

TEST_P (SceneRendererTierTests, FogScattersTheSunPhysically)
{
    checkFogScattersTheSunPhysically (GetParam());
}

TEST_P (SceneRendererTierTests, FogOverTheBackgroundKeepsItsBlendWithDepthOfField)
{
    checkFogOverTheBackgroundKeepsItsBlendWithDepthOfField (GetParam());
}

TEST_P (SceneRendererTierTests, FogBeforeBloomGlowsLikeADimmerEmitter)
{
    checkFogBeforeBloomGlowsLikeADimmerEmitter (GetParam());
}

TEST_P (SceneRendererTierTests, BackgroundColorLerpsTowardsTheFog)
{
    checkBackgroundColorLerpsTowardsTheFog (GetParam());
}

TEST_P (SceneRendererTierTests, FogScattersForwardTowardsTheSun)
{
    checkFogScattersForwardTowardsTheSun (GetParam());
}
