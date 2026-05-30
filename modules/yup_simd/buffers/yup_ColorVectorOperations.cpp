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
    for (int i = 0; i < numPixels; ++i)
    {
        const uint32 pixel = pixels[i];
        const uint32 alpha = (pixel >> 24) & 0xffu;
        const uint32 red = premultiplyComponent ((pixel >> 16) & 0xffu, alpha);
        const uint32 green = premultiplyComponent ((pixel >> 8) & 0xffu, alpha);
        const uint32 blue = premultiplyComponent (pixel & 0xffu, alpha);

        pixels[i] = (alpha << 24) | (red << 16) | (green << 8) | blue;
    }
}

void YUP_CALLTYPE ColorVectorOperations::premultiplyRGBA (uint8* pixels, int numPixels) noexcept
{
    auto* pixel = pixels;

    for (int i = 0; i < numPixels; ++i)
    {
        const uint32 alpha = pixel[3];

        pixel[0] = static_cast<uint8> (premultiplyComponent (pixel[0], alpha));
        pixel[1] = static_cast<uint8> (premultiplyComponent (pixel[1], alpha));
        pixel[2] = static_cast<uint8> (premultiplyComponent (pixel[2], alpha));
        pixel += 4;
    }
}

void YUP_CALLTYPE ColorVectorOperations::convertARGBtoRGBA (const uint32* src, uint32* dst, int numPixels) noexcept
{
    for (int i = 0; i < numPixels; ++i)
    {
        const uint32 pixel = src[i];
        dst[i] = ((pixel & 0x00ffffffu) << 8) | ((pixel >> 24) & 0xffu);
    }
}

void YUP_CALLTYPE ColorVectorOperations::convertGrayscaleToRGBA (const uint8* src, uint8* dst, int numPixels) noexcept
{
    for (int i = 0; i < numPixels; ++i)
    {
        const uint32 value = *src++;
        const auto rgba = packRGBA (value, value, value, 255u);
        std::memcpy (dst, &rgba, sizeof (rgba));
        dst += 4;
    }
}

void YUP_CALLTYPE ColorVectorOperations::convertRGBToRGBA (const uint8* src, uint8* dst, int numPixels) noexcept
{
    for (int i = 0; i < numPixels; ++i)
    {
        const auto rgba = packRGBA (src[0], src[1], src[2], 255u);
        std::memcpy (dst, &rgba, sizeof (rgba));
        src += 3;
        dst += 4;
    }
}

void YUP_CALLTYPE ColorVectorOperations::lerpRows (const float* rowA, const float* rowB, float* dst, float t, int numPixels) noexcept
{
    const auto t4 = Float4::broadcast (t);
    const auto minusOne = Float4::broadcast (-1.0f);
    int i = 0;

    for (; i < numPixels; ++i)
    {
        const auto a = Float4::loadUnaligned (rowA + i * 4);
        const auto b = Float4::loadUnaligned (rowB + i * 4);
        a.mulAdd (b + (a * minusOne), t4).storeUnaligned (dst + i * 4);
    }
}

} // namespace yup
