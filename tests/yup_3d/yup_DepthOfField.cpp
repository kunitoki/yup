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

#include <yup_3d/yup_3d.h>

using namespace yup;

class DepthOfFieldTests : public ::testing::Test
{
protected:
    /** True when p lies inside the regular polygon with n sides inscribed in the unit circle, first vertex at rotation. */
    static bool insidePolygon (Point<float> p, int n, float rotation)
    {
        const auto step = MathConstants<float>::twoPi / static_cast<float> (n);
        const auto apothem = std::cos (step * 0.5f);

        for (int i = 0; i < n; ++i)
        {
            const auto normalAngle = rotation + step * (static_cast<float> (i) + 0.5f);
            const auto distance = p.getX() * std::cos (normalAngle) + p.getY() * std::sin (normalAngle);
            if (distance > apothem + 1.0e-4f)
                return false;
        }

        return true;
    }
};

TEST_F (DepthOfFieldTests, BokehSamplesHaveTheRingCount)
{
    EXPECT_EQ (DepthOfField::makeBokehSamples (3, 0, 0.0f).size(), size_t (1 + 8 + 16 + 24));
    EXPECT_EQ (DepthOfField::makeBokehSamples (DepthOfField::bokehRings, 0, 0.0f).size(), size_t (DepthOfField::numBokehSamples));
    EXPECT_EQ (DepthOfField::numBokehSamples, 81);
}

TEST_F (DepthOfFieldTests, RoundBokehFillsTheUnitDisk)
{
    const auto samples = DepthOfField::makeBokehSamples (4, 0, 0.0f);
    float maxRadius = 0.0f;
    Point<float> mean;

    for (const auto& p : samples)
    {
        maxRadius = jmax (maxRadius, p.magnitude());
        mean += p;
    }

    EXPECT_NEAR (maxRadius, 1.0f, 1.0e-4f);
    EXPECT_NEAR (mean.getX() / static_cast<float> (samples.size()), 0.0f, 1.0e-4f);
    EXPECT_NEAR (mean.getY() / static_cast<float> (samples.size()), 0.0f, 1.0e-4f);
    EXPECT_EQ (samples.front(), Point<float>());
}

TEST_F (DepthOfFieldTests, PolygonBokehStaysInsideThePolygonAndReachesItsVertices)
{
    for (int blades : { 3, 5, 6, 8 })
    {
        const auto samples = DepthOfField::makeBokehSamples (4, blades, 0.3f);
        float maxRadius = 0.0f;

        for (const auto& p : samples)
        {
            EXPECT_TRUE (insidePolygon (p, blades, 0.3f)) << blades << " blades";
            maxRadius = jmax (maxRadius, p.magnitude());
        }

        EXPECT_GT (maxRadius, 0.95f) << blades << " blades";
    }
}

TEST_F (DepthOfFieldTests, PackedCircleOfConfusionMatchesTheCamera)
{
    CameraNode camera;
    camera.fStop = 1.4f;
    camera.focusDistance = 2.0f;

    // The setup pass stores the signed radius in its own texels: half the diameter getCircleOfConfusion() returns
    constexpr float height = 400.0f;
    const auto lens = DepthOfField::getLens (&camera, height);

    for (const auto distance : { 0.5f, 3.0f, 40.0f })
    {
        const auto expected = 0.5f * camera.getCircleOfConfusion (distance, height);
        const auto packed = lens.scale * (1.0f - lens.focus / distance);
        EXPECT_NEAR (packed, expected, 1.0e-4f * std::abs (expected)) << distance;
        EXPECT_NE (expected, 0.0f) << distance;
    }

    // The cameras that cannot blur pack no lens, which the pass skips
    EXPECT_EQ (DepthOfField::getLens (nullptr, height).scale, 0.0f);
    EXPECT_EQ (DepthOfField::getLens (&camera, 0.0f).scale, 0.0f);

    camera.fStop = 0.0f;
    EXPECT_EQ (DepthOfField::getLens (&camera, height).scale, 0.0f);

    camera.fStop = 1.4f;
    camera.projection = CameraNode::Projection::orthographic;
    EXPECT_EQ (DepthOfField::getLens (&camera, height).scale, 0.0f);

    camera.projection = CameraNode::Projection::perspective;
    camera.yFov = 0.0f;
    EXPECT_EQ (DepthOfField::getLens (&camera, height).scale, 0.0f);
}

TEST_F (DepthOfFieldTests, ShaderLinearizationGivesTheInverseViewDistance)
{
    // lib/dof.glsl turns a stored depth into 1 / distance through the projection the shaders read,
    // indexed as GLSL does (p[column][row] at column * 4 + row), for a finite and an infinite far plane
    for (const auto zFar : { 100.0f, 0.0f })
    {
        CameraNode camera;
        camera.zFar = zFar;

        const auto projection = camera.getProjectionMatrix (1.6f);
        const auto* p = projection.getData();

        for (const auto distance : { 0.5f, 3.0f, 40.0f })
        {
            const auto depth = (p[10] * -distance + p[14]) / (p[11] * -distance + p[15]);
            const auto inverseDistance = (depth * p[11] - p[10]) / (depth * p[15] - p[14]);
            EXPECT_NEAR (inverseDistance, 1.0f / distance, 1.0e-3f / distance) << zFar << " " << distance;
        }

        // The far plane: 1 / zFar, or exactly 0 at depth 1 when it is infinite
        EXPECT_NEAR ((p[11] - p[10]) / (p[15] - p[14]), zFar > 0.0f ? 1.0f / zFar : 0.0f, 1.0e-5f) << zFar;
    }
}

#if YUP_APPLE
/** DepthOfField through the full tier, without temporal anti-aliasing unless a test asks for it, in
    the depth of field scenes of SceneRendererBaselineScenes (depthOfFieldSize). The behaviors both
    tiers share are SceneRendererTierTests, these are the full tier's own, plus the raster tier's
    prepass and capability rules.
*/
class DepthOfFieldMetalTests : public SceneRendererMetalTests
{
protected:
    static constexpr int size = depthOfFieldSize;

    /** The full tier with depth of field on and @p taaSamples, see setUpDepthOfFieldScene(). */
    static CameraNode& setUpScene (Scene& scene, float focusDistance, int taaSamples = 1)
    {
        return setUpDepthOfFieldScene (scene, RenderSettings::TierPreference::full, focusDistance, taaSamples);
    }

    /** A black backdrop at 10, in focus with a focus of 10, and an emissive square at
        @p occluderDistance in front, 30 px tall, from x = 36 px to its right edge at x = 64 px. That
        edge is the border of the half resolution texels 31 and 32, so of tiles 1 and 2: the halo
        right of it lies in tiles holding no occluder texel, which only the dilation reaches.
    */
    static void buildTileEdgeScene (Scene& scene, float occluderDistance, float emission)
    {
        const auto unitsPerPixel = 2.0f * occluderDistance * std::tan (0.4f) / static_cast<float> (size);
        addEmitter (scene, { -50.0f, -50.0f, 100.0f, 100.0f }, 10.0f, 0.0f);
        addEmitter (scene, { -28.0f * unitsPerPixel, -15.0f * unitsPerPixel, 28.0f * unitsPerPixel, 30.0f * unitsPerPixel }, occluderDistance, emission);
    }

    /** How much further the light of a bokeh centered in the frame reaches towards the vertices of
        a 5-blade aperture than towards its edges: the luma-weighted mean radius of the 5 sectors of
        36 degrees around the vertex directions over that of the 5 around the edge directions, less 1.
        Sectors average over the dots of the sample pattern, so a round bokeh gives about 0.
    */
    static double vertexExcess (const Image& image)
    {
        std::array<double, 2> moment {}, weight {};

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const auto dx = x + 0.5 - size / 2;
                const auto dy = y + 0.5 - size / 2;
                const auto luma = meanLuminance (image, { x, y, 1, 1 });
                const auto sector = static_cast<int> (std::round (std::atan2 (dy, dx) / (MathConstants<double>::twoPi / 10.0)));
                const auto parity = static_cast<size_t> ((sector % 2 + 2) % 2);

                moment[parity] += luma * std::sqrt (dx * dx + dy * dy);
                weight[parity] += luma;
            }
        }

        return (moment[0] / jmax (weight[0], 1.0)) / jmax (moment[1] / jmax (weight[1], 1.0), 1.0) - 1.0;
    }
};

TEST_F (DepthOfFieldMetalTests, DepthOfFieldRecordsNothingWhenItCannotBlur)
{
    // Nothing is created, drawn or kept while it is off or the camera cannot blur
    Scene scene;
    auto& camera = setUpScene (scene, 0.2f);
    buildBarsScene (scene, 10.0f);

    const auto expectNoPass = [&] (SceneRenderer& renderer, const char* name)
    {
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << name << ": " << renderer.getLastError();
        EXPECT_EQ (renderer.getDepthOfField(), nullptr) << name;
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr) << name;
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion), nullptr) << name;
        EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;
    };

    {
        setDepthOfField (scene, false);
        SceneRenderer renderer;
        expectNoPass (renderer, "off");
        setDepthOfField (scene, true);
    }

    {
        camera.fStop = 0.0f;
        SceneRenderer renderer;
        expectNoPass (renderer, "fStop 0");
        camera.fStop = 1.4f;
    }

    {
        camera.projection = CameraNode::Projection::orthographic;
        SceneRenderer renderer;
        expectNoPass (renderer, "orthographic");
        camera.projection = CameraNode::Projection::perspective;
    }

    {
        setDepthOfField (scene, true, 0.0f);
        SceneRenderer renderer;
        expectNoPass (renderer, "no blur size");
        setDepthOfField (scene, true);
    }

    {
        // The default camera of a scene without one has no depth of field
        Scene withoutCamera;
        setFullTierTaa (withoutCamera, 1);
        setDepthOfField (withoutCamera, true);
        buildBarsScene (withoutCamera, 10.0f);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, withoutCamera, size, size), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getDepthOfField(), nullptr);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    {
        // The raster tier, where a camera that cannot blur takes neither the HDR route nor the prepass
        auto settings = scene.getRenderSettings();
        settings.tier = RenderSettings::TierPreference::raster;
        scene.setRenderSettings (settings);
        camera.fStop = 0.0f;

        SceneRenderer renderer;
        expectNoPass (renderer, "raster");
        EXPECT_TRUE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::depthOfField, scene.getRenderSettings()));
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float);

        camera.fStop = 1.4f;
        settings.tier = RenderSettings::TierPreference::full;
        scene.setRenderSettings (settings);
    }

    // Once blurring, a frame that cannot blur any more keeps nothing but the pipelines
    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
    ASSERT_NE (renderer.getDepthOfField(), nullptr);
    EXPECT_GT (renderer.getDepthOfField()->getNumCachedTargets(), 0u);
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion), nullptr);

    camera.fStop = 0.0f;
    ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getDepthOfField()->getNumCachedTargets(), 0u);
    EXPECT_EQ (renderer.getDepthOfField()->getOutput(), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (DepthOfFieldMetalTests, DepthOfFieldWithInfiniteFarPlaneStaysFinite)
{
    Scene scene;
    auto& camera = setUpScene (scene, 1.0f);
    camera.zFar = 0.0f;
    buildBarsScene (scene, 1000.0f);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, size);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_GT (meanLuminance (image, { 0, 0, size, size }), 10.0);

    const auto blurred = renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField);
    const auto coc = renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion);
    ASSERT_NE (blurred, nullptr);
    ASSERT_NE (coc, nullptr);
    EXPECT_EQ (blurred->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (coc->getFormat(), GpuTextureFormat::r16float);
    EXPECT_EQ (coc->getWidth(), size / 2);

    // Each check compiles a shader at runtime, about 0.2 s: the result alone stays well within the
    // budget, and DepthOfFieldTests.ShaderLinearizationGivesTheInverseViewDistance pins the finite CoC at depth 1
    EXPECT_TRUE (isFiniteTexture (blurred));
}
#endif

TEST_F (DepthOfFieldMetalTests, PolygonBokehDiffersFromRound)
{
    // A small bright square far out of focus: the blur takes the shape of the aperture, a disk or a
    // pentagon. The pentagon reaches further towards its vertices than towards its edges, so on a
    // circle between its apothem and its corners, every other one of 10 directions 36 degrees apart
    // is darker. One renderer changes the blades between its frames
    Scene scene;
    auto& camera = setUpScene (scene, 0.1f);
    camera.fStop = 0.7f;
    setDepthOfField (scene, true, 64.0f);
    addEmitter (scene, { -0.2f, -0.2f, 0.4f, 0.4f }, 10.0f, 200.0f);

    // A black backdrop covers the frame, so the blur keeps its focus on the aperture shape
    addEmitter (scene, { -50.0f, -50.0f, 100.0f, 100.0f }, 50.0f, 0.0f);
    ASSERT_GT (circleOfConfusion (scene, 10.0f), 64.0f);

    SceneRenderer renderer;
    const auto alternation = [&] (int blades, double* ringLuma = nullptr)
    {
        camera.apertureBlades = blades;
        const auto image = renderToImage (renderer, scene, size);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();

        // The bokeh is 64 px wide: its edges lie 26 px (apothem) to 32 px (vertices) from the center, plus 3 px of the square
        std::array<double, 2> sums {};
        for (int i = 0; i < 10; ++i)
        {
            const auto angle = MathConstants<double>::twoPi * i / 10.0;
            const auto x = static_cast<int> (std::round (64.0 + 31.0 * std::cos (angle)));
            const auto y = static_cast<int> (std::round (64.0 + 31.0 * std::sin (angle)));
            sums[static_cast<size_t> (i % 2)] += meanLuminance (image, { x - 1, y - 1, 3, 3 });
        }

        if (ringLuma != nullptr)
            *ringLuma = (sums[0] + sums[1]) / 10.0;

        return std::abs (sums[0] - sums[1]) / jmax (sums[0], sums[1], 1.0);
    };

    // Any count below 3 is round, -1 included, from the first frame on
    double negativeRing = 0.0;
    const auto negative = alternation (-1, &negativeRing);
    const auto round = alternation (0);
    const auto pentagon = alternation (5);

    RecordProperty ("round", String (round, 3).toStdString());
    RecordProperty ("pentagon", String (pentagon, 3).toStdString());

    EXPECT_LT (round, 0.1);
    EXPECT_GT (pentagon, 0.1);
    EXPECT_LT (negative, 0.1);
    EXPECT_GT (negativeRing, 50.0);
}

TEST_F (DepthOfFieldMetalTests, PolygonBokehHoldsBelowTheGatherRadius)
{
    // The square's CoC is 0.66 of its backdrop's, which sets the gather radius around it: the
    // gather must still take the pentagon of the square's own CoC, not the polygon of the gather
    // radius cut by a disk of the square's CoC, which keeps the samples near the edges of the third
    // ring and drops those near its vertices
    Scene scene;
    auto& camera = setUpScene (scene, 0.1f);
    camera.fStop = 0.7f;
    setDepthOfField (scene, true, 64.0f);

    constexpr float distance = 0.197f;
    const auto unitsPerPixel = 2.0f * distance * std::tan (0.4f) / static_cast<float> (size);
    addEmitter (scene, { -3.0f * unitsPerPixel, -3.0f * unitsPerPixel, 6.0f * unitsPerPixel, 6.0f * unitsPerPixel }, distance, 200.0f);
    addEmitter (scene, { -50.0f, -50.0f, 100.0f, 100.0f }, 50.0f, 0.0f);

    const auto ratio = circleOfConfusion (scene, distance) / jmin (64.0f, circleOfConfusion (scene, 50.0f));
    ASSERT_GT (ratio, 0.62f);
    ASSERT_LT (ratio, 0.70f);

    SceneRenderer renderer;
    camera.apertureBlades = 5;
    const auto pentagon = renderToImage (renderer, scene, size);
    ASSERT_TRUE (pentagon.isValid()) << renderer.getLastError();

    camera.apertureBlades = 0;
    const auto round = renderToImage (renderer, scene, size);
    ASSERT_TRUE (round.isValid()) << renderer.getLastError();

    const auto pentagonExcess = vertexExcess (pentagon);
    const auto roundExcess = vertexExcess (round);
    RecordProperty ("pentagonExcess", String (pentagonExcess, 3).toStdString());
    RecordProperty ("roundExcess", String (roundExcess, 3).toStdString());

    // Measured 0.095 for the pentagon, 0.052 with the polygon cut by a disk, and 0.002 for the round bokeh
    EXPECT_GT (pentagonExcess, 0.075);
    EXPECT_LT (std::abs (roundExcess), 0.03);
}

TEST_F (DepthOfFieldMetalTests, GivingUpDepthOfFieldRendersUnblurred)
{
    Scene scene;
    setUpScene (scene, 0.2f);
    buildBarsScene (scene, 10.0f);

    SceneRenderer renderer;
    const auto render = [&] (bool blurs, bool givenUp, const char* name)
    {
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << name << ": " << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField) != nullptr, blurs) << name;
        ASSERT_NE (renderer.getDepthOfField(), nullptr) << name;
        EXPECT_EQ (renderer.getDepthOfField()->hasGivenUp(), givenUp) << name;

        // A frame without the blur keeps none of its targets, after a frame that blurred too
        if (! blurs)
            EXPECT_EQ (renderer.getDepthOfField()->getNumCachedTargets(), 0u) << name;
    };

    // Only failures in a row give up: a frame that blurs in between starts the count over
    renderer.failDepthOfFieldForTesting (1);
    render (false, false, "first failure");
    render (true, false, "recovered");
    EXPECT_GT (renderer.getDepthOfField()->getNumCachedTargets(), 0u);

    renderer.failDepthOfFieldForTesting (1);
    render (false, false, "failure after a blur");
    render (true, false, "recovered again");

    renderer.failDepthOfFieldForTesting (2);
    render (false, false, "first of two failures");
    render (false, true, "second of two failures");
    EXPECT_FALSE (renderer.getDepthOfField()->getGiveUpReason().isEmpty());

    // Given up, it no longer tries, and the frames are the ones without depth of field
    const auto image = renderToImage (renderer, scene, size);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);

    setDepthOfField (scene, false);
    const auto without = renderToImage (renderer, scene, size);
    EXPECT_EQ (countDifferences (image, without), 0);
}

TEST_F (DepthOfFieldMetalTests, MissingInputIsASkipNotAFailure)
{
    // A frame without the depth (the raster prepass that could not be drawn) skips the blur, and
    // never counts towards giving up, so the blur comes back with the depth
    DepthOfField depthOfField (device);
    FrameResources resources (device);
    ViewData view;
    CameraNode camera;
    RenderSettings settings;
    settings.depthOfField = true;

    for (int i = 0; i < 3; ++i)
    {
        resources.beginFrame (64, 64);
        auto frame = GpuFrame::begin (device);
        EXPECT_EQ (depthOfField.render (frame, view, settings, resources, { nullptr, false, nullptr, &camera }), nullptr);
        frame.submit();
        resources.endFrame();
    }

    EXPECT_FALSE (depthOfField.hasGivenUp());
    EXPECT_EQ (depthOfField.getNumCachedTargets(), 0u);
}

TEST_F (DepthOfFieldMetalTests, RasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion)
{
    checkRasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion();
}

TEST_F (DepthOfFieldMetalTests, RasterDepthOfFieldUnavailableRecordsNothing)
{
    checkRasterDepthOfFieldUnavailableRecordsNothing();
}

TEST_F (DepthOfFieldMetalTests, RasterDepthOfFieldSkipsFramesWithoutThePrepassDepth)
{
    checkRasterDepthOfFieldSkipsFramesWithoutThePrepassDepth();
}

TEST_F (DepthOfFieldMetalTests, RasterDepthOfFieldThatGaveUpStopsItsRoute)
{
    checkRasterDepthOfFieldThatGaveUpStopsItsRoute();
}

TEST_F (DepthOfFieldMetalTests, NearFieldReachesAcrossTileEdges)
{
    // The occluder ends on a tile border, so its halo on the right lies in tiles without it: only
    // the dilation of the tiles gathers it there
    Scene scene;
    setUpScene (scene, 10.0f);
    buildTileEdgeScene (scene, 0.2f, 8.0f);
    ASSERT_LT (circleOfConfusion (scene, 0.2f), -14.0f);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, size);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    setDepthOfField (scene, false);
    const auto without = renderToImage (renderer, scene, size);
    ASSERT_TRUE (without.isValid()) << renderer.getLastError();

    const Rectangle<int> halo (66, 52, 4, 24);
    const auto lit = meanLuminance (image, halo);
    const auto dark = meanLuminance (without, halo);
    RecordProperty ("halo", String (lit, 2).toStdString());
    RecordProperty ("haloWithout", String (dark, 2).toStdString());

    // Measured 222 against 0: without the dilation the tiles right of the edge hold no near CoC
    EXPECT_GT (lit, dark + 20.0);
}

TEST_F (DepthOfFieldMetalTests, BlurWhenUpscalingMatchesTheFullSizeOne)
{
    // Upscaling shades the depth at half the size of the color, which the blur follows: the
    // radiance across the right edge of an out-of-focus occluder ramps as without upscaling
    const auto profile = [this] (bool upscaled)
    {
        Scene scene;
        setUpScene (scene, 10.0f, 4);
        buildTileEdgeScene (scene, 0.2f, 1.0f);

        auto settings = scene.getRenderSettings();
        settings.upscaling = upscaled ? RenderSettings::Upscaling::temporal : RenderSettings::Upscaling::off;
        settings.renderScale = 0.5f;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        const auto image = renderConverged (renderer, scene, size);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged());
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);

        std::vector<double> radiance;
        for (int x = 46; image.isValid() && x < 82; ++x)
            radiance.push_back (reinhardRadiance (image.getPixelColor (x, size / 2).getGreen()));

        return radiance;
    };

    const auto native = profile (false);
    const auto upscaled = profile (true);
    ASSERT_EQ (native.size(), upscaled.size());
    ASSERT_FALSE (native.empty());

    const auto contrast = *std::max_element (native.begin(), native.end()) - *std::min_element (native.begin(), native.end());
    ASSERT_GT (contrast, 0.3);

    double largestDifference = 0.0;
    for (size_t i = 0; i < native.size(); ++i)
        largestDifference = jmax (largestDifference, std::abs (upscaled[i] - native[i]));

    RecordProperty ("largestDifference", String (largestDifference / contrast, 3).toStdString());

    // On Apple M2 Max: 0.024 of the contrast at most, and 0.088 with the blur's intermediates
    // following the smaller depth instead of the color
    EXPECT_LT (largestDifference / contrast, 0.05);
}

TEST_F (DepthOfFieldMetalTests, NearFieldIsContinuousAcrossTheSilhouette)
{
    // Across the right edge of an out-of-focus occluder over a black backdrop, the radiance falls
    // from the occluder's to 0 over the blur, without a step at the silhouette
    Scene scene;
    setUpScene (scene, 10.0f);
    buildTileEdgeScene (scene, 0.2f, 1.0f);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, size);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    std::vector<double> radiance;
    for (int x = 50; x < 78; ++x)
        radiance.push_back (reinhardRadiance (image.getPixelColor (x, size / 2).getGreen()));

    const auto contrast = *std::max_element (radiance.begin(), radiance.end()) - *std::min_element (radiance.begin(), radiance.end());
    ASSERT_GT (contrast, 0.3);

    double largestStep = 0.0;
    for (size_t i = 1; i < radiance.size(); ++i)
        largestStep = jmax (largestStep, std::abs (radiance[i] - radiance[i - 1]));

    RecordProperty ("largestStep", String (largestStep / contrast, 3).toStdString());

    // A disk of 7.5 px spreads the contrast over 15 px: about 0.07 a pixel, and measured 0.14 at
    // the steepest. A step at the silhouette measured 0.42
    EXPECT_LT (largestStep / contrast, 0.25);
}

TEST_F (DepthOfFieldMetalTests, FaintCoverageKeepsItsLightOverTheBackground)
{
    // A bright square of very low coverage (a blended material of alpha 0.004, as a light much
    // smaller than a pixel resolves after antialiasing) far out of focus: spread by the blur, its
    // coverage falls below 1e-4 in every pixel while its premultiplied light stays visible. Over the
    // black background color it keeps that light, as over a visible black environment
    Scene scene;
    auto& camera = setUpScene (scene, 0.1f);
    camera.fStop = 0.7f;
    setDepthOfField (scene, true, 64.0f);
    addEmitter (scene, { -0.2f, -0.2f, 0.4f, 0.4f }, 10.0f, 60000.0f, 0.004f);

    SceneRenderer renderer;
    const auto overBackground = renderToImage (renderer, scene, size);
    ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
    scene.setEnvironmentVisible (true);
    const auto overEnvironment = renderToImage (renderer, scene, size);
    ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

    const auto background = meanLuminance (overBackground, { 0, 0, size, size });
    const auto environment = meanLuminance (overEnvironment, { 0, 0, size, size });
    RecordProperty ("overBackground", String (background, 2).toStdString());
    RecordProperty ("overEnvironment", String (environment, 2).toStdString());
    // Measured 56.23 against 56.24; tone mapping the un-premultiplied color times the coverage gave 0
    ASSERT_GT (environment, 10.0);
    EXPECT_NEAR (background, environment, 0.05 * environment);
}

TEST_F (DepthOfFieldMetalTests, RadianceCompositeFailureFallsBackToTheDisplayBlend)
{
    // A blurred frame whose radiance composite cannot compile still tone maps, with the display
    // space blend of the frames without depth of field, instead of failing the frame
    FrameResources resources (device);
    ViewData view;
    ToneMapping toneMapping;
    toneMapping.failRadianceCompositeForTesting();

    for (int i = 0; i < 2; ++i)
    {
        resources.beginFrame (8, 8);
        const auto source = resources.acquire (GpuTextureDesc (8, 8, GpuTextureFormat::rgba16float, true));
        const auto destination = resources.acquire (GpuTextureDesc (8, 8, GpuTextureFormat::rgba8unorm, true));
        const auto blurAmount = resources.acquire (GpuTextureDesc (8, 8, GpuTextureFormat::r8unorm, true));
        ASSERT_NE (source, nullptr);
        ASSERT_NE (destination, nullptr);
        ASSERT_NE (blurAmount, nullptr);

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (toneMapping.render (device, frame, view, resources, source, false, nullptr, destination, Colors::black, blurAmount)) << i;
        EXPECT_TRUE (frame.waitForGPU().wasOk()) << i;
        resources.endFrame();
    }
}

TEST_F (DepthOfFieldMetalTests, DepthOfFieldChangesKeepTheAccumulationConverged)
{
    // The blur runs after the accumulation: turning it on and moving the focus change the image
    // but neither restart nor discard the converged history
    Scene scene;
    auto& camera = setUpScene (scene, 10.0f, 8);
    setDepthOfField (scene, false);
    buildBarsScene (scene, 10.0f);

    SceneRenderer renderer;
    const auto converged = renderConverged (renderer, scene, 64);
    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    setDepthOfField (scene, true);
    camera.focusDistance = 0.2f;
    const auto blurred = renderToImage (renderer, scene, 64);
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (countDifferences (blurred, converged), 0);

    camera.fStop = 4.0f;
    camera.apertureBlades = 6;
    const auto stoppedDown = renderToImage (renderer, scene, 64);
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (countDifferences (stoppedDown, blurred), 0);
}

TEST_F (DepthOfFieldMetalTests, DepthOfFieldRunsBeforeBloom)
{
    // A square bright enough to glow when sharp spreads below the bloom threshold once blurred:
    // with the blur before bloom, no glow reaches past the bokeh
    Scene scene;
    auto& camera = setUpScene (scene, 0.1f);
    camera.fStop = 0.7f;
    setDepthOfField (scene, true, 64.0f);
    addEmitter (scene, { -0.2f, -0.2f, 0.4f, 0.4f }, 10.0f, 30.0f);
    addEmitter (scene, { -50.0f, -50.0f, 100.0f, 100.0f }, 50.0f, 0.0f);

    const auto glowRing = [] (const Image& image)
    {
        return meanLuminance (image, { 4, 4, 120, 6 }) + meanLuminance (image, { 4, 118, 120, 6 });
    };

    SceneRenderer renderer;
    const auto withoutBloom = renderToImage (renderer, scene, size);
    ASSERT_TRUE (withoutBloom.isValid()) << renderer.getLastError();

    auto settings = scene.getRenderSettings();
    settings.bloom = true;
    settings.bloomIntensity = 1.0f;
    scene.setRenderSettings (settings);
    const auto withBloom = renderToImage (renderer, scene, size);
    ASSERT_TRUE (withBloom.isValid()) << renderer.getLastError();

    // Sharp, the square glows over the whole frame
    setDepthOfField (scene, false, 64.0f);
    const auto sharpBloom = renderToImage (renderer, scene, size);
    ASSERT_TRUE (sharpBloom.isValid()) << renderer.getLastError();

    ASSERT_GT (glowRing (sharpBloom), glowRing (withoutBloom) + 10.0);
    EXPECT_LT (glowRing (withBloom), glowRing (withoutBloom) + 2.0);
}
#endif
