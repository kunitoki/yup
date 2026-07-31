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

namespace GpuPipelineHelpers
{

rive::ore::VertexFormat toOreVertexFormat (GpuVertexFormat f)
{
    switch (f)
    {
        case GpuVertexFormat::float1:
            return rive::ore::VertexFormat::float1;
        case GpuVertexFormat::float2:
            return rive::ore::VertexFormat::float2;
        case GpuVertexFormat::float3:
            return rive::ore::VertexFormat::float3;
        case GpuVertexFormat::float4:
            return rive::ore::VertexFormat::float4;
        case GpuVertexFormat::uint8x4:
            return rive::ore::VertexFormat::uint8x4;
        case GpuVertexFormat::snorm8x4:
            return rive::ore::VertexFormat::snorm8x4;
        case GpuVertexFormat::unorm8x4:
            return rive::ore::VertexFormat::unorm8x4;
        default:
            return rive::ore::VertexFormat::float4;
    }
}

rive::ore::VertexStepMode toOreStepMode (GpuVertexStepMode m)
{
    return m == GpuVertexStepMode::instance ? rive::ore::VertexStepMode::instance
                                            : rive::ore::VertexStepMode::vertex;
}

rive::ore::PrimitiveTopology toOreTopology (GpuPrimitiveTopology t)
{
    switch (t)
    {
        case GpuPrimitiveTopology::pointList:
            return rive::ore::PrimitiveTopology::pointList;
        case GpuPrimitiveTopology::lineList:
            return rive::ore::PrimitiveTopology::lineList;
        case GpuPrimitiveTopology::lineStrip:
            return rive::ore::PrimitiveTopology::lineStrip;
        case GpuPrimitiveTopology::triangleList:
            return rive::ore::PrimitiveTopology::triangleList;
        case GpuPrimitiveTopology::triangleStrip:
            return rive::ore::PrimitiveTopology::triangleStrip;
        default:
            return rive::ore::PrimitiveTopology::triangleList;
    }
}

rive::ore::IndexFormat toOreIndexFormat (GpuIndexFormat f)
{
    switch (f)
    {
        case GpuIndexFormat::uint16:
            return rive::ore::IndexFormat::uint16;
        case GpuIndexFormat::uint32:
            return rive::ore::IndexFormat::uint32;
        case GpuIndexFormat::none:
        default:
            return rive::ore::IndexFormat::none;
    }
}

rive::ore::CullMode toOreCullMode (GpuCullMode m)
{
    switch (m)
    {
        case GpuCullMode::front:
            return rive::ore::CullMode::front;
        case GpuCullMode::back:
            return rive::ore::CullMode::back;
        case GpuCullMode::none:
        default:
            return rive::ore::CullMode::none;
    }
}

rive::ore::FaceWinding toOreWinding (GpuFaceWinding w)
{
    return w == GpuFaceWinding::clockwise ? rive::ore::FaceWinding::clockwise
                                          : rive::ore::FaceWinding::counterClockwise;
}

rive::ore::CompareFunction toOreCompare (GpuCompareFunction c)
{
    switch (c)
    {
        case GpuCompareFunction::never:
            return rive::ore::CompareFunction::never;
        case GpuCompareFunction::less:
            return rive::ore::CompareFunction::less;
        case GpuCompareFunction::equal:
            return rive::ore::CompareFunction::equal;
        case GpuCompareFunction::lessEqual:
            return rive::ore::CompareFunction::lessEqual;
        case GpuCompareFunction::greater:
            return rive::ore::CompareFunction::greater;
        case GpuCompareFunction::notEqual:
            return rive::ore::CompareFunction::notEqual;
        case GpuCompareFunction::greaterEqual:
            return rive::ore::CompareFunction::greaterEqual;
        case GpuCompareFunction::always:
        default:
            return rive::ore::CompareFunction::always;
    }
}

rive::ore::StencilOp toOreStencilOp (GpuStencilOp o)
{
    switch (o)
    {
        case GpuStencilOp::keep:
            return rive::ore::StencilOp::keep;
        case GpuStencilOp::zero:
            return rive::ore::StencilOp::zero;
        case GpuStencilOp::replace:
            return rive::ore::StencilOp::replace;
        case GpuStencilOp::incrementClamp:
            return rive::ore::StencilOp::incrementClamp;
        case GpuStencilOp::decrementClamp:
            return rive::ore::StencilOp::decrementClamp;
        case GpuStencilOp::invert:
            return rive::ore::StencilOp::invert;
        case GpuStencilOp::incrementWrap:
            return rive::ore::StencilOp::incrementWrap;
        case GpuStencilOp::decrementWrap:
            return rive::ore::StencilOp::decrementWrap;
        default:
            return rive::ore::StencilOp::keep;
    }
}

rive::ore::BlendFactor toOreBlendFactor (GpuBlendFactor f)
{
    switch (f)
    {
        case GpuBlendFactor::zero:
            return rive::ore::BlendFactor::zero;
        case GpuBlendFactor::one:
            return rive::ore::BlendFactor::one;
        case GpuBlendFactor::srcColor:
            return rive::ore::BlendFactor::srcColor;
        case GpuBlendFactor::oneMinusSrcColor:
            return rive::ore::BlendFactor::oneMinusSrcColor;
        case GpuBlendFactor::srcAlpha:
            return rive::ore::BlendFactor::srcAlpha;
        case GpuBlendFactor::oneMinusSrcAlpha:
            return rive::ore::BlendFactor::oneMinusSrcAlpha;
        case GpuBlendFactor::dstColor:
            return rive::ore::BlendFactor::dstColor;
        case GpuBlendFactor::oneMinusDstColor:
            return rive::ore::BlendFactor::oneMinusDstColor;
        case GpuBlendFactor::dstAlpha:
            return rive::ore::BlendFactor::dstAlpha;
        case GpuBlendFactor::oneMinusDstAlpha:
            return rive::ore::BlendFactor::oneMinusDstAlpha;
        default:
            return rive::ore::BlendFactor::one;
    }
}

rive::ore::BlendOp toOreBlendOp (GpuBlendOp o)
{
    switch (o)
    {
        case GpuBlendOp::add:
            return rive::ore::BlendOp::add;
        case GpuBlendOp::subtract:
            return rive::ore::BlendOp::subtract;
        case GpuBlendOp::reverseSubtract:
            return rive::ore::BlendOp::reverseSubtract;
        case GpuBlendOp::min:
            return rive::ore::BlendOp::min;
        case GpuBlendOp::max:
            return rive::ore::BlendOp::max;
        default:
            return rive::ore::BlendOp::add;
    }
}

rive::ore::TextureFormat toOreTextureFormat (GpuTextureFormat f)
{
    switch (f)
    {
        case GpuTextureFormat::rgba8unorm:
            return rive::ore::TextureFormat::rgba8unorm;
        case GpuTextureFormat::bgra8unorm:
            return rive::ore::TextureFormat::bgra8unorm;
        case GpuTextureFormat::rgba16float:
            return rive::ore::TextureFormat::rgba16float;
        case GpuTextureFormat::depth24plusStencil8:
            return rive::ore::TextureFormat::depth24plusStencil8;
        case GpuTextureFormat::depth32float:
            return rive::ore::TextureFormat::depth32float;
        default:
            return rive::ore::TextureFormat::rgba8unorm;
    }
}

} // namespace GpuPipelineHelpers

//==============================================================================

struct GpuPipeline::Impl
{
    /** A sampler auto-created for one sampler binding declared by the layouts. */
    struct SamplerBinding
    {
        uint32_t binding;
        rive::rcp<rive::ore::Sampler> sampler;
    };

    rive::ore::Context* oreCtx = nullptr;
    rive::rcp<rive::ore::ShaderModule> vertModule;
    rive::rcp<rive::ore::ShaderModule> fragModule;
    rive::rcp<rive::ore::Pipeline> pipeline;
    std::vector<rive::rcp<rive::ore::BindGroupLayout>> layouts; // indexed by group; may contain null entries

    // One sampler per sampler binding the layouts declare, indexed by group.
    // GpuRenderPass fills every declared sampler slot with a linear/clamp-to-edge
    // sampler; that descriptor never varies, so the samplers are created here once
    // rather than per draw. The pipeline outlives the passes that reference them.
    std::vector<std::vector<SamplerBinding>> samplersPerGroup;

    // Vertex-layout storage backing PipelineDesc's raw pointers. The ore
    // Pipeline copies PipelineDesc by value but keeps the vertexBuffers /
    // attributes pointers, reading them at draw time - so this storage must
    // outlive the pipeline.
    std::vector<std::vector<rive::ore::VertexAttribute>> vertexAttrStorage;
    std::vector<rive::ore::VertexBufferLayout> vertexLayoutStorage;
};

//==============================================================================

GpuPipeline::~GpuPipeline() = default;

GpuPipeline::Impl* GpuPipeline::getImpl() noexcept
{
    return impl.getPayload<Impl>();
}

const GpuPipeline::Impl* GpuPipeline::getImpl() const noexcept
{
    return impl.getPayload<Impl>();
}

//==============================================================================

ResultValue<GpuPipeline::Ptr> GpuPipeline::compile (GpuDevice::Ptr ctx,
                                                    const GpuShaderSource& vs,
                                                    const GpuShaderSource& fs,
                                                    const GpuPipelineOptions& pipelineOptions)
{
    using namespace GpuPipelineHelpers;

    auto oreCtx = ctx->getGpuContext();
    if (oreCtx == nullptr)
        return makeResultValueFail ("GpuDevice was not created with Options::enableOreContext = true");

    if (vs.code == nullptr || vs.codeSize == 0)
        return makeResultValueFail ("Vertex shader code is empty");

    if (vs.bindingMap == nullptr || vs.bindingMapSize == 0)
        return makeResultValueFail ("Vertex shader binding-map sidecar is required but not provided");

    if (fs.code == nullptr || fs.codeSize == 0)
        return makeResultValueFail ("Fragment shader code is empty");

    if (fs.bindingMap == nullptr || fs.bindingMapSize == 0)
        return makeResultValueFail ("Fragment shader binding-map sidecar is required but not provided");

    // Populates an ore ShaderModuleDesc from a GpuShaderSource. The D3D11/D3D12
    // backends compile HLSL from source at first use (AMD drivers crash on
    // cross-process DXBC), so HLSL sources must be routed through the dedicated
    // hlslSource fields rather than the generic code pointer.
    auto fillModuleDesc = [] (rive::ore::ShaderModuleDesc& desc,
                              const GpuShaderSource& src,
                              rive::ore::ShaderStage stage,
                              const char* label)
    {
        desc.language = rive::ore::ShaderLanguage::glsl;
        desc.code = src.code;
        desc.codeSize = src.codeSize;
        desc.stage = stage;
        desc.label = label;
        desc.bindingMapBytes = src.bindingMap;
        desc.bindingMapSize = src.bindingMapSize;
        desc.glFixupBytes = src.glFixup;
        desc.glFixupSize = src.glFixupSize;

        switch (src.language)
        {
            case GpuShaderLanguage::wgsl:
                desc.language = rive::ore::ShaderLanguage::wgsl;
                break;

            case GpuShaderLanguage::hlsl:
                desc.hlslSource = static_cast<const char*> (src.code);
                desc.hlslSourceSize = src.codeSize;
                desc.hlslEntryPoint = src.entryPoint;
                break;

            default:
                break;
        }
    };

    // Compile vertex shader module.
    rive::ore::ShaderModuleDesc vsd;
    fillModuleDesc (vsd, vs, rive::ore::ShaderStage::vertex, "GpuPipeline VS");

    auto vertModule = oreCtx->makeShaderModule (vsd);
    if (vertModule == nullptr)
        return makeResultValueFail ("Failed to compile vertex shader: " + oreCtx->lastError());

    // Compile fragment shader module.
    rive::ore::ShaderModuleDesc fsd;
    fillModuleDesc (fsd, fs, rive::ore::ShaderStage::fragment, "GpuPipeline FS");

    auto fragModule = oreCtx->makeShaderModule (fsd);
    if (fragModule == nullptr)
        return makeResultValueFail ("Failed to compile fragment shader: " + oreCtx->lastError());

    // Derive BindGroupLayouts by merging the VS and FS binding maps.
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
        maxGroup = jmax (maxGroup, (uint32_t) e.group);

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
    for (size_t i = 0; i < vsMap.size(); ++i)
        addEntry (vsMap.at (i));

    const auto& fsMap = fragModule->m_bindingMap;
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
            desc.label = "GpuPipeline BGL";
            layouts[g] = oreCtx->makeBindGroupLayout (desc);
        }
    }

    // Build the raw layout-pointer array required by PipelineDesc.
    std::vector<rive::ore::BindGroupLayout*> layoutPtrs (numGroups, nullptr);
    for (uint32_t g = 0; g < numGroups; ++g)
        layoutPtrs[g] = layouts[g].get();

    // Create the pipeline up-front so the vertex-layout storage that backs
    // PipelineDesc's raw pointers lives inside the object that owns the
    // pipeline. The ore Pipeline copies PipelineDesc by value but keeps the
    // vertexBuffers / attributes pointers, reading them at draw time.
    auto pipe = GpuPipeline::Ptr { new GpuPipeline() };
    pipe->impl = TypeErasedObject (GpuPipeline::Impl {});

    auto* implRef = pipe->getImpl();
    implRef->vertexAttrStorage.resize (pipelineOptions.vertexBufferCount);
    implRef->vertexLayoutStorage.resize (pipelineOptions.vertexBufferCount);

    for (uint32_t i = 0; i < pipelineOptions.vertexBufferCount; ++i)
    {
        const auto& src = pipelineOptions.vertexBuffers[i];
        auto& attrs = implRef->vertexAttrStorage[i];
        attrs.resize (src.attributeCount);

        for (uint32_t a = 0; a < src.attributeCount; ++a)
        {
            attrs[a].format = toOreVertexFormat (src.attributes[a].format);
            attrs[a].offset = src.attributes[a].offset;
            attrs[a].shaderSlot = src.attributes[a].shaderLocation;
        }

        implRef->vertexLayoutStorage[i].stride = src.stride;
        implRef->vertexLayoutStorage[i].stepMode = toOreStepMode (src.stepMode);
        implRef->vertexLayoutStorage[i].attributes = attrs.empty() ? nullptr : attrs.data();
        implRef->vertexLayoutStorage[i].attributeCount = src.attributeCount;
    }

    rive::ore::PipelineDesc pipeDesc;
    pipeDesc.vertexModule = vertModule.get();
    pipeDesc.vertexEntryPoint = (vs.entryPoint != nullptr) ? vs.entryPoint : "vs_main";
    pipeDesc.fragmentModule = fragModule.get();
    pipeDesc.fragmentEntryPoint = (fs.entryPoint != nullptr) ? fs.entryPoint : "fs_main";
    pipeDesc.vertexBuffers = implRef->vertexLayoutStorage.empty() ? nullptr : implRef->vertexLayoutStorage.data();
    pipeDesc.vertexBufferCount = (uint32_t) implRef->vertexLayoutStorage.size();
    pipeDesc.topology = toOreTopology (pipelineOptions.topology);
    pipeDesc.indexFormat = toOreIndexFormat (pipelineOptions.indexFormat);
    pipeDesc.cullMode = toOreCullMode (pipelineOptions.cullMode);
    pipeDesc.winding = toOreWinding (pipelineOptions.winding);

    // Color targets. Default to a single alpha-blended rgba8unorm target when
    // none are specified, matching the classic fullscreen post-process pipeline.
    if (pipelineOptions.colorTargetCount == 0)
    {
        pipeDesc.colorCount = 1;
        pipeDesc.colorTargets[0].format = rive::ore::TextureFormat::rgba8unorm;
        pipeDesc.colorTargets[0].blendEnabled = true;
        pipeDesc.colorTargets[0].blend.srcColor = rive::ore::BlendFactor::srcAlpha;
        pipeDesc.colorTargets[0].blend.dstColor = rive::ore::BlendFactor::oneMinusSrcAlpha;
        pipeDesc.colorTargets[0].blend.colorOp = rive::ore::BlendOp::add;
        pipeDesc.colorTargets[0].blend.srcAlpha = rive::ore::BlendFactor::one;
        pipeDesc.colorTargets[0].blend.dstAlpha = rive::ore::BlendFactor::oneMinusSrcAlpha;
        pipeDesc.colorTargets[0].blend.alphaOp = rive::ore::BlendOp::add;
    }
    else
    {
        const uint32_t count = jmin<uint32_t> (pipelineOptions.colorTargetCount, 4);
        pipeDesc.colorCount = count;

        for (uint32_t i = 0; i < count; ++i)
        {
            const auto& src = pipelineOptions.colorTargets[i];
            pipeDesc.colorTargets[i].format = toOreTextureFormat (src.format);
            pipeDesc.colorTargets[i].blendEnabled = src.blendEnabled;
            pipeDesc.colorTargets[i].blend.srcColor = toOreBlendFactor (src.blend.srcColor);
            pipeDesc.colorTargets[i].blend.dstColor = toOreBlendFactor (src.blend.dstColor);
            pipeDesc.colorTargets[i].blend.colorOp = toOreBlendOp (src.blend.colorOp);
            pipeDesc.colorTargets[i].blend.srcAlpha = toOreBlendFactor (src.blend.srcAlpha);
            pipeDesc.colorTargets[i].blend.dstAlpha = toOreBlendFactor (src.blend.dstAlpha);
            pipeDesc.colorTargets[i].blend.alphaOp = toOreBlendOp (src.blend.alphaOp);
        }
    }

    // Depth/stencil. rgba8unorm is the ore sentinel for "no depth/stencil".
    if (pipelineOptions.depthStencil.enabled)
    {
        pipeDesc.depthStencil.format = toOreTextureFormat (pipelineOptions.depthStencil.format);
        pipeDesc.depthStencil.depthCompare = toOreCompare (pipelineOptions.depthStencil.depthCompare);
        pipeDesc.depthStencil.depthWriteEnabled = pipelineOptions.depthStencil.depthWriteEnabled;

        auto fillStencilFace = [] (rive::ore::StencilFaceState& dst, const GpuStencilFaceState& src)
        {
            dst.compare = toOreCompare (src.compare);
            dst.failOp = toOreStencilOp (src.failOp);
            dst.depthFailOp = toOreStencilOp (src.depthFailOp);
            dst.passOp = toOreStencilOp (src.passOp);
        };

        fillStencilFace (pipeDesc.stencilFront, pipelineOptions.stencilFront);
        fillStencilFace (pipeDesc.stencilBack, pipelineOptions.stencilBack);
        pipeDesc.stencilReadMask = pipelineOptions.stencilReadMask;
        pipeDesc.stencilWriteMask = pipelineOptions.stencilWriteMask;
    }

    pipeDesc.sampleCount = pipelineOptions.sampleCount;
    pipeDesc.bindGroupLayouts = layoutPtrs.empty() ? nullptr : layoutPtrs.data();
    pipeDesc.bindGroupLayoutCount = (uint32_t) layoutPtrs.size();
    pipeDesc.label = "GpuPipeline Pipeline";

    std::string pipeError;
    auto pipeline = oreCtx->makePipeline (pipeDesc, &pipeError);
    if (pipeline == nullptr)
        return makeResultValueFail ("Failed to create pipeline: " + pipeError);

    implRef->oreCtx = oreCtx;
    implRef->vertModule = std::move (vertModule);
    implRef->fragModule = std::move (fragModule);
    implRef->pipeline = std::move (pipeline);
    implRef->layouts = std::move (layouts);

    // Create the auto-samplers up front. Their descriptor is fixed, so one per
    // declared binding serves every draw encoded with this pipeline.
    implRef->samplersPerGroup.resize (implRef->layouts.size());

    for (size_t g = 0; g < implRef->layouts.size(); ++g)
    {
        auto* layout = implRef->layouts[g].get();
        if (layout == nullptr)
            continue;

        for (const auto& entry : layout->entries())
        {
            if (entry.kind != rive::ore::BindingKind::sampler
                && entry.kind != rive::ore::BindingKind::comparisonSampler)
            {
                continue;
            }

            rive::ore::SamplerDesc sd;
            sd.minFilter = rive::ore::Filter::linear;
            sd.magFilter = rive::ore::Filter::linear;
            sd.wrapU = rive::ore::WrapMode::clampToEdge;
            sd.wrapV = rive::ore::WrapMode::clampToEdge;

            if (auto sampler = oreCtx->makeSampler (sd))
                implRef->samplersPerGroup[g].push_back ({ entry.binding, std::move (sampler) });
        }
    }

    return makeResultValueOk (pipe);
}

//==============================================================================

namespace
{

ShaderLanguage shaderLanguageForApi (GpuPlatform api)
{
    switch (api)
    {
        case GpuPlatform::Metal:
            return ShaderLanguage::msl;
        case GpuPlatform::Direct3D:
            return ShaderLanguage::hlsl;
        case GpuPlatform::OpenGLES:
            return ShaderLanguage::essl;
        case GpuPlatform::WebGPU:
            return ShaderLanguage::wgsl;
        default:
            return ShaderLanguage::glsl;
    }
}

GpuShaderLanguage gpuShaderLanguageForApi (GpuPlatform api)
{
    switch (api)
    {
        case GpuPlatform::Metal:
            return GpuShaderLanguage::msl;
        case GpuPlatform::Direct3D:
            return GpuShaderLanguage::hlsl;
        case GpuPlatform::WebGPU:
            return GpuShaderLanguage::wgsl;
        default:
            return GpuShaderLanguage::glsl;
    }
}

} // namespace

ResultValue<GpuPipeline::Ptr> GpuPipeline::compileFromBundle (GpuDevice::Ptr ctx,
                                                              const ShaderBundle& bundle,
                                                              const GpuPipelineOptions& pipelineOptions)
{
    const auto api = ctx->getPlatform();
    const auto targetLang = shaderLanguageForApi (api);
    const auto gpuLang = gpuShaderLanguageForApi (api);

    const ShaderInfo* vsInfo = bundle.findShader (ShaderStage::vertex, targetLang);
    if (vsInfo == nullptr && targetLang == ShaderLanguage::essl)
        vsInfo = bundle.findShader (ShaderStage::vertex, ShaderLanguage::glsl);

    if (vsInfo == nullptr)
        return makeResultValueFail ("Shader bundle has no vertex variant for the current graphics API");

    const ShaderInfo* fsInfo = bundle.findShader (ShaderStage::fragment, targetLang);
    if (fsInfo == nullptr && targetLang == ShaderLanguage::essl)
        fsInfo = bundle.findShader (ShaderStage::fragment, ShaderLanguage::glsl);

    if (fsInfo == nullptr)
        return makeResultValueFail ("Shader bundle has no fragment variant for the current graphics API");

    auto vsMap = makeShaderBindingMapBlob (vsInfo->reflection, ShaderStage::vertex);
    auto fsMap = makeShaderBindingMapBlob (fsInfo->reflection, ShaderStage::fragment);

    auto vsSource = vsInfo->source.toRawUTF8();
    auto fsSource = fsInfo->source.toRawUTF8();

    // GL / GLES bind UBO blocks and sampler units by name after linking, so
    // build the name→slot fixup table for the GLSL/ESSL targets. Empty (and
    // ignored) for every other backend.
    const bool isGLTarget = (gpuLang == GpuShaderLanguage::glsl);
    std::vector<uint8_t> vsFixup, fsFixup;
    if (isGLTarget)
    {
        vsFixup = makeGLFixupBlob (vsInfo->reflection);
        fsFixup = makeGLFixupBlob (fsInfo->reflection);
    }

    // SPIRV-Cross renames the GLSL "main" entry point to "main0" in MSL.
    auto resolveEntry = [gpuLang] (const ShaderInfo& info) -> String
    {
        if (gpuLang == GpuShaderLanguage::msl && info.entryPoint == "main")
            return "main0";
        return info.entryPoint;
    };

    const auto vsEntryStr = resolveEntry (*vsInfo);
    const auto fsEntryStr = resolveEntry (*fsInfo);
    auto vsEntry = vsEntryStr.toRawUTF8();
    auto fsEntry = fsEntryStr.toRawUTF8();

    GpuShaderSource vs;
    vs.language = gpuLang;
    vs.code = vsSource;
    vs.codeSize = (uint32_t) strlen (vsSource);
    vs.bindingMap = vsMap.data();
    vs.bindingMapSize = (uint32_t) vsMap.size();
    vs.glFixup = vsFixup.empty() ? nullptr : vsFixup.data();
    vs.glFixupSize = (uint32_t) vsFixup.size();
    vs.entryPoint = vsEntry;

    GpuShaderSource fs;
    fs.language = gpuLang;
    fs.code = fsSource;
    fs.codeSize = (uint32_t) strlen (fsSource);
    fs.bindingMap = fsMap.data();
    fs.bindingMapSize = (uint32_t) fsMap.size();
    fs.glFixup = fsFixup.empty() ? nullptr : fsFixup.data();
    fs.glFixupSize = (uint32_t) fsFixup.size();
    fs.entryPoint = fsEntry;

    return compile (ctx, vs, fs, pipelineOptions);
}

#if YUP_ENABLE_SHADER_TRANSPILER

ResultValue<GpuPipeline::Ptr> GpuPipeline::compileFromGlsl (GpuDevice::Ptr ctx,
                                                            const String& vertexGlsl,
                                                            const String& fragmentGlsl,
                                                            const GpuPipelineOptions& pipelineOptions)
{
    const auto targetLang = shaderLanguageForApi (ctx->getPlatform());

    ShaderBundleCompiler compiler;

    auto makeEntry = [&] (ShaderStage stage)
    {
        ShaderBundleEntry entry;
        entry.stage = stage;
        entry.targetLanguages = { targetLang };
        entry.options.spirvOptimization = SpvOptimizationMode::none;
        return entry;
    };

    auto vsBundle = [&]
    {
        ShaderBundleCompileRequest request;
        request.source = vertexGlsl;
        request.sourceLanguage = ShaderLanguage::glsl;
        request.entries.push_back (makeEntry (ShaderStage::vertex));
        return compiler.compile (request);
    }();

    if (vsBundle.failed())
        return makeResultValueFail ("Vertex shader compile failed: " + vsBundle.getErrorMessage());

    auto fsBundle = [&]
    {
        ShaderBundleCompileRequest request;
        request.source = fragmentGlsl;
        request.sourceLanguage = ShaderLanguage::glsl;
        request.entries.push_back (makeEntry (ShaderStage::fragment));
        return compiler.compile (request);
    }();

    if (fsBundle.failed())
        return makeResultValueFail ("Fragment shader compile failed: " + fsBundle.getErrorMessage());

    // Merge both stages into a single bundle for compileFromBundle().
    ShaderBundle bundle;
    for (const auto& info : vsBundle.getReference().getShaders())
        bundle.addShader (info);
    for (const auto& info : fsBundle.getReference().getShaders())
        bundle.addShader (info);

    return compileFromBundle (ctx, bundle, pipelineOptions);
}

#endif

} // namespace yup
