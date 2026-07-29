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

namespace yup
{

//==============================================================================
class YUP_API ColorVectorOperations
{
public:
    /** Premultiplies alpha in-place on a row of packed `0xAARRGGBB` pixels. */
    static void YUP_CALLTYPE premultiplyARGB (uint32* pixels, int numPixels) noexcept;

    /** Premultiplies alpha in-place on a row of packed RGBA pixels. */
    static void YUP_CALLTYPE premultiplyRGBA (uint32* pixels, int numPixels) noexcept;

    /** Converts packed `0xAARRGGBB` pixels to packed `0xRRGGBBAA` pixels.

        The source and destination buffers may alias for in-place conversion.
    */
    static void YUP_CALLTYPE convertARGBtoRGBA (const uint32* src, uint32* dst, int numPixels) noexcept;

    /** Swaps the R and B channels of packed BGRA pixels to RGBA in place.

        Processes 4 pixels per iteration using xsimd; falls back to a
        scalar R ↔ B swap loop for the tail.

        The pixel count must accurately reflect the number of pixels in `pixels`.
    */
    static void YUP_CALLTYPE convertBGRAtoRGBA (uint32* pixels, int numPixels) noexcept;

    /** Expands 8-bit grayscale pixels to packed RGBA pixels with opaque alpha. */
    static void YUP_CALLTYPE convertGrayscaleToRGBA (const uint8* src, uint32* dst, int numPixels) noexcept;

    /** Expands RGB byte pixels to packed RGBA pixels with opaque alpha. */
    static void YUP_CALLTYPE convertRGBToRGBA (const uint8* src, uint32* dst, int numPixels) noexcept;

    /** Blends rows of float RGBA pixels using `dst = rowA + (rowB - rowA) * t`. */
    static void YUP_CALLTYPE lerpRows (const float* rowA, const float* rowB, float* dst, float t, int numPixels) noexcept;
};

} // namespace yup
