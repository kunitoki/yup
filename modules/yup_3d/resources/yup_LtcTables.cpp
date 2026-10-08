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

#include "yup_LtcTables.inc"

static_assert (std::size (ltcInverseMatrices) == LtcTables::size * LtcTables::size * 4);
static_assert (std::size (ltcAmplitudes) == LtcTables::size * LtcTables::size * 2);

size_t getCellIndex (int roughnessIndex, int angleIndex) noexcept
{
    jassert (isPositiveAndBelow (roughnessIndex, LtcTables::size) && isPositiveAndBelow (angleIndex, LtcTables::size));

    const auto r = jlimit (0, LtcTables::size - 1, roughnessIndex);
    const auto a = jlimit (0, LtcTables::size - 1, angleIndex);
    return static_cast<size_t> (r * LtcTables::size + a);
}

/** Uploads four floats per cell, in cell order, as a size x size rgba16float texture. */
GpuTexture::Ptr createHalfTexture (const GpuDevice::Ptr& device, Span<const float> texels, const String& label)
{
    if (device == nullptr)
        return nullptr;

    GpuTextureDesc desc (LtcTables::size, LtcTables::size, GpuTextureFormat::rgba16float);
    desc.label = label;

    auto texture = GpuTexture::create (device, desc);
    if (texture == nullptr)
        return nullptr;

    std::vector<uint16> halves (texels.size());
    std::transform (texels.begin(), texels.end(), halves.begin(), floatToHalf);

    GpuTextureDataDesc data;
    data.data = halves.data();

    return texture->upload (data) ? texture : nullptr;
}

} // namespace

//==============================================================================

std::array<float, 4> LtcTables::getInverseMatrix (int roughnessIndex, int angleIndex) noexcept
{
    const auto* cell = ltcInverseMatrices + getCellIndex (roughnessIndex, angleIndex) * 4;
    return { cell[0], cell[1], cell[2], cell[3] };
}

std::array<float, 2> LtcTables::getAmplitude (int roughnessIndex, int angleIndex) noexcept
{
    const auto* cell = ltcAmplitudes + getCellIndex (roughnessIndex, angleIndex) * 2;
    return { cell[0], cell[1] };
}

GpuTexture::Ptr LtcTables::createMatrixTexture (const GpuDevice::Ptr& device)
{
    return createHalfTexture (device, ltcInverseMatrices, "LtcMatrix");
}

GpuTexture::Ptr LtcTables::createAmplitudeTexture (const GpuDevice::Ptr& device)
{
    std::vector<float> texels;
    texels.reserve (static_cast<size_t> (size * size * 4));

    for (size_t cell = 0; cell < std::size (ltcAmplitudes) / 2; ++cell)
        texels.insert (texels.end(), { ltcAmplitudes[cell * 2], ltcAmplitudes[cell * 2 + 1], 0.0f, 1.0f });

    return createHalfTexture (device, texels, "LtcAmplitude");
}

} // namespace yup
