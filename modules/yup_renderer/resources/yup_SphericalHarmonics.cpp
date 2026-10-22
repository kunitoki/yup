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

namespace yup
{

//==============================================================================

std::array<float, 9> SphericalHarmonics::evaluateBasis (const Vector3<float>& direction) noexcept
{
    const auto x = direction.getX();
    const auto y = direction.getY();
    const auto z = direction.getZ();

    return { 0.282095f,
             0.488603f * y,
             0.488603f * z,
             0.488603f * x,
             1.092548f * x * y,
             1.092548f * y * z,
             0.315392f * (3.0f * z * z - 1.0f),
             1.092548f * x * z,
             0.546274f * (x * x - y * y) };
}

SphericalHarmonics::Coefficients SphericalHarmonics::project (const std::function<Vector3<float> (const Vector3<float>&)>& radiance, int width)
{
    jassert (radiance != nullptr && width >= 2);
    if (radiance == nullptr || width < 2)
        return {};

    const auto height = width / 2;
    std::vector<float> pixels (static_cast<size_t> (width * height * 4));

    for (int row = 0; row < height; ++row)
    {
        for (int column = 0; column < width; ++column)
        {
            const auto value = radiance (EnvironmentMap::getDirection ({ (static_cast<float> (column) + 0.5f) / static_cast<float> (width),
                                                                         (static_cast<float> (row) + 0.5f) / static_cast<float> (height) }));

            auto* texel = pixels.data() + (row * width + column) * 4;
            texel[0] = value.getX();
            texel[1] = value.getY();
            texel[2] = value.getZ();
        }
    }

    return project (pixels, width, height);
}

SphericalHarmonics::Coefficients SphericalHarmonics::project (Span<const float> pixels, int width, int height)
{
    jassert (width > 0 && height > 0 && pixels.size() >= static_cast<size_t> (width * height * 4));
    if (width <= 0 || height <= 0 || pixels.size() < static_cast<size_t> (width * height * 4))
        return {};

    constexpr auto pi = MathConstants<float>::pi;

    Coefficients projection {};

    for (int row = 0; row < height; ++row)
    {
        const auto theta = (static_cast<float> (row) + 0.5f) / static_cast<float> (height) * pi;
        const auto solidAngle = (2.0f * pi / static_cast<float> (width)) * (pi / static_cast<float> (height)) * std::sin (theta);

        for (int column = 0; column < width; ++column)
        {
            const auto direction = EnvironmentMap::getDirection ({ (static_cast<float> (column) + 0.5f) / static_cast<float> (width),
                                                                   (static_cast<float> (row) + 0.5f) / static_cast<float> (height) });

            const auto* texel = pixels.data() + (row * width + column) * 4;
            const auto value = Vector3<float> (texel[0], texel[1], texel[2]) * solidAngle;
            const auto basis = evaluateBasis (direction);

            for (size_t i = 0; i < basis.size(); ++i)
                projection[i] = projection[i] + value * basis[i];
        }
    }

    return projection;
}

// Ramamoorthi and Hanrahan, "An Efficient Representation for Irradiance Environment Maps"
// (SIGGRAPH 2001): the clamped cosine scales the bands by pi, 2 pi / 3 and pi / 4
SphericalHarmonics::Coefficients SphericalHarmonics::toIrradiance (const Coefficients& radiance) noexcept
{
    static constexpr std::array<float, 9> factors { 0.282095f,
                                                    0.488603f * 2.0f / 3.0f,
                                                    0.488603f * 2.0f / 3.0f,
                                                    0.488603f * 2.0f / 3.0f,
                                                    1.092548f * 0.25f,
                                                    1.092548f * 0.25f,
                                                    0.315392f * 0.25f,
                                                    1.092548f * 0.25f,
                                                    0.546274f * 0.25f };

    Coefficients irradiance;

    for (size_t i = 0; i < factors.size(); ++i)
        irradiance[i] = radiance[i] * factors[i];

    return irradiance;
}

Vector3<float> SphericalHarmonics::evaluateIrradiance (const Coefficients& irradiance, const Vector3<float>& normal, int numBands) noexcept
{
    const auto x = normal.getX();
    const auto y = normal.getY();
    const auto z = normal.getZ();

    auto result = irradiance[0]
                + irradiance[1] * y
                + irradiance[2] * z
                + irradiance[3] * x;

    if (numBands > 2)
    {
        result = result
               + irradiance[4] * (x * y)
               + irradiance[5] * (y * z)
               + irradiance[6] * (3.0f * z * z - 1.0f)
               + irradiance[7] * (x * z)
               + irradiance[8] * (x * x - y * y);
    }

    return { jmax (0.0f, result.getX()), jmax (0.0f, result.getY()), jmax (0.0f, result.getZ()) };
}

//==============================================================================

float SphericalHarmonics::encodeRatio (float coefficient, float dc) noexcept
{
    return dc > 0.0f ? jlimit (0.0f, 1.0f, coefficient / (dc * ratioBound) * 0.5f + 0.5f) : 0.5f;
}

float SphericalHarmonics::decodeRatio (float encoded, float dc) noexcept
{
    return (encoded - 0.5f) * 2.0f * dc * ratioBound;
}

} // namespace yup
