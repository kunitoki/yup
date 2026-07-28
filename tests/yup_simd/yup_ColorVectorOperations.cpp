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

#include <yup_simd/yup_simd.h>

using namespace yup;

// ==============================================================================
// premultiplyARGB tests
// ==============================================================================

TEST (ColorVectorOpsTests, PremultiplyARGBMatchesScalarReference)
{
    uint32_t pixels[] = { 0x80ff8040u, 0xff010203u, 0x00010203u };

    ColorVectorOperations::premultiplyARGB (pixels, 3);

    EXPECT_EQ (pixels[0], 0x80804020u);
    EXPECT_EQ (pixels[1], 0xff010203u);
    EXPECT_EQ (pixels[2], 0x00000000u);
}

TEST (ColorVectorOpsTests, PremultiplyARGBFullAlphaUnchanged)
{
    // alpha=0xff: all channels should be unchanged
    uint32_t pixels[] = { 0xff112233u };
    ColorVectorOperations::premultiplyARGB (pixels, 1);
    EXPECT_EQ (pixels[0], 0xff112233u);
}

TEST (ColorVectorOpsTests, PremultiplyARGBZeroAlphaBlacksOut)
{
    // alpha=0x00: all channels should become 0
    uint32_t pixels[] = { 0x00aabbccu };
    ColorVectorOperations::premultiplyARGB (pixels, 1);
    EXPECT_EQ (pixels[0], 0x00000000u);
}

TEST (ColorVectorOpsTests, PremultiplyARGBHalfAlpha)
{
    // alpha=0x80 (128): channels should be approximately halved
    // premultiply: (c * 128 + 127) / 255
    uint32_t pixels[] = { 0x80ffu << 16 | 0x80u << 8 | 0x80u };
    pixels[0] = (0x80u << 24) | (0xffu << 16) | (0x80u << 8) | 0x80u;
    const uint32_t alpha = 0x80u;
    const uint32_t expectedRed = (0xffu * alpha + 127u) / 255u;
    const uint32_t expectedGreen = (0x80u * alpha + 127u) / 255u;
    const uint32_t expectedBlue = (0x80u * alpha + 127u) / 255u;

    ColorVectorOperations::premultiplyARGB (pixels, 1);

    EXPECT_EQ ((pixels[0] >> 24) & 0xffu, alpha);
    EXPECT_EQ ((pixels[0] >> 16) & 0xffu, expectedRed);
    EXPECT_EQ ((pixels[0] >> 8) & 0xffu, expectedGreen);
    EXPECT_EQ (pixels[0] & 0xffu, expectedBlue);
}

TEST (ColorVectorOpsTests, PremultiplyARGBZeroPixels)
{
    // Should not write to output buffer
    uint32_t pixels[] = { 0xdeadbeefu };
    ColorVectorOperations::premultiplyARGB (pixels, 0);
    EXPECT_EQ (pixels[0], 0xdeadbeefu);
}

// ==============================================================================
// premultiplyRGBA tests
// ==============================================================================

TEST (ColorVectorOpsTests, PremultiplyRGBAMatchesScalarReference)
{
    uint8 pixels[] = { 255, 128, 64, 128, 1, 2, 3, 255, 1, 2, 3, 0 };

    ColorVectorOperations::premultiplyRGBA (pixels, 3);

    EXPECT_EQ (pixels[0], 128);
    EXPECT_EQ (pixels[1], 64);
    EXPECT_EQ (pixels[2], 32);
    EXPECT_EQ (pixels[3], 128);
    EXPECT_EQ (pixels[4], 1);
    EXPECT_EQ (pixels[5], 2);
    EXPECT_EQ (pixels[6], 3);
    EXPECT_EQ (pixels[7], 255);
    EXPECT_EQ (pixels[8], 0);
    EXPECT_EQ (pixels[9], 0);
    EXPECT_EQ (pixels[10], 0);
    EXPECT_EQ (pixels[11], 0);
}

TEST (ColorVectorOpsTests, PremultiplyRGBAFullAlphaUnchanged)
{
    uint8 pixels[] = { 100, 150, 200, 255 };
    ColorVectorOperations::premultiplyRGBA (pixels, 1);
    EXPECT_EQ (pixels[0], 100);
    EXPECT_EQ (pixels[1], 150);
    EXPECT_EQ (pixels[2], 200);
    EXPECT_EQ (pixels[3], 255);
}

TEST (ColorVectorOpsTests, PremultiplyRGBAZeroAlphaBlacksOut)
{
    uint8 pixels[] = { 100, 150, 200, 0 };
    ColorVectorOperations::premultiplyRGBA (pixels, 1);
    EXPECT_EQ (pixels[0], 0);
    EXPECT_EQ (pixels[1], 0);
    EXPECT_EQ (pixels[2], 0);
    EXPECT_EQ (pixels[3], 0);
}

TEST (ColorVectorOpsTests, PremultiplyRGBAZeroPixels)
{
    uint8 pixels[] = { 99, 98, 97, 96 };
    ColorVectorOperations::premultiplyRGBA (pixels, 0);
    EXPECT_EQ (pixels[0], 99);
}

// ==============================================================================
// convertARGBtoRGBA tests
// ==============================================================================

TEST (ColorVectorOpsTests, ConvertARGBtoRGBA)
{
    const uint32_t argb[] = { 0x12345678u, 0xaabbccddu };
    uint32_t rgba[2] = {};

    ColorVectorOperations::convertARGBtoRGBA (argb, rgba, 2);

    EXPECT_EQ (rgba[0], 0x34567812u);
    EXPECT_EQ (rgba[1], 0xbbccddaau);
}

TEST (ColorVectorOpsTests, ConvertARGBtoRGBAInPlace)
{
    uint32_t pixels[] = { 0x12345678u, 0xaabbccddu };

    ColorVectorOperations::convertARGBtoRGBA (pixels, pixels, 2);

    EXPECT_EQ (pixels[0], 0x34567812u);
    EXPECT_EQ (pixels[1], 0xbbccddaau);
}

TEST (ColorVectorOpsTests, ConvertARGBtoRGBAZeroPixels)
{
    const uint32_t argb[] = { 0x12345678u };
    uint32_t rgba[] = { 0xdeadbeefu };
    ColorVectorOperations::convertARGBtoRGBA (argb, rgba, 0);
    EXPECT_EQ (rgba[0], 0xdeadbeefu);
}

TEST (ColorVectorOpsTests, ConvertARGBtoRGBASinglePixel)
{
    // 0xAARRGGBB -> 0xRRGGBBAA
    const uint32_t argb[] = { 0xaabbccddu };
    uint32_t rgba[] = { 0u };
    ColorVectorOperations::convertARGBtoRGBA (argb, rgba, 1);
    EXPECT_EQ (rgba[0], 0xbbccddaau);
}

TEST (ColorVectorOpsTests, ConvertARGBtoRGBAAllZero)
{
    const uint32_t argb[] = { 0x00000000u };
    uint32_t rgba[] = { 0xffffffffu };
    ColorVectorOperations::convertARGBtoRGBA (argb, rgba, 1);
    EXPECT_EQ (rgba[0], 0x00000000u);
}

TEST (ColorVectorOpsTests, ConvertARGBtoRGBAAllOnes)
{
    const uint32_t argb[] = { 0xffffffffu };
    uint32_t rgba[] = { 0u };
    ColorVectorOperations::convertARGBtoRGBA (argb, rgba, 1);
    EXPECT_EQ (rgba[0], 0xffffffffu);
}

// ==============================================================================
// convertGrayscaleToRGBA tests
// ==============================================================================

TEST (ColorVectorOpsTests, ConvertGrayscaleToRGBA)
{
    const uint8 gray[] = { 0, 127, 255 };
    uint8 rgba[12] = {};

    ColorVectorOperations::convertGrayscaleToRGBA (gray, rgba, 3);

    EXPECT_EQ (rgba[0], 0);
    EXPECT_EQ (rgba[1], 0);
    EXPECT_EQ (rgba[2], 0);
    EXPECT_EQ (rgba[3], 255);
    EXPECT_EQ (rgba[4], 127);
    EXPECT_EQ (rgba[5], 127);
    EXPECT_EQ (rgba[6], 127);
    EXPECT_EQ (rgba[7], 255);
    EXPECT_EQ (rgba[8], 255);
    EXPECT_EQ (rgba[9], 255);
    EXPECT_EQ (rgba[10], 255);
    EXPECT_EQ (rgba[11], 255);
}

TEST (ColorVectorOpsTests, ConvertGrayscaleToRGBAAlwaysOpaqueAlpha)
{
    const uint8 gray[] = { 128 };
    uint8 rgba[4] = {};
    ColorVectorOperations::convertGrayscaleToRGBA (gray, rgba, 1);
    EXPECT_EQ (rgba[0], 128);
    EXPECT_EQ (rgba[1], 128);
    EXPECT_EQ (rgba[2], 128);
    EXPECT_EQ (rgba[3], 255);
}

TEST (ColorVectorOpsTests, ConvertGrayscaleToRGBAZeroPixels)
{
    const uint8 gray[] = { 99 };
    uint8 rgba[4] = { 1, 2, 3, 4 };
    ColorVectorOperations::convertGrayscaleToRGBA (gray, rgba, 0);
    EXPECT_EQ (rgba[0], 1);
}

// ==============================================================================
// convertRGBToRGBA tests
// ==============================================================================

TEST (ColorVectorOpsTests, ConvertRGBToRGBA)
{
    const uint8 rgb[] = { 1, 2, 3, 4, 5, 6 };
    uint8 rgba[8] = {};

    ColorVectorOperations::convertRGBToRGBA (rgb, rgba, 2);

    EXPECT_EQ (rgba[0], 1);
    EXPECT_EQ (rgba[1], 2);
    EXPECT_EQ (rgba[2], 3);
    EXPECT_EQ (rgba[3], 255);
    EXPECT_EQ (rgba[4], 4);
    EXPECT_EQ (rgba[5], 5);
    EXPECT_EQ (rgba[6], 6);
    EXPECT_EQ (rgba[7], 255);
}

TEST (ColorVectorOpsTests, ConvertRGBToRGBAAlwaysOpaqueAlpha)
{
    const uint8 rgb[] = { 10, 20, 30 };
    uint8 rgba[4] = {};
    ColorVectorOperations::convertRGBToRGBA (rgb, rgba, 1);
    EXPECT_EQ (rgba[0], 10);
    EXPECT_EQ (rgba[1], 20);
    EXPECT_EQ (rgba[2], 30);
    EXPECT_EQ (rgba[3], 255);
}

TEST (ColorVectorOpsTests, ConvertRGBToRGBAZeroPixels)
{
    const uint8 rgb[] = { 99, 98, 97 };
    uint8 rgba[4] = { 1, 2, 3, 4 };
    ColorVectorOperations::convertRGBToRGBA (rgb, rgba, 0);
    EXPECT_EQ (rgba[0], 1);
}

// ==============================================================================
// lerpRows tests
// ==============================================================================

TEST (ColorVectorOpsTests, LerpRowsMatchesScalarReference)
{
    const float rowA[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 0.5f, 0.0f, 1.0f };
    const float rowB[] = { 1.0f, 0.75f, 0.5f, 0.25f, 0.0f, 0.25f, 0.5f, 0.75f };
    float dst[8] = {};

    ColorVectorOperations::lerpRows (rowA, rowB, dst, 0.25f, 2);

    for (int i = 0; i < 8; ++i)
        EXPECT_NEAR (dst[i], rowA[i] + (rowB[i] - rowA[i]) * 0.25f, 1.0e-5f);
}

TEST (ColorVectorOpsTests, LerpRowsAtZeroGivesRowA)
{
    const float rowA[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float rowB[] = { 5.0f, 6.0f, 7.0f, 8.0f };
    float dst[4] = {};

    ColorVectorOperations::lerpRows (rowA, rowB, dst, 0.0f, 1);

    for (int i = 0; i < 4; ++i)
        EXPECT_NEAR (dst[i], rowA[i], 1.0e-5f);
}

TEST (ColorVectorOpsTests, LerpRowsAtOneGivesRowB)
{
    const float rowA[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float rowB[] = { 5.0f, 6.0f, 7.0f, 8.0f };
    float dst[4] = {};

    ColorVectorOperations::lerpRows (rowA, rowB, dst, 1.0f, 1);

    for (int i = 0; i < 4; ++i)
        EXPECT_NEAR (dst[i], rowB[i], 1.0e-5f);
}

TEST (ColorVectorOpsTests, LerpRowsAtHalfIsMidpoint)
{
    const float rowA[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float rowB[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float dst[4] = {};

    ColorVectorOperations::lerpRows (rowA, rowB, dst, 0.5f, 1);

    for (int i = 0; i < 4; ++i)
        EXPECT_NEAR (dst[i], 0.5f, 1.0e-5f);
}

TEST (ColorVectorOpsTests, LerpRowsZeroPixels)
{
    const float rowA[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    const float rowB[] = { 2.0f, 2.0f, 2.0f, 2.0f };
    float dst[4] = { 9.0f, 9.0f, 9.0f, 9.0f };

    ColorVectorOperations::lerpRows (rowA, rowB, dst, 0.5f, 0);

    for (int i = 0; i < 4; ++i)
        EXPECT_FLOAT_EQ (dst[i], 9.0f);
}

TEST (ColorVectorOpsTests, LerpRowsMultiplePixels)
{
    // 4 pixels = 16 floats
    float rowA[16], rowB[16], dst[16];
    for (int i = 0; i < 16; ++i)
    {
        rowA[i] = (float) i;
        rowB[i] = (float) (i + 16);
    }

    ColorVectorOperations::lerpRows (rowA, rowB, dst, 0.5f, 4);

    for (int i = 0; i < 16; ++i)
        EXPECT_NEAR (dst[i], rowA[i] + (rowB[i] - rowA[i]) * 0.5f, 1.0e-4f);
}

// ==============================================================================
// convertBGRAtoRGBA tests
// ==============================================================================

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBASwapsRedAndBlue)
{
    uint8 pixels[] = { 10, 20, 30, 40, 50, 60, 70, 80 };
    // After swap:  R↔B, G and A unchanged
    // pixel 0: {30, 20, 10, 40}
    // pixel 1: {70, 60, 50, 80}

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 2);

    EXPECT_EQ (pixels[0], 30);
    EXPECT_EQ (pixels[1], 20);
    EXPECT_EQ (pixels[2], 10);
    EXPECT_EQ (pixels[3], 40);
    EXPECT_EQ (pixels[4], 70);
    EXPECT_EQ (pixels[5], 60);
    EXPECT_EQ (pixels[6], 50);
    EXPECT_EQ (pixels[7], 80);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBASinglePixel)
{
    uint8 pixels[] = { 1, 2, 3, 255 };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 1);

    EXPECT_EQ (pixels[0], 3);
    EXPECT_EQ (pixels[1], 2);
    EXPECT_EQ (pixels[2], 1);
    EXPECT_EQ (pixels[3], 255);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAAllChannelsEqual)
{
    // When R == B, the swap is a no-op.
    uint8 pixels[] = { 100, 200, 100, 255 };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 1);

    EXPECT_EQ (pixels[0], 100);
    EXPECT_EQ (pixels[1], 200);
    EXPECT_EQ (pixels[2], 100);
    EXPECT_EQ (pixels[3], 255);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAAllZero)
{
    uint8 pixels[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 2);

    EXPECT_EQ (pixels[0], 0);
    EXPECT_EQ (pixels[1], 0);
    EXPECT_EQ (pixels[2], 0);
    EXPECT_EQ (pixels[3], 0);
    EXPECT_EQ (pixels[4], 0);
    EXPECT_EQ (pixels[5], 0);
    EXPECT_EQ (pixels[6], 0);
    EXPECT_EQ (pixels[7], 0);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAAllOnes)
{
    uint8 pixels[] = { 255, 255, 255, 255, 255, 255, 255, 255 };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 2);

    for (int i = 0; i < 8; ++i)
        EXPECT_EQ (pixels[i], 255);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAZeroPixels)
{
    uint8 pixels[] = { 99, 98, 97, 96 };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 0);

    EXPECT_EQ (pixels[0], 99);
    EXPECT_EQ (pixels[1], 98);
    EXPECT_EQ (pixels[2], 97);
    EXPECT_EQ (pixels[3], 96);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBANegativeCount)
{
    uint8 pixels[] = { 1, 2, 3, 4 };

    // Should no-op gracefully.
    ColorVectorOperations::convertBGRAtoRGBA (pixels, -1);

    EXPECT_EQ (pixels[0], 1);
    EXPECT_EQ (pixels[1], 2);
    EXPECT_EQ (pixels[2], 3);
    EXPECT_EQ (pixels[3], 4);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBALargePixelCount)
{
    // Exercise SIMD path with 100 pixels (400 bytes) — well past any
    // scalar tail boundary.
    constexpr int kCount = 100;
    uint8 pixels[kCount * 4];
    for (int i = 0; i < kCount; ++i)
    {
        pixels[i * 4 + 0] = (uint8) (i & 0xff);       // R = i
        pixels[i * 4 + 1] = (uint8) ((i * 2) & 0xff); // G
        pixels[i * 4 + 2] = (uint8) ((i * 3) & 0xff); // B
        pixels[i * 4 + 3] = (uint8) ((i * 5) & 0xff); // A
    }

    ColorVectorOperations::convertBGRAtoRGBA (pixels, kCount);

    for (int i = 0; i < kCount; ++i)
    {
        EXPECT_EQ (pixels[i * 4 + 0], (uint8) ((i * 3) & 0xff)); // was B
        EXPECT_EQ (pixels[i * 4 + 1], (uint8) ((i * 2) & 0xff)); // G unchanged
        EXPECT_EQ (pixels[i * 4 + 2], (uint8) (i & 0xff));       // was R
        EXPECT_EQ (pixels[i * 4 + 3], (uint8) ((i * 5) & 0xff)); // A unchanged
    }
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAOddPixelCount)
{
    // 5 pixels = 20 bytes — not a multiple of 4 (SIMD) pixels.
    // The scalar tail path handles the last pixel.
    uint8 pixels[] = {
        1, 2, 10, 255, // R=1, G=2, B=10, A=255
        3,
        4,
        20,
        128, // R=3, G=4, B=20, A=128
        5,
        6,
        30,
        64, // R=5, G=6, B=30, A=64
        7,
        8,
        40,
        32, // R=7, G=8, B=40, A=32
        9,
        11,
        50,
        16 // R=9, G=11, B=50, A=16
    };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 5);

    EXPECT_EQ (pixels[0], 10); // was B of pixel 0
    EXPECT_EQ (pixels[1], 2);
    EXPECT_EQ (pixels[2], 1); // was R of pixel 0
    EXPECT_EQ (pixels[3], 255);

    EXPECT_EQ (pixels[4], 20);
    EXPECT_EQ (pixels[5], 4);
    EXPECT_EQ (pixels[6], 3);
    EXPECT_EQ (pixels[7], 128);

    EXPECT_EQ (pixels[8], 30);
    EXPECT_EQ (pixels[9], 6);
    EXPECT_EQ (pixels[10], 5);
    EXPECT_EQ (pixels[11], 64);

    EXPECT_EQ (pixels[12], 40);
    EXPECT_EQ (pixels[13], 8);
    EXPECT_EQ (pixels[14], 7);
    EXPECT_EQ (pixels[15], 32);

    EXPECT_EQ (pixels[16], 50);
    EXPECT_EQ (pixels[17], 11);
    EXPECT_EQ (pixels[18], 9);
    EXPECT_EQ (pixels[19], 16);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGRADoubleSwapIsIdentity)
{
    // Two swaps should restore the original values.
    uint8 pixels[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120 };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 3);
    ColorVectorOperations::convertBGRAtoRGBA (pixels, 3);

    EXPECT_EQ (pixels[0], 10);
    EXPECT_EQ (pixels[1], 20);
    EXPECT_EQ (pixels[2], 30);
    EXPECT_EQ (pixels[3], 40);
    EXPECT_EQ (pixels[8], 90);
    EXPECT_EQ (pixels[9], 100);
    EXPECT_EQ (pixels[10], 110);
    EXPECT_EQ (pixels[11], 120);
}
