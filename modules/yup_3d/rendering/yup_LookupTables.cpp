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
GpuTexture::Ptr LookupTables::createTexture (const GpuDevice::Ptr& device)
{
    if (device == nullptr)
        return nullptr;

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
            for (const auto value : { amplitude[0], amplitude[1], 0.0f, 1.0f })
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
