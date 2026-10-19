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

class RayTests : public ::testing::Test
{
protected:
    const BoundingBox unitBox { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } };
};

TEST_F (RayTests, DefaultsToTheOriginAlongMinusZ)
{
    const Ray ray;

    EXPECT_FLOAT_EQ (ray.getOrigin().getZ(), 0.0f);
    EXPECT_FLOAT_EQ (ray.getDirection().getZ(), -1.0f);
    EXPECT_FLOAT_EQ (ray.getPointAt (3.0f).getZ(), -3.0f);
}

TEST_F (RayTests, ReturnsWhereItEntersABoxInFront)
{
    const auto distance = Ray ({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, -1.0f }).intersect (unitBox);

    ASSERT_TRUE (distance.has_value());
    EXPECT_FLOAT_EQ (*distance, 4.0f);
}

TEST_F (RayTests, MissesBoxesBesideAndBehindIt)
{
    EXPECT_FALSE (Ray ({ 3.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, -1.0f }).intersect (unitBox).has_value());
    EXPECT_FALSE (Ray ({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 1.0f }).intersect (unitBox).has_value());
    EXPECT_FALSE (Ray ({ 0.0f, 0.0f, 5.0f }, { 1.0f, 0.0f, -1.0f }).intersect (unitBox).has_value());
}

TEST_F (RayTests, NeverHitsAnEmptyBox)
{
    EXPECT_FALSE (Ray().intersect (BoundingBox()).has_value());
}

TEST_F (RayTests, ReturnsTheMinDistanceFromInside)
{
    const Ray ray;

    EXPECT_FLOAT_EQ (ray.intersect (unitBox).value_or (-1.0f), 0.0f);
    EXPECT_FLOAT_EQ (ray.intersect (unitBox, 0.5f).value_or (-1.0f), 0.5f);
    EXPECT_FALSE (ray.intersect (unitBox, 2.0f).has_value());
}

TEST_F (RayTests, RespectsTheMaxDistance)
{
    const Ray ray ({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, -1.0f });

    EXPECT_FALSE (ray.intersect (unitBox, 0.0f, 3.0f).has_value());
    EXPECT_FLOAT_EQ (ray.intersect (unitBox, 0.0f, 4.0f).value_or (-1.0f), 4.0f);
}

TEST_F (RayTests, CountsDistancesInLengthsOfTheDirection)
{
    EXPECT_FLOAT_EQ (Ray ({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, -2.0f }).intersect (unitBox).value_or (-1.0f), 2.0f);
}

TEST_F (RayTests, TransformedRaysKeepTheirDistances)
{
    const Ray ray ({ 10.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, -1.0f });
    const auto world = Matrix4::compose ({ 10.0f, 0.0f, 0.0f }, {}, { 2.0f, 2.0f, 2.0f });

    // The box spans z in [-2, 2] in world space, the ray enters it at z = 2
    const auto local = ray.transformedBy (world.inverted());
    EXPECT_NEAR (local.intersect (unitBox).value_or (-1.0f), 3.0f, 1.0e-5f);
    EXPECT_NEAR (local.getDirection().getZ(), -0.5f, 1.0e-6f);
}
