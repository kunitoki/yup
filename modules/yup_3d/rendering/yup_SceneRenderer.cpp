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
/*

// The scene shader is precompiled into a .ysl bundle embedded in
// yup_SceneRendererShader.inc (do not edit by hand). Regenerate it after
// changing yup_SceneRendererShader.vert / .frag with (absolute paths, the
// recipe runs in the shader bundler folder):

    just shader_bundler \
       --vert   "$PWD/modules/yup_3d/rendering/yup_SceneRendererShader.vert" \
       --frag   "$PWD/modules/yup_3d/rendering/yup_SceneRendererShader.frag" \
       --output /tmp/yup_SceneRendererShader.ysl \
       --target-langs glsl,essl,hlsl,msl,wgsl

// then embed the bundle bytes into the .inc (keep the two-line comment header):

   xxd -i /tmp/yup_SceneRendererShader.ysl \
       | sed -e '1d' -e '/^};/d' -e '/_len =/d' -e '/^[[:space:]]*$/d' \
       > /tmp/yup_SceneRendererShader.inc.body
   { echo '// Generated shader bundle (yup_SceneRendererShader.ysl) - do not edit by hand.'; \
     echo '// Regenerate with the command at the top of yup_SceneRenderer.cpp.'; \
     cat /tmp/yup_SceneRendererShader.inc.body; } \
       > modules/yup_3d/rendering/yup_SceneRendererShader.inc

*/

#if __has_include("yup_SceneRendererShader.inc")
// Embedded precompiled shader bundle (.ysl), consumed by ShaderBundle::loadFromData().
constexpr uint8_t sceneRendererShaderBundle[] = {
#include "yup_SceneRendererShader.inc"
};
#define YUP_3D_SCENE_SHADER_AVAILABLE 1
#else
#define YUP_3D_SCENE_SHADER_AVAILABLE 0
#endif

//==============================================================================
// std140 mirrors of the uniform blocks in yup_SceneRendererShader.vert / .frag

struct SceneFrameData
{
    float view[16];
    float projection[16];
    float cameraPosition[4];
    float ambient[4];
    float lightInfo[4];
    float lightPositions[SceneRenderer::maxLights][4];
    float lightDirections[SceneRenderer::maxLights][4];
    float lightColors[SceneRenderer::maxLights][4];
    float lightSpots[SceneRenderer::maxLights][4];
};

struct SceneDrawData
{
    float model[16];
    float normalMatrix[16];
};

struct SceneMaterialData
{
    float baseColorFactor[4];
    float emissiveFactor[4];
    float params[4];
    float flags[4];
};

static_assert (sizeof (SceneFrameData) == 688 && sizeof (SceneFrameData) % 16 == 0);
static_assert (sizeof (SceneDrawData) == 128);
static_assert (sizeof (SceneMaterialData) == 64);

float sceneSrgbToLinear (float value) noexcept
{
    return value <= 0.04045f ? value / 12.92f : std::pow ((value + 0.055f) / 1.055f, 2.4f);
}

void copySceneMatrix (const Matrix4& matrix, float* destination) noexcept
{
    std::copy_n (matrix.getData(), 16, destination);
}

GpuTexture::Ptr createSolidSceneTexture (const GpuDevice::Ptr& device, uint32 rgba)
{
    auto texture = GpuTexture::create (device, GpuTextureDesc (1, 1, GpuTextureFormat::rgba8unorm));
    if (texture == nullptr)
        return nullptr;

    const uint8 texel[] = { static_cast<uint8> (rgba >> 24), static_cast<uint8> (rgba >> 16), static_cast<uint8> (rgba >> 8), static_cast<uint8> (rgba) };

    GpuTextureDataDesc data;
    data.data = texel;
    return texture->upload (data) ? texture : nullptr;
}

} // namespace

//==============================================================================

struct SceneRenderer::Impl
{
    GpuDevice::Ptr device;
    std::array<GpuPipeline::Ptr, 8> pipelines; // [blend * 4 + doubleSided * 2 + mirrored]
    uint32_t sampleCount = 1;
    GpuTarget::Ptr colorTarget;
    GpuTarget::Ptr multisampledTarget;
    GpuTexture::Ptr depthTexture;
    GpuTexture::Ptr whiteTexture;
    GpuTexture::Ptr flatNormalTexture;
    GpuSampler::Ptr defaultSampler;
    SceneDrawList drawList;
    std::optional<Result> preparation;

    static size_t getPipelineIndex (const SceneDrawList::DrawItem& item) noexcept
    {
        return (item.material->alphaMode == Material::AlphaMode::blend ? 4u : 0u)
             + (item.material->doubleSided ? 2u : 0u)
             + (item.mirrored ? 1u : 0u);
    }

    /** Creates the pipelines and default textures once, remembering a failure instead of retrying every frame. */
    Result prepareResources()
    {
        if (! preparation)
            preparation = createResources();

        return *preparation;
    }

    Result createResources()
    {
        whiteTexture = createSolidSceneTexture (device, 0xffffffff);
        flatNormalTexture = createSolidSceneTexture (device, 0x8080ffff);
        defaultSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::repeat));

        if (whiteTexture == nullptr || flatNormalTexture == nullptr || defaultSampler == nullptr)
            return Result::fail ("Unable to create the default scene textures");

#if YUP_3D_SCENE_SHADER_AVAILABLE
        auto bundle = ShaderBundle::loadFromData (sceneRendererShaderBundle, sizeof (sceneRendererShaderBundle));
        if (bundle.failed())
            return Result::fail ("Unable to load the scene shader bundle: " + bundle.getErrorMessage());

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
                });
            options.topology = GpuPrimitiveTopology::triangleList;
            options.indexFormat = GpuIndexFormat::uint32;
            options.cullMode = doubleSided ? GpuCullMode::none : GpuCullMode::back;

            // A mirroring transform reverses the winding of the triangles on screen
            options.winding = mirrored ? GpuFaceWinding::clockwise : GpuFaceWinding::counterClockwise;

            auto& target = options.colorTargets.emplace_back();
            target.format = GpuTextureFormat::rgba8unorm;
            target.blendEnabled = blend;

            options.depthStencil.enabled = true;
            options.depthStencil.format = GpuTextureFormat::depth24plusStencil8;
            options.depthStencil.depthCompare = GpuCompareFunction::lessEqual;
            options.depthStencil.depthWriteEnabled = ! blend;
            options.sampleCount = sampleCount;

            auto result = GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
            if (result.failed())
                return Result::fail ("Unable to compile the scene pipeline: " + result.getErrorMessage());

            pipelines[index] = result.getValue();
        }

        return Result::ok();
#else
        return Result::fail ("The yup_3d scene shader has not been cooked: see the top of yup_SceneRenderer.cpp");
#endif
    }

    bool prepareTargets (int width, int height)
    {
        if (colorTarget != nullptr && colorTarget->getWidth() == width && colorTarget->getHeight() == height)
            return true;

        const auto w = static_cast<uint32_t> (width);
        const auto h = static_cast<uint32_t> (height);

        // With MSAA the pass draws into multisampled color and depth, resolved into colorTarget
        GpuTextureDesc depthDesc (w, h, GpuTextureFormat::depth24plusStencil8, true);
        depthDesc.sampleCount = sampleCount;
        depthDesc.label = "SceneRenderer depth";

        colorTarget = GpuTarget::create (device, width, height);
        depthTexture = GpuTexture::create (device, depthDesc);
        multisampledTarget = nullptr;

        if (sampleCount > 1)
        {
            GpuTextureDesc colorDesc (w, h, GpuTextureFormat::rgba8unorm, true);
            colorDesc.sampleCount = sampleCount;
            colorDesc.label = "SceneRenderer multisampled color";

            multisampledTarget = GpuTarget::create (device, colorDesc);
            if (multisampledTarget == nullptr)
                return false;
        }

        return colorTarget != nullptr && depthTexture != nullptr;
    }

    void bindTexture (GpuRenderPass& pass, int textureBinding, const Texture::Ptr& texture, const GpuTexture::Ptr& fallback)
    {
        if (texture != nullptr)
        {
            auto gpuTexture = texture->getGpuTexture (device);
            auto gpuSampler = texture->getGpuSampler (device);

            if (gpuTexture != nullptr && gpuSampler != nullptr)
            {
                pass.setTexture (0, textureBinding, gpuTexture);
                pass.setSampler (0, textureBinding + 5, gpuSampler);
                return;
            }
        }

        pass.setTexture (0, textureBinding, fallback);
        pass.setSampler (0, textureBinding + 5, defaultSampler);
    }

    static SceneFrameData makeFrameData (const Scene& scene, Span<const SceneDrawList::Light> lights, const Matrix4& view, const Matrix4& projection, const Vector3<float>& cameraPosition)
    {
        SceneFrameData frameData {};

        copySceneMatrix (view, frameData.view);
        copySceneMatrix (projection, frameData.projection);

        frameData.cameraPosition[0] = cameraPosition.getX();
        frameData.cameraPosition[1] = cameraPosition.getY();
        frameData.cameraPosition[2] = cameraPosition.getZ();

        const auto ambient = scene.getAmbientColor();
        frameData.ambient[0] = sceneSrgbToLinear (ambient.getRedFloat());
        frameData.ambient[1] = sceneSrgbToLinear (ambient.getGreenFloat());
        frameData.ambient[2] = sceneSrgbToLinear (ambient.getBlueFloat());
        frameData.ambient[3] = scene.getExposure();

        int count = 0;
        for (const auto& light : lights)
        {
            if (count == maxLights)
                break;

            const auto& node = *light.node;
            const auto i = static_cast<size_t> (count++);

            frameData.lightPositions[i][0] = light.position.getX();
            frameData.lightPositions[i][1] = light.position.getY();
            frameData.lightPositions[i][2] = light.position.getZ();
            frameData.lightPositions[i][3] = static_cast<float> (node.type);

            frameData.lightDirections[i][0] = light.direction.getX();
            frameData.lightDirections[i][1] = light.direction.getY();
            frameData.lightDirections[i][2] = light.direction.getZ();
            frameData.lightDirections[i][3] = node.range;

            frameData.lightColors[i][0] = node.color[0];
            frameData.lightColors[i][1] = node.color[1];
            frameData.lightColors[i][2] = node.color[2];
            frameData.lightColors[i][3] = node.intensity;

            // KHR_lights_punctual cone falloff: clamp (cos * scale + offset)
            const auto cosOuter = std::cos (node.outerConeAngle);
            const auto cosInner = std::cos (node.innerConeAngle);
            const auto scale = 1.0f / jmax (0.001f, cosInner - cosOuter);
            frameData.lightSpots[i][0] = scale;
            frameData.lightSpots[i][1] = -cosOuter * scale;
        }

        if (count == 0 && scene.isUsingDefaultLight())
        {
            const auto direction = Vector3<float> (-0.35f, -0.75f, -0.55f).normalized();

            frameData.lightDirections[0][0] = direction.getX();
            frameData.lightDirections[0][1] = direction.getY();
            frameData.lightDirections[0][2] = direction.getZ();
            frameData.lightColors[0][0] = frameData.lightColors[0][1] = frameData.lightColors[0][2] = 1.0f;
            frameData.lightColors[0][3] = 3.0f;
            count = 1;
        }

        frameData.lightInfo[0] = static_cast<float> (count);
        return frameData;
    }

    static SceneMaterialData makeMaterialData (const Material& material, bool hasNormalTexture)
    {
        SceneMaterialData data {};

        std::copy (material.baseColorFactor.begin(), material.baseColorFactor.end(), data.baseColorFactor);
        std::copy (material.emissiveFactor.begin(), material.emissiveFactor.end(), data.emissiveFactor);
        data.emissiveFactor[3] = material.alphaCutoff;

        data.params[0] = material.metallicFactor;
        data.params[1] = material.roughnessFactor;
        data.params[2] = material.normalScale;
        data.params[3] = material.occlusionStrength;

        data.flags[0] = static_cast<float> (material.alphaMode);
        data.flags[1] = material.baseColorTexture != nullptr && material.baseColorTexture->isSrgb() ? 1.0f : 0.0f;
        data.flags[2] = material.emissiveTexture != nullptr && material.emissiveTexture->isSrgb() ? 1.0f : 0.0f;
        data.flags[3] = hasNormalTexture ? 1.0f : 0.0f;

        return data;
    }
};

//==============================================================================

SceneRenderer::SceneRenderer()
    : impl (std::make_unique<Impl>())
{
}

SceneRenderer::~SceneRenderer() = default;

//==============================================================================

GpuTexture::Ptr SceneRenderer::render (const GpuDevice::Ptr& device, Scene& scene, int widthPx, int heightPx)
{
    const auto fail = [this] (const String& error) -> GpuTexture::Ptr
    {
        lastError = error;
        return nullptr;
    };

    if (device == nullptr || widthPx <= 0 || heightPx <= 0)
        return fail ("No device, or an empty size");

    // The largest power of two within both the request and what the device supports
    uint32_t samples = 1;
    while (samples * 2 <= static_cast<uint32_t> (sampleCount) && samples * 2 <= device->getMaximumSampleCount())
        samples *= 2;

    if (device != impl->device || samples != impl->sampleCount)
    {
        impl = std::make_unique<Impl>();
        impl->device = device;
        impl->sampleCount = samples;
    }

    if (auto result = impl->prepareResources(); result.failed())
        return fail (result.getErrorMessage());

    if (! impl->prepareTargets (widthPx, heightPx))
        return fail ("Unable to create the render targets");

    // Camera: the active one, or a default perspective camera at (0, 0, 5) looking at the origin
    const auto aspect = static_cast<float> (widthPx) / static_cast<float> (heightPx);
    Matrix4 view;
    Matrix4 projection;
    Vector3<float> cameraPosition;

    if (auto* camera = scene.getActiveCamera())
    {
        const auto cameraWorld = camera->getEntity() != nullptr ? camera->getEntity()->getWorldMatrix() : Matrix4();
        view = cameraWorld.inverted();
        projection = camera->getProjectionMatrix (aspect);
        cameraPosition = cameraWorld.transformPoint ({});
    }
    else
    {
        cameraPosition = { 0.0f, 0.0f, 5.0f };
        view = Matrix4::lookAt (cameraPosition, {}, { 0.0f, 1.0f, 0.0f });
        projection = CameraNode().getProjectionMatrix (aspect);
    }

    auto& drawList = impl->drawList;
    drawList.build (*scene.getRoot(), view);

    const auto frameData = Impl::makeFrameData (scene, drawList.getLights(), view, projection, cameraPosition);

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return fail ("Unable to begin a GPU frame");

    {
        auto& surface = impl->multisampledTarget != nullptr ? impl->multisampledTarget : impl->colorTarget;

        auto pass = surface->beginRenderPass (frame, { true, scene.getBackgroundColor() });
        if (! pass.isValid())
            return fail ("Unable to begin the render pass");

        if (impl->multisampledTarget != nullptr)
            pass.setResolveTarget (0, impl->colorTarget->asTexture());

        pass.setDepthStencilAttachment (impl->depthTexture);

        GpuPipeline* currentPipeline = nullptr;

        const auto drawItems = [&] (Span<const SceneDrawList::DrawItem> items)
        {
            for (const auto& item : items)
            {
                auto vertexBuffer = item.mesh->getVertexBuffer (device, item.primitiveIndex);
                auto indexBuffer = item.mesh->getIndexBuffer (device, item.primitiveIndex);
                if (vertexBuffer == nullptr || indexBuffer == nullptr)
                    continue;

                const auto& material = *item.material;
                const auto& pipeline = impl->pipelines[Impl::getPipelineIndex (item)];

                if (pipeline.get() != currentPipeline)
                {
                    pass.setPipeline (pipeline);
                    currentPipeline = pipeline.get();
                }

                SceneDrawData drawData {};
                copySceneMatrix (item.world, drawData.model);
                copySceneMatrix (item.world.inverted().transposed(), drawData.normalMatrix);

                const auto normalTexture = material.normalTexture != nullptr ? material.normalTexture->getGpuTexture (device) : nullptr;
                const auto materialData = Impl::makeMaterialData (material, normalTexture != nullptr);

                pass.setUniformBuffer (0, 0, &frameData, sizeof (frameData));
                pass.setUniformBuffer (0, 1, &drawData, sizeof (drawData));
                pass.setUniformBuffer (0, 2, &materialData, sizeof (materialData));

                impl->bindTexture (pass, 3, material.baseColorTexture, impl->whiteTexture);
                impl->bindTexture (pass, 4, material.metallicRoughnessTexture, impl->whiteTexture);
                impl->bindTexture (pass, 5, material.normalTexture, impl->flatNormalTexture);
                impl->bindTexture (pass, 6, material.occlusionTexture, impl->whiteTexture);
                impl->bindTexture (pass, 7, material.emissiveTexture, impl->whiteTexture);

                pass.setVertexBuffer (0, vertexBuffer);
                pass.setIndexBuffer (GpuIndexFormat::uint32, indexBuffer);
                pass.drawIndexed (static_cast<uint32_t> (item.primitive->indices.size()));
            }
        };

        drawItems (drawList.getOpaqueItems());
        drawItems (drawList.getBlendItems());

        pass.finish();
    }

    frame.submit();

    lastError = {};
    return impl->colorTarget->asTexture();
}

} // namespace yup
