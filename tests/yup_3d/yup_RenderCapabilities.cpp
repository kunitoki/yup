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
