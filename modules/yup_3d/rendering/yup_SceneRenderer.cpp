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

// The scene, shadow and background shaders are precompiled into .ysl bundles embedded in
// yup_SceneRendererShader.inc, yup_SceneShadowShader.inc and yup_SceneBackgroundShader.inc
// (do not edit by hand). Regenerate them after changing their .vert / .frag with (absolute
// paths, the recipe runs in the shader bundler folder):

   for name in SceneRendererShader SceneShadowShader SceneBackgroundShader; do
     just shader_bundler \
        --vert   "$PWD/modules/yup_3d/shaders/yup_$name.vert" \
        --frag   "$PWD/modules/yup_3d/shaders/yup_$name.frag" \
        --output /tmp/yup_$name.ysl \
        --target-langs glsl,essl,hlsl,msl,spirv,wgsl

     # then embed the bundle bytes into the .inc (keep the two-line comment header)
     xxd -i /tmp/yup_$name.ysl \
        | sed -e '1d' -e '/^};/d' -e '/_len =/d' -e '/^[[:space:]]*$/d' \
        > /tmp/yup_$name.inc.body
     { echo "// Generated shader bundle (yup_$name.ysl) - do not edit by hand."; \
       echo '// Regenerate with the command at the top of yup_SceneRenderer.cpp.'; \
       cat /tmp/yup_$name.inc.body; } \
        > modules/yup_3d/shaders/yup_$name.inc
   done

*/

constexpr uint8_t sceneRendererShaderBundle[] = {
#include "../shaders/yup_SceneRendererShader.inc"
};
constexpr uint8_t sceneShadowShaderBundle[] = {
#include "../shaders/yup_SceneShadowShader.inc"
};
constexpr uint8_t sceneBackgroundShaderBundle[] = {
#include "../shaders/yup_SceneBackgroundShader.inc"
};

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
    float shadowMatrix[16];
    float shadowInfo[4];
    float environmentInfo[4];
    float irradiance[9][4];
};

struct SceneShadowData
{
    float shadowMatrix[16];
};

struct SceneBackgroundData
{
    float inverseViewProjection[16];
    float info[4];
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

static_assert (sizeof (SceneFrameData) == 928 && sizeof (SceneFrameData) % 16 == 0);
static_assert (sizeof (SceneShadowData) == 64);
static_assert (sizeof (SceneBackgroundData) == 80);
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
    GpuTexture::Ptr blackTexture;
    GpuTexture::Ptr flatNormalTexture;
    GpuSampler::Ptr defaultSampler;
    GpuPipeline::Ptr shadowPipeline;
    GpuPipeline::Ptr backgroundPipeline;
    GpuTarget::Ptr shadowTarget;
    GpuTexture::Ptr shadowDepthTexture;
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
        blackTexture = createSolidSceneTexture (device, 0x000000ff);
        flatNormalTexture = createSolidSceneTexture (device, 0x8080ffff);
        defaultSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::repeat));

        if (whiteTexture == nullptr || blackTexture == nullptr || flatNormalTexture == nullptr || defaultSampler == nullptr)
            return Result::fail ("Unable to create the default scene textures");

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

        // Depth seen from the shadow casting light, packed into rgba8unorm; the body of a model
        // can mix windings, so nothing is culled
        auto shadowBundle = ShaderBundle::loadFromData (sceneShadowShaderBundle, sizeof (sceneShadowShaderBundle));
        if (shadowBundle.failed())
            return Result::fail ("Unable to load the shadow shader bundle: " + shadowBundle.getErrorMessage());

        GpuPipelineOptions shadowOptions;
        shadowOptions.vertexBuffers.emplace_back (
            static_cast<uint32_t> (sizeof (Mesh::Vertex)),
            GpuVertexStepMode::vertex,
            std::vector<GpuVertexAttribute> { { GpuVertexFormat::float3, static_cast<uint32_t> (offsetof (Mesh::Vertex, position)), 0 } });
        shadowOptions.topology = GpuPrimitiveTopology::triangleList;
        shadowOptions.indexFormat = GpuIndexFormat::uint32;
        shadowOptions.cullMode = GpuCullMode::none;
        auto& shadowTarget = shadowOptions.colorTargets.emplace_back();
        shadowTarget.format = GpuTextureFormat::rgba8unorm;
        shadowTarget.blendEnabled = false; // The packed depth uses all four channels, alpha included
        shadowOptions.depthStencil.enabled = true;
        shadowOptions.depthStencil.format = GpuTextureFormat::depth24plusStencil8;
        shadowOptions.depthStencil.depthCompare = GpuCompareFunction::lessEqual;
        shadowOptions.depthStencil.depthWriteEnabled = true;

        auto shadowResult = GpuPipeline::compileFromBundle (device, shadowBundle.getReference(), shadowOptions);
        if (shadowResult.failed())
            return Result::fail ("Unable to compile the shadow pipeline: " + shadowResult.getErrorMessage());

        shadowPipeline = shadowResult.getValue();

        // The environment behind everything: drawn first, without testing or writing depth
        auto backgroundBundle = ShaderBundle::loadFromData (sceneBackgroundShaderBundle, sizeof (sceneBackgroundShaderBundle));
        if (backgroundBundle.failed())
            return Result::fail ("Unable to load the background shader bundle: " + backgroundBundle.getErrorMessage());

        GpuPipelineOptions backgroundOptions;
        backgroundOptions.topology = GpuPrimitiveTopology::triangleList;
        backgroundOptions.cullMode = GpuCullMode::none;
        auto& backgroundTarget = backgroundOptions.colorTargets.emplace_back();
        backgroundTarget.format = GpuTextureFormat::rgba8unorm;
        backgroundTarget.blendEnabled = false;
        backgroundOptions.depthStencil.enabled = true;
        backgroundOptions.depthStencil.format = GpuTextureFormat::depth24plusStencil8;
        backgroundOptions.depthStencil.depthCompare = GpuCompareFunction::always;
        backgroundOptions.depthStencil.depthWriteEnabled = false;
        backgroundOptions.sampleCount = sampleCount;

        auto backgroundResult = GpuPipeline::compileFromBundle (device, backgroundBundle.getReference(), backgroundOptions);
        if (backgroundResult.failed())
            return Result::fail ("Unable to compile the background pipeline: " + backgroundResult.getErrorMessage());

        backgroundPipeline = backgroundResult.getValue();

        return Result::ok();
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

    bool prepareShadowTarget()
    {
        if (shadowTarget != nullptr && shadowDepthTexture != nullptr)
            return true;

        const auto size = static_cast<uint32_t> (shadowMapSize);

        GpuTextureDesc colorDesc (size, size, GpuTextureFormat::rgba8unorm, true);
        colorDesc.label = "SceneRenderer shadow map";

        GpuTextureDesc depthDesc (size, size, GpuTextureFormat::depth24plusStencil8, true);
        depthDesc.label = "SceneRenderer shadow depth";

        shadowTarget = GpuTarget::create (device, colorDesc);
        shadowDepthTexture = GpuTexture::create (device, depthDesc);

        return shadowTarget != nullptr && shadowDepthTexture != nullptr;
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

    auto frameData = Impl::makeFrameData (scene, drawList.getLights(), view, projection, cameraPosition);

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
    const auto aces = scene.getToneMapping() == Scene::ToneMapping::aces;

    frameData.environmentInfo[0] = hasEnvironment ? scene.getEnvironmentIntensity() : 0.0f;
    frameData.environmentInfo[1] = lastLevel;
    frameData.environmentInfo[2] = aces ? 1.0f : 0.0f;

    if (hasEnvironment)
    {
        const auto& coefficients = environment->getIrradianceCoefficients();

        for (size_t i = 0; i < coefficients.size(); ++i)
        {
            frameData.irradiance[i][0] = coefficients[i].getX();
            frameData.irradiance[i][1] = coefficients[i].getY();
            frameData.irradiance[i][2] = coefficients[i].getZ();
        }
    }

    // The first shadow casting directional light shading the frame, fitted to the opaque items
    int shadowLight = -1;
    Matrix4 shadowMatrix;

    {
        const auto lights = drawList.getLights();
        const auto numLights = jmin (static_cast<int> (lights.size()), maxLights);

        for (int i = 0; i < numLights && shadowLight < 0; ++i)
        {
            const auto& node = *lights[static_cast<size_t> (i)].node;
            if (node.castsShadows && node.type == LightNode::Type::directional)
                shadowLight = i;
        }

        BoundingBox bounds;
        for (const auto& item : drawList.getOpaqueItems())
            bounds.expand (item.primitive->bounds.transformedBy (item.world));

        if (shadowLight >= 0 && ! bounds.isEmpty() && impl->prepareShadowTarget())
        {
            const auto center = bounds.getCenter();
            const auto radius = jmax (0.001f, bounds.getRadius());
            const auto direction = lights[static_cast<size_t> (shadowLight)].direction.normalized();
            const auto up = std::abs (direction.getY()) < 0.99f ? Vector3<float> (0.0f, 1.0f, 0.0f) : Vector3<float> (1.0f, 0.0f, 0.0f);

            // Orthographic around the bounding sphere: the depth from 0 to 1 is linear
            const auto lightView = Matrix4::lookAt (center - direction * (radius * 2.0f), center, up);
            const auto lightProjection = Matrix4::orthographic (-radius, radius, -radius, radius, radius, radius * 3.0f);
            shadowMatrix = lightView.followedBy (lightProjection);

            // A texel covers 2 * radius / size in the world, and 1 / size of the depth range
            const auto texel = 1.0f / static_cast<float> (shadowMapSize);
            copySceneMatrix (shadowMatrix, frameData.shadowMatrix);
            frameData.shadowInfo[1] = texel;
            frameData.shadowInfo[2] = 1.5f * 2.0f * radius * texel;
            frameData.shadowInfo[3] = 1.5f * texel;
        }
        else
        {
            shadowLight = -1;
        }

        frameData.shadowInfo[0] = static_cast<float> (shadowLight);
    }

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return fail ("Unable to begin a GPU frame");

    const auto bindGeometry = [&] (GpuRenderPass& pass, const SceneDrawList::DrawItem& item)
    {
        auto vertexBuffer = item.mesh->getVertexBuffer (device, item.primitiveIndex);
        auto indexBuffer = item.mesh->getIndexBuffer (device, item.primitiveIndex);
        if (vertexBuffer == nullptr || indexBuffer == nullptr)
            return false;

        pass.setVertexBuffer (0, vertexBuffer);
        pass.setIndexBuffer (GpuIndexFormat::uint32, indexBuffer);
        return true;
    };

    if (shadowLight >= 0)
    {
        // Cleared to the far depth, so nothing outside the drawn items shadows
        auto pass = impl->shadowTarget->beginRenderPass (frame, { true, Colors::white });
        if (! pass.isValid())
            return fail ("Unable to begin the shadow pass");

        pass.setDepthStencilAttachment (impl->shadowDepthTexture);
        pass.setPipeline (impl->shadowPipeline);

        SceneShadowData shadowData {};
        copySceneMatrix (shadowMatrix, shadowData.shadowMatrix);

        for (const auto& item : drawList.getOpaqueItems())
        {
            SceneDrawData drawData {};
            copySceneMatrix (item.world, drawData.model);

            pass.setUniformBuffer (0, 0, &shadowData, sizeof (shadowData));
            pass.setUniformBuffer (0, 1, &drawData, sizeof (drawData));

            if (bindGeometry (pass, item))
                pass.drawIndexed (static_cast<uint32_t> (item.primitive->indices.size()));
        }

        pass.finish();
    }

    {
        auto& surface = impl->multisampledTarget != nullptr ? impl->multisampledTarget : impl->colorTarget;

        auto pass = surface->beginRenderPass (frame, { true, scene.getBackgroundColor() });
        if (! pass.isValid())
            return fail ("Unable to begin the render pass");

        if (impl->multisampledTarget != nullptr)
            pass.setResolveTarget (0, impl->colorTarget->asTexture());

        pass.setDepthStencilAttachment (impl->depthTexture);

        GpuPipeline* currentPipeline = nullptr;

        if (hasEnvironment && scene.isEnvironmentVisible())
        {
            SceneBackgroundData backgroundData {};
            copySceneMatrix (view.followedBy (projection).inverted(), backgroundData.inverseViewProjection);
            backgroundData.info[0] = scene.getEnvironmentIntensity();
            backgroundData.info[1] = scene.getEnvironmentBlur() * lastLevel;
            backgroundData.info[2] = aces ? 1.0f : 0.0f;
            backgroundData.info[3] = scene.getExposure();

            pass.setPipeline (impl->backgroundPipeline);
            currentPipeline = impl->backgroundPipeline.get();

            pass.setUniformBuffer (0, 0, &backgroundData, sizeof (backgroundData));
            pass.setTexture (0, 1, environmentTexture);
            pass.setSampler (0, 2, environmentSampler);
            pass.draw (3);
        }

        const auto shadowTexture = shadowLight >= 0 ? impl->shadowTarget->asTexture() : impl->whiteTexture;

        const auto drawItems = [&] (Span<const SceneDrawList::DrawItem> items)
        {
            for (const auto& item : items)
            {
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

                // Bound even when unused: every declared resource must be. The shadow map is
                // read with texelFetch, so it needs no sampler of its own
                pass.setTexture (0, 13, hasEnvironment ? environmentTexture : impl->blackTexture);
                pass.setTexture (0, 14, shadowTexture);
                pass.setSampler (0, 15, hasEnvironment ? environmentSampler : impl->defaultSampler);

                if (bindGeometry (pass, item))
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
