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

#if (YUP_EMSCRIPTEN && RIVE_WEBGPU) || YUP_RIVE_USE_DAWN

namespace yup
{

//==============================================================================

class GpuComputePipelineWebGPU final : public GpuComputePipeline
{
public:
    GpuComputePipelineWebGPU (wgpu::ComputePipeline pipeline, GpuWorkgroupSize wgs)
        : computePipeline (std::move (pipeline))
        , workgroupSize (wgs)
    {
    }

    GpuWorkgroupSize getWorkgroupSize() const noexcept override { return workgroupSize; }

    wgpu::ComputePipeline getPipeline() const noexcept { return computePipeline; }

private:
    wgpu::ComputePipeline computePipeline;
    GpuWorkgroupSize workgroupSize;
};

//==============================================================================

ResultValue<GpuComputePipeline::Ptr> yup_constructComputePipelineWebGPU (GpuDevice& ctx,
                                                                         const GpuShaderSource& source,
                                                                         const GpuWorkgroupSize& workgroupSize)
{
    if (source.code == nullptr || source.codeSize == 0)
        return makeResultValueFail ("Compute shader source is empty");

    if (source.language != GpuShaderLanguage::wgsl)
        return makeResultValueFail ("WebGPU compute shaders must be WGSL");

    const char* wgslSource = static_cast<const char*> (source.code);
    const char* entryPointName = source.entryPoint != nullptr ? source.entryPoint : "main";

    wgpu::Device device;
#if YUP_EMSCRIPTEN && RIVE_WEBGPU
    device = static_cast<GpuDeviceWebGPU&> (ctx).getWgpuDevice();
#elif YUP_RIVE_USE_DAWN
    device = static_cast<GpuDeviceDawn&> (ctx).getDevice();
#endif

    wgpu::ShaderModuleWGSLDescriptor wgslDesc {};
    wgslDesc.code = wgslSource;

    wgpu::ShaderModuleDescriptor shaderModuleDesc {};
    shaderModuleDesc.nextInChain = &wgslDesc;
    shaderModuleDesc.label = "GpuComputePipeline CS";

    wgpu::ShaderModule shaderModule = device.CreateShaderModule (&shaderModuleDesc);
    if (shaderModule == nullptr)
        return makeResultValueFail ("WebGPU compute shader module creation failed");

    wgpu::ComputePipelineDescriptor pipelineDesc {};
    pipelineDesc.label = "GpuComputePipeline";
    pipelineDesc.compute.module = shaderModule;
    pipelineDesc.compute.entryPoint = entryPointName;

    wgpu::ComputePipeline computePipeline = device.CreateComputePipeline (&pipelineDesc);
    if (computePipeline == nullptr)
        return makeResultValueFail ("WebGPU compute pipeline creation failed");

    return makeResultValueOk (GpuComputePipeline::Ptr (new GpuComputePipelineWebGPU (std::move (computePipeline), workgroupSize)));
}

} // namespace yup

#endif // WebGPU / Dawn
