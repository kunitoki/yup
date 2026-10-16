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

#if YUP_LINUX
class SceneRendererGLTests : public SceneRendererBaselineScenes
{
protected:
    void SetUp() override
    {
        setenv ("LIBGL_ALWAYS_SOFTWARE", "1", 0);
        setenv ("GALLIUM_DRIVER", "llvmpipe", 0);

        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengl");
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow ("yup_3d_gl_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (window == nullptr)
            GTEST_SKIP() << "Cannot create an SDL window: " << SDL_GetError();

        glContext = SDL_GL_CreateContext (window);
        if (glContext == nullptr)
            GTEST_SKIP() << "Cannot create an OpenGL context: " << SDL_GetError();

        SDL_GL_MakeCurrent (window, glContext);

        GpuDevice::Options options;
        options.loaderFunction = (GpuDevice::LoaderFunction) SDL_GL_GetProcAddress;
        options.readableFramebuffer = true;

        device = GpuDevice::create (GpuPlatform::OpenGL, options);
        if (device == nullptr)
            GTEST_SKIP() << "No OpenGL device available";

        context = GraphicsContext::createContext (GpuPlatform::OpenGL, options, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No OpenGL graphics context available";
    }

    void TearDown() override
    {
        context = nullptr;
        device = nullptr;

        if (glContext != nullptr)
            SDL_GL_DestroyContext (glContext);

        if (window != nullptr)
            SDL_DestroyWindow (window);
    }

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
};

TEST_F (SceneRendererGLTests, CapturesBaselineScenes)
{
    captureBaselineScenes ("gl");
}

TEST_F (SceneRendererGLTests, MatchesPhaseZeroBaselineOnRaster)
{
    for (const auto* name : { "shadows", "materials", "edges" })
        if (! loadBaseline (String ("gl_") + name + ".png").isValid())
            GTEST_SKIP() << "No gl_" << name << ".png baseline captured yet";

    checkMatchesPhaseZeroBaseline ("gl");
}

TEST_F (SceneRendererGLTests, DefaultSettingsAddNoOptInPass)
{
    checkDefaultSettingsAddNoOptInPass (RenderSettings::TierPreference::raster);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererGLTests, RasterShadesMoreThanEightLightsPerScene)
{
    checkShadesMoreThanEightLightsPerScene (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, TinyRadiusMatchesPunctual)
{
    checkTinyRadiusMatchesPunctual (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SceneColorHasNoNaN)
{
    checkSceneColorHasNoNaN (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AreaLightRadiusWidensTheHighlightAndStaysFinite)
{
    checkRadiusWidensTheHighlight (RenderSettings::TierPreference::raster);
    checkAreaLightsStayFinite (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, LayeredAreaLightsStayFinite)
{
    checkAreaLightsStayFinite (RenderSettings::TierPreference::raster, true);
}

TEST_F (SceneRendererGLTests, LayersUnderATinyAreaLightMatchPunctual)
{
    checkLayersUnderATinyAreaLightMatchPunctual (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ShadowedRegionIsFinite)
{
    checkShadowedRegionIsFinite (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RendersWithoutTheShadowCompareSampler)
{
    checkRendersWithoutTheShadowCompareSampler (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PrepassMatchesTheGBufferEncoding)
{
    checkPrepassMatchesTheGBufferEncoding (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AmbientOcclusionDarkensTheContactCrease)
{
    checkAmbientOcclusionDarkensTheContactCrease (RenderSettings::TierPreference::raster);
}
#endif

TEST_F (SceneRendererGLTests, PrepassIsNotCreatedByDefault)
{
    checkPrepassIsNotCreatedByDefault (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RendersWithoutTheDepthNormalPrepass)
{
    checkRendersWithoutTheDepthNormalPrepass (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AmbientOcclusionOffAllocatesNothing)
{
    checkAmbientOcclusionOffAllocatesNothing (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AmbientOcclusionUnavailableOnRasterRendersFallback)
{
    checkAmbientOcclusionUnavailableOnRasterRendersFallback (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SsrUnavailableOnRasterRendersFallback)
{
    checkScreenSpaceReflectionsUnavailableOnRasterRendersFallback (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AmbientOcclusionLeavesTheDirectLightAlone)
{
    checkAmbientOcclusionLeavesTheDirectLightAlone (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ConvergedAmbientOcclusionIsDeterministic)
{
    checkConvergedAmbientOcclusionIsDeterministic (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BlendedSurfacesSkipAmbientOcclusion)
{
    checkBlendedSurfacesSkipAmbientOcclusion (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BentNormalsTurnTheDiffuseLightToTheOpenSide)
{
    checkBentNormalsTurnTheDiffuseLightToTheOpenSide (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterAmbientOcclusionOffAgainRestoresTheFrame)
{
    checkRasterAmbientOcclusionOffAgainRestoresTheFrame();
}

TEST_F (SceneRendererGLTests, RendersWithoutTheShadowPipelines)
{
    checkRendersWithoutTheShadowPipelines (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed)
{
    checkShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ShadowedLightOutOfReachAllocatesNoAtlas)
{
    checkShadowedLightOutOfReachAllocatesNoAtlas (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedShadowMapKeepsTheOtherOneCached)
{
    checkFailedShadowMapKeepsTheOtherOneCached (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AtlasIsReleasedWhenNoLightNeedsIt)
{
    checkAtlasIsReleasedWhenNoLightNeedsIt (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AtlasRedrawsOnlyTheViewsACasterReaches)
{
    checkAtlasRedrawsOnlyTheViewsACasterReaches (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PartiallyRedrawnAtlasMatchesAFreshOne)
{
    checkPartiallyRedrawnAtlasMatchesAFreshOne (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SingleSpotTakesHalfTheAtlas)
{
    checkSingleSpotTakesHalfTheAtlas (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, LocalViewsShrinkToFitTheAtlas)
{
    checkLocalViewsShrinkToFitTheAtlas (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, LocalLightsKeepTheirQuarterTiles)
{
    checkLocalLightsKeepTheirQuarterTiles (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AtlasRelayoutRedrawsTheMovedTiles)
{
    checkAtlasRelayoutRedrawsTheMovedTiles (RenderSettings::TierPreference::raster);
}

// With GL storing another depth than the shaders compute (clip z remapped differently on the two
// sides) the near shadow disappears or the whole ground darkens: these two catch either way
TEST_F (SceneRendererGLTests, SunShadowReachesNearAndFarCasters)
{
    checkSunShadowReachesNearAndFarCasters (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, CascadeTransitionHasNoSeam)
{
    checkCascadeTransitionHasNoSeam (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, AreaSunCastsAShadow)
{
    checkAreaSunCastsAShadow (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, MaskedCasterShadowsOnlyWhatItKeeps)
{
    checkMaskedCasterShadowsOnlyWhatItKeeps (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PointLightShadowsOnAllSixFaces)
{
    checkPointLightShadowsOnAllSixFaces (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PointShadowHasNoFaceSeams)
{
    checkPointShadowHasNoFaceSeams (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SpotLightCastsAShadow)
{
    checkSpotCastsAShadow (RenderSettings::TierPreference::raster, 0.0f);
}

TEST_F (SceneRendererGLTests, AreaSpotCastsAShadowTowardsItsCenter)
{
    checkSpotCastsAShadow (RenderSettings::TierPreference::raster, 0.1f);
    checkAreaSpotCastsAShadowTowardsItsCenter (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ManyShadowedLightsRenderWithoutError)
{
    checkManyShadowedLightsRenderWithoutError (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ContactShadowsIgnoredOnRaster)
{
    checkContactShadowsIgnoredOnRaster (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ContactShadowSettingsLiveInTheFirstShadowRecord)
{
    checkContactShadowSettingsLiveInTheFirstShadowRecord (RenderSettings::TierPreference::raster);
}

// The blocker search reads the raw depth through its own texture unit, next to the compared one
TEST_F (SceneRendererGLTests, PcssPenumbraWidensWithLightRadius)
{
    checkPcssPenumbraWidensWithLightRadius (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, PenumbraWidensWithDistanceFromTheBlocker)
{
    checkPenumbraWidensWithDistanceFromTheBlocker (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterSoftShadowsOptIn)
{
    checkRasterSoftShadowsOptIn();
}

TEST_F (SceneRendererGLTests, RasterSoftShadowVariantFollowsTheSettings)
{
    checkRasterSoftShadowVariantFollowsTheSettings();
}

TEST_F (SceneRendererGLTests, SpotPenumbraWidensWithLightRadius)
{
    checkSpotPenumbraWidensWithLightRadius (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SoftShadowsFilterAtLeastTwoTexels)
{
    checkSoftShadowsFilterAtLeastTwoTexels (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SoftPointShadowHasNoFaceSeams)
{
    checkPointShadowHasNoFaceSeams (RenderSettings::TierPreference::raster, 0.1f);
}

TEST_F (SceneRendererGLTests, SoftSunLeavesAGrazedGroundLit)
{
    checkSoftSunLeavesAGrazedGroundLit (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ShadowFilterVariantFollowsSoftAndContactShadows)
{
    checkShadowFilterVariantFollowsSoftAndContactShadows (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ReflectionProbeShowsWhatTheCameraCannotSee)
{
    checkReflectionProbeShowsWhatTheCameraCannotSee (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ReplacedProbeIsBakedAgain)
{
    checkReplacedProbeIsBakedAgain (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, MovingAProbeBakesItAgain)
{
    checkMovingAProbeBakesItAgain (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ProbeBlendWithoutEnvironmentIsFinite)
{
    checkProbeBlendWithoutEnvironmentIsFinite (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BoxProbeWithParallaxReflects)
{
    checkBoxProbeWithParallaxReflects (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BakeProbesReportsPendingCount)
{
    checkBakeProbesReportsPendingCount (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RemovedProbesReleaseTheAtlas)
{
    checkRemovedProbesReleaseTheAtlas (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ProbeOfAnOpenSkyMatchesTheEnvironment)
{
    checkProbeOfAnOpenSkyMatchesTheEnvironment (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RgbmProbeShowsLikeTheFloatOne)
{
    checkRgbmProbeShowsLikeTheFloatOne (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ProbeKeepsTheMultipleScatteringOfRoughMetals)
{
    checkProbeKeepsTheMultipleScatteringOfRoughMetals (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BoxProbeParallaxPlacesTheReflectionOnTheWalls)
{
    checkBoxProbeParallaxPlacesTheReflectionOnTheWalls (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedProbeCapturesLeaveTheFrameWithoutProbes)
{
    checkFailedProbeCapturesLeaveTheFrameWithoutProbes (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, IrradianceVolumeTintsTheFloorNearTheRedWall)
{
    checkIrradianceVolumeTintsTheFloorNearTheRedWall (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, VisibilityStopsLightLeakingThroughTheWall)
{
    checkVisibilityStopsLightLeakingThroughTheWall (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, MovingAVolumeBakesItAgain)
{
    checkMovingAVolumeBakesItAgain (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, BakeProbesBakesTheVolumes)
{
    checkBakeProbesBakesTheVolumes (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RemovedVolumesReleaseTheAtlases)
{
    checkRemovedVolumesReleaseTheAtlases (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RgbmVolumeLightsLikeTheFloatOne)
{
    checkRgbmVolumeLightsLikeTheFloatOne (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedVolumeCapturesLeaveTheFrameWithoutVolumes)
{
    checkFailedVolumeCapturesLeaveTheFrameWithoutVolumes (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ProgressiveVolumeBakeShadesOnceComplete)
{
    checkProgressiveVolumeBakeShadesOnceComplete (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedVolumeCapturesLeaveTheProbesReflecting)
{
    checkFailedVolumeCapturesLeaveTheProbesReflecting (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ResizingAVolumeBakesItAgain)
{
    checkResizingAVolumeBakesItAgain (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, VolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity)
{
    checkVolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ChangedVolumeBakesAgainWhole)
{
    checkChangedVolumeBakesAgainWhole (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedProbeCapturesLeaveTheVolumesLighting)
{
    checkFailedProbeCapturesLeaveTheVolumesLighting (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ClearcoatAddsASharpHighlightOnARoughBase)
{
    checkClearcoatAddsASharpHighlightOnARoughBase (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ClearcoatIsWidenedOverCurvedPixels)
{
    checkClearcoatIsWidenedOverCurvedPixels (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SheenBrightensGrazingEdges)
{
    checkSheenBrightensGrazingEdges (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SheenPassesTheWhiteFurnaceTest)
{
    checkSheenPassesTheWhiteFurnaceTest (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, LayeredVariantFollowsTheMaterials)
{
    checkLayeredVariantFollowsTheMaterials (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, FailedLayersVariantShadesWithoutTheLayers)
{
    checkFailedLayersVariantShadesWithoutTheLayers (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, SheenOverAWhiteBaseKeepsTheEnergy)
{
    checkSheenOverAWhiteBaseKeepsTheEnergy (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ClearcoatDimsTheEmission)
{
    checkClearcoatDimsTheEmission (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, ClearcoatReflectsTheProbe)
{
    checkClearcoatReflectsTheProbe (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldBlursOutOfFocusBarsOnly)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::depthOfField, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkDepthOfFieldBlursOutOfFocusBarsOnly (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterNearFieldBleedsOverTheFocusedBackground)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::depthOfField, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkNearFieldBleedsOverTheFocusedBackground (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterInFocusEdgesMatchWithoutDepthOfField)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::depthOfField, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkInFocusEdgesMatchWithoutDepthOfField (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldOffChangesNothing)
{
    checkDepthOfFieldOffChangesNothing (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldKeepsTheSilhouetteBlendOverTheBackground)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::depthOfField, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkDepthOfFieldKeepsTheSilhouetteBlendOverTheBackground (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterBlurredHighlightsKeepTheirBrightnessOverTheBackground)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::depthOfField, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkBlurredHighlightsKeepTheirBrightnessOverTheBackground (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::depthOfField, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkRasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion();
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldUnavailableRecordsNothing)
{
    checkRasterDepthOfFieldUnavailableRecordsNothing();
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldSkipsFramesWithoutThePrepassDepth)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::depthOfField, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkRasterDepthOfFieldSkipsFramesWithoutThePrepassDepth();
}

TEST_F (SceneRendererGLTests, RasterDepthOfFieldThatGaveUpStopsItsRoute)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::depthOfField, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster depth of field is unavailable on this device";

    checkRasterDepthOfFieldThatGaveUpStopsItsRoute();
}

TEST_F (SceneRendererGLTests, RasterMotionBlurRendersTheUnblurredFallback)
{
    checkMotionBlurOnRasterRendersTheUnblurredFallback();
}

TEST_F (SceneRendererGLTests, RasterFogAttenuatesWithDistance)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkFogAttenuatesWithDistance (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterFogTransmittanceMatchesTheCpu)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkFogTransmittanceMatchesTheCpu (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterInfiniteFarPlaneFogStaysFinite)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkInfiniteFarPlaneFogStaysFinite (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterFogIsLitByTheAmbientLightAndTheEnvironment)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkFogIsLitByTheAmbientLightAndTheEnvironment (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterFogScattersTheSunPhysically)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkFogScattersTheSunPhysically (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterFogOverTheBackgroundKeepsItsBlendWithDepthOfField)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkFogOverTheBackgroundKeepsItsBlendWithDepthOfField (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterFogBeforeBloomGlowsLikeADimmerEmitter)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkFogBeforeBloomGlowsLikeADimmerEmitter (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterBackgroundColorLerpsTowardsTheFog)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkBackgroundColorLerpsTowardsTheFog (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterFogScattersForwardTowardsTheSun)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkFogScattersForwardTowardsTheSun (RenderSettings::TierPreference::raster);
}

TEST_F (SceneRendererGLTests, RasterWorldFogUnavailableRecordsNothing)
{
    checkRasterWorldFogUnavailableRecordsNothing();
}

TEST_F (SceneRendererGLTests, RasterWorldFogDrawsThePrepassWithoutAmbientOcclusion)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkRasterWorldFogDrawsThePrepassWithoutAmbientOcclusion();
}

TEST_F (SceneRendererGLTests, RasterWorldFogRunsBeforeTheDepthOfField)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkRasterWorldFogRunsBeforeTheDepthOfField();
}

TEST_F (SceneRendererGLTests, RasterWorldFogSkipsFramesWithoutThePrepassDepth)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkRasterWorldFogSkipsFramesWithoutThePrepassDepth();
}

TEST_F (SceneRendererGLTests, RasterWorldFogThatGaveUpStopsItsRoute)
{
    if (! isFeatureAvailable (RenderCapabilities::Feature::worldFog, RenderSettings::TierPreference::raster))
        GTEST_SKIP() << "Raster world fog is unavailable on this device";

    checkRasterWorldFogThatGaveUpStopsItsRoute();
}
#endif
