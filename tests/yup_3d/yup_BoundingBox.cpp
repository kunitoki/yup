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

#include <yup_3d/yup_3d.h>

using namespace yup;

class BoundingBoxTests : public ::testing::Test
{
protected:
    static void expectNear (const Vector3<float>& actual, const Vector3<float>& expected, float tolerance = 1.0e-5f)
    {
        EXPECT_NEAR (actual.getX(), expected.getX(), tolerance);
        EXPECT_NEAR (actual.getY(), expected.getY(), tolerance);
        EXPECT_NEAR (actual.getZ(), expected.getZ(), tolerance);
    }
};

TEST_F (BoundingBoxTests, DefaultIsEmpty)
{
    const BoundingBox box;

    EXPECT_TRUE (box.isEmpty());
    expectNear (box.getCenter(), {});
    expectNear (box.getSize(), {});
    EXPECT_FLOAT_EQ (box.getRadius(), 0.0f);
    EXPECT_TRUE (box.transformedBy (Matrix4::scaling (2.0f)).isEmpty());
}

TEST_F (BoundingBoxTests, ExpandByPointsGrowsToContainThem)
{
    BoundingBox box;
    box.expand ({ 1.0f, 2.0f, 3.0f });

    EXPECT_FALSE (box.isEmpty());
    expectNear (box.getSize(), {});

    box.expand ({ -1.0f, 4.0f, 0.0f });

    expectNear (box.getMin(), { -1.0f, 2.0f, 0.0f });
    expectNear (box.getMax(), { 1.0f, 4.0f, 3.0f });
}

TEST_F (BoundingBoxTests, ExpandByBoxIgnoresEmptyBoxes)
{
    BoundingBox box ({ 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
    box.expand (BoundingBox());

    expectNear (box.getMin(), { 0.0f, 0.0f, 0.0f });
    expectNear (box.getMax(), { 1.0f, 1.0f, 1.0f });

    box.expand (BoundingBox ({ 2.0f, -1.0f, 0.5f }, { 3.0f, 0.0f, 0.5f }));

    expectNear (box.getMin(), { 0.0f, -1.0f, 0.0f });
    expectNear (box.getMax(), { 3.0f, 1.0f, 1.0f });
}

TEST_F (BoundingBoxTests, CenterSizeAndRadius)
{
    const BoundingBox box ({ -1.0f, 0.0f, 2.0f }, { 3.0f, 4.0f, 6.0f });

    expectNear (box.getCenter(), { 1.0f, 2.0f, 4.0f });
    expectNear (box.getSize(), { 4.0f, 4.0f, 4.0f });
    EXPECT_NEAR (box.getRadius(), std::sqrt (48.0f) * 0.5f, 1.0e-5f);
}

TEST_F (BoundingBoxTests, TransformedByNonUniformScaleAndTranslation)
{
    const BoundingBox box ({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f });
    const auto transformed = box.transformedBy (Matrix4::scaling ({ 2.0f, 3.0f, 4.0f }).followedBy (Matrix4::translation ({ 10.0f, 0.0f, 0.0f })));

    expectNear (transformed.getMin(), { 8.0f, -3.0f, -4.0f });
    expectNear (transformed.getMax(), { 12.0f, 3.0f, 4.0f });
}

TEST_F (BoundingBoxTests, TransformedByRotationContainsAllCorners)
{
    const BoundingBox box ({ 0.0f, 0.0f, 0.0f }, { 2.0f, 1.0f, 1.0f });
    const auto transformed = box.transformedBy (Matrix4::rotationZ (MathConstants<float>::halfPi));

    // (x, y) -> (-y, x)
    expectNear (transformed.getMin(), { -1.0f, 0.0f, 0.0f });
    expectNear (transformed.getMax(), { 0.0f, 2.0f, 1.0f });
}
