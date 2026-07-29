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

namespace
{
constexpr uint32_t makeRGBA (uint8_t r, uint8_t g, uint8_t b, uint8_t a) noexcept
{
    return static_cast<uint32_t> (r)
         | (static_cast<uint32_t> (g) << 8)
         | (static_cast<uint32_t> (b) << 16)
         | (static_cast<uint32_t> (a) << 24);
}

constexpr uint32_t makeBGRA (uint8_t b, uint8_t g, uint8_t r, uint8_t a) noexcept
{
    return static_cast<uint32_t> (b)
         | (static_cast<uint32_t> (g) << 8)
         | (static_cast<uint32_t> (r) << 16)
         | (static_cast<uint32_t> (a) << 24);
}
} // namespace

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

TEST (ColorVectorOpsTests, PremultiplyARGBLargePixelCount)
{
    // Exercise SIMD path with 100 pixels — well past the scalar tail boundary.
    constexpr int kCount = 100;
    uint32_t pixels[kCount];
    for (int i = 0; i < kCount; ++i)
    {
        const uint32_t r = (uint32_t) ((i * 3) & 0xff);
        const uint32_t g = (uint32_t) ((i * 5) & 0xff);
        const uint32_t b = (uint32_t) ((i * 7) & 0xff);
        const uint32_t a = (uint32_t) ((i * 11) & 0xff);
        pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }

    ColorVectorOperations::premultiplyARGB (pixels, kCount);

    for (int i = 0; i < kCount; ++i)
    {
        const uint32_t a = (uint32_t) ((i * 11) & 0xff);
        const uint32_t expectedR = ((uint32_t) ((i * 3) & 0xff) * a + 127u) / 255u;
        const uint32_t expectedG = ((uint32_t) ((i * 5) & 0xff) * a + 127u) / 255u;
        const uint32_t expectedB = ((uint32_t) ((i * 7) & 0xff) * a + 127u) / 255u;

        EXPECT_EQ ((pixels[i] >> 24) & 0xffu, a);
        EXPECT_EQ ((pixels[i] >> 16) & 0xffu, expectedR);
        EXPECT_EQ ((pixels[i] >> 8) & 0xffu, expectedG);
        EXPECT_EQ (pixels[i] & 0xffu, expectedB);
    }
}

TEST (ColorVectorOpsTests, PremultiplyARGBOddPixelCount)
{
    // 5 pixels — not a multiple of 4, exercises both SIMD and scalar tail.
    uint32_t pixels[] = {
        (0x80u << 24) | (0xffu << 16) | (0x80u << 8) | 0x80u,
        (0x40u << 24) | (0x40u << 16) | (0x40u << 8) | 0x40u,
        (0xffu << 24) | (0x10u << 16) | (0x20u << 8) | 0x30u,
        (0x00u << 24) | (0xaau << 16) | (0xbbu << 8) | 0xccu,
        (0xc0u << 24) | (0x22u << 16) | (0x44u << 8) | 0x66u
    };

    ColorVectorOperations::premultiplyARGB (pixels, 5);

    for (int i = 0; i < 5; ++i)
    {
        const uint32_t a = (pixels[i] >> 24) & 0xffu;
        const uint32_t r = (pixels[i] >> 16) & 0xffu;
        const uint32_t g = (pixels[i] >> 8) & 0xffu;
        const uint32_t b = pixels[i] & 0xffu;

        // Verify premultiplied channel ≤ min(original channel, alpha)
        EXPECT_LE (r, a);
        EXPECT_LE (g, a);
        EXPECT_LE (b, a);
    }
}

// ==============================================================================
// premultiplyRGBA tests
// ==============================================================================

TEST (ColorVectorOpsTests, PremultiplyRGBAMatchesScalarReference)
{
    // RGBA pixels as uint32: [R,G,B,A] with premultiply
    uint32_t pixels[] = {
        makeRGBA (255, 128, 64, 128),
        makeRGBA (1, 2, 3, 255),
        makeRGBA (1, 2, 3, 0)
    };

    ColorVectorOperations::premultiplyRGBA (pixels, 3);

    EXPECT_EQ (pixels[0], makeRGBA (128, 64, 32, 128));
    EXPECT_EQ (pixels[1], makeRGBA (1, 2, 3, 255));
    EXPECT_EQ (pixels[2], 0x00000000u);
}

TEST (ColorVectorOpsTests, PremultiplyRGBAFullAlphaUnchanged)
{
    uint32_t pixels[] = { makeRGBA (100, 150, 200, 255) };
    ColorVectorOperations::premultiplyRGBA (pixels, 1);
    EXPECT_EQ (pixels[0], makeRGBA (100, 150, 200, 255));
}

TEST (ColorVectorOpsTests, PremultiplyRGBAZeroAlphaBlacksOut)
{
    uint32_t pixels[] = { makeRGBA (100, 150, 200, 0) };
    ColorVectorOperations::premultiplyRGBA (pixels, 1);
    EXPECT_EQ (pixels[0], 0x00000000u);
}

TEST (ColorVectorOpsTests, PremultiplyRGBAZeroPixels)
{
    uint32_t pixels[] = { makeRGBA (99, 98, 97, 96) };
    ColorVectorOperations::premultiplyRGBA (pixels, 0);
    EXPECT_EQ (pixels[0], makeRGBA (99, 98, 97, 96));
}

TEST (ColorVectorOpsTests, PremultiplyRGBALargePixelCount)
{
    // Exercise SIMD path with 100 pixels — well past the scalar tail boundary.
    constexpr int kCount = 100;
    uint32_t pixels[kCount];
    for (int i = 0; i < kCount; ++i)
    {
        const uint8_t r = (uint8_t) ((i * 3) & 0xff);
        const uint8_t g = (uint8_t) ((i * 5) & 0xff);
        const uint8_t b = (uint8_t) ((i * 7) & 0xff);
        const uint8_t a = (uint8_t) ((i * 11) & 0xff);
        pixels[i] = makeRGBA (r, g, b, a);
    }

    ColorVectorOperations::premultiplyRGBA (pixels, kCount);

    for (int i = 0; i < kCount; ++i)
    {
        const uint32_t a = (uint32_t) ((i * 11) & 0xff);
        const uint32_t expectedR = ((uint32_t) ((i * 3) & 0xff) * a + 127u) / 255u;
        const uint32_t expectedG = ((uint32_t) ((i * 5) & 0xff) * a + 127u) / 255u;
        const uint32_t expectedB = ((uint32_t) ((i * 7) & 0xff) * a + 127u) / 255u;

        EXPECT_EQ ((pixels[i] >> 24) & 0xffu, a);
        EXPECT_EQ (pixels[i] & 0xffu, expectedR);
        EXPECT_EQ ((pixels[i] >> 8) & 0xffu, expectedG);
        EXPECT_EQ ((pixels[i] >> 16) & 0xffu, expectedB);
    }
}

TEST (ColorVectorOpsTests, PremultiplyRGBAOddPixelCount)
{
    // 5 pixels — not a multiple of 4, exercises both SIMD and scalar tail.
    uint32_t pixels[] = {
        makeRGBA (255, 128, 64, 128),
        makeRGBA (64, 64, 64, 64),
        makeRGBA (16, 32, 48, 255),
        makeRGBA (170, 187, 204, 0),
        makeRGBA (34, 68, 102, 192)
    };

    ColorVectorOperations::premultiplyRGBA (pixels, 5);

    for (int i = 0; i < 5; ++i)
    {
        const uint32_t a = (pixels[i] >> 24) & 0xffu;
        const uint32_t r = pixels[i] & 0xffu;
        const uint32_t g = (pixels[i] >> 8) & 0xffu;
        const uint32_t b = (pixels[i] >> 16) & 0xffu;

        // Verify premultiplied channel ≤ min(original channel, alpha)
        EXPECT_LE (r, a);
        EXPECT_LE (g, a);
        EXPECT_LE (b, a);
    }
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

TEST (ColorVectorOpsTests, ConvertARGBtoRGBALargePixelCount)
{
    // Exercise SIMD path with 100 pixels — well past the scalar tail boundary.
    constexpr int kCount = 100;
    uint32_t argb[kCount];
    for (int i = 0; i < kCount; ++i)
        argb[i] = (uint32_t) (((i * 7) & 0xff) << 24) | (uint32_t) (((i * 3) & 0xff) << 16) | (uint32_t) (((i * 5) & 0xff) << 8) | (uint32_t) (i & 0xff);

    uint32_t rgba[kCount] = {};
    ColorVectorOperations::convertARGBtoRGBA (argb, rgba, kCount);

    for (int i = 0; i < kCount; ++i)
    {
        // ARGB 0xAARRGGBB -> RGBA 0xRRGGBBAA
        EXPECT_EQ ((rgba[i] >> 24) & 0xffu, (uint32_t) ((i * 3) & 0xff)); // old R -> new R
        EXPECT_EQ ((rgba[i] >> 16) & 0xffu, (uint32_t) ((i * 5) & 0xff)); // old G -> new G
        EXPECT_EQ ((rgba[i] >> 8) & 0xffu, (uint32_t) (i & 0xff));        // old B -> new B
        EXPECT_EQ (rgba[i] & 0xffu, (uint32_t) ((i * 7) & 0xff));         // old A -> new A
    }
}

TEST (ColorVectorOpsTests, ConvertARGBtoRGBAOddPixelCount)
{
    // 5 pixels — not a multiple of 4, exercises both SIMD and scalar tail.
    const uint32_t argb[] = {
        0x12345678u, 0xaabbccddu, 0xffeeddccu, 0x01020304u, 0x99887766u
    };
    uint32_t rgba[5] = {};

    ColorVectorOperations::convertARGBtoRGBA (argb, rgba, 5);

    EXPECT_EQ (rgba[0], 0x34567812u);
    EXPECT_EQ (rgba[1], 0xbbccddaau);
    EXPECT_EQ (rgba[2], 0xeeddccffu);
    EXPECT_EQ (rgba[3], 0x02030401u);
    EXPECT_EQ (rgba[4], 0x88776699u);
}

// ==============================================================================
// convertGrayscaleToRGBA tests
// ==============================================================================

TEST (ColorVectorOpsTests, ConvertGrayscaleToRGBA)
{
    const uint8_t gray[] = { 0, 127, 255 };
    uint32_t rgba[3] = {};

    ColorVectorOperations::convertGrayscaleToRGBA (gray, rgba, 3);

    EXPECT_EQ (rgba[0], makeRGBA (0, 0, 0, 255));
    EXPECT_EQ (rgba[1], makeRGBA (127, 127, 127, 255));
    EXPECT_EQ (rgba[2], makeRGBA (255, 255, 255, 255));
}

TEST (ColorVectorOpsTests, ConvertGrayscaleToRGBAAlwaysOpaqueAlpha)
{
    const uint8_t gray[] = { 128 };
    uint32_t rgba[1] = {};
    ColorVectorOperations::convertGrayscaleToRGBA (gray, rgba, 1);
    EXPECT_EQ (rgba[0], makeRGBA (128, 128, 128, 255));
}

TEST (ColorVectorOpsTests, ConvertGrayscaleToRGBAZeroPixels)
{
    const uint8_t gray[] = { 99 };
    uint32_t rgba[] = { 0x04030201u };
    ColorVectorOperations::convertGrayscaleToRGBA (gray, rgba, 0);
    EXPECT_EQ (rgba[0], 0x04030201u);
}

// ==============================================================================
// convertRGBToRGBA tests
// ==============================================================================

TEST (ColorVectorOpsTests, ConvertRGBToRGBA)
{
    const uint8_t rgb[] = { 1, 2, 3, 4, 5, 6 };
    uint32_t rgba[2] = {};

    ColorVectorOperations::convertRGBToRGBA (rgb, rgba, 2);

    EXPECT_EQ (rgba[0], makeRGBA (1, 2, 3, 255));
    EXPECT_EQ (rgba[1], makeRGBA (4, 5, 6, 255));
}

TEST (ColorVectorOpsTests, ConvertRGBToRGBAAlwaysOpaqueAlpha)
{
    const uint8_t rgb[] = { 10, 20, 30 };
    uint32_t rgba[1] = {};
    ColorVectorOperations::convertRGBToRGBA (rgb, rgba, 1);
    EXPECT_EQ (rgba[0], makeRGBA (10, 20, 30, 255));
}

TEST (ColorVectorOpsTests, ConvertRGBToRGBAZeroPixels)
{
    const uint8_t rgb[] = { 99, 98, 97 };
    uint32_t rgba[] = { 0x04030201u };
    ColorVectorOperations::convertRGBToRGBA (rgb, rgba, 0);
    EXPECT_EQ (rgba[0], 0x04030201u);
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
    // BGRA pixels: bytes [B,G,R,A], swap → RGBA: bytes [R,G,B,A]
    uint32_t pixels[] = {
        makeBGRA (10, 20, 30, 40),
        makeBGRA (50, 60, 70, 80)
    };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 2);

    EXPECT_EQ (pixels[0], makeBGRA (30, 20, 10, 40)); // R↔B
    EXPECT_EQ (pixels[1], makeBGRA (70, 60, 50, 80));
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBASinglePixel)
{
    uint32_t pixels[] = { makeBGRA (1, 2, 3, 255) };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 1);

    EXPECT_EQ (pixels[0], makeBGRA (3, 2, 1, 255));
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAAllChannelsEqual)
{
    // When R == B, the swap is a no-op.
    uint32_t pixels[] = { makeBGRA (100, 200, 100, 255) };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 1);

    EXPECT_EQ (pixels[0], makeBGRA (100, 200, 100, 255));
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAAllZero)
{
    uint32_t pixels[] = { 0x00000000u, 0x00000000u };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 2);

    EXPECT_EQ (pixels[0], 0x00000000u);
    EXPECT_EQ (pixels[1], 0x00000000u);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAAllOnes)
{
    uint32_t pixels[] = { 0xffffffffu, 0xffffffffu };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 2);

    EXPECT_EQ (pixels[0], 0xffffffffu);
    EXPECT_EQ (pixels[1], 0xffffffffu);
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAZeroPixels)
{
    uint32_t pixels[] = { makeBGRA (99, 98, 97, 96) };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 0);

    EXPECT_EQ (pixels[0], makeBGRA (99, 98, 97, 96));
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBANegativeCount)
{
    uint32_t pixels[] = { makeBGRA (1, 2, 3, 4) };

    // Should no-op gracefully.
    ColorVectorOperations::convertBGRAtoRGBA (pixels, -1);

    EXPECT_EQ (pixels[0], makeBGRA (1, 2, 3, 4));
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBALargePixelCount)
{
    // Exercise SIMD path with 100 pixels — well past any scalar tail boundary.
    constexpr int kCount = 100;
    uint32_t pixels[kCount];
    for (int i = 0; i < kCount; ++i)
        pixels[i] = makeBGRA ((uint8_t) (i & 0xff),
                              (uint8_t) ((i * 2) & 0xff),
                              (uint8_t) ((i * 3) & 0xff),
                              (uint8_t) ((i * 5) & 0xff));

    ColorVectorOperations::convertBGRAtoRGBA (pixels, kCount);

    for (int i = 0; i < kCount; ++i)
    {
        const auto p = pixels[i];
        EXPECT_EQ ((p >> 0) & 0xffu, (uint32_t) ((i * 3) & 0xff));  // was B
        EXPECT_EQ ((p >> 8) & 0xffu, (uint32_t) ((i * 2) & 0xff));  // G unchanged
        EXPECT_EQ ((p >> 16) & 0xffu, (uint32_t) (i & 0xff));       // was R
        EXPECT_EQ ((p >> 24) & 0xffu, (uint32_t) ((i * 5) & 0xff)); // A unchanged
    }
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGBAOddPixelCount)
{
    // 5 pixels — not a multiple of 4 (SIMD) pixels.
    // The scalar tail path handles the last pixel.
    uint32_t pixels[] = {
        makeBGRA (1, 2, 10, 255),
        makeBGRA (3, 4, 20, 128),
        makeBGRA (5, 6, 30, 64),
        makeBGRA (7, 8, 40, 32),
        makeBGRA (9, 11, 50, 16)
    };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 5);

    EXPECT_EQ (pixels[0], makeBGRA (10, 2, 1, 255));
    EXPECT_EQ (pixels[1], makeBGRA (20, 4, 3, 128));
    EXPECT_EQ (pixels[2], makeBGRA (30, 6, 5, 64));
    EXPECT_EQ (pixels[3], makeBGRA (40, 8, 7, 32));
    EXPECT_EQ (pixels[4], makeBGRA (50, 11, 9, 16));
}

TEST (ColorVectorOpsTests, ConvertBGRAtoRGRADoubleSwapIsIdentity)
{
    // Two swaps should restore the original values.
    uint32_t pixels[] = {
        makeBGRA (10, 20, 30, 40),
        makeBGRA (50, 60, 70, 80),
        makeBGRA (90, 100, 110, 120)
    };

    ColorVectorOperations::convertBGRAtoRGBA (pixels, 3);
    ColorVectorOperations::convertBGRAtoRGBA (pixels, 3);

    EXPECT_EQ (pixels[0], makeBGRA (10, 20, 30, 40));
    EXPECT_EQ (pixels[1], makeBGRA (50, 60, 70, 80));
    EXPECT_EQ (pixels[2], makeBGRA (90, 100, 110, 120));
}
