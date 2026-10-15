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

struct GpuRenderPass::Impl
{
    struct TextureBinding
    {
        int group;
        int binding;
        GpuTexture::Ptr texture;
        std::optional<GpuTextureViewDesc> view;
    };

    struct SamplerBinding
    {
        int group;
        int binding;
        GpuSampler::Ptr sampler;
    };

    struct UboBinding
    {
        int group;
        int binding;
        std::vector<uint8_t> data;

        /** Where the frame's uniform ring holds the bytes of a block a pipeline declares dynamic.
            A chunk of 0 means they have not been written since they changed. */
        uint32_t ringChunk = 0;
        uint32_t ringOffset = 0;
    };

    struct StorageBinding
    {
        int group;
        int binding;
        GpuBuffer::Ptr buffer;
    };

    struct VertexBinding
    {
        int slot;
        GpuBuffer::Ptr buffer;
        rive::ore::Buffer* oreBuffer = nullptr;
    };

    struct ColorAttachment
    {
        GpuTexture::Ptr texture;
        GpuTextureViewDesc view;
        GpuRenderOptions options;
        GpuTexture::Ptr resolveTexture;
        GpuTextureViewDesc resolveView;
    };

    /** The bind group a draw built for a group, reused by later draws until a binding its layout
        declares changes or a pipeline switch changes the group's layout or default samplers. */
    struct GroupCache
    {
        rive::rcp<rive::ore::BindGroup> group;
        bool dirty = true;

        /** The ring chunk the group's dynamic uniform buffers are bound to, or 0 without any. */
        uint32_t ringChunk = 0;

        /** The native storage buffers the group was built with: an update may swap a buffer's backing. */
        std::vector<void*> storageHandles;
    };

    /** The dynamic offsets of a bind, in the order of the layout's dynamic entries. */
    struct DynamicOffsets
    {
        std::array<uint32_t, GpuPipelineHelpers::maxDynamicUniformBuffers> values {};
        uint32_t count = 0;

        bool operator== (const DynamicOffsets& other) const noexcept
        {
            return count == other.count && std::equal (values.begin(), values.begin() + count, other.values.begin());
        }
    };

    /** Calls @p visit with the native handle of every storage buffer bound to @p group, in binding order. */
    template <typename Visitor>
    void forEachStorageHandle (uint32_t group, Visitor&& visit) const
    {
        for (const auto& sb : storageBindings)
        {
            if (sb.group != (int) group || sb.buffer == nullptr)
                continue;

            const auto* bufferImpl = sb.buffer->getImpl();
            visit (bufferImpl != nullptr ? bufferImpl->getNativeStorageHandleForRead() : nullptr);
        }
    }

    bool storageHandlesMatch (uint32_t group, const GroupCache& cache) const
    {
        size_t index = 0;
        bool matches = true;

        forEachStorageHandle (group, [&] (void* handle)
        {
            matches = matches && index < cache.storageHandles.size() && cache.storageHandles[index] == handle;
            ++index;
        });

        return matches && index == cache.storageHandles.size();
    }

    rive::ore::Context* oreCtx = nullptr;
    GpuFrame::Impl* framePools = nullptr;

    // Attachment 0 is the surface the pass was begun on; 1..3 are MRT extras.
    // A pass begun on a depth-only target has no color attachment at all, and
    // its surface is the depth attachment.
    ColorAttachment colorAttachments[4];
    uint32_t colorCount = 1;

    GpuTexture::Ptr depthTexture;
    GpuTextureViewDesc depthView;
    GpuDepthStencilOptions depthOptions;

    int width = 0;
    int height = 0;

    GpuPipeline::Ptr pipelineRef;
    rive::ore::Pipeline* orePipeline = nullptr;
    const std::vector<rive::rcp<rive::ore::BindGroupLayout>>* oreLayouts = nullptr;
    const std::vector<std::vector<GpuPipeline::Impl::SamplerBinding>>* oreSamplers = nullptr;
    const std::vector<std::vector<uint32_t>>* dynamicBindings = nullptr;

    // Sticky per-pass state, replayed into every encoded ore render pass.
    bool viewportSet = false;
    float viewportRect[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    bool scissorSet = false;
    uint32_t scissorRect[4] = { 0, 0, 0, 0 };
    bool stencilReferenceSet = false;
    uint32_t stencilReference = 0;
    bool blendColorSet = false;
    GpuColor blendColor;

    // How many draws have been encoded. The attachments are only cleared when the
    // backend pass is opened, so a pass that has to be reopened - because
    // something else took the context's active pass - must load rather than clear.
    uint32_t encodeCount = 0;
    bool finished = false;

    std::vector<TextureBinding> textureBindings;
    std::vector<SamplerBinding> samplerBindings;
    std::vector<UboBinding> uboBindings;
    std::vector<StorageBinding> storageBindings;
    std::vector<VertexBinding> vertexBindings;

    GpuBuffer::Ptr indexBuffer;
    rive::ore::Buffer* indexOreBuffer = nullptr;
    rive::ore::IndexFormat indexFormat = rive::ore::IndexFormat::none;

    /** The backend render pass every draw of this GpuRenderPass encodes into.

        Opened lazily by the first draw and held until finish(), so a pass costs
        one command encoder rather than one per draw. */
    std::unique_ptr<rive::ore::RenderPass> orePass;

    /** Groups 0..3; any higher group is rebuilt by every draw. */
    std::array<GroupCache, 4> groupCaches;

    /** Scratch lists encode() clears and refills, so draws stop allocating once they have grown. */
    std::vector<std::pair<uint32_t, rive::rcp<rive::ore::BindGroup>>> bindGroups;
    std::vector<rive::ore::BindGroupDesc::UBOEntry> uboEntries;
    std::vector<rive::ore::BindGroupDesc::TexEntry> texEntries;
    std::vector<rive::ore::BindGroupDesc::StorageEntry> storageEntries;
    std::vector<rive::ore::BindGroupDesc::SampEntry> sampEntries;

    /** What the open backend pass was last given, so draws only encode what changed. Held, so
        an address cannot be reused by another object while it is compared against.

        Metal, Vulkan and WebGPU latch the current backing of each uniform buffer when a group is
        bound. Skipping a group bound with the same dynamic offsets is only safe because no range
        a recorded command reads is written again during the frame: pooled uniform buffers are
        written once, the uniform ring writes each range once, unsynchronized, before the draw that
        reads it is recorded, and a storage buffer whose backing changed rebuilds its group
        (storageHandlesMatch). Binding a caller-owned buffer that can be updated between draws
        needs its upload generation in the group cache. */
    rive::rcp<rive::ore::Pipeline> encodedPipeline;
    std::array<rive::rcp<rive::ore::BindGroup>, 4> encodedGroups;
    std::array<DynamicOffsets, 4> encodedOffsets;
    bool passStateDirty = true;

    /** False on OpenGL, whose bindings are global: a texture upload or a pipeline compiled between
        two draws changes them behind the pass, so every draw encodes its whole state. */
    bool backendKeepsState = true;

    /** Set when a texture binding or the pipeline changed, for the next draw to check the views again. */
    bool textureViewsChanged = true;
    bool textureViewsConflict = false;
    int textureViewChecks = 0;

    /** Marks @p group for a rebuild after its @p binding changed, unless the current pipeline's layout
        for the group lacks the binding: a switch to a pipeline declaring it rebuilds the group anyway. */
    void markGroupDirty (int group, int binding) noexcept
    {
        if (group < 0 || group >= (int) groupCaches.size())
            return;

        const auto* layout = oreLayouts != nullptr && (size_t) group < oreLayouts->size() ? (*oreLayouts)[(size_t) group].get() : nullptr;
        if (layout == nullptr || layout->findEntry ((uint32_t) binding) != nullptr)
            groupCaches[(size_t) group].dirty = true;
    }

    /** True when the current pipeline binds the uniform buffer at @p group and @p binding at a dynamic offset. */
    bool declaresDynamic (int group, int binding) const
    {
        if (oreLayouts == nullptr || group < 0 || (size_t) group >= oreLayouts->size())
            return false;

        const auto* layout = (*oreLayouts)[(size_t) group].get();
        return layout != nullptr && binding >= 0 && layout->hasDynamicOffset ((uint32_t) binding);
    }

    /** Calls @p visit with every uniform buffer bound to a dynamic slot of the current pipeline. */
    template <typename Visitor>
    void forEachDynamicUniform (Visitor&& visit)
    {
        for (size_t group = 0; group < dynamicBindings->size(); ++group)
        {
            for (const auto binding : (*dynamicBindings)[group])
            {
                if (auto* ub = findUniformBuffer ((int) group, (int) binding))
                    visit (*ub);
            }
        }
    }

    UboBinding* findUniformBuffer (int group, int binding)
    {
        for (auto& ub : uboBindings)
        {
            if (ub.group == group && ub.binding == binding)
                return &ub;
        }

        return nullptr;
    }

    /** Writes the dynamic uniform buffers the ring's current chunk lacks, moving the ring to a new
        chunk, and writing all of them, when they do not fit: a draw binds a single chunk. */
    bool writeDynamicUniforms();

    /** The offsets a bind of @p group's @p bindGroup takes. */
    DynamicOffsets collectDynamicOffsets (uint32_t group, const rive::ore::BindGroup& bindGroup);

    static bool layoutDeclares (const rive::ore::BindGroupLayout& layout, uint32_t binding, bool asTexture);

    /** True when one texture is bound with two different views. OpenGL applies a sampled mip range
        to the texture object, so such a draw would sample the last range in both slots. */
    bool hasConflictingTextureViews() const;

    bool encode (uint32_t count, bool indexed, uint32_t instanceCount, uint32_t first, int32_t baseVertex, uint32_t firstInstance);

    /** Opens the backend render pass if it is not already open. */
    bool ensurePassOpen();

    /** Finishes the backend render pass and deregisters it from the context. */
    void closePass();

    /** Fills the ore render pass descriptor from the bound attachments. The views
        come from each texture's cache, and are also retained by the frame so they
        outlive the submitted commands even if the caller drops the texture. */
    bool buildRenderPassDesc (rive::ore::RenderPassDesc& rpDesc);

    /** Applies the sticky viewport / scissor / stencil / blend state to a pass. */
    void applyPassState (rive::ore::RenderPass& renderPass) const;
};

//==============================================================================

bool GpuRenderPass::Impl::ensurePassOpen()
{
    if (orePass != nullptr && orePass->isFinished())
        closePass();

    if (orePass != nullptr)
        return true;

    if (oreCtx == nullptr || framePools == nullptr)
        return false;

    rive::ore::RenderPassDesc rpDesc;
    if (! buildRenderPassDesc (rpDesc))
        return false;

    YUP_AUTORELEASEPOOL
    {
        if (framePools->openPass != nullptr)
            framePools->openPass->finish();

        if (framePools->openComputePass != nullptr)
            yup_finishComputePass (*framePools->openComputePass);

        auto scopeIndex = -1;
        auto* sampler = framePools->samplerForNextPass (scopeIndex);
        if (sampler != nullptr)
            sampler->beginRenderPass (scopeIndex);

        orePass = oreCtx->beginRenderPass (rpDesc);

        if (sampler != nullptr)
            sampler->endRenderPass();
    }

    if (orePass == nullptr)
        return false;

    framePools->openPass = orePass.get();

    // A new backend pass starts without any state
    const auto platform = framePools->device != nullptr ? framePools->device->getPlatform() : GpuPlatform::Headless;
    backendKeepsState = platform != GpuPlatform::OpenGL && platform != GpuPlatform::OpenGLES;
    encodedPipeline = nullptr;
    encodedGroups = {};
    passStateDirty = true;
    return true;
}

void GpuRenderPass::Impl::closePass()
{
    if (orePass == nullptr)
        return;

    YUP_AUTORELEASEPOOL
    {
        orePass->finish();
    }

    if (framePools != nullptr && framePools->openPass == orePass.get())
        framePools->openPass = nullptr;

    orePass = nullptr;
}

//==============================================================================

bool GpuRenderPass::Impl::buildRenderPassDesc (rive::ore::RenderPassDesc& rpDesc)
{
    using namespace GpuPipelineHelpers;

    // Only the first encoded draw honours a clear; the rest load, so multiple
    // draws accumulate into one surface and share one depth buffer.
    const bool honourClears = encodeCount == 0;

    if (colorCount == 0 && depthTexture == nullptr)
        return false;

    rpDesc.colorCount = colorCount;

    for (uint32_t i = 0; i < colorCount; ++i)
    {
        const auto& attachment = colorAttachments[i];
        if (attachment.texture == nullptr)
            return false;

        auto view = attachment.texture->getOrCreateAttachmentView (*oreCtx, attachment.view);
        if (view == nullptr)
            return false;

        auto& ca = rpDesc.colorAttachments[i];
        ca.view = view.get();
        ca.loadOp = honourClears ? toOreLoadOp (attachment.options.loadOp) : rive::ore::LoadOp::load;
        ca.storeOp = toOreStoreOp (attachment.options.storeOp);
        ca.clearColor = { attachment.options.clearColor.red,
                          attachment.options.clearColor.green,
                          attachment.options.clearColor.blue,
                          attachment.options.clearColor.alpha };

        framePools->liveViews.push_back (std::move (view));

        if (attachment.resolveTexture != nullptr)
        {
            auto resolveView = attachment.resolveTexture->getOrCreateAttachmentView (*oreCtx, attachment.resolveView);
            if (resolveView != nullptr)
            {
                ca.resolveTarget = resolveView.get();
                framePools->liveViews.push_back (std::move (resolveView));
            }
        }
    }

    if (depthTexture != nullptr)
    {
        auto view = depthTexture->getOrCreateAttachmentView (*oreCtx, depthView);
        if (view == nullptr)
            return false;

        auto& ds = rpDesc.depthStencil;
        ds.view = view.get();
        ds.depthLoadOp = honourClears ? toOreLoadOp (depthOptions.depthLoadOp) : rive::ore::LoadOp::load;
        ds.depthStoreOp = toOreStoreOp (depthOptions.depthStoreOp);
        ds.depthClearValue = depthOptions.depthClearValue;
        ds.stencilLoadOp = honourClears ? toOreLoadOp (depthOptions.stencilLoadOp) : rive::ore::LoadOp::load;
        ds.stencilStoreOp = toOreStoreOp (depthOptions.stencilStoreOp);
        ds.stencilClearValue = depthOptions.stencilClearValue;

        framePools->liveViews.push_back (std::move (view));
    }

    return true;
}

void GpuRenderPass::Impl::applyPassState (rive::ore::RenderPass& renderPass) const
{
    if (viewportSet)
    {
        renderPass.setViewport (viewportRect[0],
                                viewportRect[1],
                                viewportRect[2],
                                viewportRect[3],
                                viewportRect[4],
                                viewportRect[5]);
    }
    else
    {
        renderPass.setViewport (0.0f, 0.0f, (float) width, (float) height);
    }

    if (scissorSet)
        renderPass.setScissorRect (scissorRect[0], scissorRect[1], scissorRect[2], scissorRect[3]);

    if (stencilReferenceSet)
        renderPass.setStencilReference (stencilReference);

    if (blendColorSet)
        renderPass.setBlendColor (blendColor.red, blendColor.green, blendColor.blue, blendColor.alpha);
}

//==============================================================================

bool GpuRenderPass::Impl::layoutDeclares (const rive::ore::BindGroupLayout& layout, uint32_t binding, bool asTexture)
{
    for (const auto& e : layout.entries())
    {
        if (e.binding != binding)
            continue;

        return asTexture ? (e.kind == rive::ore::BindingKind::sampledTexture
                            || e.kind == rive::ore::BindingKind::storageTexture)
                         : (e.kind == rive::ore::BindingKind::uniformBuffer);
    }

    return false;
}

bool GpuRenderPass::Impl::hasConflictingTextureViews() const
{
    const auto isBound = [this] (const TextureBinding& tb)
    {
        return tb.texture != nullptr
            && tb.group >= 0
            && (size_t) tb.group < oreLayouts->size()
            && (*oreLayouts)[(size_t) tb.group] != nullptr
            && layoutDeclares (*(*oreLayouts)[(size_t) tb.group], (uint32_t) tb.binding, true);
    };

    for (size_t a = 0; a < textureBindings.size(); ++a)
    {
        const auto& first = textureBindings[a];
        if (! isBound (first))
            continue;

        for (size_t b = a + 1; b < textureBindings.size(); ++b)
        {
            const auto& second = textureBindings[b];
            if (second.texture != first.texture || ! isBound (second))
                continue;

            if (first.view.value_or (first.texture->getWholeTextureViewDesc()) != second.view.value_or (second.texture->getWholeTextureViewDesc()))
                return true;
        }
    }

    return false;
}

//==============================================================================

bool GpuRenderPass::Impl::writeDynamicUniforms()
{
    if (dynamicBindings == nullptr)
        return true;

    auto& ring = framePools->uniformRing;
    const auto isWritten = [&ring] (const UboBinding& ub) { return ub.ringChunk != 0 && ub.ringChunk == ring.chunkId; };

    uint32_t staleBytes = 0;
    uint32_t allBytes = 0;

    forEachDynamicUniform ([&] (const UboBinding& ub)
    {
        const auto bytes = ring.slotSize (ub.data.size());
        allBytes += bytes;

        if (! isWritten (ub))
            staleBytes += bytes;
    });

    if (staleBytes == 0)
        return true;

    if (! ring.fits (staleBytes) && ! framePools->startUniformChunk (allBytes))
    {
        YUP_DBG ("GpuRenderPass: no uniform buffer for the dynamic uniforms of a draw");
        return false;
    }

    forEachDynamicUniform ([&] (UboBinding& ub)
    {
        if (isWritten (ub))
            return;

        ub.ringOffset = framePools->writeUniforms (ub.data.data(), (uint32_t) ub.data.size());
        ub.ringChunk = ring.chunkId;
    });

    return true;
}

GpuRenderPass::Impl::DynamicOffsets GpuRenderPass::Impl::collectDynamicOffsets (uint32_t group, const rive::ore::BindGroup& bindGroup)
{
    DynamicOffsets offsets;
    if (dynamicBindings == nullptr || group >= dynamicBindings->size())
        return offsets;

    // Vulkan and WebGPU take an offset per dynamic entry of the layout, 0 for one left unbound;
    // Metal, OpenGL and Direct3D 11 one per dynamic uniform buffer the group holds
    const auto& bindings = (*dynamicBindings)[group];
    const bool perLayoutEntry = bindGroup.dynamicOffsetCount() == bindings.size();

    for (const auto binding : bindings)
    {
        const auto* ub = findUniformBuffer ((int) group, (int) binding);
        if (ub != nullptr || perLayoutEntry)
            offsets.values[offsets.count++] = ub != nullptr ? ub->ringOffset : 0;
    }

    return offsets;
}

bool GpuRenderPass::Impl::encode (uint32_t count,
                                  bool indexed,
                                  uint32_t instanceCount,
                                  uint32_t first,
                                  int32_t baseVertex,
                                  uint32_t firstInstance)
{
    if (oreCtx == nullptr || orePipeline == nullptr || framePools == nullptr || oreLayouts == nullptr)
        return false;

    if (indexed && indexOreBuffer == nullptr)
        return false;

    if (! ensurePassOpen())
        return false;

    const auto& layouts = *oreLayouts;

    if (! writeDynamicUniforms())
        return false;

    const auto ringChunk = framePools->uniformRing.chunkId;

    bindGroups.clear();

    const bool hasAnyBindings = ! uboBindings.empty() || ! textureBindings.empty() || ! storageBindings.empty();

#if YUP_DEBUG
    if (std::exchange (textureViewsChanged, false))
    {
        textureViewsConflict = hasConflictingTextureViews();
        ++textureViewChecks;
    }

    jassertquiet (! textureViewsConflict);
#endif

    for (uint32_t groupIdx = 0; groupIdx < layouts.size(); ++groupIdx)
    {
        auto* layout = layouts[groupIdx].get();
        if (layout == nullptr)
            continue;

        // Resources bound into a cached group were retained by this frame when it was built.
        auto* cache = groupIdx < groupCaches.size() ? &groupCaches[groupIdx] : nullptr;
        if (cache != nullptr && ! cache->dirty && storageHandlesMatch (groupIdx, *cache)
            && (cache->ringChunk == 0 || cache->ringChunk == ringChunk))
        {
            bindGroups.push_back ({ groupIdx, cache->group });
            continue;
        }

        const std::vector<GpuPipeline::Impl::SamplerBinding>* groupSamplers = nullptr;
        if (oreSamplers != nullptr && groupIdx < oreSamplers->size())
            groupSamplers = &(*oreSamplers)[groupIdx];

        const bool layoutHasSamplers = groupSamplers != nullptr && ! groupSamplers->empty();

        if (! hasAnyBindings && ! layoutHasSamplers)
            continue;

        // A group missing a declared resource is rebuilt by the next draw instead of being reused
        bool anySkipped = false;
        bool anyDynamic = false;

        // UBO entries for this group.
        uboEntries.clear();
        for (const auto& ub : uboBindings)
        {
            if (ub.group != (int) groupIdx || ! layoutDeclares (*layout, (uint32_t) ub.binding, false))
                continue;

            // The ring holds the block in its current chunk, and the bind moves to its offset
            if (layout->hasDynamicOffset ((uint32_t) ub.binding))
            {
                rive::ore::BindGroupDesc::UBOEntry entry;
                entry.slot = (uint32_t) ub.binding;
                entry.buffer = framePools->uniformRing.chunk.get();
                entry.offset = 0;
                entry.size = (uint32_t) ub.data.size();
                uboEntries.push_back (entry);
                anyDynamic = true;
                continue;
            }

            auto buf = framePools->acquireUniformBuffer (ub.data.data(), ub.data.size());
            if (buf == nullptr)
            {
                anySkipped = true;
                continue;
            }

            rive::ore::BindGroupDesc::UBOEntry entry;
            entry.slot = (uint32_t) ub.binding;
            entry.buffer = buf.get();
            entry.offset = 0;
            entry.size = (uint32_t) ub.data.size();
            uboEntries.push_back (entry);
        }

        // Texture entries for this group.
        texEntries.clear();
        for (const auto& tb : textureBindings)
        {
            if (tb.group != (int) groupIdx || tb.texture == nullptr || ! layoutDeclares (*layout, (uint32_t) tb.binding, true))
                continue;

            auto view = tb.view.has_value() ? tb.texture->getOrCreateView (*oreCtx, false, *tb.view)
                                            : tb.texture->getOrCreateSamplingView (*oreCtx);
            if (view == nullptr)
            {
                anySkipped = true;
                continue;
            }

            rive::ore::BindGroupDesc::TexEntry entry;
            entry.slot = (uint32_t) tb.binding;
            entry.view = view.get();
            texEntries.push_back (entry);
            framePools->liveViews.push_back (std::move (view));
        }

        // Storage buffer entries for this group. Ore takes native handles and does
        // not retain them, so the frame keeps the buffers alive, except on Vulkan
        // where Rive's resource purgatory keeps the native buffer instead.
        storageEntries.clear();
        for (const auto& sb : storageBindings)
        {
            if (sb.group != (int) groupIdx || sb.buffer == nullptr)
                continue;

            const auto* layoutEntry = layout->findEntry ((uint32_t) sb.binding);
            if (layoutEntry == nullptr)
                continue;

            // Render passes only read storage buffers: the shader must declare the block readonly.
            // Drawing with the declared buffer unbound is undefined on Vulkan and Metal, so skip the draw.
            jassert (layoutEntry->kind == rive::ore::BindingKind::storageBufferRO);
            if (layoutEntry->kind != rive::ore::BindingKind::storageBufferRO)
                return false;

            auto* bufferImpl = sb.buffer->getImpl();
            auto* nativeBuffer = bufferImpl != nullptr ? bufferImpl->getNativeStorageHandleForRead() : nullptr;
            if (nativeBuffer == nullptr)
            {
                YUP_DBG ("GpuRenderPass: storage buffer at group " << sb.group << " binding " << sb.binding << " has no native handle on this backend");
                jassertfalse;
                return false;
            }

            rive::ore::BindGroupDesc::StorageEntry entry;
            entry.slot = (uint32_t) sb.binding;
            entry.nativeBuffer = nativeBuffer;
            entry.offset = 0;
            entry.size = sb.buffer->getSizeInBytes();
            storageEntries.push_back (entry);

#if YUP_RIVE_USE_VULKAN
            // Reads and updates of the buffer wait for this frame. A dropped Vulkan buffer already
            // waits in Rive's resource purgatory until the GPU is done with it, and a retired frame
            // holding the GpuBuffer would keep the device alive through the buffer's reference
            if (auto& storage = bufferImpl->vkStorageBuffer; storage.buffer != nullptr)
            {
                // A frame begun before the last use's work must not move the stamp back
                if (framePools->generation > storage.lastUseGeneration)
                {
                    storage.lastUseCommands = static_cast<VkCommandBuffer> (framePools->commandBuffer);
                    storage.lastUseGeneration = framePools->generation;
                }

                continue;
            }
#endif

#if YUP_RIVE_USE_METAL && YUP_APPLE
            // An update while this frame may still read the buffer swaps its backing instead of waiting
            if (bufferImpl->mtlStorageBuffer != nil)
                bufferImpl->mtlUses.add ((__bridge id<MTLCommandBuffer>) framePools->commandBuffer);
#endif

            framePools->liveStorageBuffers.push_back (sb.buffer);
        }

        // Sampler entries: the pipeline's per-layout defaults, with any slot the
        // caller overrode via setSampler() swapped in.
        sampEntries.clear();
        if (groupSamplers != nullptr)
        {
            for (const auto& sb : *groupSamplers)
            {
                rive::rcp<rive::ore::Sampler> sampler = sb.sampler;

                for (const auto& userSampler : samplerBindings)
                {
                    if (userSampler.group != (int) groupIdx || (uint32_t) userSampler.binding != sb.binding)
                        continue;

                    if (userSampler.sampler != nullptr && userSampler.sampler->getOreSampler() != nullptr)
                        sampler = userSampler.sampler->getOreSampler();

                    break;
                }

                rive::ore::BindGroupDesc::SampEntry se;
                se.slot = sb.binding;
                se.sampler = sampler.get();
                sampEntries.push_back (se);
                framePools->liveSamplers.push_back (std::move (sampler));
            }
        }

        rive::ore::BindGroupDesc bgDesc;
        bgDesc.layout = layout;
        bgDesc.ubos = uboEntries.empty() ? nullptr : uboEntries.data();
        bgDesc.uboCount = (uint32_t) uboEntries.size();
        bgDesc.textures = texEntries.empty() ? nullptr : texEntries.data();
        bgDesc.textureCount = (uint32_t) texEntries.size();
        bgDesc.samplers = sampEntries.empty() ? nullptr : sampEntries.data();
        bgDesc.samplerCount = (uint32_t) sampEntries.size();
        bgDesc.storageBuffers = storageEntries.empty() ? nullptr : storageEntries.data();
        bgDesc.storageBufferCount = (uint32_t) storageEntries.size();

        auto bg = oreCtx->makeBindGroup (bgDesc);

        if (cache != nullptr)
        {
            cache->group = bg;
            cache->dirty = bg == nullptr || anySkipped;
            cache->ringChunk = anyDynamic ? ringChunk : 0;
            cache->storageHandles.clear();
            forEachStorageHandle (groupIdx, [cache] (void* handle) { cache->storageHandles.push_back (handle); });
        }

        if (bg == nullptr)
        {
            YUP_DBG ("GpuRenderPass: failed to create bind group " << (int) groupIdx << ": " << String (oreCtx->lastError()));
            continue;
        }

        bindGroups.push_back ({ groupIdx, std::move (bg) });
    }

    auto& renderPass = *orePass;

    if (! backendKeepsState)
    {
        encodedPipeline = nullptr;
        encodedGroups = {};
        passStateDirty = true;
    }

    // A pipeline switch can invalidate the bound groups (a new root signature on D3D12, a new
    // layout on Vulkan), so they and the pass state are encoded again after it
    if (encodedPipeline.get() != orePipeline)
    {
        oreCtx->clearLastError();
        renderPass.setPipeline (orePipeline);
        if (! oreCtx->lastError().empty())
        {
            YUP_DBG ("GpuRenderPass: incompatible pipeline: " << String (oreCtx->lastError()));
            jassertfalse;
            return false;
        }

        encodedPipeline = rive::ref_rcp (orePipeline);
        encodedGroups = {};
        passStateDirty = true;
    }

    if (std::exchange (passStateDirty, false))
        applyPassState (renderPass);

    for (auto& [groupIdx, bg] : bindGroups)
    {
        const auto offsets = collectDynamicOffsets (groupIdx, *bg);

        const auto tracked = groupIdx < encodedGroups.size();
        if (tracked && encodedGroups[groupIdx] == bg && encodedOffsets[groupIdx] == offsets)
            continue;

#if YUP_DEBUG
        if (offsets.count > 0)
        {
            std::string error;
            const bool valid = rive::ore::validateSetBindGroup (groupIdx, bg.get(), offsets.values.data(), offsets.count, framePools->uniformRing.alignment, &error);
            if (! valid)
                YUP_DBG ("GpuRenderPass: " << String (error));

            jassert (valid);
        }
#endif

        oreCtx->clearLastError();
        renderPass.setBindGroup (groupIdx, bg.get(), offsets.count > 0 ? offsets.values.data() : nullptr, offsets.count);

        // A failed bind (a descriptor allocation on Vulkan or WebGPU) stays unrecorded, for the next draw to retry
        if (tracked)
        {
            encodedGroups[groupIdx] = oreCtx->lastError().empty() ? bg : nullptr;
            encodedOffsets[groupIdx] = offsets;
        }
    }

    for (const auto& vb : vertexBindings)
    {
        if (vb.oreBuffer != nullptr)
            renderPass.setVertexBuffer ((uint32_t) vb.slot, vb.oreBuffer, 0);
    }

    if (indexed)
    {
        renderPass.setIndexBuffer (indexOreBuffer, indexFormat, 0);
        renderPass.drawIndexed (count, instanceCount, first, baseVertex, firstInstance);
    }
    else
    {
        renderPass.draw (count, instanceCount, first, firstInstance);
    }

    ++encodeCount;
    return true;
}

//==============================================================================

GpuRenderPass::Impl* GpuRenderPass::getImpl() noexcept
{
    static_assert (sizeof (Impl) <= ImplSizeBytes, "GpuRenderPass::ImplSizeBytes is too small for GpuRenderPass::Impl");

    return impl.getPayload<Impl>();
}

const GpuRenderPass::Impl* GpuRenderPass::getImpl() const noexcept
{
    return impl.getPayload<Impl>();
}

//==============================================================================

GpuRenderPass::GpuRenderPass (GpuRenderPass&&) noexcept = default;

GpuRenderPass& GpuRenderPass::operator= (GpuRenderPass&& other) noexcept
{
    if (this != &other)
    {
        finish();
        impl = std::move (other.impl);
    }

    return *this;
}

GpuRenderPass::~GpuRenderPass()
{
    finish();
}

//==============================================================================

bool GpuRenderPass::isValid() const noexcept
{
    auto* i = getImpl();
    return i != nullptr && i->oreCtx != nullptr && ! i->finished;
}

//==============================================================================

void GpuRenderPass::setPipeline (GpuPipeline::Ptr pipeline)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    // Held until the comparison below is done, so the outgoing pipeline's layouts stay alive
    const auto previous = std::exchange (i->pipelineRef, pipeline);
    const auto* previousImpl = previous != nullptr ? previous->getImpl() : nullptr;
    const auto* pipeImpl = pipeline->getImpl();

    if (previous != pipeline)
        i->textureViewsChanged = true;

    if (pipeImpl != nullptr)
    {
        i->orePipeline = pipeImpl->pipeline.get();
        i->oreLayouts = &pipeImpl->layouts;
        i->oreSamplers = &pipeImpl->samplersPerGroup;
        i->dynamicBindings = &pipeImpl->dynamicBindingsPerGroup;
    }
    else
    {
        i->orePipeline = nullptr;
        i->oreLayouts = nullptr;
        i->oreSamplers = nullptr;
        i->dynamicBindings = nullptr;
    }

    // A group stays valid only while every switch keeps its layout entries and default samplers
    for (size_t group = 0; group < i->groupCaches.size(); ++group)
    {
        if (pipeImpl == nullptr || previousImpl == nullptr || ! pipeImpl->sharesGroupWith (*previousImpl, group))
            i->groupCaches[group].dirty = true;
    }
}

void GpuRenderPass::setTexture (int group, int binding, GpuTexture::Ptr texture)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    for (auto& tb : i->textureBindings)
    {
        if (tb.group == group && tb.binding == binding)
        {
            if (tb.texture == texture && ! tb.view.has_value())
                return;

            tb.texture = std::move (texture);
            tb.view = std::nullopt;
            i->textureViewsChanged = true;
            i->markGroupDirty (group, binding);
            return;
        }
    }

    i->textureViewsChanged = true;
    i->markGroupDirty (group, binding);
    i->textureBindings.push_back ({ group, binding, std::move (texture), std::nullopt });
}

void GpuRenderPass::setTexture (int group, int binding, GpuTexture::Ptr texture, const GpuTextureViewDesc& view)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    const bool inRange = texture == nullptr
                      || (view.mipCount > 0
                          && view.layerCount > 0
                          && view.baseMipLevel + view.mipCount <= texture->getMipLevels()
                          && view.baseLayer + view.layerCount <= texture->getDepthOrArrayLayers());

    jassert (inRange);
    if (! inRange)
        return;

    for (auto& tb : i->textureBindings)
    {
        if (tb.group == group && tb.binding == binding)
        {
            if (tb.texture == texture && tb.view == view)
                return;

            tb.texture = std::move (texture);
            tb.view = view;
            i->textureViewsChanged = true;
            i->markGroupDirty (group, binding);
            return;
        }
    }

    i->textureViewsChanged = true;
    i->markGroupDirty (group, binding);
    i->textureBindings.push_back ({ group, binding, std::move (texture), view });
}

void GpuRenderPass::setSampler (int group, int binding, GpuSampler::Ptr sampler)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    for (auto& sb : i->samplerBindings)
    {
        if (sb.group == group && sb.binding == binding)
        {
            if (sb.sampler == sampler)
                return;

            sb.sampler = std::move (sampler);
            i->markGroupDirty (group, binding);
            return;
        }
    }

    i->markGroupDirty (group, binding);
    i->samplerBindings.push_back ({ group, binding, std::move (sampler) });
}

void GpuRenderPass::setUniformBuffer (int group, int binding, const void* data, size_t byteSize)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    jassert (data != nullptr && byteSize > 0);
    if (data == nullptr || byteSize == 0)
        return;

    const auto* bytes = static_cast<const uint8_t*> (data);

    for (auto& ub : i->uboBindings)
    {
        if (ub.group == group && ub.binding == binding)
        {
            if (std::equal (ub.data.begin(), ub.data.end(), bytes, bytes + byteSize))
                return;

            const bool resized = ub.data.size() != byteSize;
            ub.data.assign (bytes, bytes + byteSize);
            ub.ringChunk = 0;

            // A dynamic block only moves its offset, unless its size, which the group holds, changed
            if (resized || ! i->declaresDynamic (group, binding))
                i->markGroupDirty (group, binding);

            return;
        }
    }

    i->markGroupDirty (group, binding);

    Impl::UboBinding ub;
    ub.group = group;
    ub.binding = binding;
    ub.data.assign (bytes, bytes + byteSize);
    i->uboBindings.push_back (std::move (ub));
}

void GpuRenderPass::setStorageBuffer (int group, int binding, GpuBuffer::Ptr buffer)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    const bool supported = i->framePools != nullptr && i->framePools->device != nullptr && i->framePools->device->isFragmentStorageBufferAvailable();
    jassert (supported);
    if (! supported)
        return;

    jassert (buffer == nullptr || buffer->getType() == GpuBufferType::storage);
    if (buffer != nullptr && buffer->getType() != GpuBufferType::storage)
        return;

    for (auto& sb : i->storageBindings)
    {
        if (sb.group == group && sb.binding == binding)
        {
            if (sb.buffer == buffer)
                return;

            sb.buffer = std::move (buffer);
            i->markGroupDirty (group, binding);
            return;
        }
    }

    i->markGroupDirty (group, binding);
    i->storageBindings.push_back ({ group, binding, std::move (buffer) });
}

void GpuRenderPass::setVertexBuffer (int slot, GpuBuffer::Ptr buffer)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    auto* ore = (buffer != nullptr && buffer->getImpl() != nullptr) ? buffer->getImpl()->oreBuffer.get() : nullptr;

    for (auto& vb : i->vertexBindings)
    {
        if (vb.slot == slot)
        {
            vb.buffer = std::move (buffer);
            vb.oreBuffer = ore;
            return;
        }
    }

    i->vertexBindings.push_back ({ slot, std::move (buffer), ore });
}

void GpuRenderPass::setIndexBuffer (GpuIndexFormat format, GpuBuffer::Ptr buffer)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    i->indexOreBuffer = (buffer != nullptr && buffer->getImpl() != nullptr) ? buffer->getImpl()->oreBuffer.get() : nullptr;
    i->indexBuffer = std::move (buffer);
    i->indexFormat = GpuPipelineHelpers::toOreIndexFormat (format);
}

//==============================================================================

void GpuRenderPass::setColorAttachment (int index,
                                        GpuTexture::Ptr texture,
                                        const GpuRenderOptions& options,
                                        const GpuTextureViewDesc& view)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    // Attachments are baked into the backend pass when it opens, which the first
    // draw does - setting one afterwards would silently do nothing.
    jassert (i->orePass == nullptr);

    // Index 0 belongs to the surface the pass was begun on; a depth-only pass takes none.
    jassert (index >= 1 && index < 4 && i->colorCount > 0);
    if (index < 1 || index >= 4 || i->colorCount == 0)
        return;

    jassert (texture == nullptr || texture->isRenderTarget());
    i->colorAttachments[index].texture = std::move (texture);
    i->colorAttachments[index].view = view;
    i->colorAttachments[index].options = options;

    uint32_t count = 1;
    for (uint32_t n = 0; n < 4; ++n)
    {
        if (i->colorAttachments[n].texture != nullptr)
            count = n + 1;
    }

    i->colorCount = count;
}

void GpuRenderPass::setDepthStencilAttachment (GpuTexture::Ptr texture,
                                               const GpuDepthStencilOptions& options,
                                               const GpuTextureViewDesc& view)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    jassert (i->orePass == nullptr);
    jassert (texture == nullptr || isDepthStencilFormat (texture->getFormat()));
    jassert (texture == nullptr || texture->isRenderTarget());

    // The depth attachment of a depth-only pass is the surface it was begun on.
    jassert (i->colorCount > 0);
    if (i->colorCount == 0)
        return;

    i->depthTexture = std::move (texture);
    i->depthView = view;
    i->depthOptions = options;
}

void GpuRenderPass::setResolveTarget (int index, GpuTexture::Ptr texture, const GpuTextureViewDesc& view)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    jassert (i->orePass == nullptr);
    jassert (index >= 0 && index < 4 && i->colorCount > 0);
    if (index < 0 || index >= 4 || i->colorCount == 0)
        return;

    i->colorAttachments[index].resolveTexture = std::move (texture);
    i->colorAttachments[index].resolveView = view;
}

//==============================================================================

void GpuRenderPass::setViewport (float x, float y, float width, float height, float minDepth, float maxDepth)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    i->viewportSet = true;
    i->passStateDirty = true;
    i->viewportRect[0] = x;
    i->viewportRect[1] = y;
    i->viewportRect[2] = width;
    i->viewportRect[3] = height;
    i->viewportRect[4] = minDepth;
    i->viewportRect[5] = maxDepth;
}

void GpuRenderPass::setScissorRect (int x, int y, int width, int height)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    jassert (x >= 0 && y >= 0 && width >= 0 && height >= 0);

    i->scissorSet = true;
    i->passStateDirty = true;
    i->scissorRect[0] = (uint32_t) jmax (0, x);
    i->scissorRect[1] = (uint32_t) jmax (0, y);
    i->scissorRect[2] = (uint32_t) jmax (0, width);
    i->scissorRect[3] = (uint32_t) jmax (0, height);
}

void GpuRenderPass::setStencilReference (uint32_t reference)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    i->stencilReferenceSet = true;
    i->passStateDirty = true;
    i->stencilReference = reference;
}

void GpuRenderPass::setBlendColor (GpuColor color)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    i->blendColorSet = true;
    i->passStateDirty = true;
    i->blendColor = color;
}

//==============================================================================

bool GpuRenderPass::draw (uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    if (! isValid())
        return false;

    return getImpl()->encode (vertexCount, false, instanceCount, firstVertex, 0, firstInstance);
}

bool GpuRenderPass::drawIndexed (uint32_t indexCount,
                                 uint32_t instanceCount,
                                 uint32_t firstIndex,
                                 int32_t baseVertex,
                                 uint32_t firstInstance)
{
    if (! isValid())
        return false;

    return getImpl()->encode (indexCount, true, instanceCount, firstIndex, baseVertex, firstInstance);
}

//==============================================================================

bool GpuRenderPass::finish()
{
    auto* i = getImpl();
    if (i == nullptr || i->finished)
        return false;

    const bool wantsClear = (i->colorCount > 0 && i->colorAttachments[0].options.loadOp == GpuLoadOp::clear)
                         || (i->depthTexture != nullptr && i->depthOptions.depthLoadOp == GpuLoadOp::clear);

    if (i->encodeCount == 0 && wantsClear)
        i->ensurePassOpen();

    i->closePass();

    i->finished = true;
    return true;
}

int GpuRenderPass::getTextureViewChecksForTesting() const noexcept
{
    const auto* i = getImpl();
    return i != nullptr ? i->textureViewChecks : 0;
}

} // namespace yup
