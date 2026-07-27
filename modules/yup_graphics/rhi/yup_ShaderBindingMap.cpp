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
                        : (stage == ShaderStage::compute) ? 2
                                                          : 1;

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

//==============================================================================

std::vector<uint8_t> makeGLFixupBlob (const ShaderReflection& reflection)
{
    // Blob format consumed by ore ShaderModule::applyGLFixupFromDesc():
    //   [0]      version byte (== 1)
    //   [1..2]   entry count (uint16, little-endian)
    //   per entry:
    //     [0]    kind: 0 = UBO block binding, 1 = sampler texture unit
    //     [1]    slot
    //     [2..3] name length (uint16, little-endian)
    //     [...]  name bytes (not null-terminated)
    // Kind values mirror rive::ore::ShaderModule::GLFixupEntry::Kind.
    constexpr uint8_t kKindUBOBlock = 0;
    constexpr uint8_t kKindSamplerUniform = 1;

    struct FixupEntry
    {
        uint8_t kind;
        uint8_t slot;
        String name;
    };

    std::vector<FixupEntry> entries;

    for (const auto& ub : reflection.uniformBuffers)
        entries.push_back ({ kKindUBOBlock, (uint8_t) ub.backendSlot, ub.name });

    for (const auto& cs : reflection.glCombinedSamplers)
        entries.push_back ({ kKindSamplerUniform, (uint8_t) cs.textureSlot, cs.name });

    if (entries.empty())
        return {};

    std::vector<uint8_t> blob;
    blob.push_back (1); // version
    blob.push_back ((uint8_t) (entries.size() & 0xff));
    blob.push_back ((uint8_t) ((entries.size() >> 8) & 0xff));

    for (const auto& e : entries)
    {
        const auto nameBytes = e.name.toRawUTF8();
        const auto nameLen = (uint32_t) e.name.getNumBytesAsUTF8();

        blob.push_back (e.kind);
        blob.push_back (e.slot);
        blob.push_back ((uint8_t) (nameLen & 0xff));
        blob.push_back ((uint8_t) ((nameLen >> 8) & 0xff));
        blob.insert (blob.end(), nameBytes, nameBytes + nameLen);
    }

    return blob;
}

} // namespace yup
