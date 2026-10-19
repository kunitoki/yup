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

#include <yup_renderer/yup_renderer.h>

using namespace yup;

class WorldFogTests : public ::testing::Test
{
protected:
    /** Midpoint-rule optical depth with many steps, the reference for the closed form. */
    static float numericOpticalDepth (const WorldFog& fog, const Vector3<float>& a, const Vector3<float>& b)
    {
        constexpr int steps = 20000;
        const auto delta = (b - a) * (1.0f / steps);
        double sum = 0.0;

        for (int i = 0; i < steps; ++i)
            sum += fog.getDensity ((a + delta * (static_cast<float> (i) + 0.5f)).getY());

        return static_cast<float> (sum * delta.length());
    }

    static WorldFog makeFog (float density, float falloff)
    {
        WorldFog fog;
        fog.enabled = true;
        fog.density = density;
        fog.heightFalloff = falloff;
        fog.baseHeight = 1.0f;
        return fog;
    }
};

TEST_F (WorldFogTests, DisabledFogHasNoOpticalDepth)
{
    WorldFog fog;
    EXPECT_FALSE (fog.enabled);
    EXPECT_EQ (fog.getDensity (0.0f), 0.0f);
    EXPECT_EQ (fog.getOpticalDepth ({ 0, 0, 0 }, { 0, 0, -100 }), 0.0f);
}

TEST_F (WorldFogTests, DensityThinsAboveTheBase)
{
    const auto fog = makeFog (0.05f, 0.3f);

    EXPECT_FLOAT_EQ (fog.getDensity (1.0f), 0.05f);
    EXPECT_NEAR (fog.getDensity (11.0f), 0.05f * std::exp (-3.0f), 1.0e-8f);
    EXPECT_GT (fog.getDensity (-5.0f), 0.05f);
}

TEST_F (WorldFogTests, UniformFogIsDensityTimesLength)
{
    EXPECT_NEAR (makeFog (0.1f, 0.0f).getOpticalDepth ({ 0, 5, 0 }, { 3, 9, 0 }), 0.5f, 1.0e-5f);
}

TEST_F (WorldFogTests, ClosedFormMatchesNumericIntegration)
{
    struct Segment
    {
        float falloff;
        Vector3<float> from, to;
    };

    const Segment segments[] = {
        { 0.3f, { 0, 0, 0 }, { 0, 10, -20 } },      // rising
        { 0.3f, { 0, 8, 0 }, { 5, -2, -30 } },      // falling
        { 0.3f, { 0, 3, 0 }, { 0, 3, -50 } },       // horizontal: zero slope, the series branch avoids 0 / 0
        { 0.3f, { 0, 3, 0 }, { 0, 3.0001f, -50 } }, // nearly horizontal: the series branch
        { 0.3f, { 0, 40, 0 }, { 0, -5, -10 } },     // steep fall from far above the base
        { 2.0f, { 0, 50, 0 }, { 0, 0, -10 } },      // the density grows by e^100 along the segment
        { -2.0f, { 0, -10, 0 }, { 0, -60, -10 } }   // fog thickening upwards: the denser end is the upper one
    };

    for (const auto& segment : segments)
    {
        const auto fog = makeFog (0.05f, segment.falloff);
        const auto reference = numericOpticalDepth (fog, segment.from, segment.to);
        EXPECT_NEAR (fog.getOpticalDepth (segment.from, segment.to), reference, reference * 1.0e-3f);
    }
}

TEST_F (WorldFogTests, OpticalDepthIsSymmetric)
{
    const auto fog = makeFog (0.05f, 0.3f);
    EXPECT_NEAR (fog.getOpticalDepth ({ 1, 2, 3 }, { -4, 9, -20 }), fog.getOpticalDepth ({ -4, 9, -20 }, { 1, 2, 3 }), 1.0e-5f);
}

TEST_F (WorldFogTests, ClampedDensityBelowTheBaseIsIntegratedExactly)
{
    // With a falloff of 2 the density stops growing 20 units below the base, at y = -19
    const auto fog = makeFog (0.05f, 2.0f);

    const Vector3<float> crossing[] = { { 0, -30, 0 }, { 0, 5, -10 } };
    const auto crossingReference = numericOpticalDepth (fog, crossing[0], crossing[1]);
    EXPECT_NEAR (fog.getOpticalDepth (crossing[0], crossing[1]), crossingReference, crossingReference * 1.0e-3f);
    EXPECT_NEAR (fog.getOpticalDepth (crossing[1], crossing[0]), crossingReference, crossingReference * 1.0e-3f);

    const Vector3<float> below[] = { { 0, -40, 0 }, { 0, -25, -10 } };
    EXPECT_NEAR (fog.getOpticalDepth (below[0], below[1]), fog.getDensity (-40.0f) * (below[1] - below[0]).length(), fog.getDensity (-40.0f) * 1.0e-3f);
}

TEST_F (WorldFogTests, DeepBelowTheBaseStaysFinite)
{
    const auto fog = makeFog (0.05f, 2.0f);
    EXPECT_TRUE (std::isfinite (fog.getOpticalDepth ({ 0, -500, 0 }, { 0, -400, -10 })));
    EXPECT_TRUE (std::isfinite (fog.getOpticalDepth ({ 0, 500, 0 }, { 0, -500, -10 })));
}

TEST_F (WorldFogTests, EqualityComparesTheFields)
{
    const auto fog = makeFog (0.05f, 0.3f);
    auto other = fog;
    EXPECT_TRUE (fog == other);

    other.anisotropy = 0.5f;
    EXPECT_FALSE (fog == other);

    other = fog;
    other.color = { 0.5f, 1.0f, 1.0f };
    EXPECT_FALSE (fog == other);
}

TEST_F (WorldFogTests, NodeEnablesItsFogByDefault)
{
    const WorldFogNode node;
    EXPECT_TRUE (node.fog.enabled);

    const auto fog = makeFog (0.1f, 0.5f);
    EXPECT_TRUE (WorldFogNode (fog).fog == fog);
}
