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

    rive::ore::Context* oreCtx = nullptr;
    GpuFrame::Impl* framePools = nullptr;

    GpuTexture::Ptr outputTexture;
    int width = 0;
    int height = 0;
    GpuRenderOptions options;

    // Resolved from the bound GpuPipeline by GpuRenderPass::setPipeline (which
    // is a friend of GpuPipeline). Kept alive by pipelineRef.
    GpuPipeline::Ptr pipelineRef;
    rive::ore::Pipeline* orePipeline = nullptr;
    const std::vector<rive::rcp<rive::ore::BindGroupLayout>>* oreLayouts = nullptr;

    bool finished = false;

    std::vector<TextureBinding> textureBindings;
    std::vector<UboBinding> uboBindings;
    std::vector<VertexBinding> vertexBindings;

    GpuBuffer::Ptr indexBuffer;
    rive::ore::Buffer* indexOreBuffer = nullptr;
    rive::ore::IndexFormat indexFormat = rive::ore::IndexFormat::none;

    bool encode (uint32_t count, bool indexed);

    // Creates an ore TextureView for a GpuTexture. The correct view kind depends
    // on how the texture is used, and the preference order differs between color
    // attachments and sampled inputs:
    //
    // - Color attachments must be bound through a render-target view. On D3D the
    //   canvas wrapper (wrapCanvasTexture) exposes the RTV; binding an SRV-backed
    //   rive-texture view instead leaves no RTV bound and the draw is discarded
    //   (DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET). Prefer wrapCanvasTexture, and
    //   fall back to the underlying GPU texture.
    // - Sampled inputs must be bound through an SRV-backed view. wrapCanvasTexture
    //   only exposes a render-target view, which has no shader-resource view on
    //   D3D - sampling it reads nothing. Prefer the underlying GPU texture, which
    //   wrapRiveTexture() wraps with a proper SRV, and fall back to the canvas
    //   view.
    //
    // This is a member of the nested Impl so it can read GpuTexture internals:
    // GpuRenderPass is a friend of GpuTexture, and a nested class shares the
    // enclosing class's access rights (C++11).
    static rive::rcp<rive::ore::TextureView> createView (rive::ore::Context& oreCtx, const GpuTexture& tex, bool forRenderTarget)
    {
        if (forRenderTarget)
        {
            if (auto rc = tex.getInternalRenderCanvas())
                return oreCtx.wrapCanvasTexture (rc.get());

            if (auto gpuTex = tex.getOrAdoptGpuTexture())
                return oreCtx.wrapRiveTexture (gpuTex.get(), (uint32_t) tex.getWidth(), (uint32_t) tex.getHeight());

            return nullptr;
        }

        // Prefer the Y-flipped mirror for GL/GLES canvas textures so sampled
        // inputs present UV.y=0=top (matching Metal/D3D convention) without
        // requiring backend-specific UV logic in user shaders.
        if (tex.sampledTexture != nullptr)
            return oreCtx.wrapRiveTexture (tex.sampledTexture.get(), (uint32_t) tex.getWidth(), (uint32_t) tex.getHeight());

        if (auto gpuTex = tex.getOrAdoptGpuTexture())
            return oreCtx.wrapRiveTexture (gpuTex.get(), (uint32_t) tex.getWidth(), (uint32_t) tex.getHeight());

        if (auto rc = tex.getInternalRenderCanvas())
            return oreCtx.wrapCanvasTexture (rc.get());

        return nullptr;
    }
};

//==============================================================================

bool GpuRenderPass::Impl::encode (uint32_t count, bool indexed)
{
    if (oreCtx == nullptr || orePipeline == nullptr || framePools == nullptr || oreLayouts == nullptr)
        return false;

    if (indexed && indexOreBuffer == nullptr)
        return false;

    if (outputTexture == nullptr)
        return false;

    auto outputView = createView (*oreCtx, *outputTexture, true);
    if (outputView == nullptr)
        return false;

    framePools->liveViews.push_back (outputView);

    const auto& layouts = *oreLayouts;

    std::vector<std::pair<uint32_t, rive::rcp<rive::ore::BindGroup>>> bindGroups;

    // Fast path: skip bind-group creation when there are no UBOs, textures,
    // or samplers to bind (common for simple vertex-only draws).
    const bool hasAnyBindings = ! uboBindings.empty() || ! textureBindings.empty();

    for (uint32_t groupIdx = 0; groupIdx < layouts.size(); ++groupIdx)
    {
        auto* layout = layouts[groupIdx].get();
        if (layout == nullptr)
            continue;

        // Check if this layout declares any sampler slots we'd need to fill.
        bool layoutHasSamplers = false;
        if (! hasAnyBindings)
        {
            for (const auto& entry : layout->entries())
            {
                if (entry.kind == rive::ore::BindingKind::sampler
                    || entry.kind == rive::ore::BindingKind::comparisonSampler)
                {
                    layoutHasSamplers = true;
                    break;
                }
            }
        }

        if (! hasAnyBindings && ! layoutHasSamplers)
            continue;

        // UBO entries for this group.
        std::vector<rive::ore::BindGroupDesc::UBOEntry> uboEntries;

        for (const auto& ub : uboBindings)
        {
            if (ub.group != (int) groupIdx)
                continue;

            rive::ore::BufferDesc bufDesc;
            bufDesc.usage = rive::ore::BufferUsage::uniform;
            bufDesc.size = (uint32_t) ub.data.size();
            bufDesc.data = ub.data.data();
            bufDesc.immutable = true;

            auto buf = oreCtx->makeBuffer (bufDesc);
            if (buf == nullptr)
                continue;

            rive::ore::BindGroupDesc::UBOEntry entry;
            entry.slot = (uint32_t) ub.binding;
            entry.buffer = buf.get();
            entry.offset = 0;
            entry.size = (uint32_t) ub.data.size();
            uboEntries.push_back (entry);
            framePools->liveBuffers.push_back (std::move (buf));
        }

        // Texture entries for this group.
        std::vector<rive::ore::BindGroupDesc::TexEntry> texEntries;

        for (const auto& tb : textureBindings)
        {
            if (tb.group != (int) groupIdx || tb.texture == nullptr)
                continue;

            auto view = createView (*oreCtx, *tb.texture, false);
            if (view == nullptr)
                continue;

            rive::ore::BindGroupDesc::TexEntry entry;
            entry.slot = (uint32_t) tb.binding;
            entry.view = view.get();
            texEntries.push_back (entry);
            framePools->liveViews.push_back (std::move (view));
        }

        // Sampler entries - auto-create one linear+clamp sampler for each
        // sampler binding declared in the layout.
        std::vector<rive::ore::BindGroupDesc::SampEntry> sampEntries;

        for (const auto& layoutEntry : layout->entries())
        {
            if (layoutEntry.kind != rive::ore::BindingKind::sampler
                && layoutEntry.kind != rive::ore::BindingKind::comparisonSampler)
            {
                continue;
            }
            rive::ore::SamplerDesc sd;
            sd.minFilter = rive::ore::Filter::linear;
            sd.magFilter = rive::ore::Filter::linear;
            sd.wrapU = rive::ore::WrapMode::clampToEdge;
            sd.wrapV = rive::ore::WrapMode::clampToEdge;

            auto samp = oreCtx->makeSampler (sd);
            if (samp == nullptr)
                continue;

            rive::ore::BindGroupDesc::SampEntry se;
            se.slot = layoutEntry.binding;
            se.sampler = samp.get();
            sampEntries.push_back (se);
            framePools->liveSamplers.push_back (std::move (samp));
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
        if (bg != nullptr)
            bindGroups.push_back ({ groupIdx, std::move (bg) });
    }

    // Encode the render pass into the current frame.
    rive::ore::RenderPassDesc rpDesc;
    rpDesc.colorCount = 1;
    rpDesc.colorAttachments[0].view = outputView.get();
    rpDesc.colorAttachments[0].loadOp = options.clear ? rive::ore::LoadOp::clear : rive::ore::LoadOp::load;
    rpDesc.colorAttachments[0].storeOp = rive::ore::StoreOp::store;
    rpDesc.colorAttachments[0].clearColor = { options.clearColor.red,
                                              options.clearColor.green,
                                              options.clearColor.blue,
                                              options.clearColor.alpha };

    auto renderPass = oreCtx->beginRenderPass (rpDesc);
    renderPass->setPipeline (orePipeline);
    renderPass->setViewport (0.0f, 0.0f, (float) width, (float) height);

    for (auto& [groupIdx, bg] : bindGroups)
        renderPass->setBindGroup (groupIdx, bg.get());

    for (const auto& vb : vertexBindings)
    {
        if (vb.oreBuffer != nullptr)
            renderPass->setVertexBuffer ((uint32_t) vb.slot, vb.oreBuffer, 0);
    }

    if (indexed)
    {
        renderPass->setIndexBuffer (indexOreBuffer, indexFormat, 0);
        renderPass->drawIndexed (count);
    }
    else
    {
        renderPass->draw (count);
    }

    renderPass->finish();
    return true;
}

//==============================================================================

GpuRenderPass::Impl* GpuRenderPass::getImpl() noexcept
{
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
    }
    else
    {
        i->orePipeline = nullptr;
        i->oreLayouts = nullptr;
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

bool GpuRenderPass::draw (uint32_t vertexCount)
{
    if (! isValid())
        return false;

    return getImpl()->encode (vertexCount, false);
}

bool GpuRenderPass::drawIndexed (uint32_t indexCount)
{
    if (! isValid())
        return false;

    return getImpl()->encode (indexCount, true);
}

//==============================================================================

bool GpuRenderPass::finish()
{
    auto* i = getImpl();
    if (i == nullptr || i->finished)
        return false;

    // When a clear was requested but no draw was submitted (no pipeline set),
    // encode a clear-only render pass so the framebuffer is actually cleared.
    if (i->options.clear && i->orePipeline == nullptr && i->oreCtx != nullptr && i->outputTexture != nullptr)
    {
        auto outputView = Impl::createView (*i->oreCtx, *i->outputTexture, true);
        if (outputView != nullptr)
        {
            rive::ore::RenderPassDesc rpDesc;
            rpDesc.colorCount = 1;
            rpDesc.colorAttachments[0].view = outputView.get();
            rpDesc.colorAttachments[0].loadOp = rive::ore::LoadOp::clear;
            rpDesc.colorAttachments[0].storeOp = rive::ore::StoreOp::store;
            rpDesc.colorAttachments[0].clearColor = { i->options.clearColor.red,
                                                      i->options.clearColor.green,
                                                      i->options.clearColor.blue,
                                                      i->options.clearColor.alpha };

            auto renderPass = i->oreCtx->beginRenderPass (rpDesc);
            renderPass->finish();
        }
    }

    i->finished = true;
    return true;
}

} // namespace yup
