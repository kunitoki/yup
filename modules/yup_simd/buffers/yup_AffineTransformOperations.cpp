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

void YUP_CALLTYPE AffineTransformOperations::transformPoints (float* xs, float* ys, int numPoints, float sx, float shx, float tx, float shy, float sy, float ty) noexcept
{
    transformPoints (xs, ys, xs, ys, numPoints, sx, shx, tx, shy, sy, ty);
}

void YUP_CALLTYPE AffineTransformOperations::transformPoints (const float* srcXs, const float* srcYs, float* dstXs, float* dstYs, int numPoints, float sx, float shx, float tx, float shy, float sy, float ty) noexcept
{
    int i = 0;

    const auto sx4 = Float4::broadcast (sx);
    const auto shx4 = Float4::broadcast (shx);
    const auto tx4 = Float4::broadcast (tx);
    const auto shy4 = Float4::broadcast (shy);
    const auto sy4 = Float4::broadcast (sy);
    const auto ty4 = Float4::broadcast (ty);

    for (; i + Float4::size <= numPoints; i += Float4::size)
    {
        const auto x = Float4::loadUnaligned (srcXs + i);
        const auto y = Float4::loadUnaligned (srcYs + i);

        const auto outX = tx4.mulAdd (sx4, x).mulAdd (shx4, y);
        const auto outY = ty4.mulAdd (shy4, x).mulAdd (sy4, y);

        outX.storeUnaligned (dstXs + i);
        outY.storeUnaligned (dstYs + i);
    }

    for (; i < numPoints; ++i)
    {
        const float x = srcXs[i];
        const float y = srcYs[i];

        dstXs[i] = sx * x + shx * y + tx;
        dstYs[i] = shy * x + sy * y + ty;
    }
}

} // namespace yup
