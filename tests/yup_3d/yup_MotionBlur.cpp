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

#if YUP_APPLE
//==============================================================================
/** MotionBlur through the full tier, without temporal anti-aliasing unless a test asks for it,
    in the scenes of buildMovingBarScene(): the camera at the origin looking down -Z, a bar
    motionBarDistance away moved between two frames. The raster tier's fallback is
    SceneRendererTierTests.MotionBlurOnRasterRendersTheUnblurredFallback.
*/
class MotionBlurMetalTests : public SceneRendererMetalTests
{
protected:
    static constexpr int size = motionBlurSize;

    /** The full tier accumulating @p taaSamples, with motion blur on, the shutter open @p shutter of the frame. */
    static void setUpMotionBlur (Scene& scene, int taaSamples = 1, float shutter = 1.0f)
    {
        setFullTierTaa (scene, taaSamples);
        setMotionBlur (scene, true, shutter);
    }

    /** The column analogue of edgeWidth(): the pixels between 10% and 90% of the luminance range
        along the column segment [yFrom, yTo).
    */
    static int verticalEdgeWidth (const Image& image, int column, int yFrom, int yTo)
    {
        std::vector<double> luma;
        for (int y = yFrom; y < yTo; ++y)
            luma.push_back (meanLuminance (image, { column, y, 1, 1 }));

        const auto [low, high] = std::minmax_element (luma.begin(), luma.end());
        const auto range = *high - *low;

        return static_cast<int> (std::count_if (luma.begin(), luma.end(), [&] (double l)
        {
            return l > *low + 0.1 * range && l < *low + 0.9 * range;
        }));
    }

    /** The horizontal pixel span [left, right) of the bar of buildMovingBarScene() at @p x units. */
    static Range<float> barSpan (const Scene& scene, float x)
    {
        const auto pixelsPerUnit = motionPixelsPerUnit (scene);
        return { size * 0.5f + (x - 0.1f) * pixelsPerUnit, size * 0.5f + (x + 0.1f) * pixelsPerUnit };
    }

    /** Renders the bar at @p from then at @p to with @p renderer, and returns the second frame. */
    Image renderMove (SceneRenderer& renderer, Scene& scene, EntityNode& bar, float from, float to)
    {
        moveBar (bar, from);
        if (! renderToImage (renderer, scene, size).isValid())
            return {};

        moveBar (bar, to);
        return renderToImage (renderer, scene, size);
    }

    /** The summed luma of the smear outside a bar spanning @p span: the 8 px wide strips left and
        right of it, over the rows the bar covers.
    */
    static double smearLuminance (const Image& image, Range<float> span)
    {
        const auto left = static_cast<int> (std::floor (span.getStart())) - 8;
        const auto right = static_cast<int> (std::ceil (span.getEnd()));
        return (meanLuminance (image, { left, 30, 8, 68 }) + meanLuminance (image, { right, 30, 8, 68 })) * 8.0 * 68.0;
    }
};

TEST_F (MotionBlurMetalTests, MotionBlurSmearsAlongTheMotionOnly)
{
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    // 0.1 units at the bar's distance move it 7.6 px: with the shutter open all frame, the smear
    // spans that much, centered on where the bar is now, so its vertical edges ramp and reach about
    // 3.8 px out of it, and its horizontal ones stay steps
    const auto motion = 0.1f * motionPixelsPerUnit (scene);
    ASSERT_GT (motion, 6.0f);
    ASSERT_LT (motion, 2.0f * MotionBlur::tileSize);

    SceneRenderer renderer;
    const auto blurred = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    // The bar spans x 64.0 to 79.1 and y 26.2 to 101.8: the ramps of both vertical edges along the
    // middle row, the plan's measure, the smear past each of them, and the top edge
    const auto rampWidth = edgeWidth (blurred, size / 2, 50, 94);
    const auto leftReach = meanLuminance (blurred, { 61, 40, 2, 48 });
    const auto rightReach = meanLuminance (blurred, { 80, 40, 2, 48 });
    const auto topWidth = verticalEdgeWidth (blurred, 72, 10, 50);
    RecordProperty ("rampWidth", rampWidth);
    RecordProperty ("leftReach", String (leftReach, 1).toStdString());
    RecordProperty ("rightReach", String (rightReach, 1).toStdString());
    RecordProperty ("topEdgeWidth", topWidth);

    EXPECT_GE (rampWidth, 4);
    EXPECT_GT (leftReach, 50.0);
    EXPECT_GT (rightReach, 50.0);
    EXPECT_LE (topWidth, 2);

    // A smear, not a dilation: the outermost lit pixels are partly covered, dimmer than the interior
    const auto interior = meanLuminance (blurred, { 72, 40, 1, 48 });
    EXPECT_LT (meanLuminance (blurred, { 61, 40, 1, 48 }), interior - 30.0);
    EXPECT_LT (meanLuminance (blurred, { 81, 40, 1, 48 }), interior - 30.0);

    // The scene color before the blur stays sharp: the smear runs after it
    EXPECT_LE (edgeWidth (readBack (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)), size / 2, 50, 94), 2);

    // Not the move alone: the same move without motion blur keeps steps
    setMotionBlur (scene, false);
    const auto sharp = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
    ASSERT_TRUE (sharp.isValid()) << renderer.getLastError();
    EXPECT_LE (edgeWidth (sharp, size / 2, 50, 94), 2);
    EXPECT_LT (meanLuminance (sharp, { 61, 40, 2, 48 }), 1.0);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
}

TEST_F (MotionBlurMetalTests, FirstFrameIsNotBlurred)
{
    // A transform set before the first frame is no motion: the frame records no smear and equals
    // the frame without motion blur
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);
    moveBar (*bar, 0.1f);

    SceneRenderer renderer;
    const auto first = renderToImage (renderer, scene, size);
    ASSERT_TRUE (first.isValid()) << renderer.getLastError();
    EXPECT_EQ (renderer.getMotionBlur(), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    setMotionBlur (scene, false);
    const auto off = renderToImage (renderer, scene, size);
    ASSERT_TRUE (off.isValid()) << renderer.getLastError();
    EXPECT_LE (maxChannelDifference (first, off), 1);
}

TEST_F (MotionBlurMetalTests, TeleportIsClampedToMaximumRadius)
{
    // A jump of a whole unit, 76 px, far more than a smear spans: nothing lights up farther than
    // tileSize from where the bar lands, so no pixel is reached farther than 2 * tileSize, the
    // longest smear. A gather blur can only smear what is on screen, so the bar lands in view
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);
    ASSERT_GT (motionPixelsPerUnit (scene), 4.0f * MotionBlur::tileSize);

    SceneRenderer renderer;
    const auto blurred = renderMove (renderer, scene, *bar, -0.6f, 0.4f);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    const auto span = barSpan (scene, 0.4f);
    const auto reach = static_cast<float> (MotionBlur::tileSize) + 2.0f;

    int litBeyondReach = 0;
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const auto center = static_cast<float> (x) + 0.5f;
            const auto distance = jmax (span.getStart() - center, center - span.getEnd(), 0.0f);
            if ((distance > reach || y < 24 || y > 104) && meanLuminance (blurred, { x, y, 1, 1 }) > 2.0)
                ++litBeyondReach;
        }
    }

    // Within its reach the smear shows, on both sides of the landing place
    const auto leftSmear = meanLuminance (blurred, { static_cast<int> (span.getStart()) - 12, 40, 8, 48 });
    const auto rightSmear = meanLuminance (blurred, { static_cast<int> (span.getEnd()) + 5, 40, 8, 48 });
    RecordProperty ("litBeyondReach", litBeyondReach);
    RecordProperty ("leftSmear", String (leftSmear, 2).toStdString());
    RecordProperty ("rightSmear", String (rightSmear, 2).toStdString());

    EXPECT_EQ (litBeyondReach, 0);
    EXPECT_GT (leftSmear, 10.0);
    EXPECT_GT (rightSmear, 10.0);

#if YUP_ENABLE_SHADER_TRANSPILER
    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur)));
#endif
}

TEST_F (MotionBlurMetalTests, CameraMotionBlursStaticObjects)
{
    {
        // Moving the camera 0.1 to the right moves the still bar 7.6 px to the left on screen
        Scene scene;
        buildMovingBarScene (scene);
        setUpMotionBlur (scene);
        auto* camera = scene.getRoot()->findChild ("camera");
        ASSERT_NE (camera, nullptr);

        SceneRenderer renderer;
        ASSERT_TRUE (renderToImage (renderer, scene, size).isValid()) << renderer.getLastError();
        camera->setPosition ({ 0.1f, 0.0f, 0.0f });
        const auto blurred = renderToImage (renderer, scene, size);
        ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

        // The bar spans x 48.9 to 64.0
        EXPECT_GE (edgeWidth (blurred, size / 2, 36, 78), 4);
        EXPECT_GT (meanLuminance (blurred, { 46, 40, 2, 48 }), 50.0);
        EXPECT_GT (meanLuminance (blurred, { 65, 40, 2, 48 }), 50.0);
        EXPECT_LE (verticalEdgeWidth (blurred, 56, 10, 50), 2);
    }

    // Turning the camera moves the environment, whose motion only GBufferBackground writes: its
    // sharp edge, a white half to the right of the view axis, smears as the geometry does
    Scene scene;
    setUpMotionBlur (scene);
    scene.setBackgroundColor (Colors::black);
    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& direction)
    {
        return direction.getX() > 0.0f ? Vector3<float> (1.0f, 1.0f, 1.0f) : Vector3<float>();
    }, 512, 1));
    scene.setEnvironmentVisible (true);
    addCamera (scene, {}, { 0.0f, 0.0f, -1.0f });
    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);

    const auto turn = [&] (SceneRenderer& renderer)
    {
        aimCamera (*camera, {}, { 0.0f, 0.0f, -1.0f });
        if (! renderToImage (renderer, scene, size).isValid())
            return Image();

        aimCamera (*camera, {}, { std::sin (0.08f), 0.0f, -std::cos (0.08f) });
        return renderToImage (renderer, scene, size);
    };

    SceneRenderer renderer;
    const auto blurred = turn (renderer);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    setMotionBlur (scene, false);
    const auto sharp = turn (renderer);
    ASSERT_TRUE (sharp.isValid()) << renderer.getLastError();

    const auto blurredWidth = edgeWidth (blurred, size / 2, 32, 96);
    const auto sharpWidth = edgeWidth (sharp, size / 2, 32, 96);
    RecordProperty ("environmentEdgeWidth", blurredWidth);
    RecordProperty ("environmentSharpEdgeWidth", sharpWidth);
    EXPECT_GE (blurredWidth, sharpWidth + 3);
}

TEST_F (MotionBlurMetalTests, MotionBlurSmearsAfterAMoveUnderTemporalAntiAliasing)
{
    // With TAA, the frame after a move restarts the convergence and reprojects the history along
    // the motion, clipping it to the new frame: the TAA output shows the bar where it moved to,
    // and the motion blur smears that
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene, 8);

    SceneRenderer renderer;
    ASSERT_TRUE (renderConverged (renderer, scene, size).isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());
    EXPECT_EQ (renderer.getMotionBlur(), nullptr);

    moveBar (*bar, 0.1f);
    const auto blurred = renderToImage (renderer, scene, size);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
    EXPECT_FALSE (renderer.isConverged());

    const auto resolved = readBack (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor));
    const auto resolvedWidth = edgeWidth (resolved, size / 2, 50, 94);
    const auto rampWidth = edgeWidth (blurred, size / 2, 50, 94);
    const auto leftReach = meanLuminance (blurred, { 61, 40, 2, 48 });
    RecordProperty ("resolvedRampWidth", resolvedWidth);
    RecordProperty ("resolvedLeftReach", String (meanLuminance (resolved, { 61, 40, 2, 48 }), 1).toStdString());
    RecordProperty ("rampWidth", rampWidth);
    RecordProperty ("leftReach", String (leftReach, 1).toStdString());

    // TAA shows the bar where it moved to, sharp and without a trail where it was: the smear is the motion blur's
    EXPECT_LE (resolvedWidth, 2);
    EXPECT_LT (meanLuminance (resolved, { 61, 40, 2, 48 }), 1.0);
    EXPECT_GE (rampWidth, resolvedWidth + 2);
    EXPECT_GT (leftReach, 50.0);
}

TEST_F (MotionBlurMetalTests, MotionBlurSmearsTheWholeFrameWhenUpscaling)
{
    // Upscaling shades the velocity at half the size of the color the tiles follow: a bar moving
    // in the bottom right quadrant smears there as without upscaling
    const auto smear = [this] (bool upscaled, bool motionBlur)
    {
        Scene scene;
        auto* bar = buildMovingBarScene (scene);
        setUpMotionBlur (scene, 4);
        setMotionBlur (scene, motionBlur, 1.0f);

        auto settings = scene.getRenderSettings();
        settings.upscaling = upscaled ? RenderSettings::Upscaling::temporal : RenderSettings::Upscaling::off;
        settings.renderScale = 0.5f;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        const auto image = renderMove (renderer, scene, *bar, 0.4f, 0.5f);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur) != nullptr, motionBlur);

        // The 8 px wide strips left and right of the bar, below the middle of the frame
        const auto span = barSpan (scene, 0.5f);
        const auto left = static_cast<int> (std::floor (span.getStart())) - 8;
        const auto right = static_cast<int> (std::ceil (span.getEnd()));
        EXPECT_GT (left, size / 2);
        return image.isValid() ? meanLuminance (image, { left, 70, 8, 28 }) + meanLuminance (image, { right, 70, 8, 28 }) : 0.0;
    };

    const auto native = smear (false, true);
    const auto upscaled = smear (true, true);
    const auto unblurred = smear (true, false);
    RecordProperty ("nativeSmear", String (native, 1).toStdString());
    RecordProperty ("upscaledSmear", String (upscaled, 1).toStdString());
    RecordProperty ("unblurredUpscaled", String (unblurred, 1).toStdString());

    // On Apple M2 Max: 95.9 without upscaling, 77.1 with it, whose single frame after the move is
    // softer, and 4.3 unblurred. Tiles misread at half size would leave the strips unsmeared
    EXPECT_GT (native, 50.0);
    EXPECT_GT (upscaled, native * 0.6);
    EXPECT_LT (unblurred, upscaled * 0.25);
}

TEST_F (MotionBlurMetalTests, MotionBlurOnAStillFrameRecordsNoPass)
{
    // With motion blur on, still frames create, draw and keep nothing: the frames converging, the
    // converged ones, and those after a move
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene, 4);

    SceneRenderer renderer;
    for (int frame = 0; frame < 8; ++frame)
    {
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getMotionBlur(), nullptr) << frame;
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << frame;

        if (renderer.isConverged())
            break;
    }

    ASSERT_TRUE (renderer.isConverged());
    ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.getMotionBlur(), nullptr);

    // Not vacuous: a move smears and keeps its targets
    moveBar (*bar, 0.1f);
    ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
    ASSERT_NE (renderer.getMotionBlur(), nullptr);
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
    EXPECT_GT (renderer.getMotionBlur()->getNumCachedTargets(), 0u);

    // The still frames after it release them
    for (int frame = 0; frame < 6; ++frame)
    {
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << frame;
        EXPECT_EQ (renderer.getMotionBlur()->getOutput(), nullptr) << frame;
        EXPECT_EQ (renderer.getMotionBlur()->getNumCachedTargets(), 0u) << frame;
    }

    EXPECT_TRUE (renderer.isConverged());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}

TEST_F (MotionBlurMetalTests, NonFiniteTransformsAreNoMotion)
{
    // An item whose transform holds a NaN, the same every frame, compares as the restarting
    // signature hashes it: no motion, so the still frames record no motion blur and converge
    Scene scene;
    buildMovingBarScene (scene);
    auto* broken = addMotionQuad (scene, { -0.8f, -0.1f, 0.2f, 0.2f }, 2.0f);
    broken->setPosition ({ std::numeric_limits<float>::quiet_NaN(), 0.0f, -motionBarDistance });
    setUpMotionBlur (scene, 4);

    SceneRenderer renderer;
    renderer.setFrameStatsEnabled (true);
    for (int frame = 0; frame < 8; ++frame)
    {
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.getMotionBlur(), nullptr) << frame;
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << frame;

        if (renderer.isConverged())
            break;
    }

    EXPECT_TRUE (renderer.isConverged());
    EXPECT_TRUE (renderer.waitForGPU().wasOk());

    // Not vacuous: the item with the NaN is drawn
    EXPECT_EQ (renderer.getLastFrameStats().drawItems, 2);
}

TEST_F (MotionBlurMetalTests, MotionBlurOffOrAClosedShutterRecordsNothingWhileMoving)
{
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    for (const auto [motionBlur, shutter] : { std::pair { false, 1.0f }, std::pair { true, 0.0f }, std::pair { true, -1.0f } })
    {
        setMotionBlur (scene, motionBlur, shutter);

        SceneRenderer renderer;
        const auto image = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getMotionBlur(), nullptr) << shutter;
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << shutter;
        EXPECT_LE (edgeWidth (image, size / 2, 50, 72), 2) << shutter;
    }
}

TEST_F (MotionBlurMetalTests, SmearedHighlightsKeepTheirBrightnessOverTheBackground)
{
    // A bright bar moving over the black background color, without an environment, against the
    // same move over a visible black environment: the smear spreads the same light over both, so
    // it is as bright, not capped by its partial coverage (preflight R22)
    Scene scene;
    auto* bar = buildMovingBarScene (scene, 20.0f);
    setUpMotionBlur (scene);

    SceneRenderer renderer;
    const auto overBackground = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
    scene.setEnvironmentVisible (true);
    const auto overEnvironment = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

    // Not vacuous: the smear reaches out of the bar, which spans x 71.6 to 86.7 at 0.2
    ASSERT_GT (meanLuminance (overEnvironment, { 64, 40, 6, 48 }), 20.0);

    const auto background = smearLuminance (overBackground, barSpan (scene, 0.2f));
    const auto environment = smearLuminance (overEnvironment, barSpan (scene, 0.2f));
    RecordProperty ("overBackground", String (background, 1).toStdString());
    RecordProperty ("overEnvironment", String (environment, 1).toStdString());

    // Over black both tone map the same smeared radiance, so only the 8 bit rounding differs
    EXPECT_NEAR (background, environment, 0.02 * environment);
}

TEST_F (MotionBlurMetalTests, SmearedHighlightsKeepTheirBrightnessWhileTheCameraTurns)
{
    // Turning the camera moves the bar and the background alike, so every tile takes the uniform
    // path: its smear too keeps its brightness over the background color (preflight R22)
    Scene scene;
    buildMovingBarScene (scene, 20.0f);
    setUpMotionBlur (scene);
    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);

    const auto turn = [&] (SceneRenderer& renderer)
    {
        aimCamera (*camera, {}, { 0.0f, 0.0f, -1.0f });
        if (! renderToImage (renderer, scene, size).isValid())
            return Image();

        aimCamera (*camera, {}, { std::sin (0.08f), 0.0f, -std::cos (0.08f) });
        return renderToImage (renderer, scene, size);
    };

    SceneRenderer renderer;
    const auto overBackground = turn (renderer);
    ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
    scene.setEnvironmentVisible (true);
    const auto overEnvironment = turn (renderer);
    ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

    // Turned 0.08 rad to the right, the bar spans x 44.2 to 59.5, and moved about 15 px
    const Range<float> span (44.2f, 59.5f);
    ASSERT_GT (meanLuminance (overEnvironment, { 38, 40, 5, 48 }), 20.0);

    const auto background = smearLuminance (overBackground, span);
    const auto environment = smearLuminance (overEnvironment, span);
    RecordProperty ("overBackground", String (background, 1).toStdString());
    RecordProperty ("overEnvironment", String (environment, 1).toStdString());
    EXPECT_NEAR (background, environment, 0.02 * environment);
}

TEST_F (MotionBlurMetalTests, FogOverTheBackgroundKeepsItsBlendWhileTheCameraTurns)
{
    // Turning the camera smears the fog's coverage of the background color into itself: none of it
    // moves, so the blur amount stays near 0 there and tone mapping keeps its display-space blend
    // (preflight R24). On the rows above and below the bar, the linear output of the motion blur
    // stays near its input, and its blur amount is at most 0.02
    Scene scene;
    buildMovingBarScene (scene);
    setUpMotionBlur (scene);
    scene.setAmbientColor (fogAmbientColor());
    scene.setWorldFog (makeFog (0.08f));
    scene.getActiveCamera()->zFar = 10.0f;

    auto* camera = scene.getRoot()->findChild ("camera");
    ASSERT_NE (camera, nullptr);

    SceneRenderer renderer;
    aimCamera (*camera, {}, { 0.0f, 0.0f, -1.0f });
    ASSERT_TRUE (renderToImage (renderer, scene, size).isValid()) << renderer.getLastError();

    aimCamera (*camera, {}, { std::sin (0.08f), 0.0f, -std::cos (0.08f) });
    ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.waitForGPU().wasOk());

    const auto output = renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur);
    const auto input = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
    ASSERT_NE (output, nullptr);
    ASSERT_NE (input, nullptr);
    ASSERT_NE (renderer.getMotionBlur(), nullptr);
    ASSERT_NE (renderer.getMotionBlur()->getBlurAmount(), nullptr);

    const auto mask = readUnitChannel (renderer.getMotionBlur()->getBlurAmount(), "r");
    std::array<std::vector<float>, 3> smeared, sharp;
    for (size_t channel = 0; channel < 3; ++channel)
    {
        const char* name = std::array<const char*, 3> { "r", "g", "b" }[channel];
        smeared[channel] = readUnitChannel (output, name);
        sharp[channel] = readUnitChannel (input, name);
        ASSERT_EQ (smeared[channel].size(), static_cast<size_t> (size * size));
        ASSERT_EQ (sharp[channel].size(), smeared[channel].size());
    }

    ASSERT_EQ (mask.size(), static_cast<size_t> (size * size));

    // The bar spans rows 26 to 102. The taps of the 8 columns at each side, half the smear's reach,
    // clamp at the frame edge, so they average the fog's coverage unevenly: they are left out
    auto largestMask = 0.0f;
    auto largestDifference = 0.0f;
    auto background = 0.0;
    for (const auto band : { Rectangle<int> (8, 0, size - 16, 20), Rectangle<int> (8, 108, size - 16, 20) })
    {
        for (int y = band.getY(); y < band.getBottom(); ++y)
        {
            for (int x = band.getX(); x < band.getRight(); ++x)
            {
                const auto index = static_cast<size_t> (y * size + x);
                largestMask = jmax (largestMask, mask[index]);
                background += sharp[0][index];

                for (size_t channel = 0; channel < 3; ++channel)
                    largestDifference = jmax (largestDifference, std::abs (smeared[channel][index] - sharp[channel][index]));
            }
        }
    }

    background /= 2.0 * 20.0 * (size - 16);
    RecordProperty ("largestBlurAmount", String (largestMask, 4).toStdString());
    RecordProperty ("largestLinearDifference", String (largestDifference, 5).toStdString());
    RecordProperty ("background", String (background, 4).toStdString());

    // Not vacuous: the background shows the fog's partial coverage. Measured: a blur amount of
    // 3 / 255 and a linear difference of 5e-4, against a background of about 0.25
    EXPECT_GT (background, 0.1);
    EXPECT_LE (largestMask, 0.02f);
    EXPECT_LE (largestDifference, 2.0e-3f);
}

TEST_F (MotionBlurMetalTests, StillPartialCoverageKeepsItsBlendWhileTheBarMoves)
{
    // Half transparent quads, standing for antialiased edges, keep the frame's blend with the
    // background color while the bar moves: one far from the bar, one in its tiles just above its
    // top edge, which the horizontal motion does not reach (preflight R21, R22). The bar moves
    // 7.6 px, and 1.5 px, a half motion of 0.76 px: slow enough that the tiles around the still
    // quad hold motions of almost the same length
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    addMotionQuad (scene, { -0.8f, -0.1f, 0.2f, 0.2f }, 4.0f, 0.5f);
    addMotionQuad (scene, { -0.1f, 0.56f, 0.3f, 0.2f }, 4.0f, 0.5f);

    // The quads span x 3.4 to 18.6, y 56.4 to 71.6, and x 56.4 to 79.1, y 7.2 to 22.4
    const Rectangle<int> quads[] = { { 4, 57, 14, 14 }, { 57, 8, 22, 14 } };

    for (const auto move : { 0.1f, 0.02f })
    {
        const auto name = String (move);
        setUpMotionBlur (scene);

        SceneRenderer renderer;
        const auto on = renderMove (renderer, scene, *bar, 0.0f, move);
        ASSERT_TRUE (on.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << name;

        setMotionBlur (scene, false);
        const auto off = renderMove (renderer, scene, *bar, 0.0f, move);
        ASSERT_TRUE (off.isValid()) << renderer.getLastError();

        for (const auto& quad : quads)
        {
            ASSERT_GT (meanLuminance (off, quad), 20.0) << name << " " << quad.toString();

            int differing = 0;
            int largest = 0;
            for (int y = quad.getY(); y < quad.getBottom(); ++y)
            {
                for (int x = quad.getX(); x < quad.getRight(); ++x)
                {
                    const auto a = on.getPixelColor (x, y);
                    const auto b = off.getPixelColor (x, y);
                    const auto difference = jmax (std::abs (a.getRed() - b.getRed()), std::abs (a.getGreen() - b.getGreen()), std::abs (a.getBlue() - b.getBlue()));
                    largest = jmax (largest, difference);
                    differing += difference > 2 ? 1 : 0;
                }
            }

            RecordProperty ((name + "_" + quad.toString().replace (" ", "_") + "_largestDifference").toStdString(), largest);
            EXPECT_EQ (differing, 0) << name << " " << quad.toString();
        }
    }
}

TEST_F (MotionBlurMetalTests, NoSmearWithoutAContinuingHistory)
{
    // A resize, a restart of the accumulation (a camera cut) and a frame the GPU failed break the
    // history the motion is measured against: the next frame does not smear, though the bar moved,
    // and the one after it does again (preflight R23)
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    const auto render = [&] (SceneRenderer& renderer, float x, int width, bool smears, const char* name)
    {
        moveBar (*bar, x);
        ASSERT_NE (renderer.render (device, scene, width, size), nullptr) << name << ": " << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur) != nullptr, smears) << name;
    };

    {
        // The aspect ratio changes the projection, which alone would read as motion everywhere
        SceneRenderer renderer;
        render (renderer, 0.0f, size, false, "before the resize");
        render (renderer, 0.1f, size + 32, false, "resized");
        render (renderer, 0.2f, size + 32, true, "after the resize");
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    {
        SceneRenderer renderer;
        render (renderer, 0.0f, size, false, "before the restart");
        renderer.restartAccumulation();
        render (renderer, 0.1f, size, false, "restarted");
        render (renderer, 0.2f, size, true, "after the restart");
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    {
        SceneRenderer renderer;
        render (renderer, 0.0f, size, false, "before the failure");
        device->failNextFrameForTesting ("injected failure");
        render (renderer, 0.0f, size, false, "failing");

        // Frames complete in order, so once a later frame did the failed one has too
        {
            auto fence = GpuFrame::begin (device);
            EXPECT_TRUE (fence.waitForGPU().wasOk());
        }

        render (renderer, 0.1f, size, false, "after the failure");
        EXPECT_EQ (renderer.getLastGpuFailure(), "injected failure");
        render (renderer, 0.2f, size, true, "the next one");
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }
}

TEST_F (MotionBlurMetalTests, SmearedFrameIsNotFinal)
{
    // Without TAA, a smeared frame still reports that it is not converged, so a component repaints
    // once more and the still frame after it shows the bar sharp (preflight R23)
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    SceneRenderer renderer;
    ASSERT_TRUE (renderMove (renderer, scene, *bar, 0.0f, 0.1f).isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
    EXPECT_FALSE (renderer.isConverged());

    const auto still = renderToImage (renderer, scene, size);
    ASSERT_TRUE (still.isValid()) << renderer.getLastError();
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_LE (edgeWidth (still, size / 2, 50, 94), 2);
}

TEST_F (MotionBlurMetalTests, SmearBlendsItsCoverageOverAGrayBackground)
{
    // The smear carries coverage: outside the bar, a smeared pixel composites the gray background
    // color by the coverage it lacks. So against the same move over black it is brighter by less
    // than the gray: not by the whole gray (no coverage kept), nor by nothing (full coverage). The
    // bar is dim, so the sums stay within the 8 bit range (preflight R22)
    Scene scene;
    auto* bar = buildMovingBarScene (scene, 0.1f);
    setUpMotionBlur (scene);

    SceneRenderer renderer;
    const auto overBlack = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overBlack.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    constexpr int gray = 100;
    scene.setBackgroundColor (Color (static_cast<uint8> (gray), static_cast<uint8> (gray), static_cast<uint8> (gray)));
    const auto overGray = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overGray.isValid()) << renderer.getLastError();

    // The strips of 8 px on both sides of the bar, which spans x 71.6 to 86.7, where the smear
    // covers at least a fifth (the level over black above 40)
    int smeared = 0;
    int outside = 0;
    for (const auto x0 : { 63, 87 })
    {
        for (int y = 30; y < 98; ++y)
        {
            for (int x = x0; x < x0 + 8; ++x)
            {
                if (overBlack.getPixelColor (x, y).getGreen() <= 40)
                    continue;

                ++smeared;
                const auto added = static_cast<int> (overGray.getPixelColor (x, y).getGreen()) - static_cast<int> (overBlack.getPixelColor (x, y).getGreen());
                outside += added <= 2 || added >= gray - 5 ? 1 : 0;
            }
        }
    }

    RecordProperty ("smearedPixels", smeared);
    RecordProperty ("outsideTheBlend", outside);
    EXPECT_GT (smeared, 40);
    EXPECT_EQ (outside, 0);
}

TEST_F (MotionBlurMetalTests, DiagonalMotionSmearsAlongItsDirectionAcrossTiles)
{
    // A square moving 7.6 px right and 7.6 px up: its smear reaches past its top right and bottom
    // left corners, not past the other two. Its top right corner lands at (78, 50), next to the
    // corner of the 16 px tiles at (80, 48): the tile beyond it holds none of the square, so only
    // its diagonal neighbor carries the motion there
    Scene scene;
    scene.setBackgroundColor (Colors::black);
    scene.setAmbientColor (Colors::black);
    addCamera (scene, {}, { 0.0f, 0.0f, -1.0f });
    auto* square = addMotionQuad (scene, { -0.1f, -0.1f, 0.2f, 0.2f }, 20.0f);
    setUpMotionBlur (scene);

    const auto pixelsPerUnit = motionPixelsPerUnit (scene);
    const auto x = (70.5f - 64.0f) / pixelsPerUnit;
    const auto y = (64.0f - 57.5f) / pixelsPerUnit;

    const auto render = [&] (SceneRenderer& renderer)
    {
        square->setPosition ({ x - 0.1f, y - 0.1f, -motionBarDistance });
        if (! renderToImage (renderer, scene, size).isValid())
            return Image();

        square->setPosition ({ x, y, -motionBarDistance });
        return renderToImage (renderer, scene, size);
    };

    SceneRenderer renderer;
    const auto blurred = render (renderer);
    ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    setMotionBlur (scene, false);
    const auto sharp = render (renderer);
    ASSERT_TRUE (sharp.isValid()) << renderer.getLastError();

    const auto luma = [] (const Image& image, int px, int py) { return meanLuminance (image, { px, py, 1, 1 }); };
    RecordProperty ("topRight", String (luma (blurred, 80, 47), 1).toStdString());
    RecordProperty ("bottomLeft", String (luma (blurred, 61, 67), 1).toStdString());
    RecordProperty ("topLeft", String (luma (blurred, 61, 47), 1).toStdString());
    RecordProperty ("bottomRight", String (luma (blurred, 80, 67), 1).toStdString());

    EXPECT_GT (luma (blurred, 80, 47), 10.0);
    EXPECT_GT (luma (blurred, 61, 67), 10.0);
    EXPECT_LT (luma (blurred, 61, 47), 2.0);
    EXPECT_LT (luma (blurred, 80, 67), 2.0);
    EXPECT_LT (luma (sharp, 80, 47), 2.0);
    EXPECT_LT (luma (sharp, 61, 67), 2.0);
}

TEST_F (MotionBlurMetalTests, MotionBlurRunsAfterTheDepthOfField)
{
    // Both blur the bar, the depth of field first: the smear carries the share of coverage it
    // spread, so the blurred light stays as bright over the background color as over a visible
    // black environment, and the result is finite. A still square far from the bar, blurred by the
    // depth of field only, lies in tiles where nothing moves, which keep the share as it is
    Scene scene;
    auto* bar = buildMovingBarScene (scene, 20.0f);
    setUpMotionBlur (scene);
    addMotionQuad (scene, { -0.9f, -0.1f, 0.2f, 0.2f }, 20.0f);
    setDepthOfField (scene, true);
    auto& camera = *scene.getActiveCamera();
    camera.fStop = 1.4f;
    camera.focusDistance = 0.5f;
    ASSERT_GT (camera.getCircleOfConfusion (motionBarDistance, static_cast<float> (size)), 4.0f);

    SceneRenderer renderer;
    const auto overBackground = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);
    ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

#if YUP_ENABLE_SHADER_TRANSPILER
    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur)));
#endif

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
    scene.setEnvironmentVisible (true);
    const auto overEnvironment = renderMove (renderer, scene, *bar, 0.0f, 0.2f);
    ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

    const auto background = meanLuminance (overBackground, { 0, 0, size, size }) * size * size;
    const auto environment = meanLuminance (overEnvironment, { 0, 0, size, size }) * size * size;
    RecordProperty ("overBackground", String (background, 1).toStdString());
    RecordProperty ("overEnvironment", String (environment, 1).toStdString());
    EXPECT_NEAR (background, environment, 0.02 * environment);
}

TEST_F (MotionBlurMetalTests, GivingUpMotionBlurRendersUnsmeared)
{
    // The bar moves on every frame, so each one tries
    Scene scene;
    auto* bar = buildMovingBarScene (scene);
    setUpMotionBlur (scene);

    SceneRenderer renderer;
    ASSERT_TRUE (renderToImage (renderer, scene, size).isValid()) << renderer.getLastError();

    auto x = 0.0f;
    const auto render = [&] (bool smears, bool givenUp, const char* name)
    {
        x = x > 0.0f ? 0.0f : 0.1f;
        moveBar (*bar, x);
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << name << ": " << renderer.getLastError();
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur) != nullptr, smears) << name;
        ASSERT_NE (renderer.getMotionBlur(), nullptr) << name;
        EXPECT_EQ (renderer.getMotionBlur()->hasGivenUp(), givenUp) << name;

        // A frame without the smear keeps none of its targets, after a frame that smeared too
        if (! smears)
            EXPECT_EQ (renderer.getMotionBlur()->getNumCachedTargets(), 0u) << name;
    };

    // Only failures in a row give up: a frame that smears in between starts the count over
    renderer.failMotionBlurForTesting (1);
    render (false, false, "first failure");
    render (true, false, "recovered");
    EXPECT_GT (renderer.getMotionBlur()->getNumCachedTargets(), 0u);

    renderer.failMotionBlurForTesting (1);
    render (false, false, "failure after a smear");
    render (true, false, "recovered again");

    renderer.failMotionBlurForTesting (2);
    render (false, false, "first of two failures");
    render (false, true, "second of two failures");
    EXPECT_FALSE (renderer.getMotionBlur()->getGiveUpReason().isEmpty());

    // Given up, it no longer tries, and a move renders as without motion blur
    const auto image = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr);

    setMotionBlur (scene, false);
    const auto without = renderMove (renderer, scene, *bar, 0.0f, 0.1f);
    EXPECT_EQ (countDifferences (image, without), 0);
}

TEST_F (MotionBlurMetalTests, MissingInputIsASkipNotAFailure)
{
    // A frame without the velocity, or without any input, skips the smear, and never counts towards
    // giving up: more frames than the two failures in a row that give up
    MotionBlur motionBlur (device);
    FrameResources resources (device);
    ViewData view;
    RenderSettings settings;
    settings.motionBlur = true;

    for (int i = 0; i < 3; ++i)
    {
        resources.beginFrame (64, 64);
        const auto color = resources.acquire (GpuTextureDesc (64, 64, GpuTextureFormat::rgba16float, true));
        const auto depth = resources.acquire (GpuTextureDesc (64, 64, GpuTextureFormat::depth32float, true));
        ASSERT_NE (color, nullptr);
        ASSERT_NE (depth, nullptr);

        auto frame = GpuFrame::begin (device);
        EXPECT_EQ (motionBlur.render (frame, view, settings, resources, { color, depth, nullptr, nullptr }), nullptr);
        EXPECT_EQ (motionBlur.render (frame, view, settings, resources, {}), nullptr);
        frame.submit();
        resources.endFrame();
    }

    EXPECT_FALSE (motionBlur.hasGivenUp());
    EXPECT_EQ (motionBlur.getNumCachedTargets(), 0u);
}

TEST_F (MotionBlurMetalTests, MotionBlurChangesKeepTheAccumulationConverged)
{
    // Motion blur applies after the accumulation: turning it on or changing the shutter neither
    // restarts nor discards it
    Scene scene;
    buildMovingBarScene (scene);
    setUpMotionBlur (scene, 4);
    setMotionBlur (scene, false);

    SceneRenderer renderer;
    ASSERT_TRUE (renderConverged (renderer, scene, size).isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    for (const auto shutter : { 1.0f, 0.25f })
    {
        setMotionBlur (scene, true, shutter);
        ASSERT_NE (renderer.render (device, scene, size, size), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged()) << shutter;
    }

    EXPECT_TRUE (renderer.waitForGPU().wasOk());
}
#endif
