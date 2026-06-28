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

#pragma once

#include <yup_graphics/yup_graphics.h>

using namespace yup;

/** Generates a checkerboard test image.
    Even squares are red (ARGB: 0xFFFF0000), odd squares are blue (ARGB: 0xFF0000FF).
    Square size is 4 pixels. Works for any PixelFormat.
*/
inline Image generateTestImage (int w, int h, PixelFormat fmt)
{
    Image img (w, h, fmt);

    const bool useAlpha = (fmt == PixelFormat::RGBA);
    const uint32 even = useAlpha ? 0x80FF0000u : 0xFFFF0000u;
    const uint32 odd = useAlpha ? 0x800000FFu : 0xFF0000FFu;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            bool isEven = ((x / 4) + (y / 4)) % 2 == 0;
            img.setPixel (x, y, isEven ? even : odd);
        }
    }

    return img;
}

/** Compares two images pixel-by-pixel (RGB channels only) within the given per-channel tolerance. */
inline bool imagesAreEqual (const Image& a, const Image& b, int tolerance = 0)
{
    if (a.getWidth() != b.getWidth() || a.getHeight() != b.getHeight())
        return false;

    if (a.getPixelFormat() != b.getPixelFormat())
        return false;

    for (int y = 0; y < a.getHeight(); ++y)
    {
        for (int x = 0; x < a.getWidth(); ++x)
        {
            uint32 pa = a.getPixel (x, y);
            uint32 pb = b.getPixel (x, y);

            if (std::abs (int ((pa >> 16) & 0xFF) - int ((pb >> 16) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pa >> 8) & 0xFF) - int ((pb >> 8) & 0xFF)) > tolerance)
                return false;
            if (std::abs (int ((pa >> 0) & 0xFF) - int ((pb >> 0) & 0xFF)) > tolerance)
                return false;
        }
    }

    return true;
}
