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
namespace
{

// Labels are ignored
bool isSamePooledTextureDesc (const GpuTextureDesc& a, const GpuTextureDesc& b) noexcept
{
    return a.width == b.width
        && a.height == b.height
        && a.format == b.format
        && a.type == b.type
        && a.depthOrArrayLayers == b.depthOrArrayLayers
        && a.mipLevels == b.mipLevels
        && a.sampleCount == b.sampleCount
        && a.renderTarget == b.renderTarget
        && a.storage == b.storage;
}

} // namespace

//==============================================================================
FrameResources::FrameResources (GpuDevice::Ptr device)
    : device (std::move (device))
{
}

//==============================================================================
void FrameResources::beginFrame (int widthPx, int heightPx)
{
    ++frameCounter;

    if (widthPx != width || heightPx != height)
    {
        transients.clear();
        histories.clear();
    }

    width = widthPx;
    height = heightPx;
}

GpuTexture::Ptr FrameResources::acquire (const GpuTextureDesc& desc)
{
    for (auto& transient : transients)
    {
        if (! transient.acquired && isSamePooledTextureDesc (transient.desc, desc))
        {
            transient.acquired = true;
            return transient.texture;
        }
    }

    auto texture = GpuTexture::create (device, desc);
    if (texture == nullptr)
        return nullptr;

    transients.push_back ({ desc, texture, true });
    return texture;
}

//==============================================================================
FrameResources::History FrameResources::getHistory (StringRef name, const GpuTextureDesc& desc)
{
    auto pair = std::find_if (histories.begin(), histories.end(), [&] (const HistoryPair& h)
    {
        return h.name == name;
    });

    if (pair == histories.end())
    {
        histories.push_back ({ String (name), desc });
        pair = std::prev (histories.end());
    }

    if (pair->textures[0] == nullptr || pair->textures[1] == nullptr || ! isSamePooledTextureDesc (pair->desc, desc))
    {
        pair->desc = desc;
        pair->textures[0] = GpuTexture::create (device, desc);
        pair->textures[1] = GpuTexture::create (device, desc);
        pair->valid = false;
    }

    pair->used = true;
    return { pair->textures[1 - pair->writeIndex], pair->textures[pair->writeIndex], pair->valid };
}

void FrameResources::invalidateHistory() noexcept
{
    for (auto& pair : histories)
    {
        // A pair already read this frame was written from the stale history: the next frame can't trust it either
        pair.invalidatedAfterUse = pair.invalidatedAfterUse || pair.used;
        pair.valid = false;
    }
}

//==============================================================================
GpuBuffer::Ptr FrameResources::acquireStorageBuffer (StorageSlot slot, size_t byteSize)
{
    storageAcquired[static_cast<size_t> (slot)] = true;

    auto& buffer = storageBuffers[static_cast<size_t> (slot)][frameCounter % storageRingSize];

    if (buffer == nullptr || buffer->getSizeInBytes() < byteSize)
    {
        const std::vector<uint8> zeros (std::bit_ceil (jmax (byteSize, size_t (4))));
        buffer = device->createBuffer (GpuBufferType::storage, zeros.data(), zeros.size());
    }

    return buffer;
}

//==============================================================================
const FrameResources::Samplers* FrameResources::getSamplers()
{
    if (! samplersCreated.has_value())
    {
        GpuSamplerDesc compareDesc (GpuFilter::linear, GpuWrapMode::clampToEdge);
        compareDesc.compare = GpuCompareFunction::lessEqual;

        samplers.linearClamp = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::clampToEdge));
        samplers.pointClamp = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
        samplersCreated = samplers.linearClamp != nullptr && samplers.pointClamp != nullptr;

        // Only the shadow maps read it: without it the frames still render, and the shadow passes check it
        samplers.shadowCompare = failShadowCompareSampler ? nullptr : GpuSampler::create (device, compareDesc);
        if (*samplersCreated && samplers.shadowCompare == nullptr)
            YUP_DBG ("FrameResources: unable to create the shadow comparison sampler");
    }

    return *samplersCreated ? &samplers : nullptr;
}

//==============================================================================
void FrameResources::endFrame()
{
    std::erase_if (transients, [] (const Transient& t)
    {
        return ! t.acquired;
    });

    for (auto& transient : transients)
        transient.acquired = false;

    std::erase_if (histories, [] (const HistoryPair& pair)
    {
        return ! pair.used;
    });

    for (auto& pair : histories)
    {
        pair.writeIndex = 1 - pair.writeIndex;
        pair.valid = ! pair.invalidatedAfterUse;
        pair.invalidatedAfterUse = false;
        pair.used = false;
    }

    // The frames in flight retain the buffers they bound
    for (size_t slot = 0; slot < numStorageSlots; ++slot)
    {
        if (! storageAcquired[slot])
            storageBuffers[slot] = {};

        storageAcquired[slot] = false;
    }
}

//==============================================================================
int FrameResources::getWidth() const noexcept
{
    return width;
}

int FrameResources::getHeight() const noexcept
{
    return height;
}

} // namespace yup
