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

    rive::ore::Context* oreCtx = nullptr;
    GpuFrame::Impl* framePools = nullptr;

    // Attachment 0 is the surface the pass was begun on; 1..3 are MRT extras.
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
    std::vector<VertexBinding> vertexBindings;

    GpuBuffer::Ptr indexBuffer;
    rive::ore::Buffer* indexOreBuffer = nullptr;
    rive::ore::IndexFormat indexFormat = rive::ore::IndexFormat::none;

    /** The backend render pass every draw of this GpuRenderPass encodes into.

        Opened lazily by the first draw and held until finish(), so a pass costs
        one command encoder rather than one per draw. */
    std::unique_ptr<rive::ore::RenderPass> orePass;

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

    orePass = oreCtx->beginRenderPass (rpDesc);
    if (orePass == nullptr)
        return false;

    oreCtx->setActiveRenderPass (orePass.get());
    return true;
}

void GpuRenderPass::Impl::closePass()
{
    if (orePass == nullptr)
        return;

    orePass->finish();

    if (oreCtx != nullptr && oreCtx->activeRenderPass() == orePass.get())
        oreCtx->setActiveRenderPass (nullptr);

    orePass = nullptr;
}

//==============================================================================

bool GpuRenderPass::Impl::buildRenderPassDesc (rive::ore::RenderPassDesc& rpDesc)
{
    using namespace GpuPipelineHelpers;

    // Only the first encoded draw honours a clear; the rest load, so multiple
    // draws accumulate into one surface and share one depth buffer.
    const bool honourClears = encodeCount == 0;

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

    std::vector<std::pair<uint32_t, rive::rcp<rive::ore::BindGroup>>> bindGroups;

    const bool hasAnyBindings = ! uboBindings.empty() || ! textureBindings.empty();

    for (uint32_t groupIdx = 0; groupIdx < layouts.size(); ++groupIdx)
    {
        auto* layout = layouts[groupIdx].get();
        if (layout == nullptr)
            continue;

        const std::vector<GpuPipeline::Impl::SamplerBinding>* groupSamplers = nullptr;
        if (oreSamplers != nullptr && groupIdx < oreSamplers->size())
            groupSamplers = &(*oreSamplers)[groupIdx];

        const bool layoutHasSamplers = groupSamplers != nullptr && ! groupSamplers->empty();

        if (! hasAnyBindings && ! layoutHasSamplers)
            continue;

        auto layoutDeclares = [layout] (uint32_t binding, bool asTexture)
        {
            for (const auto& e : layout->entries())
            {
                if (e.binding != binding)
                    continue;

                return asTexture ? (e.kind == rive::ore::BindingKind::sampledTexture
                                    || e.kind == rive::ore::BindingKind::storageTexture)
                                 : (e.kind == rive::ore::BindingKind::uniformBuffer);
            }

            return false;
        };

        // UBO entries for this group.
        std::vector<rive::ore::BindGroupDesc::UBOEntry> uboEntries;
        for (const auto& ub : uboBindings)
        {
            if (ub.group != (int) groupIdx || ! layoutDeclares ((uint32_t) ub.binding, false))
                continue;

            auto buf = framePools->acquireUniformBuffer (ub.data.data(), ub.data.size());
            if (buf == nullptr)
                continue;

            rive::ore::BindGroupDesc::UBOEntry entry;
            entry.slot = (uint32_t) ub.binding;
            entry.buffer = buf.get();
            entry.offset = 0;
            entry.size = (uint32_t) ub.data.size();
            uboEntries.push_back (entry);
        }

        // Texture entries for this group.
        std::vector<rive::ore::BindGroupDesc::TexEntry> texEntries;
        for (const auto& tb : textureBindings)
        {
            if (tb.group != (int) groupIdx || tb.texture == nullptr || ! layoutDeclares ((uint32_t) tb.binding, true))
                continue;

            auto view = tb.texture->getOrCreateSamplingView (*oreCtx);
            if (view == nullptr)
                continue;

            rive::ore::BindGroupDesc::TexEntry entry;
            entry.slot = (uint32_t) tb.binding;
            entry.view = view.get();
            texEntries.push_back (entry);
            framePools->liveViews.push_back (std::move (view));
        }

        // Sampler entries: the pipeline's per-layout defaults, with any slot the
        // caller overrode via setSampler() swapped in.
        std::vector<rive::ore::BindGroupDesc::SampEntry> sampEntries;
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

        auto bg = oreCtx->makeBindGroup (bgDesc);

        if (bg == nullptr)
        {
            YUP_DBG ("GpuRenderPass: failed to create bind group " << (int) groupIdx << ": " << String (oreCtx->lastError()));
            continue;
        }

        bindGroups.push_back ({ groupIdx, std::move (bg) });
    }

    auto& renderPass = *orePass;

    oreCtx->clearLastError();
    renderPass.setPipeline (orePipeline);
    if (! oreCtx->lastError().empty())
    {
        YUP_DBG ("GpuRenderPass: incompatible pipeline: " << String (oreCtx->lastError()));
        jassertfalse;
        return false;
    }

    applyPassState (renderPass);

    for (auto& [groupIdx, bg] : bindGroups)
        renderPass.setBindGroup (groupIdx, bg.get());

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

    i->pipelineRef = pipeline;

    if (auto* pipeImpl = pipeline->getImpl())
    {
        i->orePipeline = pipeImpl->pipeline.get();
        i->oreLayouts = &pipeImpl->layouts;
        i->oreSamplers = &pipeImpl->samplersPerGroup;
    }
    else
    {
        i->orePipeline = nullptr;
        i->oreLayouts = nullptr;
        i->oreSamplers = nullptr;
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
            tb.texture = std::move (texture);
            return;
        }
    }

    i->textureBindings.push_back ({ group, binding, std::move (texture) });
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
            sb.sampler = std::move (sampler);
            return;
        }
    }

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

    for (auto& ub : i->uboBindings)
    {
        if (ub.group == group && ub.binding == binding)
        {
            ub.data.assign (static_cast<const uint8_t*> (data), static_cast<const uint8_t*> (data) + byteSize);
            return;
        }
    }

    Impl::UboBinding ub;
    ub.group = group;
    ub.binding = binding;
    ub.data.assign (static_cast<const uint8_t*> (data), static_cast<const uint8_t*> (data) + byteSize);
    i->uboBindings.push_back (std::move (ub));
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

    // Index 0 belongs to the surface the pass was begun on.
    jassert (index >= 1 && index < 4);
    if (index < 1 || index >= 4)
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
    jassert (index >= 0 && index < 4);
    if (index < 0 || index >= 4)
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
    i->stencilReference = reference;
}

void GpuRenderPass::setBlendColor (GpuColor color)
{
    auto* i = getImpl();
    if (i == nullptr)
        return;

    i->blendColorSet = true;
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

    const bool wantsClear = i->colorAttachments[0].options.loadOp == GpuLoadOp::clear
                         || (i->depthTexture != nullptr && i->depthOptions.depthLoadOp == GpuLoadOp::clear);

    if (i->encodeCount == 0 && wantsClear)
        i->ensurePassOpen();

    i->closePass();

    i->finished = true;
    return true;
}

} // namespace yup
