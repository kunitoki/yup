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

namespace yup
{

namespace
{

static uint32 premultiplyComponent (uint32 component, uint32 alpha) noexcept
{
    return (component * alpha + 127u) / 255u;
}

static uint32 packRGBA (uint32 red, uint32 green, uint32 blue, uint32 alpha) noexcept
{
    return red | (green << 8) | (blue << 16) | (alpha << 24);
}
} // namespace

//==============================================================================
void YUP_CALLTYPE ColorVectorOperations::premultiplyARGB (uint32* pixels, int numPixels) noexcept
{
    const auto c127 = Uint32x4 (127u);
    const auto c255 = Uint32x4 (255u);
    const auto mask8 = Uint32x4 (0xFFu);

    int i = 0;

    for (; i + 4 <= numPixels; i += 4)
    {
        auto p = Uint32x4::loadUnaligned (pixels + i);
        const auto a = p >> 24;

        auto r = ((p >> 16) & mask8) * a;
        auto g = ((p >> 8) & mask8) * a;
        auto b = (p & mask8) * a;

        r = (r + c127) / c255;
        g = (g + c127) / c255;
        b = (b + c127) / c255;

        ((a << 24) | (r << 16) | (g << 8) | b).storeUnaligned (pixels + i);
    }

    for (; i < numPixels; ++i)
    {
        const uint32 pixel = pixels[i];
        const uint32 alpha = (pixel >> 24) & 0xffu;
        const uint32 red = premultiplyComponent ((pixel >> 16) & 0xffu, alpha);
        const uint32 green = premultiplyComponent ((pixel >> 8) & 0xffu, alpha);
        const uint32 blue = premultiplyComponent (pixel & 0xffu, alpha);

        pixels[i] = (alpha << 24) | (red << 16) | (green << 8) | blue;
    }
}

void YUP_CALLTYPE ColorVectorOperations::premultiplyRGBA (uint32* pixels, int numPixels) noexcept
{
    const auto c127 = Uint32x4 (127u);
    const auto c255 = Uint32x4 (255u);
    const auto mask8 = Uint32x4 (0xFFu);

    int i = 0;

    for (; i + 4 <= numPixels; i += 4)
    {
        auto p = Uint32x4::loadUnaligned (pixels + i);
        const auto a = p >> 24;

        auto r = (p & mask8) * a;
        auto g = ((p >> 8) & mask8) * a;
        auto b = ((p >> 16) & mask8) * a;

        r = (r + c127) / c255;
        g = (g + c127) / c255;
        b = (b + c127) / c255;

        ((a << 24) | (b << 16) | (g << 8) | r).storeUnaligned (pixels + i);
    }

    for (; i < numPixels; ++i)
    {
        const uint32 pixel = pixels[i];
        const uint32 alpha = pixel >> 24;
        const uint32 r = premultiplyComponent (pixel & 0xFFu, alpha);
        const uint32 g = premultiplyComponent ((pixel >> 8) & 0xFFu, alpha);
        const uint32 b = premultiplyComponent ((pixel >> 16) & 0xFFu, alpha);

        pixels[i] = (alpha << 24) | (b << 16) | (g << 8) | r;
    }
}

void YUP_CALLTYPE ColorVectorOperations::convertARGBtoRGBA (const uint32* src, uint32* dst, int numPixels) noexcept
{
    int i = 0;

    for (; i + 4 <= numPixels; i += 4)
    {
        const auto p = Uint32x4::loadUnaligned (src + i);
        ((p << 8) | (p >> 24)).storeUnaligned (dst + i);
    }

    for (; i < numPixels; ++i)
    {
        const uint32 pixel = src[i];
        dst[i] = ((pixel & 0x00ffffffu) << 8) | ((pixel >> 24) & 0xffu);
    }
}

//==============================================================================
void YUP_CALLTYPE ColorVectorOperations::convertBGRAtoRGBA (uint32* pixels, int numPixels) noexcept
{
    if (numPixels <= 0)
        return;

    const auto maskAG = Uint32x4 (0xFF00FF00u);
    const auto maskFF = Uint32x4 (0xFFu);

    const int simdPixels = numPixels & ~3;
    int i = 0;

    for (; i < simdPixels; i += 4)
    {
        const auto p = Uint32x4::loadUnaligned (pixels + i);

        // BGRA (uint32 little-endian): bytes [B,G,R,A]
        // RGBA: bytes [R,G,B,A]
        // Transformation: swap byte 0 (B) ↔ byte 2 (R), keep bytes 1 (G) and 3 (A).
        const auto result = (p & maskAG) | ((p & maskFF) << 16) | ((p >> 16) & maskFF);
        result.storeUnaligned (pixels + i);
    }

    // Scalar tail for remaining < 4 pixels.
    for (; i < numPixels; ++i)
    {
        const auto p = pixels[i];
        pixels[i] = (p & 0xFF00FF00u) | ((p & 0xFFu) << 16) | ((p >> 16) & 0xFFu);
    }
}

void YUP_CALLTYPE ColorVectorOperations::convertGrayscaleToRGBA (const uint8* src, uint32* dst, int numPixels) noexcept
{
    for (int i = 0; i < numPixels; ++i)
    {
        const uint32 value = *src++;
        dst[i] = packRGBA (value, value, value, 255u);
    }
}

void YUP_CALLTYPE ColorVectorOperations::convertRGBToRGBA (const uint8* src, uint32* dst, int numPixels) noexcept
{
    for (int i = 0; i < numPixels; ++i)
    {
        dst[i] = packRGBA (src[0], src[1], src[2], 255u);
        src += 3;
    }
}

void YUP_CALLTYPE ColorVectorOperations::lerpRows (const float* rowA, const float* rowB, float* dst, float t, int numPixels) noexcept
{
    const auto t4 = Float32x4::broadcast (t);
    const auto minusOne = Float32x4::broadcast (-1.0f);
    int i = 0;

    for (; i < numPixels; ++i)
    {
        const auto a = Float32x4::loadUnaligned (rowA + i * 4);
        const auto b = Float32x4::loadUnaligned (rowB + i * 4);
        a.mulAdd (b + (a * minusOne), t4).storeUnaligned (dst + i * 4);
    }
}

} // namespace yup
