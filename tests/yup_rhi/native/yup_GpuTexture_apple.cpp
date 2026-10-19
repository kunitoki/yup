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

#include <yup_rhi/yup_rhi.h>

using namespace yup;

#if YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// Metal - real device rendering, read back through a canvas-backed target
//==============================================================================

class GpuTextureMetalTests : public GpuReadbackFixture
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";
    }

    /** The readback row showing texture row @p row of a 4x4 level: fullscreenVertex puts v = 0 at the bottom. */
    static int readbackRow (int row) { return 3 - row; }
};

TEST_F (GpuTextureMetalTests, SamplesOneMipThroughANarrowedView)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 2;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    clearMip (texture, 0, GpuColor (1.0f, 0.0f, 0.0f, 1.0f));
    clearMip (texture, 1, GpuColor (0.0f, 1.0f, 0.0f, 1.0f));

    GpuTextureViewDesc mipOne (1, 0);
    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, texture, mipOne);
    });

    ASSERT_FALSE (pixels.empty());
    EXPECT_LE (pixels[0], 5);
    EXPECT_GE (pixels[1], 250);
}

TEST_F (GpuTextureMetalTests, GenerateMipsFillsEveryLevelFromTheBase)
{
    GpuTextureDesc desc (16, 16, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 5;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    clearMip (texture, 0, GpuColor (0.0f, 0.0f, 1.0f, 1.0f));

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (texture->generateMips (frame));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 1, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, texture, GpuTextureViewDesc (4, 0));
    });

    ASSERT_FALSE (pixels.empty());
    EXPECT_GE (pixels[2], 250);
}

TEST_F (GpuTextureMetalTests, GenerateMipsKeepsTheOrientationOfEveryLevel)
{
    GpuTextureDesc desc (16, 16, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 3;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> source (16 * 16 * 4);
    for (size_t row = 0; row < 16; ++row)
    {
        for (size_t column = 0; column < 16; ++column)
        {
            auto* texel = source.data() + (row * 16 + column) * 4;
            texel[0] = row < 8 ? 255 : 0;
            texel[2] = row < 8 ? 0 : 255;
            texel[3] = 255;
        }
    }

    GpuTextureDataDesc data;
    data.data = source.data();
    ASSERT_TRUE (texture->upload (data));

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (texture->generateMips (frame));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto readLevel = [&] (uint32_t level)
    {
        return drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass)
        {
            pass.setTexture (0, 0, texture, GpuTextureViewDesc (level, 0));
        });
    };

    const auto base = readLevel (0);
    ASSERT_FALSE (base.empty());
    const auto topRow = 0u;
    const auto bottomRow = 3u * 4u * 4u;

    for (uint32_t level = 1; level < 3; ++level)
    {
        const auto pixels = readLevel (level);
        ASSERT_FALSE (pixels.empty());

        for (uint32_t channel = 0; channel < 3; ++channel)
        {
            EXPECT_NEAR (pixels[topRow + channel], base[topRow + channel], 4) << "level " << level << " top channel " << channel;
            EXPECT_NEAR (pixels[bottomRow + channel], base[bottomRow + channel], 4) << "level " << level << " bottom channel " << channel;
        }
    }
}

TEST_F (GpuTextureMetalTests, UploadIntoARenderTargetWritesTheRequestedMipRegion)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 2;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    clearMip (texture, 1, GpuColor (0.0f, 0.0f, 0.0f, 1.0f));

    // Two rows of two texels, red green over blue white, with a padded row stride
    // and no padding after the last row.
    const uint8_t region[] = { 255, 0, 0, 255, 0, 255, 0, 255, 128, 128, 128, 128, 0, 0, 255, 255, 255, 255, 255, 255 };

    GpuTextureDataDesc data;
    data.data = region;
    data.mipLevel = 1;
    data.x = 2;
    data.y = 1;
    data.width = 2;
    data.height = 2;
    data.bytesPerRow = 12;
    ASSERT_TRUE (texture->upload (data));

    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, texture, GpuTextureViewDesc (1, 0));
    });
    ASSERT_FALSE (pixels.empty());

    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            uint8_t expected[4] = { 0, 0, 0, 255 };
            if (row >= 1 && row <= 2 && column >= 2)
                std::memcpy (expected, region + (row - 1) * 12 + (column - 2) * 4, 4);

            const auto* texel = pixels.data() + (readbackRow (row) * 4 + column) * 4;
            for (int channel = 0; channel < 4; ++channel)
                EXPECT_NEAR (texel[channel], expected[channel], 2) << "row " << row << " column " << column << " channel " << channel;
        }
    }
}

TEST_F (GpuTextureMetalTests, UploadIntoA3DRenderTargetWritesTheRequestedSlices)
{
    GpuTextureDesc desc (4, 4, GpuTextureFormat::rgba8unorm, true);
    desc.type = GpuTextureType::texture3D;
    desc.depthOrArrayLayers = 4;
    auto volume = GpuTexture::create (device, desc);
    ASSERT_NE (volume, nullptr);

    // Slices 1 and 2: red encodes the row, blue the column, green the slice
    std::vector<uint8_t> texels (4 * 4 * 2 * 4);
    for (int slice = 0; slice < 2; ++slice)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 4; ++column)
            {
                auto* texel = texels.data() + ((slice * 4 + row) * 4 + column) * 4;
                texel[0] = static_cast<uint8_t> (row * 64);
                texel[1] = static_cast<uint8_t> (80 + slice * 80);
                texel[2] = static_cast<uint8_t> (column * 64);
                texel[3] = 255;
            }
        }
    }

    GpuTextureDataDesc data;
    data.data = texels.data();
    data.z = 1;
    data.depth = 2;
    ASSERT_TRUE (volume->upload (data));

    auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
    ASSERT_NE (sampler, nullptr);

    for (int slice = 0; slice < 2; ++slice)
    {
        const auto w = String ((slice + 1.5f) / 4.0f);
        const auto pixels = drawIntoReadback (R"(#version 450
layout(set = 0, binding = 0) uniform texture3D volume;
layout(set = 0, binding = 1) uniform sampler volumeSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = textureLod (sampler3D (volume, volumeSampler), vec3 (uv, )" + w + R"(), 0.0); })",
                                              4,
                                              [&] (GpuRenderPass& pass)
        {
            pass.setTexture (0, 0, volume);
            pass.setSampler (0, 1, sampler);
        });
        ASSERT_FALSE (pixels.empty());

        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 4; ++column)
            {
                const auto* expected = texels.data() + ((slice * 4 + row) * 4 + column) * 4;
                const auto* texel = pixels.data() + (readbackRow (row) * 4 + column) * 4;
                for (int channel = 0; channel < 4; ++channel)
                    EXPECT_NEAR (texel[channel], expected[channel], 2) << "slice " << slice << " row " << row << " column " << column << " channel " << channel;
            }
        }
    }
}

TEST_F (GpuTextureMetalTests, UploadIntoACombinedDepthStencilRenderTargetFails)
{
    for (auto format : { GpuTextureFormat::depth24plusStencil8, GpuTextureFormat::depth32floatStencil8 })
    {
        auto texture = GpuTexture::create (device, GpuTextureDesc (8, 8, format, true));
        ASSERT_NE (texture, nullptr);

        std::vector<uint8_t> texels (8 * 8 * 8, 0);
        GpuTextureDataDesc data;
        data.data = texels.data();
        EXPECT_FALSE (texture->upload (data)) << (int) format;
    }
}

TEST_F (GpuTextureMetalTests, GenerateMipsReachesTheLastLevelOfAnOddSizedTexture)
{
    GpuTextureDesc desc (5, 5, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 3;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    clearMip (texture, 0, GpuColor (0.0f, 1.0f, 0.0f, 1.0f));

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (texture->generateMips (frame));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 1, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, texture, GpuTextureViewDesc (2, 0));
    });

    ASSERT_FALSE (pixels.empty());
    EXPECT_GE (pixels[1], 250);
}

TEST_F (GpuTextureMetalTests, GenerateMipsRejectsUnsupportedTextures)
{
    auto frame = GpuFrame::begin (device);

    auto single = GpuTexture::create (device, GpuTextureDesc (8, 8, GpuTextureFormat::rgba8unorm, true));
    ASSERT_NE (single, nullptr);
    EXPECT_FALSE (single->generateMips (frame));

    GpuTextureDesc notTarget (8, 8, GpuTextureFormat::rgba8unorm, false);
    notTarget.mipLevels = 2;
    auto notTargetTexture = GpuTexture::create (device, notTarget);
    ASSERT_NE (notTargetTexture, nullptr);
    EXPECT_FALSE (notTargetTexture->generateMips (frame));

    GpuTextureDesc cube (8, 8, GpuTextureFormat::rgba8unorm, true);
    cube.type = GpuTextureType::cube;
    cube.depthOrArrayLayers = 6;
    cube.mipLevels = 2;
    auto cubeTexture = GpuTexture::create (device, cube);
    ASSERT_NE (cubeTexture, nullptr);
    EXPECT_FALSE (cubeTexture->generateMips (frame));

    GpuTextureDesc depth (8, 8, GpuTextureFormat::depth32float, true);
    depth.mipLevels = 2;
    auto depthTexture = GpuTexture::create (device, depth);
    ASSERT_NE (depthTexture, nullptr);
    EXPECT_FALSE (depthTexture->generateMips (frame));

    GpuTextureDesc multisampled (8, 8, GpuTextureFormat::rgba8unorm, true);
    multisampled.mipLevels = 1;
    multisampled.sampleCount = 4;
    auto multisampledTexture = GpuTexture::create (device, multisampled);
    ASSERT_NE (multisampledTexture, nullptr);
    EXPECT_FALSE (multisampledTexture->generateMips (frame));

    frame.submit();
}

TEST_F (GpuTextureMetalTests, WholeViewAfterNarrowedViewSeesAllLevels)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 2;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    clearMip (texture, 0, GpuColor (1.0f, 0.0f, 0.0f, 1.0f));
    clearMip (texture, 1, GpuColor (0.0f, 1.0f, 0.0f, 1.0f));

    drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass) { pass.setTexture (0, 0, texture, GpuTextureViewDesc (1, 0)); });
    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass) { pass.setTexture (0, 0, texture); });

    ASSERT_FALSE (pixels.empty());
    EXPECT_GE (pixels[0], 250); // lod 0 of the whole texture is mip 0 again
    EXPECT_LE (pixels[1], 5);
}

TEST_F (GpuTextureMetalTests, ComparisonSamplerComparesAgainstStoredDepth)
{
    for (auto format : { GpuTextureFormat::depth16unorm, GpuTextureFormat::depth32float, GpuTextureFormat::depth24plusStencil8 })
    {
        auto depth = makeClearedDepth (format, 0.25f, 8);
        ASSERT_NE (depth, nullptr);

        GpuSamplerDesc compareDesc (GpuFilter::linear, GpuWrapMode::clampToEdge);
        compareDesc.compare = GpuCompareFunction::lessEqual;
        auto sampler = GpuSampler::create (device, compareDesc);
        ASSERT_NE (sampler, nullptr);

        const auto pixels = drawIntoReadback (compareFragment, 8, [&] (GpuRenderPass& pass)
        {
            pass.setTexture (0, 0, depth);
            pass.setSampler (0, 1, sampler);
        });

        ASSERT_EQ (pixels.size(), 8u * 8u * 4u);
        const auto center = (4u * 8u + 4u) * 4u;
        EXPECT_GE (pixels[center + 0], 250) << "format " << (int) format;
        EXPECT_LE (pixels[center + 1], 5) << "format " << (int) format;
    }
}

TEST_F (GpuTextureMetalTests, ShadowSamplerSlotDefaultsToAComparisonSampler)
{
    auto depth = makeClearedDepth (GpuTextureFormat::depth32float, 0.25f, 8);
    ASSERT_NE (depth, nullptr);

    // No setSampler: the pipeline's default sampler for the samplerShadow slot must compare
    const auto pixels = drawIntoReadback (compareFragment, 8, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, depth);
    });

    ASSERT_EQ (pixels.size(), 8u * 8u * 4u);
    const auto center = (4u * 8u + 4u) * 4u;
    EXPECT_GE (pixels[center + 0], 250);
    EXPECT_LE (pixels[center + 1], 5);
}

TEST_F (GpuTextureMetalTests, ArrayComparisonSamplerComparesEachLayer)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::depth32float, true);
    desc.type = GpuTextureType::array2D;
    desc.depthOrArrayLayers = 2;

    auto depth = GpuTexture::create (device, desc);
    ASSERT_NE (depth, nullptr);
    ASSERT_TRUE (clearDepth (depth, 0.25f, 0));
    ASSERT_TRUE (clearDepth (depth, 0.75f, 1));

    GpuSamplerDesc compareDesc (GpuFilter::linear, GpuWrapMode::clampToEdge);
    compareDesc.compare = GpuCompareFunction::lessEqual;
    auto sampler = GpuSampler::create (device, compareDesc);
    ASSERT_NE (sampler, nullptr);

    // Reference 0.5 sits between the layers: it fails against 0.25 and passes against 0.75
    const auto pixels = drawIntoReadback (R"(#version 450
layout(set = 0, binding = 0) uniform texture2DArray shadowMaps;
layout(set = 0, binding = 1) uniform samplerShadow shadowSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    float layer0 = texture (sampler2DArrayShadow (shadowMaps, shadowSampler), vec4 (uv, 0.0, 0.5));
    float layer1 = texture (sampler2DArrayShadow (shadowMaps, shadowSampler), vec4 (uv, 1.0, 0.5));
    color = vec4 (layer0, layer1, 0.0, 1.0);
})",
                                          8,
                                          [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, depth);
        pass.setSampler (0, 1, sampler);
    });

    ASSERT_EQ (pixels.size(), 8u * 8u * 4u);
    const auto center = (4u * 8u + 4u) * 4u;
    EXPECT_LE (pixels[center + 0], 5);
    EXPECT_GE (pixels[center + 1], 250);
}

TEST_F (GpuTextureMetalTests, SameDepthTextureBindsForCompareAndRawReads)
{
    auto depth = makeClearedDepth (GpuTextureFormat::depth32float, 0.25f, 8);
    ASSERT_NE (depth, nullptr);

    GpuSamplerDesc compareDesc (GpuFilter::linear, GpuWrapMode::clampToEdge);
    compareDesc.compare = GpuCompareFunction::lessEqual;
    auto compareSampler = GpuSampler::create (device, compareDesc);
    auto pointSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
    ASSERT_NE (compareSampler, nullptr);
    ASSERT_NE (pointSampler, nullptr);

    // One depth texture at two bindings: compared through one sampler, read raw through the other
    const auto pixels = drawIntoReadback (dualBindingFragment, 8, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, depth);
        pass.setTexture (0, 1, depth);
        pass.setSampler (0, 2, compareSampler);
        pass.setSampler (0, 3, pointSampler);
    });

    ASSERT_EQ (pixels.size(), 8u * 8u * 4u);
    const auto center = (4u * 8u + 4u) * 4u;
    EXPECT_GE (pixels[center + 0], 250);     // 0.2 <= 0.25
    EXPECT_NEAR (pixels[center + 1], 64, 2); // 0.25 * 255
}

TEST_F (GpuTextureMetalTests, DepthOnlyPassWritesALayerOfAnArray)
{
    auto depth = makeDepthArray (8);
    ASSERT_NE (depth, nullptr);

    ASSERT_TRUE (renderDepthOnly (depth, 0, 0.25f));
    ASSERT_TRUE (renderDepthOnly (depth, 1, 1.0f, { compileDepthOnlyPipeline (0.5f) }));

    // Layer 1 holds 0.5: reference 0.4 passes lessEqual, 0.6 fails
    const auto written = compareDepthLayer (depth, 1, GpuCompareFunction::lessEqual, 0.4f, 0.6f);
    ASSERT_FALSE (written.empty());
    EXPECT_GE (written[0], 250);
    EXPECT_LE (written[1], 5);

    // Layer 0 keeps its clear-only 0.25
    const auto cleared = compareDepthLayer (depth, 0, GpuCompareFunction::lessEqual, 0.2f, 0.3f);
    ASSERT_FALSE (cleared.empty());
    EXPECT_GE (cleared[0], 250);
    EXPECT_LE (cleared[1], 5);
}

TEST_F (GpuTextureMetalTests, DepthBiasShiftsWrittenDepth)
{
    auto depth = makeDepthArray (8);
    ASSERT_NE (depth, nullptr);

    // Bias pushes away from the viewer, so depth grows: 1000 units of 2^-24 near 0.5 add about 6e-5
    ASSERT_TRUE (renderDepthOnly (depth, 0, 1.0f, { compileDepthOnlyPipeline (0.5f) }));
    ASSERT_TRUE (renderDepthOnly (depth, 1, 1.0f, { compileDepthOnlyPipeline (0.5f, 1000) }));

    // reference < stored: 0.5 passes only above 0.5, 0.51 bounds the shift
    const auto unbiased = compareDepthLayer (depth, 0, GpuCompareFunction::less, 0.5f, 0.51f);
    ASSERT_FALSE (unbiased.empty());
    EXPECT_LE (unbiased[0], 5);
    EXPECT_LE (unbiased[1], 5);

    const auto biased = compareDepthLayer (depth, 1, GpuCompareFunction::less, 0.5f, 0.51f);
    ASSERT_FALSE (biased.empty());
    EXPECT_GE (biased[0], 250);
    EXPECT_LE (biased[1], 5);
}

TEST_F (GpuTextureMetalTests, DepthBiasDoesNotLeakIntoTheNextPipelineOfAPass)
{
    auto depth = makeDepthArray (8);
    ASSERT_NE (depth, nullptr);

    ASSERT_TRUE (renderDepthOnly (depth, 0, 1.0f, { compileDepthOnlyPipeline (0.5f, 1000), compileDepthOnlyPipeline (0.5f) }));

    // The unbiased draw comes last and overwrites with exactly 0.5
    const auto pixels = compareDepthLayer (depth, 0, GpuCompareFunction::less, 0.5f, 0.51f);
    ASSERT_FALSE (pixels.empty());
    EXPECT_LE (pixels[0], 5);
    EXPECT_LE (pixels[1], 5);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
