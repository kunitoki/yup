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

ResultValue<GpuComputePipeline::Ptr> GpuComputePipeline::compile (GpuDevice::Ptr ctx,
                                                                  const GpuShaderSource& source,
                                                                  const GpuWorkgroupSize& workgroupSize)
{
    if (ctx == nullptr)
        return makeResultValueFail ("GpuDevice is null");

    if (! ctx->isComputeAvailable())
        return makeResultValueFail ("Compute shaders are not available on this backend");

    switch (ctx->getPlatform())
    {
#if YUP_RIVE_USE_METAL && YUP_APPLE
        case GpuPlatform::Metal:
            return yup_constructComputePipelineMetal (*ctx, source, workgroupSize);
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
        case GpuPlatform::Direct3D:
            return yup_constructComputePipelineD3D11 (*ctx, source, workgroupSize);
#endif

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        case GpuPlatform::WebGPU:
            return yup_constructComputePipelineWebGPU (*ctx, source, workgroupSize);
#endif

#if YUP_RIVE_USE_VULKAN
        case GpuPlatform::Vulkan:
            return yup_constructComputePipelineVulkan (*ctx, source, workgroupSize);
#endif

#if YUP_RHI_USE_GL_COMPUTE
        case GpuPlatform::OpenGL:
        case GpuPlatform::OpenGLES:
        {
            std::optional<ResultValue<GpuComputePipeline::Ptr>> result;
            ctx->runOnComputeContext ([&]
            {
                result.emplace (yup_constructComputePipelineGL (ctx, source, workgroupSize));
            });

            if (! result.has_value())
                return makeResultValueFail ("GL compute context is not available");

            return std::move (*result);
        }
#endif

        default:
            return makeResultValueFail ("Unsupported GPU platform for compute pipelines");
    }
}

//==============================================================================

ResultValue<GpuComputePipeline::Ptr> GpuComputePipeline::compileFromBundle (GpuDevice::Ptr ctx,
                                                                            const ShaderBundle& bundle,
                                                                            const GpuWorkgroupSize& workgroupSize)
{
    if (ctx == nullptr)
        return makeResultValueFail ("GpuDevice is null");

    if (! ctx->isComputeAvailable())
        return makeResultValueFail ("Compute shaders are not available on this backend");

    GpuShaderLanguage targetLang;
    switch (ctx->getPlatform())
    {
        case GpuPlatform::Metal:
            targetLang = GpuShaderLanguage::msl;
            break;
        case GpuPlatform::Direct3D:
            targetLang = GpuShaderLanguage::hlsl;
            break;
        case GpuPlatform::WebGPU:
            targetLang = GpuShaderLanguage::wgsl;
            break;
        case GpuPlatform::OpenGL:
        case GpuPlatform::OpenGLES:
            targetLang = GpuShaderLanguage::glsl;
            break;
        case GpuPlatform::Vulkan:
            targetLang = GpuShaderLanguage::spirv;
            break;
        default:
            return makeResultValueFail ("Unsupported GPU platform");
    }

    auto* shader = bundle.findShader (ShaderStage::compute, shaderLanguageForApi (ctx->getPlatform()));
    if (shader == nullptr)
        return makeResultValueFail ("Bundle does not contain a compute shader for this platform");

    GpuShaderSource source;
    source.language = targetLang;
    source.code = shaderCodeBytes (*shader);

    // Vulkan creates the pipeline layout from it
    if (targetLang == GpuShaderLanguage::spirv)
        source.bindingMap = makeShaderBindingMapBlob (shader->reflection, ShaderStage::compute);

    source.entryPoint = (targetLang == GpuShaderLanguage::msl && shader->entryPoint == "main") ? String ("main0") : shader->entryPoint;

    GpuWorkgroupSize wgs = workgroupSize;
    if (wgs.x == 1 && wgs.y == 1 && wgs.z == 1)
    {
        const auto& reflWgs = shader->reflection.workgroupSize;
        if (reflWgs.x > 0 && reflWgs.y > 0 && reflWgs.z > 0)
            wgs = GpuWorkgroupSize { reflWgs.x, reflWgs.y, reflWgs.z };
    }

    return compile (ctx, source, wgs);
}

#if YUP_ENABLE_SHADER_TRANSPILER

ResultValue<GpuComputePipeline::Ptr> GpuComputePipeline::compileFromGlsl (GpuDevice::Ptr ctx,
                                                                          const String& glsl,
                                                                          const GpuWorkgroupSize& workgroupSize)
{
    if (ctx == nullptr)
        return makeResultValueFail ("GpuDevice is null");

    if (! ctx->isComputeAvailable())
        return makeResultValueFail ("Compute shaders are not available on this backend");

    GpuShaderLanguage targetLang;
    switch (ctx->getPlatform())
    {
        case GpuPlatform::Metal:
            targetLang = GpuShaderLanguage::msl;
            break;
        case GpuPlatform::Direct3D:
            targetLang = GpuShaderLanguage::hlsl;
            break;
        case GpuPlatform::WebGPU:
            targetLang = GpuShaderLanguage::wgsl;
            break;
        case GpuPlatform::OpenGL:
        case GpuPlatform::OpenGLES:
            targetLang = GpuShaderLanguage::glsl;
            break;
        case GpuPlatform::Vulkan:
            targetLang = GpuShaderLanguage::spirv;
            break;
        default:
            return makeResultValueFail ("Unsupported GPU platform");
    }

    ShaderTranspiler transpiler;

    if (targetLang == GpuShaderLanguage::spirv)
    {
        // Vulkan loads the SPIR-V itself, with the layout described by its binding map
        TranspileOptions options;
        auto spirv = transpiler.compileToSPIRV (glsl, ShaderStage::compute, ShaderLanguage::glsl, options);
        if (spirv.failed())
            return makeResultValueFail ("GLSL compilation failed: " + spirv.getErrorMessage());

        auto reflection = transpiler.reflectFromSPIRV (spirv.getReference(), ShaderLanguage::spirv, options);
        if (reflection.failed())
            return makeResultValueFail ("SPIR-V reflection failed: " + reflection.getErrorMessage());

        GpuWorkgroupSize wgs = workgroupSize;
        const auto& reflWgs = reflection.getReference().workgroupSize;
        if (wgs.x == 1 && wgs.y == 1 && wgs.z == 1 && reflWgs.x > 0 && reflWgs.y > 0 && reflWgs.z > 0)
            wgs = GpuWorkgroupSize { reflWgs.x, reflWgs.y, reflWgs.z };

        const auto& module = spirv.getReference();
        auto* bytes = static_cast<const uint8*> (module.getData());

        GpuShaderSource source;
        source.language = targetLang;
        source.code.assign (bytes, bytes + module.getSize());
        source.bindingMap = makeShaderBindingMapBlob (reflection.getReference(), ShaderStage::compute);

        return compile (ctx, source, wgs);
    }

    auto transpileResult = transpiler.transpile (glsl, ShaderStage::compute, ShaderLanguage::glsl, shaderLanguageForApi (ctx->getPlatform()));
    if (transpileResult.failed())
        return makeResultValueFail ("GLSL transpilation failed: " + transpileResult.getErrorMessage());

    auto reflectionResult = transpiler.reflect (glsl, ShaderStage::compute, ShaderLanguage::glsl);
    GpuWorkgroupSize wgs = workgroupSize;
    if (wgs.x == 1 && wgs.y == 1 && wgs.z == 1 && reflectionResult.wasOk())
    {
        const auto& reflWgs = reflectionResult.getReference().workgroupSize;
        if (reflWgs.x > 0 && reflWgs.y > 0 && reflWgs.z > 0)
            wgs = GpuWorkgroupSize { reflWgs.x, reflWgs.y, reflWgs.z };
    }

    const auto& nativeSource = transpileResult.getReference();

    GpuShaderSource source;
    source.language = targetLang;
    source.code = gpuShaderSourceBytes (nativeSource);

    return compile (ctx, source, wgs);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER

} // namespace yup
