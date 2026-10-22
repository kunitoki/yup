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

#if YUP_ENABLE_SHADER_TRANSPILER

class ProbeRendererMetalTests : public ProbeRendererGpuTests
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";

        probes = std::make_unique<ProbeRenderer> (device);
    }
};

TEST_F (ProbeRendererMetalTests, RadianceIsHalfFloatWhenRenderable)
{
    EXPECT_EQ (probes->getRadianceFormat(), GpuTextureFormat::rgba16float);

    auto cube = probes->createCube (64);
    ASSERT_NE (cube, nullptr);
    EXPECT_EQ (cube->getType(), GpuTextureType::cube);
    EXPECT_EQ (cube->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (cube->getWidth(), 64);
    EXPECT_EQ (cube->getDepthOrArrayLayers(), 6u);
    EXPECT_EQ (cube->getMipLevels(), 7u);

    probes->setForceRgbmForTesting (true);
    EXPECT_EQ (probes->getRadianceFormat(), GpuTextureFormat::rgba8unorm);
    auto rgbmCube = probes->createCube (8);
    ASSERT_NE (rgbmCube, nullptr);
    EXPECT_EQ (rgbmCube->getFormat(), GpuTextureFormat::rgba8unorm);

    EXPECT_EQ (probes->createCube (0), nullptr);

    // A shorter mip chain on request, never a longer one
    probes->setForceRgbmForTesting (false);
    auto shortCube = probes->createCube (64, 4);
    ASSERT_NE (shortCube, nullptr);
    EXPECT_EQ (shortCube->getMipLevels(), 4u);
    EXPECT_EQ (probes->createCube (8, 5), nullptr);
}

TEST_F (ProbeRendererMetalTests, RejectsMissingOrWrongShapedTextures)
{
    auto cube = probes->createCube (16);
    auto other = probes->createCube (16);
    auto plane = makeAtlasWithMips (64, 64, 3);
    auto atlas = makeAtlasWithMips (256, 128, 5);
    ASSERT_NE (cube, nullptr);
    ASSERT_NE (other, nullptr);
    ASSERT_NE (plane, nullptr);
    ASSERT_NE (atlas, nullptr);

    GpuTextureDesc notTargetDesc (16, 16, GpuTextureFormat::rgba16float);
    notTargetDesc.type = GpuTextureType::cube;
    notTargetDesc.depthOrArrayLayers = 6;
    auto notTarget = GpuTexture::create (device, notTargetDesc);
    ASSERT_NE (notTarget, nullptr);

    auto wrongFormat = GpuTexture::create (device, [] { GpuTextureDesc d (16, 16, GpuTextureFormat::rgba8unorm, true); d.type = GpuTextureType::cube; d.depthOrArrayLayers = 6; return d; }());
    ASSERT_NE (wrongFormat, nullptr);

    auto frame = GpuFrame::begin (device);

    EXPECT_FALSE (probes->equirectToCube (frame, nullptr, cube));
    EXPECT_FALSE (probes->equirectToCube (frame, plane, nullptr));
    EXPECT_FALSE (probes->equirectToCube (frame, cube, other));
    EXPECT_FALSE (probes->equirectToCube (frame, plane, plane));
    EXPECT_FALSE (probes->equirectToCube (frame, plane, notTarget));
    EXPECT_FALSE (probes->equirectToCube (frame, plane, wrongFormat));

    EXPECT_FALSE (probes->downsampleCube (frame, nullptr));
    EXPECT_FALSE (probes->downsampleCube (frame, plane));
    EXPECT_FALSE (probes->downsampleCube (frame, notTarget));

    EXPECT_FALSE (probes->prefilterToCube (frame, nullptr, cube));
    EXPECT_FALSE (probes->prefilterToCube (frame, cube, nullptr));
    EXPECT_FALSE (probes->prefilterToCube (frame, cube, cube));
    EXPECT_FALSE (probes->prefilterToCube (frame, plane, cube));
    EXPECT_FALSE (probes->prefilterToCube (frame, cube, plane));

    EXPECT_FALSE (probes->prefilterToAtlas (frame, nullptr, atlas, { 0, 0, 128, 128 }, 5));
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, nullptr, { 0, 0, 128, 128 }, 5));
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, cube, { 0, 0, 16, 16 }, 1));
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 0, 0, 128, 128 }, 6));   // more mips than the atlas
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 0, 0, 128, 128 }, 0));   // no mip
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 192, 0, 128, 128 }, 5)); // outside
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 8, 0, 128, 128 }, 5));   // not aligned
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 0, 0, 128, 64 }, 5));    // not square
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 0, 0, 16, 16 }, 5));     // 1 texel at the last mip

    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());
}

TEST_F (ProbeRendererMetalTests, ConstantRadianceStaysConstantAtEveryMip)
{
    checkConstantRadianceStaysConstantAtEveryMip (2.0e-3f);
}

TEST_F (ProbeRendererMetalTests, RgbmConstantRadianceStaysConstantAtEveryMip)
{
    probes->setForceRgbmForTesting (true);
    checkConstantRadianceStaysConstantAtEveryMip (0.02f);
}

TEST_F (ProbeRendererMetalTests, DownsampleKeepsTheEnergyOfABrightSpot)
{
    auto cube = makeSourceCube (brightSpot, 64);
    ASSERT_NE (cube, nullptr);

    const auto total = integrateRed (cube, 0);
    EXPECT_GT (total, 3.0); // the spot alone holds about 3.1

    for (int mip = 1; mip < static_cast<int> (cube->getMipLevels()); ++mip)
        EXPECT_NEAR (integrateRed (cube, mip) / total, 1.0, 0.01) << "mip " << mip;
}

TEST_F (ProbeRendererMetalTests, PrefilterKeepsTheEnergyOfABrightSpot)
{
    auto cube = makeCubeFrom (brightSpot, 64);
    ASSERT_NE (cube, nullptr);

    const auto total = integrateRed (cube, 0);
    EXPECT_GT (total, 3.0);

    for (int mip = 1; mip < static_cast<int> (cube->getMipLevels()); ++mip)
        EXPECT_NEAR (integrateRed (cube, mip) / total, 1.0, 0.08) << "mip " << mip;
}

TEST_F (ProbeRendererMetalTests, PrefilterSpreadsABrightSpot)
{
    auto cube = makeCubeFrom (brightSpot, 64);
    ASSERT_NE (cube, nullptr);

    // Faces of two texels have none near +X, so a lookup there averages texels far from the spot
    // (a single texel faces it again): the peak is followed down to faces of four texels
    auto previousPeak = sampleRed (cube, { 1.0f, 0.0f, 0.0f }, 0);

    for (int mip = 1; (cube->getWidth() >> mip) >= 4; ++mip)
    {
        const auto peak = sampleRed (cube, { 1.0f, 0.0f, 0.0f }, mip);
        EXPECT_LT (peak, previousPeak) << "mip " << mip;
        previousPeak = peak;
    }

    // The long GGX tail reaches 20 degrees off the spot, three texels of mip 2 away, where the
    // copy and a plain mip chain still read the 0.1 background
    const auto offAxis = Vector3<float> (std::cos (0.35f), std::sin (0.35f), 0.0f);
    EXPECT_LT (sampleRed (cube, offAxis, 0), 0.2f);
    EXPECT_GT (sampleRed (cube, offAxis, 2), 0.5f);

    // The spot reaches the copy at full strength and the roughest mip much dimmer
    EXPECT_GT (sampleRed (cube, { 1.0f, 0.0f, 0.0f }, 0), 50.0f);
    EXPECT_LT (sampleRed (cube, { 1.0f, 0.0f, 0.0f }, static_cast<int> (cube->getMipLevels()) - 1), 5.0f);
}

TEST_F (ProbeRendererMetalTests, PrefilterMatchesTheCpuGgxReference)
{
    checkPrefilterMatchesTheCpuGgxReference (0.05f);
}

TEST_F (ProbeRendererMetalTests, RgbmPrefilterMatchesTheCpuGgxReference)
{
    probes->setForceRgbmForTesting (true);
    checkPrefilterMatchesTheCpuGgxReference (0.08f);
}

TEST_F (ProbeRendererMetalTests, OctahedralTileMatchesTheCube)
{
    checkOctahedralTileMatchesTheCube (4.0f / 255.0f);
}

TEST_F (ProbeRendererMetalTests, PrefilterToAtlasKeepsTheOtherTiles)
{
    checkPrefilterToAtlasKeepsTheOtherTiles();
}

TEST_F (ProbeRendererMetalTests, FractionalLodReadsNoNeighborTile)
{
    checkFractionalLodReadsNoNeighborTile();
}

TEST_F (ProbeRendererMetalTests, PrefilterMipZeroIsACopyOfTheSource)
{
    checkPrefilterMipZeroIsACopyOfTheSource();
}

TEST_F (ProbeRendererMetalTests, PrefilteredCubeHasNoSeams)
{
    checkPrefilteredCubeHasNoSeams();
}

TEST_F (ProbeRendererMetalTests, IrradianceOfAConstantCubeIsItsRadiance)
{
    // Over a constant radiance the first band is the radiance itself, the second is zero
    checkIrradianceMatchesTheCpuProjection (constantRadiance, 0.01f);
}

TEST_F (ProbeRendererMetalTests, IrradianceMatchesTheCpuProjection)
{
    checkIrradianceMatchesTheCpuProjection (skyGradient, 0.01f);
}

TEST_F (ProbeRendererMetalTests, RgbmIrradianceMatchesTheCpuProjection)
{
    probes->setForceRgbmForTesting (true);
    ASSERT_EQ (probes->getRadianceFormat(), GpuTextureFormat::rgba8unorm);
    checkIrradianceMatchesTheCpuProjection (skyGradient, 0.03f);
}

TEST_F (ProbeRendererMetalTests, IrradianceRejectsMisplacedTexels)
{
    auto cube = probes->createCube (32);
    GpuTextureDesc desc (8, 8, probes->getRadianceFormat(), true);
    auto atlas = GpuTexture::create (device, desc);
    ASSERT_NE (cube, nullptr);
    ASSERT_NE (atlas, nullptr);

    auto frame = GpuFrame::begin (device);
    EXPECT_FALSE (probes->projectIrradiance (frame, cube, atlas, { 0, 5 })); // Four rows from 5 leave the atlas
    EXPECT_FALSE (probes->projectIrradiance (frame, cube, atlas, { -1, 0 }));
    EXPECT_FALSE (probes->projectIrradiance (frame, nullptr, atlas, { 0, 0 }));
    EXPECT_FALSE (probes->projectIrradiance (frame, cube, nullptr, { 0, 0 }));
    frame.submit();
}

TEST_F (ProbeRendererMetalTests, VisibilityHoldsTheMomentsOfTheDistance)
{
    checkVisibilityHoldsTheMomentsOfTheDistance();
}

TEST_F (ProbeRendererMetalTests, VisibilityBorderMirrorsTheTileEdges)
{
    checkVisibilityBorderMirrorsTheTileEdges();
}

TEST_F (ProbeRendererMetalTests, VisibilityRejectsMisplacedTiles)
{
    auto cube = makeDistanceCube (8);
    GpuTextureDesc desc (24, 24, ProbeRenderer::visibilityFormat, true);
    auto atlas = GpuTexture::create (device, desc);
    ASSERT_NE (cube, nullptr);
    ASSERT_NE (atlas, nullptr);

    auto frame = GpuFrame::begin (device);
    EXPECT_FALSE (probes->renderVisibility (frame, cube, atlas, { 8, 0 }, 16, 1.0f)); // 18 texels from 8 leave the atlas
    EXPECT_FALSE (probes->renderVisibility (frame, cube, atlas, { 0, 0 }, 1, 1.0f));
    EXPECT_FALSE (probes->renderVisibility (frame, nullptr, atlas, { 0, 0 }, 16, 1.0f));
    frame.submit();
}

//==============================================================================
class EnvironmentMapMetalTests : public EnvironmentMapGpuTests
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";

        probes = std::make_unique<ProbeRenderer> (device);
    }
};

TEST_F (EnvironmentMapMetalTests, BuildsAPrefilteredCube)
{
    checkBuildsAPrefilteredCube();
    EXPECT_EQ (probes->getRadianceFormat(), GpuTextureFormat::rgba16float); // Half floats render on Metal
}

TEST_F (EnvironmentMapMetalTests, ConstantEnvironmentStaysConstantAtEveryLevel)
{
    checkConstantEnvironmentStaysConstantAtEveryLevel (1.0e-3f);
}

TEST_F (EnvironmentMapMetalTests, BuildKeepsWhatTheFrameDoesNotRetain)
{
    checkBuildKeepsWhatTheFrameDoesNotRetain();
}

TEST_F (EnvironmentMapMetalTests, RgbmEnvironmentMatchesFloat)
{
    checkRgbmEnvironmentMatchesFloat();
}

TEST_F (EnvironmentMapMetalTests, FailedBuildReportsItsStatus)
{
    checkFailedBuildReportsItsStatus();
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
