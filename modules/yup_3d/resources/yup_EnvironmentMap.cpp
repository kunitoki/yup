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

//==============================================================================

EnvironmentMap::CubeBuild EnvironmentMap::buildCube (ProbeRenderer& probes) const
{
    const auto& device = probes.getDevice();
    if (device == nullptr)
        return {};

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return {};

    const auto format = probes.getRadianceFormat();
    const auto rgbm = format == GpuTextureFormat::rgba8unorm;
    const auto height = width / 2;

    // An RGBM image is sampled at mip 0 only: averaging RGBM texels would mix their multipliers
    GpuTextureDesc imageDesc (static_cast<uint32> (width), static_cast<uint32> (height), format, ! rgbm);
    imageDesc.mipLevels = rgbm ? 1u : static_cast<uint32> (std::log2 (width)) + 1;
    imageDesc.label = "EnvironmentMap image";

    // The recorded passes keep these textures until the GPU is done with them, but not the
    // generateMips() pipeline the image caches: the caller keeps the image
    auto image = GpuTexture::create (device, imageDesc);
    auto source = probes.createCube (getFaceSize());
    auto cube = probes.createCube (getFaceSize(), numLevels);

    std::vector<uint8> encoded;
    GpuTextureDataDesc data;
    data.data = pixels.data();

    if (rgbm)
    {
        encoded.resize (pixels.size());
        for (size_t i = 0; i < pixels.size(); i += 4)
        {
            const float rgb[] = { halfToFloat (pixels[i]), halfToFloat (pixels[i + 1]), halfToFloat (pixels[i + 2]) };
            encodeRgbm (rgb, encoded.data() + i);
        }

        data.data = encoded.data();
    }

    if (image == nullptr || source == nullptr || cube == nullptr || ! image->upload (data))
    {
        YUP_DBG ("EnvironmentMap: unable to create the textures of the cube");
        return { nullptr, frame.getStatus() };
    }

    const auto recorded = (rgbm || image->generateMips (frame))
                       && probes.equirectToCube (frame, image, source)
                       && probes.downsampleCube (frame, source)
                       && probes.prefilterToCube (frame, source, cube);

    CubeBuild build { recorded ? cube : nullptr, frame.getStatus(), rgbm ? nullptr : image };
    frame.submit();

    if (! recorded)
        YUP_DBG ("EnvironmentMap: unable to record the passes building the cube");

    return build;
}

} // namespace yup
