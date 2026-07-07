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

std::vector<uint8_t> makeShaderBindingMapBlob (const ShaderReflection& reflection,
                                               ShaderStage stage)
{
    using namespace rive::ore;

    const uint8_t stageMask = (stage == ShaderStage::vertex)
                                ? BindingMap::kStageVertex
                                : (stage == ShaderStage::compute)
                                      ? BindingMap::kStageCompute
                                      : BindingMap::kStageFragment;

    const int slotIndex = (stage == ShaderStage::vertex)
                            ? 0
                            : (stage == ShaderStage::compute) ? 2 : 1;

    BindingMap bm;

    auto pushEntry = [&] (const ShaderReflection::ResourceBinding& res, ResourceKind kind, bool isTexture)
    {
        BindingMap::Entry e {};
        e.group = (uint8_t) res.set;
        e.binding = (uint8_t) res.binding;
        e.kind = kind;
        e.stageMask = stageMask;
        e.backendSlot[slotIndex] = (uint16_t) res.backendSlot;

        if (isTexture)
        {
            e.textureViewDim = TextureViewDim::D2;
            e.textureSampleType = TextureSampleType::Float;
            e.textureMultisampled = false;
        }

        bm.push (e);
    };

    for (const auto& ub : reflection.uniformBuffers)
        pushEntry (ub, ResourceKind::UniformBuffer, false);

    for (const auto& sb : reflection.storageBuffers)
        pushEntry (sb, ResourceKind::StorageBufferRW, false);

    for (const auto& img : reflection.separateImages)
        pushEntry (img, ResourceKind::SampledTexture, true);

    for (const auto& img : reflection.storageImages)
        pushEntry (img, ResourceKind::StorageTexture, true);

    for (const auto& samp : reflection.separateSamplers)
        pushEntry (samp, ResourceKind::Sampler, false);

    bm.finalize();
    return bm.toBlob();
}

} // namespace yup
