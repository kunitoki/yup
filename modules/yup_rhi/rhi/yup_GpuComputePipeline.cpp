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
#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
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
#elif YUP_RIVE_USE_DAWN
        case GpuPlatform::WebGPU:
            return yup_constructComputePipelineWebGPU (*ctx, source, workgroupSize);
#endif
#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID
        case GpuPlatform::OpenGL:
        case GpuPlatform::OpenGLES:
            return yup_constructComputePipelineGL (source, workgroupSize);
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
        default:
            return makeResultValueFail ("Unsupported GPU platform");
    }

    auto* shader = bundle.findShader (ShaderStage::compute, shaderLanguageForApi (ctx->getPlatform()));
    if (shader == nullptr)
        return makeResultValueFail ("Bundle does not contain a compute shader for this platform");

    GpuShaderSource source;
    source.language = targetLang;
    source.code = static_cast<const char*> (shader->source.toRawUTF8());
    source.codeSize = static_cast<uint32_t> (shader->source.getNumBytesAsUTF8());
    source.entryPoint = shader->entryPoint.toRawUTF8();

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
        default:
            return makeResultValueFail ("Unsupported GPU platform");
    }

    ShaderTranspiler transpiler;
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
    source.code = static_cast<const char*> (nativeSource.toRawUTF8());
    source.codeSize = static_cast<uint32_t> (nativeSource.getNumBytesAsUTF8());

    return compile (ctx, source, wgs);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER

} // namespace yup
