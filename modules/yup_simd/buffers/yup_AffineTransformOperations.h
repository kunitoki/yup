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
class YUP_API AffineTransformOperations
{
public:
    /** Transforms points stored in separate x/y arrays in-place. */
    static void YUP_CALLTYPE transformPoints (float* xs, float* ys, int numPoints, float sx, float shx, float tx, float shy, float sy, float ty) noexcept;

    /** Transforms points stored in separate x/y source arrays into destination arrays. */
    static void YUP_CALLTYPE transformPoints (const float* srcXs, const float* srcYs, float* dstXs, float* dstYs, int numPoints, float sx, float shx, float tx, float shy, float sy, float ty) noexcept;
};

} // namespace yup
