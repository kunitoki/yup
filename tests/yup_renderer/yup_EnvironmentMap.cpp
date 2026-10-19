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

class EnvironmentMapTests : public ::testing::Test
{
protected:
    static Vector3<float> constantRadiance (const Vector3<float>&)
    {
        return { 0.5f, 0.25f, 1.0f };
    }

    static Vector3<float> upperHemisphere (const Vector3<float>& direction)
    {
        return direction.getY() > 0.0f ? Vector3<float> (1.0f, 1.0f, 1.0f) : Vector3<float>();
    }
};

TEST_F (EnvironmentMapTests, FacesAreAQuarterOfTheWidth)
{
    EnvironmentMap environment (constantRadiance, 64, 4);
    EXPECT_EQ (environment.getFaceSize(), 16);
    EXPECT_EQ (environment.getNumLevels(), 4);
}

TEST_F (EnvironmentMapTests, NumberOfLevelsIsLimitedByTheFaceSize)
{
    EnvironmentMap environment (constantRadiance, 32, 10);
    EXPECT_EQ (environment.getNumLevels(), 4); // 8, 4, 2, 1
}

TEST_F (EnvironmentMapTests, IrradianceMatchesTheCpuReference)
{
    const EnvironmentMap environment (upperHemisphere, 128, 1);
    const auto expected = SphericalHarmonics::toIrradiance (SphericalHarmonics::project (upperHemisphere));
    const auto& actual = environment.getIrradianceCoefficients();

    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_NEAR (actual[i].getX(), expected[i].getX(), 0.02f) << "coefficient " << i;
        EXPECT_NEAR (actual[i].getY(), expected[i].getY(), 0.02f) << "coefficient " << i;
        EXPECT_NEAR (actual[i].getZ(), expected[i].getZ(), 0.02f) << "coefficient " << i;
    }

    // Not vacuous: the sky lights from above
    EXPECT_GT (actual[1].getX(), 0.1f);
}

TEST_F (EnvironmentMapTests, RoughnessSpansTheLevels)
{
    EXPECT_FLOAT_EQ (EnvironmentMap::getRoughnessForLevel (0, 6), 0.0f);
    EXPECT_FLOAT_EQ (EnvironmentMap::getRoughnessForLevel (5, 6), 1.0f);
    EXPECT_FLOAT_EQ (EnvironmentMap::getRoughnessForLevel (0, 1), 0.0f);
}

TEST_F (EnvironmentMapTests, TextureCoordinatesRoundTrip)
{
    for (const auto& direction : { Vector3<float> (0.0f, 0.0f, 1.0f), Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (-0.48f, 0.6f, -0.64f), Vector3<float> (0.0f, -0.8f, -0.6f) })
    {
        const auto uv = EnvironmentMap::getTextureCoordinates (direction);
        EXPECT_GE (uv.getX(), 0.0f);
        EXPECT_LT (uv.getX(), 1.0f);

        const auto back = EnvironmentMap::getDirection (uv);
        EXPECT_NEAR (back.getX(), direction.getX(), 1.0e-5f);
        EXPECT_NEAR (back.getY(), direction.getY(), 1.0e-5f);
        EXPECT_NEAR (back.getZ(), direction.getZ(), 1.0e-5f);
    }

    EXPECT_NEAR (EnvironmentMap::getTextureCoordinates ({ 0.0f, 1.0f, 0.0f }).getY(), 0.0f, 1.0e-6f);
    EXPECT_NEAR (EnvironmentMap::getTextureCoordinates ({ 0.0f, -1.0f, 0.0f }).getY(), 1.0f, 1.0e-6f);
}
