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

TEST (AffineTransformOpsTests, TransformPointsMatchesScalarReference)
{
    constexpr int numPoints = 6;
    const float srcXs[numPoints] = { -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f };
    const float srcYs[numPoints] = { 3.0f, 2.0f, 1.0f, 0.0f, -1.0f, -2.0f };
    float dstXs[numPoints] = {};
    float dstYs[numPoints] = {};

    constexpr float sx = 2.0f;
    constexpr float shx = -0.5f;
    constexpr float tx = 4.0f;
    constexpr float shy = 0.25f;
    constexpr float sy = -3.0f;
    constexpr float ty = 1.0f;

    AffineTransformOperations::transformPoints (srcXs, srcYs, dstXs, dstYs, numPoints, sx, shx, tx, shy, sy, ty);

    for (int i = 0; i < numPoints; ++i)
    {
        EXPECT_NEAR (dstXs[i], sx * srcXs[i] + shx * srcYs[i] + tx, 1.0e-5f);
        EXPECT_NEAR (dstYs[i], shy * srcXs[i] + sy * srcYs[i] + ty, 1.0e-5f);
    }
}
