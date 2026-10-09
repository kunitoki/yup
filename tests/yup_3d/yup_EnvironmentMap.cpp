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

    /** A small bright spot towards +X, like a sun. */
    static Vector3<float> brightSpot (const Vector3<float>& direction)
    {
        return direction.getX() > 0.995f ? Vector3<float> (100.0f, 100.0f, 100.0f) : Vector3<float> (0.1f, 0.1f, 0.1f);
    }

    /** The red channel integrated over the sphere. */
    static float integrateRed (const EnvironmentMap& environment, int level)
    {
        const auto width = environment.getLevelWidth (level);
        const auto height = environment.getLevelHeight (level);
        const auto pixels = environment.getLevelPixels (level);
        constexpr auto pi = MathConstants<float>::pi;

        auto sum = 0.0f;

        for (int row = 0; row < height; ++row)
        {
            const auto theta = (static_cast<float> (row) + 0.5f) / static_cast<float> (height) * pi;
            const auto solidAngle = (2.0f * pi / static_cast<float> (width)) * (pi / static_cast<float> (height)) * std::sin (theta);

            for (int column = 0; column < width; ++column)
                sum += pixels[static_cast<size_t> ((row * width + column) * 4)] * solidAngle;
        }

        return sum;
    }
};

TEST_F (EnvironmentMapTests, LevelsHalveInSize)
{
    const EnvironmentMap environment (constantRadiance, 64, 4);

    ASSERT_EQ (environment.getNumLevels(), 4);

    for (int level = 0; level < 4; ++level)
    {
        EXPECT_EQ (environment.getLevelWidth (level), 64 >> level);
        EXPECT_EQ (environment.getLevelHeight (level), 32 >> level);
        EXPECT_EQ (environment.getLevelPixels (level).size(), static_cast<size_t> ((64 >> level) * (32 >> level) * 4));
    }
}

TEST_F (EnvironmentMapTests, NumberOfLevelsIsLimitedByTheSize)
{
    const EnvironmentMap environment (constantRadiance, 16, 10);

    ASSERT_EQ (environment.getNumLevels(), 4);
    EXPECT_EQ (environment.getLevelWidth (3), 2);
    EXPECT_EQ (environment.getLevelHeight (3), 1);
    EXPECT_TRUE (environment.getLevelPixels (4).empty());
}

TEST_F (EnvironmentMapTests, ConstantRadianceStaysConstantAtEveryLevel)
{
    const EnvironmentMap environment (constantRadiance, 64, 5);

    for (int level = 0; level < environment.getNumLevels(); ++level)
    {
        const auto pixels = environment.getLevelPixels (level);

        for (size_t i = 0; i < pixels.size(); i += 4)
        {
            EXPECT_NEAR (pixels[i + 0], 0.5f, 1.0e-4f);
            EXPECT_NEAR (pixels[i + 1], 0.25f, 1.0e-4f);
            EXPECT_NEAR (pixels[i + 2], 1.0f, 1.0e-4f);
            EXPECT_NEAR (pixels[i + 3], 1.0f, 1.0e-4f);
        }
    }
}

TEST_F (EnvironmentMapTests, ConstantRadianceGivesTheSameIrradianceEverywhere)
{
    const EnvironmentMap environment (constantRadiance, 128, 1);

    for (const auto& normal : { Vector3<float> (0.0f, 1.0f, 0.0f), Vector3<float> (0.0f, -1.0f, 0.0f), Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 0.6f, -0.8f) })
    {
        const auto irradiance = environment.getIrradiance (normal);
        EXPECT_NEAR (irradiance.getX(), 0.5f, 0.01f);
        EXPECT_NEAR (irradiance.getY(), 0.25f, 0.01f);
        EXPECT_NEAR (irradiance.getZ(), 1.0f, 0.01f);
    }
}

TEST_F (EnvironmentMapTests, IrradianceFacesTheLight)
{
    const EnvironmentMap environment (upperHemisphere, 128, 1);

    EXPECT_GT (environment.getIrradiance ({ 0.0f, 1.0f, 0.0f }).getX(), 0.9f);
    EXPECT_LT (environment.getIrradiance ({ 0.0f, -1.0f, 0.0f }).getX(), 0.1f);
    EXPECT_NEAR (environment.getIrradiance ({ 1.0f, 0.0f, 0.0f }).getX(), 0.5f, 0.05f);
}

TEST_F (EnvironmentMapTests, BlurKeepsTheEnergyOfASmallBrightLight)
{
    const EnvironmentMap environment (brightSpot, 256, 6);
    const auto total = integrateRed (environment, 0);

    for (int level = 1; level < environment.getNumLevels(); ++level)
        EXPECT_NEAR (integrateRed (environment, level) / total, 1.0f, 0.05f) << "level " << level;
}

TEST_F (EnvironmentMapTests, BlurSpreadsABrightLight)
{
    const EnvironmentMap environment (brightSpot, 256, 6);

    // The brightest red texel dims at every level, as the light spreads around the spot
    const auto peakAt = [&] (int level)
    {
        const auto pixels = environment.getLevelPixels (level);

        auto peak = 0.0f;
        for (size_t i = 0; i < pixels.size(); i += 4)
            peak = jmax (peak, pixels[i]);

        return peak;
    };

    for (int level = 1; level < environment.getNumLevels(); ++level)
        EXPECT_LT (peakAt (level), peakAt (level - 1));
}

TEST_F (EnvironmentMapTests, RoughnessSpansTheLevels)
{
    EXPECT_FLOAT_EQ (EnvironmentMap::getRoughnessForLevel (0, 6), 0.0f);
    EXPECT_FLOAT_EQ (EnvironmentMap::getRoughnessForLevel (5, 6), 1.0f);
    EXPECT_FLOAT_EQ (EnvironmentMap::getRoughnessForLevel (0, 1), 0.0f);

    EXPECT_FLOAT_EQ (EnvironmentMap::getBlurForRoughness (0.0f), 0.0f);
    EXPECT_LT (EnvironmentMap::getBlurForRoughness (0.5f), EnvironmentMap::getBlurForRoughness (0.8f));
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

TEST_F (EnvironmentMapTests, NoGpuObjectsWithoutDevice)
{
    EnvironmentMap environment (constantRadiance, 16, 2);

    EXPECT_EQ (environment.getGpuTexture (nullptr), nullptr);
    EXPECT_EQ (environment.getGpuSampler (nullptr), nullptr);
}
