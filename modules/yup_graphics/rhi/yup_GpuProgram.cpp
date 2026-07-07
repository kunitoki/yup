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

struct GpuProgram::Impl
{
    struct TextureBinding
    {
        int group;
        int binding;
        Texture::Ptr texture;
    };

    struct UboBinding
    {
        int group;
        int binding;
        std::vector<uint8_t> data;
    };

    rive::ore::Context* oreCtx = nullptr;
    rive::rcp<rive::ore::ShaderModule> vertModule;
    rive::rcp<rive::ore::ShaderModule> fragModule;
    rive::rcp<rive::ore::Pipeline> pipeline;
    std::vector<rive::rcp<rive::ore::BindGroupLayout>> layouts; // indexed by group; may contain null entries

    std::vector<TextureBinding> textureBindings;
    std::vector<UboBinding> uboBindings;

    // Resources that must remain alive from dispatch() until waitForGPU() completes.
    // Cleared by beginFrame() (safe: GPU finished before a new frame starts) and
    // by waitForGPU() (safe: GPU has synchronised).
    std::vector<rive::rcp<rive::ore::Buffer>> liveBuffers;
    std::vector<rive::rcp<rive::ore::TextureView>> liveViews;
    std::vector<rive::rcp<rive::ore::Sampler>> liveSamplers;
};

//==============================================================================

GpuProgram::~GpuProgram() = default;

//==============================================================================

void GpuProgram::setTexture (int group, int binding, Texture::Ptr texture)
{
    jassert (impl != nullptr);
    if (impl == nullptr)
        return;

    for (auto& tb : impl->textureBindings)
    {
        if (tb.group == group && tb.binding == binding)
        {
            tb.texture = std::move (texture);
            return;
        }
    }

    impl->textureBindings.push_back ({ group, binding, std::move (texture) });
}

void GpuProgram::setUniformBuffer (int group, int binding, const void* data, size_t byteSize)
{
    jassert (impl != nullptr);
    jassert (data != nullptr && byteSize > 0);
    if (impl == nullptr || data == nullptr || byteSize == 0)
        return;

    for (auto& ub : impl->uboBindings)
    {
        if (ub.group == group && ub.binding == binding)
        {
            ub.data.assign (static_cast<const uint8_t*> (data),
                            static_cast<const uint8_t*> (data) + byteSize);
            return;
        }
    }

    Impl::UboBinding ub;
    ub.group = group;
    ub.binding = binding;
    ub.data.assign (static_cast<const uint8_t*> (data),
                    static_cast<const uint8_t*> (data) + byteSize);
    impl->uboBindings.push_back (std::move (ub));
}

//==============================================================================

bool GpuProgram::beginFrame()
{
    if (impl == nullptr || impl->oreCtx == nullptr)
        return false;

    // Drop resources from any prior frame before beginning a new one.
    // By convention the caller must not beginFrame() again before waitForGPU().
    impl->liveBuffers.clear();
    impl->liveViews.clear();
    impl->liveSamplers.clear();

    impl->oreCtx->beginFrame ({});
    return true;
}

bool GpuProgram::endFrame()
{
    if (impl == nullptr || impl->oreCtx == nullptr)
        return false;

    impl->oreCtx->endFrame();
    return true;
}

void GpuProgram::waitForGPU()
{
    if (impl == nullptr || impl->oreCtx == nullptr)
        return;

    impl->oreCtx->waitForGPU();

    // GPU has finished; safe to release all transient resources.
    impl->liveBuffers.clear();
    impl->liveViews.clear();
    impl->liveSamplers.clear();
}

//==============================================================================

bool GpuProgram::dispatch (GpuCanvas& output)
{
    if (impl == nullptr || impl->oreCtx == nullptr || impl->pipeline == nullptr)
        return false;

    auto* oreCtx = impl->oreCtx;

    auto outputTex = output.asTexture();
    if (outputTex == nullptr)
        return false;

    rive::rcp<rive::ore::TextureView> outputView;
    if (auto rc = outputTex->getInternalRenderCanvas())
        outputView = oreCtx->wrapCanvasTexture (rc.get());
    else if (auto gpuTex = outputTex->getOrAdoptGpuTexture())
        outputView = oreCtx->wrapRiveTexture (gpuTex.get(), (uint32_t) outputTex->getWidth(), (uint32_t) outputTex->getHeight());

    if (outputView == nullptr)
        return false;

    impl->liveViews.push_back (outputView);

    std::vector<std::pair<uint32_t, rive::rcp<rive::ore::BindGroup>>> bindGroups;

    // Determine which groups have at least one binding.
    const auto& layouts = impl->layouts;

    for (uint32_t groupIdx = 0; groupIdx < layouts.size(); ++groupIdx)
    {
        auto* layout = layouts[groupIdx].get();
        if (layout == nullptr)
            continue;

        // UBO entries for this group.
        std::vector<rive::ore::BindGroupDesc::UBOEntry> uboEntries;

        for (const auto& ub : impl->uboBindings)
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
            impl->liveBuffers.push_back (std::move (buf));
        }

        // Texture entries for this group.
        std::vector<rive::ore::BindGroupDesc::TexEntry> texEntries;

        for (const auto& tb : impl->textureBindings)
        {
            if (tb.group != (int) groupIdx || tb.texture == nullptr)
                continue;

            rive::rcp<rive::ore::TextureView> view;
            if (auto rc = tb.texture->getInternalRenderCanvas())
                view = oreCtx->wrapCanvasTexture (rc.get());
            else if (auto gpuTex = tb.texture->getOrAdoptGpuTexture())
                view = oreCtx->wrapRiveTexture (gpuTex.get(), (uint32_t) tb.texture->getWidth(), (uint32_t) tb.texture->getHeight());

            if (view == nullptr)
                continue;

            rive::ore::BindGroupDesc::TexEntry entry;
            entry.slot = (uint32_t) tb.binding;
            entry.view = view.get();
            texEntries.push_back (entry);
            impl->liveViews.push_back (std::move (view));
        }

        // Sampler entries — auto-create one linear+clamp sampler for each
        // sampler binding declared in the layout.
        std::vector<rive::ore::BindGroupDesc::SampEntry> sampEntries;

        for (const auto& layoutEntry : layout->entries())
        {
            if (layoutEntry.kind != rive::ore::BindingKind::sampler
                && layoutEntry.kind != rive::ore::BindingKind::comparisonSampler)
                continue;

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
            impl->liveSamplers.push_back (std::move (samp));
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

    // Encode the render pass into the current frame (beginFrame/endFrame
    // are the caller's responsibility — see beginFrame() / endFrame()).
    rive::ore::RenderPassDesc rpDesc;
    rpDesc.colorCount = 1;
    rpDesc.colorAttachments[0].view = outputView.get();
    rpDesc.colorAttachments[0].loadOp = rive::ore::LoadOp::clear;
    rpDesc.colorAttachments[0].storeOp = rive::ore::StoreOp::store;
    rpDesc.colorAttachments[0].clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

    auto renderPass = oreCtx->beginRenderPass (rpDesc);
    renderPass->setPipeline (impl->pipeline.get());
    renderPass->setViewport (0.0f, 0.0f, (float) output.getWidth(), (float) output.getHeight());

    for (auto& [groupIdx, bg] : bindGroups)
        renderPass->setBindGroup (groupIdx, bg.get());

    renderPass->draw (3); // fullscreen triangle — no vertex buffer needed
    renderPass->finish();

    return true;
}

//==============================================================================

rive::ore::Context* GpuProgram::oreContext() const noexcept
{
    return impl != nullptr ? impl->oreCtx : nullptr;
}

rive::ore::Pipeline* GpuProgram::orePipeline() const noexcept
{
    return (impl != nullptr && impl->pipeline != nullptr) ? impl->pipeline.get() : nullptr;
}

//==============================================================================

GpuProgram::Ptr GpuProgram::compile (GraphicsContext& ctx,
                                     const GpuShaderSource& vs,
                                     const GpuShaderSource& fs,
                                     std::string* outError)
{
    auto setError = [&] (const char* msg)
    {
        if (outError)
            *outError = msg;
        return GpuProgram::Ptr {};
    };

    auto* oreCtx = ctx.gpuContext();
    if (oreCtx == nullptr)
        return setError ("GraphicsContext was not created with Options::enableOreContext = true");

    if (vs.code == nullptr || vs.codeSize == 0)
        return setError ("Vertex shader code is empty");

    if (vs.bindingMap == nullptr || vs.bindingMapSize == 0)
        return setError ("Vertex shader binding-map sidecar is required but not provided");

    if (fs.code == nullptr || fs.codeSize == 0)
        return setError ("Fragment shader code is empty");

    if (fs.bindingMap == nullptr || fs.bindingMapSize == 0)
        return setError ("Fragment shader binding-map sidecar is required but not provided");

    // Compile vertex shader module.
    rive::ore::ShaderModuleDesc vsd;
    vsd.code = vs.code;
    vsd.codeSize = vs.codeSize;
    vsd.language = (vs.language == GpuShaderLanguage::wgsl)
                     ? rive::ore::ShaderLanguage::wgsl
                     : rive::ore::ShaderLanguage::glsl;
    vsd.stage = rive::ore::ShaderStage::vertex;
    vsd.label = "GpuProgram VS";
    vsd.bindingMapBytes = vs.bindingMap;
    vsd.bindingMapSize = vs.bindingMapSize;

    auto vertModule = oreCtx->makeShaderModule (vsd);
    if (vertModule == nullptr)
    {
        if (outError)
            *outError = "Failed to compile vertex shader: " + oreCtx->lastError();
        return nullptr;
    }

    // Compile fragment shader module.
    rive::ore::ShaderModuleDesc fsd;
    fsd.code = fs.code;
    fsd.codeSize = fs.codeSize;
    fsd.language = (fs.language == GpuShaderLanguage::wgsl)
                     ? rive::ore::ShaderLanguage::wgsl
                     : rive::ore::ShaderLanguage::glsl;
    fsd.stage = rive::ore::ShaderStage::fragment;
    fsd.label = "GpuProgram FS";
    fsd.bindingMapBytes = fs.bindingMap;
    fsd.bindingMapSize = fs.bindingMapSize;

    auto fragModule = oreCtx->makeShaderModule (fsd);
    if (fragModule == nullptr)
    {
        if (outError)
            *outError = "Failed to compile fragment shader: " + oreCtx->lastError();
        return nullptr;
    }

    // Derive BindGroupLayouts by merging the VS and FS binding maps.
    // Each entry is identified by (group, binding); stage masks are OR'd.
    struct MergedEntry
    {
        uint32_t group;
        uint32_t binding;
        rive::ore::ResourceKind kind;
        uint32_t stageMask;
        uint16_t slotVS;
        uint16_t slotFS;
        rive::ore::TextureViewDim texViewDim;
        rive::ore::TextureSampleType texSampleType;
        bool texMultisampled;
    };

    std::vector<MergedEntry> merged;
    uint32_t maxGroup = 0;

    auto addEntry = [&] (const rive::ore::BindingMap::Entry& e)
    {
        maxGroup = std::max (maxGroup, (uint32_t) e.group);

        for (auto& m : merged)
        {
            if (m.group == e.group && m.binding == e.binding)
            {
                m.stageMask |= e.stageMask;
                if (e.backendSlot[0] != rive::ore::BindingMap::kAbsent)
                    m.slotVS = e.backendSlot[0];
                if (e.backendSlot[1] != rive::ore::BindingMap::kAbsent)
                    m.slotFS = e.backendSlot[1];
                return;
            }
        }

        MergedEntry me;
        me.group = e.group;
        me.binding = e.binding;
        me.kind = e.kind;
        me.stageMask = e.stageMask;
        me.slotVS = e.backendSlot[0];
        me.slotFS = e.backendSlot[1];
        me.texViewDim = e.textureViewDim;
        me.texSampleType = e.textureSampleType;
        me.texMultisampled = e.textureMultisampled;
        merged.push_back (me);
    };

    const auto& vsMap = vertModule->m_bindingMap;
    const auto& fsMap = fragModule->m_bindingMap;

    for (size_t i = 0; i < vsMap.size(); ++i)
        addEntry (vsMap.at (i));
    for (size_t i = 0; i < fsMap.size(); ++i)
        addEntry (fsMap.at (i));

    // Build one BindGroupLayout per @group used.
    const uint32_t numGroups = merged.empty() ? 0 : maxGroup + 1;
    std::vector<rive::rcp<rive::ore::BindGroupLayout>> layouts (numGroups);

    for (uint32_t g = 0; g < numGroups; ++g)
    {
        std::vector<rive::ore::BindGroupLayoutEntry> entries;

        for (const auto& me : merged)
        {
            if (me.group != g)
                continue;

            rive::ore::BindGroupLayoutEntry entry;
            entry.binding = me.binding;

            switch (me.kind)
            {
                case rive::ore::ResourceKind::UniformBuffer:
                    entry.kind = rive::ore::BindingKind::uniformBuffer;
                    break;
                case rive::ore::ResourceKind::StorageBufferRO:
                    entry.kind = rive::ore::BindingKind::storageBufferRO;
                    break;
                case rive::ore::ResourceKind::StorageBufferRW:
                    entry.kind = rive::ore::BindingKind::storageBufferRW;
                    break;
                case rive::ore::ResourceKind::SampledTexture:
                    entry.kind = rive::ore::BindingKind::sampledTexture;
                    break;
                case rive::ore::ResourceKind::StorageTexture:
                    entry.kind = rive::ore::BindingKind::storageTexture;
                    break;
                case rive::ore::ResourceKind::Sampler:
                    entry.kind = rive::ore::BindingKind::sampler;
                    break;
                case rive::ore::ResourceKind::ComparisonSampler:
                    entry.kind = rive::ore::BindingKind::comparisonSampler;
                    break;
            }

            entry.visibility.mask = 0;
            if (me.stageMask & rive::ore::BindingMap::kStageVertex)
                entry.visibility.mask |= rive::ore::StageVisibility::kVertex;
            if (me.stageMask & rive::ore::BindingMap::kStageFragment)
                entry.visibility.mask |= rive::ore::StageVisibility::kFragment;

            entry.nativeSlotVS = (me.slotVS != rive::ore::BindingMap::kAbsent) ? me.slotVS : rive::ore::BindGroupLayoutEntry::kNativeSlotAbsent;
            entry.nativeSlotFS = (me.slotFS != rive::ore::BindingMap::kAbsent) ? me.slotFS : rive::ore::BindGroupLayoutEntry::kNativeSlotAbsent;

            if (me.kind == rive::ore::ResourceKind::SampledTexture
                || me.kind == rive::ore::ResourceKind::StorageTexture)
            {
                switch (me.texViewDim)
                {
                    case rive::ore::TextureViewDim::D2:
                        entry.textureViewDim = rive::ore::TextureViewDimension::texture2D;
                        break;
                    case rive::ore::TextureViewDim::Cube:
                        entry.textureViewDim = rive::ore::TextureViewDimension::cube;
                        break;
                    case rive::ore::TextureViewDim::D3:
                        entry.textureViewDim = rive::ore::TextureViewDimension::texture3D;
                        break;
                    case rive::ore::TextureViewDim::D2Array:
                        entry.textureViewDim = rive::ore::TextureViewDimension::array2D;
                        break;
                    default:
                        entry.textureViewDim = rive::ore::TextureViewDimension::texture2D;
                        break;
                }

                using ST = rive::ore::BindGroupLayoutEntry::SampleType;
                switch (me.texSampleType)
                {
                    case rive::ore::TextureSampleType::Float:
                        entry.textureSampleType = ST::floatFilterable;
                        break;
                    case rive::ore::TextureSampleType::UnfilterableFloat:
                        entry.textureSampleType = ST::floatUnfilterable;
                        break;
                    case rive::ore::TextureSampleType::Depth:
                        entry.textureSampleType = ST::depth;
                        break;
                    case rive::ore::TextureSampleType::Sint:
                        entry.textureSampleType = ST::sint;
                        break;
                    case rive::ore::TextureSampleType::Uint:
                        entry.textureSampleType = ST::uint;
                        break;
                    default:
                        entry.textureSampleType = ST::floatFilterable;
                        break;
                }

                entry.textureMultisampled = me.texMultisampled;
            }

            entries.push_back (entry);
        }

        if (! entries.empty())
        {
            rive::ore::BindGroupLayoutDesc desc;
            desc.groupIndex = g;
            desc.entries = entries.data();
            desc.entryCount = (uint32_t) entries.size();
            desc.label = "GpuProgram BGL";
            layouts[g] = oreCtx->makeBindGroupLayout (desc);
        }
    }

    // Build the raw layout-pointer array required by PipelineDesc.
    std::vector<rive::ore::BindGroupLayout*> layoutPtrs (numGroups, nullptr);
    for (uint32_t g = 0; g < numGroups; ++g)
        layoutPtrs[g] = layouts[g].get();

    // Fullscreen triangle pipeline — no vertex buffers, rgba8unorm output.
    rive::ore::PipelineDesc pipeDesc;
    pipeDesc.vertexModule = vertModule.get();
    pipeDesc.vertexEntryPoint = (vs.entryPoint != nullptr) ? vs.entryPoint : "vs_main";
    pipeDesc.fragmentModule = fragModule.get();
    pipeDesc.fragmentEntryPoint = (fs.entryPoint != nullptr) ? fs.entryPoint : "fs_main";
    pipeDesc.topology = rive::ore::PrimitiveTopology::triangleList;
    pipeDesc.colorCount = 1;
    pipeDesc.colorTargets[0].format = rive::ore::TextureFormat::rgba8unorm;
    pipeDesc.colorTargets[0].blendEnabled = true;
    pipeDesc.colorTargets[0].blend.srcColor = rive::ore::BlendFactor::srcAlpha;
    pipeDesc.colorTargets[0].blend.dstColor = rive::ore::BlendFactor::oneMinusSrcAlpha;
    pipeDesc.colorTargets[0].blend.colorOp = rive::ore::BlendOp::add;
    pipeDesc.colorTargets[0].blend.srcAlpha = rive::ore::BlendFactor::one;
    pipeDesc.colorTargets[0].blend.dstAlpha = rive::ore::BlendFactor::oneMinusSrcAlpha;
    pipeDesc.colorTargets[0].blend.alphaOp = rive::ore::BlendOp::add;
    pipeDesc.bindGroupLayouts = layoutPtrs.empty() ? nullptr : layoutPtrs.data();
    pipeDesc.bindGroupLayoutCount = (uint32_t) layoutPtrs.size();
    pipeDesc.label = "GpuProgram Pipeline";

    std::string pipeError;
    auto pipeline = oreCtx->makePipeline (pipeDesc, &pipeError);
    if (pipeline == nullptr)
    {
        if (outError)
            *outError = "Failed to create pipeline: " + pipeError;
        return nullptr;
    }

    auto* prog = new GpuProgram();
    prog->impl = std::make_unique<Impl>();
    prog->impl->oreCtx = oreCtx;
    prog->impl->vertModule = std::move (vertModule);
    prog->impl->fragModule = std::move (fragModule);
    prog->impl->pipeline = std::move (pipeline);
    prog->impl->layouts = std::move (layouts);
    return prog;
}

} // namespace yup
