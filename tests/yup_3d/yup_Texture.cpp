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

class TextureResourceTests : public ::testing::Test
{
};

TEST_F (TextureResourceTests, CreateMipChainHalvesDownToOnePixel)
{
    Image image (5, 3, PixelFormat::RGBA);
    image.fill (0xff00ff00);

    const auto levels = Texture::createMipChain (image);

    // 5x3 -> 2x1 -> 1x1
    ASSERT_EQ (levels.size(), 3u);
    EXPECT_EQ (levels[0].size(), 5u * 3u * 4u);
    EXPECT_EQ (levels[1].size(), 2u * 1u * 4u);
    EXPECT_EQ (levels[2].size(), 4u);

    // Opaque green stays opaque green, in RGBA byte order
    EXPECT_EQ (levels[2][0], 0);
    EXPECT_EQ (levels[2][1], 255);
    EXPECT_EQ (levels[2][2], 0);
    EXPECT_EQ (levels[2][3], 255);
}

TEST_F (TextureResourceTests, CreateMipChainAveragesPixels)
{
    Image image (2, 2, PixelFormat::RGBA);
    image.fill (0xff000000);
    image.setPixel (0, 0, 0xffffffff);
    image.setPixel (1, 1, 0xffffffff);

    const auto levels = Texture::createMipChain (image);

    ASSERT_EQ (levels.size(), 2u);
    EXPECT_NEAR (levels[1][0], 128, 1);
    EXPECT_EQ (levels[1][3], 255);
}

TEST_F (TextureResourceTests, CreateMipChainConvertsRgbToOpaqueRgba)
{
    Image image (1, 1, PixelFormat::RGB);
    image.setPixel (0, 0, 0xff102030);

    const auto levels = Texture::createMipChain (image);

    ASSERT_EQ (levels.size(), 1u);
    EXPECT_EQ (levels[0], (std::vector<uint8> { 0x10, 0x20, 0x30, 0xff }));
}

TEST_F (TextureResourceTests, CreateMipChainOfInvalidImageIsEmpty)
{
    EXPECT_TRUE (Texture::createMipChain (Image()).empty());
}

TEST_F (TextureResourceTests, KeepsImageSamplerAndColorSpace)
{
    Image image (2, 2, PixelFormat::RGBA);
    const GpuSamplerDesc sampler (GpuFilter::linear, GpuWrapMode::repeat);

    const Texture texture (image, sampler, true);

    EXPECT_EQ (texture.getImage().getWidth(), 2);
    EXPECT_EQ (texture.getSamplerDesc().wrapU, GpuWrapMode::repeat);
    EXPECT_EQ (texture.getSamplerDesc().magFilter, GpuFilter::linear);
    EXPECT_TRUE (texture.isSrgb());
}

TEST_F (TextureResourceTests, GpuOnlyTextureHasNoImage)
{
    Texture texture (GpuTexture::Ptr(), GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::clampToEdge), true);

    EXPECT_TRUE (texture.isGpuOnly());
    EXPECT_FALSE (texture.getImage().isValid());
    EXPECT_TRUE (texture.isSrgb());
    EXPECT_EQ (texture.getSamplerDesc().wrapU, GpuWrapMode::clampToEdge);
    EXPECT_EQ (texture.getGpuTexture (nullptr), nullptr);

    texture.setGpuTexture (nullptr);
    EXPECT_FALSE (Texture (Image (1, 1, PixelFormat::RGBA)).isGpuOnly());
}

TEST_F (TextureResourceTests, CountsEverySetGpuTexture)
{
    Texture texture (GpuTexture::Ptr {});
    EXPECT_EQ (texture.getChangeCount(), 0u);

    // Content redrawn into the same GpuTexture is a change too
    texture.setGpuTexture (nullptr);
    texture.setGpuTexture (nullptr);
    EXPECT_EQ (texture.getChangeCount(), 2u);
}

class TextureGpuResourceTests : public ::testing::Test
{
protected:
    struct AnisotropicOreContext : public ::testing::NiceMock<MockOreContext>
    {
        explicit AnisotropicOreContext (bool anisotropic) { m_features.anisotropicFiltering = anisotropic; }
    };

    /** Makes @p oreCtx create mocked textures, counting the textures and their uploads. */
    void countTexturesAndUploads (MockOreContext& oreCtx)
    {
        ON_CALL (oreCtx, makeTexture (::testing::_))
            .WillByDefault (::testing::Invoke ([this] (const rive::ore::TextureDesc& desc)
        {
            ++texturesMade;
            lastMipCount = desc.numMipmaps;

            auto texture = rive::make_rcp<::testing::NiceMock<MockOreTexture>> (desc);
            ON_CALL (*texture, uploadImpl (::testing::_))
                .WillByDefault (::testing::Invoke ([this] (const rive::ore::TextureDataDesc&)
            {
                ++uploads;
            }));
            return rive::rcp<rive::ore::Texture> (texture);
        }));
    }

    static GpuSamplerDesc linearSampler (uint32_t anisotropy)
    {
        GpuSamplerDesc desc (GpuFilter::linear, GpuWrapMode::repeat);
        desc.mipmapFilter = GpuFilter::linear;
        desc.maxAnisotropy = anisotropy;
        return desc;
    }

    int texturesMade = 0;
    int uploads = 0;
    uint32_t lastMipCount = 0;
};

TEST_F (TextureGpuResourceTests, UploadsEveryMipLevelOncePerDevice)
{
    ::testing::NiceMock<MockOreContext> oreCtx;
    countTexturesAndUploads (oreCtx);

    GpuDevice::Ptr device = new OreInjectedGpuDevice (&oreCtx);
    Texture texture (Image (4, 4, PixelFormat::RGBA));

    const auto first = texture.getGpuTexture (device);
    ASSERT_NE (first, nullptr);
    EXPECT_EQ (texturesMade, 1);
    EXPECT_EQ (lastMipCount, 3u);
    EXPECT_EQ (uploads, 3);

    EXPECT_EQ (texture.getGpuTexture (device), first);
    EXPECT_EQ (texturesMade, 1);

    GpuDevice::Ptr otherDevice = new OreInjectedGpuDevice (&oreCtx);
    const auto second = texture.getGpuTexture (otherDevice);
    ASSERT_NE (second, nullptr);
    EXPECT_NE (second, first);
    EXPECT_EQ (texturesMade, 2);
    EXPECT_EQ (uploads, 6);
}

TEST_F (TextureGpuResourceTests, FailedCreationReturnsNull)
{
    ::testing::NiceMock<MockOreContext> oreCtx;
    GpuDevice::Ptr device = new OreInjectedGpuDevice (&oreCtx);

    Texture invalid { Image() };
    EXPECT_EQ (invalid.getGpuTexture (device), nullptr);

    ON_CALL (oreCtx, makeTexture (::testing::_)).WillByDefault (::testing::Return (nullptr));
    Texture rejected (Image (2, 2, PixelFormat::RGBA));
    EXPECT_EQ (rejected.getGpuTexture (device), nullptr);
}

TEST_F (TextureGpuResourceTests, GpuOnlyTextureKeepsItsTextureAcrossDevices)
{
    ::testing::NiceMock<MockOreContext> oreCtx;
    countTexturesAndUploads (oreCtx);

    GpuDevice::Ptr device = new OreInjectedGpuDevice (&oreCtx);
    const auto gpuTexture = GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm));
    ASSERT_NE (gpuTexture, nullptr);

    Texture texture (gpuTexture);
    GpuDevice::Ptr otherDevice = new OreInjectedGpuDevice (&oreCtx);
    EXPECT_EQ (texture.getGpuTexture (device), gpuTexture);
    EXPECT_EQ (texture.getGpuTexture (otherDevice), gpuTexture);
    EXPECT_EQ (uploads, 0);
}

TEST_F (TextureGpuResourceTests, AnisotropyNeedsDeviceSupportAndLinearFiltering)
{
    const auto samplerAnisotropy = [] (bool deviceSupport, const GpuSamplerDesc& desc)
    {
        AnisotropicOreContext oreCtx (deviceSupport);
        uint32_t anisotropy = 0;
        EXPECT_CALL (oreCtx, makeSampler (::testing::_))
            .WillOnce (::testing::Invoke ([&anisotropy] (const rive::ore::SamplerDesc& d)
        {
            anisotropy = d.maxAnisotropy;
            return rive::rcp<rive::ore::Sampler> (rive::make_rcp<TestOreSampler>());
        }));

        GpuDevice::Ptr device = new OreInjectedGpuDevice (&oreCtx);
        Texture texture (Image (1, 1, PixelFormat::RGBA), desc);

        const auto sampler = texture.getGpuSampler (device);
        EXPECT_NE (sampler, nullptr);
        EXPECT_EQ (texture.getGpuSampler (device), sampler);
        return anisotropy;
    };

    EXPECT_EQ (samplerAnisotropy (true, linearSampler (8)), 8u);
    EXPECT_EQ (samplerAnisotropy (false, linearSampler (8)), 1u);

    auto nearestMips = linearSampler (8);
    nearestMips.mipmapFilter = GpuFilter::nearest;
    EXPECT_EQ (samplerAnisotropy (true, nearestMips), 1u);

    Texture texture (Image (1, 1, PixelFormat::RGBA));
    EXPECT_EQ (texture.getGpuSampler (nullptr), nullptr);
}
