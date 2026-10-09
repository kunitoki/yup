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
