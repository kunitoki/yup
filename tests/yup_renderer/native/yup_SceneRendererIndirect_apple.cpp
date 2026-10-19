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

TEST_F (SceneRendererMetalTests, RendersEnvironmentShadowsAndBackground)
{
    Scene scene;
    scene.getRoot()->addChild (makePlates());

    // Shining down -Z, tilted so the shadow of the small plate falls beside it
    auto sun = EntityNode::Ptr (new EntityNode ("sun"));
    sun->attach<LightNode>().castsShadows = true;
    sun->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));
    scene.getRoot()->addChild (sun);

    scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    scene.getRoot()->getNode<EnvironmentNode>()->visible = true;
    scene.getRoot()->attach<ToneMappingNode> (ToneMappingOperator::aces);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
    ASSERT_TRUE (image.isValid());

    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    const auto corner = image.getPixelColor (1, 1);
    EXPECT_NEAR (corner.getRed(), 206, 3);
    EXPECT_NEAR (corner.getGreen(), 206, 3);
    EXPECT_NEAR (corner.getBlue(), 206, 3);
}

TEST_F (SceneRendererMetalTests, RgbmEnvironmentShadesLikeTheFloatOne)
{
    // Below the float floor the renderer's environment cube holds RGBM, and LightingBlock.probeInfo
    // tells the background and the surfaces to decode it. Undecoded, the corner would show 0.996, not 0.5
    const auto render = [this] (bool rgbm)
    {
        Scene scene;
        scene.getRoot()->addChild (makePlates());
        scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.getRoot()->getNode<EnvironmentNode>()->visible = true;
        scene.getRoot()->attach<ToneMappingNode> (ToneMappingOperator::aces);

        SceneRenderer renderer;
        if (rgbm)
            renderer.forceRgbmRadianceForTesting();

        auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto floatImage = render (false);
    const auto rgbmImage = render (true);
    ASSERT_TRUE (floatImage.isValid());
    ASSERT_TRUE (rgbmImage.isValid());

    EXPECT_NEAR (rgbmImage.getPixelColor (1, 1).getRed(), 206, 1);
    EXPECT_NEAR (floatImage.getPixelColor (1, 1).getRed(), 206, 1);

    // The plates in the middle reflect the environment
    const Rectangle<int> plates (24, 24, 16, 16);
    EXPECT_NEAR (meanLuminance (rgbmImage, plates), meanLuminance (floatImage, plates), 0.02 * meanLuminance (floatImage, plates));
}

TEST_F (SceneRendererMetalTests, RadianceFormatChangeRebuildsTheCube)
{
    // One source of truth: when the renderer's radiance format changes, the cube it built is
    // dropped and built again in the new format, never decoded with the other one
    Scene scene;
    scene.getRoot()->addChild (makePlates());
    scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    scene.getRoot()->getNode<EnvironmentNode>()->visible = true;
    scene.getRoot()->attach<ToneMappingNode> (ToneMappingOperator::aces);

    SceneRenderer renderer;
    const auto floatImage = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (floatImage.isValid()) << renderer.getLastError();
    EXPECT_NEAR (floatImage.getPixelColor (1, 1).getRed(), 206, 1);

    renderer.forceRgbmRadianceForTesting();
    const auto rgbmImage = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (rgbmImage.isValid()) << renderer.getLastError();
    EXPECT_NEAR (rgbmImage.getPixelColor (1, 1).getRed(), 206, 1);
}

TEST_F (SceneRendererMetalTests, OneEnvironmentDrawnByTwoRenderers)
{
    // Each renderer keeps the cube of its own device: drawing the same environment alternately
    // builds nothing again and restarts no convergence, and nothing stays behind once they are gone
    auto otherDevice = GpuDevice::create (GpuPlatform::Metal, {});
    ASSERT_NE (otherDevice, nullptr);

    EnvironmentMap::Ptr environment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));

    Scene scene;
    buildEdgesScene (scene);
    scene.getRoot()->attach<EnvironmentNode> (environment);
    scene.getRoot()->getNode<EnvironmentNode>()->visible = true;

    {
        SceneRenderer first, second;
        setFullTierTaa (first, 4);
        setFullTierTaa (second, 4);
        for (int i = 0; i < 4; ++i)
        {
            ASSERT_NE (first.render (device, scene, 64, 64), nullptr) << first.getLastError();
            ASSERT_NE (second.render (otherDevice, scene, 64, 64), nullptr) << second.getLastError();
        }

        EXPECT_TRUE (first.isConverged());
        EXPECT_TRUE (second.isConverged());

        ASSERT_NE (first.render (device, scene, 64, 64), nullptr) << first.getLastError();
        ASSERT_NE (second.render (otherDevice, scene, 64, 64), nullptr) << second.getLastError();
        EXPECT_TRUE (first.isConverged());
        EXPECT_TRUE (second.isConverged());

        // The test, the scene and one slot per renderer
        EXPECT_EQ (environment->getReferenceCount(), 4);

        EXPECT_TRUE (first.waitForGPU().wasOk());
        EXPECT_TRUE (second.waitForGPU().wasOk());
    }

    EXPECT_EQ (environment->getReferenceCount(), 2);
}

TEST_F (SceneRendererMetalTests, EnvironmentChangeDropsTheOldCube)
{
    EnvironmentMap::Ptr first (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    EnvironmentMap::Ptr second (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.25f, 0.25f, 0.25f); }, 32, 3));

    Scene scene;
    buildEdgesScene (scene);
    scene.getRoot()->attach<EnvironmentNode> (first);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (first->getReferenceCount(), 3); // The test, the scene and the renderer's slot

    scene.getRoot()->attach<EnvironmentNode> (second);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (first->getReferenceCount(), 1);
    EXPECT_EQ (second->getReferenceCount(), 3);

    scene.getRoot()->attach<EnvironmentNode> (nullptr);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (second->getReferenceCount(), 1);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, RebuiltEnvironmentRestartsTheFullTier)
{
    // The environment cube is built in a frame of its own, before the first frame. When the GPU fails
    // to build it, the first frame shades with an undefined cube and the next one rebuilds it: the
    // samples taken with the undefined cube must leave the running mean
    Scene scene;
    buildEdgesScene (scene);
    scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    // Frames complete in order, so once a later frame did the build has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    for (int i = 0; i < 3; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_FALSE (renderer.isConverged());

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, RepeatedEnvironmentFailureConvergesWithoutIt)
{
    // Two failed builds in a row leave the renderer without the environment: the background shows
    // instead of it, and the full tier converges instead of restarting every frame
    const auto background = Color (static_cast<uint8> (40), static_cast<uint8> (70), static_cast<uint8> (110));

    Scene scene;
    buildEdgesScene (scene);
    scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    scene.getRoot()->getNode<EnvironmentNode>()->visible = true;
    scene.getRoot()->attach<ToneMappingNode> (ToneMappingOperator::aces);
    scene.getRoot()->attach<BackgroundNode> (background);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 4);
    for (int i = 0; i < 2; ++i)
    {
        device->failNextFrameForTesting ("injected failure");
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_TRUE (renderer.isConverged());

    const auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid());
    EXPECT_TRUE (renderer.isConverged());

    // The background color, not the environment's 206
    const auto corner = image.getPixelColor (1, 1);
    EXPECT_NEAR (corner.getRed(), background.getRed(), 2);
    EXPECT_NEAR (corner.getGreen(), background.getGreen(), 2);
    EXPECT_NEAR (corner.getBlue(), background.getBlue(), 2);
}

TEST_F (SceneRendererMetalTests, AmbientOcclusionSettingsRestartAccumulation)
{
    // Each change moves the value the full tier resolves away from its default
    const std::pair<const char*, std::function<void (RenderSettings&)>> changes[] = {
        { "ambientOcclusion", [] (RenderSettings& settings) { settings.ambientOcclusion = false; } },
        { "aoDistance", [] (RenderSettings& settings) { settings.aoDistance = 0.5f; } },
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

        // Restarted, not invalidated, see ShadowSettingsRestartAccumulation
        const auto first = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_GE (countIntermediatePixels (first, 40), 4) << name;
    }
}

// The screen-space reflection tests converge 8 jittered samples at 64 x 64: the floor of
// buildOnScreenEmitterScene() is a mirror (roughness 0.02), whose rays barely spread, so the running
// mean of 8 frames holds its reflection, and the first frame, which has no history to read it from
// yet, weighs an eighth of it
TEST_F (SceneRendererMetalTests, SsrReflectsAnOnScreenEmitter)
{
    const auto render = [&] (bool screenSpaceReflections, SceneRenderer& renderer)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        setFullTierTaa (renderer, 8);
        setScreenSpaceReflections (renderer, screenSpaceReflections);

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
    // Measured 191 against 85: the threshold keeps three quarters of that margin
    EXPECT_GT (floorOn.getRed(), floorOff.getRed() + 80);
    EXPECT_NEAR (floorOn.getGreen(), floorOff.getGreen(), 2); // The emitter is red only

    EXPECT_EQ (off.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    const auto reflections = on.getPassOutput (SceneRenderer::PassOutput::reflections);
    ASSERT_NE (reflections, nullptr);
    EXPECT_EQ (reflections->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (reflections->getWidth(), 64u);

#if YUP_ENABLE_SHADER_TRANSPILER
    EXPECT_TRUE (isFiniteTexture (reflections));
    EXPECT_FALSE (isSaturatedTexture (reflections));

    const auto confidence = mapTexels (reflections, "vec4(t.aaa, 1.0)");
    ASSERT_TRUE (confidence.isValid());
    RecordProperty ("floorConfidence", String (meanLuminance (confidence, region), 1).toStdString());
    EXPECT_GT (meanLuminance (confidence, region), 200.0);
#endif
}

TEST_F (SceneRendererMetalTests, SsrConvergedRendersAreIdentical)
{
    // The rays turn with the frame only under temporal anti-aliasing, whose running mean freezes
    // once converged (preflight R13): two renderers converging on the same scene end on one image
    Scene scene;
    buildOnScreenEmitterScene (scene);

    SceneRenderer warmUp;
    setFullTierTaa (warmUp, 8);
    ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());

    SceneRenderer a, b;
    a.setRenderSettings (warmUp.getRenderSettings());
    b.setRenderSettings (warmUp.getRenderSettings());
    const auto imageA = renderConverged (a, scene, 64);
    const auto imageB = renderConverged (b, scene, 64);
    ASSERT_TRUE (imageA.isValid() && imageB.isValid());
    EXPECT_TRUE (a.isConverged() && b.isConverged());
    EXPECT_NE (a.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);

    auto differing = 0;
    for (int y = 0; y < imageA.getHeight(); ++y)
        for (int x = 0; x < imageA.getWidth(); ++x)
            differing += imageA.getPixelColor (x, y) != imageB.getPixelColor (x, y) ? 1 : 0;

    EXPECT_EQ (differing, 0);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsrWithoutHistoryShadesAsWithoutIt)
{
    // A renderer's first frame has no history to read reflections from: every pixel takes the
    // confidence 0 branch, which leaves the frame bit for bit as without them. The environment the
    // floor reflects would show any confidence taken from it
    SceneRenderer warmUp;
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());
    }

    const auto renderBits = [&] (bool screenSpaceReflections)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));

        SceneRenderer renderer;
        setFullTierTaa (renderer, 8);
        setScreenSpaceReflections (renderer, screenSpaceReflections);
        EXPECT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);

        // The 8 bytes of the half float scene color, three to an opaque rgba8 image
        std::vector<Image> bits;
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        for (const auto* bytes : { "vec4(lo.xyz, 1.0)", "vec4(lo.w, hi.xy, 1.0)", "vec4(hi.zw, 0.0, 1.0)" })
        {
            const auto* halves = "#define lo unpackUnorm4x8(packHalf2x16(t.rg))\n#define hi unpackUnorm4x8(packHalf2x16(t.ba))\n";
            bits.push_back (sceneColor != nullptr ? mapTexels (sceneColor, bytes, halves) : Image());
        }

        return bits;
    };

    const auto withReflections = renderBits (true);
    const auto without = renderBits (false);

    for (size_t part = 0; part < withReflections.size(); ++part)
    {
        ASSERT_TRUE (withReflections[part].isValid() && without[part].isValid());

        auto differing = 0;
        for (int y = 0; y < without[part].getHeight(); ++y)
            for (int x = 0; x < without[part].getWidth(); ++x)
                differing += withReflections[part].getPixelColor (x, y) != without[part].getPixelColor (x, y) ? 1 : 0;

        EXPECT_EQ (differing, 0) << part;
    }

    // The stand-in the lighting pass binds without reflections is 0 in every channel, so that branch
    // is the one every pixel takes
    const auto* lightingPass = warmUp.getLightingPass();
    ASSERT_NE (lightingPass, nullptr);
    const auto standIn = lightingPass->getReflectionsStandIn();
    ASSERT_NE (standIn, nullptr);
    const auto nonZero = mapTexels (standIn, "any(notEqual(t, vec4(0.0))) ? vec4(1.0) : vec4(0.0, 0.0, 0.0, 1.0)");
    ASSERT_TRUE (nonZero.isValid());
    EXPECT_EQ (nonZero.getPixelColor (0, 0).getRed(), 0);

    // And that branch kept the floor's reflection of the environment, which a stand-in of
    // confidence 1 would have replaced with its black: the floor is brighter than without one
    const auto floorLuminance = [&] (bool environment)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);

        if (environment)
            scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));

        SceneRenderer renderer;
        setFullTierTaa (renderer, 8);
        const auto image = renderToImage (renderer, scene, 64);
        return image.isValid() ? meanLuminance (image, reflectedEmitterRegion()) : -1.0;
    };

    const auto withEnvironment = floorLuminance (true);
    const auto withoutEnvironment = floorLuminance (false);
    RecordProperty ("floorWithEnvironment", String (withEnvironment, 1).toStdString());
    RecordProperty ("floorWithoutEnvironment", String (withoutEnvironment, 1).toStdString());
    EXPECT_GT (withEnvironment, withoutEnvironment + 20.0);
}

TEST_F (SceneRendererMetalTests, SsrScalesWithTheExposure)
{
    // The history holds exposed radiance, divided by the exposure of the frame it was rendered with:
    // converged at exposure 1 then at 2, the reflection scales like the frame without it does.
    // Dividing by nothing, or by the exposure twice, scales it by 4 or 1
    const auto renderRatio = [&] (bool screenSpaceReflections)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene, 0.3f);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 8);
        setScreenSpaceReflections (renderer, screenSpaceReflections);
        const auto meanRed = [&]
        {
            EXPECT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            return sceneColor != nullptr ? meanLinearRed (sceneColor, reflectedEmitterRegion()) : -1.0;
        };

        const auto atOne = meanRed();
        scene.getRoot()->attach<ExposureNode> (2.0f);
        const auto atTwo = meanRed();
        RecordProperty (screenSpaceReflections ? "floorRedOn" : "floorRedOff", (String (atOne, 4) + " -> " + String (atTwo, 4)).toStdString());
        return atOne > 0.0 ? atTwo / atOne : -1.0;
    };

    const auto ratioOn = renderRatio (true);
    const auto ratioOff = renderRatio (false);
    RecordProperty ("ratioOn", String (ratioOn, 3).toStdString());
    RecordProperty ("ratioOff", String (ratioOff, 3).toStdString());
    EXPECT_GT (ratioOff, 1.5);
    EXPECT_NEAR (ratioOn, ratioOff, 0.02 * ratioOff);
}
#endif

TEST_F (SceneRendererMetalTests, SsrFallsBackToProbesOffScreen)
{
    // buildMirrorFloorScene() hides its emitter above the frustum: no ray finds it on screen, so
    // the floor reflects the probe's capture of it alike with and without screen-space reflections
    const auto render = [&] (bool screenSpaceReflections, SceneRenderer& renderer)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setFullTierTaa (renderer, 8);
        setScreenSpaceReflections (renderer, screenSpaceReflections);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    SceneRenderer on, off;
    const auto withReflections = render (true, on);
    const auto without = render (false, off);
    ASSERT_TRUE (withReflections.isValid() && without.isValid());

    const auto floorOn = meanColor (withReflections, floorRegion());
    const auto floorOff = meanColor (without, floorRegion());
    RecordProperty ("floorGreenOn", static_cast<int> (floorOn.getGreen()));
    RecordProperty ("floorGreenOff", static_cast<int> (floorOff.getGreen()));
    EXPECT_GT (floorOn.getGreen(), floorOn.getRed() + 40);
    EXPECT_GT (floorOff.getGreen(), floorOff.getRed() + 40);
    EXPECT_NEAR (floorOn.getRed(), floorOff.getRed(), 2);
    EXPECT_NEAR (floorOn.getGreen(), floorOff.getGreen(), 2);
    EXPECT_NEAR (floorOn.getBlue(), floorOff.getBlue(), 2);

    // The rays ran and missed
    const auto reflections = on.getPassOutput (SceneRenderer::PassOutput::reflections);
    ASSERT_NE (reflections, nullptr);

#if YUP_ENABLE_SHADER_TRANSPILER
    const auto confidence = mapTexels (reflections, "vec4(t.aaa, 1.0)");
    ASSERT_TRUE (confidence.isValid());
    EXPECT_LT (meanLuminance (confidence, floorRegion()), 1.0);
#endif
}

TEST_F (SceneRendererMetalTests, SsrSettingRestartsAccumulation)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Restarted, not invalidated: the history stays valid for the reflections to come back at once
    setScreenSpaceReflections (renderer, false);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);

    setScreenSpaceReflections (renderer, std::nullopt);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsrConvergedRegionMatchesAPrimedHistory)
{
    // The first frame has no history to read reflections from, and enters the running mean without
    // them. Temporal anti-aliasing clips it to the neighborhood of the next frames, which hold the
    // reflection, so the converged reflection keeps no share of it: it matches a renderer whose
    // history was primed by an earlier convergence, then restarted by a setting that changes
    // nothing on screen (contact shadows, without a light)
    const auto renderRed = [&] (bool primed)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene, 0.3f);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 8);
        EXPECT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();

        if (primed)
        {
            auto settings = renderer.getRenderSettings();
            settings.contactShadows = false;
            renderer.setRenderSettings (settings);
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr);
            EXPECT_FALSE (renderer.isConverged());
            EXPECT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
        }

        EXPECT_TRUE (renderer.isConverged());
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        return sceneColor != nullptr ? meanLinearRed (sceneColor, reflectedEmitterRegion()) : -1.0;
    };

    const auto fresh = renderRed (false);
    const auto primed = renderRed (true);
    RecordProperty ("floorRedFresh", String (fresh, 4).toStdString());
    RecordProperty ("floorRedPrimed", String (primed, 4).toStdString());

    // The reflection is 0.3 of the 0.4 the floor shows: a missing eighth of it would be 0.0375,
    // 9% of the floor. The rest of the frame is the same in both, up to the noise of the rays
    EXPECT_GT (fresh, 0.3);
    EXPECT_NEAR (fresh, primed, 0.01 * primed);
}
#endif

TEST_F (SceneRendererMetalTests, GivingUpSsrRestartsAccumulation)
{
    // Reflections that stop failing twice in a row change the image: a converged renderer starts over
    Scene scene;
    buildOnScreenEmitterScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    ASSERT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);

    renderer.failScreenSpaceReflectionsForTesting();

    for (const auto givesUp : { false, true })
    {
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
        ASSERT_NE (renderer.getReflectionPass(), nullptr);
        EXPECT_EQ (renderer.getReflectionPass()->hasGivenUp(), givesUp);
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsrDoesNotLeakAcrossDepthEdges)
{
    // A rough bar floats in front of the reflecting floor, across rows 32 to 34 of the reflection.
    // The bar traces no rays, so a floor pixel next to it would count the bar's texels as misses if
    // the resolve did not leave out the texels of another depth and normal: its confidence would
    // fall along the bar's edges. With them left out, the rows along the bar keep the confidence of
    // the rows further from it
    Scene scene;
    buildOnScreenEmitterScene (scene);

    std::vector<Mesh::Vertex> vertices (4);
    vertices[0].position = { -1.0f, 0.2f, 0.5f };
    vertices[1].position = { 1.0f, 0.2f, 0.5f };
    vertices[2].position = { 1.0f, 0.3f, 0.5f };
    vertices[3].position = { -1.0f, 0.3f, 0.5f };
    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    auto rough = Material::Ptr (new Material());
    rough->baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
    rough->metallicFactor = 0.0f;
    rough->roughnessFactor = 1.0f;

    auto mesh = Mesh::Ptr (new Mesh ("bar"));
    mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, rough);
    auto bar = EntityNode::Ptr (new EntityNode ("bar"));
    bar->attach<MeshNode> (mesh);
    scene.getRoot()->addChild (bar);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    ASSERT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
    const auto reflections = renderer.getPassOutput (SceneRenderer::PassOutput::reflections);
    ASSERT_NE (reflections, nullptr);

    const auto confidence = mapTexels (reflections, "vec4(t.aaa, 1.0)");
    ASSERT_TRUE (confidence.isValid());

    const auto edges = (meanLuminance (confidence, { 24, 31, 16, 1 }) + meanLuminance (confidence, { 24, 35, 16, 1 })) * 0.5;
    const auto inside = (meanLuminance (confidence, { 24, 28, 16, 2 }) + meanLuminance (confidence, { 24, 37, 16, 1 })) / 2.0;
    RecordProperty ("edgeConfidence", String (edges, 1).toStdString());
    RecordProperty ("insideConfidence", String (inside, 1).toStdString());
    EXPECT_GT (inside, 200.0);
    EXPECT_GT (edges, 0.9 * inside);
}

TEST_F (SceneRendererMetalTests, SsrWeighsNeighborRaysByThePixelsLobe)
{
    // A mirror floor under an emitter of 4 red and green bands. The rays are traced at half
    // resolution, so a pixel reuses the rays of its neighbors, which hit the bands a pixel or so
    // away from its own reflection: weighed by its lobe over the rays' pdf, the nearest of them
    // dominates and the reflected bands stay as distinct as the emitter's own, where a plain mean of
    // the 4 rays would blend each band into the next
    Scene scene;
    buildMirrorFloorScene (scene, false);

    for (int band = 0; band < 4; ++band)
    {
        std::vector<Mesh::Vertex> vertices (4);
        const auto bottom = 0.2f * static_cast<float> (band);
        vertices[0].position = { -0.6f, bottom, -0.5f };
        vertices[1].position = { 0.6f, bottom, -0.5f };
        vertices[2].position = { 0.6f, bottom + 0.2f, -0.5f };
        vertices[3].position = { -0.6f, bottom + 0.2f, -0.5f };
        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        const auto red = band % 2 == 0;
        auto mesh = Mesh::Ptr (new Mesh ("band"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeEmitterMaterial ({ red ? 1.0f : 0.0f, red ? 0.0f : 1.0f, 0.0f }));
        auto entity = EntityNode::Ptr (new EntityNode ("band"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
    }

    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    const auto image = renderConverged (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    // The mean red-green difference over columns 24 to 39 of rows from first to last
    const auto contrast = [&] (int first, int last)
    {
        auto sum = 0.0;
        for (int y = first; y <= last; ++y)
            for (int x = 24; x < 40; ++x)
                sum += std::abs (static_cast<int> (image.getPixelColor (x, y).getRed()) - static_cast<int> (image.getPixelColor (x, y).getGreen()));

        return sum / (16.0 * (last - first + 1));
    };

    const auto emitter = contrast (10, 24);
    const auto reflected = contrast (28, 38);
    RecordProperty ("emitterContrast", String (emitter, 1).toStdString());
    RecordProperty ("reflectedContrast", String (reflected, 1).toStdString());

    // Measured 56.7 of the emitter's 166.2 (0.34), and 49.0 (0.29) with a plain mean of the rays
    EXPECT_GT (reflected, 0.32 * emitter);
}
#endif

TEST_F (SceneRendererMetalTests, SsrOffReleasesItsTargets)
{
    Scene scene;
    buildOnScreenEmitterScene (scene);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 8);
    EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getReflectionPass(), nullptr); // Nothing to read on the first frame, nothing created

    EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    const auto* pass = renderer.getReflectionPass();
    ASSERT_NE (pass, nullptr);
    EXPECT_GT (pass->getNumCachedTargets(), 0u);

    const auto* hiZ = renderer.getHiZPyramid();
    ASSERT_NE (hiZ, nullptr);
    EXPECT_GT (hiZ->getNumCachedTargets(), 0u);

    // Kept, with no target, so turning them back on compiles nothing
    setScreenSpaceReflections (renderer, false);
    EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getReflectionPass(), pass);
    EXPECT_EQ (pass->getNumCachedTargets(), 0u);
    EXPECT_EQ (hiZ->getNumCachedTargets(), 0u);

    // Back on, then a resize: the new size has no history yet, and keeps no target of the old one
    setScreenSpaceReflections (renderer, std::nullopt);
    EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_GT (hiZ->getNumCachedTargets(), 0u);

    EXPECT_NE (renderer.render (device, scene, 48, 48), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getHiZPyramid(), hiZ);
    EXPECT_EQ (pass->getNumCachedTargets(), 0u);
    EXPECT_EQ (hiZ->getNumCachedTargets(), 0u);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, FailedSsrShadesWithoutIt)
{
    // Pipelines that cannot compile never fail the frame: it shades as without the reflections,
    // the renderer tries once more, then gives up
    const auto render = [&] (bool fail, std::optional<bool> screenSpaceReflections, SceneRenderer& renderer)
    {
        Scene scene;
        buildOnScreenEmitterScene (scene);
        setFullTierTaa (renderer, 8);
        setScreenSpaceReflections (renderer, screenSpaceReflections);

        if (fail)
            renderer.failScreenSpaceReflectionsForTesting();

        std::vector<bool> givenUp;
        for (int frame = 0; frame < 3; ++frame)
        {
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
            givenUp.push_back (renderer.getReflectionPass() != nullptr && renderer.getReflectionPass()->hasGivenUp());
        }

        // The first frame has no history to trace from, the next two fail
        EXPECT_EQ (givenUp, (std::vector<bool> { false, false, fail }));
        return renderConverged (renderer, scene, 64);
    };

    SceneRenderer failing, off;
    const auto image = render (true, std::nullopt, failing);
    const auto without = render (false, false, off);
    ASSERT_TRUE (image.isValid() && without.isValid());
    EXPECT_TRUE (failing.getLastError().isEmpty()) << failing.getLastError();

    // Giving up restarted the accumulation of the failing renderer, so its silhouettes converged
    // over other jitter offsets: the floor matches, the edges may not to the level
    const auto floorFailing = meanColor (image, reflectedEmitterRegion());
    const auto floorWithout = meanColor (without, reflectedEmitterRegion());
    EXPECT_NEAR (floorFailing.getRed(), floorWithout.getRed(), 1);
    EXPECT_NEAR (floorFailing.getGreen(), floorWithout.getGreen(), 1);
}

TEST_F (SceneRendererMetalTests, SsrUnavailableOnRasterRendersFallback)
{
    checkScreenSpaceReflectionsUnavailableOnRasterRendersFallback (RenderSettings::TierPreference::raster);
}

// The screen-space diffuse GI tests converge 8 jittered samples at 64 x 64 on the full tier. The GI
// reads the TAA history, so the first frame of a renderer gathers nothing (preflight R3)
TEST_F (SceneRendererMetalTests, ScreenSpaceGIBouncesColorOntoTheFloor)
{
    // The red room without a volume: the floor in front of the red wall shows the gray environment
    // alone without GI, and with it also the red the wall emits
    const auto render = [&] (bool screenSpaceGI, SceneRenderer& renderer)
    {
        Scene scene;
        buildGIRedRoomScene (scene);
        setScreenSpaceGI (renderer, screenSpaceGI);

        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged());
        return image;
    };

    SceneRenderer on, off;
    const auto withGI = render (true, on);
    const auto without = render (false, off);
    ASSERT_TRUE (withGI.isValid() && without.isValid());

    // The floor right in front of the wall, below its bottom edge
    const auto [top, bottom] = findRedWallRows (without);
    ASSERT_GE (top, 0);
    ASSERT_LT (bottom, 56);
    const Rectangle<int> floor { 16, bottom + 2, 32, 6 };

    const auto floorOn = meanColor (withGI, floor);
    const auto floorOff = meanColor (without, floor);
    RecordProperty ("wallRows", (String (top) + " to " + String (bottom)).toStdString());
    RecordProperty ("floorOn", floorOn.toString().toStdString());
    RecordProperty ("floorOff", floorOff.toString().toStdString());
    RecordProperty ("ratioOn", String (redOverGreen (floorOn), 3).toStdString());
    RecordProperty ("ratioOff", String (redOverGreen (floorOff), 3).toStdString());
    EXPECT_NEAR (redOverGreen (floorOff), 1.0f, 0.05f);
    // Measured 1.232 against 1.000: the threshold keeps three quarters of that margin
    EXPECT_GT (redOverGreen (floorOn), redOverGreen (floorOff) + 0.17f);

    // The gathered light adds to the environment's: a mix of the two would trade some of the
    // gray for the wall's red, and the green would drop
    EXPECT_GE (static_cast<int> (floorOn.getGreen()), static_cast<int> (floorOff.getGreen()) - 1);

    // At the default ambient occlusion distance of 0.2 m, for the record
    {
        Scene scene;
        buildGIRedRoomScene (scene);

        SceneRenderer renderer;
        setScreenSpaceGI (renderer, true);
        auto settings = renderer.getRenderSettings();
        settings.aoDistance = 0.2f;
        renderer.setRenderSettings (settings);
        const auto image = renderConverged (renderer, scene, 64);
        RecordProperty ("ratioOnDefaultDistance", String (redOverGreen (meanColor (image, floor)), 3).toStdString());
    }

    // Only the renderer with GI keeps its output: rgba16float at half resolution
    ASSERT_NE (off.getAmbientOcclusionPass(), nullptr);
    EXPECT_EQ (off.getAmbientOcclusionPass()->getIndirectDiffuse(), nullptr);
    ASSERT_NE (on.getAmbientOcclusionPass(), nullptr);
    const auto indirect = on.getAmbientOcclusionPass()->getIndirectDiffuse();
    ASSERT_NE (indirect, nullptr);
    EXPECT_EQ (indirect->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (indirect->getWidth(), 32u);
    EXPECT_EQ (on.getAmbientOcclusionPass()->getOutput()->getFormat(), GpuTextureFormat::rgba8unorm);

#if YUP_ENABLE_SHADER_TRANSPILER
    EXPECT_TRUE (isFiniteTexture (indirect));
    EXPECT_FALSE (isSaturatedTexture (indirect));
#endif
}

TEST_F (SceneRendererMetalTests, ScreenSpaceGILeavesAnOpenPlaneAsItIs)
{
    // Nothing occludes an open plane: the gather adds nothing, so the frame is the one without GI
    // up to a level of rounding, within two at the far edge of the floor. A regression guard: the
    // samples on the plane itself are rejected as not facing the pixel, so the confidence is about
    // 0 and a mix of the probe light and the gathered light would leave the plane as it is too
    // (ScreenSpaceGIBouncesColorOntoTheFloor checks that the gathered light only adds)
    const auto render = [&] (bool screenSpaceGI, SceneRenderer& renderer)
    {
        Scene scene;
        buildFacingAwayWallScene (scene);
        setScreenSpaceGI (renderer, screenSpaceGI);
        scene.getRoot()->removeChild (scene.getRoot()->findChild ("red wall"));

        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    SceneRenderer on, off;
    const auto withGI = render (true, on);
    const auto without = render (false, off);
    ASSERT_TRUE (withGI.isValid() && without.isValid());
    ASSERT_NE (on.getAmbientOcclusionPass(), nullptr);
    EXPECT_NE (on.getAmbientOcclusionPass()->getIndirectDiffuse(), nullptr);

    // The largest difference of a channel, over the frame and below its first rows, where the far
    // edge of the floor meets the background: a sample there carries the background's tint
    const auto largestDifference = [&] (int firstRow)
    {
        auto largest = 0;
        for (int y = firstRow; y < 64; ++y)
        {
            for (int x = 0; x < 64; ++x)
            {
                const auto a = withGI.getPixelColor (x, y);
                const auto b = without.getPixelColor (x, y);
                largest = jmax (largest, std::abs (a.getRed() - b.getRed()), std::abs (a.getGreen() - b.getGreen()), std::abs (a.getBlue() - b.getBlue()));
            }
        }

        return largest;
    };

    RecordProperty ("largestDifference", largestDifference (0));
    RecordProperty ("largestDifferenceBelowTheEdge", largestDifference (6));
    EXPECT_LE (largestDifference (0), 2);
    EXPECT_LE (largestDifference (6), 1);
    EXPECT_GT (meanLuminance (without, { 0, 32, 64, 32 }), 60.0); // Not vacuous: the floor is lit
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsgiWithoutHistoryShadesAsWithoutIt)
{
    // A frame without a history to gather from traces the ambient occlusion alone, which leaves it
    // bit for bit as without GI: a renderer's first frame, and the frame after restartAccumulation(),
    // which keeps the history's textures but no longer trusts them
    SceneRenderer warmUp;
    {
        Scene scene;
        buildGIRedRoomScene (scene);
        setScreenSpaceGI (warmUp, false);
        ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());
    }

    // The 8 bytes of the half float scene color, three to an opaque rgba8 image
    const auto sceneColorBits = [&] (SceneRenderer& renderer)
    {
        std::vector<Image> bits;
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        for (const auto* bytes : { "vec4(lo.xyz, 1.0)", "vec4(lo.w, hi.xy, 1.0)", "vec4(hi.zw, 0.0, 1.0)" })
        {
            const auto* halves = "#define lo unpackUnorm4x8(packHalf2x16(t.rg))\n#define hi unpackUnorm4x8(packHalf2x16(t.ba))\n";
            bits.push_back (sceneColor != nullptr ? mapTexels (sceneColor, bytes, halves) : Image());
        }

        return bits;
    };

    const auto expectSameBits = [&] (const std::vector<Image>& withGI, const std::vector<Image>& without, const char* when)
    {
        for (size_t part = 0; part < withGI.size(); ++part)
        {
            ASSERT_TRUE (withGI[part].isValid() && without[part].isValid()) << when;

            auto differing = 0;
            for (int y = 0; y < without[part].getHeight(); ++y)
                for (int x = 0; x < without[part].getWidth(); ++x)
                    differing += withGI[part].getPixelColor (x, y) != without[part].getPixelColor (x, y) ? 1 : 0;

            EXPECT_EQ (differing, 0) << when << " " << part;
        }
    };

    // Each renderer draws as many frames, so both jitter alike, then the history is invalidated
    const auto renderBits = [&] (bool screenSpaceGI, int frames, bool restart)
    {
        Scene scene;
        buildGIRedRoomScene (scene);

        SceneRenderer renderer;
        setScreenSpaceGI (renderer, screenSpaceGI);
        for (int i = 0; i < frames; ++i)
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

        if (restart)
        {
            EXPECT_TRUE (renderer.getAmbientOcclusionPass() != nullptr && (renderer.getAmbientOcclusionPass()->getIndirectDiffuse() != nullptr) == screenSpaceGI);
            renderer.restartAccumulation();
        }

        EXPECT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getAmbientOcclusionPass() != nullptr && renderer.getAmbientOcclusionPass()->getIndirectDiffuse() == nullptr);
        return sceneColorBits (renderer);
    };

    expectSameBits (renderBits (true, 0, false), renderBits (false, 0, false), "first frame");
    expectSameBits (renderBits (true, 9, true), renderBits (false, 9, true), "after restartAccumulation()");
}
#endif

TEST_F (SceneRendererMetalTests, SsgiConvergedRendersAreIdentical)
{
    // The slices turn with the frame only under temporal anti-aliasing, whose running mean freezes
    // once converged (preflight R13): two renderers converging on the same scene end on one image
    Scene scene;
    buildGIRedRoomScene (scene);

    SceneRenderer warmUp;
    setScreenSpaceGI (warmUp, true);
    ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());

    SceneRenderer a, b;
    a.setRenderSettings (warmUp.getRenderSettings());
    b.setRenderSettings (warmUp.getRenderSettings());
    const auto imageA = renderConverged (a, scene, 64);
    const auto imageB = renderConverged (b, scene, 64);
    ASSERT_TRUE (imageA.isValid() && imageB.isValid());
    EXPECT_TRUE (a.isConverged() && b.isConverged());
    ASSERT_NE (a.getAmbientOcclusionPass(), nullptr);
    EXPECT_NE (a.getAmbientOcclusionPass()->getIndirectDiffuse(), nullptr);

    auto differing = 0;
    for (int y = 0; y < imageA.getHeight(); ++y)
        for (int x = 0; x < imageA.getWidth(); ++x)
            differing += imageA.getPixelColor (x, y) != imageB.getPixelColor (x, y) ? 1 : 0;

    EXPECT_EQ (differing, 0);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SsgiScalesWithTheExposure)
{
    // The history holds exposed radiance, divided by the exposure of the frame it was rendered with:
    // converged at exposure 0.25 then at 0.5, the floor with GI doubles like the floor without it.
    // Dividing by nothing, or by the exposure twice, would scale the gathered red by 4 or 1
    const auto renderRatio = [&] (bool screenSpaceGI)
    {
        Scene scene;
        buildGIRedRoomScene (scene);
        scene.getRoot()->attach<ExposureNode> (0.25f);

        SceneRenderer renderer;
        setScreenSpaceGI (renderer, screenSpaceGI);
        Rectangle<int> floor;
        const auto meanRed = [&]
        {
            const auto [top, bottom] = findRedWallRows (renderConverged (renderer, scene, 64));
            EXPECT_TRUE (top >= 0 && bottom < 56) << top << " " << bottom;
            floor = floor.isEmpty() ? Rectangle<int> { 16, bottom + 2, 32, 6 } : floor;

            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            return sceneColor != nullptr ? meanLinearRed (sceneColor, floor) : -1.0;
        };

        const auto atQuarter = meanRed();
        scene.getRoot()->attach<ExposureNode> (0.5f);
        const auto atHalf = meanRed();
        RecordProperty (screenSpaceGI ? "floorRedOn" : "floorRedOff", (String (atQuarter, 4) + " -> " + String (atHalf, 4)).toStdString());
        return atQuarter > 0.0 ? atHalf / atQuarter : -1.0;
    };

    const auto ratioOn = renderRatio (true);
    const auto ratioOff = renderRatio (false);
    RecordProperty ("ratioOn", String (ratioOn, 3).toStdString());
    RecordProperty ("ratioOff", String (ratioOff, 3).toStdString());
    EXPECT_GT (ratioOff, 1.5);

    // Measured 1.977 against 1.995: the gathered light, more than half of the red, changes a lot
    // from frame to frame, and the running mean of temporal anti-aliasing weighs each frame by
    // 1 / (1 + luma), more so at the higher exposure. A missing division moved the ratio to 2.459
    EXPECT_NEAR (ratioOn, ratioOff, 0.03 * ratioOff);
}
#endif

TEST_F (SceneRendererMetalTests, SsgiConvergedRegionMatchesAPrimedHistory)
{
    // A renderer's first frame has no history to gather from, and enters the running mean of temporal
    // anti-aliasing without GI (preflight R31, extended to GI by R33). The converged light then
    // barely depends on the history the renderer started from: it matches a renderer whose history
    // was primed by an earlier convergence, then restarted by a setting that changes nothing on
    // screen (contact shadows, without a light)
    const auto renderRed = [&] (bool primed)
    {
        Scene scene;
        buildGIRedRoomScene (scene);
        scene.getRoot()->attach<ExposureNode> (0.5f); // The floor's red stays below 1, which meanLinearRed() reads

        SceneRenderer renderer;
        setScreenSpaceGI (renderer, true);
        const auto [top, bottom] = findRedWallRows (renderConverged (renderer, scene, 64));
        EXPECT_TRUE (top >= 0 && bottom < 56) << top << " " << bottom;

        if (primed)
        {
            auto settings = renderer.getRenderSettings();
            settings.contactShadows = false;
            renderer.setRenderSettings (settings);
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr);
            EXPECT_FALSE (renderer.isConverged());
            EXPECT_TRUE (renderConverged (renderer, scene, 64).isValid()) << renderer.getLastError();
        }

        EXPECT_TRUE (renderer.isConverged());
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        return sceneColor != nullptr ? meanLinearRed (sceneColor, { 16, bottom + 2, 32, 6 }) : -1.0;
    };

    const auto fresh = renderRed (false);
    const auto primed = renderRed (true);
    RecordProperty ("floorRedFresh", String (fresh, 4).toStdString());
    RecordProperty ("floorRedPrimed", String (primed, 4).toStdString());
    EXPECT_GT (fresh, 0.3);

    // Measured 0.5107 against 0.5182. The gathered light is noisy, so the converged floor depends on
    // which frames the 8 samples were: renderers starting 0 to 3 frames apart converge between 0.5085
    // and 0.5223, 2.7% apart. The first frame dilutes the converged GI by about 1.4% on average over
    // those starts, which this tolerance cannot see: it guards against a larger dilution
    EXPECT_NEAR (fresh, primed, 0.03 * primed);
}

TEST_F (SceneRendererMetalTests, SsgiIgnoresSurfacesFacingAway)
{
    // The floor behind the wall sees only its back: the wall occludes it, but lights it with nothing.
    // The floor in front of it, which its emitting side faces, turns red
    const auto render = [&] (bool screenSpaceGI)
    {
        Scene scene;
        buildFacingAwayWallScene (scene);

        SceneRenderer renderer;
        setScreenSpaceGI (renderer, screenSpaceGI);
        const auto image = renderConverged (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto withGI = render (true);
    const auto without = render (false);
    ASSERT_TRUE (withGI.isValid() && without.isValid());

    const auto [top, bottom] = findRedWallRows (without);
    ASSERT_GT (top, 4);
    ASSERT_LT (bottom, 59);

    const Rectangle<int> behind { 16, top - 6, 32, 3 };
    const Rectangle<int> front { 16, bottom + 2, 32, 3 };
    const auto behindOn = meanColor (withGI, behind);
    const auto behindOff = meanColor (without, behind);
    const auto frontOn = meanColor (withGI, front);
    const auto frontOff = meanColor (without, front);
    RecordProperty ("wallRows", (String (top) + " to " + String (bottom)).toStdString());
    RecordProperty ("behindOn", behindOn.toString().toStdString());
    RecordProperty ("behindOff", behindOff.toString().toStdString());
    RecordProperty ("frontOn", frontOn.toString().toStdString());
    RecordProperty ("frontOff", frontOff.toString().toStdString());

    // Measured 1.374 against 1.000 in front, and the same color behind with and without GI, in a
    // band 4 to 6 rows above the wall's edge, which its antialiasing does not reach. Lighting from
    // the wall's back too made the band 1.045 against 1.000: 2 levels of red are about 0.013
    EXPECT_GT (redOverGreen (frontOn), redOverGreen (frontOff) + 0.2f); // Not vacuous: the wall lights what it faces
    EXPECT_NEAR (redOverGreen (behindOn), redOverGreen (behindOff), 0.015f);
}

TEST_F (SceneRendererMetalTests, SsgiSettingRestartsAccumulation)
{
    Scene scene;
    buildGIRedRoomScene (scene);

    SceneRenderer renderer;
    setScreenSpaceGI (renderer, false);
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Restarted, not invalidated: the history stays valid for the GI to gather at once
    auto settings = renderer.getRenderSettings();
    settings.screenSpaceGI = true;
    renderer.setRenderSettings (settings);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    ASSERT_NE (renderer.getAmbientOcclusionPass(), nullptr);
    EXPECT_NE (renderer.getAmbientOcclusionPass()->getIndirectDiffuse(), nullptr);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, FailedSsgiKeepsTheAmbientOcclusionAndGivesUp)
{
    // A GI failure leaves the frame with its ambient occlusion; the second in a row gives the GI
    // up, which changes the image as much as turning it off: a converged renderer starts over
    Scene scene;
    buildGIRedRoomScene (scene);

    // Renders until converged, counting the frames, so a reference renderer can draw as many
    auto frames = 0;
    const auto converge = [&] (SceneRenderer& renderer, Scene& target)
    {
        for (int i = 0; i < 64; ++i)
        {
            ++frames;
            if (renderer.render (device, target, 64, 64) == nullptr || renderer.isConverged())
                break;
        }

        ++frames;
        return renderToImage (renderer, target, 64);
    };

    SceneRenderer renderer;
    setScreenSpaceGI (renderer, true);
    ASSERT_TRUE (converge (renderer, scene).isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());
    const auto* pass = renderer.getAmbientOcclusionPass();
    ASSERT_NE (pass, nullptr);
    ASSERT_NE (pass->getIndirectDiffuse(), nullptr);

    renderer.failScreenSpaceGIForTesting();

    for (const auto givesUp : { false, true })
    {
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ++frames;
        EXPECT_EQ (renderer.getAmbientOcclusionPass(), pass);
        EXPECT_EQ (pass->getIndirectDiffuse(), nullptr);
        EXPECT_NE (pass->getOutput(), nullptr);
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion), nullptr);
        EXPECT_EQ (pass->getNumCachedIndirectDiffuseTargets(), 0u);
        EXPECT_EQ (pass->hasGivenUpIndirectDiffuse(), givesUp);
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ++frames;
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_TRUE (renderer.getLastError().isEmpty());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    Scene offScene;
    buildGIRedRoomScene (offScene);

    // The restart keeps the history, which still holds a share of the light gathered before: how
    // much of it the converged frame keeps, against a renderer that never had GI
    {
        const auto lingering = converge (renderer, scene);
        SceneRenderer off;
        setScreenSpaceGI (off, false);
        const auto without = renderConverged (off, offScene, 64);
        ASSERT_TRUE (lingering.isValid() && without.isValid());

        const auto [top, bottom] = findRedWallRows (without);
        const Rectangle<int> floor { 16, bottom + 2, 32, 6 };
        RecordProperty ("lingeringPixels", countDifferencesAboveOneLevel (lingering, without));
        RecordProperty ("lingeringFloor", (meanColor (lingering, floor).toString() + " against " + meanColor (without, floor).toString()).toStdString());
    }

    // A new size discards the history: given up, the frame is then the one without GI, against a
    // renderer that drew as many frames before the resize, so both jitter alike
    ASSERT_NE (renderer.render (device, scene, 48, 48), nullptr) << renderer.getLastError();
    const auto failing = renderConverged (renderer, scene, 64);

    SceneRenderer off;
    setScreenSpaceGI (off, false);
    for (int i = 0; i < frames; ++i)
        ASSERT_NE (off.render (device, offScene, 64, 64), nullptr) << off.getLastError();

    ASSERT_NE (off.render (device, offScene, 48, 48), nullptr) << off.getLastError();
    const auto without = renderConverged (off, offScene, 64);
    ASSERT_TRUE (failing.isValid() && without.isValid());
    EXPECT_EQ (countDifferencesAboveOneLevel (failing, without), 0);
}

TEST_F (SceneRendererMetalTests, SsgiOffReleasesItsTargets)
{
    // A frame without GI releases the GI targets only, the ambient occlusion keeps its own
    Scene scene;
    buildGIRedRoomScene (scene);

    SceneRenderer renderer;
    setScreenSpaceGI (renderer, true);
    for (int i = 0; i < 2; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto* pass = renderer.getAmbientOcclusionPass();
    ASSERT_NE (pass, nullptr);
    EXPECT_GT (pass->getNumCachedIndirectDiffuseTargets(), 0u);

    auto settings = renderer.getRenderSettings();
    settings.screenSpaceGI = false;
    renderer.setRenderSettings (settings);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (pass->getNumCachedIndirectDiffuseTargets(), 0u);
    EXPECT_EQ (pass->getIndirectDiffuse(), nullptr);
    EXPECT_GT (pass->getNumCachedTargets(), 0u);

    settings.screenSpaceGI = true;
    renderer.setRenderSettings (settings);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_GT (pass->getNumCachedIndirectDiffuseTargets(), 0u);

    // A new size has no history yet: the frame traces the ambient occlusion alone
    ASSERT_NE (renderer.render (device, scene, 48, 48), nullptr) << renderer.getLastError();
    EXPECT_EQ (pass->getNumCachedIndirectDiffuseTargets(), 0u);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, SpecularOcclusionIsContinuousAndKeepsGrazingReflections)
{
    // The self-contained block of lib/probes.glsl the shaders reading the ambient occlusion compile,
    // without the environment lookups and their includes around it
    const auto library = "#define YUP_3D_AMBIENT_OCCLUSION\n"
                       + File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getParentDirectory()
                             .getChildFile ("modules/yup_renderer/shaders/lib/probes.glsl")
                             .loadFileAsString()
                             .fromFirstOccurrenceOf ("#ifdef YUP_3D_AMBIENT_OCCLUSION", true, false)
                             .upToLastOccurrenceOf ("#endif", false, false);
    ASSERT_TRUE (library.contains ("gtaoSpecularOcclusion"));

    // Red: full visibility hides nothing, even at grazing. Green: no visibility hides everything.
    // Blue: an open floor (visibility 250 / 255) keeps a smooth reflection 85 degrees off its normal
    const auto bounds = mapTexels (GeometryBindings::createSolidTexture (device, 0xffffffff), "vec4("
                                   "gtaoSpecularOcclusion(0.05, 1.0, 0.8), "
                                   "gtaoSpecularOcclusion(0.5, 0.0, 0.5), "
                                   "gtaoSpecularOcclusion(cos(radians(85.0)), 250.0 / 255.0, 0.1), 1.0)",
                                   library);
    ASSERT_TRUE (bounds.isValid());
    EXPECT_EQ (bounds.getPixelColor (0, 0).getRed(), 255);
    EXPECT_EQ (bounds.getPixelColor (0, 0).getGreen(), 0);
    EXPECT_GE (bounds.getPixelColor (0, 0).getBlue(), 230);

    // Over n.v (columns, 0 to 1) and visibility (rows, 0.9 to 1, the open surfaces), for a smooth and
    // a rough lobe: continuous, and rising with the visibility
    GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba8unorm, true);
    const auto grid = GpuTexture::create (device, desc);
    ASSERT_NE (grid, nullptr);

    const auto sweep = mapTexels (grid, "vec4("
                                  "gtaoSpecularOcclusion((gl_FragCoord.x - 0.5) / 63.0, 0.9 + 0.1 * (gl_FragCoord.y - 0.5) / 63.0, 0.1), "
                                  "gtaoSpecularOcclusion((gl_FragCoord.x - 0.5) / 63.0, 0.9 + 0.1 * (gl_FragCoord.y - 0.5) / 63.0, 0.7), 0.0, 1.0)",
                                  library);
    ASSERT_TRUE (sweep.isValid());

    auto largestStep = 0, smallestGrazing = 255, notRising = 0;
    for (int y = 0; y < 64; ++y)
    {
        for (int x = 0; x < 64; ++x)
        {
            const auto c = sweep.getPixelColor (x, y);
            if (x > 0)
                largestStep = jmax (largestStep, std::abs (static_cast<int> (c.getRed()) - static_cast<int> (sweep.getPixelColor (x - 1, y).getRed())));
            if (y > 0)
                largestStep = jmax (largestStep, std::abs (static_cast<int> (c.getRed()) - static_cast<int> (sweep.getPixelColor (x, y - 1).getRed())));

            // Grazing smooth reflections, 80 to 88 degrees off the normal, at visibility 0.98 and above
            const auto nDotV = static_cast<float> (x) / 63.0f, visibility = 0.9f + 0.1f * static_cast<float> (y) / 63.0f;
            if (nDotV >= std::cos (degreesToRadians (88.0f)) && nDotV <= std::cos (degreesToRadians (80.0f)) && visibility >= 0.98f)
                smallestGrazing = jmin (smallestGrazing, static_cast<int> (c.getRed()));

            if (y > 0)
            {
                notRising += c.getRed() + 1 < sweep.getPixelColor (x, y - 1).getRed() ? 1 : 0;
                notRising += c.getGreen() + 1 < sweep.getPixelColor (x, y - 1).getGreen() ? 1 : 0;
            }
        }
    }

    RecordProperty ("largestStep", largestStep);
    RecordProperty ("smallestGrazing", smallestGrazing);
    EXPECT_LE (largestStep, 8);
    EXPECT_GE (smallestGrazing, 230);
    EXPECT_EQ (notRising, 0);
}

TEST_F (SceneRendererMetalTests, AmbientOcclusionUpsampleReadsWhereEachTexelWasTraced)
{
    // Texel i of the half resolution ambient occlusion was traced at the full resolution texel 2i:
    // that fragment must read the center of texel i, and texel 2i + 1 halfway to the next one
    const auto library = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getParentDirectory()
                             .getChildFile ("modules/yup_renderer/shaders/lib/probes.glsl")
                             .loadFileAsString()
                             .fromFirstOccurrenceOf ("#ifdef YUP_3D_AMBIENT_OCCLUSION", true, false)
                             .upToLastOccurrenceOf ("#endif", false, false);

    // Red: 100 + 50 * (the texel coordinate read for fragment 2i, minus i + 0.5); green: the same for 2i + 1, minus i + 1
    const auto mapped = mapTexels (GeometryBindings::createSolidTexture (device, 0xffffffff),
                                   "vec4((100.0 + 50.0 * (gtaoUpsampleUv(vec2(10.5), vec2(32.0)).x * 32.0 - 5.5)) / 255.0, "
                                   "(100.0 + 50.0 * (gtaoUpsampleUv(vec2(11.5), vec2(32.0)).y * 32.0 - 6.0)) / 255.0, 0.0, 1.0)",
                                   "#define YUP_3D_AMBIENT_OCCLUSION\n" + library);
    ASSERT_TRUE (mapped.isValid());
    EXPECT_NEAR (mapped.getPixelColor (0, 0).getRed(), 100, 1);
    EXPECT_NEAR (mapped.getPixelColor (0, 0).getGreen(), 100, 1);
}

TEST_P (SceneRendererTierTests, AmbientOcclusionDarkensTheContactCrease)
{
    checkAmbientOcclusionDarkensTheContactCrease (GetParam());
}
#endif

TEST_P (SceneRendererTierTests, AmbientOcclusionOffAllocatesNothing)
{
    checkAmbientOcclusionOffAllocatesNothing (GetParam());
}

TEST_P (SceneRendererTierTests, AmbientOcclusionUnavailableOnRasterRendersFallback)
{
    checkAmbientOcclusionUnavailableOnRasterRendersFallback (GetParam());
}

TEST_P (SceneRendererTierTests, AmbientOcclusionLeavesTheDirectLightAlone)
{
    checkAmbientOcclusionLeavesTheDirectLightAlone (GetParam());
}

TEST_P (SceneRendererTierTests, ConvergedAmbientOcclusionIsDeterministic)
{
    checkConvergedAmbientOcclusionIsDeterministic (GetParam());
}

TEST_P (SceneRendererTierTests, BlendedSurfacesSkipAmbientOcclusion)
{
    checkBlendedSurfacesSkipAmbientOcclusion (GetParam());
}

TEST_P (SceneRendererTierTests, BentNormalsTurnTheDiffuseLightToTheOpenSide)
{
    checkBentNormalsTurnTheDiffuseLightToTheOpenSide (GetParam());
}

TEST_F (SceneRendererMetalTests, RasterAmbientOcclusionOffAgainRestoresTheFrame)
{
    checkRasterAmbientOcclusionOffAgainRestoresTheFrame();
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, InfiniteFarPlaneShowsTheEnvironment)
{
    // A glTF camera without zfar has an infinite far plane: the sky it shows must be finite and
    // look exactly as through the same camera with a distant far plane. The environment changes
    // with the direction, so a flipped or wrong sky direction shows
    const auto full = GetParam() == RenderSettings::TierPreference::full;

    const auto renderSky = [&] (float zFar)
    {
        Scene scene;
        scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([] (const Vector3<float>& d)
                                                  { return Vector3<float> (0.5f + 0.4f * d.getX(), 0.5f + 0.4f * d.getY(), 0.5f + 0.4f * d.getZ()); },
                                                  64,
                                                  3));
        scene.getRoot()->getNode<EnvironmentNode>()->visible = true;

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        auto& node = camera->attach<CameraNode>();
        node.zNear = 0.05f;
        node.zFar = zFar;
        camera->setRotation (Quaternion::fromAxisAngle (Vector3<float> (1.0f, 1.0f, 0.0f).normalized(), 0.6f));
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        // Scene color holds raw radiance on full without temporal anti-aliasing
        SceneRenderer renderer;
        if (full)
            setFullTierTaa (renderer, 1);
        else
            setTier (renderer, RenderSettings::TierPreference::raster);

        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

        if (full)
        {
            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            EXPECT_NE (sceneColor, nullptr);
            EXPECT_TRUE (sceneColor != nullptr && isFiniteTexture (sceneColor)) << "zFar " << zFar;
            EXPECT_FALSE (sceneColor == nullptr || isSaturatedTexture (sceneColor)) << "zFar " << zFar;
        }

        return image;
    };

    const auto infinite = renderSky (0.0f);
    const auto finite = renderSky (1.0e5f);
    ASSERT_TRUE (infinite.isValid());
    ASSERT_TRUE (finite.isValid());

    // Not vacuous: the sky varies across the frame, below white
    const auto topLeft = meanLuminance (finite, { 0, 0, 8, 8 });
    const auto bottomRight = meanLuminance (finite, { 56, 56, 8, 8 });
    EXPECT_GT (std::abs (topLeft - bottomRight), 5.0);
    EXPECT_LT (jmax (topLeft, bottomRight), 250.0);

    EXPECT_EQ (countDifferences (infinite, finite), 0);
}
#endif

TEST_P (SceneRendererTierTests, ReflectionProbeShowsWhatTheCameraCannotSee)
{
    checkReflectionProbeShowsWhatTheCameraCannotSee (GetParam());
}

TEST_P (SceneRendererTierTests, ReplacedProbeIsBakedAgain)
{
    checkReplacedProbeIsBakedAgain (GetParam());
}

TEST_P (SceneRendererTierTests, MovingAProbeBakesItAgain)
{
    checkMovingAProbeBakesItAgain (GetParam());
}

TEST_P (SceneRendererTierTests, ProbeBlendWithoutEnvironmentIsFinite)
{
    checkProbeBlendWithoutEnvironmentIsFinite (GetParam());
}

TEST_P (SceneRendererTierTests, BoxProbeWithParallaxReflects)
{
    checkBoxProbeWithParallaxReflects (GetParam());
}

TEST_P (SceneRendererTierTests, BakeProbesReportsPendingCount)
{
    checkBakeProbesReportsPendingCount (GetParam());
}

TEST_P (SceneRendererTierTests, RemovedProbesReleaseTheAtlas)
{
    checkRemovedProbesReleaseTheAtlas (GetParam());
}

TEST_P (SceneRendererTierTests, ProbeOfAnOpenSkyMatchesTheEnvironment)
{
    checkProbeOfAnOpenSkyMatchesTheEnvironment (GetParam());
}

TEST_P (SceneRendererTierTests, RgbmProbeShowsLikeTheFloatOne)
{
    checkRgbmProbeShowsLikeTheFloatOne (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, GpuFailureBakesTheProbesAgain)
{
    // The frame baking the probe failed on the GPU: the next frame cannot trust its tile
    Scene scene;
    buildMirrorFloorScene (scene);
    addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getNumBakes (renderer), 1);

    // Frames complete in order, so once a later frame did the failed one has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
    EXPECT_EQ (getNumBakes (renderer), 2);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getNumBakes (renderer), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}
#endif

TEST_P (SceneRendererTierTests, ProbeKeepsTheMultipleScatteringOfRoughMetals)
{
    checkProbeKeepsTheMultipleScatteringOfRoughMetals (GetParam());
}

TEST_P (SceneRendererTierTests, BoxProbeParallaxPlacesTheReflectionOnTheWalls)
{
    checkBoxProbeParallaxPlacesTheReflectionOnTheWalls (GetParam());
}

TEST_P (SceneRendererTierTests, FailedProbeCapturesLeaveTheFrameWithoutProbes)
{
    checkFailedProbeCapturesLeaveTheFrameWithoutProbes (GetParam());
}

TEST_F (SceneRendererMetalTests, NestedProbesBlendAlikeOnBothTiers)
{
    // The large probe captures the emitter green; the small one, nested in it over the near floor,
    // captures it red afterwards. Raster picks both for the floor, full blends them per pixel in the
    // same order: the floor matches band by band
    const auto render = [this] (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f, "large");

        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        EXPECT_EQ (renderer.bakeProbes (device, scene), 1);

        setEmitterColor (scene, { 1.0f, 0.0f, 0.0f });
        addReflectionProbe (scene, { 0.0f, 0.5f, 0.5f }, 1.5f, "small").falloff = 0.5f;

        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 2);
        return image;
    };

    const auto raster = render (RenderSettings::TierPreference::raster);
    const auto full = render (RenderSettings::TierPreference::full);
    ASSERT_TRUE (raster.isValid() && full.isValid());

    auto green = false, red = false;
    for (const auto& band : { Rectangle<int> (16, 4, 32, 8), Rectangle<int> (16, 16, 32, 8), Rectangle<int> (16, 28, 32, 8), Rectangle<int> (16, 44, 32, 12) })
    {
        const auto expected = meanColor (raster, band);
        const auto actual = meanColor (full, band);
        RecordProperty (("raster" + band.toString()).toStdString(), expected.toString().toStdString());
        RecordProperty (("full" + band.toString()).toStdString(), actual.toString().toStdString());

        EXPECT_NEAR (actual.getRed(), expected.getRed(), 6) << band.toString();
        EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 6) << band.toString();
        EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 6) << band.toString();

        green = green || expected.getGreen() > expected.getRed() + 30;
        red = red || expected.getRed() > expected.getGreen() + 30;
    }

    // Not vacuous: the far floor shows the large probe, the near one the small
    EXPECT_TRUE (green);
    EXPECT_TRUE (red);
}

TEST_F (SceneRendererMetalTests, FullTierShadesAtMost64Probes)
{
    // 65 baked probes: the full tier packs the 64 smallest, the largest left out, logged and not asserted
    Scene scene;
    for (int i = 1; i <= 65; ++i)
        addReflectionProbe (scene, { static_cast<float> (i), 0.0f, 0.0f }, static_cast<float> (i), "probe");

    SceneRenderer renderer;
    ASSERT_EQ (renderer.bakeProbes (device, scene), 65);
    ASSERT_NE (renderer.getProbeBaker(), nullptr);

    SceneDrawList list;
    list.build (*scene.getRoot(), Matrix4());
    ASSERT_EQ (list.getReflectionProbes().size(), 65u);

    const auto packed = renderer.getProbeBaker()->packReflectionProbesByPriority (list);
    ASSERT_EQ (packed.size(), 64u);
    EXPECT_FLOAT_EQ (packed.front().influence[0], 1.0f);
    EXPECT_FLOAT_EQ (packed.back().influence[0], 64.0f);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, FullTierPacksTheProbesInTheRasterPriorityOrder)
{
    // The smaller influence first whatever the bake order, so the full tier blends as raster ranks
    Scene scene;
    buildMirrorFloorScene (scene);
    addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f, "large");
    addReflectionProbe (scene, { 1.0f, 1.0f, -1.0f }, 5.0f, "small");

    SceneRenderer renderer;
    ASSERT_EQ (renderer.bakeProbes (device, scene), 2);
    ASSERT_NE (renderer.getProbeBaker(), nullptr);

    SceneDrawList list;
    list.build (*scene.getRoot(), Matrix4());

    const auto packed = renderer.getProbeBaker()->packReflectionProbesByPriority (list);
    ASSERT_EQ (packed.size(), 2u);
    EXPECT_FLOAT_EQ (packed[0].influence[0], 5.0f);
    EXPECT_FLOAT_EQ (packed[1].influence[0], 20.0f);

    // Both baked into their own tile of the atlas
    EXPECT_GT (packed[0].atlasRect[2], 0.0f);
    EXPECT_NE (packed[0].atlasRect[0], packed[1].atlasRect[0]);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, ReflectionProbeChangesRestartAccumulation)
{
    // Each change shows on the full tier once converged: a bake asked for, a field of the probe, and
    // its entity moving, which also bakes it again
    const std::pair<const char*, std::function<void (EntityNode&)>> changes[] = {
        { "requestBake", [] (EntityNode& probe) { probe.getNode<ReflectionProbeNode>()->requestBake(); } },
        { "intensity", [] (EntityNode& probe) { probe.getNode<ReflectionProbeNode>()->intensity = 0.5f; } },
        { "falloff", [] (EntityNode& probe) { probe.getNode<ReflectionProbeNode>()->falloff = 2.0f; } },
        { "position", [] (EntityNode& probe) { probe.setPosition ({ 0.5f, 1.0f, -1.0f }); } },
    };

    for (const auto& [name, change] : changes)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 8);
        for (int i = 0; i < 12; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*scene.getRoot()->findChild ("probe"));

        // Restarted, not invalidated
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }
}

TEST_P (SceneRendererTierTests, IrradianceVolumeTintsTheFloorNearTheRedWall)
{
    checkIrradianceVolumeTintsTheFloorNearTheRedWall (GetParam());
}

TEST_P (SceneRendererTierTests, VisibilityStopsLightLeakingThroughTheWall)
{
    checkVisibilityStopsLightLeakingThroughTheWall (GetParam());
}

TEST_P (SceneRendererTierTests, MovingAVolumeBakesItAgain)
{
    checkMovingAVolumeBakesItAgain (GetParam());
}

TEST_P (SceneRendererTierTests, BakeProbesBakesTheVolumes)
{
    checkBakeProbesBakesTheVolumes (GetParam());
}

TEST_P (SceneRendererTierTests, RemovedVolumesReleaseTheAtlases)
{
    checkRemovedVolumesReleaseTheAtlases (GetParam());
}

TEST_P (SceneRendererTierTests, RgbmVolumeLightsLikeTheFloatOne)
{
    checkRgbmVolumeLightsLikeTheFloatOne (GetParam());
}

TEST_P (SceneRendererTierTests, FailedVolumeCapturesLeaveTheFrameWithoutVolumes)
{
    checkFailedVolumeCapturesLeaveTheFrameWithoutVolumes (GetParam());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, GpuFailureBakesTheVolumesAgain)
{
    // The frame baking the volume failed on the GPU: the next frame cannot trust its blocks. The
    // environment cube is built first, in a frame of its own, so the failure hits the bake
    Scene scene;
    buildRedRoomScene (scene, false);

    SceneRenderer renderer;
    setTier (renderer, GetParam());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    addIrradianceVolume (scene, 4, 2, 2);
    device->failNextFrameForTesting ("injected failure");
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getNumVolumeBakes (renderer), 1);

    // Frames complete in order, so once a later frame did the failed one has too
    {
        auto fence = GpuFrame::begin (device);
        EXPECT_TRUE (fence.waitForGPU().wasOk());
    }

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
    EXPECT_EQ (getNumVolumeBakes (renderer), 2);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (getNumVolumeBakes (renderer), 2);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}
#endif

TEST_P (SceneRendererTierTests, ChangedVolumeBakesAgainWhole)
{
    checkChangedVolumeBakesAgainWhole (GetParam());
}

TEST_P (SceneRendererTierTests, FailedProbeCapturesLeaveTheVolumesLighting)
{
    checkFailedProbeCapturesLeaveTheVolumesLighting (GetParam());
}

TEST_P (SceneRendererTierTests, ProgressiveVolumeBakeShadesOnceComplete)
{
    checkProgressiveVolumeBakeShadesOnceComplete (GetParam());
}

TEST_P (SceneRendererTierTests, FailedVolumeCapturesLeaveTheProbesReflecting)
{
    checkFailedVolumeCapturesLeaveTheProbesReflecting (GetParam());
}

TEST_P (SceneRendererTierTests, ResizingAVolumeBakesItAgain)
{
    checkResizingAVolumeBakesItAgain (GetParam());
}

TEST_P (SceneRendererTierTests, VolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity)
{
    checkVolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity (GetParam());
}

TEST_F (SceneRendererMetalTests, IrradianceVolumeShadesAlikeOnBothTiers)
{
    // Raster picks the volume per draw by its center, full per pixel: with one volume over the whole
    // floor both shade the floor from the same probes. The tolerance of 6 levels is T7's tier parity
    // one: the two tiers differ by their geometric normal (the full tier stands the shading normal in
    // for it) and their AO and MSAA, about a level here
    const auto render = [this] (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene);
        addIrradianceVolume (scene, 6, 2, 4);

        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    };

    const auto raster = render (RenderSettings::TierPreference::raster);
    const auto full = render (RenderSettings::TierPreference::full);
    ASSERT_TRUE (raster.isValid() && full.isValid());

    for (const auto& region : { leftFloorRegion(), rightFloorRegion(), Rectangle<int> (40, 19, 16, 26) })
    {
        const auto expected = meanColor (raster, region);
        const auto actual = meanColor (full, region);
        RecordProperty (("raster" + region.toString()).toStdString(), expected.toString().toStdString());
        RecordProperty (("full" + region.toString()).toStdString(), actual.toString().toStdString());

        EXPECT_NEAR (actual.getRed(), expected.getRed(), 6) << region.toString();
        EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 6) << region.toString();
        EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 6) << region.toString();
    }

    // Not vacuous: the volume tints the floor between the walls
    EXPECT_GT (redOverGreen (meanColor (raster, leftFloorRegion())), 1.3f);
}

TEST_F (SceneRendererMetalTests, IrradianceVolumeChangesRestartAccumulation)
{
    // Each change shows on the full tier once converged: a bake asked for, a field of the volume,
    // and its entity moving, which also bakes it again
    const std::pair<const char*, std::function<void (EntityNode&)>> changes[] = {
        { "requestBake", [] (EntityNode& volume) { volume.getNode<IrradianceVolumeNode>()->requestBake(); } },
        { "intensity", [] (EntityNode& volume) { volume.getNode<IrradianceVolumeNode>()->intensity = 0.5f; } },
        { "falloff", [] (EntityNode& volume) { volume.getNode<IrradianceVolumeNode>()->falloff = 1.0f; } },
        { "normalBias", [] (EntityNode& volume) { volume.getNode<IrradianceVolumeNode>()->normalBias = 0.2f; } },
        { "position", [] (EntityNode& volume) { volume.setPosition ({ 0.0f, 1.6f, 0.0f }); } },
    };

    for (const auto& [name, change] : changes)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 8);
        for (int i = 0; i < 12; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*scene.getRoot()->findChild ("volume"));

        // Restarted, not invalidated
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }
}

TEST_F (SceneRendererMetalTests, LightingPassShadesProbesAndVolumesWithItsLocalIndirectVariant)
{
    // Preflight R27: the full tier shades its opaque surfaces with the local indirect variants of
    // the lighting pass only in the frames with reflection probes or irradiance volumes
    Scene scene;
    buildRedRoomScene (scene, false);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 1);
    const auto usesLocalIndirect = [&]
    {
        EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        const auto* lightingPass = renderer.getLightingPass();
        return lightingPass != nullptr && lightingPass->usesLocalIndirect();
    };

    EXPECT_FALSE (usesLocalIndirect());

    addIrradianceVolume (scene, 4, 2, 2);
    EXPECT_TRUE (usesLocalIndirect());

    scene.getRoot()->removeChild (scene.getRoot()->findChild ("volume"));
    EXPECT_FALSE (usesLocalIndirect());

    addReflectionProbe (scene, { 0.0f, 1.0f, 0.0f }, 20.0f);
    EXPECT_TRUE (usesLocalIndirect());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, FailedLocalIndirectLightingShadesWithoutVolumes)
{
    // A local indirect variant the device cannot compile leaves the full tier's frames rendering,
    // their opaque surfaces without the volume
    const auto render = [this] (bool fail)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        setFullTierTaa (renderer, 1);
        if (fail)
            renderer.failLocalIndirectLightingForTesting();

        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_NE (renderer.getLightingPass(), nullptr);
        EXPECT_EQ (renderer.getLightingPass() != nullptr && renderer.getLightingPass()->usesLocalIndirect(), ! fail);
        return image;
    };

    const auto failed = render (true);
    const auto working = render (false);
    ASSERT_TRUE (failed.isValid() && working.isValid());

    EXPECT_NEAR (redOverGreen (meanColor (failed, leftFloorRegion())), 1.0f, 0.05f);
    EXPECT_GT (redOverGreen (meanColor (working, leftFloorRegion())), 1.2f); // Not vacuous
}

TEST_F (SceneRendererMetalTests, FullTierShadesAtMost8Volumes)
{
    // 9 baked volumes: the full tier packs the 8 smallest, the largest left out, logged and not asserted
    Scene scene;
    for (int i = 1; i <= 9; ++i)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("volume"));
        auto& volume = entity->attach<IrradianceVolumeNode>();
        volume.extents = { static_cast<float> (i), 1.0f, 1.0f };
        volume.resolutionX = volume.resolutionY = volume.resolutionZ = 2;
        scene.getRoot()->addChild (entity);
    }

    // 72 probe positions take two calls (ProbeBaker::maxVolumeProbesPerBake)
    SceneRenderer renderer;
    auto calls = 0;
    while (renderer.bakeProbes (device, scene) > 0 && calls < 8)
        ++calls;

    EXPECT_EQ (calls, (9 * 8 + ProbeBaker::maxVolumeProbesPerBake - 1) / ProbeBaker::maxVolumeProbesPerBake);
    ASSERT_EQ (getNumVolumeBakes (renderer), 9);
    ASSERT_NE (renderer.getProbeBaker(), nullptr);

    SceneDrawList list;
    list.build (*scene.getRoot(), Matrix4());
    ASSERT_EQ (list.getIrradianceVolumes().size(), 9u);

    const auto packed = renderer.getProbeBaker()->packIrradianceVolumesByPriority (list);
    ASSERT_EQ (packed.size(), 8u);

    // Smallest first: worldToUnit scales x by 1 / (2 * extent)
    EXPECT_FLOAT_EQ (packed.front().worldToUnit[0], 0.5f);
    EXPECT_FLOAT_EQ (packed.back().worldToUnit[0], 1.0f / 16.0f);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (SceneRendererMetalTests, VolumeBakeUniformPoolStaysSmall)
{
    // Preflight R18 and R28: volume bakes in the red room plus 100 small casters, each its own draw
    // on every capture face: the brief's 6 x 2 x 4 volume, then the largest one, 7 x 7 x 7, which
    // bakes over several frames, waited on and then not. The device's uniform buffers, pooled or held
    // by frames in flight, are recorded once each bake completed and the frames returned them. On a
    // device of its own: the pool only grows, so the suite's device holds the peaks of earlier tests
    const auto poolDevice = GpuDevice::create (GpuPlatform::Metal, {});
    ASSERT_NE (poolDevice, nullptr);

    const auto bake = [&poolDevice] (int nx, int ny, int nz, const char* name, bool waitEachFrame = true)
    {
        Scene scene;
        buildRedRoomScene (scene);
        addSmallCasters (scene, 100);
        addIrradianceVolume (scene, nx, ny, nz);

        SceneRenderer renderer;
        auto frames = 0;
        while (getNumVolumeBakes (renderer) < 1 && frames < 64)
        {
            EXPECT_NE (renderer.render (poolDevice, scene, 64, 64), nullptr) << renderer.getLastError();
            if (waitEachFrame)
                EXPECT_TRUE (renderer.waitForGPU().wasOk());
            ++frames;
        }

        for (int i = 0; i < 3; ++i)
        {
            EXPECT_NE (renderer.render (poolDevice, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
        }

        EXPECT_EQ (getNumVolumeBakes (renderer), 1) << name;

        const auto pool = poolDevice->getUniformBufferPoolSizeForTesting();
        RecordProperty ((String (name) + "FramesToBake").toStdString(), frames);
        RecordProperty ((String (name) + "UniformBuffers").toStdString(), pool.first);
        RecordProperty ((String (name) + "UniformBytes").toStdString(), static_cast<int> (pool.second));
        return pool;
    };

    // The pool only grows, so each figure is the running peak of the device after that bake, an upper bound of it
    const auto before = poolDevice->getUniformBufferPoolSizeForTesting();
    const auto brief = bake (6, 2, 4, "brief");
    const auto largest = bake (IrradianceVolumeNode::maxResolution, IrradianceVolumeNode::maxResolution, IrradianceVolumeNode::maxResolution, "largest");

    // As an application renders, without waiting: the frames in flight hold their buffers longer
    const auto unwaited = bake (IrradianceVolumeNode::maxResolution, IrradianceVolumeNode::maxResolution, IrradianceVolumeNode::maxResolution, "unwaited", false);

    EXPECT_GT (brief.second, before.second);
    EXPECT_LE (brief.second, static_cast<size_t> (16 * 1024 * 1024));
    EXPECT_LE (largest.second, static_cast<size_t> (16 * 1024 * 1024));
    EXPECT_LE (unwaited.second, static_cast<size_t> (16 * 1024 * 1024));
}
