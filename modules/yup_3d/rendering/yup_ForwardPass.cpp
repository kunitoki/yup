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

namespace
{

//==============================================================================
// Precompiled bundles (do not edit the .inc by hand). Regenerate them after changing
// shaders/yup_ForwardPass.* or shaders/yup_Background.* with: just yup3d_shaders

constexpr uint8_t forwardPassShaderBundle[] = {
#include "../shaders/yup_ForwardPass.inc"
};
constexpr uint8_t forwardPassFullShaderBundle[] = {
#include "../shaders/yup_ForwardPass_full.inc"
};
constexpr uint8_t backgroundShaderBundle[] = {
#include "../shaders/yup_Background.inc"
};
constexpr uint8_t backgroundFullShaderBundle[] = {
#include "../shaders/yup_Background_full.inc"
};

//==============================================================================
// std140 mirrors of the uniform blocks in yup_ForwardPass.vert / .frag and yup_Background.frag

// LightingBlock of yup_ForwardPass.frag and yup_Background.frag. environmentInfo: x intensity
// (0 for none), y last level, z background blur level, w output mode (ForwardPass::Output)
struct ForwardLightingBlock
{
    float ambient[4];
    float lightInfo[4];
    float lightPositions[SceneRenderer::maxLights][4];
    float lightDirections[SceneRenderer::maxLights][4];
    float lightColors[SceneRenderer::maxLights][4];
    float lightSpots[SceneRenderer::maxLights][4];
    float shadowMatrix[16];
    float shadowInfo[4];
    float environmentInfo[4];
    float irradiance[9][4];
};

struct ForwardDrawBlock
{
    float model[16];
    float normalMatrix[16];
    float previousModel[16];
};

struct ForwardMaterialBlock
{
    float baseColorFactor[4];
    float emissiveFactor[4];
    float params[4];
    float flags[4];
    GpuMaterial extension; // specular, extended
};

static_assert (sizeof (ForwardLightingBlock) == 784);
static_assert (sizeof (ForwardDrawBlock) == 192);
static_assert (sizeof (ForwardMaterialBlock) == 96);
static_assert (offsetof (ForwardMaterialBlock, extension) == 64);

float forwardSrgbToLinear (float value) noexcept
{
    return value <= 0.04045f ? value / 12.92f : std::pow ((value + 0.055f) / 1.055f, 2.4f);
}

void copyForwardMatrix (const Matrix4& matrix, float* destination) noexcept
{
    std::copy_n (matrix.getData(), 16, destination);
}

GpuTexture::Ptr createSolidForwardTexture (const GpuDevice::Ptr& device, uint32 rgba)
{
    auto texture = GpuTexture::create (device, GpuTextureDesc (1, 1, GpuTextureFormat::rgba8unorm));
    if (texture == nullptr)
        return nullptr;

    const uint8 texel[] = { static_cast<uint8> (rgba >> 24), static_cast<uint8> (rgba >> 16), static_cast<uint8> (rgba >> 8), static_cast<uint8> (rgba) };

    GpuTextureDataDesc data;
    data.data = texel;
    return texture->upload (data) ? texture : nullptr;
}

size_t getForwardPipelineIndex (const SceneDrawList::DrawItem& item) noexcept
{
    return (item.material->alphaMode == Material::AlphaMode::blend ? 4u : 0u)
         + (item.material->doubleSided ? 2u : 0u)
         + (item.mirrored ? 1u : 0u);
}

ForwardLightingBlock makeForwardLightingBlock (const Scene& scene, Span<const SceneDrawList::Light> lights)
{
    ForwardLightingBlock block {};

    const auto ambient = scene.getAmbientColor();
    block.ambient[0] = forwardSrgbToLinear (ambient.getRedFloat());
    block.ambient[1] = forwardSrgbToLinear (ambient.getGreenFloat());
    block.ambient[2] = forwardSrgbToLinear (ambient.getBlueFloat());

    int count = 0;
    for (const auto& light : lights)
    {
        if (count == SceneRenderer::maxLights)
            break;

        const auto packed = GpuSceneData::packLight (light, -1);
        const auto i = static_cast<size_t> (count++);

        std::copy_n (packed.positionType, 4, block.lightPositions[i]);
        std::copy_n (packed.directionRange, 4, block.lightDirections[i]);
        std::copy_n (packed.colorIntensity, 4, block.lightColors[i]);
        std::copy_n (packed.spotRadiusShadow, 4, block.lightSpots[i]);
    }

    block.lightInfo[0] = static_cast<float> (count);
    return block;
}

ForwardMaterialBlock makeForwardMaterialBlock (const Material& material, bool hasNormalTexture)
{
    ForwardMaterialBlock block {};

    std::copy (material.baseColorFactor.begin(), material.baseColorFactor.end(), block.baseColorFactor);
    std::copy (material.emissiveFactor.begin(), material.emissiveFactor.end(), block.emissiveFactor);
    block.emissiveFactor[3] = material.alphaCutoff;

    block.params[0] = material.metallicFactor;
    block.params[1] = material.roughnessFactor;
    block.params[2] = material.normalScale;
    block.params[3] = material.occlusionStrength;

    block.flags[0] = static_cast<float> (material.alphaMode);
    block.flags[1] = material.baseColorTexture != nullptr && material.baseColorTexture->isSrgb() ? 1.0f : 0.0f;
    block.flags[2] = material.emissiveTexture != nullptr && material.emissiveTexture->isSrgb() ? 1.0f : 0.0f;
    block.flags[3] = hasNormalTexture ? 1.0f : 0.0f;

    block.extension = GpuSceneData::packMaterial (material);

    return block;
}

} // namespace

//==============================================================================
struct ForwardPass::Variant
{
    GpuTextureFormat colorFormat;
    GpuTextureFormat depthFormat;
    uint32_t sampleCount = 1;
    bool velocity = false;

    std::array<GpuPipeline::Ptr, 8> pipelines; // [blend * 4 + doubleSided * 2 + mirrored]
    GpuPipeline::Ptr background;
    String error;

    bool matches (const Targets& targets, uint32_t samples) const noexcept
    {
        return colorFormat == targets.color->getFormat()
            && depthFormat == targets.depth->getFormat()
            && sampleCount == samples
            && velocity == (targets.velocity != nullptr);
    }

    String compile (const GpuDevice::Ptr& device)
    {
        auto bundle = velocity ? ShaderBundle::loadFromData (forwardPassFullShaderBundle, sizeof (forwardPassFullShaderBundle))
                               : ShaderBundle::loadFromData (forwardPassShaderBundle, sizeof (forwardPassShaderBundle));
        if (bundle.failed())
            return "Unable to load the scene shader bundle: " + bundle.getErrorMessage();

        const auto addColorTargets = [this] (GpuPipelineOptions& options, bool blend)
        {
            auto& target = options.colorTargets.emplace_back();
            target.format = colorFormat;
            target.blendEnabled = blend;

            if (velocity)
            {
                auto& velocityTarget = options.colorTargets.emplace_back();
                velocityTarget.format = GpuTextureFormat::rg16float;
                velocityTarget.blendEnabled = false;
            }
        };

        for (size_t index = 0; index < pipelines.size(); ++index)
        {
            const auto blend = (index & 4) != 0;
            const auto doubleSided = (index & 2) != 0;
            const auto mirrored = (index & 1) != 0;

            GpuPipelineOptions options;
            options.vertexBuffers.emplace_back (
                static_cast<uint32_t> (sizeof (Mesh::Vertex)),
                GpuVertexStepMode::vertex,
                std::vector<GpuVertexAttribute> {
                    { GpuVertexFormat::float3, static_cast<uint32_t> (offsetof (Mesh::Vertex, position)), 0 },
                    { GpuVertexFormat::float3, static_cast<uint32_t> (offsetof (Mesh::Vertex, normal)), 1 },
                    { GpuVertexFormat::float2, static_cast<uint32_t> (offsetof (Mesh::Vertex, uv)), 2 },
                    { GpuVertexFormat::float4, static_cast<uint32_t> (offsetof (Mesh::Vertex, color)), 3 },
                    { GpuVertexFormat::float4, static_cast<uint32_t> (offsetof (Mesh::Vertex, tangent)), 4 },
                });
            options.topology = GpuPrimitiveTopology::triangleList;
            options.indexFormat = GpuIndexFormat::uint32;
            options.cullMode = doubleSided ? GpuCullMode::none : GpuCullMode::back;

            // A mirroring transform reverses the winding of the triangles on screen
            options.winding = mirrored ? GpuFaceWinding::clockwise : GpuFaceWinding::counterClockwise;

            addColorTargets (options, blend);

            options.depthStencil.enabled = true;
            options.depthStencil.format = depthFormat;
            options.depthStencil.depthCompare = GpuCompareFunction::lessEqual;
            options.depthStencil.depthWriteEnabled = ! blend;
            options.sampleCount = sampleCount;

            auto result = GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
            if (result.failed())
                return "Unable to compile the scene pipeline: " + result.getErrorMessage();

            pipelines[index] = result.getValue();
        }

        // The environment behind everything: drawn first, without testing or writing depth
        auto backgroundBundle = velocity ? ShaderBundle::loadFromData (backgroundFullShaderBundle, sizeof (backgroundFullShaderBundle))
                                         : ShaderBundle::loadFromData (backgroundShaderBundle, sizeof (backgroundShaderBundle));
        if (backgroundBundle.failed())
            return "Unable to load the background shader bundle: " + backgroundBundle.getErrorMessage();

        GpuPipelineOptions options;
        options.topology = GpuPrimitiveTopology::triangleList;
        options.cullMode = GpuCullMode::none;
        addColorTargets (options, false);
        options.depthStencil.enabled = true;
        options.depthStencil.format = depthFormat;
        options.depthStencil.depthCompare = GpuCompareFunction::always;
        options.depthStencil.depthWriteEnabled = false;
        options.sampleCount = sampleCount;

        auto result = GpuPipeline::compileFromBundle (device, backgroundBundle.getReference(), options);
        if (result.failed())
            return "Unable to compile the background pipeline: " + result.getErrorMessage();

        background = result.getValue();
        return {};
    }
};

//==============================================================================
ForwardPass::ForwardPass() = default;

ForwardPass::~ForwardPass() = default;

//==============================================================================
Result ForwardPass::createDefaults (const GpuDevice::Ptr& device)
{
    whiteTexture = createSolidForwardTexture (device, 0xffffffff);
    blackTexture = createSolidForwardTexture (device, 0x000000ff);
    flatNormalTexture = createSolidForwardTexture (device, 0x8080ffff);
    defaultSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::repeat));
    pointClampSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));

    if (whiteTexture == nullptr || blackTexture == nullptr || flatNormalTexture == nullptr || defaultSampler == nullptr || pointClampSampler == nullptr)
        return Result::fail ("Unable to create the default scene textures");

    return Result::ok();
}

ResultValue<ForwardPass::Variant*> ForwardPass::prepareVariant (const GpuDevice::Ptr& device, const Targets& targets, uint32_t sampleCount)
{
    if (device == nullptr || targets.color == nullptr || targets.depth == nullptr)
        return makeResultValueFail ("No device or no forward targets");

    if (! defaults)
        defaults = createDefaults (device);

    if (defaults->failed())
        return makeResultValueFail (defaults->getErrorMessage());

    auto existing = std::find_if (variants.begin(), variants.end(), [&] (const auto& variant)
    {
        return variant->matches (targets, sampleCount);
    });

    if (existing == variants.end())
    {
        auto variant = std::make_unique<Variant>();
        variant->colorFormat = targets.color->getFormat();
        variant->depthFormat = targets.depth->getFormat();
        variant->sampleCount = sampleCount;
        variant->velocity = targets.velocity != nullptr;
        variant->error = variant->compile (device);

        variants.push_back (std::move (variant));
        existing = std::prev (variants.end());
    }

    if ((*existing)->error.isNotEmpty())
        return makeResultValueFail ((*existing)->error);

    return makeResultValueOk (existing->get());
}

Result ForwardPass::prepare (const GpuDevice::Ptr& device, const Targets& targets, uint32_t sampleCount)
{
    const auto variant = prepareVariant (device, targets, sampleCount);
    return variant.failed() ? Result::fail (variant.getErrorMessage()) : Result::ok();
}

//==============================================================================
bool ForwardPass::render (const GpuDevice::Ptr& device, GpuFrame& frame, const Scene& scene, const ViewData& view,
                          Span<const SceneDrawList::Light> lights, Span<const SceneDrawList::DrawItem> opaque,
                          Span<const SceneDrawList::DrawItem> blend, const ShadowRenderer::Result& shadow,
                          const Targets& targets, Output output, uint32_t sampleCount)
{
    const auto prepared = prepareVariant (device, targets, sampleCount);
    if (prepared.failed())
        return false;

    const auto& variant = *prepared.getValue();

    if (surface == nullptr || surface->asTexture() != targets.color)
        surface = GpuTarget::createFromTexture (device, targets.color);

    if (surface == nullptr)
        return false;

    // The environment, uploaded the first time a device draws it
    GpuTexture::Ptr environmentTexture;
    GpuSampler::Ptr environmentSampler;
    const auto& environment = scene.getEnvironment();

    if (environment != nullptr && scene.getEnvironmentIntensity() > 0.0f)
    {
        environmentTexture = environment->getGpuTexture (device);
        environmentSampler = environment->getGpuSampler (device);
    }

    const auto hasEnvironment = environmentTexture != nullptr && environmentSampler != nullptr;
    const auto lastLevel = hasEnvironment ? static_cast<float> (environment->getNumLevels() - 1) : 0.0f;

    auto lighting = makeForwardLightingBlock (scene, lights);
    copyForwardMatrix (shadow.shadowMatrix, lighting.shadowMatrix);
    std::copy_n (shadow.shadowInfo, 4, lighting.shadowInfo);

    lighting.environmentInfo[0] = hasEnvironment ? scene.getEnvironmentIntensity() : 0.0f;
    lighting.environmentInfo[1] = lastLevel;
    lighting.environmentInfo[2] = scene.getEnvironmentBlur() * lastLevel;
    lighting.environmentInfo[3] = static_cast<float> (output);

    if (hasEnvironment)
    {
        const auto& coefficients = environment->getIrradianceCoefficients();

        for (size_t i = 0; i < coefficients.size(); ++i)
        {
            lighting.irradiance[i][0] = coefficients[i].getX();
            lighting.irradiance[i][1] = coefficients[i].getY();
            lighting.irradiance[i][2] = coefficients[i].getZ();
        }
    }

    // HDR scene color starts transparent: ToneMapping composites the display-referred background under it
    const auto clearColor = output == Output::ldrInline ? GpuColor (scene.getBackgroundColor()) : GpuColor::transparentBlack();

    auto pass = surface->beginRenderPass (frame, { true, clearColor });
    if (! pass.isValid())
        return false;

    if (targets.resolve != nullptr)
        pass.setResolveTarget (0, targets.resolve);

    if (targets.velocity != nullptr)
        pass.setColorAttachment (1, targets.velocity, { true, GpuColor::transparentBlack() });

    pass.setDepthStencilAttachment (targets.depth);

    // Per-frame sets, bound once: every declared resource must be bound, even when unused
    pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));
    pass.setSampler (0, 3, pointClampSampler);
    pass.setUniformBuffer (1, 0, &lighting, sizeof (lighting));
    pass.setTexture (1, 1, hasEnvironment ? environmentTexture : blackTexture);
    pass.setSampler (1, 2, hasEnvironment ? environmentSampler : defaultSampler);
    pass.setTexture (1, 3, shadow.map != nullptr ? shadow.map : whiteTexture);

    GpuPipeline* currentPipeline = nullptr;

    if (hasEnvironment && scene.isEnvironmentVisible())
    {
        pass.setPipeline (variant.background);
        currentPipeline = variant.background.get();

        pass.draw (3);
    }

    const auto bindTexture = [&] (int textureBinding, const Texture::Ptr& texture, const GpuTexture::Ptr& fallback)
    {
        if (texture != nullptr)
        {
            auto gpuTexture = texture->getGpuTexture (device);
            auto gpuSampler = texture->getGpuSampler (device);

            if (gpuTexture != nullptr && gpuSampler != nullptr)
            {
                pass.setTexture (2, textureBinding, gpuTexture);
                pass.setSampler (2, textureBinding + 5, gpuSampler);
                return;
            }
        }

        pass.setTexture (2, textureBinding, fallback);
        pass.setSampler (2, textureBinding + 5, defaultSampler);
    };

    const auto drawItems = [&] (Span<const SceneDrawList::DrawItem> items)
    {
        for (const auto& item : items)
        {
            const auto& material = *item.material;
            const auto& pipeline = variant.pipelines[getForwardPipelineIndex (item)];

            if (pipeline.get() != currentPipeline)
            {
                pass.setPipeline (pipeline);
                currentPipeline = pipeline.get();
            }

            // SceneRenderer resolves MeshNode::previousWorld against its frame stamps before drawing
            ForwardDrawBlock draw {};
            copyForwardMatrix (item.world, draw.model);
            copyForwardMatrix (item.world.inverted().transposed(), draw.normalMatrix);
            copyForwardMatrix (item.node != nullptr ? item.node->previousWorld : item.world, draw.previousModel);

            const auto normalTexture = material.normalTexture != nullptr ? material.normalTexture->getGpuTexture (device) : nullptr;
            const auto materialBlock = makeForwardMaterialBlock (material, normalTexture != nullptr);

            pass.setUniformBuffer (2, 0, &draw, sizeof (draw));
            pass.setUniformBuffer (2, 1, &materialBlock, sizeof (materialBlock));

            bindTexture (2, material.baseColorTexture, whiteTexture);
            bindTexture (3, material.metallicRoughnessTexture, whiteTexture);
            bindTexture (4, material.normalTexture, flatNormalTexture);
            bindTexture (5, material.occlusionTexture, whiteTexture);
            bindTexture (6, material.emissiveTexture, whiteTexture);

            auto vertexBuffer = item.mesh->getVertexBuffer (device, item.primitiveIndex);
            auto indexBuffer = item.mesh->getIndexBuffer (device, item.primitiveIndex);
            if (vertexBuffer == nullptr || indexBuffer == nullptr)
                continue;

            pass.setVertexBuffer (0, vertexBuffer);
            pass.setIndexBuffer (GpuIndexFormat::uint32, indexBuffer);
            pass.drawIndexed (static_cast<uint32_t> (item.primitive->indices.size()));
        }
    };

    drawItems (opaque);
    drawItems (blend);

    return pass.finish();
}

} // namespace yup
