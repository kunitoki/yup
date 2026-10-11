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


#include <yup_3d/yup_3d.h>

#include <gtest/gtest.h>

using namespace yup;

class SphericalHarmonicsTests : public ::testing::Test
{
protected:
    static Vector3<float> constantRadiance (const Vector3<float>&) { return { 0.5f, 0.25f, 1.0f }; }

    static Vector3<float> upperHemisphere (const Vector3<float>& direction)
    {
        return direction.getY() > 0.0f ? Vector3<float> (1.0f, 1.0f, 1.0f) : Vector3<float>();
    }

    static Vector3<float> randomDirection (Random& random)
    {
        const auto z = random.nextFloat() * 2.0f - 1.0f;
        const auto phi = random.nextFloat() * MathConstants<float>::twoPi;
        const auto r = std::sqrt (jmax (0.0f, 1.0f - z * z));
        return { r * std::cos (phi), r * std::sin (phi), z };
    }
};

TEST_F (SphericalHarmonicsTests, ConstantRadianceGivesConstantIrradiance)
{
    const auto irradiance = SphericalHarmonics::toIrradiance (SphericalHarmonics::project (constantRadiance));

    for (const auto& normal : { Vector3<float> (1, 0, 0), Vector3<float> (0, 1, 0), Vector3<float> (0, 0, -1) })
    {
        const auto e = SphericalHarmonics::evaluateIrradiance (irradiance, normal);
        EXPECT_NEAR (e.getX(), 0.5f, 0.01f);
        EXPECT_NEAR (e.getY(), 0.25f, 0.01f);
        EXPECT_NEAR (e.getZ(), 1.0f, 0.01f);
    }
}

TEST_F (SphericalHarmonicsTests, IrradianceFacesTheLight)
{
    const auto irradiance = SphericalHarmonics::toIrradiance (SphericalHarmonics::project (upperHemisphere));

    EXPECT_GT (SphericalHarmonics::evaluateIrradiance (irradiance, { 0.0f, 1.0f, 0.0f }).getX(), 0.9f);
    EXPECT_LT (SphericalHarmonics::evaluateIrradiance (irradiance, { 0.0f, -1.0f, 0.0f }).getX(), 0.1f);
    EXPECT_NEAR (SphericalHarmonics::evaluateIrradiance (irradiance, { 1.0f, 0.0f, 0.0f }).getX(), 0.5f, 0.05f);
}

TEST_F (SphericalHarmonicsTests, TwoBandsKeepTheDirection)
{
    const auto irradiance = SphericalHarmonics::toIrradiance (SphericalHarmonics::project (upperHemisphere));

    EXPECT_GT (SphericalHarmonics::evaluateIrradiance (irradiance, { 0.0f, 1.0f, 0.0f }, 2).getX(),
               SphericalHarmonics::evaluateIrradiance (irradiance, { 0.0f, -1.0f, 0.0f }, 2).getX());
}

TEST_F (SphericalHarmonicsTests, SingleDirectionStaysWithinTheRatioBound)
{
    Random random (42);

    for (int i = 0; i < 1000; ++i)
    {
        const auto direction = randomDirection (random);

        // A delta light: radiance coefficients are the basis itself
        SphericalHarmonics::Coefficients radiance {};
        const auto basis = SphericalHarmonics::evaluateBasis (direction);
        for (size_t k = 0; k < basis.size(); ++k)
            radiance[k] = Vector3<float> (basis[k], basis[k], basis[k]);

        const auto irradiance = SphericalHarmonics::toIrradiance (radiance);
        for (size_t k = 1; k < irradiance.size(); ++k)
            EXPECT_LE (std::abs (irradiance[k].getX() / irradiance[0].getX()), SphericalHarmonics::ratioBound + 1.0e-4f);
    }
}

TEST_F (SphericalHarmonicsTests, RatioEncodingRoundTripsWithin8Bits)
{
    const auto dc = 0.8f;

    for (auto value : { -1.6f, -0.4f, 0.0f, 0.3f, 1.59f })
    {
        const auto encoded = SphericalHarmonics::encodeRatio (value, dc);
        EXPECT_GE (encoded, 0.0f);
        EXPECT_LE (encoded, 1.0f);

        const auto quantized = std::round (encoded * 255.0f) / 255.0f;
        EXPECT_NEAR (SphericalHarmonics::decodeRatio (quantized, dc), value, dc * SphericalHarmonics::ratioBound / 255.0f + 1.0e-5f);
    }
}

TEST_F (SphericalHarmonicsTests, RatioEncodingOfZeroDcIsNeutral)
{
    EXPECT_FLOAT_EQ (SphericalHarmonics::encodeRatio (0.0f, 0.0f), 0.5f);
    EXPECT_FLOAT_EQ (SphericalHarmonics::decodeRatio (0.5f, 0.0f), 0.0f);
}

TEST_F (SphericalHarmonicsTests, BasisIsOrthonormalOverTheSphere)
{
    for (size_t j = 0; j < 9; ++j)
    {
        const auto coefficients = SphericalHarmonics::project ([j] (const Vector3<float>& direction)
        {
            const auto value = SphericalHarmonics::evaluateBasis (direction)[j];
            return Vector3<float> (value, value, value);
        });

        for (size_t k = 0; k < coefficients.size(); ++k)
            EXPECT_NEAR (coefficients[k].getX(), j == k ? 1.0f : 0.0f, 0.01f);
    }
}

TEST_F (SphericalHarmonicsTests, PixelsProjectLikeTheFunctionAtTheirCenters)
{
    constexpr int width = 64;
    constexpr int height = width / 2;

    std::vector<float> pixels (static_cast<size_t> (width * height * 4));
    for (int row = 0; row < height; ++row)
    {
        for (int column = 0; column < width; ++column)
        {
            const auto value = upperHemisphere (EnvironmentMap::getDirection ({ (static_cast<float> (column) + 0.5f) / static_cast<float> (width),
                                                                                (static_cast<float> (row) + 0.5f) / static_cast<float> (height) }));

            auto* texel = pixels.data() + (row * width + column) * 4;
            texel[0] = value.getX();
            texel[1] = value.getY();
            texel[2] = value.getZ();
        }
    }

    const auto fromPixels = SphericalHarmonics::project (pixels, width, height);
    const auto fromFunction = SphericalHarmonics::project (upperHemisphere, width);

    for (size_t k = 0; k < fromPixels.size(); ++k)
        EXPECT_FLOAT_EQ (fromPixels[k].getY(), fromFunction[k].getY());

    EXPECT_GT (fromPixels[1].getY(), 0.0f);
}

TEST_F (SphericalHarmonicsTests, RatioEncodingClampsBeyondTheBound)
{
    EXPECT_FLOAT_EQ (SphericalHarmonics::encodeRatio (5.0f, 0.8f), 1.0f);
    EXPECT_FLOAT_EQ (SphericalHarmonics::encodeRatio (-5.0f, 0.8f), 0.0f);
}
