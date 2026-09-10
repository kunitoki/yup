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
        case GpuVertexFormat::sint8x4:
            return rive::ore::VertexFormat::sint8x4;
        case GpuVertexFormat::snorm8x4:
            return rive::ore::VertexFormat::snorm8x4;
        case GpuVertexFormat::unorm8x4:
            return rive::ore::VertexFormat::unorm8x4;
        case GpuVertexFormat::uint16x2:
            return rive::ore::VertexFormat::uint16x2;
        case GpuVertexFormat::sint16x2:
            return rive::ore::VertexFormat::sint16x2;
        case GpuVertexFormat::unorm16x2:
            return rive::ore::VertexFormat::unorm16x2;
        case GpuVertexFormat::snorm16x2:
            return rive::ore::VertexFormat::snorm16x2;
        case GpuVertexFormat::uint16x4:
            return rive::ore::VertexFormat::uint16x4;
        case GpuVertexFormat::sint16x4:
            return rive::ore::VertexFormat::sint16x4;
        case GpuVertexFormat::float16x2:
            return rive::ore::VertexFormat::float16x2;
        case GpuVertexFormat::float16x4:
            return rive::ore::VertexFormat::float16x4;
        case GpuVertexFormat::uint32:
            return rive::ore::VertexFormat::uint32;
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
        case GpuBlendFactor::srcAlphaSaturated:
            return rive::ore::BlendFactor::srcAlphaSaturated;
        case GpuBlendFactor::blendColor:
            return rive::ore::BlendFactor::blendColor;
        case GpuBlendFactor::oneMinusBlendColor:
            return rive::ore::BlendFactor::oneMinusBlendColor;
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
        case GpuTextureFormat::r8unorm:
            return rive::ore::TextureFormat::r8unorm;
        case GpuTextureFormat::rg8unorm:
            return rive::ore::TextureFormat::rg8unorm;
        case GpuTextureFormat::rgba8unorm:
            return rive::ore::TextureFormat::rgba8unorm;
        case GpuTextureFormat::rgba8snorm:
            return rive::ore::TextureFormat::rgba8snorm;
        case GpuTextureFormat::bgra8unorm:
            return rive::ore::TextureFormat::bgra8unorm;
        case GpuTextureFormat::rgba16float:
            return rive::ore::TextureFormat::rgba16float;
        case GpuTextureFormat::rg16float:
            return rive::ore::TextureFormat::rg16float;
        case GpuTextureFormat::r16float:
            return rive::ore::TextureFormat::r16float;
        case GpuTextureFormat::rgba32float:
            return rive::ore::TextureFormat::rgba32float;
        case GpuTextureFormat::rg32float:
            return rive::ore::TextureFormat::rg32float;
        case GpuTextureFormat::r32float:
            return rive::ore::TextureFormat::r32float;
        case GpuTextureFormat::rgb10a2unorm:
            return rive::ore::TextureFormat::rgb10a2unorm;
        case GpuTextureFormat::r11g11b10float:
            return rive::ore::TextureFormat::r11g11b10float;
        case GpuTextureFormat::depth16unorm:
            return rive::ore::TextureFormat::depth16unorm;
        case GpuTextureFormat::depth24plusStencil8:
            return rive::ore::TextureFormat::depth24plusStencil8;
        case GpuTextureFormat::depth32float:
            return rive::ore::TextureFormat::depth32float;
        case GpuTextureFormat::depth32floatStencil8:
            return rive::ore::TextureFormat::depth32floatStencil8;
        case GpuTextureFormat::bc1unorm:
            return rive::ore::TextureFormat::bc1unorm;
        case GpuTextureFormat::bc3unorm:
            return rive::ore::TextureFormat::bc3unorm;
        case GpuTextureFormat::bc7unorm:
            return rive::ore::TextureFormat::bc7unorm;
        case GpuTextureFormat::etc2rgb8:
            return rive::ore::TextureFormat::etc2rgb8;
        case GpuTextureFormat::etc2rgba8:
            return rive::ore::TextureFormat::etc2rgba8;
        case GpuTextureFormat::astc4x4:
            return rive::ore::TextureFormat::astc4x4;
        case GpuTextureFormat::astc6x6:
            return rive::ore::TextureFormat::astc6x6;
        case GpuTextureFormat::astc8x8:
            return rive::ore::TextureFormat::astc8x8;
        default:
            return rive::ore::TextureFormat::rgba8unorm;
    }
}

rive::ore::TextureType toOreTextureType (GpuTextureType t)
{
    switch (t)
    {
        case GpuTextureType::cube:
            return rive::ore::TextureType::cube;
        case GpuTextureType::texture3D:
            return rive::ore::TextureType::texture3D;
        case GpuTextureType::array2D:
            return rive::ore::TextureType::array2D;
        case GpuTextureType::texture2D:
        default:
            return rive::ore::TextureType::texture2D;
    }
}

rive::ore::TextureViewDimension toOreViewDimension (GpuTextureViewDimension d)
{
    switch (d)
    {
        case GpuTextureViewDimension::cube:
            return rive::ore::TextureViewDimension::cube;
        case GpuTextureViewDimension::texture3D:
            return rive::ore::TextureViewDimension::texture3D;
        case GpuTextureViewDimension::array2D:
            return rive::ore::TextureViewDimension::array2D;
        case GpuTextureViewDimension::cubeArray:
            return rive::ore::TextureViewDimension::cubeArray;
        case GpuTextureViewDimension::texture2D:
        default:
            return rive::ore::TextureViewDimension::texture2D;
    }
}

/** The view dimension that covers a whole texture of the given storage shape. */
GpuTextureViewDimension defaultViewDimension (GpuTextureType t)
{
    switch (t)
    {
        case GpuTextureType::cube:
            return GpuTextureViewDimension::cube;
        case GpuTextureType::texture3D:
            return GpuTextureViewDimension::texture3D;
        case GpuTextureType::array2D:
            return GpuTextureViewDimension::array2D;
        case GpuTextureType::texture2D:
        default:
            return GpuTextureViewDimension::texture2D;
    }
}

rive::ore::TextureAspect toOreTextureAspect (GpuTextureAspect a)
{
    switch (a)
    {
        case GpuTextureAspect::depthOnly:
            return rive::ore::TextureAspect::depthOnly;
        case GpuTextureAspect::stencilOnly:
            return rive::ore::TextureAspect::stencilOnly;
        case GpuTextureAspect::all:
        default:
            return rive::ore::TextureAspect::all;
    }
}

rive::ore::LoadOp toOreLoadOp (GpuLoadOp op)
{
    switch (op)
    {
        case GpuLoadOp::load:
            return rive::ore::LoadOp::load;
        case GpuLoadOp::dontCare:
            return rive::ore::LoadOp::dontCare;
        case GpuLoadOp::clear:
        default:
            return rive::ore::LoadOp::clear;
    }
}

rive::ore::StoreOp toOreStoreOp (GpuStoreOp op)
{
    return op == GpuStoreOp::discard ? rive::ore::StoreOp::discard : rive::ore::StoreOp::store;
}

rive::ore::Filter toOreFilter (GpuFilter f)
{
    return f == GpuFilter::linear ? rive::ore::Filter::linear : rive::ore::Filter::nearest;
}

rive::ore::WrapMode toOreWrapMode (GpuWrapMode w)
{
    switch (w)
    {
        case GpuWrapMode::repeat:
            return rive::ore::WrapMode::repeat;
        case GpuWrapMode::mirrorRepeat:
            return rive::ore::WrapMode::mirrorRepeat;
        case GpuWrapMode::clampToEdge:
        default:
            return rive::ore::WrapMode::clampToEdge;
    }
}

rive::ore::ColorWriteMask toOreColorWriteMask (GpuColorWriteMask m)
{
    return static_cast<rive::ore::ColorWriteMask> (static_cast<uint8_t> (m));
}

rive::ore::SamplerDesc toOreSamplerDesc (const GpuSamplerDesc& src)
{
    rive::ore::SamplerDesc sd;
    sd.minFilter = toOreFilter (src.minFilter);
    sd.magFilter = toOreFilter (src.magFilter);
    sd.mipmapFilter = toOreFilter (src.mipmapFilter);
    sd.wrapU = toOreWrapMode (src.wrapU);
    sd.wrapV = toOreWrapMode (src.wrapV);
    sd.wrapW = toOreWrapMode (src.wrapW);
    sd.compare = src.compare.has_value() ? toOreCompare (*src.compare) : rive::ore::CompareFunction::none;
    sd.minLod = src.minLod;
    sd.maxLod = src.maxLod;
    sd.maxAnisotropy = src.maxAnisotropy;
    sd.label = src.label.isNotEmpty() ? src.label.toRawUTF8() : nullptr;
    return sd;
}

} // namespace GpuPipelineHelpers

//==============================================================================

struct GpuPipeline::Impl
{
    struct SamplerBinding
    {
        uint32_t binding;
        rive::rcp<rive::ore::Sampler> sampler;
    };

    rive::ore::Context* oreCtx = nullptr;
    rive::rcp<rive::ore::ShaderModule> vertModule;
    rive::rcp<rive::ore::ShaderModule> fragModule;
    rive::rcp<rive::ore::Pipeline> pipeline;
    std::vector<rive::rcp<rive::ore::BindGroupLayout>> layouts;
    std::vector<std::vector<SamplerBinding>> samplersPerGroup;
    std::vector<std::vector<rive::ore::VertexAttribute>> vertexAttrStorage;
    std::vector<rive::ore::VertexBufferLayout> vertexLayoutStorage;

    // ore::Pipeline keeps a shallow copy of the PipelineDesc and dereferences its
    // entry-point names long after compile() returns, so the pipeline has to own them.
    std::string vertexEntryPointStorage;
    std::string fragmentEntryPointStorage;
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

    if (vs.code.empty())
        return makeResultValueFail ("Vertex shader code is empty");

    if (vs.bindingMap.empty())
        return makeResultValueFail ("Vertex shader binding-map sidecar is required but not provided");

    if (fs.code.empty())
        return makeResultValueFail ("Fragment shader code is empty");

    if (fs.bindingMap.empty())
        return makeResultValueFail ("Fragment shader binding-map sidecar is required but not provided");

    auto fillModuleDesc = [] (rive::ore::ShaderModuleDesc& desc,
                              const GpuShaderSource& src,
                              rive::ore::ShaderStage stage,
                              const char* label)
    {
        desc.language = rive::ore::ShaderLanguage::glsl;
        desc.code = src.code.data();
        desc.codeSize = (uint32_t) src.code.size();
        desc.stage = stage;
        desc.label = label;
        desc.bindingMapBytes = src.bindingMap.data();
        desc.bindingMapSize = (uint32_t) src.bindingMap.size();
        desc.glFixupBytes = src.glFixup.empty() ? nullptr : src.glFixup.data();
        desc.glFixupSize = (uint32_t) src.glFixup.size();

        switch (src.language)
        {
            case GpuShaderLanguage::wgsl:
                desc.language = rive::ore::ShaderLanguage::wgsl;
                break;

            case GpuShaderLanguage::hlsl:
                desc.hlslSource = reinterpret_cast<const char*> (src.code.data());
                desc.hlslSourceSize = (uint32_t) src.code.size();
                desc.hlslEntryPoint = src.entryPoint.isNotEmpty() ? src.entryPoint.toRawUTF8() : nullptr;
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

    // Create the pipeline up-front.
    auto pipe = GpuPipeline::Ptr { new GpuPipeline() };
    pipe->impl = TypeErasedObject (GpuPipeline::Impl {});

    auto* implRef = pipe->getImpl();

    const auto numVertexBuffers = pipelineOptions.vertexBuffers.size();
    implRef->vertexAttrStorage.resize (numVertexBuffers);
    implRef->vertexLayoutStorage.resize (numVertexBuffers);

    for (size_t i = 0; i < numVertexBuffers; ++i)
    {
        const auto& src = pipelineOptions.vertexBuffers[i];
        auto& attrs = implRef->vertexAttrStorage[i];
        attrs.resize (src.attributes.size());

        for (size_t a = 0; a < src.attributes.size(); ++a)
        {
            attrs[a].format = toOreVertexFormat (src.attributes[a].format);
            attrs[a].offset = src.attributes[a].offset;
            attrs[a].shaderSlot = src.attributes[a].shaderLocation;
        }

        implRef->vertexLayoutStorage[i].stride = src.stride;
        implRef->vertexLayoutStorage[i].stepMode = toOreStepMode (src.stepMode);
        implRef->vertexLayoutStorage[i].attributes = attrs.empty() ? nullptr : attrs.data();
        implRef->vertexLayoutStorage[i].attributeCount = (uint32_t) attrs.size();
    }

    implRef->vertexEntryPointStorage = vs.entryPoint.isNotEmpty() ? vs.entryPoint.toStdString() : "vs_main";
    implRef->fragmentEntryPointStorage = fs.entryPoint.isNotEmpty() ? fs.entryPoint.toStdString() : "fs_main";

    rive::ore::PipelineDesc pipeDesc;
    pipeDesc.vertexModule = vertModule.get();
    pipeDesc.vertexEntryPoint = implRef->vertexEntryPointStorage.c_str();
    pipeDesc.fragmentModule = fragModule.get();
    pipeDesc.fragmentEntryPoint = implRef->fragmentEntryPointStorage.c_str();
    pipeDesc.vertexBuffers = implRef->vertexLayoutStorage.empty() ? nullptr : implRef->vertexLayoutStorage.data();
    pipeDesc.vertexBufferCount = (uint32_t) implRef->vertexLayoutStorage.size();
    pipeDesc.topology = toOreTopology (pipelineOptions.topology);
    pipeDesc.indexFormat = toOreIndexFormat (pipelineOptions.indexFormat);
    pipeDesc.cullMode = toOreCullMode (pipelineOptions.cullMode);
    pipeDesc.winding = toOreWinding (pipelineOptions.winding);

    // Color targets.
    jassert (pipelineOptions.colorTargets.size() <= 4);

    if (pipelineOptions.colorTargets.empty())
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
        const auto count = jmin<size_t> (pipelineOptions.colorTargets.size(), 4);
        pipeDesc.colorCount = (uint32_t) count;

        for (size_t i = 0; i < count; ++i)
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
            pipeDesc.colorTargets[i].writeMask = toOreColorWriteMask (src.writeMask);
        }
    }

    // Depth/stencil.
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
            sd.wrapW = rive::ore::WrapMode::clampToEdge;

            if (entry.kind == rive::ore::BindingKind::comparisonSampler)
                sd.compare = rive::ore::CompareFunction::lessEqual;

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

    // GL / GLES bind UBO blocks and sampler units by name after linking, so
    // build the name→slot fixup table for the GLSL/ESSL targets.
    std::vector<uint8_t> vsFixup, fsFixup;
    if (gpuLang == GpuShaderLanguage::glsl)
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

    GpuShaderSource vs;
    vs.language = gpuLang;
    vs.code = gpuShaderSourceBytes (vsInfo->source);
    vs.bindingMap = vsMap;
    vs.glFixup = vsFixup;
    vs.entryPoint = resolveEntry (*vsInfo);

    GpuShaderSource fs;
    fs.language = gpuLang;
    fs.code = gpuShaderSourceBytes (fsInfo->source);
    fs.bindingMap = fsMap;
    fs.glFixup = fsFixup;
    fs.entryPoint = resolveEntry (*fsInfo);

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

    ShaderBundle bundle;
    for (const auto& info : vsBundle.getReference().getShaders())
        bundle.addShader (info);
    for (const auto& info : fsBundle.getReference().getShaders())
        bundle.addShader (info);

    return compileFromBundle (ctx, bundle, pipelineOptions);
}

#endif

} // namespace yup
