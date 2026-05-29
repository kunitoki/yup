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

class AffineTransformOpsTests : public ::testing::Test
{
protected:
    static void transformScalar (const float* srcXs, const float* srcYs,
                                  float* dstXs, float* dstYs, int numPoints,
                                  float sx, float shx, float tx, float shy, float sy, float ty) noexcept
    {
        for (int i = 0; i < numPoints; ++i)
        {
            const float x = srcXs[i];
            const float y = srcYs[i];
            dstXs[i] = sx * x + shx * y + tx;
            dstYs[i] = shy * x + sy * y + ty;
        }
    }
};

TEST_F (AffineTransformOpsTests, TransformPointsMatchesScalarReference)
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

TEST_F (AffineTransformOpsTests, TransformPointsInPlace)
{
    constexpr int numPoints = 5;
    float xs[numPoints] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    float ys[numPoints] = { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };

    const float origXs[numPoints] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    const float origYs[numPoints] = { 5.0f, 4.0f, 3.0f, 2.0f, 1.0f };

    constexpr float sx = 2.0f, shx = 0.5f, tx = 1.0f;
    constexpr float shy = -0.5f, sy = 3.0f, ty = -1.0f;

    AffineTransformOperations::transformPoints (xs, ys, numPoints, sx, shx, tx, shy, sy, ty);

    for (int i = 0; i < numPoints; ++i)
    {
        EXPECT_NEAR (xs[i], sx * origXs[i] + shx * origYs[i] + tx, 1.0e-5f);
        EXPECT_NEAR (ys[i], shy * origXs[i] + sy * origYs[i] + ty, 1.0e-5f);
    }
}

TEST_F (AffineTransformOpsTests, IdentityTransformLeavesPointsUnchanged)
{
    constexpr int numPoints = 8;
    const float srcXs[numPoints] = { 1.0f, -2.0f, 3.0f, -4.0f, 5.0f, -6.0f, 7.0f, -8.0f };
    const float srcYs[numPoints] = { 8.0f, -7.0f, 6.0f, -5.0f, 4.0f, -3.0f, 2.0f, -1.0f };
    float dstXs[numPoints] = {};
    float dstYs[numPoints] = {};

    // Identity: sx=1, shx=0, tx=0, shy=0, sy=1, ty=0
    AffineTransformOperations::transformPoints (srcXs, srcYs, dstXs, dstYs, numPoints, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    for (int i = 0; i < numPoints; ++i)
    {
        EXPECT_FLOAT_EQ (dstXs[i], srcXs[i]);
        EXPECT_FLOAT_EQ (dstYs[i], srcYs[i]);
    }
}

TEST_F (AffineTransformOpsTests, PureTranslation)
{
    constexpr int numPoints = 6;
    const float srcXs[numPoints] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    const float srcYs[numPoints] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
    float dstXs[numPoints] = {};
    float dstYs[numPoints] = {};

    constexpr float tx = 100.0f, ty = 200.0f;

    // Pure translation: sx=1, shx=0, shy=0, sy=1
    AffineTransformOperations::transformPoints (srcXs, srcYs, dstXs, dstYs, numPoints, 1.0f, 0.0f, tx, 0.0f, 1.0f, ty);

    for (int i = 0; i < numPoints; ++i)
    {
        EXPECT_FLOAT_EQ (dstXs[i], srcXs[i] + tx);
        EXPECT_FLOAT_EQ (dstYs[i], srcYs[i] + ty);
    }
}

TEST_F (AffineTransformOpsTests, PureScale)
{
    constexpr int numPoints = 8;
    const float srcXs[numPoints] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    const float srcYs[numPoints] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    float dstXs[numPoints] = {};
    float dstYs[numPoints] = {};

    constexpr float sx = 2.0f, sy = -0.5f;

    AffineTransformOperations::transformPoints (srcXs, srcYs, dstXs, dstYs, numPoints, sx, 0.0f, 0.0f, 0.0f, sy, 0.0f);

    for (int i = 0; i < numPoints; ++i)
    {
        EXPECT_FLOAT_EQ (dstXs[i], sx * srcXs[i]);
        EXPECT_FLOAT_EQ (dstYs[i], sy * srcYs[i]);
    }
}

TEST_F (AffineTransformOpsTests, SinglePointTransform)
{
    const float srcX = 3.0f;
    const float srcY = 4.0f;
    float dstX = 0.0f;
    float dstY = 0.0f;

    constexpr float sx = 2.0f, shx = 1.0f, tx = -1.0f;
    constexpr float shy = 0.5f, sy = -1.0f, ty = 3.0f;

    AffineTransformOperations::transformPoints (&srcX, &srcY, &dstX, &dstY, 1, sx, shx, tx, shy, sy, ty);

    EXPECT_NEAR (dstX, sx * srcX + shx * srcY + tx, 1.0e-5f);
    EXPECT_NEAR (dstY, shy * srcX + sy * srcY + ty, 1.0e-5f);
}

TEST_F (AffineTransformOpsTests, ZeroPointsDoesNothing)
{
    float dstXs[4] = { 99.0f, 99.0f, 99.0f, 99.0f };
    float dstYs[4] = { 99.0f, 99.0f, 99.0f, 99.0f };

    // numPoints=0 should not write to output
    AffineTransformOperations::transformPoints (nullptr, nullptr, dstXs, dstYs, 0, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FLOAT_EQ (dstXs[i], 99.0f);
        EXPECT_FLOAT_EQ (dstYs[i], 99.0f);
    }
}

TEST_F (AffineTransformOpsTests, LargeBufferMatchesScalarReference)
{
    constexpr int numPoints = 100;
    float srcXs[numPoints], srcYs[numPoints];
    float dstXs[numPoints], dstYs[numPoints];
    float refXs[numPoints], refYs[numPoints];

    for (int i = 0; i < numPoints; ++i)
    {
        srcXs[i] = (float) (i - 50);
        srcYs[i] = (float) (i * 2 - 100);
    }

    constexpr float sx = 1.5f, shx = 0.3f, tx = 7.0f;
    constexpr float shy = -0.2f, sy = 2.0f, ty = -3.0f;

    AffineTransformOperations::transformPoints (srcXs, srcYs, dstXs, dstYs, numPoints, sx, shx, tx, shy, sy, ty);
    transformScalar (srcXs, srcYs, refXs, refYs, numPoints, sx, shx, tx, shy, sy, ty);

    for (int i = 0; i < numPoints; ++i)
    {
        EXPECT_NEAR (dstXs[i], refXs[i], 1.0e-4f);
        EXPECT_NEAR (dstYs[i], refYs[i], 1.0e-4f);
    }
}

TEST_F (AffineTransformOpsTests, ThreePointsUsesScalarTail)
{
    // 3 points: the SIMD path handles 4 at a time, so all 3 go through scalar tail
    constexpr int numPoints = 3;
    const float srcXs[numPoints] = { 1.0f, 2.0f, 3.0f };
    const float srcYs[numPoints] = { 4.0f, 5.0f, 6.0f };
    float dstXs[numPoints] = {};
    float dstYs[numPoints] = {};
    float refXs[numPoints] = {};
    float refYs[numPoints] = {};

    constexpr float sx = 2.0f, shx = 0.5f, tx = 1.0f;
    constexpr float shy = -0.5f, sy = 3.0f, ty = -1.0f;

    AffineTransformOperations::transformPoints (srcXs, srcYs, dstXs, dstYs, numPoints, sx, shx, tx, shy, sy, ty);
    transformScalar (srcXs, srcYs, refXs, refYs, numPoints, sx, shx, tx, shy, sy, ty);

    for (int i = 0; i < numPoints; ++i)
    {
        EXPECT_NEAR (dstXs[i], refXs[i], 1.0e-5f);
        EXPECT_NEAR (dstYs[i], refYs[i], 1.0e-5f);
    }
}
