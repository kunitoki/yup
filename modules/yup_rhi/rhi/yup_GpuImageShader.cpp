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

#if YUP_ENABLE_SHADER_TRANSPILER

namespace
{

// The source of shaders/yup_GpuImageShader.vert, below its license: keep the two identical
constexpr const char* imageShaderVertexSource = R"(#version 450

// The vertex stage of a GpuImageShader: one triangle covering the target. No vertex buffer:
// positions come from gl_VertexIndex. uv runs from (0, 0) at the top-left texel to (1, 1) at the
// bottom-right one on every backend, so a shader samples its input at the pixel it writes.

layout(location = 0) out vec2 uv;

void main()
{
    vec2 p = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));

    uv = vec2(p.x, 1.0 - p.y);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)";

} // namespace

#endif

//==============================================================================

GpuImageShader::~GpuImageShader() = default;

//==============================================================================

ResultValue<GpuImageShader::Ptr> GpuImageShader::createFromBundle (ShaderBundle::Ptr bundle)
{
    if (bundle == nullptr)
        return makeResultValueFail ("No shader bundle");

    // Every variant of a stage reflects the same shader, so the first one tells what it declares
    const auto findStage = [&bundle] (ShaderStage stage) -> const ShaderInfo*
    {
        for (const auto& shader : bundle->getShaders())
        {
            if (shader.stage == stage)
                return &shader;
        }

        return nullptr;
    };

    if (findStage (ShaderStage::vertex) == nullptr)
        return makeResultValueFail ("The shader bundle has no vertex stage");

    const auto* fragment = findStage (ShaderStage::fragment);
    if (fragment == nullptr)
        return makeResultValueFail ("The shader bundle has no fragment stage");

    const auto& reflection = fragment->reflection;

    // GLSL and ESSL variants also list the samplers they fold each texture and sampler into
    if (reflection.sampledImages.size() > reflection.glCombinedSamplers.size())
        return makeResultValueFail ("The fragment stage declares a combined sampler2D, which cannot be bound: declare a texture2D and a sampler, and combine them where the texture is read");

    if (reflection.uniformBuffers.size() > 1)
        return makeResultValueFail ("The fragment stage declares " + String (static_cast<int> (reflection.uniformBuffers.size())) + " uniform blocks: an image shader has one at most");

    // A bundle can come from a file: a block larger than uniforms hold is refused before its bytes are made
    if (! reflection.uniformBuffers.empty() && reflection.uniformBuffers.front().blockSize > ShaderUniforms::maxSizeInBytes)
        return makeResultValueFail ("The fragment stage declares a uniform block of " + String (reflection.uniformBuffers.front().blockSize) + " bytes: an image shader takes " + String (static_cast<int> (ShaderUniforms::maxSizeInBytes)) + " at most");

    if (reflection.separateImages.size() > 2)
        return makeResultValueFail ("The fragment stage declares " + String (static_cast<int> (reflection.separateImages.size())) + " textures: an image shader has two at most");

    Ptr shader (new GpuImageShader());
    shader->bundle = bundle;

    if (! reflection.uniformBuffers.empty())
    {
        const auto& block = reflection.uniformBuffers.front();
        shader->blockUniforms = ShaderUniforms (block);
        shader->blockBinding = { static_cast<int> (block.set), static_cast<int> (block.binding) };
    }

    for (const auto& image : reflection.separateImages)
    {
        if (image.imageDim != ShaderReflection::ImageDimension::dim2D || image.imageArrayed || image.imageMS)
            return makeResultValueFail ("The fragment stage declares the texture " + image.name + ", which is not a texture2D");

        shader->textureBindings.push_back ({ static_cast<int> (image.set), static_cast<int> (image.binding) });
    }

    std::sort (shader->textureBindings.begin(), shader->textureBindings.end(), [] (const Binding& a, const Binding& b)
    {
        return std::tie (a.group, a.binding) < std::tie (b.group, b.binding);
    });

    return makeResultValueOk (std::move (shader));
}

ResultValue<GpuImageShader::Ptr> GpuImageShader::createFromData (const void* data, size_t size)
{
    if (data == nullptr || size == 0)
        return makeResultValueFail ("No shader data");

    static constexpr char riffMagic[] = { 'R', 'I', 'F', 'F' };

    if (size >= sizeof (riffMagic) && std::memcmp (data, riffMagic, sizeof (riffMagic)) == 0)
    {
        auto loaded = ShaderBundle::loadFromData (data, size);
        if (loaded.failed())
            return makeResultValueFail (loaded.getErrorMessage());

        return createFromBundle (new ShaderBundle (std::move (loaded.getReference())));
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    return createFromGlsl (String::createStringFromData (data, static_cast<int> (size)));
#else
    return makeResultValueFail ("The shader is not a compiled .ysl bundle: GLSL source is only compiled when the shader transpiler is built in");
#endif
}

#if YUP_ENABLE_SHADER_TRANSPILER

ResultValue<GpuImageShader::Ptr> GpuImageShader::createFromGlsl (const String& fragmentGlsl)
{
    ShaderBundleCompileRequest request;

    // Both stages go in one request, for the languages yup_shader_bundler targets by default
    const auto addStage = [&request] (ShaderStage stage, const String& source)
    {
        auto& entry = request.entries.emplace_back();
        entry.stage = stage;
        entry.source = source;
        entry.targetLanguages = { ShaderLanguage::glsl, ShaderLanguage::essl, ShaderLanguage::hlsl, ShaderLanguage::msl, ShaderLanguage::spirv, ShaderLanguage::wgsl };
    };

    addStage (ShaderStage::vertex, imageShaderVertexSource);
    addStage (ShaderStage::fragment, fragmentGlsl);

    auto compiled = ShaderBundleCompiler().compile (request);
    if (compiled.failed())
        return makeResultValueFail (compiled.getErrorMessage());

    return createFromBundle (new ShaderBundle (std::move (compiled.getReference())));
}

#endif

//==============================================================================

int GpuImageShader::getNumInputTextures() const noexcept
{
    return static_cast<int> (textureBindings.size());
}

ShaderUniforms GpuImageShader::createUniforms() const
{
    return blockUniforms;
}

//==============================================================================

Result GpuImageShader::draw (const GpuDevice::Ptr& device,
                             GpuFrame& frame,
                             GpuTarget& target,
                             const ShaderUniforms& uniforms,
                             const GpuTexture::Ptr& firstInput,
                             const GpuTexture::Ptr& secondInput) const
{
    if (device == nullptr || ! device->isGpuAvailable() || device->isDeviceLost())
        return Result::fail ("The device has no GPU to draw the image shader with");

    // Uniforms of another size would leave the shader reading past them
    jassert (uniforms.getSizeInBytes() == blockUniforms.getSizeInBytes());
    if (uniforms.getSizeInBytes() != blockUniforms.getSizeInBytes())
        return Result::fail ("The uniforms are not the ones of the image shader: take them from createUniforms()");

    const GpuTexture::Ptr inputs[] = { firstInput, secondInput };

    for (size_t i = 0; i < textureBindings.size(); ++i)
    {
        if (inputs[i] == nullptr || ! inputs[i]->isValid())
            return Result::fail ("The image shader has no texture to read as its input " + String (static_cast<int> (i) + 1));
    }

    const auto texture = target.asTexture();
    if (texture == nullptr || isDepthStencilFormat (texture->getFormat()) || texture->getSampleCount() != 1)
        return Result::fail ("An image shader draws into a color target that is not multisampled");

    // The device keeps the pipeline, so the shader holds nothing that must not outlive a device
    const auto pipeline = device->getOrCompilePipelineFor (pipelineOwner, texture->getFormat(), [this, &device] (GpuTextureFormat format)
    {
        GpuPipelineOptions options;
        auto& colorTarget = options.colorTargets.emplace_back();
        colorTarget.format = format;
        colorTarget.blendEnabled = false;

        return GpuPipeline::compileFromBundle (device, *bundle, options);
    });

    if (pipeline.failed())
        return Result::fail (pipeline.getErrorMessage());

    auto pass = target.beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    if (! pass.isValid())
        return Result::fail ("Unable to begin the render pass of the image shader");

    pass.setPipeline (pipeline.getReference());

    if (! uniforms.isEmpty())
        pass.setUniformBuffer (blockBinding.group, blockBinding.binding, uniforms.getData(), uniforms.getSizeInBytes());

    for (size_t i = 0; i < textureBindings.size(); ++i)
        pass.setTexture (textureBindings[i].group, textureBindings[i].binding, inputs[i]);

    const bool drawn = pass.draw (3);
    pass.finish();

    return drawn ? Result::ok() : Result::fail ("Unable to draw the image shader");
}

} // namespace yup
