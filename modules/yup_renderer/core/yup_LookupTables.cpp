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

/** Uploads one vector of half float rgba texels per layer, as a width x width rgba16float array. */
GpuTexture::Ptr createHalfArray (const GpuDevice::Ptr& device, int width, const std::array<std::vector<uint16>, LookupTables::numLayers>& layers, const String& label)
{
    if (device == nullptr)
        return nullptr;

    GpuTextureDesc desc (static_cast<uint32_t> (width), static_cast<uint32_t> (width), GpuTextureFormat::rgba16float);
    desc.type = GpuTextureType::array2D;
    desc.depthOrArrayLayers = LookupTables::numLayers;
    desc.label = label;

    auto texture = GpuTexture::create (device, desc);
    if (texture == nullptr)
        return nullptr;

    for (size_t layer = 0; layer < layers.size(); ++layer)
    {
        GpuTextureDataDesc data;
        data.data = layers[layer].data();
        data.layer = static_cast<uint32_t> (layer);

        if (! texture->upload (data))
            return nullptr;
    }

    return texture;
}

} // namespace

//==============================================================================
std::vector<float> LookupTables::computeSheenAlbedo()
{
    // Over the half vectors h, d omega_l = 4 (v.h) d omega_h, so E is the sum over the elevations of
    // h of D (alpha, n.h) times the integral of V (n.l) 4 (v.h) over their azimuths, which depends
    // on the view alone: one set of those per column, then one weighted sum per cell. The midpoint
    // rule, the azimuths over half the circle by symmetry
    constexpr size_t elevations = 256;
    constexpr size_t azimuths = 32;
    constexpr auto cells = static_cast<size_t> (size);
    const auto pi = MathConstants<double>::pi;
    const auto dTheta = pi * 0.5 / elevations;
    const auto dPhi = pi / azimuths;

    std::array<double, elevations> sinTheta {}, cosTheta {};
    for (size_t i = 0; i < elevations; ++i)
    {
        sinTheta[i] = std::sin ((static_cast<double> (i) + 0.5) * dTheta);
        cosTheta[i] = std::cos ((static_cast<double> (i) + 0.5) * dTheta);
    }

    std::array<double, azimuths> cosPhi {};
    for (size_t j = 0; j < azimuths; ++j)
        cosPhi[j] = std::cos ((static_cast<double> (j) + 0.5) * dPhi);

    std::vector<double> weights (cells * elevations);
    for (size_t angle = 0; angle < cells; ++angle)
    {
        const auto u = static_cast<double> (angle) / static_cast<double> (cells - 1);
        const auto nDotV = std::max (1.0 - u * u, 1.0e-4);
        const auto sinV = std::sqrt (1.0 - nDotV * nDotV);

        for (size_t i = 0; i < elevations; ++i)
        {
            // V (n.l) 4 (v.h), with V = 1 / (4 (n.l + n.v - n.l n.v))
            auto sum = 0.0;
            for (const auto c : cosPhi)
            {
                const auto vDotH = sinV * sinTheta[i] * c + nDotV * cosTheta[i];
                const auto nDotL = 2.0 * vDotH * cosTheta[i] - nDotV;
                if (vDotH > 0.0 && nDotL > 0.0)
                    sum += nDotL * vDotH / (nDotL + nDotV - nDotL * nDotV);
            }

            weights[angle * elevations + i] = sum * sinTheta[i] * dTheta * dPhi * 2.0;
        }
    }

    std::vector<float> albedo (cells * cells);
    std::array<double, elevations> distribution {};

    for (size_t roughness = 0; roughness < cells; ++roughness)
    {
        // The Charlie distribution, (2 + 1 / alpha) sin^(1 / alpha) (theta_h) / (2 pi)
        const auto perceptual = std::max (static_cast<double> (roughness) / static_cast<double> (cells - 1), static_cast<double> (minSheenRoughness));
        const auto inverseAlpha = 1.0 / (perceptual * perceptual);
        for (size_t i = 0; i < elevations; ++i)
            distribution[i] = (2.0 + inverseAlpha) * std::pow (sinTheta[i], inverseAlpha) / (2.0 * pi);

        for (size_t angle = 0; angle < cells; ++angle)
        {
            const auto sum = std::inner_product (distribution.begin(), distribution.end(), weights.begin() + static_cast<std::ptrdiff_t> (angle * elevations), 0.0);
            albedo[roughness * cells + angle] = static_cast<float> (std::min (sum, 1.0));
        }
    }

    return albedo;
}

//==============================================================================
GpuTexture::Ptr LookupTables::createTexture (const GpuDevice::Ptr& device)
{
    if (device == nullptr)
        return nullptr;

    // The same on every device: computed once per process
    static const auto sheenAlbedo = computeSheenAlbedo();

    std::array<std::vector<uint16>, numLayers> layers;
    for (auto& layer : layers)
        layer.reserve (static_cast<size_t> (size * size * 4));

    for (int roughness = 0; roughness < size; ++roughness)
    {
        for (int angle = 0; angle < size; ++angle)
        {
            for (const auto value : LtcTables::getInverseMatrix (roughness, angle))
                layers[0].push_back (floatToHalf (value));

            const auto amplitude = LtcTables::getAmplitude (roughness, angle);
            for (const auto value : { amplitude[0], amplitude[1], sheenAlbedo[static_cast<size_t> (roughness * size + angle)], 1.0f })
                layers[1].push_back (floatToHalf (value));
        }
    }

    return createHalfArray (device, size, layers, "LookupTables");
}

GpuTexture::Ptr LookupTables::createBlackTexture (const GpuDevice::Ptr& device)
{
    std::array<std::vector<uint16>, numLayers> layers;
    for (auto& layer : layers)
        layer.assign (4, floatToHalf (0.0f));

    return createHalfArray (device, 1, layers, "LookupTables black");
}

} // namespace yup
