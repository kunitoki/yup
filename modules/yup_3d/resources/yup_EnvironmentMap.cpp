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

namespace
{

//==============================================================================
uint16 floatToHalf (float value) noexcept
{
    uint32 bits = 0;
    std::memcpy (&bits, &value, sizeof (bits));

    const auto sign = static_cast<uint16> ((bits >> 16) & 0x8000u);
    const auto exponent = static_cast<int> ((bits >> 23) & 0xffu) - 127 + 15;
    const auto mantissa = bits & 0x7fffffu;

    if (exponent <= 0)
        return sign; // Too small for a normal half: flushed to zero

    if (exponent >= 31)
        return static_cast<uint16> (sign | 0x7bffu); // Clamped to the largest finite half

    // Round to nearest
    auto half = static_cast<uint32> (sign) | (static_cast<uint32> (exponent) << 10) | (mantissa >> 13);
    if ((mantissa & 0x1000u) != 0)
        ++half;

    return static_cast<uint16> (jmin (half, static_cast<uint32> (sign | 0x7bffu)));
}

/** Halves a level, averaging each 2x2 block by the solid angle of its texels. */
std::vector<float> downsampleLevel (const std::vector<float>& source, int sourceWidth, int sourceHeight)
{
    const auto width = jmax (1, sourceWidth / 2);
    const auto height = jmax (1, sourceHeight / 2);

    std::vector<float> result (static_cast<size_t> (width * height * 4));

    const auto rowWeight = [sourceHeight] (int row)
    {
        return std::sin ((static_cast<float> (row) + 0.5f) / static_cast<float> (sourceHeight) * MathConstants<float>::pi);
    };

    for (int row = 0; row < height; ++row)
    {
        const auto row0 = jmin (row * 2, sourceHeight - 1);
        const auto row1 = jmin (row * 2 + 1, sourceHeight - 1);
        const auto weight0 = rowWeight (row0);
        const auto weight1 = rowWeight (row1);
        const auto scale = 0.5f / (weight0 + weight1);

        for (int column = 0; column < width; ++column)
        {
            const auto column0 = jmin (column * 2, sourceWidth - 1);
            const auto column1 = jmin (column * 2 + 1, sourceWidth - 1);

            for (int channel = 0; channel < 4; ++channel)
            {
                const auto at = [&] (int r, int c) { return source[static_cast<size_t> ((r * sourceWidth + c) * 4 + channel)]; };

                result[static_cast<size_t> ((row * width + column) * 4 + channel)]
                    = scale * (weight0 * (at (row0, column0) + at (row0, column1)) + weight1 * (at (row1, column0) + at (row1, column1)));
            }
        }
    }

    return result;
}

/** Blurs a latitude-longitude level on the sphere: every texel gathers its neighbors weighted
    by a von Mises-Fisher kernel, exp ((cos (angle) - 1) / sigma^2), which is a gaussian of the
    angle for small blurs, and by their solid angle. Normalizing by the gathered weight keeps a
    constant environment constant and the energy of the lights.
*/
void blurLevel (std::vector<float>& pixels, int width, int height, float sigmaRadians)
{
    if (sigmaRadians <= 0.0f)
        return;

    constexpr auto pi = MathConstants<float>::pi;
    const auto rowAngle = pi / static_cast<float> (height);
    const auto columnAngle = 2.0f * pi / static_cast<float> (width);
    const auto inverseVariance = 1.0f / (sigmaRadians * sigmaRadians);
    const auto reach = 3.0f * sigmaRadians;

    std::vector<float> sinTheta (static_cast<size_t> (height)), cosTheta (static_cast<size_t> (height));
    std::vector<float> sinPhi (static_cast<size_t> (width)), cosPhi (static_cast<size_t> (width));

    for (int row = 0; row < height; ++row)
    {
        const auto theta = (static_cast<float> (row) + 0.5f) * rowAngle;
        sinTheta[static_cast<size_t> (row)] = std::sin (theta);
        cosTheta[static_cast<size_t> (row)] = std::cos (theta);
    }

    for (int column = 0; column < width; ++column)
    {
        const auto phi = (static_cast<float> (column) + 0.5f) * columnAngle;
        sinPhi[static_cast<size_t> (column)] = std::sin (phi);
        cosPhi[static_cast<size_t> (column)] = std::cos (phi);
    }

    const auto source = pixels;
    const auto rowReach = static_cast<int> (std::ceil (reach / rowAngle));

    for (int row = 0; row < height; ++row)
    {
        const auto firstRow = jmax (0, row - rowReach);
        const auto lastRow = jmin (height - 1, row + rowReach);

        // The parallels shrink towards the poles: the closest one to a pole sets the column reach
        auto smallestSin = 1.0f;
        for (int r = firstRow; r <= lastRow; ++r)
            smallestSin = jmin (smallestSin, sinTheta[static_cast<size_t> (r)]);

        const auto columnReach = jmin (width / 2, static_cast<int> (std::ceil (reach / (columnAngle * jmax (smallestSin, 1.0e-3f)))));
        const auto firstOffset = -columnReach;
        const auto lastOffset = columnReach * 2 + 1 > width ? width - columnReach - 1 : columnReach;

        for (int column = 0; column < width; ++column)
        {
            float sum[4] = {};
            auto totalWeight = 0.0f;

            for (int r = firstRow; r <= lastRow; ++r)
            {
                const auto rowIndex = static_cast<size_t> (r);
                const auto sinProduct = sinTheta[static_cast<size_t> (row)] * sinTheta[rowIndex];
                const auto cosProduct = cosTheta[static_cast<size_t> (row)] * cosTheta[rowIndex];

                for (int offset = firstOffset; offset <= lastOffset; ++offset)
                {
                    const auto c = ((column + offset) % width + width) % width;

                    // cos (phi1 - phi2) from the tables
                    const auto cosDelta = cosPhi[static_cast<size_t> (column)] * cosPhi[static_cast<size_t> (c)]
                                        + sinPhi[static_cast<size_t> (column)] * sinPhi[static_cast<size_t> (c)];
                    const auto cosAngle = sinProduct * cosDelta + cosProduct;

                    const auto weight = std::exp ((cosAngle - 1.0f) * inverseVariance) * sinTheta[rowIndex];
                    const auto* texel = source.data() + (r * width + c) * 4;

                    for (int channel = 0; channel < 4; ++channel)
                        sum[channel] += weight * texel[channel];

                    totalWeight += weight;
                }
            }

            auto* target = pixels.data() + (row * width + column) * 4;
            for (int channel = 0; channel < 4; ++channel)
                target[channel] = sum[channel] / totalWeight;
        }
    }
}

} // namespace

//==============================================================================

EnvironmentMap::EnvironmentMap (const RadianceFunction& radiance, int levelWidth, int numLevels)
    : width (jmax (8, levelWidth))
{
    jassert (isPowerOfTwo (levelWidth) && numLevels >= 1);

    numLevels = jlimit (1, static_cast<int> (std::log2 (width / 2)) + 1, numLevels);

    // Level 0: four samples per texel, on a rotated grid
    {
        const auto levelWidthPx = getLevelWidth (0);
        const auto levelHeightPx = getLevelHeight (0);
        static constexpr std::array<std::array<float, 2>, 4> offsets { { { 0.375f, 0.125f }, { 0.875f, 0.375f }, { 0.125f, 0.625f }, { 0.625f, 0.875f } } };

        auto& pixels = levels.emplace_back (static_cast<size_t> (levelWidthPx * levelHeightPx * 4));

        for (int row = 0; row < levelHeightPx; ++row)
        {
            for (int column = 0; column < levelWidthPx; ++column)
            {
                Vector3<float> sum;

                for (const auto& offset : offsets)
                {
                    sum = sum + radiance (getDirection ({ (static_cast<float> (column) + offset[0]) / static_cast<float> (levelWidthPx),
                                                          (static_cast<float> (row) + offset[1]) / static_cast<float> (levelHeightPx) }));
                }

                auto* texel = pixels.data() + (row * levelWidthPx + column) * 4;
                texel[0] = sum.getX() * 0.25f;
                texel[1] = sum.getY() * 0.25f;
                texel[2] = sum.getZ() * 0.25f;
                texel[3] = 1.0f;
            }
        }
    }

    // Every other level: the previous one halved, then blurred by what its roughness adds
    for (int level = 1; level < numLevels; ++level)
    {
        auto pixels = downsampleLevel (levels.back(), getLevelWidth (level - 1), getLevelHeight (level - 1));

        const auto blur = getBlurForRoughness (getRoughnessForLevel (level, numLevels));
        const auto previousBlur = getBlurForRoughness (getRoughnessForLevel (level - 1, numLevels));
        blurLevel (pixels, getLevelWidth (level), getLevelHeight (level), std::sqrt (jmax (0.0f, blur * blur - previousBlur * previousBlur)));

        levels.push_back (std::move (pixels));
    }

    // Spherical harmonics of the radiance, integrated over level 0
    constexpr auto pi = MathConstants<float>::pi;
    const auto levelWidthPx = getLevelWidth (0);
    const auto levelHeightPx = getLevelHeight (0);
    const auto& pixels = levels.front();

    std::array<Vector3<float>, 9> projection {};

    for (int row = 0; row < levelHeightPx; ++row)
    {
        const auto theta = (static_cast<float> (row) + 0.5f) / static_cast<float> (levelHeightPx) * pi;
        const auto solidAngle = (2.0f * pi / static_cast<float> (levelWidthPx)) * (pi / static_cast<float> (levelHeightPx)) * std::sin (theta);

        for (int column = 0; column < levelWidthPx; ++column)
        {
            const auto direction = getDirection ({ (static_cast<float> (column) + 0.5f) / static_cast<float> (levelWidthPx),
                                                   (static_cast<float> (row) + 0.5f) / static_cast<float> (levelHeightPx) });

            const auto* texel = pixels.data() + (row * levelWidthPx + column) * 4;
            const auto value = Vector3<float> (texel[0], texel[1], texel[2]) * solidAngle;

            const auto x = direction.getX();
            const auto y = direction.getY();
            const auto z = direction.getZ();

            const std::array<float, 9> basis { 0.282095f,
                                               0.488603f * y,
                                               0.488603f * z,
                                               0.488603f * x,
                                               1.092548f * x * y,
                                               1.092548f * y * z,
                                               0.315392f * (3.0f * z * z - 1.0f),
                                               1.092548f * x * z,
                                               0.546274f * (x * x - y * y) };

            for (size_t i = 0; i < basis.size(); ++i)
                projection[i] = projection[i] + value * basis[i];
        }
    }

    // Convolved with the cosine lobe and divided by pi (Ramamoorthi and Hanrahan), folded with
    // the basis constants so that getIrradiance() is a plain polynomial of the normal
    static constexpr std::array<float, 9> factors { 0.282095f,
                                                    0.488603f * 2.0f / 3.0f,
                                                    0.488603f * 2.0f / 3.0f,
                                                    0.488603f * 2.0f / 3.0f,
                                                    1.092548f * 0.25f,
                                                    1.092548f * 0.25f,
                                                    0.315392f * 0.25f,
                                                    1.092548f * 0.25f,
                                                    0.546274f * 0.25f };

    for (size_t i = 0; i < factors.size(); ++i)
        irradiance[i] = projection[i] * factors[i];
}

EnvironmentMap::~EnvironmentMap() = default;

//==============================================================================

int EnvironmentMap::getLevelWidth (int level) const noexcept
{
    return jmax (1, width >> level);
}

int EnvironmentMap::getLevelHeight (int level) const noexcept
{
    return jmax (1, (width / 2) >> level);
}

Span<const float> EnvironmentMap::getLevelPixels (int level) const noexcept
{
    if (! isPositiveAndBelow (level, getNumLevels()))
        return {};

    return levels[static_cast<size_t> (level)];
}

float EnvironmentMap::getRoughnessForLevel (int level, int numLevels) noexcept
{
    return numLevels > 1 ? jlimit (0.0f, 1.0f, static_cast<float> (level) / static_cast<float> (numLevels - 1)) : 0.0f;
}

float EnvironmentMap::getBlurForRoughness (float roughness) noexcept
{
    // The reflected GGX lobe spreads about twice its half vector, alpha being roughness squared;
    // capped where a planar gaussian stops being a fair approximation on the sphere
    return jmin (std::sqrt (2.0f) * roughness * roughness, 0.8f);
}

//==============================================================================

Vector3<float> EnvironmentMap::getIrradiance (const Vector3<float>& normal) const noexcept
{
    const auto x = normal.getX();
    const auto y = normal.getY();
    const auto z = normal.getZ();

    const auto result = irradiance[0]
                      + irradiance[1] * y
                      + irradiance[2] * z
                      + irradiance[3] * x
                      + irradiance[4] * (x * y)
                      + irradiance[5] * (y * z)
                      + irradiance[6] * (3.0f * z * z - 1.0f)
                      + irradiance[7] * (x * z)
                      + irradiance[8] * (x * x - y * y);

    return { jmax (0.0f, result.getX()), jmax (0.0f, result.getY()), jmax (0.0f, result.getZ()) };
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

GpuTexture::Ptr EnvironmentMap::getGpuTexture (const GpuDevice::Ptr& device)
{
    if (device == nullptr)
        return nullptr;

    useDevice (device);

    if (gpuTexture != nullptr)
        return gpuTexture;

    GpuTextureDesc desc (static_cast<uint32_t> (getLevelWidth (0)), static_cast<uint32_t> (getLevelHeight (0)), GpuTextureFormat::rgba16float);
    desc.mipLevels = static_cast<uint32_t> (getNumLevels());
    desc.label = "EnvironmentMap";

    auto texture = GpuTexture::create (device, desc);
    if (texture == nullptr)
        return nullptr;

    std::vector<uint16> halves;

    for (int level = 0; level < getNumLevels(); ++level)
    {
        const auto pixels = getLevelPixels (level);

        halves.resize (pixels.size());
        std::transform (pixels.begin(), pixels.end(), halves.begin(), floatToHalf);

        GpuTextureDataDesc data;
        data.data = halves.data();
        data.mipLevel = static_cast<uint32_t> (level);

        if (! texture->upload (data))
            return nullptr;
    }

    gpuTexture = std::move (texture);
    return gpuTexture;
}

GpuSampler::Ptr EnvironmentMap::getGpuSampler (const GpuDevice::Ptr& device)
{
    if (device == nullptr)
        return nullptr;

    useDevice (device);

    if (gpuSampler == nullptr)
    {
        GpuSamplerDesc desc (GpuFilter::linear, GpuWrapMode::repeat);
        desc.mipmapFilter = GpuFilter::linear;
        desc.wrapV = GpuWrapMode::clampToEdge;
        desc.maxLod = static_cast<float> (getNumLevels() - 1);

        gpuSampler = GpuSampler::create (device, desc);
    }

    return gpuSampler;
}

void EnvironmentMap::useDevice (const GpuDevice::Ptr& device)
{
    if (device == cachedDevice)
        return;

    cachedDevice = device;
    gpuTexture = nullptr;
    gpuSampler = nullptr;
}

} // namespace yup
