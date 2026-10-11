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

#include <yup_3d/yup_3d.h>

#include <gtest/gtest.h>

using namespace yup;

class RenderCapabilitiesTests : public ::testing::Test
{
protected:
    static RenderCapabilities::DeviceFacts desktopMetal()
    {
        RenderCapabilities::DeviceFacts facts;
        facts.platform = GpuPlatform::Metal;
        facts.compute = true;
        facts.fragmentStorageBuffers = true;
        facts.storageTextures = true;
        facts.rgba16floatRenderable = true;
        facts.rgba8Samples = 4;
        facts.rgba16floatSamples = 4;
        facts.colorAttachments = 4;
        facts.depthNormalPrepass = true;
        return facts;
    }

    static RenderCapabilities::DeviceFacts webgl2 (bool halfFloat, uint32_t halfFloatSamples)
    {
        RenderCapabilities::DeviceFacts facts;
        facts.platform = GpuPlatform::OpenGLES;
        facts.rgba16floatRenderable = halfFloat;
        facts.rgba8Samples = 4;
        facts.rgba16floatSamples = halfFloatSamples;
        facts.colorAttachments = 4;
        facts.depthNormalPrepass = halfFloat;
        return facts;
    }

    using Tier = RenderCapabilities::Tier;
    using Feature = RenderCapabilities::Feature;
    using Preference = RenderSettings::TierPreference;
};

TEST_F (RenderCapabilitiesTests, DesktopMetalIsFull)
{
    EXPECT_EQ (RenderCapabilities::selectTier (desktopMetal(), Preference::automatic), Tier::full);
}

TEST_F (RenderCapabilitiesTests, GlesWithComputeIsRaster)
{
    auto facts = webgl2 (true, 4);
    facts.compute = true; // GLES 3.1
    EXPECT_EQ (RenderCapabilities::selectTier (facts, Preference::full), Tier::raster);
}

TEST_F (RenderCapabilitiesTests, DesktopGLIsRaster)
{
    auto facts = desktopMetal();
    facts.platform = GpuPlatform::OpenGL;
    EXPECT_EQ (RenderCapabilities::selectTier (facts, Preference::full), Tier::raster);
}

TEST_F (RenderCapabilitiesTests, ForcedRasterOnCapableDevice)
{
    EXPECT_EQ (RenderCapabilities::selectTier (desktopMetal(), Preference::raster), Tier::raster);
}

TEST_F (RenderCapabilitiesTests, MobileDefaultsToRasterButCanOptIn)
{
    auto facts = desktopMetal();
    facts.mobile = true;
    EXPECT_EQ (RenderCapabilities::selectTier (facts, Preference::automatic), Tier::raster);
    EXPECT_EQ (RenderCapabilities::selectTier (facts, Preference::full), Tier::full);
}

TEST_F (RenderCapabilitiesTests, BackendThatFailedTheSpikeIsRaster)
{
    auto facts = desktopMetal();
    facts.platform = GpuPlatform::Vulkan; // until Phase 0 Task 10 passes
    EXPECT_EQ (RenderCapabilities::selectTier (facts, Preference::full),
               RenderCapabilities::passedInteropSpike (GpuPlatform::Vulkan) ? Tier::full : Tier::raster);
}

TEST_F (RenderCapabilitiesTests, MissingHalfFloatTargetsIsRaster)
{
    auto facts = desktopMetal();
    facts.rgba16floatRenderable = false;
    EXPECT_EQ (RenderCapabilities::selectTier (facts, Preference::automatic), Tier::raster);
}

TEST_F (RenderCapabilitiesTests, WebGL2WithoutFloatHasNoHdrPost)
{
    RenderSettings settings;
    settings.bloom = true;

    RenderCapabilities caps (webgl2 (false, 0), settings, 4);
    EXPECT_EQ (caps.getTier(), Tier::raster);
    EXPECT_FALSE (caps.isAvailable (Feature::bloom));
    EXPECT_FALSE (caps.isEnabled (Feature::bloom, settings));
    EXPECT_FALSE (caps.isEnabled (Feature::hdrSceneColor, settings));
}

TEST_F (RenderCapabilitiesTests, HalfFloatBelowRequestedSamplesHasNoHdrPost)
{
    RenderSettings settings;
    settings.bloom = true;

    RenderCapabilities caps (webgl2 (true, 2), settings, 4);
    EXPECT_FALSE (caps.isAvailable (Feature::bloom));

    RenderCapabilities enough (webgl2 (true, 4), settings, 4);
    EXPECT_TRUE (enough.isEnabled (Feature::bloom, settings));
    EXPECT_TRUE (enough.isEnabled (Feature::hdrSceneColor, settings)); // bloom pulls in the HDR target
}

TEST_F (RenderCapabilitiesTests, RasterDefaultsKeepTodaysFrame)
{
    RenderSettings settings;
    RenderCapabilities caps (webgl2 (true, 4), settings, 4);
    EXPECT_FALSE (caps.isEnabled (Feature::hdrSceneColor, settings));
    EXPECT_FALSE (caps.isEnabled (Feature::temporalAntiAliasing, settings));
    EXPECT_FALSE (caps.isEnabled (Feature::bloom, settings));
}

TEST_F (RenderCapabilitiesTests, FullDefaultsAreHdrAndTaa)
{
    RenderSettings settings;
    RenderCapabilities caps (desktopMetal(), settings, 4);
    EXPECT_TRUE (caps.isEnabled (Feature::hdrSceneColor, settings));
    EXPECT_TRUE (caps.isEnabled (Feature::temporalAntiAliasing, settings));
    EXPECT_FALSE (caps.isEnabled (Feature::bloom, settings));
}

TEST_F (RenderCapabilitiesTests, FullTierNeedsComputeAndFragmentStorageBuffers)
{
    auto facts = desktopMetal();
    EXPECT_EQ (RenderCapabilities::selectTier (facts, Preference::automatic), Tier::full);

    facts.compute = false;
    EXPECT_EQ (RenderCapabilities::selectTier (facts, Preference::full), Tier::raster);

    facts.compute = true;
    facts.fragmentStorageBuffers = false;
    EXPECT_EQ (RenderCapabilities::selectTier (facts, Preference::full), Tier::raster);
}

TEST_F (RenderCapabilitiesTests, SoftShadowsDefaultOnFullOffOnRaster)
{
    RenderSettings settings;
    const RenderCapabilities full (desktopMetal(), settings, 4);
    const RenderCapabilities raster (webgl2 (true, 4), settings, 4);

    EXPECT_TRUE (full.isEnabled (Feature::softShadows, settings));
    EXPECT_TRUE (raster.isAvailable (Feature::softShadows));
    EXPECT_FALSE (raster.isEnabled (Feature::softShadows, settings));

    settings.softShadows = true;
    EXPECT_TRUE (raster.isEnabled (Feature::softShadows, settings));

    settings.softShadows = false;
    EXPECT_FALSE (full.isEnabled (Feature::softShadows, settings));
}

TEST_F (RenderCapabilitiesTests, ContactShadowsAreFullTierOnly)
{
    RenderSettings settings;
    settings.contactShadows = true;

    EXPECT_TRUE (RenderCapabilities (desktopMetal(), settings, 4).isEnabled (Feature::contactShadows, settings));
    EXPECT_FALSE (RenderCapabilities (webgl2 (true, 4), settings, 4).isAvailable (Feature::contactShadows));
    EXPECT_FALSE (RenderCapabilities (webgl2 (true, 4), settings, 4).isEnabled (Feature::contactShadows, settings));

    settings.contactShadows = false;
    EXPECT_FALSE (RenderCapabilities (desktopMetal(), settings, 4).isEnabled (Feature::contactShadows, settings));
}

TEST_F (RenderCapabilitiesTests, CascadeCountFollowsTierAndSettings)
{
    RenderSettings settings;
    EXPECT_EQ (RenderCapabilities (desktopMetal(), settings, 4).getCascadeCount (settings), 4);
    EXPECT_EQ (RenderCapabilities (webgl2 (true, 4), settings, 4).getCascadeCount (settings), RenderCapabilities::defaultRasterCascades);

    settings.sunCascades = 9;
    EXPECT_EQ (RenderCapabilities (desktopMetal(), settings, 4).getCascadeCount (settings), 4);
    settings.sunCascades = 1;
    EXPECT_EQ (RenderCapabilities (webgl2 (true, 4), settings, 4).getCascadeCount (settings), 1);
    settings.sunCascades = -3;
    EXPECT_EQ (RenderCapabilities (desktopMetal(), settings, 4).getCascadeCount (settings), 1);
}

TEST_F (RenderCapabilitiesTests, ShadowResolutionIsClamped)
{
    RenderSettings settings;
    EXPECT_EQ (RenderCapabilities::getShadowResolution (settings), 2048);

    settings.shadowResolution = 16;
    EXPECT_EQ (RenderCapabilities::getShadowResolution (settings), RenderCapabilities::minShadowResolution);

    settings.shadowResolution = 1 << 20;
    EXPECT_EQ (RenderCapabilities::getShadowResolution (settings), RenderCapabilities::maxShadowResolution);

    EXPECT_EQ (RenderCapabilities::minShadowResolution, 256);
    EXPECT_EQ (RenderCapabilities::maxShadowResolution, 4096);
}

TEST_F (RenderCapabilitiesTests, AmbientOcclusionDefaultsPerTier)
{
    RenderSettings settings;
    EXPECT_TRUE (RenderCapabilities (desktopMetal(), settings, 4).isEnabled (Feature::ambientOcclusion, settings));
    EXPECT_FALSE (RenderCapabilities (webgl2 (true, 4), settings, 4).isEnabled (Feature::ambientOcclusion, settings));

    settings.ambientOcclusion = true;
    EXPECT_TRUE (RenderCapabilities (webgl2 (true, 4), settings, 4).isEnabled (Feature::ambientOcclusion, settings));

    settings.ambientOcclusion = false;
    EXPECT_FALSE (RenderCapabilities (desktopMetal(), settings, 4).isEnabled (Feature::ambientOcclusion, settings));
}

TEST_F (RenderCapabilitiesTests, AmbientOcclusionNeedsAPositiveDistance)
{
    RenderSettings settings;
    settings.ambientOcclusion = true;

    for (const auto distance : { 0.0f, -0.1f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity() })
    {
        settings.aoDistance = distance;
        EXPECT_FALSE (RenderCapabilities (desktopMetal(), settings, 4).isEnabled (Feature::ambientOcclusion, settings)) << distance;
        EXPECT_FALSE (RenderCapabilities (webgl2 (true, 4), settings, 4).isEnabled (Feature::ambientOcclusion, settings)) << distance;
    }

    settings.aoDistance = 0.001f;
    EXPECT_TRUE (RenderCapabilities (desktopMetal(), settings, 4).isEnabled (Feature::ambientOcclusion, settings));
}

TEST_F (RenderCapabilitiesTests, AmbientOcclusionUnavailableOnRasterWithoutHalfFloat)
{
    auto facts = webgl2 (false, 0);
    facts.rgba16floatRenderable = false;
    facts.depthNormalPrepass = false;

    RenderSettings settings;
    settings.ambientOcclusion = true;
    RenderCapabilities capabilities (facts, settings, 4);

    EXPECT_FALSE (capabilities.isAvailable (Feature::ambientOcclusion));
    EXPECT_FALSE (capabilities.isEnabled (Feature::ambientOcclusion, settings));
}

TEST_F (RenderCapabilitiesTests, AmbientOcclusionOnRasterNeedsTheDepthNormalPrepass)
{
    RenderSettings settings;
    settings.ambientOcclusion = true;

    // The fact alone decides, whatever the half float targets
    auto facts = webgl2 (true, 4);
    facts.depthNormalPrepass = false;
    EXPECT_FALSE (RenderCapabilities (facts, settings, 4).isAvailable (Feature::ambientOcclusion));

    facts.depthNormalPrepass = true;
    EXPECT_TRUE (RenderCapabilities (facts, settings, 4).isAvailable (Feature::ambientOcclusion));

    // The full tier reads the G-buffer instead, whatever the fact says
    auto full = desktopMetal();
    full.depthNormalPrepass = false;
    EXPECT_TRUE (RenderCapabilities (full, settings, 4).isEnabled (Feature::ambientOcclusion, settings));
}
