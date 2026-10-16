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

#if YUP_APPLE
TEST_F (SceneRendererMetalTests, ShadowCastingLightAfterEightOthersCasts)
{
    // Eight dark lights come first in the list and the shadow casting sun ninth: the frame must
    // still match the same scene without them, shadow included
    const auto render = [this] (int numLamps)
    {
        Scene scene;
        for (int i = 0; i < numLamps; ++i)
        {
            auto lamp = EntityNode::Ptr (new EntityNode ("lamp"));
            lamp->attach<LightNode> (LightNode::Type::point).intensity = 0.0f;
            scene.getRoot()->addChild (lamp);
        }

        buildShadowsScene (scene);

        SceneRenderer renderer;
        setTier (renderer, RenderSettings::TierPreference::raster);
        const auto image = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return image;
    };

    const auto withLamps = render (8);
    const auto withoutLamps = render (0);
    ASSERT_TRUE (withLamps.isValid() && withoutLamps.isValid());

    checkShadows (withLamps);

    const auto differing = countDifferences (withLamps, withoutLamps);
    RecordProperty ("shadowsDiffering", differing);
    EXPECT_EQ (differing, 0);
}

TEST_F (SceneRendererMetalTests, LightRadiusChangeRestartsTheFullTier)
{
    Scene scene;
    buildEdgesScene (scene);

    auto* light = scene.getRoot()->findChild ("sun")->getNode<LightNode>();
    ASSERT_NE (light, nullptr);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    light->radius = 0.05f;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, AreaLightVariantSwitchRestartsAndConvergesAgain)
{
    // A radius switches the shading passes to their area light variant mid-session, and back when
    // it goes: like any light change it restarts the convergence over the kept history, and no
    // frame fails
    Scene scene;
    buildEdgesScene (scene);

    auto* light = scene.getRoot()->findChild ("sun")->getNode<LightNode>();
    ASSERT_NE (light, nullptr);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    for (const auto radius : { 0.0f, 0.05f, 0.0f })
    {
        light->radius = radius;

        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << radius;

        for (int i = 0; i < 4; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        EXPECT_TRUE (renderer.isConverged()) << radius;
    }

    const auto gpuResult = renderer.waitForGPU();
    EXPECT_TRUE (gpuResult.wasOk()) << gpuResult.getErrorMessage();
}

TEST_F (SceneRendererMetalTests, KhrMaterialChangesRestartAccumulation)
{
    const std::pair<const char*, std::function<void (Material&)>> changes[] = {
        { "emissiveStrength", [] (Material& material) { material.emissiveStrength = 2.0f; } },
        { "ior", [] (Material& material) { material.ior = 1.33f; } },
        { "specularFactor", [] (Material& material) { material.specularFactor = 0.5f; } },
        { "specularColorFactor", [] (Material& material) { material.specularColorFactor[1] = 0.5f; } },
    };

    for (const auto& [name, change] : changes)
    {
        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;
        material->emissiveFactor = { 0.1f, 0.1f, 0.1f };

        Scene scene;
        addPlate (scene, material);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 4);
        for (int i = 0; i < 4; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*material);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
    }
}

TEST_F (SceneRendererMetalTests, ShadowSettingsRestartAccumulation)
{
    // Each change moves the value the full tier resolves away from its default
    const std::pair<const char*, std::function<void (RenderSettings&)>> changes[] = {
        { "sunCascades", [] (RenderSettings& settings) { settings.sunCascades = 2; } },
        { "shadowResolution", [] (RenderSettings& settings) { settings.shadowResolution = 1024; } },
        { "softShadows", [] (RenderSettings& settings) { settings.softShadows = false; } },
        { "contactShadows", [] (RenderSettings& settings) { settings.contactShadows = false; } },
    };

    for (const auto& [name, change] : changes)
    {
        Scene scene;
        buildEdgesScene (scene);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 8);
        for (int i = 0; i < 8; ++i)
            ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        auto settings = renderer.getRenderSettings();
        change (settings);
        renderer.setRenderSettings (settings);

        // Restarted, not invalidated: the first frame blends into the kept history, so its
        // silhouettes still have partially covered pixels. A discarded history leaves none
        const auto first = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_GE (countIntermediatePixels (first, 40), 4) << name;
    }
}

TEST_F (SceneRendererMetalTests, LightShadowFieldsRestartAccumulation)
{
    const std::pair<const char*, std::function<void (LightNode&)>> changes[] = {
        { "shadowBias", [] (LightNode& light) { light.shadowBias = 2.0f; } },
        { "contactShadows", [] (LightNode& light) { light.contactShadows = true; } },
        { "contactShadowDistance", [] (LightNode& light) { light.contactShadowDistance = 0.5f; } },
        { "contactShadowThickness", [] (LightNode& light) { light.contactShadowThickness = 0.1f; } },
    };

    for (const auto& [name, change] : changes)
    {
        Scene scene;
        buildEdgesScene (scene);

        auto* light = scene.getRoot()->findChild ("sun")->getNode<LightNode>();
        ASSERT_NE (light, nullptr);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 8);
        for (int i = 0; i < 8; ++i)
            ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*light);

        // Restarted, not invalidated, see ShadowSettingsRestartAccumulation
        const auto first = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_GE (countIntermediatePixels (first, 40), 4) << name;
    }
}

TEST_F (SceneRendererMetalTests, FullTierShadesManyLights)
{
    // 256 lights on a single draw: more than any per-draw light list holds, so the full tier must
    // find each fragment's lights through the clusters
    Scene scene;
    buildLightGridScene (scene, true);

    SceneRenderer renderer;
    setTier (renderer, RenderSettings::TierPreference::full);
    const auto image = renderToImage (renderer, scene, 256);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    const auto lit = countLitGridQuads (image);
    RecordProperty ("litQuads", lit);
    EXPECT_EQ (lit, 256);
}

TEST_F (SceneRendererMetalTests, RasterLightSelectionDoesNotPopUnderMovingLights)
{
    // 32 lights orbiting over 9 pads: each pad has more candidates than the 8 slots of a raster
    // draw, and lights crossing the selection cutoff or their range must fade instead of switching
    // on or off within a frame. The full tier shades every light through the clusters: its largest
    // change between two frames is what the motion alone produces
    const auto measureLargestFrameChange = [this] (RenderSettings::TierPreference tier)
    {
        Scene scene;
        const auto lights = buildOrbitingLightsScene (scene);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 1);
        setTier (renderer, tier);
        Image previous;
        int largest = 0;

        for (int frame = 0; frame < 90; ++frame)
        {
            moveOrbitingLights (lights, frame / 30.0);

            const auto image = renderToImage (renderer, scene, 128);
            EXPECT_TRUE (image.isValid()) << renderer.getLastError();
            if (! image.isValid())
                return -1;

            if (previous.isValid())
                largest = jmax (largest, maxChannelDifference (image, previous));

            previous = image;
        }

        return largest;
    };

    const auto raster = measureLargestFrameChange (RenderSettings::TierPreference::raster);
    const auto full = measureLargestFrameChange (RenderSettings::TierPreference::full);
    ASSERT_GT (full, 0);

    RecordProperty ("rasterLargestFrameChange", raster);
    RecordProperty ("fullLargestFrameChange", full);
    EXPECT_LE (raster, 2 * full);
}

TEST_F (SceneRendererMetalTests, FullTierRendersWithoutAnyLight)
{
    // No light and no default light: the light and cluster buffers stay empty, emission still shows.
    // A light buffer smaller than one GpuLight renders fine without validation: this test only
    // fails on it under Metal API validation (MTL_DEBUG_LAYER=1)
    Scene scene;
    buildGlowScene (scene, 0.5f);
    scene.setUsingDefaultLight (false);

    auto& renderer = sharedRenderer();
    setTier (renderer, RenderSettings::TierPreference::full);
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_GT (image.getPixelColor (64, 64).getRed(), 100);
    EXPECT_EQ (image.getPixelColor (4, 4).getRed(), 0);
}

TEST_F (SceneRendererMetalTests, SoftShadowFilterFollowsTheTierAndTaa)
{
    // GpuShadowData::filterInfo = (soft, filter taps, blocker taps, flags) per preflight R18: 8 + 8
    // rotating taps where temporal anti-aliasing averages them (flag 2), 16 + 16 still ones on full
    // without it, 8 + 8 still ones for raster's opt-in, and none without soft shadows or without a
    // shadowed light that has a size
    const struct
    {
        const char* name;
        RenderSettings::TierPreference tier;
        int taaSamples;
        std::optional<bool> softShadows;
        float radius;
        float taps;
        bool temporal;
    } cases[] = {
        { "full, TAA", RenderSettings::TierPreference::full, 16, {}, 0.05f, 8.0f, true },
        { "full, no TAA", RenderSettings::TierPreference::full, 1, {}, 0.05f, 16.0f, false },
        { "full, soft off", RenderSettings::TierPreference::full, 16, false, 0.05f, 0.0f, true },
        { "full, punctual sun", RenderSettings::TierPreference::full, 16, {}, 0.0f, 0.0f, true },
        { "raster opt-in", RenderSettings::TierPreference::raster, 16, true, 0.05f, 8.0f, false },
        { "raster default", RenderSettings::TierPreference::raster, 16, {}, 0.05f, 0.0f, false },
    };

    for (const auto& c : cases)
    {
        Scene scene;
        buildPenumbraScene (scene, c.radius);

        SceneRenderer renderer;
        auto settings = renderer.getRenderSettings();
        settings.tier = c.tier;
        settings.taaSamples = c.taaSamples;
        settings.softShadows = c.softShadows;
        renderer.setRenderSettings (settings);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << c.name << ": " << renderer.getLastError();

        const auto shadowData = shadows (renderer).getShadowData();
        const auto& filter = shadowData.filterInfo;
        EXPECT_EQ (filter[0], c.taps > 0.0f ? 1.0f : 0.0f) << c.name;
        EXPECT_EQ (filter[1], c.taps) << c.name;
        EXPECT_EQ (filter[2], c.taps) << c.name;
        EXPECT_EQ ((static_cast<int> (filter[3]) & 2) != 0, c.temporal) << c.name;
    }
}

TEST_P (SceneRendererTierTests, AreaLightVariantFollowsTheLightRadius)
{
    // A sun mirrored off a smooth black plate turns from punctual into a disk and back within one
    // renderer, each frame shading with the variant its lights need. The punctual variant ignores
    // the radius, so only the area light variant widens the highlight; the punctual frames before
    // and after the disk match, and the disk frame matches a renderer drawing it from the start
    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
    material->metallicFactor = 0.0f;
    material->roughnessFactor = 0.1f;

    Scene scene;
    addPlate (scene, material);
    scene.setAmbientColor (Colors::black);

    auto sun = EntityNode::Ptr (new EntityNode ("sun"));
    auto& light = sun->attach<LightNode>();
    scene.getRoot()->addChild (sun);

    // Without temporal anti-aliasing every full tier frame stands on its own
    SceneRenderer renderer;
    if (GetParam() == RenderSettings::TierPreference::full)
        setFullTierTaa (renderer, 1);
    else
        setTier (renderer, GetParam());

    std::array<Image, 3> frames;
    const std::array<float, 3> radii { 0.0f, 0.05f, 0.0f };

    for (size_t i = 0; i < frames.size(); ++i)
    {
        light.radius = radii[i];
        frames[i] = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (frames[i].isValid()) << i << ": " << renderer.getLastError();
    }

    // The canvas read back dithers the frames by a level, within what countDifferences() ignores
    EXPECT_EQ (countDifferences (frames[0], frames[2]), 0);
    EXPECT_GT (countDifferences (frames[1], frames[0]), 0);

    light.radius = 0.05f;
    SceneRenderer fresh;
    fresh.setRenderSettings (renderer.getRenderSettings());
    const auto disk = renderToImage (fresh, scene, 128);
    ASSERT_TRUE (disk.isValid()) << fresh.getLastError();
    EXPECT_EQ (countDifferences (frames[1], disk), 0);
}

TEST_P (SceneRendererTierTests, EmissiveStrengthScalesEmission)
{
    // An unlit black metal plate: no diffuse and no reflectance, only its emission
    const auto renderCenter = [this] (float emission, float strength)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->emissiveFactor = { emission, emission, emission };
        material->emissiveStrength = strength;

        Scene scene;
        addPlate (scene, material);
        scene.setUsingDefaultLight (false);
        scene.setExposure (1.0f);
        scene.setToneMapping (Scene::ToneMapping::reinhard);

        SceneRenderer renderer;
        setTier (renderer, GetParam());
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return image.isValid() ? image.getPixelColor (32, 32) : Color();
    };

    const auto plain = renderCenter (0.1f, 1.0f);
    const auto strengthened = renderCenter (0.1f, 4.0f);
    const auto reference = renderCenter (0.4f, 1.0f);
    RecordProperty ("plain", static_cast<int> (plain.getRed()));
    RecordProperty ("strengthened", static_cast<int> (strengthened.getRed()));

    EXPECT_NEAR (strengthened.getRed(), reference.getRed(), 2);
    EXPECT_NEAR (strengthened.getGreen(), reference.getGreen(), 2);
    EXPECT_NEAR (strengthened.getBlue(), reference.getBlue(), 2);
    EXPECT_GT (strengthened.getRed(), plain.getRed() + 20);
}

TEST_P (SceneRendererTierTests, HigherIorBrightensDielectricSpecular)
{
    // A black dielectric only reflects: under a grazing sun, its highlight follows the reflectance
    const auto renderMean = [this] (float ior)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.5f;
        material->ior = ior;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 10.0f;
        sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 1.3f));
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        setTier (renderer, GetParam());
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return image.isValid() ? meanLuminance (image, { 24, 24, 16, 16 }) : -1.0;
    };

    const auto water = renderMean (1.33f);
    const auto diamond = renderMean (2.4f);
    RecordProperty ("water", String (water, 2).toStdString());
    RecordProperty ("diamond", String (diamond, 2).toStdString());

    EXPECT_GT (water, 0.0);
    EXPECT_GT (diamond, water + 5.0);
}

TEST_P (SceneRendererTierTests, SpecularFactorAndColorShapeTheDielectricHighlight)
{
    // The highlight of HigherIorBrightensDielectricSpecular, as its mean red and green levels
    const auto renderHighlight = [this] (float specularFactor, std::array<float, 3> specularColor)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.5f;
        material->ior = 2.4f;
        material->specularFactor = specularFactor;
        material->specularColorFactor = specularColor;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 10.0f;
        sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 1.3f));
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        setTier (renderer, GetParam());
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();

        std::pair<double, double> redGreen { -1.0, -1.0 };
        if (! image.isValid())
            return redGreen;

        redGreen = { 0.0, 0.0 };
        for (int y = 24; y < 40; ++y)
        {
            for (int x = 24; x < 40; ++x)
            {
                redGreen.first += image.getPixelColor (x, y).getRed() / 256.0;
                redGreen.second += image.getPixelColor (x, y).getGreen() / 256.0;
            }
        }

        return redGreen;
    };

    const auto white = renderHighlight (1.0f, { 1.0f, 1.0f, 1.0f });
    const auto none = renderHighlight (0.0f, { 1.0f, 1.0f, 1.0f });
    const auto red = renderHighlight (1.0f, { 1.0f, 0.0f, 0.0f });
    RecordProperty ("whiteRed", String (white.first, 2).toStdString());
    RecordProperty ("noneRed", String (none.first, 2).toStdString());
    RecordProperty ("redRed", String (red.first, 2).toStdString());
    RecordProperty ("redGreen", String (red.second, 2).toStdString());

    // A zero factor removes the reflection, grazing incidence included; a red color keeps only red
    EXPECT_GT (white.first, 20.0);
    EXPECT_LE (none.first, 1.0);
    EXPECT_GT (red.first, red.second + 20.0);
}

TEST_P (SceneRendererTierTests, SpecularFactorDimsTheDiffuseOnce)
{
    // An ior of 0 reflects f0 = f90 = specularFactor at every angle. The diffuse is what a white base
    // adds over a black one, whose specular is the same
    const auto diffuseAt = [&] (float specularFactor)
    {
        return renderHeadOnRadiance (makeDielectric (1.0f, 0.0f, specularFactor), GetParam())[1]
             - renderHeadOnRadiance (makeDielectric (0.0f, 0.0f, specularFactor), GetParam())[1];
    };

    const auto unreflected = diffuseAt (0.0f);
    const auto ratio = diffuseAt (0.5f) / unreflected;
    RecordProperty ("unreflected", String (unreflected, 3).toStdString());
    RecordProperty ("ratio", String (ratio, 3).toStdString());

    // 2 / pi without any specular; dimmed by 1 - 0.5, where counting the factor twice gives 1 - 0.25
    EXPECT_NEAR (unreflected, 2.0 / MathConstants<double>::pi, 0.03);
    EXPECT_NEAR (ratio, 0.5, 0.05);
}

TEST_P (SceneRendererTierTests, ColoredSpecularDimsEveryDiffuseChannel)
{
    // A red specular color at an ior of 0 reflects all the red: KHR_materials_specular takes the
    // largest channel, so nothing is left to the diffuse, in green either
    const auto white = renderHeadOnRadiance (makeDielectric (1.0f, 0.0f, 1.0f, { 1.0f, 0.0f, 0.0f }), GetParam());
    const auto black = renderHeadOnRadiance (makeDielectric (0.0f, 0.0f, 1.0f, { 1.0f, 0.0f, 0.0f }), GetParam());
    const auto greenDiffuse = white[1] - black[1];
    RecordProperty ("greenDiffuse", String (greenDiffuse, 3).toStdString());

    // Weighting each channel on its own leaves a green diffuse of about 2 / pi
    EXPECT_LT (greenDiffuse, 0.03);
    EXPECT_NEAR (black[0], 2.0 / 4.0 / MathConstants<double>::pi, 0.03); // The red specular, f0 = 1
}

TEST_P (SceneRendererTierTests, DiffuseLiesUnderTheDielectricLayerOnly)
{
    // Half metallic red: the metal reflects f0 = 1 in red, but the diffuse half sits under the
    // dielectric layer alone (f0 = 0.04), as in the glTF mix of the dielectric and metal BRDFs
    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 1.0f, 0.0f, 0.0f, 1.0f };
    material->metallicFactor = 0.5f;

    const auto red = renderHeadOnRadiance (material, GetParam())[0];
    const auto f0 = 0.5 * 0.04 + 0.5 * 1.0;
    const auto expected = 2.0 * (f0 / 4.0 + 0.5 * (1.0 - 0.04)) / MathConstants<double>::pi;
    RecordProperty ("red", String (red, 3).toStdString());
    RecordProperty ("expected", String (expected, 3).toStdString());

    // Dimming the diffuse by the mixed reflectance instead (1 - 0.52) gives 0.236
    EXPECT_NEAR (red, expected, 0.02);
}

TEST_P (SceneRendererTierTests, AmbientDiffuseLiesUnderTheDielectricLayer)
{
    // Ambient light only. A white dielectric reflects f0 and diffuses what its layer leaves,
    // 1 - f0: all of the ambient light, whatever its ior. An ior of 0 reflects everything, f0 = 1,
    // where an unweighted diffuse would double the energy
    const auto ambient = Color (static_cast<uint8> (128), static_cast<uint8> (128), static_cast<uint8> (128));
    const auto linearAmbient = std::pow ((128.0 / 255.0 + 0.055) / 1.055, 2.4);

    const auto mirror = renderHeadOnRadiance (makeDielectric (1.0f, 0.0f, 1.0f), GetParam(), 0.0f, ambient)[1];
    const auto plastic = renderHeadOnRadiance (makeDielectric (1.0f, 1.5f, 1.0f), GetParam(), 0.0f, ambient)[1];
    RecordProperty ("mirror", String (mirror, 4).toStdString());
    RecordProperty ("plastic", String (plastic, 4).toStdString());

    EXPECT_NEAR (mirror, linearAmbient, 0.01);
    EXPECT_NEAR (plastic, linearAmbient, 0.01);
}

TEST_P (SceneRendererTierTests, DrawsFindTheirLightsWhenDirectionalLightsArePackedFirst)
{
    // The point light comes first in the list, but the sun is packed first: the plate, out of
    // the point light's range, selects only the sun, which must reach it through the packed order
    auto material = Material::Ptr (new Material());
    material->metallicFactor = 0.0f;

    Scene scene;
    scene.setAmbientColor (Colors::black);

    auto lamp = EntityNode::Ptr (new EntityNode ("lamp"));
    auto& point = lamp->attach<LightNode> (LightNode::Type::point);
    point.color = { 1.0f, 0.0f, 0.0f };
    point.intensity = 100.0f;
    point.range = 1.0f;
    lamp->setPosition ({ 10.0f, 0.0f, 1.0f });
    scene.getRoot()->addChild (lamp);

    auto sun = EntityNode::Ptr (new EntityNode ("sun"));
    auto& sunLight = sun->attach<LightNode>();
    sunLight.color = { 0.0f, 1.0f, 0.0f };
    sunLight.intensity = 2.0f;
    scene.getRoot()->addChild (sun);

    addPlate (scene, material);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    const auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    const auto center = image.getPixelColor (32, 32);
    EXPECT_GE (static_cast<int> (center.getGreen()), 100);
    EXPECT_LE (static_cast<int> (center.getRed()), 10);
}

TEST_P (SceneRendererTierTests, ShadesMoreThanEightLightsPerScene)
{
    checkShadesMoreThanEightLightsPerScene (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, ZeroTangentFallsBackToDerivativeFrame)
{
    // The normal map tilts the plate towards +x, where a grazing sun shines from: the tangent
    // (1, 0, 0, 1) is the frame the derivatives build
    const auto tiltedTowardsX = Color (191, 128, 247);
    const auto sunFromX = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 1.2f);

    const auto tangentFrame = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsX, sunFromX);
    const auto derivativeFrame = renderNormalMappedPlate ({ 0.0f, 0.0f, 0.0f, 0.0f }, tiltedTowardsX, sunFromX);
    const auto unmapped = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, std::nullopt, sunFromX);
    const auto flipped = renderNormalMappedPlate ({ -1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsX, sunFromX);
    RecordProperty ("tangentFrame", String (tangentFrame, 2).toStdString());
    RecordProperty ("derivativeFrame", String (derivativeFrame, 2).toStdString());
    RecordProperty ("unmapped", String (unmapped, 2).toStdString());
    RecordProperty ("flipped", String (flipped, 2).toStdString());

    EXPECT_NEAR (derivativeFrame, tangentFrame, tangentFrame * 0.02);

    // Both frames really apply the map
    EXPECT_GT (tangentFrame, unmapped * 1.1);

    // Authored tangents are really used: reversed, they tilt the plate away from the sun
    EXPECT_LT (flipped, unmapped);
}

TEST_P (SceneRendererTierTests, BitangentHandednessSurvivesMirroring)
{
    // The normal map tilts the plate towards +y (decreasing v), where a grazing sun shines from:
    // only the bitangent, and so the handedness, decides the shading
    const auto tiltedTowardsY = Color (128, 191, 247);
    const auto sunFromY = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -1.2f);

    const auto tangentFrame = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsY, sunFromY);
    const auto derivativeFrame = renderNormalMappedPlate ({ 0.0f, 0.0f, 0.0f, 0.0f }, tiltedTowardsY, sunFromY);
    const auto unmapped = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, std::nullopt, sunFromY);
    const auto leftHanded = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, -1.0f }, tiltedTowardsY, sunFromY);
    const auto mirroredTangentFrame = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsY, sunFromY, -1.0f);
    const auto mirroredDerivativeFrame = renderNormalMappedPlate ({ 0.0f, 0.0f, 0.0f, 0.0f }, tiltedTowardsY, sunFromY, -1.0f);
    RecordProperty ("tangentFrame", String (tangentFrame, 2).toStdString());
    RecordProperty ("derivativeFrame", String (derivativeFrame, 2).toStdString());
    RecordProperty ("unmapped", String (unmapped, 2).toStdString());
    RecordProperty ("leftHanded", String (leftHanded, 2).toStdString());
    RecordProperty ("mirroredTangentFrame", String (mirroredTangentFrame, 2).toStdString());
    RecordProperty ("mirroredDerivativeFrame", String (mirroredDerivativeFrame, 2).toStdString());

    EXPECT_NEAR (derivativeFrame, tangentFrame, tangentFrame * 0.02);
    EXPECT_GT (tangentFrame, unmapped * 1.1);

    // w = -1 turns the bitangent, and the tilt, away from the sun
    EXPECT_LT (leftHanded, unmapped);

    // A mirroring transform keeps the bitangent pointing up the texture, with or without tangents
    EXPECT_NEAR (mirroredTangentFrame, tangentFrame, tangentFrame * 0.02);
    EXPECT_NEAR (mirroredDerivativeFrame, tangentFrame, tangentFrame * 0.02);
}

TEST_P (SceneRendererTierTests, GBufferNormalFacesTheCamera)
{
    Scene scene;
    buildShadowsScene (scene);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto normalRoughness = renderer.getPassOutput (SceneRenderer::PassOutput::gbufferNormal);
    if (GetParam() == RenderSettings::TierPreference::raster)
    {
        EXPECT_EQ (normalRoughness, nullptr);
        return;
    }

    ASSERT_NE (normalRoughness, nullptr);
    EXPECT_EQ (normalRoughness->getFormat(), GpuTextureFormat::rgba16float);

    // The plates face the camera: +Z
    const auto normal = readGBufferNormal (normalRoughness).getPixelColor (32, 32);
    EXPECT_NEAR (normal.getRed(), 128, 3);
    EXPECT_NEAR (normal.getGreen(), 128, 3);
    EXPECT_NEAR (normal.getBlue(), 255, 3);

    // A cleared texel decodes to +Z as well: only drawn texels have a roughness
    const auto roughness = mapTexels (normalRoughness, "vec4(t.z, 0.0, 0.0, 1.0)");
    ASSERT_TRUE (roughness.isValid());
    EXPECT_GT (roughness.getPixelColor (32, 32).getRed(), 64);
    EXPECT_EQ (roughness.getPixelColor (1, 1).getRed(), 0);
}
#endif

TEST_P (SceneRendererTierTests, RendersWithoutTheDepthNormalPrepass)
{
    checkRendersWithoutTheDepthNormalPrepass (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, TinyRadiusMatchesPunctual)
{
    checkTinyRadiusMatchesPunctual (GetParam());
}

TEST_P (SceneRendererTierTests, RadiusWidensTheHighlight)
{
    checkRadiusWidensTheHighlight (GetParam());
}

TEST_P (SceneRendererTierTests, SunDiskSoftensNothingButSpecular)
{
    // A sun of angular radius 0.01 is a disk of radiance I / (pi sin^2 radius), so a rough plate
    // receives the illuminance of the punctual sun, I
    const auto meanAt = [this] (float radius)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 1.0f;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        auto& node = sun->attach<LightNode>();
        node.intensity = 2.0f;
        node.radius = radius;
        sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 0.5f));
        scene.getRoot()->addChild (sun);

        return meanLuma (renderLuma (scene, 128), 128, { 40, 40, 48, 48 });
    };

    const auto punctual = meanAt (0.0f);
    const auto disk = meanAt (0.01f);
    RecordProperty ("punctual", String (punctual, 4).toStdString());
    RecordProperty ("disk", String (disk, 4).toStdString());

    ASSERT_GT (punctual, 0.05);
    EXPECT_NEAR (disk / punctual, 1.0, 0.01);
}

TEST_P (SceneRendererTierTests, AreaLightsStayFinite)
{
    checkAreaLightsStayFinite (GetParam());
}

TEST_P (SceneRendererTierTests, LayeredAreaLightsStayFinite)
{
    checkAreaLightsStayFinite (GetParam(), true);
}

TEST_P (SceneRendererTierTests, LayersUnderATinyAreaLightMatchPunctual)
{
    checkLayersUnderATinyAreaLightMatchPunctual (GetParam());
}

TEST_P (SceneRendererTierTests, LargeLowSphereLightDiffuseIsExact)
{
    // A sphere light of angular radius 25 degrees, its center 17 degrees over a diffuse-only plate:
    // the horizon cuts its cap, and the plate center receives albedo / pi times the radiance
    // I / (pi r^2) integrated over the clipped cap, by quadrature
    const auto sigma = MathConstants<double>::pi * 25.0 / 180.0;
    const auto elevation = MathConstants<double>::pi * 17.0 / 180.0;
    const auto distance = 10.0;
    const auto radius = distance * std::sin (sigma);
    const auto intensity = 400.0;
    const auto albedo = 0.8;

    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { static_cast<float> (albedo), static_cast<float> (albedo), static_cast<float> (albedo), 1.0f };
    material->metallicFactor = 0.0f;
    material->roughnessFactor = 1.0f;
    material->specularFactor = 0.0f;

    Scene scene;
    addPlate (scene, material);
    scene.setAmbientColor (Colors::black);

    auto light = EntityNode::Ptr (new EntityNode ("sphere"));
    auto& node = light->attach<LightNode> (LightNode::Type::point);
    node.intensity = static_cast<float> (intensity);
    node.radius = static_cast<float> (radius);
    light->setPosition ({ static_cast<float> (distance * std::cos (elevation)), 0.0f, static_cast<float> (distance * std::sin (elevation)) });
    scene.getRoot()->addChild (light);

    const auto measured = meanLuma (renderLuma (scene, 128), 128, { 63, 63, 2, 2 });
    const auto radiance = intensity / (MathConstants<double>::pi * radius * radius);
    const auto expected = albedo * radiance * SceneRendererAreaLightMathTests::capQuadrature (sigma, MathConstants<double>::halfPi - elevation);
    RecordProperty ("measured", String (measured, 4).toStdString());
    RecordProperty ("expected", String (expected, 4).toStdString());

    ASSERT_GT (expected, 0.1);
    EXPECT_NEAR (measured / expected, 1.0, 0.03);
}

TEST_P (SceneRendererTierTests, SphereLightSpecularMatchesGgx)
{
    // A white metal plate (f0 = f90 = 1) seen at n.v = 0.5 mirrors a sphere light of angular radius
    // 10 degrees: the LTC specular matches the GGX lobe integrated over the cap by quadrature, within
    // the fit error at roughness 0.5
    const auto sigma = MathConstants<double>::pi * 10.0 / 180.0;
    const auto distance = 3.0;
    const auto radius = distance * std::sin (sigma);
    const auto intensity = 2.0;
    const auto roughness = 0.5;

    auto material = Material::Ptr (new Material());
    material->metallicFactor = 1.0f;
    material->roughnessFactor = static_cast<float> (roughness);

    Scene scene;
    auto* plate = addPlate (scene, material);
    scene.setAmbientColor (Colors::black);

    // Tilted so the normal makes 60 degrees with the direction to the camera
    const auto tilt = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -MathConstants<float>::pi / 3.0f);
    plate->setRotation (tilt);

    const auto normal = tilt.rotate ({ 0.0f, 0.0f, 1.0f });
    const Vector3<float> toCamera { 0.0f, 0.0f, 1.0f };
    const auto mirrored = normal * (2.0f * normal.dotProduct (toCamera)) - toCamera;

    auto light = EntityNode::Ptr (new EntityNode ("sphere"));
    auto& node = light->attach<LightNode> (LightNode::Type::point);
    node.intensity = static_cast<float> (intensity);
    node.radius = static_cast<float> (radius);
    light->setPosition (mirrored * static_cast<float> (distance));
    scene.getRoot()->addChild (light);

    const auto measured = meanLuma (renderLuma (scene, 128), 128, { 63, 63, 2, 2 });
    const auto radiance = intensity / (MathConstants<double>::pi * radius * radius);
    const auto expected = radiance * SceneRendererAreaLightMathTests::ggxCapQuadrature (0.5, roughness * roughness, { -std::sqrt (0.75), 0.0, 0.5 }, sigma);
    RecordProperty ("measured", String (measured, 4).toStdString());
    RecordProperty ("expected", String (expected, 4).toStdString());

    ASSERT_GT (expected, 0.1);
    EXPECT_NEAR (measured / expected, 1.0, 0.1);
}

TEST_P (SceneRendererTierTests, TinySpotMatchesPunctualOffAxis)
{
    // A spot of radius 0.001 is a sphere under the cone of its center: 35 degrees off its axis, near
    // the 45 degree edge of its cone, it lights the plate like the punctual spot
    const auto meanAt = [this] (float radius)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.5f;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto light = EntityNode::Ptr (new EntityNode ("spot"));
        auto& node = light->attach<LightNode> (LightNode::Type::spot);
        node.intensity = 5.0f;
        node.radius = radius;
        light->setPosition ({ 0.0f, 0.0f, 1.0f });
        scene.getRoot()->addChild (light);

        // The plate point at tan (35 degrees) = 0.7 along x
        return meanLuma (renderLuma (scene, 128), 128, { 84, 62, 2, 4 });
    };

    const auto punctual = meanAt (0.0f);
    const auto area = meanAt (0.001f);
    RecordProperty ("punctual", String (punctual, 4).toStdString());
    RecordProperty ("area", String (area, 4).toStdString());

    ASSERT_GT (punctual, 0.01);
    EXPECT_NEAR (area / punctual, 1.0, 0.02);
}

TEST_P (SceneRendererTierTests, SunDiskWidensTheHighlight)
{
    // A sun shining down the view mirrors off a smooth black plate into a small highlight; an angular
    // radius of 0.05 spreads it over more bright pixels
    const auto brightPixelsAt = [this] (float radius)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.1f;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().radius = radius;
        scene.getRoot()->addChild (sun);

        const auto luma = renderLuma (scene, 128);
        return static_cast<int> (std::count_if (luma.begin(), luma.end(), [] (double value) { return value > 2.5; }));
    };

    const auto sharp = brightPixelsAt (0.0f);
    const auto wide = brightPixelsAt (0.05f);
    RecordProperty ("sharp", sharp);
    RecordProperty ("wide", wide);

    EXPECT_GT (sharp, 0);
    EXPECT_GT (wide, 2 * sharp);
}

TEST_P (SceneRendererTierTests, RendersWithoutTheLookupTables)
{
    // When the lookup tables cannot be created, frames still render: area lights lose their
    // specular light, never their diffuse light, and nothing turns non-finite
    const auto full = GetParam() == RenderSettings::TierPreference::full;

    const auto render = [&] (bool withTables)
    {
        Scene scene;
        buildAreaLightsScene (scene);

        SceneRenderer renderer;
        if (full)
            setFullTierTaa (renderer, 1);
        else
            setTier (renderer, RenderSettings::TierPreference::raster);

        if (! withTables)
            renderer.failLookupTablesForTesting();

        const auto image = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

        if (full)
        {
            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            EXPECT_NE (sceneColor, nullptr);
            EXPECT_TRUE (sceneColor != nullptr && isFiniteTexture (sceneColor));
            EXPECT_FALSE (sceneColor == nullptr || isSaturatedTexture (sceneColor));
        }

        return image;
    };

    const auto with = render (true);
    const auto without = render (false);
    ASSERT_TRUE (with.isValid());
    ASSERT_TRUE (without.isValid());

    // Only specular light is lost: no pixel brightens, some darken, and none goes black (a NaN
    // reads black or white on Metal)
    int brighter = 0, darker = 0;
    auto darkest = 255.0;
    for (int y = 0; y < 128; ++y)
    {
        for (int x = 0; x < 128; ++x)
        {
            const auto before = meanLuminance (with, { x, y, 1, 1 });
            const auto after = meanLuminance (without, { x, y, 1, 1 });
            brighter += after > before + 2.0 ? 1 : 0;
            darker += after < before - 2.0 ? 1 : 0;
            darkest = jmin (darkest, after);
        }
    }

    RecordProperty ("darker", darker);
    EXPECT_EQ (brighter, 0);
    EXPECT_GT (darker, 100);
    EXPECT_GT (darkest, 5.0);

    // The environment's specular light goes too: a white rough metal, which only reflects, turns
    // black under a uniform environment
    const auto metal = renderHeadOnRadiance (Material::Ptr (new Material()), GetParam(), 0.0f, Colors::black, 0.5f);
    const auto blackMetal = renderHeadOnRadiance (Material::Ptr (new Material()), GetParam(), 0.0f, Colors::black, 0.5f, true);
    EXPECT_GT (metal[1], 0.1);
    EXPECT_LT (blackMetal[1], 0.005);
}

TEST_P (SceneRendererTierTests, RoughWhiteMetalPassesTheWhiteFurnaceTest)
{
    // A white rough metal facing the view, under a uniform environment and no light, reflects all
    // of it. The split sum alone reflects the single-scattering albedo, 1 - ln 2 at roughness 1
    // (the analytic fit used before Phase 4 gave 0.45); the multiple-scattering compensation
    // (Fdez-Aguera 2019) returns the rest
    const auto radiance = renderHeadOnRadiance (Material::Ptr (new Material()), GetParam(), 0.0f, Colors::black, 0.5f);
    RecordProperty ("roughMetalBrightness", String (radiance[1] / 0.5, 4).toStdString());

    for (const auto channel : radiance)
        EXPECT_NEAR (channel, 0.5, 0.02);

    // A colored metal tells the average Fresnel apart: each channel regains FssEss Favg Ems /
    // (1 - Favg Ems), with Favg = f0 + (1 - f0) / 21. At f0 = 0.5 that is 0.2409 of the
    // environment, where the tables alone give 0.1534 and Favg = 1 would give 0.5. A brighter
    // environment keeps the darkest channel well above the 8-bit steps of the image
    const std::array<float, 3> f0 { 1.0f, 0.5f, 0.2f };
    const auto environment = 2.0f;

    auto colored = Material::Ptr (new Material());
    colored->baseColorFactor = { f0[0], f0[1], f0[2], 1.0f };
    const auto coloredRadiance = renderHeadOnRadiance (colored, GetParam(), 0.0f, Colors::black, environment);

    const auto amplitude = LtcTables::getAmplitude (LtcTables::size - 1, 0);
    for (size_t channel = 0; channel < f0.size(); ++channel)
    {
        const auto singleScattering = f0[channel] * (amplitude[0] - amplitude[1]) + amplitude[1];
        const auto averageFresnel = f0[channel] + (1.0f - f0[channel]) / 21.0f;
        const auto missing = 1.0f - amplitude[0];
        const auto expected = environment * (singleScattering + singleScattering * averageFresnel * missing / (1.0f - averageFresnel * missing));

        EXPECT_NEAR (coloredRadiance[channel], expected, expected * 0.1) << "channel " << channel;
    }
}

TEST_P (SceneRendererTierTests, RendersWithoutTheShadowCompareSampler)
{
    checkRendersWithoutTheShadowCompareSampler (GetParam());
}

TEST_P (SceneRendererTierTests, BaseColorAboveOneShadesAsOne)
{
    // glTF base colors lie within [0, 1]: a larger factor is clamped alike by the forward pass and
    // the G-buffer, so both tiers agree
    const auto one = renderHeadOnRadiance (makeDielectric (1.0f, 1.5f, 1.0f), GetParam());
    const auto two = renderHeadOnRadiance (makeDielectric (2.0f, 1.5f, 1.0f), GetParam());

    ASSERT_GT (one[0], 0.1);
    for (size_t channel = 0; channel < 3; ++channel)
        EXPECT_NEAR (two[channel] / one[channel], 1.0, 0.02) << "channel " << channel;
}
#endif

TEST_P (SceneRendererTierTests, RendersWithoutTheShadowPipelines)
{
    checkRendersWithoutTheShadowPipelines (GetParam());
}

TEST_P (SceneRendererTierTests, ShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed)
{
    checkShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed (GetParam());
}

TEST_P (SceneRendererTierTests, SunShadowReachesNearAndFarCasters)
{
    checkSunShadowReachesNearAndFarCasters (GetParam());
}

TEST_P (SceneRendererTierTests, CascadeTransitionHasNoSeam)
{
    checkCascadeTransitionHasNoSeam (GetParam());
}

TEST_P (SceneRendererTierTests, AreaSunCastsAShadow)
{
    checkAreaSunCastsAShadow (GetParam());
}

TEST_P (SceneRendererTierTests, MaskedCasterShadowsOnlyWhatItKeeps)
{
    checkMaskedCasterShadowsOnlyWhatItKeeps (GetParam());
}

TEST_P (SceneRendererTierTests, ShadowMapsRenderOnlyWhenTheShadowsChange)
{
    // A still scene draws its shadow maps once, a moved caster draws them again, and so does
    // restartAccumulation(), for changes the renderer cannot see
    Scene scene;
    buildLongGroundScene (scene, 2.0f);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    for (int frame = 0; frame < 3; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    auto* caster = scene.getRoot()->findChild ("caster");
    ASSERT_NE (caster, nullptr);
    caster->setPosition (caster->getPosition() + Vector3<float> (0.05f, 0.0f, 0.0f));

    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);

    renderer.restartAccumulation();
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 3);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, ShadowMapsSurviveMovingLightsAndRedrawnOpaqueTextures)
{
    // The Scene3D demo's frames: lights casting no shadow orbit and an opaque caster's texture is
    // redrawn, under the jittered camera of TAA on the full tier. None of it changes the shadows
    auto material = Material::Ptr (new Material());
    auto texture = Texture::Ptr (new Texture (GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm))));
    material->baseColorTexture = texture;

    Scene scene;
    buildLongGroundScene (scene, 2.0f, material);

    auto orbiting = EntityNode::Ptr (new EntityNode ("orbiting"));
    orbiting->attach<LightNode> (LightNode::Type::point);
    scene.getRoot()->addChild (orbiting);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    for (int frame = 0; frame < 4; ++frame)
    {
        orbiting->setPosition ({ 0.2f * static_cast<float> (frame), 1.0f, -2.0f });
        texture->setGpuTexture (GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm)));
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    }

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, MaskedCasterChangesDrawTheShadowMapsAgain)
{
    // A masked caster casts what its alpha test keeps: a new cutoff, base alpha or base color
    // texture content changes the maps of an otherwise still scene
    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 0.8f };
    material->alphaMode = Material::AlphaMode::mask;

    Scene scene;
    buildLongGroundScene (scene, 2.0f, material);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    material->alphaCutoff = 0.9f;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);

    material->baseColorFactor[3] = 1.0f;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 3);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, GpuFailureDrawsTheShadowMapsAgain)
{
    // The frame drawing the maps failed on the GPU: the next frame cannot trust them
    Scene scene;
    buildLongGroundScene (scene, 2.0f);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    // Frames complete in order, so once a later frame did the failed one has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, OrbitingACompactSceneDoesNotRedrawTheShadows)
{
    // A device-sized box on a ground of about its size, seen whole: one map fitted to the scene is
    // finer than the first cascade and ignores the camera, so orbiting around draws it only once
    Scene scene;
    auto& box = addCompactScene (scene);

    addCamera (scene, { 5.0f, 4.0f, 0.0f }, {});
    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    for (int frame = 0; frame < 8; ++frame)
    {
        const auto yaw = 0.3f * static_cast<float> (frame);
        aimCamera (*camera, { 5.0f * std::cos (yaw), 4.0f, 5.0f * std::sin (yaw) }, {});

        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).getNumRecords(), 1) << "frame " << frame;
    }

    // Zooming out and panning keep the map too
    aimCamera (*camera, { 6.5f, 5.2f, 0.0f }, {});
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    aimCamera (*camera, { 6.5f, 5.2f, 0.3f }, { 0.0f, 0.0f, 0.3f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRecords(), 1);
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    // Moving the box draws it again
    box.setPosition ({ 0.1f, 0.3f, 0.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, LargeSceneKeepsViewFittedCascades)
{
    // The long ground near one end: no single map over 200 m matches the first cascade, so the tier's
    // cascades stay fitted to the view, and a camera move fits them again
    Scene scene;
    buildLongGroundScene (scene, 2.0f);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    ASSERT_NE (renderer.getCapabilities(), nullptr);
    const auto cascades = renderer.getCapabilities()->getCascadeCount (renderer.getRenderSettings());
    EXPECT_EQ (cascades, GetParam() == RenderSettings::TierPreference::full ? 4 : RenderCapabilities::defaultRasterCascades);
    EXPECT_EQ (shadows (renderer).getNumRecords(), cascades);
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);
    camera->setPosition (camera->getPosition() + Vector3<float> (0.5f, 0.0f, 0.0f));

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRecords(), cascades);
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, CasterMovingInsideTheLastCascadeRedrawsOnlyIt)
{
    // The far cube moves sideways, still beyond every cascade but the last: the ground keeps the
    // fit of every cascade, so only the last one holds anything new
    Scene scene;
    buildLongGroundScene (scene, 2.0f);
    auto& farCaster = addFarCaster (scene);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto cascades = shadows (renderer).getNumRecords();
    ASSERT_GT (cascades, 1);

    auto expected = getCascadeRenderCounts (renderer, cascades);
    EXPECT_EQ (expected, std::vector<int> (static_cast<size_t> (cascades), 1));

    farCaster.setPosition (farCaster.getPosition() + Vector3<float> (0.5f, 0.0f, 0.0f));
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    ++expected.back();
    EXPECT_EQ (getCascadeRenderCounts (renderer, cascades), expected);
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, CasterLeavingACascadeRedrawsIt)
{
    // The near cube moves beside the far one: the first cascade, which it leaves, and the last
    // one, which it enters, draw again
    Scene scene;
    buildLongGroundScene (scene, 2.0f);
    addFarCaster (scene);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto cascades = shadows (renderer).getNumRecords();
    ASSERT_GT (cascades, 1);

    auto* caster = scene.getRoot()->findChild ("caster");
    ASSERT_NE (caster, nullptr);
    caster->setPosition ({ 3.0f, caster->getPosition().getY(), -60.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto counts = getCascadeRenderCounts (renderer, cascades);
    EXPECT_EQ (counts.front(), 2);
    EXPECT_EQ (counts.back(), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, ShadowSettingOrSunChangeRedrawsEveryCascade)
{
    Scene scene;
    buildLongGroundScene (scene, 2.0f);
    addFarCaster (scene);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto cascades = shadows (renderer).getNumRecords();
    ASSERT_GT (cascades, 1);

    // The sun turns a little
    auto* sun = scene.getRoot()->findChild ("sun");
    ASSERT_NE (sun, nullptr);
    aimCamera (*sun, { -10.0f * std::sin (0.62f), 10.0f * std::cos (0.62f), 0.0f }, {});
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getCascadeRenderCounts (renderer, cascades), std::vector<int> (static_cast<size_t> (cascades), 2));

    // The shadow resolution changes
    auto settings = renderer.getRenderSettings();
    settings.shadowResolution = 1024;
    renderer.setRenderSettings (settings);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getCascadeRenderCounts (renderer, cascades), std::vector<int> (static_cast<size_t> (cascades), 3));
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, MovingAShadowedPointLightKeepsTheSunCascades)
{
    // The point light's views live in the atlas: moving it draws the atlas again, never a cascade
    Scene scene;
    buildLongGroundScene (scene, 2.0f);

    auto lamp = EntityNode::Ptr (new EntityNode ("lamp"));
    auto& light = lamp->attach<LightNode> (LightNode::Type::point);
    light.castsShadows = true;
    light.range = 5.0f;
    lamp->setPosition ({ 1.0f, 1.0f, -2.0f });
    scene.getRoot()->addChild (lamp);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_GT (shadows (renderer).getAtlasSize(), 0);

    const auto cascades = static_cast<int> (shadows (renderer).getShadowData().sunInfo[0]);
    ASSERT_GT (cascades, 1);
    const auto counts = getCascadeRenderCounts (renderer, cascades);

    lamp->setPosition ({ 1.2f, 1.0f, -2.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);
    EXPECT_EQ (getCascadeRenderCounts (renderer, cascades), counts);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, OrthographicCameraTakesTheSceneFitThatNeverFadesOut)
{
    // A wide orthographic view from above whose far plane cuts the ground off: its one view-fitted
    // cascade would fade out and follow every pan, the scene fit covers every receiver and stays
    Scene scene;
    addCompactScene (scene);
    addCamera (scene, { 0.0f, 5.0f, 0.0f }, {}, { 0.0f, 0.0f, -1.0f });

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);
    auto& lens = *scene.getActiveCamera();
    lens.projection = CameraNode::Projection::orthographic;
    lens.xMag = 3.0f;
    lens.yMag = 3.0f;
    lens.zFar = 4.9f;

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    for (const auto& offset : { Vector3<float> (0.0f, 0.0f, 0.0f), Vector3<float> (0.5f, 0.0f, 0.0f), Vector3<float> (0.5f, 0.0f, 0.3f) })
    {
        camera->setPosition (Vector3<float> (0.0f, 5.0f, 0.0f) + offset);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        const auto data = shadows (renderer).getShadowData();
        EXPECT_EQ (data.sunInfo[0], 1.0f);
        EXPECT_EQ (data.sunInfo[3], 0.0f);
    }

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, SceneFitHoldsWhileTheFirstCascadeIsNoFiner)
{
    // The scene fit of the compact scene has a radius of 1.40625. From "near" the first cascade has a
    // radius of 1.5 (one quantization step smaller: 1.375), so a fresh renderer keeps the cascades
    // there; from "far" it has 1.625 on raster, 1.75 on full, so the scene fit takes over, and holds
    // when the camera comes back near, its texels still no larger than the first cascade's
    Scene scene;
    addCompactScene (scene);
    addCamera (scene, { 5.0f, 4.0f, 0.0f }, {});

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);

    const auto full = GetParam() == RenderSettings::TierPreference::full;
    const auto direction = Vector3<float> (5.0f, 4.0f, 0.0f).normalized();
    const auto nearEye = direction * (full ? 3.25f : 2.75f);
    const auto farEye = direction * (full ? 3.5f : 3.0f);

    {
        SceneRenderer fresh;
        setTier (fresh, GetParam());
        aimCamera (*camera, nearEye, {});
        ASSERT_NE (fresh.render (device, scene, 64, 64), nullptr) << fresh.getLastError();
        EXPECT_GT (shadows (fresh).getShadowData().sunInfo[0], 1.0f);
        EXPECT_TRUE (fresh.waitForGPU().wasOk());
    }

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    for (int frame = 0; frame < 6; ++frame)
    {
        aimCamera (*camera, frame % 2 == 0 ? farEye : nearEye, {});
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).getShadowData().sunInfo[0], 1.0f) << "frame " << frame;
    }

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, PointLightShadowsOnAllSixFaces)
{
    checkPointLightShadowsOnAllSixFaces (GetParam());
}

TEST_P (SceneRendererTierTests, PointShadowHasNoFaceSeams)
{
    checkPointShadowHasNoFaceSeams (GetParam());
}

TEST_P (SceneRendererTierTests, SpotLightCastsAShadow)
{
    checkSpotCastsAShadow (GetParam(), 0.0f);
}

TEST_P (SceneRendererTierTests, AreaSpotCastsAShadowTowardsItsCenter)
{
    // The area light variant shadows a spot with a radius from its center, as a punctual spot
    checkSpotCastsAShadow (GetParam(), 0.1f);
    checkAreaSpotCastsAShadowTowardsItsCenter (GetParam());
}

TEST_P (SceneRendererTierTests, AtlasNeverExceedsTheShadowResolution)
{
    // The atlas side is a power of two: rounded down, so it never exceeds the setting (raster) or
    // twice the setting (full)
    const auto full = GetParam() == RenderSettings::TierPreference::full;

    Scene scene;
    buildSpotOverGround (scene);

    SceneRenderer renderer;
    auto settings = renderer.getRenderSettings();
    settings.tier = GetParam();
    settings.shadowResolution = full ? 1100 : 2100;
    renderer.setRenderSettings (settings);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (shadows (renderer).getAtlasSize(), 2048);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_P (SceneRendererTierTests, OrthographicCameraSizesTilesWhateverItsDistance)
{
    // An orthographic camera 2 units high over a 256 px viewport sees the 2 m reach of the light
    // as 256 px of radius: a 512 texel tile, as near as far
    const auto tileSizeFrom = [this] (float distance)
    {
        Scene scene;
        addGround (scene, 5.0f);

        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.range = 2.0f;
        light.castsShadows = true;
        entity->setPosition ({ 0.0f, 0.5f, 0.0f });
        scene.getRoot()->addChild (entity);

        addCamera (scene, { 0.0f, distance, 0.0f }, {}, { 0.0f, 0.0f, -1.0f });
        auto& camera = *scene.getActiveCamera();
        camera.projection = CameraNode::Projection::orthographic;
        camera.yMag = 1.0f;

        SceneRenderer renderer;
        setTier (renderer, GetParam());
        EXPECT_NE (renderer.render (device, scene, 256, 256), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        return shadows (renderer).getTileSize (0);
    };

    EXPECT_EQ (tileSizeFrom (5.0f), 512);
    EXPECT_EQ (tileSizeFrom (200.0f), 512);
}

TEST_P (SceneRendererTierTests, ManyShadowedLightsRenderWithoutError)
{
    checkManyShadowedLightsRenderWithoutError (GetParam());
}

TEST_P (SceneRendererTierTests, ShadowedLightOutOfReachAllocatesNoAtlas)
{
    checkShadowedLightOutOfReachAllocatesNoAtlas (GetParam());
}

TEST_P (SceneRendererTierTests, FailedShadowMapKeepsTheOtherOneCached)
{
    checkFailedShadowMapKeepsTheOtherOneCached (GetParam());
}

TEST_P (SceneRendererTierTests, AtlasIsReleasedWhenNoLightNeedsIt)
{
    checkAtlasIsReleasedWhenNoLightNeedsIt (GetParam());
}

TEST_P (SceneRendererTierTests, AtlasRedrawsOnlyTheViewsACasterReaches)
{
    checkAtlasRedrawsOnlyTheViewsACasterReaches (GetParam());
}

TEST_P (SceneRendererTierTests, PartiallyRedrawnAtlasMatchesAFreshOne)
{
    checkPartiallyRedrawnAtlasMatchesAFreshOne (GetParam());
}

TEST_P (SceneRendererTierTests, SingleSpotTakesHalfTheAtlas)
{
    checkSingleSpotTakesHalfTheAtlas (GetParam());
}

TEST_P (SceneRendererTierTests, LocalViewsShrinkToFitTheAtlas)
{
    checkLocalViewsShrinkToFitTheAtlas (GetParam());
}

TEST_P (SceneRendererTierTests, LocalLightsKeepTheirQuarterTiles)
{
    checkLocalLightsKeepTheirQuarterTiles (GetParam());
}

TEST_P (SceneRendererTierTests, AtlasRelayoutRedrawsTheMovedTiles)
{
    checkAtlasRelayoutRedrawsTheMovedTiles (GetParam());
}

TEST_F (SceneRendererMetalTests, TierSwitchesKeepTheShadowsRight)
{
    // One renderer through raster, full with soft shadows, raster with soft shadows, raster with an
    // unshadowed sun and full again: the cascade texture changes its layers (2 and 4) and gives way
    // to its placeholder, the soft filter variants come and go, and each frame renders as a fresh
    // renderer of its tier does
    Scene scene;
    auto& sun = buildLongGroundScene (scene, 2.0f);
    sun.radius = 0.02f;

    const struct
    {
        RenderSettings::TierPreference tier;
        bool soft;
        bool castsShadows;
    } steps[] = {
        { RenderSettings::TierPreference::raster, false, true },
        { RenderSettings::TierPreference::full, true, true },
        { RenderSettings::TierPreference::raster, true, true },
        { RenderSettings::TierPreference::raster, false, false },
        { RenderSettings::TierPreference::full, false, true },
    };

    SceneRenderer renderer;
    for (const auto& step : steps)
    {
        setTierForExactFrames (renderer, step.tier);
        auto settings = renderer.getRenderSettings();
        settings.softShadows = step.soft;
        renderer.setRenderSettings (settings);
        sun.castsShadows = step.castsShadows;

        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        ASSERT_NE (renderer.getCapabilities(), nullptr);
        EXPECT_EQ (shadows (renderer).getNumRecords(), step.castsShadows ? renderer.getCapabilities()->getCascadeCount (renderer.getRenderSettings()) : 0);
        EXPECT_EQ (shadows (renderer).needsShadowFilters(), step.soft && step.castsShadows);

        SceneRenderer fresh;
        fresh.setRenderSettings (renderer.getRenderSettings());
        const auto reference = renderToImage (fresh, scene, 128);
        ASSERT_TRUE (image.isValid() && reference.isValid());
        EXPECT_EQ (countDifferences (image, reference), 0);
    }

    EXPECT_EQ (shadows (renderer).getNumRecords(), 4);
}

TEST_P (SceneRendererTierTests, AtlasShadowMapsRenderOnlyWhenTheShadowsChange)
{
    // A still scene with a shadowed spot draws its shadow maps once; a moved spot draws them again,
    // and so does a new range, which only the views of the spot's shadow see
    Scene scene;
    auto& spot = buildSpotOverGround (scene);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    for (int frame = 0; frame < 3; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

    auto* entity = scene.getRoot()->findChild ("spot");
    ASSERT_NE (entity, nullptr);
    entity->setPosition (entity->getPosition() + Vector3<float> (0.05f, 0.0f, 0.0f));

    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 2);

    spot.range = 8.0f;
    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 3);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, ShadowedRegionIsFinite)
{
    checkShadowedRegionIsFinite (GetParam());
}
#endif

TEST_P (SceneRendererTierTests, ContactShadowsDarkenSmallFeatures)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    // Without contact shadows the sun map leaves the crease lit: its normal offset lifts the crease above the step
    float texel = 0.0f, unshadowedTexel = 0.0f;
    const auto unshadowed = renderCreaseScene (GetParam(), 256, false, false, unshadowedTexel);
    const auto without = renderCreaseScene (GetParam(), 256, false, true, texel);
    const auto withContact = renderCreaseScene (GetParam(), 256, true, true, texel);
    ASSERT_TRUE (unshadowed.isValid() && without.isValid() && withContact.isValid());

    const auto reference = meanLuminance (unshadowed, creaseRect());
    const auto lit = meanLuminance (without, creaseRect());
    const auto contact = meanLuminance (withContact, creaseRect());
    const auto openLit = meanLuminance (without, openGroundRect());
    const auto openContact = meanLuminance (withContact, openGroundRect());

    RecordProperty ("sunMapTexel", String (texel, 4).toStdString());
    RecordProperty ("creaseUnshadowed", String (reference, 2).toStdString());
    RecordProperty ("creaseMapOnly", String (lit, 2).toStdString());
    RecordProperty ("creaseContact", String (contact, 2).toStdString());
    RecordProperty ("openGroundMapOnly", String (openLit, 2).toStdString());
    RecordProperty ("openGroundContact", String (openContact, 2).toStdString());

    EXPECT_GT (lit, 0.95 * reference);
    EXPECT_LT (contact, 0.8f * lit);

    // Open ground marches without hitting anything
    EXPECT_NEAR (openContact, openLit, 0.02 * openLit);
}

TEST_P (SceneRendererTierTests, ContactShadowsIgnoredOnRaster)
{
    if (GetParam() != RenderSettings::TierPreference::raster)
        GTEST_SKIP() << "Raster only";

    checkContactShadowsIgnoredOnRaster (GetParam());
}

TEST_P (SceneRendererTierTests, ContactShadowSettingsLiveInTheFirstShadowRecord)
{
    checkContactShadowSettingsLiveInTheFirstShadowRecord (GetParam());
}

TEST_P (SceneRendererTierTests, ContactShadowOnlyRecordsHaveNoAtlasTile)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    Scene scene;
    buildCreaseScene (scene, false);

    // A shadowed point light keeps the atlas alive; the second lies inside its own emitter, so its
    // views are dropped after its tiles were allocated, and its contact shadows take a record without a map
    for (const auto radius : { 0.0f, 0.5f })
    {
        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.castsShadows = true;
        light.contactShadows = true;
        light.radius = radius;
        light.range = 0.3f;
        entity->setPosition ({ 1.0f + 2.0f * radius, 0.1f, 0.0f });
        scene.getRoot()->addChild (entity);
    }

    SceneRenderer renderer;
    setCreaseShadowMap (renderer);
    setTier (renderer, GetParam());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    const auto data = shadows (renderer).getShadowData();
    const auto last = shadows (renderer).getNumRecords() - 1;
    ASSERT_GT (last, 0);
    EXPECT_EQ (data.records[last].atlasRect[2], 0.0f);
    EXPECT_FLOAT_EQ (data.records[last].contact[0], 0.2f);

    EXPECT_GT (shadows (renderer).getTileSize (1), 0);
    EXPECT_EQ (shadows (renderer).getTileSize (2), 0);
}

TEST_P (SceneRendererTierTests, ContactShadowRecordsGoToTheMostImportantLights)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    Scene scene;
    buildCreaseScene (scene, true);

    // One record more than the budget leaves: 63 small far lights listed first, then a large one
    const auto addPoint = [&] (Vector3<float> position, float range, float distance)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.contactShadows = true;
        light.contactShadowDistance = distance;
        light.range = range;
        entity->setPosition (position);
        scene.getRoot()->addChild (entity);
    };

    for (int i = 0; i < GpuShadowData::maxRecords - 1; ++i)
        addPoint ({ -5.0f - 0.1f * static_cast<float> (i), 0.1f, -10.0f }, 0.05f, 0.1f);

    addPoint ({ 0.5f, 0.5f, 0.0f }, 30.0f, 0.3f);

    SceneRenderer renderer;
    setCreaseShadowMap (renderer);
    setTier (renderer, GetParam());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    const auto data = shadows (renderer).getShadowData();
    ASSERT_EQ (shadows (renderer).getNumRecords(), GpuShadowData::maxRecords);
    EXPECT_TRUE (std::any_of (std::begin (data.records), std::end (data.records), [] (const GpuShadowRecord& record)
    {
        return record.contact[0] == 0.3f;
    }));
}

TEST_P (SceneRendererTierTests, ContactShadowsSpareAreaLightsBelowTheHorizon)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    // A sphere light whose center lies 0.1 below the ground, its top poking 0.3 above it: the ray
    // towards its center runs into the ground, which must not shadow what the sphere lights
    const auto render = [&] (bool contact)
    {
        Scene scene;
        addGround (scene, 5.0f);
        addCamera (scene, { 0.0f, 2.0f, 1.5f }, {});

        auto entity = EntityNode::Ptr (new EntityNode ("sphere"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.intensity = 5.0f;
        light.radius = 0.4f;
        light.contactShadows = contact;
        entity->setPosition ({ 0.0f, -0.1f, 0.0f });
        scene.getRoot()->addChild (entity);

        SceneRenderer renderer;
        setTier (renderer, GetParam());
        const auto image = renderConverged (renderer, scene, 128);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto without = render (false);
    const auto withContact = render (true);
    ASSERT_TRUE (without.isValid() && withContact.isValid());

    const Rectangle<int> around (40, 40, 48, 48);
    const auto lit = meanLuminance (without, around);
    const auto contact = meanLuminance (withContact, around);
    RecordProperty ("lit", String (lit, 2).toStdString());
    RecordProperty ("contact", String (contact, 2).toStdString());

    ASSERT_GT (lit, 20.0);
    EXPECT_GT (contact, 0.98 * lit);
}

TEST_P (SceneRendererTierTests, PcssPenumbraWidensWithLightRadius)
{
    checkPcssPenumbraWidensWithLightRadius (GetParam());
}

TEST_P (SceneRendererTierTests, PenumbraWidensWithDistanceFromTheBlocker)
{
    checkPenumbraWidensWithDistanceFromTheBlocker (GetParam());
}

TEST_P (SceneRendererTierTests, RasterSoftShadowsOptIn)
{
    if (GetParam() != RenderSettings::TierPreference::raster)
        GTEST_SKIP() << "Raster only";

    checkRasterSoftShadowsOptIn();
}

TEST_P (SceneRendererTierTests, SpotPenumbraWidensWithLightRadius)
{
    checkSpotPenumbraWidensWithLightRadius (GetParam());
}

TEST_P (SceneRendererTierTests, SoftShadowsFilterAtLeastTwoTexels)
{
    checkSoftShadowsFilterAtLeastTwoTexels (GetParam());
}

TEST_P (SceneRendererTierTests, SoftPointShadowHasNoFaceSeams)
{
    checkPointShadowHasNoFaceSeams (GetParam(), 0.1f);
}

TEST_P (SceneRendererTierTests, SoftSunLeavesAGrazedGroundLit)
{
    checkSoftSunLeavesAGrazedGroundLit (GetParam());
}

TEST_P (SceneRendererTierTests, RasterSoftShadowVariantFollowsTheSettings)
{
    if (GetParam() != RenderSettings::TierPreference::raster)
        GTEST_SKIP() << "Raster only: the helper builds a raster scene";

    checkRasterSoftShadowVariantFollowsTheSettings();
}

TEST_P (SceneRendererTierTests, SoftShadowsConvergeToAStableImage)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full only: raster never accumulates";

    // The rotated taps average out over the accumulation: a renderer that starts 7 frames later,
    // with other jitter offsets and tap rotations, converges to nearly the same image, and the
    // penumbra rises from the shadow to the light without a step back. The accumulation is a true
    // mean of taaSamples frames, so what is left is the statistical spread of two different sets
    // of 16 x 8 rotated taps over the widest penumbra: measured at 452 pixels beyond the 2 levels
    // countDifferences() ignores, at most 5 levels apart (6 pixels with 64 samples). Unresolved
    // noise, one rotated tap per frame, leaves 3612 pixels up to 21 levels apart
    const auto first = renderPenumbraScene (GetParam(), 0.05f, true);
    const auto second = renderPenumbraScene (GetParam(), 0.05f, true, 7);
    ASSERT_TRUE (first.isValid() && second.isValid());

    const auto differing = countDifferences (first, second);
    const auto largest = maxChannelDifference (first, second);
    RecordProperty ("differing", differing);
    RecordProperty ("largestDifference", largest);
    EXPECT_LE (differing, 600);
    EXPECT_LE (largest, 7);

    const auto row = luminanceRow (first, first.getHeight() / 2);
    const auto rising = row.back() > row.front();

    auto stepBack = 0.0;
    for (size_t x = 1; x < row.size(); ++x)
        stepBack = jmax (stepBack, rising ? row[x - 1] - row[x] : row[x] - row[x - 1]);

    const auto width = penumbraWidth (first);
    RecordProperty ("stepBack", String (stepBack, 2).toStdString());
    RecordProperty ("width", width);
    EXPECT_LE (stepBack, 2.0);
    EXPECT_GT (width, 8); // A soft penumbra, not the hard filter's couple of pixels
}

TEST_P (SceneRendererTierTests, SoftShadowsWithoutTaaHoldStill)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full only: raster has no temporal anti-aliasing to turn off";

    // Without temporal anti-aliasing nothing averages a moving pattern: the soft shadow taps keep
    // a still rotation, so consecutive frames match, and the penumbra is still soft
    const auto render = [&] (bool softShadows, Image& next)
    {
        Scene scene;
        buildPenumbraScene (scene, 0.05f);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 1);
        auto settings = renderer.getRenderSettings();
        settings.softShadows = softShadows;
        renderer.setRenderSettings (settings);

        const auto image = renderToImage (renderer, scene, 256);
        next = renderToImage (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    Image soft, softNext, hard, hardNext;
    soft = render (true, softNext);
    hard = render (false, hardNext);
    ASSERT_TRUE (soft.isValid() && softNext.isValid() && hard.isValid());

    // The canvas read back dithers the frames by a level, within what countDifferences() ignores
    const auto flickering = countDifferences (soft, softNext);
    const auto softWidth = penumbraWidth (soft);
    const auto hardWidth = penumbraWidth (hard);
    RecordProperty ("flickering", flickering);
    RecordProperty ("soft", softWidth);
    RecordProperty ("hard", hardWidth);
    EXPECT_EQ (flickering, 0);
    EXPECT_GT (softWidth, hardWidth + 4);
}

TEST_P (SceneRendererTierTests, ShadowFilterVariantFollowsSoftAndContactShadows)
{
    checkShadowFilterVariantFollowsSoftAndContactShadows (GetParam());
}

TEST_P (SceneRendererTierTests, ContactOnlyLightWithARadiusLeavesSoftFilteringOff)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    // A light with contact shadows but no shadow map holds a record without a map: its radius has
    // no shadow to soften, so the frame asks for no soft filtering, while its contact shadows
    // still select the shadow filter variant. The sun casts a hard shadow (radius 0)
    Scene scene;
    buildCreaseScene (scene, false);

    auto entity = EntityNode::Ptr (new EntityNode ("point"));
    auto& light = entity->attach<LightNode> (LightNode::Type::point);
    light.contactShadows = true;
    light.radius = 0.1f;
    entity->setPosition ({ 0.5f, 0.5f, 0.0f });
    scene.getRoot()->addChild (entity);

    SceneRenderer renderer;
    setCreaseShadowMap (renderer);
    setTier (renderer, GetParam());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    const auto data = shadows (renderer).getShadowData();
    const auto last = shadows (renderer).getNumRecords() - 1;
    ASSERT_GT (last, 0);
    EXPECT_EQ (data.records[last].atlasRect[2], 0.0f);
    EXPECT_GT (data.records[last].contact[0], 0.0f);

    EXPECT_EQ (data.filterInfo[0], 0.0f);
    EXPECT_EQ (data.filterInfo[1], 0.0f);
    EXPECT_TRUE (shadows (renderer).needsShadowFilters());
}

TEST_P (SceneRendererTierTests, ContactShadowsOfALightOutOfRangeChangeNothing)
{
    if (GetParam() != RenderSettings::TierPreference::full)
        GTEST_SKIP() << "Full tier only";

    // A point light just above the step of buildCreaseScene(), on the sun's side. Within its range
    // its contact shadows darken the ground behind the step. Out of its range it lights nothing,
    // and its contact record, still allocated, leaves the frame as it is without contact shadows,
    // although that frame shades with the shadow filter variant and the other with the default
    const auto render = [&] (float range, bool contact)
    {
        Scene scene;
        buildCreaseScene (scene, false);

        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.contactShadows = contact;
        light.range = range;
        entity->setPosition ({ -0.5f, 0.15f, 0.0f });
        scene.getRoot()->addChild (entity);

        SceneRenderer renderer;
        setCreaseShadowMap (renderer);
        setTier (renderer, GetParam());
        const auto image = renderConverged (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).needsShadowFilters(), contact);

        const auto data = shadows (renderer).getShadowData();
        const auto marches = std::any_of (std::begin (data.records), std::end (data.records), [] (const GpuShadowRecord& record)
        {
            return record.contact[0] > 0.0f;
        });

        EXPECT_EQ (marches, contact);
        return image;
    };

    const auto inRange = render (2.0f, false);
    const auto inRangeContact = render (2.0f, true);
    const auto outOfRange = render (0.08f, false);
    const auto outOfRangeContact = render (0.08f, true);
    ASSERT_TRUE (inRange.isValid() && inRangeContact.isValid() && outOfRange.isValid() && outOfRangeContact.isValid());

    const auto darkened = countDifferences (inRangeContact, inRange);
    const auto changed = countDifferences (outOfRangeContact, outOfRange);
    RecordProperty ("inRangeDarkened", darkened);
    RecordProperty ("outOfRangeChanged", changed);

    EXPECT_GT (darkened, 0);
    EXPECT_EQ (changed, 0);
}

TEST_P (SceneRendererTierTests, ClearcoatAddsASharpHighlightOnARoughBase)
{
    checkClearcoatAddsASharpHighlightOnARoughBase (GetParam());
}

TEST_P (SceneRendererTierTests, ClearcoatIsWidenedOverCurvedPixels)
{
    checkClearcoatIsWidenedOverCurvedPixels (GetParam());
}

TEST_P (SceneRendererTierTests, SheenBrightensGrazingEdges)
{
    checkSheenBrightensGrazingEdges (GetParam());
}

TEST_P (SceneRendererTierTests, SheenPassesTheWhiteFurnaceTest)
{
    checkSheenPassesTheWhiteFurnaceTest (GetParam());
}

TEST_P (SceneRendererTierTests, LayeredVariantFollowsTheMaterials)
{
    checkLayeredVariantFollowsTheMaterials (GetParam());
}

TEST_P (SceneRendererTierTests, FailedLayersVariantShadesWithoutTheLayers)
{
    checkFailedLayersVariantShadesWithoutTheLayers (GetParam());
}

TEST_P (SceneRendererTierTests, SheenOverAWhiteBaseKeepsTheEnergy)
{
    checkSheenOverAWhiteBaseKeepsTheEnergy (GetParam());
}

TEST_P (SceneRendererTierTests, ClearcoatDimsTheEmission)
{
    checkClearcoatDimsTheEmission (GetParam());
}

TEST_P (SceneRendererTierTests, ClearcoatReflectsTheProbe)
{
    checkClearcoatReflectsTheProbe (GetParam());
}

TEST_F (SceneRendererMetalTests, BlendedLayeredItemShadesOnFull)
{
    // The full forward pass shades blended items, and always compiles the layers: a half transparent
    // plate with a clearcoat, under the head-on sun and a sky, gains the coat's highlight there, and
    // stays finite. The lighting pass shades no layer, the plate being its only item
    const auto render = [this] (float clearcoat, GpuTexture::Ptr* sceneColor)
    {
        auto material = makeLayeredMaterial ({ 0.5f, 0.5f, 0.5f }, clearcoat, 0.2f, {}, 0.0f);
        material->baseColorFactor[3] = 0.5f;
        material->alphaMode = Material::AlphaMode::blend;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& d) { return Vector3<float> (0.2f, 0.2f, 0.2f) * (d.getY() * 0.5f + 0.5f); }, 32, 3));

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 3.0f;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 1);
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_FALSE (renderer.getLightingPass() != nullptr && renderer.getLightingPass()->usesMaterialLayers());

        if (sceneColor != nullptr)
            *sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);

        return image;
    };

    GpuTexture::Ptr sceneColor;
    const auto bare = render (0.0f, nullptr);
    const auto coated = render (1.0f, &sceneColor);
    ASSERT_TRUE (bare.isValid() && coated.isValid());

    const auto bareLuminance = meanLuminance (bare, { 28, 28, 8, 8 });
    const auto coatedLuminance = meanLuminance (coated, { 28, 28, 8, 8 });
    RecordProperty ("bare", String (bareLuminance, 1).toStdString());
    RecordProperty ("coated", String (coatedLuminance, 1).toStdString());
    EXPECT_GT (coatedLuminance, bareLuminance + 10.0);

#if YUP_ENABLE_SHADER_TRANSPILER
    ASSERT_NE (sceneColor, nullptr);
    EXPECT_TRUE (isFiniteTexture (sceneColor));
    EXPECT_FALSE (isSaturatedTexture (sceneColor));
#endif
}

TEST_F (SceneRendererMetalTests, ClearcoatShadesAlikeOnBothTiers)
{
    // A clearcoated sphere without a normal map, where the clearcoat normal of both tiers is the
    // surface normal, under the head-on sun and a sky: its direct and indirect lobes. The tolerance
    // of 6 levels is T7's tier parity one (the full tier's shading normal and the raster MSAA)
    const auto render = [this] (RenderSettings::TierPreference tier, float clearcoat)
    {
        Scene scene;
        addSphere (scene, 1.5f, makeLayeredMaterial ({ 0.6f, 0.1f, 0.1f }, clearcoat, 0.3f, {}, 0.0f));
        scene.setAmbientColor (Colors::black);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& direction)
        {
            const auto up = jlimit (0.0f, 1.0f, direction.getY() * 0.5f + 0.5f);
            return Vector3<float> (0.1f, 0.1f, 0.1f) + Vector3<float> (1.0f, 1.2f, 1.5f) * up;
        }, 32, 3));

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 2.0f;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        const auto image = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto raster = render (RenderSettings::TierPreference::raster, 1.0f);
    const auto full = render (RenderSettings::TierPreference::full, 1.0f);
    const auto uncoated = render (RenderSettings::TierPreference::raster, 0.0f);
    ASSERT_TRUE (raster.isValid() && full.isValid() && uncoated.isValid());

    // The highlight, the lit body, the upper and lower grazing edges reflecting the sky and the ground
    const Rectangle<int> regions[] = { { 60, 60, 8, 8 }, { 80, 56, 8, 16 }, { 60, 24, 8, 6 }, { 60, 98, 8, 6 } };
    auto coatChanges = 0;

    for (const auto& region : regions)
    {
        const auto expected = meanColor (raster, region);
        const auto actual = meanColor (full, region);
        RecordProperty (("raster" + region.toString()).toStdString(), expected.toString().toStdString());
        RecordProperty (("full" + region.toString()).toStdString(), actual.toString().toStdString());

        EXPECT_NEAR (actual.getRed(), expected.getRed(), 6) << region.toString();
        EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 6) << region.toString();
        EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 6) << region.toString();

        // Not vacuous: the clearcoat changes the region
        const auto bare = meanColor (uncoated, region);
        RecordProperty (("uncoated" + region.toString()).toStdString(), bare.toString().toStdString());
        coatChanges += std::abs (expected.getGreen() - bare.getGreen()) > 10 ? 1 : 0;
    }

    // The highlight and the sky reflected at the upper edge, at least
    EXPECT_GE (coatChanges, 2);
}

TEST_F (SceneRendererMetalTests, MaterialLayerChangesRestartAccumulation)
{
    // Preflight R11: each clearcoat and sheen field shows on the full tier once converged
    const std::pair<const char*, std::function<void (Material&)>> changes[] = {
        { "clearcoatFactor", [] (Material& material) { material.clearcoatFactor = 0.25f; } },
        { "clearcoatRoughnessFactor", [] (Material& material) { material.clearcoatRoughnessFactor = 0.6f; } },
        { "sheenColorFactor", [] (Material& material) { material.sheenColorFactor[1] = 0.5f; } },
        { "sheenRoughnessFactor", [] (Material& material) { material.sheenRoughnessFactor = 0.9f; } },
    };

    for (const auto& [name, change] : changes)
    {
        auto material = makeLayeredMaterial ({ 0.5f, 0.5f, 0.5f }, 1.0f, 0.2f, { 1.0f, 1.0f, 1.0f }, 0.3f);

        Scene scene;
        addPlate (scene, material);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 4);
        for (int i = 0; i < 4; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*material);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }
}
#endif
