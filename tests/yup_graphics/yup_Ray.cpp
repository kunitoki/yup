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

#include <yup_graphics/yup_graphics.h>

using namespace yup;

class RayTests : public ::testing::Test
{
protected:
    // Counter-clockwise when seen from +z
    const Vector3<float> a { -1.0f, -1.0f, 0.0f };
    const Vector3<float> b { 1.0f, -1.0f, 0.0f };
    const Vector3<float> c { 0.0f, 1.0f, 0.0f };

    const Ray downZ { { 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, -1.0f } };
};

TEST_F (RayTests, GetPointAtMovesAlongTheDirection)
{
    EXPECT_EQ (downZ.getPointAt (0.0f), Vector3<float> (0.0f, 0.0f, 5.0f));
    EXPECT_EQ (downZ.getPointAt (2.0f), Vector3<float> (0.0f, 0.0f, 3.0f));
}

TEST_F (RayTests, IntersectsPlaneInFront)
{
    const auto distance = downZ.intersectPlane ({ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 2.0f });
    ASSERT_TRUE (distance.has_value());
    EXPECT_FLOAT_EQ (*distance, 4.0f);
}

TEST_F (RayTests, PlaneBehindTheOriginIsMissed)
{
    EXPECT_FALSE (downZ.intersectPlane ({ 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 1.0f }).has_value());
}

TEST_F (RayTests, ParallelRayMissesPlaneAndTriangle)
{
    const Ray sideways ({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f });

    EXPECT_FALSE (sideways.intersectPlane ({}, { 0.0f, 0.0f, 1.0f }).has_value());
    EXPECT_FALSE (sideways.intersectTriangle (a, b, c, false).has_value());
}

TEST_F (RayTests, HitsFrontFacingTriangleWithBarycentrics)
{
    const auto hit = downZ.intersectTriangle (a, b, c, true);
    ASSERT_TRUE (hit.has_value());

    EXPECT_NEAR (hit->distance, 5.0f, 1.0e-5f);
    EXPECT_NEAR (hit->u, 0.25f, 1.0e-5f);
    EXPECT_NEAR (hit->v, 0.5f, 1.0e-5f);

    const auto point = a * (1.0f - hit->u - hit->v) + b * hit->u + c * hit->v;
    EXPECT_TRUE (point.approximatelyEqualTo (downZ.getPointAt (hit->distance)));
}

TEST_F (RayTests, MissesTriangleOutsideItsEdges)
{
    const Ray offside ({ 5.0f, 5.0f, 5.0f }, { 0.0f, 0.0f, -1.0f });
    EXPECT_FALSE (offside.intersectTriangle (a, b, c, false).has_value());
}

TEST_F (RayTests, TriangleBehindTheOriginIsMissed)
{
    const Ray awayFromTriangle ({ 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 1.0f });
    EXPECT_FALSE (awayFromTriangle.intersectTriangle (a, b, c, false).has_value());
}

TEST_F (RayTests, BackFaceCullingRejectsClockwiseTriangles)
{
    EXPECT_FALSE (downZ.intersectTriangle (a, c, b, true).has_value());

    const auto hit = downZ.intersectTriangle (a, c, b, false);
    ASSERT_TRUE (hit.has_value());
    EXPECT_NEAR (hit->distance, 5.0f, 1.0e-5f);
}

TEST_F (RayTests, FromViewportPointGoesThroughTheDisplayedPoint)
{
    const Rectangle<float> viewport (10.0f, 20.0f, 400.0f, 200.0f);
    const auto view = Matrix4::lookAt ({ 0.0f, 0.0f, 5.0f }, {}, { 0.0f, 1.0f, 0.0f });

    for (const bool depthZeroToOne : { true, false })
    {
        const auto viewProjection = view.followedBy (Matrix4::perspective (1.0f, 2.0f, 0.5f, 50.0f, depthZeroToOne));
        const auto inverse = viewProjection.inverted();

        const auto center = Ray::fromViewportPoint (viewport.getCenter(), viewport, inverse);
        EXPECT_NEAR (center.getOrigin().getX(), 0.0f, 1.0e-4f);
        EXPECT_NEAR (center.getOrigin().getY(), 0.0f, 1.0e-4f);
        EXPECT_GE (center.getOrigin().getZ(), 4.5f - 1.0e-4f); // At or before the near plane
        EXPECT_TRUE (center.getDirection().approximatelyEqualTo ({ 0.0f, 0.0f, -1.0f }));

        // A world point projects back onto the viewport point its ray starts from
        const Vector3<float> world (0.7f, -0.3f, 1.0f);
        const auto ndc = viewProjection.transformPoint (world);
        const Point<float> screen (viewport.getX() + (ndc.getX() + 1.0f) * 0.5f * viewport.getWidth(),
                                   viewport.getY() + (1.0f - ndc.getY()) * 0.5f * viewport.getHeight());

        const auto ray = Ray::fromViewportPoint (screen, viewport, inverse);
        const auto distance = ray.intersectPlane ({ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f });
        ASSERT_TRUE (distance.has_value());

        const auto hitPoint = ray.getPointAt (*distance);
        EXPECT_NEAR (hitPoint.getX(), world.getX(), 1.0e-3f);
        EXPECT_NEAR (hitPoint.getY(), world.getY(), 1.0e-3f);
    }
}

TEST_F (RayTests, FromViewportPointHasYPointingDown)
{
    const Rectangle<float> viewport (0.0f, 0.0f, 100.0f, 100.0f);
    const auto viewProjection = Matrix4::lookAt ({ 0.0f, 0.0f, 5.0f }, {}, { 0.0f, 1.0f, 0.0f })
                                    .followedBy (Matrix4::perspective (1.0f, 1.0f, 0.1f, 10.0f));

    const auto topLeft = Ray::fromViewportPoint ({ 0.0f, 0.0f }, viewport, viewProjection.inverted());
    EXPECT_LT (topLeft.getDirection().getX(), 0.0f);
    EXPECT_GT (topLeft.getDirection().getY(), 0.0f);
}
