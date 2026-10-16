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
class SceneRendererFrameStatsTests : public SceneRendererMetalTests
{
protected:
    static constexpr int size = 64;

    void SetUp() override
    {
        SceneRendererMetalTests::SetUp();
        if (! IsSkipped() && ! device->isScopeTimingAvailable())
            GTEST_SKIP() << "Scope timing is unavailable on this device";
    }

    /** The plates under a shadowed sun, a blended quad in front of them, a reflection probe, and a
        camera at f/1.4 focused on the small plate, on @p tier with ambient occlusion, screen-space
        reflections and GI, depth of field and bloom: every pass of the tier but the debug view, and
        the motion blur and the fog, which tests add (the scene is still and has no fog). Render it
        with setEveryPass(). */
    static void buildEveryPassScene (Scene& scene)
    {
        buildShadowsScene (scene);

        auto blended = Material::Ptr (new Material());
        blended->baseColorFactor = { 0.2f, 0.4f, 0.8f, 0.5f };
        blended->alphaMode = Material::AlphaMode::blend;
        auto* quad = addPlate (scene, blended);
        quad->setScale ({ 0.2f, 0.2f, 1.0f });
        quad->setPosition ({ -0.5f, 0.3f, 0.8f });

        addReflectionProbe (scene, { 0.0f, 0.0f, 1.0f }, 3.0f);

        addCamera (scene, { 0.0f, 0.0f, 3.0f }, {});
        auto& camera = *scene.getActiveCamera();
        camera.fStop = 1.4f;
        camera.focusDistance = 2.5f;
    }

    /** Selects @p tier with every pass of buildEveryPassScene() on. */
    static void setEveryPass (SceneRenderer& renderer, RenderSettings::TierPreference tier)
    {
        auto settings = renderer.getRenderSettings();
        settings.tier = tier;
        settings.ambientOcclusion = true;
        settings.screenSpaceReflections = true;
        settings.screenSpaceGI = true;
        settings.depthOfField = true;
        settings.bloom = true;
        renderer.setRenderSettings (settings);
    }

    /** Renders @p frames frames of @p scene at size x size, waiting for each. */
    void renderAndWait (SceneRenderer& renderer, Scene& scene, int frames)
    {
        for (int i = 0; i < frames; ++i)
        {
            ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
            ASSERT_TRUE (renderer.waitForGPU().wasOk());
        }
    }

    /** The names of the scopes of @p stats, in order, separated by spaces. */
    static String getScopeNames (const SceneRenderer::FrameStats& stats)
    {
        StringArray names;
        for (const auto& scope : stats.gpu.scopes)
            names.add (scope.name);

        return names.joinIntoString (" ");
    }

    /** The milliseconds of the scope named @p name, or -1 when the frame has none or it is unavailable. */
    static double getScopeMilliseconds (const SceneRenderer::FrameStats& stats, const String& name)
    {
        for (const auto& scope : stats.gpu.scopes)
            if (scope.name == name && scope.available)
                return scope.milliseconds;

        return -1.0;
    }

    /** Every scope of @p stats is available, and its time a finite number of milliseconds. */
    static void expectScopesMeasured (const SceneRenderer::FrameStats& stats)
    {
        for (const auto& scope : stats.gpu.scopes)
        {
            EXPECT_TRUE (scope.available) << scope.name;
            EXPECT_TRUE (std::isfinite (scope.milliseconds)) << scope.name;
            EXPECT_GE (scope.milliseconds, 0.0) << scope.name;
        }
    }

    /** @p frames frames of @p scene queued without waiting, but for the frames up to the middle one
        and then all of them, which deliver the stats of those two frames whatever the speed of the
        GPU: the distinct stats seen after each render and after each wait, in the order they
        arrived. @p prepare edits the scene before each frame, given its number in the queue from 1,
        and returns its size. */
    template <class Prepare>
    std::vector<SceneRenderer::FrameStats> queueFrames (SceneRenderer& renderer, Scene& scene, int frames, Prepare&& prepare)
    {
        std::vector<SceneRenderer::FrameStats> seen;
        const auto take = [&]
        {
            const auto& stats = renderer.getLastFrameStats();
            if (stats.frameIndex != 0 && (seen.empty() || seen.back().frameIndex != stats.frameIndex))
                seen.push_back (stats);
        };

        for (int frame = 1; frame <= frames; ++frame)
        {
            const auto [width, height] = prepare (frame);
            EXPECT_NE (renderer.render (device, scene, width, height), nullptr) << renderer.getLastError();
            take();

            if (frame == frames / 2)
            {
                EXPECT_TRUE (renderer.waitForGPU().wasOk());
                take();
            }
        }

        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        take();
        return seen;
    }

    /** Renders a frame with the stats on in the state @p prepare (true) sets up, then one with the
        stats off in the state @p prepare (false) sets up, then waits: the stats that arrive must be
        the first frame's, though the renderer then holds the second frame's state and status.
        @p prepare returns the size to render at. */
    template <class Prepare>
    SceneRenderer::FrameStats measureThenRenderAnother (SceneRenderer& renderer, Scene& scene, Prepare&& prepare)
    {
        for (const auto measured : { true, false })
        {
            renderer.setFrameStatsEnabled (measured);
            const auto [width, height] = prepare (measured);
            EXPECT_NE (renderer.render (device, scene, width, height), nullptr) << renderer.getLastError();
        }

        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        return renderer.getLastFrameStats();
    }

    /** Adds @p count small opaque plates in front of the scene, hidden, and returns them. */
    static std::vector<EntityNode*> addHiddenPlates (Scene& scene, int count)
    {
        std::vector<EntityNode*> plates;
        for (int i = 0; i < count; ++i)
        {
            auto* plate = addPlate (scene, makeWallMaterial());
            plate->setScale ({ 0.05f, 0.05f, 1.0f });
            plate->setPosition ({ -0.8f + 0.2f * static_cast<float> (i), -0.6f, 0.6f });
            plate->setVisible (false);
            plates.push_back (plate);
        }

        return plates;
    }

    /** A 4 x 4 plate facing a camera 3 units away, lit by @p numLights point lights 0.5 in front of
        it whose range covers all of it, in the colors of the light show; on the full tier with TAA
        and nothing else, so the lighting pass shades every pixel with every light. */
    static void buildLightsScene (Scene& scene, int numLights)
    {
        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;
        addPlate (scene, material)->setScale ({ 2.0f, 2.0f, 1.0f });
        addCamera (scene, { 0.0f, 0.0f, 3.0f }, {});

        for (int i = 0; i < numLights; ++i)
        {
            auto entity = EntityNode::Ptr (new EntityNode ("showLight"));
            auto& light = entity->attach<LightNode> (LightNode::Type::point);
            const auto color = Color::fromHSV (static_cast<float> (i) / static_cast<float> (numLights), 0.85f, 1.0f);
            light.color = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat() };
            light.intensity = 0.05f;
            light.range = 10.0f;

            const auto angle = static_cast<float> (i) * 2.39996f;
            entity->setPosition ({ std::cos (angle) * 1.5f, std::sin (angle) * 1.5f, 0.5f });
            scene.getRoot()->addChild (entity);
        }
    }

    /** The full tier with TAA and nothing else, as buildLightsScene() is rendered. */
    static void setLightingOnly (SceneRenderer& renderer)
    {
        auto settings = renderer.getRenderSettings();
        settings.tier = RenderSettings::TierPreference::full;
        settings.ambientOcclusion = false;
        settings.screenSpaceReflections = false;
        settings.screenSpaceGI = false;
        renderer.setRenderSettings (settings);
    }

    static double median (std::vector<double> values)
    {
        std::sort (values.begin(), values.end());
        return values[values.size() / 2];
    }
};

TEST_F (SceneRendererFrameStatsTests, FrameStatsListTheFullTierPasses)
{
    Scene scene;
    buildEveryPassScene (scene);

    SceneRenderer renderer;
    setEveryPass (renderer, RenderSettings::TierPreference::full);
    renderer.setFrameStatsEnabled (true);

    // The first frame bakes the probe, and has no history yet for the screen-space effects to read
    renderAndWait (renderer, scene, 1);
    const auto first = renderer.getLastFrameStats();
    EXPECT_EQ (first.frameIndex, 1u);
    ASSERT_TRUE (first.gpu.available);
    EXPECT_EQ (getScopeNames (first), "probeBake shadows lightCulling gbuffer ambientOcclusion lighting forward temporalAA depthOfField bloom toneMapping");
    expectScopesMeasured (first);
    EXPECT_TRUE (first.shadowsRedrawn);
    EXPECT_GT (first.shadowCasterDraws, 0);

    renderAndWait (renderer, scene, 2);
    const auto& stats = renderer.getLastFrameStats();
    EXPECT_EQ (stats.frameIndex, 3u);
    ASSERT_TRUE (stats.gpu.available);
    EXPECT_GT (stats.gpu.frameMilliseconds, 0.0);
    EXPECT_EQ (getScopeNames (stats), "shadows lightCulling gbuffer ambientOcclusion hiZ screenSpaceReflections lighting indirectDiffuse forward temporalAA depthOfField bloom toneMapping");
    expectScopesMeasured (stats);

    // The two plates and the blended quad, at the size rendered; the shadow maps of the still scene are kept
    EXPECT_EQ (stats.drawItems, 3);
    EXPECT_EQ (stats.internalWidth, size);
    EXPECT_EQ (stats.internalHeight, size);
    EXPECT_FALSE (stats.shadowsRedrawn);
    EXPECT_EQ (stats.shadowCasterDraws, 0);

    // A debug view draws last, over the final image
    auto settings = renderer.getRenderSettings();
    settings.debugView = RenderSettings::DebugView::ambientOcclusion;
    renderer.setRenderSettings (settings);
    renderAndWait (renderer, scene, 1);

    const auto& debugged = renderer.getLastFrameStats();
    EXPECT_EQ (debugged.frameIndex, 4u);
    EXPECT_TRUE (getScopeNames (debugged).endsWith ("toneMapping debugView")) << getScopeNames (debugged);
    expectScopesMeasured (debugged);
}

TEST_F (SceneRendererFrameStatsTests, FrameStatsListTheRasterPasses)
{
    Scene scene;
    buildEveryPassScene (scene);

    SceneRenderer renderer;
    setEveryPass (renderer, RenderSettings::TierPreference::raster);
    renderer.setFrameStatsEnabled (true);
    renderAndWait (renderer, scene, 2);

    const auto& stats = renderer.getLastFrameStats();
    EXPECT_EQ (stats.frameIndex, 2u);
    ASSERT_TRUE (stats.gpu.available);
    EXPECT_EQ (getScopeNames (stats), "shadows depthNormalPrepass ambientOcclusion forward depthOfField bloom toneMapping");
    expectScopesMeasured (stats);
    EXPECT_EQ (stats.drawItems, 3);

    // Depth of field alone draws the prepass for its depth, without the ambient occlusion
    auto settings = renderer.getRenderSettings();
    settings.ambientOcclusion = false;
    settings.bloom = false;
    renderer.setRenderSettings (settings);
    renderAndWait (renderer, scene, 1);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows depthNormalPrepass forward depthOfField toneMapping");
    expectScopesMeasured (renderer.getLastFrameStats());

    // A frame whose prepass targets could not be created records neither the prepass nor the depth
    // of field reading its depth, so it lists neither
    renderer.failDepthNormalPrepassTargetsForTesting (1);
    renderAndWait (renderer, scene, 1);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows forward toneMapping");
    expectScopesMeasured (renderer.getLastFrameStats());

    // Ambient occlusion alone keeps its frame: no post chain
    settings.ambientOcclusion = true;
    settings.depthOfField = false;
    renderer.setRenderSettings (settings);
    renderAndWait (renderer, scene, 1);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows depthNormalPrepass ambientOcclusion forward");
    expectScopesMeasured (renderer.getLastFrameStats());
}

TEST_F (SceneRendererFrameStatsTests, MotionBlurIsListedOnMovingFramesOnly)
{
    // Motion blur on, after the depth of field and before bloom, only on the frames that moved
    Scene scene;
    buildEveryPassScene (scene);

    SceneRenderer renderer;
    setEveryPass (renderer, RenderSettings::TierPreference::full);
    auto settings = renderer.getRenderSettings();
    settings.motionBlur = true;
    renderer.setRenderSettings (settings);
    renderer.setFrameStatsEnabled (true);
    renderAndWait (renderer, scene, 2);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows lightCulling gbuffer ambientOcclusion hiZ screenSpaceReflections lighting indirectDiffuse forward temporalAA depthOfField bloom toneMapping");

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);
    camera->setPosition (camera->getPosition() + Vector3<float> (0.05f, 0.0f, 0.0f));
    renderAndWait (renderer, scene, 1);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows lightCulling gbuffer ambientOcclusion hiZ screenSpaceReflections lighting indirectDiffuse forward temporalAA depthOfField motionBlur bloom toneMapping");
    expectScopesMeasured (renderer.getLastFrameStats());

    renderAndWait (renderer, scene, 1);
    EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows lightCulling gbuffer ambientOcclusion hiZ screenSpaceReflections lighting indirectDiffuse forward temporalAA depthOfField bloom toneMapping");
}

TEST_F (SceneRendererFrameStatsTests, FogIsListedRightAfterTheForwardPass)
{
    // Full: in place before TAA. Raster: the first post pass, before the depth of field
    for (const auto tier : { RenderSettings::TierPreference::full, RenderSettings::TierPreference::raster })
    {
        Scene scene;
        buildEveryPassScene (scene);
        scene.setWorldFog (makeFog (0.08f));

        SceneRenderer renderer;
        setEveryPass (renderer, tier);
        renderer.setFrameStatsEnabled (true);
        renderAndWait (renderer, scene, 2);

        const auto* expected = tier == RenderSettings::TierPreference::full
                                 ? "shadows lightCulling gbuffer ambientOcclusion hiZ screenSpaceReflections lighting indirectDiffuse forward fog temporalAA depthOfField bloom toneMapping"
                                 : "shadows depthNormalPrepass ambientOcclusion forward fog depthOfField bloom toneMapping";
        EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), expected);
        expectScopesMeasured (renderer.getLastFrameStats());

        // Fog alone on raster draws the prepass for its depth, then the fog and the tone mapping
        if (tier == RenderSettings::TierPreference::raster)
        {
            auto settings = renderer.getRenderSettings();
            settings.ambientOcclusion = false;
            settings.depthOfField = false;
            settings.bloom = false;
            renderer.setRenderSettings (settings);
            renderAndWait (renderer, scene, 1);
            EXPECT_EQ (getScopeNames (renderer.getLastFrameStats()), "shadows depthNormalPrepass forward fog toneMapping");
        }
    }
}

TEST_F (SceneRendererFrameStatsTests, FrameStatsBelongToOneFrame)
{
    Scene scene;
    buildShadowsScene (scene);
    const auto plates = addHiddenPlates (scene, 8);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 16);
    renderer.setFrameStatsEnabled (true);
    renderAndWait (renderer, scene, 1);

    // The odd frames of the queue draw 8 more plates and bloom, the even ones neither
    const auto setUp = [&] (bool odd)
    {
        for (auto* plate : plates)
            plate->setVisible (odd);

        auto settings = renderer.getRenderSettings();
        settings.bloom = odd;
        renderer.setRenderSettings (settings);
        return std::pair { size, size };
    };

    const auto seen = queueFrames (renderer, scene, 6, [&] (int frame) { return setUp (frame % 2 == 1); });

    RecordProperty ("statsSeen", static_cast<int> (seen.size()));
    ASSERT_GE (seen.size(), 2u);
    EXPECT_EQ (seen.back().frameIndex, 7u);

    for (size_t i = 0; i < seen.size(); ++i)
    {
        const auto& stats = seen[i];
        if (i > 0)
            EXPECT_GT (stats.frameIndex, seen[i - 1].frameIndex);

        // The first frame was rendered before the queue: frame n of the queue is frame n + 1
        const auto odd = (stats.frameIndex - 1) % 2 == 1;
        ASSERT_TRUE (stats.gpu.available) << stats.frameIndex;
        EXPECT_GT (stats.gpu.frameMilliseconds, 0.0) << stats.frameIndex;
        EXPECT_EQ (stats.drawItems, odd ? 10 : 2) << stats.frameIndex;
        EXPECT_EQ (getScopeNames (stats).contains ("bloom"), odd) << stats.frameIndex;
        expectScopesMeasured (stats);
    }

    // Whatever the speed of the GPU: frame 8 measures, frame 9 draws less without bloom and does not
    const auto measured = measureThenRenderAnother (renderer, scene, setUp);
    EXPECT_EQ (measured.frameIndex, 8u);
    ASSERT_TRUE (measured.gpu.available);
    EXPECT_EQ (measured.drawItems, 10);
    EXPECT_TRUE (getScopeNames (measured).contains ("bloom"));
    expectScopesMeasured (measured);
}

TEST_F (SceneRendererFrameStatsTests, ResizeDoesNotMixFrames)
{
    Scene scene;
    buildShadowsScene (scene);
    const auto plates = addHiddenPlates (scene, 8);

    SceneRenderer renderer;
    setFullTierTaa (renderer, 16);
    renderer.setFrameStatsEnabled (true);

    // The odd frames are wider and draw 8 more plates
    const auto setUp = [&] (bool odd)
    {
        for (auto* plate : plates)
            plate->setVisible (odd);

        return odd ? std::pair { 96, 48 } : std::pair { size, size };
    };

    const auto seen = queueFrames (renderer, scene, 6, [&] (int frame) { return setUp (frame % 2 == 1); });

    RecordProperty ("statsSeen", static_cast<int> (seen.size()));
    ASSERT_GE (seen.size(), 2u);
    EXPECT_EQ (seen.back().frameIndex, 6u);

    for (const auto& stats : seen)
    {
        const auto odd = stats.frameIndex % 2 == 1;
        ASSERT_TRUE (stats.gpu.available) << stats.frameIndex;
        EXPECT_EQ (stats.internalWidth, odd ? 96 : size) << stats.frameIndex;
        EXPECT_EQ (stats.internalHeight, odd ? 48 : size) << stats.frameIndex;
        EXPECT_EQ (stats.drawItems, odd ? 10 : 2) << stats.frameIndex;
        EXPECT_FALSE (stats.gpu.scopes.empty()) << stats.frameIndex;
        expectScopesMeasured (stats);
    }

    // Whatever the speed of the GPU: frame 7 measures at 96x48, frame 8 is smaller, draws less and does not
    const auto measured = measureThenRenderAnother (renderer, scene, setUp);
    EXPECT_EQ (measured.frameIndex, 7u);
    ASSERT_TRUE (measured.gpu.available);
    EXPECT_FALSE (measured.gpu.scopes.empty());
    EXPECT_EQ (measured.internalWidth, 96);
    EXPECT_EQ (measured.internalHeight, 48);
    EXPECT_EQ (measured.drawItems, 10);
    expectScopesMeasured (measured);
}

TEST_F (SceneRendererFrameStatsTests, StatsOffOpensNoScope)
{
    Scene scene;
    buildEveryPassScene (scene);

    SceneRenderer renderer;
    setEveryPass (renderer, RenderSettings::TierPreference::full);
    EXPECT_FALSE (renderer.isFrameStatsEnabled());
    renderAndWait (renderer, scene, 2);
    EXPECT_EQ (renderer.getNumTimingCallsForTesting(), 0);
    EXPECT_EQ (renderer.getLastFrameStats().frameIndex, 0u);

    // On, a frame turns timing on and opens its scopes
    renderer.setFrameStatsEnabled (true);
    renderAndWait (renderer, scene, 1);
    const auto calls = renderer.getNumTimingCallsForTesting();
    EXPECT_GT (calls, 1);
    EXPECT_EQ (renderer.getLastFrameStats().frameIndex, 3u);

    // Off again, the frames make no call, and the stats stay those of the last frame measured
    renderer.setFrameStatsEnabled (false);
    renderAndWait (renderer, scene, 2);
    EXPECT_EQ (renderer.getNumTimingCallsForTesting(), calls);
    EXPECT_EQ (renderer.getLastFrameStats().frameIndex, 3u);
}

TEST_F (SceneRendererFrameStatsTests, HeavierPassMeasuresLonger)
{
    Scene oneLight, manyLights;
    buildLightsScene (oneLight, 1);
    buildLightsScene (manyLights, LightClusters::maxLightsPerCluster);

    SceneRenderer oneRenderer, manyRenderer;
    setLightingOnly (oneRenderer);
    setLightingOnly (manyRenderer);
    oneRenderer.setFrameStatsEnabled (true);
    manyRenderer.setFrameStatsEnabled (true);

    // The first frames compile and warm up, and are not counted
    renderAndWait (oneRenderer, oneLight, 1);
    renderAndWait (manyRenderer, manyLights, 1);

    std::vector<double> light, heavy;
    for (int i = 0; i < 5; ++i)
    {
        renderAndWait (oneRenderer, oneLight, 1);
        renderAndWait (manyRenderer, manyLights, 1);

        light.push_back (getScopeMilliseconds (oneRenderer.getLastFrameStats(), "lighting"));
        heavy.push_back (getScopeMilliseconds (manyRenderer.getLastFrameStats(), "lighting"));
    }

    RecordProperty ("oneLight", String (median (light), 4).toStdString());
    RecordProperty ("manyLights", String (median (heavy), 4).toStdString());

    ASSERT_GE (*std::min_element (light.begin(), light.end()), 0.0);
    EXPECT_GT (median (heavy), median (light) * 4.0);
}

/** Frame times of the Scene3D light show, measured by hand: disabled in the normal suites. Run them
    on a Release build with

    GTEST_ALSO_RUN_DISABLED_TESTS=1 just test Release "SceneRendererBenchmarkTests.*"

    or one of them with "just perf run Release <filter>"; "just perf" alone runs the demo target,
    DISABLED_DemoTargetFrameTimes (see tests/data/yup_3d/perf/README.md). The scene is the demo's: examples/graphics/data/gltf/mpc.glb seen from the top, the shadowed
    key light, the orbiting show lights and a dim environment. The "moving model" configurations
    also shift the model every frame, so the shadow maps are drawn again in each one, as while
    dragging it in the demo. The "orbiting camera" configurations turn the camera around the model
    every frame instead, as the demo's free movement does from its top view (or from 0.6 rad above
    the horizon, "low"), and print how many of the timed frames drew shadow maps. Each configuration renders 30
    warm-up frames, then times 120 queued frames by wall time up to the end of the last one on the
    GPU: the GPU cost of a frame wherever the GPU is the limit.
*/
class SceneRendererBenchmarkTests : public SceneRendererMetalTests
{
protected:
    /** mpc.glb lit like the Scene3D demo, its show lights moved by animate(). */
    struct LightShow
    {
        Scene scene;
        RenderSettings renderSettings; // what the renderer timing the scene draws it with
        std::vector<EntityNode*> showLights;
        BoundingBox bounds;
        EntityNode* modelRoot = nullptr;
        EntityNode* body = nullptr; // the device the camera frames, or the model root without it
        EntityNode* cameraEntity = nullptr;
        std::vector<Material::Ptr> materials; // the model's
        Vector3<float> modelPosition;
        float cameraDistance = 0.0f;
        bool moveModel = false;
        float orbitPitch = 0.0f; // the camera orbits at this pitch when not 0
        EntityNode* knob = nullptr; // turned a little every frame when set, as by automation
        Quaternion knobRotation;
        double time = 0.0;
        int frame = 0;

        /** Builds the scene, or returns false without the model. */
        bool build (int numShowLights, float showRange, float keyRadius, RenderSettings::TierPreference tier, float aspectRatio)
        {
            const auto file = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getChildFile ("examples/graphics/data/gltf/mpc.glb");
            auto model = GltfModel::loadFromFile (file);
            if (model.failed())
                return false;

            auto root = model.getReference().createEntity();
            scene.getRoot()->addChild (root);
            materials = model.getReference().getMaterials();

            auto* device = root->findChild ("Cube");
            auto* framed = device != nullptr ? device : root.get();
            root->setPosition (-framed->computeWorldBounds().getCenter());
            bounds = framed->computeWorldBounds();
            body = framed;
            modelRoot = root.get();
            modelPosition = root->getPosition();

            // The top view of the demo, fitted to the aspect ratio
            auto camera = EntityNode::Ptr (new EntityNode ("camera"));
            auto& cameraNode = camera->attach<CameraNode>();
            const auto radius = jmax (0.001f, bounds.getRadius());
            cameraNode.zNear = radius * 0.02f;
            cameraNode.zFar = radius * 40.0f;

            const auto tanHalfFov = std::tan (cameraNode.yFov * 0.5f);
            const auto size = bounds.getSize();
            const auto distance = jmax (size.getX() * 0.5f / tanHalfFov, size.getZ() * 0.5f / (tanHalfFov * aspectRatio)) * 1.03f;
            const auto center = bounds.getCenter();
            aimCamera (*camera, center + Vector3<float> (0.0f, distance, 0.0f), center, { -1.0f, 0.0f, 0.0f });
            scene.getRoot()->addChild (camera);
            scene.setActiveCamera (camera.get());
            cameraEntity = camera.get();
            cameraDistance = distance;

            auto key = EntityNode::Ptr (new EntityNode ("keyLight"));
            auto& keyLight = key->attach<LightNode>();
            keyLight.castsShadows = true;
            keyLight.intensity = 4.0f;
            keyLight.radius = keyRadius;
            keyLight.color = { 1.0f, 0.96f, 0.9f };

            const auto azimuth = degreesToRadians (120.0f);
            const auto elevation = degreesToRadians (28.0f);
            const Vector3<float> keyDirection (std::cos (elevation) * std::cos (azimuth), std::sin (elevation), std::cos (elevation) * std::sin (azimuth));
            aimCamera (*key, keyDirection * 10.0f, {}, { 0.0f, 1.0f, 0.0f });
            scene.getRoot()->addChild (key);

            for (int i = 0; i < numShowLights; ++i)
            {
                auto entity = EntityNode::Ptr (new EntityNode ("showLight"));
                auto& showLight = entity->attach<LightNode> (LightNode::Type::point);
                const auto color = Color::fromHSV (static_cast<float> (i) / static_cast<float> (numShowLights), 0.85f, 1.0f);
                showLight.color = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat() };
                showLight.intensity = 0.5f;
                showLight.range = showRange;
                showLights.push_back (entity.get());
                scene.getRoot()->addChild (entity);
            }

            scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& direction)
            {
                const auto up = jlimit (0.0f, 1.0f, direction.getY() * 0.5f + 0.5f);
                return Vector3<float> (0.05f, 0.05f, 0.05f) + Vector3<float> (2.0f, 1.9f, 1.8f) * (up * up * up);
            }));

            scene.setEnvironmentIntensity (0.35f);
            scene.setAmbientColor (Colors::black);
            scene.setExposure (1.1f);
            scene.setToneMapping (Scene::ToneMapping::agx);
            scene.setEnvironmentVisible (true);
            scene.setEnvironmentBlur (0.5f);
            renderSettings.tier = tier;
            return true;
        }

        /** Adds to the built scene what examples/graphics/source/examples/Scene3D.h has on top of it:
            the contact shadows of the key light, the shadowed spot light over the pads, the box
            reflection probe above them, the 4 x 2 x 4 irradiance volume over the panel, the materials
            as its tuneMaterials() leaves them, the glossy emissive screen of its attachScreen(), without
            the LCD texture, and the logo it drops, and its ambient occlusion at the scale of the knobs.
            Returns false when the model has no pads to place them by.
        */
        bool matchDemo()
        {
            BoundingBox padBounds;
            for (const auto& child : body->getChildren())
                if (child->getName().startsWith ("Pad."))
                    padBounds.expand (child->computeWorldBounds());

            if (padBounds.isEmpty())
                return false;

            auto& root = *scene.getRoot();
            root.findChild ("keyLight")->getNode<LightNode>()->contactShadows = true;

            const auto padCenter = padBounds.getCenter();
            const auto azimuth = degreesToRadians (-45.0f);
            const auto elevation = degreesToRadians (45.0f);
            const Vector3<float> padDirection (std::cos (elevation) * std::cos (azimuth), std::sin (elevation), std::cos (elevation) * std::sin (azimuth));

            auto padEntity = EntityNode::Ptr (new EntityNode ("padLight"));
            auto& pad = padEntity->attach<LightNode> (LightNode::Type::spot);
            pad.castsShadows = true;
            pad.intensity = 10.0f;
            pad.radius = 0.03f;
            pad.range = 4.0f;
            pad.innerConeAngle = 0.3f;
            pad.outerConeAngle = 0.6f;
            pad.color = { 0.9f, 0.94f, 1.0f };
            aimCamera (*padEntity, padCenter + padDirection * 1.6f, padCenter, { 0.0f, 1.0f, 0.0f });
            root.addChild (padEntity);

            const auto center = bounds.getCenter();
            const auto size = bounds.getSize();
            const auto probePosition = padCenter + Vector3<float> (0.0f, padBounds.getSize().getY() * 0.5f + 0.25f, 0.0f);
            auto probeEntity = EntityNode::Ptr (new EntityNode ("reflectionProbe"));
            probeEntity->setPosition (probePosition);
            auto& probe = probeEntity->attach<ReflectionProbeNode>();
            probe.shape = ReflectionProbeNode::Shape::box;
            probe.influenceExtents = { std::abs (center.getX() - probePosition.getX()) + size.getX() * 0.5f + 0.3f,
                                       probePosition.getY() - bounds.getMin().getY() + 0.1f,
                                       std::abs (center.getZ() - probePosition.getZ()) + size.getZ() * 0.5f + 0.3f };
            probe.falloff = 0.2f;
            probe.parallaxCorrection = true;
            root.addChild (probeEntity);

            const auto layerSpacing = 0.45f;
            const auto volumeBottom = padBounds.getMax().getY() + 0.05f - layerSpacing * 0.5f;
            const auto volumeTop = volumeBottom + layerSpacing * 2.0f;
            auto volumeEntity = EntityNode::Ptr (new EntityNode ("irradianceVolume"));
            volumeEntity->setPosition ({ center.getX(), (volumeBottom + volumeTop) * 0.5f, center.getZ() });
            auto& volume = volumeEntity->attach<IrradianceVolumeNode>();
            volume.extents = { size.getX() * 0.5f + 0.6f, (volumeTop - volumeBottom) * 0.5f, size.getZ() * 0.5f + 0.6f };
            volume.resolutionX = 4;
            volume.resolutionY = 2;
            volume.resolutionZ = 4;
            volume.falloff = 0.15f;
            root.addChild (volumeEntity);

            for (const auto& material : materials)
            {
                if (material->name == "Faceplate" || material->name == "Case")
                {
                    material->roughnessFactor = 0.7f;
                }
                else if (material->name == "Knob Rubber" || material->name == "Black Plastic")
                {
                    material->roughnessFactor = 0.45f;
                    material->clearcoatFactor = 1.0f;
                    material->clearcoatRoughnessFactor = 0.08f;
                }
                else if (material->name == "Pad Rubber")
                {
                    material->sheenColorFactor = { 0.35f, 0.36f, 0.38f };
                    material->sheenRoughnessFactor = 0.5f;
                }
                else if (material->name == "Glass")
                {
                    material->baseColorFactor = { 0.02f, 0.02f, 0.02f, 1.0f };
                    material->metallicFactor = 0.0f;
                    material->roughnessFactor = 0.05f;
                    material->emissiveFactor = { 1.6f, 1.6f, 1.6f };
                }
            }

            if (auto* meshNode = body->getNode<MeshNode>(); meshNode != nullptr && meshNode->mesh != nullptr)
            {
                auto withoutLogo = Mesh::Ptr (new Mesh (meshNode->mesh->getName()));
                for (const auto& primitive : meshNode->mesh->getPrimitives())
                    if (primitive.material == nullptr || primitive.material->name != "Material.001")
                        withoutLogo->addPrimitive (primitive.vertices, primitive.indices, primitive.material);

                meshNode->mesh = withoutLogo;
            }

            renderSettings.ambientOcclusion = true;
            renderSettings.aoDistance = 0.08f;
            return true;
        }

        /** Moves the show lights one 120 Hz frame further along the orbits of the demo's LightShowAnimator,
            and the model by a hundredth of its size when moveModel is set. */
        void animate()
        {
            time += 1.0 / 120.0;

            if (knob != nullptr)
                knob->setRotation (knobRotation * Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.05f * static_cast<float> (++frame % 32)));

            if (moveModel)
                modelRoot->setPosition (modelPosition + Vector3<float> (bounds.getSize().getX() * 0.01f * static_cast<float> (++frame % 2), 0.0f, 0.0f));

            const auto center = bounds.getCenter();

            // The demo's orbit, the yaw turning 0.008 rad a frame
            if (orbitPitch != 0.0f)
            {
                const auto yaw = 0.008f * static_cast<float> (++frame);
                const Vector3<float> offset (std::cos (yaw) * std::cos (orbitPitch), std::sin (orbitPitch), -std::sin (yaw) * std::cos (orbitPitch));
                aimCamera (*cameraEntity, center + offset * cameraDistance, center);
            }

            const auto halfSize = bounds.getSize() * 0.5f;

            for (size_t i = 0; i < showLights.size(); ++i)
            {
                const auto ring = static_cast<float> (i % 4);
                const auto scale = 0.3f + 0.25f * ring;
                const auto speed = (i % 2 == 0 ? 0.35f : -0.25f) * (1.0f + 0.15f * ring);
                const auto angle = static_cast<float> (i) * 2.39996f + speed * static_cast<float> (time);
                const auto height = halfSize.getY() * (0.6f + 0.25f * ring);

                showLights[i]->setPosition ({ center.getX() + std::cos (angle) * halfSize.getX() * scale,
                                              center.getY() + height,
                                              center.getZ() + std::sin (angle) * halfSize.getZ() * scale });
            }
        }
    };

    /** The frame stats of the timed frames of timeFrames() that arrived, and the CPU time of their render() calls. */
    struct FrameSamples
    {
        std::vector<double> gpu;                      // GpuFrameTimings::frameMilliseconds, one per frame whose stats arrived
        std::vector<double> cpu;                      // the milliseconds of each timed render() call, all 120
        std::map<String, std::vector<double>> scopes; // the milliseconds of each available scope, by name
        int drawItems = 0;                            // of the last frame whose stats arrived
        RenderCapabilities::Tier tier = RenderCapabilities::Tier::raster; // the tier the last frame whose stats arrived rendered with
        int internalWidth = 0;
        int internalHeight = 0;
        bool localIndirect = false; // the last frame whose stats arrived shaded with the local indirect LightingPass variant
        bool layers = false;        // ... with its material layers variant
    };

    /** The wall milliseconds per frame of 120 queued frames after 30 warm-up ones, or -1 on a failure.
        @p shadowRedraws receives the number of the timed frames that drew shadow maps, and
        @p casterDraws the shadow caster draws of the timed frames. The renderer collects its frame
        stats when @p frameStats or @p samples is set, and waits for its frames after every @p maxFramesInFlight
        ones when it is above 0. @p samples receives the stats of the timed frames, read after every
        render() and every wait: only the latest finished frame is kept, so a wait finishing two frames
        leaves the first one out. */
    double timeFrames (LightShow& show, int width, int height, int& shadowRedraws, int* casterDraws = nullptr,
                       bool frameStats = false, int maxFramesInFlight = 0, FrameSamples* samples = nullptr)
    {
        SceneRenderer renderer;
        renderer.setRenderSettings (show.renderSettings);
        renderer.setFrameStatsEnabled (frameStats || samples != nullptr);

        auto timed = false;
        uint64 lastFrame = 0;
        const auto take = [&]
        {
            const auto& stats = renderer.getLastFrameStats();
            if (samples == nullptr || ! timed || stats.frameIndex <= lastFrame || ! stats.gpu.available)
                return;

            lastFrame = stats.frameIndex;
            samples->gpu.push_back (stats.gpu.frameMilliseconds);
            for (const auto& scope : stats.gpu.scopes)
                if (scope.available)
                    samples->scopes[scope.name].push_back (scope.milliseconds);

            samples->drawItems = stats.drawItems;
            if (const auto* capabilities = renderer.getCapabilities())
                samples->tier = capabilities->getTier();
            samples->internalWidth = stats.internalWidth;
            samples->internalHeight = stats.internalHeight;
            if (const auto* lightingPass = renderer.getLightingPass())
            {
                samples->localIndirect = lightingPass->usesLocalIndirect();
                samples->layers = lightingPass->usesMaterialLayers();
            }
        };

        const auto renderFrames = [&] (int count)
        {
            for (int i = 0; i < count; ++i)
            {
                show.animate();
                const auto renderStart = Time::getMillisecondCounterHiRes();
                if (renderer.render (device, show.scene, width, height) == nullptr)
                    return false;

                if (samples != nullptr && timed)
                    samples->cpu.push_back (Time::getMillisecondCounterHiRes() - renderStart);

                take();

                if (maxFramesInFlight > 0 && (i + 1) % maxFramesInFlight == 0 && renderer.waitForGPU().failed())
                    return false;

                take();
            }

            const auto finished = renderer.waitForGPU().wasOk();
            take();
            return finished;
        };

        if (! renderFrames (30))
            return -1.0;

        timed = true;
        lastFrame = renderer.getLastFrameStats().frameIndex;
        const auto redrawsBefore = shadows (renderer).getNumRenderedFrames();
        const auto drawsBefore = shadows (renderer).getNumCasterDraws();
        const auto start = Time::getMillisecondCounterHiRes();
        if (! renderFrames (120))
            return -1.0;

        shadowRedraws = shadows (renderer).getNumRenderedFrames() - redrawsBefore;
        if (casterDraws != nullptr)
            *casterDraws = shadows (renderer).getNumCasterDraws() - drawsBefore;

        // Frames that measured only their total would leave nothing measured to compare
        if (frameStats)
        {
            const auto& stats = renderer.getLastFrameStats();
            const auto measured = std::count_if (stats.gpu.scopes.begin(), stats.gpu.scopes.end(), [] (const auto& scope) { return scope.available; });
            EXPECT_TRUE (stats.gpu.available);
            EXPECT_GT (measured, 0);
            std::cout << "frame " << stats.frameIndex << ": " << measured << " of " << stats.gpu.scopes.size() << " scopes measured" << std::endl;
        }

        return (Time::getMillisecondCounterHiRes() - start) / 120.0;
    }

    /** What compareWithBaseline() decides for a row. */
    enum class Verdict
    {
        pass,      ///< Within 3% of the baseline, within the A/A spread of the run, or faster.
        regressed, ///< More than 3% slower than the baseline, and slower by more than the A/A spread.
        newRow,    ///< The baseline has no such row.
        invalid    ///< A time is not a positive finite number, or the spread is negative or not finite.
    };

    /** Whether a row of DISABLED_DemoTargetFrameTimes regressed against its baseline.

        @param baseline  The baseline GPU median of the row in milliseconds, empty when the baseline lacks the row.
        @param current   The GPU median of the row in this run, in milliseconds.
        @param spread    The A/A spread of the row in this run, |a - b| of its two rounds, in milliseconds.
    */
    static Verdict compareWithBaseline (std::optional<double> baseline, double current, double spread)
    {
        if (! std::isfinite (current) || current <= 0.0 || ! std::isfinite (spread) || spread < 0.0)
            return Verdict::invalid;

        if (! baseline.has_value())
            return Verdict::newRow;

        if (! std::isfinite (*baseline) || *baseline <= 0.0)
            return Verdict::invalid;

        return current > *baseline * 1.03 && current - *baseline > spread ? Verdict::regressed : Verdict::pass;
    }

    /** The value below which @p fraction of @p values lie, 0.5 for the median; 0 without values. */
    static double percentile (std::vector<double> values, double fraction)
    {
        if (values.empty())
            return 0.0;

        std::sort (values.begin(), values.end());
        return values[jmin (values.size() - 1, static_cast<size_t> (fraction * static_cast<double> (values.size())))];
    }
};

TEST_F (SceneRendererBenchmarkTests, CompareWithBaselinePassesEqualSmallAndFasterChanges)
{
    EXPECT_EQ (compareWithBaseline (10.0, 10.0, 0.0), Verdict::pass);
    EXPECT_EQ (compareWithBaseline (10.0, 10.2, 0.0), Verdict::pass);
    EXPECT_EQ (compareWithBaseline (10.0, 8.0, 0.0), Verdict::pass);
    EXPECT_EQ (compareWithBaseline (10.0, 8.0, 5.0), Verdict::pass);
}

TEST_F (SceneRendererBenchmarkTests, CompareWithBaselinePassesSlowdownInsideTheSpread)
{
    EXPECT_EQ (compareWithBaseline (10.0, 10.4, 0.5), Verdict::pass);
    EXPECT_EQ (compareWithBaseline (10.0, 10.4, 1.0), Verdict::pass);
}

TEST_F (SceneRendererBenchmarkTests, CompareWithBaselineFlagsSlowdownOutsideTheSpread)
{
    EXPECT_EQ (compareWithBaseline (10.0, 10.4, 0.1), Verdict::regressed);
    EXPECT_EQ (compareWithBaseline (10.0, 10.4, 0.0), Verdict::regressed);
    EXPECT_EQ (compareWithBaseline (10.0, 20.0, 1.0), Verdict::regressed);
}

TEST_F (SceneRendererBenchmarkTests, CompareWithBaselineReportsMissingRowAsNew)
{
    EXPECT_EQ (compareWithBaseline (std::nullopt, 10.0, 0.1), Verdict::newRow);
}

TEST_F (SceneRendererBenchmarkTests, CompareWithBaselineRejectsZeroAndNegativeInputs)
{
    EXPECT_EQ (compareWithBaseline (0.0, 10.0, 0.1), Verdict::invalid);
    EXPECT_EQ (compareWithBaseline (-1.0, 10.0, 0.1), Verdict::invalid);
    EXPECT_EQ (compareWithBaseline (10.0, 0.0, 0.1), Verdict::invalid);
    EXPECT_EQ (compareWithBaseline (10.0, -2.0, 0.1), Verdict::invalid);
    EXPECT_EQ (compareWithBaseline (10.0, 10.0, -0.1), Verdict::invalid);
    EXPECT_EQ (compareWithBaseline (10.0, std::numeric_limits<double>::quiet_NaN(), 0.1), Verdict::invalid);
}

TEST_F (SceneRendererBenchmarkTests, DISABLED_LightShowFrameTimes)
{
    const struct
    {
        const char* name;
        RenderSettings::TierPreference tier;
        int showLights;
        float range;
        bool moveModel = false;
        float orbitPitch = 0.0f;
        bool keyContactShadows = false;
        bool softShadows = true;
        bool ambientOcclusion = true; // follows the tier default when true, off otherwise
        bool probes = false;          // a box probe around the model and a sphere probe inside it
        bool volume = false;          // a 6 x 2 x 4 irradiance volume around the model
        bool layers = false;          // a clearcoat on the knob caps and a sheen on the pad rubber
        bool screenSpaceReflections = true; // follows the tier default when true, off otherwise
    } configs[] = {
        { "full show64", RenderSettings::TierPreference::full, 64, 0.5f },
        { "raster show64", RenderSettings::TierPreference::raster, 64, 0.5f },
        { "raster key+7", RenderSettings::TierPreference::raster, 7, 0.5f },
        { "full show64 range 0.35", RenderSettings::TierPreference::full, 64, 0.35f },
        { "raster show16 range 0.35", RenderSettings::TierPreference::raster, 16, 0.35f },
        { "full show64 range 0.35 moving model", RenderSettings::TierPreference::full, 64, 0.35f, true },
        { "raster show16 range 0.35 moving model", RenderSettings::TierPreference::raster, 16, 0.35f, true },
        { "full show64 range 0.35 orbiting camera", RenderSettings::TierPreference::full, 64, 0.35f, false, 1.56f },
        { "raster show16 range 0.35 orbiting camera", RenderSettings::TierPreference::raster, 16, 0.35f, false, 1.56f },
        { "full show64 range 0.35 orbiting camera low", RenderSettings::TierPreference::full, 64, 0.35f, false, 0.6f },
        { "raster show16 range 0.35 orbiting camera low", RenderSettings::TierPreference::raster, 16, 0.35f, false, 0.6f },
        { "full show64 key contact shadows", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, true },
        { "full show64 soft shadows off", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, false },
        { "full show64 ambient occlusion off", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, false },
        { "full show64 two probes", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, true, true },
        { "raster show64 two probes", RenderSettings::TierPreference::raster, 64, 0.5f, false, 0.0f, false, true, true, true },
        { "full show64 one volume", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, true, false, true },
        { "raster show64 one volume", RenderSettings::TierPreference::raster, 64, 0.5f, false, 0.0f, false, true, true, false, true },
        { "full show64 layered materials", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, true, false, false, true },
        { "raster show64 layered materials", RenderSettings::TierPreference::raster, 64, 0.5f, false, 0.0f, false, true, true, false, false, true },
        { "full show64 screen-space reflections off", RenderSettings::TierPreference::full, 64, 0.5f, false, 0.0f, false, true, true, false, false, false, false },
    };

    for (const auto& [width, height] : { std::pair { 2560, 1600 }, std::pair { 3456, 2234 } })
    {
        for (const auto& config : configs)
        {
            for (const auto keyRadius : { 0.08f, 0.0f })
            {
                LightShow show;
                if (! show.build (config.showLights, config.range, keyRadius, config.tier, static_cast<float> (width) / static_cast<float> (height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                show.moveModel = config.moveModel;
                show.orbitPitch = config.orbitPitch;
                show.scene.getRoot()->findChild ("keyLight")->getNode<LightNode>()->contactShadows = config.keyContactShadows;

                if (! config.softShadows)
                {
                    auto settings = show.renderSettings;
                    settings.softShadows = false;
                    show.renderSettings = settings;
                }

                if (! config.ambientOcclusion)
                {
                    auto settings = show.renderSettings;
                    settings.ambientOcclusion = false;
                    show.renderSettings = settings;
                }

                if (! config.screenSpaceReflections)
                {
                    auto settings = show.renderSettings;
                    settings.screenSpaceReflections = false;
                    show.renderSettings = settings;
                }

                // Baked by the warm-up frames: the probes stay still
                if (config.probes)
                {
                    const auto bounds = show.modelRoot->computeWorldBounds();
                    const auto center = bounds.getCenter();
                    const auto size = bounds.getSize();

                    auto room = EntityNode::Ptr (new EntityNode ("roomProbe"));
                    auto& box = room->attach<ReflectionProbeNode>();
                    box.shape = ReflectionProbeNode::Shape::box;
                    box.influenceExtents = size * 0.75f;
                    room->setPosition (center);
                    show.scene.getRoot()->addChild (room);

                    auto local = EntityNode::Ptr (new EntityNode ("localProbe"));
                    local->attach<ReflectionProbeNode>().influenceRadius = size.length() * 0.25f;
                    local->setPosition (center + size * 0.2f);
                    show.scene.getRoot()->addChild (local);
                }

                // Baked by the warm-up frames too: the volume stays still
                if (config.volume)
                {
                    const auto bounds = show.modelRoot->computeWorldBounds();

                    auto entity = EntityNode::Ptr (new EntityNode ("volume"));
                    auto& volume = entity->attach<IrradianceVolumeNode>();
                    volume.extents = bounds.getSize() * 0.75f;
                    volume.resolutionX = 6;
                    volume.resolutionY = 2;
                    volume.resolutionZ = 4;
                    entity->setPosition (bounds.getCenter());
                    show.scene.getRoot()->addChild (entity);
                }

                // Shared by every primitive drawn with them, so the knobs and the pads change everywhere
                for (const auto& material : config.layers ? show.materials : std::vector<Material::Ptr>())
                {
                    if (material->name == "Knob Cap")
                    {
                        material->clearcoatFactor = 1.0f;
                        material->clearcoatRoughnessFactor = 0.1f;
                    }
                    else if (material->name == "Pad Rubber")
                    {
                        material->sheenColorFactor = { 0.8f, 0.8f, 0.8f };
                        material->sheenRoughnessFactor = 0.5f;
                    }
                }

                auto shadowRedraws = 0;
                const auto milliseconds = timeFrames (show, width, height, shadowRedraws);
                EXPECT_GT (milliseconds, 0.0) << config.name;

                const auto label = String (config.name) + ", key radius " + String (keyRadius, 2) + ", " + String (width) + "x" + String (height);
                RecordProperty (label.toStdString(), String (milliseconds, 3).toStdString());
                std::cout << label.toStdString() << ": " << String (milliseconds, 3).toStdString() << " ms, "
                          << shadowRedraws << " of 120 frames drew shadow maps" << std::endl;
            }
        }
    }
}

/** The cost of the depth of field on the full tier, in the light show with 16 show lights, at the
    demo size and at 2560 x 1600. Run it like DISABLED_LightShowFrameTimes. Each round times the
    frame without depth of field twice (A/A) and with it twice: focused on the model, where only the
    background blurs, and focused well in front of it, where every pixel blurs by the largest size.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_DepthOfFieldFrameTimes)
{
    for (const auto& [width, height] : { std::pair { 1280, 800 }, std::pair { 2560, 1600 } })
    {
        std::map<String, std::vector<double>> times;

        for (int round = 0; round < 3; ++round)
        {
            for (const auto [name, focus] : { std::pair { "off", 0.0f }, std::pair { "focused", 1.0f }, std::pair { "all blurred", 0.3f }, std::pair { "off again", 0.0f } })
            {
                LightShow show;
                if (! show.build (16, 0.5f, 0.08f, RenderSettings::TierPreference::full, static_cast<float> (width) / static_cast<float> (height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                auto settings = show.renderSettings;
                settings.depthOfField = focus > 0.0f;
                show.renderSettings = settings;

                auto& camera = *show.scene.getActiveCamera();
                camera.fStop = 1.4f;
                camera.focusDistance = show.cameraDistance * focus;

                int shadowRedraws = 0;
                const auto milliseconds = timeFrames (show, width, height, shadowRedraws);
                ASSERT_GT (milliseconds, 0.0) << name;
                times[name].push_back (milliseconds);
            }
        }

        for (const auto& [name, values] : times)
        {
            String line;
            for (const auto value : values)
                line << String (value, 3) << " ";

            std::cout << width << " x " << height << " " << name << ": " << line << "ms" << std::endl;
        }
    }
}

/** Raster depth of field in the light show with 16 show lights, focused on the model at f/1.4: the
    frames without it, with it, with ambient occlusion, with both, and without it again (A/A), on a
    renderer each, interleaved over 3 rounds, at the demo size and at 2560 x 1600. Prints the wall
    milliseconds per frame and the mean GPU milliseconds of the frame and of the passes depth of field
    adds or changes, measured through the frame stats with 2 frames in flight, and the load average
    of each round. Run it like DISABLED_LightShowFrameTimes, in Release.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_RasterDepthOfFieldFrameTimes)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    const char* const columns[] = { "wall", "gpu", "depthNormalPrepass", "ambientOcclusion", "forward", "depthOfField", "toneMapping" };

    const struct
    {
        const char* name;
        bool depthOfField;
        bool ambientOcclusion;
    } configs[] = {
        { "a off", false, false },
        { "b dof", true, false },
        { "c ao", false, true },
        { "d ao + dof", true, true },
        { "e off again", false, false },
    };

    for (const auto& [width, height] : { std::pair { 1280, 800 }, std::pair { 2560, 1600 } })
    {
        std::map<String, std::map<String, std::vector<double>>> times;

        for (int round = 0; round < 3; ++round)
        {
            double load[3] = {};
            getloadavg (load, 3);
            std::cout << width << " x " << height << " round " << round << ": load " << String (load[0], 2) << std::endl;

            for (const auto& config : configs)
            {
                LightShow show;
                if (! show.build (16, 0.5f, 0.08f, RenderSettings::TierPreference::raster, static_cast<float> (width) / static_cast<float> (height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                auto settings = show.renderSettings;
                settings.depthOfField = config.depthOfField;
                settings.ambientOcclusion = config.ambientOcclusion;
                show.renderSettings = settings;

                auto& camera = *show.scene.getActiveCamera();
                camera.fStop = 1.4f;
                camera.focusDistance = show.cameraDistance;

                SceneRenderer renderer;
                renderer.setRenderSettings (show.renderSettings);
                renderer.setFrameStatsEnabled (true);

                std::map<String, double> sums;
                int measured = 0;
                uint64 lastFrame = 0;
                const auto take = [&]
                {
                    const auto& stats = renderer.getLastFrameStats();
                    if (stats.frameIndex == lastFrame || ! stats.gpu.available)
                        return;

                    lastFrame = stats.frameIndex;
                    ++measured;
                    sums["gpu"] += stats.gpu.frameMilliseconds;
                    for (const auto& scope : stats.gpu.scopes)
                        if (scope.available)
                            sums[scope.name] += scope.milliseconds;
                };

                const auto renderFrames = [&] (int count, bool collect)
                {
                    for (int i = 0; i < count; ++i)
                    {
                        show.animate();
                        if (renderer.render (device, show.scene, width, height) == nullptr)
                            return false;

                        if ((i + 1) % 2 == 0 && renderer.waitForGPU().failed())
                            return false;

                        if (collect)
                            take();
                    }

                    return renderer.waitForGPU().wasOk();
                };

                ASSERT_TRUE (renderFrames (30, false)) << config.name;
                lastFrame = renderer.getLastFrameStats().frameIndex;

                const auto start = Time::getMillisecondCounterHiRes();
                ASSERT_TRUE (renderFrames (120, true)) << config.name;
                const auto wall = (Time::getMillisecondCounterHiRes() - start) / 120.0;
                take();
                ASSERT_GT (measured, 0) << config.name;

                times[config.name]["wall"].push_back (wall);
                for (const auto* column : columns)
                    if (String (column) != "wall")
                        times[config.name][column].push_back (sums[column] / measured);
            }
        }

        for (const auto& config : configs)
        {
            String line;
            line << width << " x " << height << " " << config.name << ":";
            for (const auto* column : columns)
            {
                line << " " << column;
                for (const auto value : times[config.name][column])
                    line << " " << String (value, 3);
            }

            std::cout << line << std::endl;
        }
    }
}

/** Motion blur in the full tier light show with 16 show lights: off and on while the camera orbits
    (every frame moves, so every frame smears), on and off while only the lights move (still frames,
    which skip it), and off again while orbiting (A/A), on a renderer each, interleaved over 3
    rounds, at the demo size and at 2560 x 1600. Prints the wall milliseconds per frame and the mean
    GPU milliseconds of the frame and of the passes motion blur adds or changes, measured through
    the frame stats with 2 frames in flight, and the load average of each round. Run it like
    DISABLED_LightShowFrameTimes, in Release.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_MotionBlurFrameTimes)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    const char* const columns[] = { "wall", "gpu", "temporalAA", "motionBlur", "toneMapping" };

    const struct
    {
        const char* name;
        bool motionBlur;
        bool orbit;
    } configs[] = {
        { "a off, orbiting", false, true },
        { "b on, orbiting", true, true },
        { "c on, still", true, false },
        { "d off, still", false, false },
        { "e off, orbiting again", false, true },
    };

    for (const auto& [width, height] : { std::pair { 1280, 800 }, std::pair { 2560, 1600 } })
    {
        std::map<String, std::map<String, std::vector<double>>> times;

        for (int round = 0; round < 3; ++round)
        {
            double load[3] = {};
            getloadavg (load, 3);
            std::cout << width << " x " << height << " round " << round << ": load " << String (load[0], 2) << std::endl;

            for (const auto& config : configs)
            {
                LightShow show;
                if (! show.build (16, 0.5f, 0.08f, RenderSettings::TierPreference::full, static_cast<float> (width) / static_cast<float> (height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                show.orbitPitch = config.orbit ? 0.6f : 0.0f;

                auto settings = show.renderSettings;
                settings.motionBlur = config.motionBlur;
                show.renderSettings = settings;

                SceneRenderer renderer;
                renderer.setRenderSettings (show.renderSettings);
                renderer.setFrameStatsEnabled (true);

                std::map<String, double> sums;
                int measured = 0;
                uint64 lastFrame = 0;
                const auto take = [&]
                {
                    const auto& stats = renderer.getLastFrameStats();
                    if (stats.frameIndex == lastFrame || ! stats.gpu.available)
                        return;

                    lastFrame = stats.frameIndex;
                    ++measured;
                    sums["gpu"] += stats.gpu.frameMilliseconds;
                    for (const auto& scope : stats.gpu.scopes)
                        if (scope.available)
                            sums[scope.name] += scope.milliseconds;
                };

                const auto renderFrames = [&] (int count, bool collect)
                {
                    for (int i = 0; i < count; ++i)
                    {
                        show.animate();
                        if (renderer.render (device, show.scene, width, height) == nullptr)
                            return false;

                        if ((i + 1) % 2 == 0 && renderer.waitForGPU().failed())
                            return false;

                        if (collect)
                            take();
                    }

                    return renderer.waitForGPU().wasOk();
                };

                ASSERT_TRUE (renderFrames (30, false)) << config.name;
                lastFrame = renderer.getLastFrameStats().frameIndex;

                const auto start = Time::getMillisecondCounterHiRes();
                ASSERT_TRUE (renderFrames (120, true)) << config.name;
                const auto wall = (Time::getMillisecondCounterHiRes() - start) / 120.0;
                take();
                ASSERT_GT (measured, 0) << config.name;

                // Orbiting frames smear, still ones never create the pass
                EXPECT_EQ (renderer.getMotionBlur() != nullptr, config.motionBlur && config.orbit) << config.name;

                times[config.name]["wall"].push_back (wall);
                for (const auto* column : columns)
                    if (String (column) != "wall")
                        times[config.name][column].push_back (sums[column] / measured);
            }
        }

        for (const auto& config : configs)
        {
            String line;
            line << width << " x " << height << " " << config.name << ":";
            for (const auto* column : columns)
            {
                line << " " << column;
                for (const auto value : times[config.name][column])
                    line << " " << String (value, 3);
            }

            std::cout << line << std::endl;
        }
    }
}

/** The cost of the world fog in the light show with 16 show lights while the camera orbits, on both
    tiers, at the demo size and at 2560 x 1600: off, on, then off again (A/A), each with a renderer of
    its own, 3 interleaved rounds. Run it like DISABLED_LightShowFrameTimes.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_WorldFogFrameTimes)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    const char* const columns[] = { "wall", "gpu", "depthNormalPrepass", "forward", "fog", "temporalAA", "toneMapping" };

    const struct
    {
        const char* name;
        RenderSettings::TierPreference tier;
        bool fog;
    } configs[] = {
        { "full a off", RenderSettings::TierPreference::full, false },
        { "full b fog", RenderSettings::TierPreference::full, true },
        { "full c off again", RenderSettings::TierPreference::full, false },
        { "raster d off", RenderSettings::TierPreference::raster, false },
        { "raster e fog", RenderSettings::TierPreference::raster, true },
        { "raster f off again", RenderSettings::TierPreference::raster, false },
    };

    for (const auto& [width, height] : { std::pair { 1280, 800 }, std::pair { 2560, 1600 } })
    {
        std::map<String, std::map<String, std::vector<double>>> times;

        for (int round = 0; round < 3; ++round)
        {
            double load[3] = {};
            getloadavg (load, 3);
            std::cout << width << " x " << height << " round " << round << ": load " << String (load[0], 2) << std::endl;

            for (const auto& config : configs)
            {
                LightShow show;
                if (! show.build (16, 0.5f, 0.08f, config.tier, static_cast<float> (width) / static_cast<float> (height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                show.orbitPitch = 0.6f;

                if (config.fog)
                {
                    WorldFog fog;
                    fog.enabled = true;
                    fog.density = 0.08f;
                    fog.heightFalloff = 0.5f;
                    show.scene.setWorldFog (fog);
                }

                SceneRenderer renderer;
                renderer.setRenderSettings (show.renderSettings);
                renderer.setFrameStatsEnabled (true);

                std::map<String, double> sums;
                int measured = 0;
                uint64 lastFrame = 0;
                const auto take = [&]
                {
                    const auto& stats = renderer.getLastFrameStats();
                    if (stats.frameIndex == lastFrame || ! stats.gpu.available)
                        return;

                    lastFrame = stats.frameIndex;
                    ++measured;
                    sums["gpu"] += stats.gpu.frameMilliseconds;
                    for (const auto& scope : stats.gpu.scopes)
                        if (scope.available)
                            sums[scope.name] += scope.milliseconds;
                };

                const auto renderFrames = [&] (int count, bool collect)
                {
                    for (int i = 0; i < count; ++i)
                    {
                        show.animate();
                        if (renderer.render (device, show.scene, width, height) == nullptr)
                            return false;

                        if ((i + 1) % 2 == 0 && renderer.waitForGPU().failed())
                            return false;

                        if (collect)
                            take();
                    }

                    return renderer.waitForGPU().wasOk();
                };

                ASSERT_TRUE (renderFrames (30, false)) << config.name;
                lastFrame = renderer.getLastFrameStats().frameIndex;

                const auto start = Time::getMillisecondCounterHiRes();
                ASSERT_TRUE (renderFrames (120, true)) << config.name;
                const auto wall = (Time::getMillisecondCounterHiRes() - start) / 120.0;
                take();
                ASSERT_GT (measured, 0) << config.name;

                EXPECT_EQ (renderer.getFogApply() != nullptr, config.fog) << config.name;

                times[config.name]["wall"].push_back (wall);
                for (const auto* column : columns)
                    if (String (column) != "wall")
                        times[config.name][column].push_back (sums[column] / measured);
            }
        }

        for (const auto& config : configs)
        {
            String line;
            line << width << " x " << height << " " << config.name << ":";
            for (const auto* column : columns)
            {
                line << " " << column;
                for (const auto value : times[config.name][column])
                    line << " " << String (value, 3);
            }

            std::cout << line << std::endl;
        }
    }
}

/** The cost of SceneRenderer::setFrameStatsEnabled() in the light show with 64 show lights on the
    full tier, at the demo size, with 2 frames in flight. Run it like DISABLED_LightShowFrameTimes.
    Each round times the frames with the stats off, on, off again (A/A) and on again, each on a device
    of its own: once a device measured a frame, its later frames pay a timestamp pair each.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_FrameStatsOverhead)
{
    std::map<String, std::vector<double>> times;

    for (int round = 0; round < 5; ++round)
    {
        for (const auto [name, frameStats] : { std::pair { "a off", false }, std::pair { "b on", true }, std::pair { "c off again", false }, std::pair { "d on again", true } })
        {
            device = GpuDevice::create (GpuPlatform::Metal, {});
            ASSERT_NE (device, nullptr);

            LightShow show;
            if (! show.build (64, 0.5f, 0.08f, RenderSettings::TierPreference::full, 3456.0f / 2234.0f))
                GTEST_SKIP() << "mpc.glb is missing";

            int shadowRedraws = 0;
            const auto milliseconds = timeFrames (show, 3456, 2234, shadowRedraws, nullptr, frameStats, 2);
            ASSERT_GT (milliseconds, 0.0) << name;
            times[name].push_back (milliseconds);
        }
    }

    for (auto& [name, values] : times)
    {
        String line;
        for (const auto value : values)
            line << String (value, 3) << " ";

        std::sort (values.begin(), values.end());
        std::cout << name << ": " << line << "ms, median " << String (values[values.size() / 2], 3) << " ms" << std::endl;
    }
}

/** The Scene3D panel under automation: the demo's key light and pad spot plus two shadowed point
    lights over the pads, while one knob turns every frame, outside the reach of every point and spot
    light when one is (the test reports whether it is). Run it like DISABLED_LightShowFrameTimes, in
    Debug too, with and without MTL_DEBUG_LAYER=1: it prints the frame time and the shadow caster
    draws per frame.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_KnobAutomationFrameTimes)
{
    for (const auto [tier, screenSpaceReflections] : { std::pair { RenderSettings::TierPreference::full, true },
                                                       std::pair { RenderSettings::TierPreference::full, false },
                                                       std::pair { RenderSettings::TierPreference::raster, true } })
    {
        for (const auto [keyShadows, nearestKnob] : { std::pair { true, false }, std::pair { false, false }, std::pair { true, true }, std::pair { false, true } })
        {
            LightShow show;
            if (! show.build (0, 0.5f, 0.08f, tier, 1.6f))
                GTEST_SKIP() << "mpc.glb is missing";

            if (! screenSpaceReflections)
            {
                auto settings = show.renderSettings;
                settings.screenSpaceReflections = false;
                show.renderSettings = settings;
            }

            auto& root = *show.scene.getRoot();
            auto& key = *root.findChild ("keyLight")->getNode<LightNode>();
            key.contactShadows = true;
            key.castsShadows = keyShadows;

            // The demo's pad spot, from the front right
            BoundingBox padBounds;
            std::vector<EntityNode*> knobs;
            const std::function<void (EntityNode&)> collect = [&] (EntityNode& entity)
            {
                for (const auto& child : entity.getChildren())
                {
                    if (child->getName().startsWith ("Pad."))
                        padBounds.expand (child->computeWorldBounds());
                    else if (child->getName().startsWith ("Knob"))
                        knobs.push_back (child.get());

                    collect (*child);
                }
            };
            collect (*show.modelRoot);
            ASSERT_FALSE (padBounds.isEmpty());
            ASSERT_FALSE (knobs.empty());

            const auto padCenter = padBounds.getCenter();
            const auto fromDegrees = [] (float azimuth, float elevation)
            {
                const auto a = degreesToRadians (azimuth);
                const auto e = degreesToRadians (elevation);
                return Vector3<float> (std::cos (e) * std::cos (a), std::sin (e), std::cos (e) * std::sin (a));
            };

            auto padEntity = EntityNode::Ptr (new EntityNode ("padLight"));
            auto& pad = padEntity->attach<LightNode> (LightNode::Type::spot);
            pad.castsShadows = true;
            pad.intensity = 10.0f;
            pad.radius = 0.03f;
            pad.range = 4.0f;
            pad.innerConeAngle = 0.3f;
            pad.outerConeAngle = 0.6f;
            aimCamera (*padEntity, padCenter + fromDegrees (-45.0f, 45.0f) * 1.6f, padCenter, { 0.0f, 1.0f, 0.0f });
            root.addChild (padEntity);

            // Two small shadowed point lights over the pads
            const auto padSize = padBounds.getSize();
            for (const auto side : { -0.25f, 0.25f })
            {
                auto entity = EntityNode::Ptr (new EntityNode ("padPoint"));
                auto& point = entity->attach<LightNode> (LightNode::Type::point);
                point.castsShadows = true;
                point.intensity = 0.5f;
                point.range = jmax (padSize.getX(), padSize.getZ()) * 0.3f;
                entity->setPosition (padCenter + Vector3<float> (padSize.getX() * side, padSize.getY() + point.range * 0.5f, 0.0f));
                root.addChild (entity);
            }

            // The knob farthest from the pads, which the spot light does not reach, or the nearest one
            const auto byDistance = [&] (EntityNode* a, EntityNode* b)
            {
                return (a->computeWorldBounds().getCenter() - padCenter).length() < (b->computeWorldBounds().getCenter() - padCenter).length();
            };
            auto* knob = nearestKnob ? *std::min_element (knobs.begin(), knobs.end(), byDistance) : *std::max_element (knobs.begin(), knobs.end(), byDistance);

            const auto knobBounds = knob->computeWorldBounds();
            const auto padDirection = (padCenter - padEntity->getPosition()).normalized();
            // The knob's bounding sphere against the spot's cone, widened by a margin over the guard band of its view
            const auto reachesSpot = [&]
            {
                const auto toKnob = knobBounds.getCenter() - padEntity->getPosition();
                const auto distance = toKnob.length();
                const auto angle = std::acos (jlimit (-1.0f, 1.0f, toKnob.dotProduct (padDirection) / distance));
                return angle - std::asin (jmin (1.0f, knobBounds.getRadius() / distance)) <= pad.outerConeAngle + 0.05f;
            };

            auto reachesPoint = false;
            for (const auto& child : root.getChildren())
                if (child->getName() == "padPoint")
                    reachesPoint = reachesPoint || GpuSceneData::lightReachesBox (knobBounds, child->getPosition(), {}, child->getNode<LightNode>()->range, false, 0.0f);

            EXPECT_TRUE (nearestKnob || ! reachesSpot());
            EXPECT_TRUE (nearestKnob || ! reachesPoint);
            show.knob = knob;
            show.knobRotation = knob->getRotation();

            auto shadowRedraws = 0, casterDraws = 0;
            const auto milliseconds = timeFrames (show, 2560, 1600, shadowRedraws, &casterDraws);
            EXPECT_GT (milliseconds, 0.0);

            const auto label = String (tier == RenderSettings::TierPreference::full ? "full" : "raster")
                             + (screenSpaceReflections ? "" : ", screen-space reflections off")
                             + (keyShadows ? ", key shadowed" : ", key unshadowed")
                             + ", knob " + knob->getName() + (reachesSpot() || reachesPoint ? " inside a local light" : " outside every local light");
            RecordProperty (label.toStdString(), String (milliseconds, 3).toStdString());
            std::cout << label.toStdString() << ": " << String (milliseconds, 3).toStdString() << " ms, "
                      << shadowRedraws << " of 120 frames drew shadow maps, "
                      << String (casterDraws / 120.0, 1).toStdString() << " caster draws per frame" << std::endl;
        }
    }
}

/** What each feature of the Scene3D demo target costs on the full tier: the demo's scene
    (LightShow::matchDemo()) with its light show and orbit, as DISABLED_DemoTargetFrameTimes renders
    it, then with one feature off or changed per row. Each row is timed at native Retina and at
    Retina upscaled from 0.5, in three interleaved rounds, and printed with its GPU median and the
    median of each scope, next to their difference with the unchanged demo row, and the
    LightingPass variant it shaded with. Run it with
    "just perf run Release SceneRendererBenchmarkTests.DISABLED_FeatureCostFrameTimes".
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_FeatureCostFrameTimes)
{
    if (! device->isFrameTimingAvailable())
        GTEST_SKIP() << "Frame timing is unavailable on this device";

    const auto setSettings = [] (LightShow& show, auto&& change)
    {
        auto settings = show.renderSettings;
        change (settings);
        show.renderSettings = settings;
    };

    const auto removeLayers = [] (LightShow& show)
    {
        for (const auto& material : show.materials)
        {
            material->clearcoatFactor = 0.0f;
            material->sheenColorFactor = { 0.0f, 0.0f, 0.0f };
        }
    };

    const auto removeProbes = [] (LightShow& show)
    {
        for (const auto* name : { "reflectionProbe", "irradianceVolume" })
            show.scene.getRoot()->findChild (name)->removeFromParent();
    };

    const struct
    {
        const char* name;
        int numShowLights = 64;
        std::function<void (LightShow&)> change;
    } rows[] = {
        { "demo", 64, [] (LightShow&) {} },
        { "ambient occlusion off", 64, [&] (LightShow& show) { setSettings (show, [] (RenderSettings& s) { s.ambientOcclusion = false; }); } },
        { "reflections off", 64, [&] (LightShow& show) { setSettings (show, [] (RenderSettings& s) { s.screenSpaceReflections = false; }); } },
        { "contact shadows off", 64, [&] (LightShow& show) { setSettings (show, [] (RenderSettings& s) { s.contactShadows = false; }); } },
        { "ao + reflections + contact off", 64, [&] (LightShow& show) { setSettings (show, [] (RenderSettings& s) { s.ambientOcclusion = false; s.screenSpaceReflections = false; s.contactShadows = false; }); } },
        { "soft shadows off", 64, [&] (LightShow& show) { setSettings (show, [] (RenderSettings& s) { s.softShadows = false; }); } },
        { "no light radius", 64, [] (LightShow& show)
          {
              for (const auto* name : { "keyLight", "padLight" })
                  show.scene.getRoot()->findChild (name)->getNode<LightNode>()->radius = 0.0f;
          } },
        { "soft shadows off, no light radius", 64, [&] (LightShow& show)
          {
              setSettings (show, [] (RenderSettings& s) { s.softShadows = false; });
              for (const auto* name : { "keyLight", "padLight" })
                  show.scene.getRoot()->findChild (name)->getNode<LightNode>()->radius = 0.0f;
          } },
        { "no layers", 64, removeLayers },
        { "no probes or volume", 64, removeProbes },
        { "no local indirect", 64, [&] (LightShow& show) { removeLayers (show); removeProbes (show); } },
        { "one layered speck", 64, [&] (LightShow& show)
          {
              removeLayers (show);
              removeProbes (show);

              // A clearcoated box a hundredth of the device's size on its top: the layers variant shades every pixel for a few
              auto material = makeWallMaterial();
              material->clearcoatFactor = 1.0f;
              auto speck = EntityNode::Ptr (new EntityNode ("speck"));
              speck->attach<MeshNode> (makeBox (show.bounds.getRadius() * 0.01f, material));
              speck->setPosition (show.bounds.getCenter() + Vector3<float> (0.0f, show.bounds.getSize().getY() * 0.5f, 0.0f));
              show.scene.getRoot()->addChild (speck);
          } },
        { "16 show lights", 16, [] (LightShow&) {} },
        { "no show lights", 0, [] (LightShow&) {} },
    };

    const struct
    {
        const char* name;
        int width;
        int height;
        float renderScale;
    } sizes[] = {
        { "retina 3456x2234 px", 3456, 2234, 1.0f },
        { "retina 3456x2234 px scale 0.5", 3456, 2234, 0.5f },
    };

    constexpr int numRounds = 3;
    std::vector<std::vector<FrameSamples>> results (std::size (sizes) * std::size (rows));

    double load[3] = {};
    getloadavg (load, 3);
    std::cout << String::formatted ("Load before: %.2f %.2f %.2f", load[0], load[1], load[2]) << std::endl;

    for (int round = 0; round < numRounds; ++round)
    {
        for (size_t s = 0; s < std::size (sizes); ++s)
        {
            for (size_t i = 0; i < std::size (rows); ++i)
            {
                const auto& size = sizes[s];
                const auto& row = rows[i];

                LightShow show;
                if (! show.build (row.numShowLights, 0.5f, 0.08f, RenderSettings::TierPreference::full, static_cast<float> (size.width) / static_cast<float> (size.height)))
                    GTEST_SKIP() << "mpc.glb is missing";

                ASSERT_TRUE (show.matchDemo());
                show.orbitPitch = 1.56f;

                if (size.renderScale < 1.0f)
                    setSettings (show, [&] (RenderSettings& settings)
                    {
                        settings.upscaling = RenderSettings::Upscaling::temporal;
                        settings.renderScale = size.renderScale;
                    });

                row.change (show);

                FrameSamples samples;
                auto shadowRedraws = 0;
                ASSERT_GT (timeFrames (show, size.width, size.height, shadowRedraws, nullptr, false, 1, &samples), 0.0) << row.name;
                ASSERT_FALSE (samples.gpu.empty()) << row.name;

                if (samples.tier != RenderCapabilities::Tier::full)
                    GTEST_SKIP() << "This device has no full tier";

                results[s * std::size (rows) + i].push_back (std::move (samples));
            }
        }
    }

    getloadavg (load, 3);
    std::cout << String::formatted ("Load after: %.2f %.2f %.2f", load[0], load[1], load[2]) << std::endl;

    // The median of each round and their mean, of the frame total when scope is empty
    const auto medians = [] (const std::vector<FrameSamples>& rounds, const String& scope)
    {
        std::vector<double> values;
        for (const auto& samples : rounds)
        {
            if (scope.isEmpty())
                values.push_back (percentile (samples.gpu, 0.5));
            else
                values.push_back (samples.scopes.count (scope) != 0 ? percentile (samples.scopes.at (scope), 0.5) : 0.0);
        }

        return values;
    };

    const auto mean = [] (const std::vector<double>& values)
    {
        return std::accumulate (values.begin(), values.end(), 0.0) / static_cast<double> (values.size());
    };

    for (size_t s = 0; s < std::size (sizes); ++s)
    {
        const auto& reference = results[s * std::size (rows)];
        const auto referenceTotal = mean (medians (reference, {}));

        std::cout << "\n"
                  << sizes[s].name << ", internal " << reference[0].internalWidth << "x" << reference[0].internalHeight
                  << ", demo GPU median " << String (referenceTotal, 3) << " ms" << std::endl;

        for (size_t i = 0; i < std::size (rows); ++i)
        {
            const auto& rounds = results[s * std::size (rows) + i];

            // The frame total first, then every scope of the row or the demo row
            std::vector<String> scopes { String() };
            for (const auto* samples : { &rounds[0], &reference[0] })
                for (const auto& [name, values] : samples->scopes)
                    if (std::find (scopes.begin(), scopes.end(), name) == scopes.end())
                        scopes.push_back (name);

            std::cout << String::formatted ("  %-32s variant%s%s", rows[i].name,
                                            rounds[0].localIndirect ? " localIndirect" : "", rounds[0].layers ? " layers" : "")
                      << std::endl;

            for (const auto& scope : scopes)
            {
                const auto values = medians (rounds, scope);
                const auto referenceValue = mean (medians (reference, scope));
                const auto [low, high] = std::minmax_element (values.begin(), values.end());
                const auto value = mean (values);
                if (value == 0.0 && referenceValue == 0.0)
                    continue;

                std::cout << String::formatted ("    %-24s %8.3f ms  %+8.3f ms  %+6.1f%% of the demo frame  rounds %.3f..%.3f",
                                                scope.isEmpty() ? "frame" : scope.toRawUTF8(), value, value - referenceValue,
                                                (value - referenceValue) / referenceTotal * 100.0, *low, *high)
                          << std::endl;
            }
        }
    }
}

/** The Scene3D demo target, timed on the GPU: the demo's scene (LightShow::matchDemo()) with its
    light show on, 64 show lights on the full tier and 16 on the raster tier, while the camera orbits
    from the top as the demo's free movement starts. Two sizes: the demo's 3D view in its default
    1024 x 768 point window at 2x, 1578 x 1404 pixels, and native Retina, 3456 x 2234 pixels. The
    full tier rows are the gate, the raster ones context, and the full tier also runs upscaled from
    0.75 and 0.5 of each size (RenderSettings::upscaling). Each row runs twice, interleaved (A/A), with
    the frame stats on, waiting for each frame: with more in flight, a frame's command buffer time
    also spans the frames it waited behind.

    Prints a table per row, writes the same as JSON to the absolute path in the environment variable
    YUP_PERF_OUTPUT when set, and compares the rows with the JSON of an earlier run named by
    YUP_PERF_BASELINE when set: it reports, it never fails on a regression. Run it with "just perf",
    see tests/data/yup_3d/perf/README.md.
*/
TEST_F (SceneRendererBenchmarkTests, DISABLED_DemoTargetFrameTimes)
{
    if (! device->isFrameTimingAvailable())
        GTEST_SKIP() << "Frame timing is unavailable on this device";

    const struct
    {
        const char* name;
        RenderSettings::TierPreference tier;
        int numShowLights;
        int width;
        int height;
        float renderScale = 1.0f; // temporal upscaling below 1
    } rows[] = {
        { "full demo 1578x1404 px", RenderSettings::TierPreference::full, 64, 1578, 1404 },
        { "full retina 3456x2234 px", RenderSettings::TierPreference::full, 64, 3456, 2234 },
        { "raster demo 1578x1404 px", RenderSettings::TierPreference::raster, 16, 1578, 1404 },
        { "raster retina 3456x2234 px", RenderSettings::TierPreference::raster, 16, 3456, 2234 },
        { "full demo 1578x1404 px scale 0.75", RenderSettings::TierPreference::full, 64, 1578, 1404, 0.75f },
        { "full demo 1578x1404 px scale 0.5", RenderSettings::TierPreference::full, 64, 1578, 1404, 0.5f },
        { "full retina 3456x2234 px scale 0.75", RenderSettings::TierPreference::full, 64, 3456, 2234, 0.75f },
        { "full retina 3456x2234 px scale 0.5", RenderSettings::TierPreference::full, 64, 3456, 2234, 0.5f },
    };

    struct Round
    {
        FrameSamples samples;
        double wallMilliseconds = 0.0;
        int shadowRedraws = 0;
        int casterDraws = 0;
        double shadowsMean = 0.0; // the shadows scope over every sampled frame, those that kept the maps reading 0
        double load = 0.0;
    };

    const auto tierName = [] (RenderCapabilities::Tier tier) { return tier == RenderCapabilities::Tier::full ? "full" : "raster"; };
    std::vector<std::vector<Round>> results (std::size (rows));

    double load[3] = {};
    getloadavg (load, 3);

    for (int round = 0; round < 2; ++round)
    {
        for (size_t i = 0; i < std::size (rows); ++i)
        {
            const auto& row = rows[i];

            LightShow show;
            if (! show.build (row.numShowLights, 0.5f, 0.08f, row.tier, static_cast<float> (row.width) / static_cast<float> (row.height)))
                GTEST_SKIP() << "mpc.glb is missing";

            ASSERT_TRUE (show.matchDemo());
            show.orbitPitch = 1.56f;

            if (row.renderScale < 1.0f)
            {
                auto settings = show.renderSettings;
                settings.upscaling = RenderSettings::Upscaling::temporal;
                settings.renderScale = row.renderScale;
                show.renderSettings = settings;
            }

            Round result;
            double roundLoad[3] = {};
            getloadavg (roundLoad, 3);
            result.load = roundLoad[0];
            result.wallMilliseconds = timeFrames (show, row.width, row.height, result.shadowRedraws, &result.casterDraws, false, 1, &result.samples);
            ASSERT_GT (result.wallMilliseconds, 0.0) << row.name;
            ASSERT_FALSE (result.samples.gpu.empty()) << row.name;

            // A full row timed on the raster tier would gate the wrong renderer under the full label
            if (row.tier == RenderSettings::TierPreference::full && result.samples.tier != RenderCapabilities::Tier::full)
                GTEST_SKIP() << row.name << " rendered on the " << tierName (result.samples.tier) << " tier: this device has no full tier, so the gate rows cannot run";

            if (const auto shadowTimes = result.samples.scopes.find ("shadows"); shadowTimes != result.samples.scopes.end())
                result.shadowsMean = std::accumulate (shadowTimes->second.begin(), shadowTimes->second.end(), 0.0) / static_cast<double> (shadowTimes->second.size());

            results[i].push_back (std::move (result));
        }
    }

    const auto gpuName = SystemStats::getEnvironmentVariable ("YUP_PERF_GPU", "unknown");
#if YUP_DEBUG
    const String config ("Debug");
#else
    const String config ("Release");
#endif

    var json (new DynamicObject());
    json.getDynamicObject()->setProperty ("gpu", gpuName);
    json.getDynamicObject()->setProperty ("config", config);
    json.getDynamicObject()->setProperty ("date", Time::getCurrentTime().toISO8601 (true));
    json.getDynamicObject()->setProperty ("load", Array<var> { load[0], load[1], load[2] });

    Array<var> rowsJson;
    std::vector<double> gpuMedians, spreads;

    for (size_t i = 0; i < std::size (rows); ++i)
    {
        const auto& row = rows[i];
        const auto& rounds = results[i];
        const auto a = percentile (rounds[0].samples.gpu, 0.5);
        const auto b = percentile (rounds[1].samples.gpu, 0.5);
        gpuMedians.push_back ((a + b) * 0.5);
        spreads.push_back (std::abs (a - b));

        std::cout << "\n"
                  << String::formatted ("%s: GPU median %.3f ms (rounds %.3f and %.3f, A/A spread %.1f%%), internal %dx%d",
                                        row.name, gpuMedians.back(), a, b, spreads.back() / gpuMedians.back() * 100.0,
                                        rounds[0].samples.internalWidth, rounds[0].samples.internalHeight)
                  << "\n"
                  << String::formatted ("  %5s %8s %8s %8s %8s %8s %10s %13s %6s %6s", "round", "samples", "gpu med", "gpu p95", "wall", "cpu med", "draw items", "casters/frame", "load", "tier")
                  << std::endl;

        Array<var> roundsJson;
        std::map<String, double> scopeOrder; // the sum of the round medians of each scope
        for (size_t r = 0; r < rounds.size(); ++r)
        {
            const auto& round = rounds[r];
            const auto& samples = round.samples;
            const auto gpuMedian = percentile (samples.gpu, 0.5);
            const auto casterDraws = round.casterDraws / 120.0;

            std::cout << String::formatted ("  %5d %4d/120 %8.3f %8.3f %8.3f %8.3f %10d %13.1f %6.2f %6s",
                                            static_cast<int> (r + 1), static_cast<int> (samples.gpu.size()), gpuMedian, percentile (samples.gpu, 0.95),
                                            round.wallMilliseconds, percentile (samples.cpu, 0.5), samples.drawItems, casterDraws, round.load, tierName (samples.tier))
                      << std::endl;

            var scopesJson (new DynamicObject());
            for (const auto& [name, values] : samples.scopes)
            {
                scopeOrder[name] += percentile (values, 0.5);
                scopesJson.getDynamicObject()->setProperty (name, percentile (values, 0.5));
            }

            var roundJson (new DynamicObject());
            auto& properties = *roundJson.getDynamicObject();
            properties.setProperty ("gpuMedian", gpuMedian);
            properties.setProperty ("gpuP95", percentile (samples.gpu, 0.95));
            properties.setProperty ("samples", static_cast<int> (samples.gpu.size()));
            properties.setProperty ("frames", 120);
            properties.setProperty ("wallMilliseconds", round.wallMilliseconds);
            properties.setProperty ("cpuMedian", percentile (samples.cpu, 0.5));
            properties.setProperty ("shadowsMean", round.shadowsMean);
            properties.setProperty ("shadowRedraws", round.shadowRedraws);
            properties.setProperty ("casterDrawsPerFrame", casterDraws);
            properties.setProperty ("drawItems", samples.drawItems);
            properties.setProperty ("internalWidth", samples.internalWidth);
            properties.setProperty ("internalHeight", samples.internalHeight);
            properties.setProperty ("tier", String (tierName (samples.tier)));
            properties.setProperty ("load", round.load);
            properties.setProperty ("scopes", scopesJson);
            roundsJson.add (roundJson);
        }

        std::vector<std::pair<double, String>> sortedScopes;
        for (const auto& [name, sum] : scopeOrder)
            sortedScopes.push_back ({ sum, name });

        std::sort (sortedScopes.begin(), sortedScopes.end(), [] (const auto& x, const auto& y) { return x.first > y.first; });

        std::cout << String::formatted ("  %-24s %8s %6s %8s %6s", "scope", "r1 med", "share", "r2 med", "share") << std::endl;
        for (const auto& [sum, name] : sortedScopes)
        {
            String line = String::formatted ("  %-24s", name.toRawUTF8());
            for (const auto& round : rounds)
            {
                const auto& values = round.samples.scopes.count (name) != 0 ? round.samples.scopes.at (name) : std::vector<double>();
                const auto value = percentile (values, 0.5);
                line << String::formatted (" %8.3f %5.1f%%", value, value / percentile (round.samples.gpu, 0.5) * 100.0);
            }

            // Frames that keep the shadow maps read 0, so the median alone hides the cost of a redraw
            if (name == "shadows")
                line << String::formatted ("  mean %.3f and %.3f ms, redrawn in %d and %d of 120 frames",
                                           rounds[0].shadowsMean, rounds[1].shadowsMean,
                                           rounds[0].shadowRedraws, rounds[1].shadowRedraws);

            std::cout << line << std::endl;
        }

        if (row.tier == RenderSettings::TierPreference::full && spreads.back() > gpuMedians.back() * 0.03)
            std::cout << String::formatted ("  Warning: the A/A spread of this gate row is %.1f%%, above 3%%: the machine was not idle enough to trust it", spreads.back() / gpuMedians.back() * 100.0) << std::endl;

        var rowJson (new DynamicObject());
        rowJson.getDynamicObject()->setProperty ("name", String (row.name));
        rowJson.getDynamicObject()->setProperty ("gpuMedian", gpuMedians.back());
        rowJson.getDynamicObject()->setProperty ("spread", spreads.back() / gpuMedians.back());
        rowJson.getDynamicObject()->setProperty ("spreadMilliseconds", spreads.back());
        rowJson.getDynamicObject()->setProperty ("rounds", roundsJson);
        rowsJson.add (rowJson);
    }

    json.getDynamicObject()->setProperty ("rows", rowsJson);

    if (const auto outputPath = SystemStats::getEnvironmentVariable ("YUP_PERF_OUTPUT", {}); outputPath.isNotEmpty())
    {
        if (! File::isAbsolutePath (outputPath))
            ADD_FAILURE() << "YUP_PERF_OUTPUT is not an absolute path, nothing written: " << outputPath;
        else if (File (outputPath).getParentDirectory().createDirectory().wasOk() && File (outputPath).replaceWithText (JSON::toString (json)))
            std::cout << "\nWrote " << outputPath << std::endl;
        else
            ADD_FAILURE() << "Could not write " << outputPath;
    }

    const auto baselinePath = SystemStats::getEnvironmentVariable ("YUP_PERF_BASELINE", {});
    if (baselinePath.isEmpty())
        return;

    if (! File::isAbsolutePath (baselinePath) || ! File (baselinePath).existsAsFile())
    {
        std::cout << "\nNo baseline to compare with at " << baselinePath << std::endl;
        return;
    }

    const auto baseline = JSON::parse (File (baselinePath));
    if (! baseline.isObject())
    {
        std::cout << "\nThe baseline is not a JSON object, not compared: " << baselinePath << std::endl;
        return;
    }

    if (baseline["gpu"].toString() != gpuName || baseline["config"].toString() != config)
    {
        std::cout << "\nWarning: the baseline was recorded on " << baseline["gpu"].toString() << " in " << baseline["config"].toString()
                  << ", this run is on " << gpuName << " in " << config << ": not compared" << std::endl;
        return;
    }

    std::cout << "\nCompared with " << baselinePath << " (" << baseline["date"].toString() << "):" << std::endl;
    for (size_t i = 0; i < std::size (rows); ++i)
    {
        std::optional<double> baselineMedian;
        if (const auto* baselineRows = baseline["rows"].getArray(); baselineRows != nullptr)
            for (const auto& baselineRow : *baselineRows)
                if (baselineRow["name"].toString() == rows[i].name)
                    baselineMedian = static_cast<double> (baselineRow["gpuMedian"]);

        const auto current = gpuMedians[i];
        const auto verdict = compareWithBaseline (baselineMedian, current, spreads[i]);
        if (verdict == Verdict::newRow)
        {
            std::cout << String::formatted ("  new        %s: %.3f ms, not in the baseline", rows[i].name, current) << std::endl;
            continue;
        }

        const auto before = baselineMedian.value_or (0.0);
        std::cout << String::formatted ("  %-10s %s: %.3f ms, baseline %.3f ms, %+.3f ms (%+.1f%%), A/A spread %.3f ms",
                                        verdict == Verdict::pass ? "PASS" : (verdict == Verdict::regressed ? "REGRESSED" : "INVALID"),
                                        rows[i].name, current, before, current - before, before > 0.0 ? (current / before - 1.0) * 100.0 : 0.0, spreads[i])
                  << std::endl;
    }
}
#endif
