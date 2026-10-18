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

#if YUP_EMSCRIPTEN && RIVE_WEBGPU

namespace yup
{

//==============================================================================

class GpuComputePipelineWebGPU final : public GpuComputePipeline
{
public:
    GpuComputePipelineWebGPU (wgpu::ComputePipeline pipeline,
                              std::vector<wgpu::BindGroupLayout> layouts,
                              std::vector<WgslBindingLayout::Entry> declarations,
                              GpuWorkgroupSize wgs)
        : computePipeline (std::move (pipeline))
        , groupLayouts (std::move (layouts))
        , resources (std::move (declarations))
        , workgroupSize (wgs)
    {
    }

    GpuWorkgroupSize getWorkgroupSize() const noexcept override { return workgroupSize; }

    wgpu::ComputePipeline getPipeline() const noexcept { return computePipeline; }

    /** The layout of every bind group from 0 up to the highest group the shader declares. */
    const std::vector<wgpu::BindGroupLayout>& getGroupLayouts() const noexcept { return groupLayouts; }

    /** Every resource the shader declares, each of which a dispatch must bind. */
    const std::vector<WgslBindingLayout::Entry>& getResources() const noexcept { return resources; }

    const WgslBindingLayout::Entry* findResource (int group, int binding) const noexcept
    {
        auto found = std::find_if (resources.begin(), resources.end(), [&] (const auto& r)
        {
            return static_cast<int> (r.group) == group && static_cast<int> (r.binding) == binding;
        });

        return found != resources.end() ? std::addressof (*found) : nullptr;
    }

private:
    wgpu::ComputePipeline computePipeline;
    std::vector<wgpu::BindGroupLayout> groupLayouts;
    std::vector<WgslBindingLayout::Entry> resources;
    GpuWorkgroupSize workgroupSize;
};

//==============================================================================

namespace
{

wgpu::TextureViewDimension toWgpuViewDimension (WgslBindingLayout::ViewDimension dimension) noexcept
{
    using Dimension = WgslBindingLayout::ViewDimension;

    switch (dimension)
    {
        case Dimension::texture1D:
            return wgpu::TextureViewDimension::e1D;
        case Dimension::texture2D:
            return wgpu::TextureViewDimension::e2D;
        case Dimension::array2D:
            return wgpu::TextureViewDimension::e2DArray;
        case Dimension::cube:
            return wgpu::TextureViewDimension::Cube;
        case Dimension::cubeArray:
            return wgpu::TextureViewDimension::CubeArray;
        case Dimension::texture3D:
            return wgpu::TextureViewDimension::e3D;
    }

    return wgpu::TextureViewDimension::e2D;
}

std::optional<wgpu::TextureFormat> toWgpuStorageFormat (const String& format)
{
    static const std::map<String, wgpu::TextureFormat> formats = {
        { "rgba8unorm", wgpu::TextureFormat::RGBA8Unorm },
        { "rgba8snorm", wgpu::TextureFormat::RGBA8Snorm },
        { "rgba8uint", wgpu::TextureFormat::RGBA8Uint },
        { "rgba8sint", wgpu::TextureFormat::RGBA8Sint },
        { "rgba16uint", wgpu::TextureFormat::RGBA16Uint },
        { "rgba16sint", wgpu::TextureFormat::RGBA16Sint },
        { "rgba16float", wgpu::TextureFormat::RGBA16Float },
        { "r32uint", wgpu::TextureFormat::R32Uint },
        { "r32sint", wgpu::TextureFormat::R32Sint },
        { "r32float", wgpu::TextureFormat::R32Float },
        { "rg32uint", wgpu::TextureFormat::RG32Uint },
        { "rg32sint", wgpu::TextureFormat::RG32Sint },
        { "rg32float", wgpu::TextureFormat::RG32Float },
        { "rgba32uint", wgpu::TextureFormat::RGBA32Uint },
        { "rgba32sint", wgpu::TextureFormat::RGBA32Sint },
        { "rgba32float", wgpu::TextureFormat::RGBA32Float },
        { "bgra8unorm", wgpu::TextureFormat::BGRA8Unorm }
    };

    const auto found = formats.find (format);
    return found != formats.end() ? std::optional (found->second) : std::nullopt;
}

ResultValue<wgpu::BindGroupLayoutEntry> toWgpuLayoutEntry (const WgslBindingLayout::Entry& resource)
{
    using Layout = WgslBindingLayout;

    wgpu::BindGroupLayoutEntry entry {};
    entry.binding = resource.binding;
    entry.visibility = wgpu::ShaderStage::Compute;

    switch (resource.kind)
    {
        case Layout::ResourceKind::buffer:
            entry.buffer.type = resource.bufferType == Layout::BufferType::uniform           ? wgpu::BufferBindingType::Uniform
                              : resource.bufferType == Layout::BufferType::readOnlyStorage ? wgpu::BufferBindingType::ReadOnlyStorage
                                                                                           : wgpu::BufferBindingType::Storage;
            break;

        case Layout::ResourceKind::sampler:
            entry.sampler.type = resource.samplerType == Layout::SamplerType::comparison ? wgpu::SamplerBindingType::Comparison
                                                                                         : wgpu::SamplerBindingType::Filtering;
            break;

        case Layout::ResourceKind::texture:
            entry.texture.viewDimension = toWgpuViewDimension (resource.viewDimension);
            entry.texture.multisampled = resource.multisampled;

            switch (resource.sampleType)
            {
                case Layout::SampleType::floatFilterable:
                    entry.texture.sampleType = wgpu::TextureSampleType::Float;
                    break;
                case Layout::SampleType::floatUnfilterable:
                    entry.texture.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
                    break;
                case Layout::SampleType::depth:
                    entry.texture.sampleType = wgpu::TextureSampleType::Depth;
                    break;
                case Layout::SampleType::sint:
                    entry.texture.sampleType = wgpu::TextureSampleType::Sint;
                    break;
                case Layout::SampleType::uint:
                    entry.texture.sampleType = wgpu::TextureSampleType::Uint;
                    break;
            }
            break;

        case Layout::ResourceKind::storageTexture:
        {
            const auto format = toWgpuStorageFormat (resource.storageFormat);
            if (! format)
                return makeResultValueFail ("Unsupported storage texture format '" + resource.storageFormat + "' for '" + resource.name + "'");

            entry.storageTexture.viewDimension = toWgpuViewDimension (resource.viewDimension);
            entry.storageTexture.format = *format;
            entry.storageTexture.access = resource.storageAccess == Layout::StorageAccess::readOnly    ? wgpu::StorageTextureAccess::ReadOnly
                                        : resource.storageAccess == Layout::StorageAccess::writeOnly ? wgpu::StorageTextureAccess::WriteOnly
                                                                                                     : wgpu::StorageTextureAccess::ReadWrite;
            break;
        }
    }

    return makeResultValueOk (entry);
}

} // namespace

//==============================================================================

ResultValue<GpuComputePipeline::Ptr> yup_constructComputePipelineWebGPU (GpuDevice& ctx,
                                                                         const GpuShaderSource& source,
                                                                         const GpuWorkgroupSize& workgroupSize)
{
    if (source.code.empty())
        return makeResultValueFail ("Compute shader source is empty");

    if (source.language != GpuShaderLanguage::wgsl)
        return makeResultValueFail ("WebGPU compute shaders must be WGSL");

    const std::string wgslSource (reinterpret_cast<const char*> (source.code.data()), source.code.size());
    const char* entryPointName = source.entryPoint.isNotEmpty() ? source.entryPoint.toRawUTF8() : "main";

    // An explicit layout holds every declared resource, so binding one the shader never uses stays valid
    auto resources = WgslBindingLayout::parse (String (wgslSource));
    if (resources.failed())
        return makeResultValueFail (resources.getErrorMessage());

    uint32_t groupCount = 0;
    for (const auto& resource : resources.getReference())
        groupCount = jmax (groupCount, resource.group + 1);

    wgpu::Device device = static_cast<GpuDeviceWebGPU&> (ctx).getWgpuDevice();

    std::vector<wgpu::BindGroupLayout> groupLayouts;
    for (uint32_t group = 0; group < groupCount; ++group)
    {
        std::vector<wgpu::BindGroupLayoutEntry> entries;
        for (const auto& resource : resources.getReference())
        {
            if (resource.group != group)
                continue;

            auto entry = toWgpuLayoutEntry (resource);
            if (entry.failed())
                return makeResultValueFail (entry.getErrorMessage());

            entries.push_back (entry.getReference());
        }

        wgpu::BindGroupLayoutDescriptor layoutDesc {};
        layoutDesc.label = "GpuComputePipeline group";
        layoutDesc.entryCount = entries.size();
        layoutDesc.entries = entries.data();
        groupLayouts.push_back (device.CreateBindGroupLayout (&layoutDesc));
    }

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc {};
    pipelineLayoutDesc.label = "GpuComputePipeline";
    pipelineLayoutDesc.bindGroupLayoutCount = groupLayouts.size();
    pipelineLayoutDesc.bindGroupLayouts = groupLayouts.data();

    wgpu::ShaderSourceWGSL wgslDesc {};
    wgslDesc.code = wgslSource.c_str();

    wgpu::ShaderModuleDescriptor shaderModuleDesc {};
    shaderModuleDesc.nextInChain = &wgslDesc;
    shaderModuleDesc.label = "GpuComputePipeline CS";

    wgpu::ShaderModule shaderModule = device.CreateShaderModule (&shaderModuleDesc);
    if (shaderModule == nullptr)
        return makeResultValueFail ("WebGPU compute shader module creation failed");

    wgpu::ComputePipelineDescriptor pipelineDesc {};
    pipelineDesc.label = "GpuComputePipeline";
    pipelineDesc.layout = device.CreatePipelineLayout (&pipelineLayoutDesc);
    pipelineDesc.compute.module = shaderModule;
    pipelineDesc.compute.entryPoint = entryPointName;

    wgpu::ComputePipeline computePipeline = device.CreateComputePipeline (&pipelineDesc);
    if (computePipeline == nullptr)
        return makeResultValueFail ("WebGPU compute pipeline creation failed");

    return makeResultValueOk (GpuComputePipeline::Ptr (new GpuComputePipelineWebGPU (std::move (computePipeline),
                                                                                      std::move (groupLayouts),
                                                                                      std::move (resources.getReference()),
                                                                                      workgroupSize)));
}

} // namespace yup

#endif // YUP_EMSCRIPTEN && RIVE_WEBGPU
