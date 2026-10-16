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

class WorldFogTests : public ::testing::Test
{
protected:
    /** Midpoint-rule optical depth with many steps, the reference for the closed form. */
    static float numericOpticalDepth (const WorldFog& fog, const Vector3<float>& a, const Vector3<float>& b)
    {
        constexpr int steps = 20000;
        const auto delta = (b - a) * (1.0f / steps);
        double sum = 0.0;

        for (int i = 0; i < steps; ++i)
            sum += fog.getDensity ((a + delta * (static_cast<float> (i) + 0.5f)).getY());

        return static_cast<float> (sum * delta.length());
    }

    static WorldFog makeFog (float density, float falloff)
    {
        WorldFog fog;
        fog.enabled = true;
        fog.density = density;
        fog.heightFalloff = falloff;
        fog.baseHeight = 1.0f;
        return fog;
    }
};

TEST_F (WorldFogTests, DisabledFogHasNoOpticalDepth)
{
    WorldFog fog;
    EXPECT_FALSE (fog.enabled);
    EXPECT_EQ (fog.getDensity (0.0f), 0.0f);
    EXPECT_EQ (fog.getOpticalDepth ({ 0, 0, 0 }, { 0, 0, -100 }), 0.0f);
}

TEST_F (WorldFogTests, DensityThinsAboveTheBase)
{
    const auto fog = makeFog (0.05f, 0.3f);

    EXPECT_FLOAT_EQ (fog.getDensity (1.0f), 0.05f);
    EXPECT_NEAR (fog.getDensity (11.0f), 0.05f * std::exp (-3.0f), 1.0e-8f);
    EXPECT_GT (fog.getDensity (-5.0f), 0.05f);
}

TEST_F (WorldFogTests, UniformFogIsDensityTimesLength)
{
    EXPECT_NEAR (makeFog (0.1f, 0.0f).getOpticalDepth ({ 0, 5, 0 }, { 3, 9, 0 }), 0.5f, 1.0e-5f);
}

TEST_F (WorldFogTests, ClosedFormMatchesNumericIntegration)
{
    struct Segment
    {
        float falloff;
        Vector3<float> from, to;
    };

    const Segment segments[] = {
        { 0.3f, { 0, 0, 0 }, { 0, 10, -20 } },      // rising
        { 0.3f, { 0, 8, 0 }, { 5, -2, -30 } },      // falling
        { 0.3f, { 0, 3, 0 }, { 0, 3, -50 } },       // horizontal: zero slope, the series branch avoids 0 / 0
        { 0.3f, { 0, 3, 0 }, { 0, 3.0001f, -50 } }, // nearly horizontal: the series branch
        { 0.3f, { 0, 40, 0 }, { 0, -5, -10 } },     // steep fall from far above the base
        { 2.0f, { 0, 50, 0 }, { 0, 0, -10 } },      // the density grows by e^100 along the segment
        { -2.0f, { 0, -10, 0 }, { 0, -60, -10 } }   // fog thickening upwards: the denser end is the upper one
    };

    for (const auto& segment : segments)
    {
        const auto fog = makeFog (0.05f, segment.falloff);
        const auto reference = numericOpticalDepth (fog, segment.from, segment.to);
        EXPECT_NEAR (fog.getOpticalDepth (segment.from, segment.to), reference, reference * 1.0e-3f);
    }
}

TEST_F (WorldFogTests, OpticalDepthIsSymmetric)
{
    const auto fog = makeFog (0.05f, 0.3f);
    EXPECT_NEAR (fog.getOpticalDepth ({ 1, 2, 3 }, { -4, 9, -20 }), fog.getOpticalDepth ({ -4, 9, -20 }, { 1, 2, 3 }), 1.0e-5f);
}

TEST_F (WorldFogTests, ClampedDensityBelowTheBaseIsIntegratedExactly)
{
    // With a falloff of 2 the density stops growing 20 units below the base, at y = -19
    const auto fog = makeFog (0.05f, 2.0f);

    const Vector3<float> crossing[] = { { 0, -30, 0 }, { 0, 5, -10 } };
    const auto crossingReference = numericOpticalDepth (fog, crossing[0], crossing[1]);
    EXPECT_NEAR (fog.getOpticalDepth (crossing[0], crossing[1]), crossingReference, crossingReference * 1.0e-3f);
    EXPECT_NEAR (fog.getOpticalDepth (crossing[1], crossing[0]), crossingReference, crossingReference * 1.0e-3f);

    const Vector3<float> below[] = { { 0, -40, 0 }, { 0, -25, -10 } };
    EXPECT_NEAR (fog.getOpticalDepth (below[0], below[1]), fog.getDensity (-40.0f) * (below[1] - below[0]).length(), fog.getDensity (-40.0f) * 1.0e-3f);
}

TEST_F (WorldFogTests, DeepBelowTheBaseStaysFinite)
{
    const auto fog = makeFog (0.05f, 2.0f);
    EXPECT_TRUE (std::isfinite (fog.getOpticalDepth ({ 0, -500, 0 }, { 0, -400, -10 })));
    EXPECT_TRUE (std::isfinite (fog.getOpticalDepth ({ 0, 500, 0 }, { 0, -500, -10 })));
}

TEST_F (WorldFogTests, EqualityComparesTheFields)
{
    const auto fog = makeFog (0.05f, 0.3f);
    auto other = fog;
    EXPECT_TRUE (fog == other);

    other.anisotropy = 0.5f;
    EXPECT_FALSE (fog == other);

    other = fog;
    other.color = { 0.5f, 1.0f, 1.0f };
    EXPECT_FALSE (fog == other);
}

#if YUP_APPLE
//==============================================================================
/** The analytic world fog (FogApply) on Metal: both tiers against each other, the restarting
    signature and the failures. The checks shared with the GL twins are in SceneRendererBaselineScenes.
*/
class WorldFogMetalTests : public SceneRendererMetalTests
{
protected:
    /** The mean luminance of the near wall, the far wall and the background of buildFogWallsScene()
        with fog, rendered on @p tier. */
    std::array<double, 3> renderFogWalls (RenderSettings::TierPreference tier)
    {
        Scene scene;
        setFogTier (scene, tier);
        buildFogWallsScene (scene, true);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, fogSize);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        if (! image.isValid())
            return {};

        return { meanLuminance (image, fogNearWallArea()), meanLuminance (image, fogFarWallArea()), meanLuminance (image, fogBackgroundArea()) };
    }
};

TEST_F (WorldFogMetalTests, AnalyticFogAgreesAcrossTiers)
{
    // Both tiers apply lib/fog.glsl: the full tier in place before TAA, the raster tier as its first post pass
    const auto full = renderFogWalls (RenderSettings::TierPreference::full);
    const auto raster = renderFogWalls (RenderSettings::TierPreference::raster);

    const char* names[] = { "near", "far", "background" };
    for (size_t i = 0; i < 3; ++i)
    {
        RecordProperty (std::string (names[i]) + "Full", String (full[i], 2).toStdString());
        RecordProperty (std::string (names[i]) + "Raster", String (raster[i], 2).toStdString());
        EXPECT_GT (full[i], 20.0) << names[i];
        EXPECT_NEAR (raster[i], full[i], 0.03 * full[i]) << names[i];
    }
}

TEST_F (WorldFogMetalTests, ChangingTheFogRestartsAccumulation)
{
    // The fog is applied before TAA, so every field of an enabled fog restarts the convergence
    Scene scene;
    buildFogWallsScene (scene, true);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    const auto converge = [&]
    {
        for (int frame = 0; frame < 16 && ! renderer.isConverged(); ++frame)
            ASSERT_NE (renderer.render (device, scene, fogSize, fogSize), nullptr) << renderer.getLastError();

        ASSERT_TRUE (renderer.isConverged());
    };

    const std::pair<const char*, std::function<void (WorldFog&)>> edits[] = {
        { "density", [] (WorldFog& fog) { fog.density *= 2.0f; } },
        { "heightFalloff", [] (WorldFog& fog) { fog.heightFalloff = 0.1f; } },
        { "baseHeight", [] (WorldFog& fog) { fog.baseHeight = 1.0f; } },
        { "color", [] (WorldFog& fog) { fog.color = { 1.0f, 0.5f, 0.2f }; } },
        { "anisotropy", [] (WorldFog& fog) { fog.anisotropy = 0.5f; } },
        { "enabled", [] (WorldFog& fog) { fog.enabled = false; } },
    };

    for (const auto& [name, edit] : edits)
    {
        converge();

        auto fog = scene.getWorldFog();
        edit (fog);
        scene.setWorldFog (fog);

        ASSERT_NE (renderer.render (device, scene, fogSize, fogSize), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
    }

    // A disabled fog draws nothing, so editing it changes nothing
    converge();
    auto fog = scene.getWorldFog();
    ASSERT_FALSE (fog.enabled);
    fog.density = 0.5f;
    scene.setWorldFog (fog);
    ASSERT_NE (renderer.render (device, scene, fogSize, fogSize), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (WorldFogMetalTests, GivingUpWorldFogRendersWithoutIt)
{
    Scene scene;
    setFogTier (scene, RenderSettings::TierPreference::full);
    buildFogWallsScene (scene, false);

    SceneRenderer reference;
    const auto clear = renderToImage (reference, scene, fogSize);
    ASSERT_TRUE (clear.isValid()) << reference.getLastError();
    scene.setWorldFog (makeFog (0.08f));

    const auto isFogged = [&] (const Image& image)
    {
        return meanLuminance (image, fogBackgroundArea()) > meanLuminance (clear, fogBackgroundArea()) + 20.0;
    };

    // One failure is retried on the next frame
    {
        SceneRenderer renderer;
        renderer.failWorldFogForTesting (1);

        const auto failed = renderToImage (renderer, scene, fogSize);
        ASSERT_TRUE (failed.isValid()) << renderer.getLastError();
        EXPECT_LE (maxChannelDifference (failed, clear), 1);

        const auto retried = renderToImage (renderer, scene, fogSize);
        ASSERT_TRUE (retried.isValid()) << renderer.getLastError();
        EXPECT_TRUE (isFogged (retried));
        ASSERT_NE (renderer.getFogApply(), nullptr);
        EXPECT_FALSE (renderer.getFogApply()->hasGivenUp());
    }

    // The second failure in a row gives up for good, every frame then rendering without the fog
    SceneRenderer renderer;
    renderer.failWorldFogForTesting (2);

    for (int frame = 0; frame < 3; ++frame)
    {
        const auto image = renderToImage (renderer, scene, fogSize);
        ASSERT_TRUE (image.isValid()) << frame << ": " << renderer.getLastError();
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << frame;
        EXPECT_LE (maxChannelDifference (image, clear), 1) << frame;
        ASSERT_NE (renderer.getFogApply(), nullptr);
        EXPECT_EQ (renderer.getFogApply()->hasGivenUp(), frame >= 1) << frame;
        EXPECT_EQ (renderer.getFogApply()->getNumCachedTargets(), 0u) << frame;
    }

    EXPECT_EQ (renderer.getFogApply()->getGiveUpReason(), "the renders failed for testing");
}

TEST_F (WorldFogMetalTests, NonFiniteFogDrawsNothing)
{
    // A fog with any field that is not finite draws nothing, so nothing that is not finite reaches the scene color
    Scene scene;
    setFogTier (scene, RenderSettings::TierPreference::full);
    buildFogWallsScene (scene, false);

    SceneRenderer reference;
    const auto clear = renderToImage (reference, scene, fogSize);
    ASSERT_TRUE (clear.isValid()) << reference.getLastError();

    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const std::pair<const char*, std::function<void (WorldFog&)>> edits[] = {
        { "heightFalloff", [nan] (WorldFog& fog) { fog.heightFalloff = nan; } },
        { "color", [nan] (WorldFog& fog) { fog.color[1] = nan; } },
        { "baseHeight", [nan] (WorldFog& fog) { fog.baseHeight = nan; } },
        { "anisotropy", [] (WorldFog& fog) { fog.anisotropy = std::numeric_limits<float>::infinity(); } },
    };

    for (const auto& [name, edit] : edits)
    {
        auto fog = makeFog (0.08f, 0.3f);
        edit (fog);
        scene.setWorldFog (fog);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, fogSize);
        ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();
        EXPECT_EQ (renderer.getFogApply(), nullptr) << name;
        EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor))) << name;
        EXPECT_LE (maxChannelDifference (image, clear), 1) << name;
    }
}

TEST_F (WorldFogMetalTests, RasterWorldFogUnavailableRecordsNothing)
{
    checkRasterWorldFogUnavailableRecordsNothing();
}

TEST_F (WorldFogMetalTests, RasterWorldFogDrawsThePrepassWithoutAmbientOcclusion)
{
    checkRasterWorldFogDrawsThePrepassWithoutAmbientOcclusion();
}

TEST_F (WorldFogMetalTests, RasterWorldFogRunsBeforeTheDepthOfField)
{
    checkRasterWorldFogRunsBeforeTheDepthOfField();
}

TEST_F (WorldFogMetalTests, RasterWorldFogSkipsFramesWithoutThePrepassDepth)
{
    checkRasterWorldFogSkipsFramesWithoutThePrepassDepth();
}

TEST_F (WorldFogMetalTests, RasterWorldFogThatGaveUpStopsItsRoute)
{
    checkRasterWorldFogThatGaveUpStopsItsRoute();
}
#endif
