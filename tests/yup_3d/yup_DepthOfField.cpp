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

class DepthOfFieldTests : public ::testing::Test
{
protected:
    /** True when p lies inside the regular polygon with n sides inscribed in the unit circle, first vertex at rotation. */
    static bool insidePolygon (Point<float> p, int n, float rotation)
    {
        const auto step = MathConstants<float>::twoPi / static_cast<float> (n);
        const auto apothem = std::cos (step * 0.5f);

        for (int i = 0; i < n; ++i)
        {
            const auto normalAngle = rotation + step * (static_cast<float> (i) + 0.5f);
            const auto distance = p.getX() * std::cos (normalAngle) + p.getY() * std::sin (normalAngle);
            if (distance > apothem + 1.0e-4f)
                return false;
        }

        return true;
    }
};

TEST_F (DepthOfFieldTests, BokehSamplesHaveTheRingCount)
{
    EXPECT_EQ (DepthOfField::makeBokehSamples (3, 0, 0.0f).size(), size_t (1 + 8 + 16 + 24));
    EXPECT_EQ (DepthOfField::makeBokehSamples (DepthOfField::bokehRings, 0, 0.0f).size(), size_t (DepthOfField::numBokehSamples));
    EXPECT_EQ (DepthOfField::numBokehSamples, 81);
}

TEST_F (DepthOfFieldTests, RoundBokehFillsTheUnitDisk)
{
    const auto samples = DepthOfField::makeBokehSamples (4, 0, 0.0f);
    float maxRadius = 0.0f;
    Point<float> mean;

    for (const auto& p : samples)
    {
        maxRadius = jmax (maxRadius, p.magnitude());
        mean += p;
    }

    EXPECT_NEAR (maxRadius, 1.0f, 1.0e-4f);
    EXPECT_NEAR (mean.getX() / static_cast<float> (samples.size()), 0.0f, 1.0e-4f);
    EXPECT_NEAR (mean.getY() / static_cast<float> (samples.size()), 0.0f, 1.0e-4f);
    EXPECT_EQ (samples.front(), Point<float>());
}

TEST_F (DepthOfFieldTests, PolygonBokehStaysInsideThePolygonAndReachesItsVertices)
{
    for (int blades : { 3, 5, 6, 8 })
    {
        const auto samples = DepthOfField::makeBokehSamples (4, blades, 0.3f);
        float maxRadius = 0.0f;

        for (const auto& p : samples)
        {
            EXPECT_TRUE (insidePolygon (p, blades, 0.3f)) << blades << " blades";
            maxRadius = jmax (maxRadius, p.magnitude());
        }

        EXPECT_GT (maxRadius, 0.95f) << blades << " blades";
    }
}

TEST_F (DepthOfFieldTests, PackedCircleOfConfusionMatchesTheCamera)
{
    CameraNode camera;
    camera.fStop = 1.4f;
    camera.focusDistance = 2.0f;

    // The setup pass stores the signed radius in its own texels: half the diameter getCircleOfConfusion() returns
    constexpr float height = 400.0f;
    const auto lens = DepthOfField::getLens (&camera, height);

    for (const auto distance : { 0.5f, 3.0f, 40.0f })
    {
        const auto expected = 0.5f * camera.getCircleOfConfusion (distance, height);
        const auto packed = lens.scale * (1.0f - lens.focus / distance);
        EXPECT_NEAR (packed, expected, 1.0e-4f * std::abs (expected)) << distance;
        EXPECT_NE (expected, 0.0f) << distance;
    }

    // The cameras that cannot blur pack no lens, which the pass skips
    EXPECT_EQ (DepthOfField::getLens (nullptr, height).scale, 0.0f);
    EXPECT_EQ (DepthOfField::getLens (&camera, 0.0f).scale, 0.0f);

    camera.fStop = 0.0f;
    EXPECT_EQ (DepthOfField::getLens (&camera, height).scale, 0.0f);

    camera.fStop = 1.4f;
    camera.projection = CameraNode::Projection::orthographic;
    EXPECT_EQ (DepthOfField::getLens (&camera, height).scale, 0.0f);

    camera.projection = CameraNode::Projection::perspective;
    camera.yFov = 0.0f;
    EXPECT_EQ (DepthOfField::getLens (&camera, height).scale, 0.0f);
}

TEST_F (DepthOfFieldTests, ShaderLinearizationGivesTheInverseViewDistance)
{
    // lib/dof.glsl turns a stored depth into 1 / distance through the projection the shaders read,
    // indexed as GLSL does (p[column][row] at column * 4 + row), for a finite and an infinite far plane
    for (const auto zFar : { 100.0f, 0.0f })
    {
        CameraNode camera;
        camera.zFar = zFar;

        const auto projection = camera.getProjectionMatrix (1.6f);
        const auto* p = projection.getData();

        for (const auto distance : { 0.5f, 3.0f, 40.0f })
        {
            const auto depth = (p[10] * -distance + p[14]) / (p[11] * -distance + p[15]);
            const auto inverseDistance = (depth * p[11] - p[10]) / (depth * p[15] - p[14]);
            EXPECT_NEAR (inverseDistance, 1.0f / distance, 1.0e-3f / distance) << zFar << " " << distance;
        }

        // The far plane: 1 / zFar, or exactly 0 at depth 1 when it is infinite
        EXPECT_NEAR ((p[11] - p[10]) / (p[15] - p[14]), zFar > 0.0f ? 1.0f / zFar : 0.0f, 1.0e-5f) << zFar;
    }
}
