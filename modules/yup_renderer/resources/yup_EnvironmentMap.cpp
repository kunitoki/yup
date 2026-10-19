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

EnvironmentMap::EnvironmentMap (const RadianceFunction& radiance, int imageWidth, int levels)
    : width (jmax (8, imageWidth))
{
    jassert (isPowerOfTwo (imageWidth) && levels >= 1);

    numLevels = jlimit (1, static_cast<int> (std::log2 (getFaceSize())) + 1, levels);

    // Four samples per texel, on a rotated grid
    const auto height = width / 2;
    static constexpr std::array<std::array<float, 2>, 4> offsets { { { 0.375f, 0.125f }, { 0.875f, 0.375f }, { 0.125f, 0.625f }, { 0.625f, 0.875f } } };

    std::vector<float> image (static_cast<size_t> (width * height * 4));

    for (int row = 0; row < height; ++row)
    {
        for (int column = 0; column < width; ++column)
        {
            Vector3<float> sum;

            for (const auto& offset : offsets)
            {
                sum = sum + radiance (getDirection ({ (static_cast<float> (column) + offset[0]) / static_cast<float> (width),
                                                      (static_cast<float> (row) + offset[1]) / static_cast<float> (height) }));
            }

            auto* texel = image.data() + (row * width + column) * 4;
            texel[0] = sum.getX() * 0.25f;
            texel[1] = sum.getY() * 0.25f;
            texel[2] = sum.getZ() * 0.25f;
            texel[3] = 1.0f;
        }
    }

    // Spherical harmonics of the radiance, integrated over the full precision image
    irradiance = SphericalHarmonics::toIrradiance (SphericalHarmonics::project (image, width, height));

    pixels.resize (image.size());
    std::transform (image.begin(), image.end(), pixels.begin(), floatToHalf);
}

EnvironmentMap::~EnvironmentMap() = default;

//==============================================================================

float EnvironmentMap::getRoughnessForLevel (int level, int levels) noexcept
{
    return levels > 1 ? jlimit (0.0f, 1.0f, static_cast<float> (level) / static_cast<float> (levels - 1)) : 0.0f;
}

//==============================================================================

Point<float> EnvironmentMap::getTextureCoordinates (const Vector3<float>& direction) noexcept
{
    constexpr auto twoPi = MathConstants<float>::twoPi;

    auto u = std::atan2 (direction.getX(), direction.getZ()) / twoPi;
    if (u < 0.0f)
        u += 1.0f;

    return { u, std::acos (jlimit (-1.0f, 1.0f, direction.getY())) / MathConstants<float>::pi };
}

Vector3<float> EnvironmentMap::getDirection (Point<float> textureCoordinates) noexcept
{
    const auto phi = textureCoordinates.getX() * MathConstants<float>::twoPi;
    const auto theta = textureCoordinates.getY() * MathConstants<float>::pi;

    return { std::sin (theta) * std::sin (phi), std::cos (theta), std::sin (theta) * std::cos (phi) };
}

} // namespace yup
