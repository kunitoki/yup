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

#if YUP_RIVE_USE_VULKAN

namespace yup
{

//==============================================================================

class GpuComputePipelineVulkan final : public GpuComputePipeline
{
public:
    /** One descriptor of the pipeline layout, as the shader declared it. */
    struct Binding
    {
        uint32_t group = 0;
        uint32_t binding = 0;
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    };

    GpuComputePipelineVulkan (const VulkanDevice& device, GpuWorkgroupSize wgs)
        : vulkanDevice (device)
        , workgroupSize (wgs)
    {
    }

    /** A pass keeps the pipeline alive until its commands complete, so no GPU work can
        still reference these objects. */
    ~GpuComputePipelineVulkan() override
    {
        const auto& fn = vulkanDevice.getFunctions();
        const auto device = vulkanDevice.getDevice();

        if (pipeline != VK_NULL_HANDLE)
            fn.DestroyPipeline (device, pipeline, nullptr);

        if (pipelineLayout != VK_NULL_HANDLE)
            fn.DestroyPipelineLayout (device, pipelineLayout, nullptr);

        for (auto setLayout : setLayouts)
            fn.DestroyDescriptorSetLayout (device, setLayout, nullptr);
    }

    GpuWorkgroupSize getWorkgroupSize() const noexcept override { return workgroupSize; }

    const VulkanDevice& vulkanDevice;
    GpuWorkgroupSize workgroupSize;
    std::vector<Binding> bindings;
    std::vector<VkDescriptorSetLayout> setLayouts;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

//==============================================================================

namespace
{

std::optional<VkDescriptorType> toVkDescriptorType (rive::ore::ResourceKind kind)
{
    switch (kind)
    {
        case rive::ore::ResourceKind::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case rive::ore::ResourceKind::StorageBufferRO:
        case rive::ore::ResourceKind::StorageBufferRW:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case rive::ore::ResourceKind::SampledTexture:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case rive::ore::ResourceKind::StorageTexture:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case rive::ore::ResourceKind::Sampler:
        case rive::ore::ResourceKind::ComparisonSampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
    }

    return std::nullopt;
}

} // namespace

//==============================================================================

ResultValue<GpuComputePipeline::Ptr> yup_constructComputePipelineVulkan (GpuDevice& ctx,
                                                                         const GpuShaderSource& source,
                                                                         const GpuWorkgroupSize& workgroupSize)
{
    if (source.language != GpuShaderLanguage::spirv)
        return makeResultValueFail ("Vulkan compute pipelines need a SPIR-V module");

    if (source.code.empty() || source.code.size() % sizeof (uint32_t) != 0)
        return makeResultValueFail ("Compute shader SPIR-V module is empty or truncated");

    // SPIR-V carries no layout of its own that Vulkan can create pipelines from, the
    // binding map built from the shader reflection describes it
    auto bindingMapBlob = source.bindingMap;
    auto wgs = workgroupSize;

    if (bindingMapBlob.empty())
    {
#if YUP_ENABLE_SHADER_TRANSPILER
        auto reflection = ShaderTranspiler().reflectFromSPIRV (MemoryBlock (source.code.data(), source.code.size()), ShaderLanguage::spirv);
        if (reflection.failed())
            return makeResultValueFail ("SPIR-V reflection failed: " + reflection.getErrorMessage());

        bindingMapBlob = makeShaderBindingMapBlob (reflection.getReference(), ShaderStage::compute);

        const auto& reflWgs = reflection.getReference().workgroupSize;
        if (wgs.x == 1 && wgs.y == 1 && wgs.z == 1 && reflWgs.x > 0 && reflWgs.y > 0 && reflWgs.z > 0)
            wgs = GpuWorkgroupSize { reflWgs.x, reflWgs.y, reflWgs.z };
#else
        return makeResultValueFail ("Vulkan compute pipelines need the shader binding map, or YUP_ENABLE_SHADER_TRANSPILER to reflect it");
#endif
    }

    rive::ore::BindingMap bindingMap;
    if (! rive::ore::BindingMap::fromBlob (bindingMapBlob.data(), bindingMapBlob.size(), &bindingMap))
        return makeResultValueFail ("Compute shader binding map is unreadable");

    const auto& vulkanDevice = static_cast<GpuDeviceVulkan&> (ctx).getVulkanDevice();
    const auto& fn = vulkanDevice.getFunctions();
    const auto device = vulkanDevice.getDevice();

    auto pipeline = ReferenceCountedObjectPtr<GpuComputePipelineVulkan> (new GpuComputePipelineVulkan (vulkanDevice, wgs));

    uint32_t groupCount = 0;
    for (size_t i = 0; i < bindingMap.size(); ++i)
    {
        const auto& entry = bindingMap.at (i);

        const auto type = toVkDescriptorType (entry.kind);
        if (! type.has_value())
            return makeResultValueFail ("Compute shader uses an unsupported resource kind");

        pipeline->bindings.push_back ({ entry.group, entry.binding, *type });
        groupCount = jmax (groupCount, static_cast<uint32_t> (entry.group) + 1u);
    }

    // Groups the shader skips still need a (empty) layout, every set index must be valid
    for (uint32_t group = 0; group < groupCount; ++group)
    {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        for (const auto& binding : pipeline->bindings)
        {
            if (binding.group != group)
                continue;

            VkDescriptorSetLayoutBinding layoutBinding {};
            layoutBinding.binding = binding.binding;
            layoutBinding.descriptorType = binding.type;
            layoutBinding.descriptorCount = 1;
            layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindings.push_back (layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t> (layoutBindings.size());
        layoutInfo.pBindings = layoutBindings.data();

        VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
        if (fn.CreateDescriptorSetLayout (device, &layoutInfo, nullptr, &setLayout) != VK_SUCCESS)
            return makeResultValueFail ("vkCreateDescriptorSetLayout failed for a compute pipeline");

        pipeline->setLayouts.push_back (setLayout);
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t> (pipeline->setLayouts.size());
    pipelineLayoutInfo.pSetLayouts = pipeline->setLayouts.data();

    if (fn.CreatePipelineLayout (device, &pipelineLayoutInfo, nullptr, &pipeline->pipelineLayout) != VK_SUCCESS)
        return makeResultValueFail ("vkCreatePipelineLayout failed for a compute pipeline");

    VkShaderModuleCreateInfo moduleInfo {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = source.code.size();
    moduleInfo.pCode = reinterpret_cast<const uint32_t*> (source.code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (fn.CreateShaderModule (device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS)
        return makeResultValueFail ("vkCreateShaderModule failed for a compute shader");

    const auto entryPoint = source.entryPoint.isNotEmpty() ? source.entryPoint : String ("main");

    VkComputePipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineInfo.stage.module = shaderModule;
    pipelineInfo.stage.pName = entryPoint.toRawUTF8();
    pipelineInfo.layout = pipeline->pipelineLayout;

    const auto result = fn.CreateComputePipelines (device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline->pipeline);
    fn.DestroyShaderModule (device, shaderModule, nullptr);

    if (result != VK_SUCCESS)
        return makeResultValueFail ("vkCreateComputePipelines failed (VkResult " + String (static_cast<int> (result)) + ")");

    return makeResultValueOk (GpuComputePipeline::Ptr (pipeline.get()));
}

} // namespace yup

#endif // YUP_RIVE_USE_VULKAN
