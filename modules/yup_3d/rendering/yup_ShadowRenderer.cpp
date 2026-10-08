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

// Regenerate after changing shaders/yup_ShadowPass.vert / .frag with: just yup3d_shaders
constexpr uint8_t shadowPassShaderBundle[] = {
#include "../shaders/yup_ShadowPass.inc"
};

// std140 mirrors of the uniform blocks in yup_ShadowPass.vert
struct ShadowPassData
{
    float shadowMatrix[16];
};

struct ShadowPassDrawData
{
    float model[16];
    float normalMatrix[16];
};

static_assert (sizeof (ShadowPassData) == 64);
static_assert (sizeof (ShadowPassDrawData) == 128);

} // namespace

//==============================================================================
ShadowRenderer::ShadowRenderer() = default;

ShadowRenderer::~ShadowRenderer() = default;

//==============================================================================
yup::Result ShadowRenderer::prepare (const GpuDevice::Ptr& device)
{
    if (preparation)
        return *preparation;

    preparation = yup::Result::ok();

    // Depth seen from the shadow casting light, packed into rgba8unorm; the body of a model
    // can mix windings, so nothing is culled
    auto bundle = ShaderBundle::loadFromData (shadowPassShaderBundle, sizeof (shadowPassShaderBundle));
    if (bundle.failed())
        return *(preparation = yup::Result::fail ("Unable to load the shadow shader bundle: " + bundle.getErrorMessage()));

    GpuPipelineOptions options;
    options.vertexBuffers.emplace_back (
        static_cast<uint32_t> (sizeof (Mesh::Vertex)),
        GpuVertexStepMode::vertex,
        std::vector<GpuVertexAttribute> { { GpuVertexFormat::float3, static_cast<uint32_t> (offsetof (Mesh::Vertex, position)), 0 } });
    options.topology = GpuPrimitiveTopology::triangleList;
    options.indexFormat = GpuIndexFormat::uint32;
    options.cullMode = GpuCullMode::none;
    auto& colorTarget = options.colorTargets.emplace_back();
    colorTarget.format = GpuTextureFormat::rgba8unorm;
    colorTarget.blendEnabled = false; // The packed depth uses all four channels, alpha included
    options.depthStencil.enabled = true;
    options.depthStencil.format = GpuTextureFormat::depth24plusStencil8;
    options.depthStencil.depthCompare = GpuCompareFunction::lessEqual;
    options.depthStencil.depthWriteEnabled = true;

    auto compiled = GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
    if (compiled.failed())
        return *(preparation = yup::Result::fail ("Unable to compile the shadow pipeline: " + compiled.getErrorMessage()));

    pipeline = compiled.getValue();
    return *preparation;
}

bool ShadowRenderer::prepareTarget (const GpuDevice::Ptr& device)
{
    if (target != nullptr && depthTexture != nullptr)
        return true;

    const auto size = static_cast<uint32_t> (SceneRenderer::shadowMapSize);

    GpuTextureDesc colorDesc (size, size, GpuTextureFormat::rgba8unorm, true);
    colorDesc.label = "SceneRenderer shadow map";

    GpuTextureDesc depthDesc (size, size, GpuTextureFormat::depth24plusStencil8, true);
    depthDesc.label = "SceneRenderer shadow depth";

    target = GpuTarget::create (device, colorDesc);
    depthTexture = GpuTexture::create (device, depthDesc);

    return target != nullptr && depthTexture != nullptr;
}

//==============================================================================
ShadowRenderer::Result ShadowRenderer::render (const GpuDevice::Ptr& device, GpuFrame& frame, const SceneDrawList& drawList)
{
    Result result;

    if (pipeline == nullptr)
        return result;

    // The first shadow casting directional light shading the frame, fitted to the opaque items
    const auto lights = drawList.getLights();
    const auto numLights = jmin (static_cast<int> (lights.size()), SceneRenderer::maxLights);

    int shadowLight = -1;
    for (int i = 0; i < numLights && shadowLight < 0; ++i)
    {
        const auto& node = *lights[static_cast<size_t> (i)].node;
        if (node.castsShadows && node.type == LightNode::Type::directional)
            shadowLight = i;
    }

    BoundingBox bounds;
    for (const auto& item : drawList.getOpaqueItems())
        bounds.expand (item.primitive->bounds.transformedBy (item.world));

    if (shadowLight < 0 || bounds.isEmpty() || ! prepareTarget (device))
        return result;

    const auto center = bounds.getCenter();
    const auto radius = jmax (0.001f, bounds.getRadius());
    const auto direction = lights[static_cast<size_t> (shadowLight)].direction.normalized();
    const auto up = std::abs (direction.getY()) < 0.99f ? Vector3<float> (0.0f, 1.0f, 0.0f) : Vector3<float> (1.0f, 0.0f, 0.0f);

    // Orthographic around the bounding sphere: the depth from 0 to 1 is linear
    const auto lightView = Matrix4::lookAt (center - direction * (radius * 2.0f), center, up);
    const auto lightProjection = Matrix4::orthographic (-radius, radius, -radius, radius, radius, radius * 3.0f);
    const auto shadowMatrix = lightView.followedBy (lightProjection);

    // Cleared to the far depth, so nothing outside the drawn items shadows
    auto pass = target->beginRenderPass (frame, { true, Colors::white });
    if (! pass.isValid())
    {
        result.error = "Unable to begin the shadow pass";
        return result;
    }

    pass.setDepthStencilAttachment (depthTexture);
    pass.setPipeline (pipeline);

    ShadowPassData shadowData {};
    std::copy_n (shadowMatrix.getData(), 16, shadowData.shadowMatrix);
    pass.setUniformBuffer (0, 0, &shadowData, sizeof (shadowData));

    for (const auto& item : drawList.getOpaqueItems())
    {
        auto vertexBuffer = item.mesh->getVertexBuffer (device, item.primitiveIndex);
        auto indexBuffer = item.mesh->getIndexBuffer (device, item.primitiveIndex);
        if (vertexBuffer == nullptr || indexBuffer == nullptr)
            continue;

        ShadowPassDrawData drawData {};
        std::copy_n (item.world.getData(), 16, drawData.model);
        pass.setUniformBuffer (0, 1, &drawData, sizeof (drawData));

        pass.setVertexBuffer (0, vertexBuffer);
        pass.setIndexBuffer (GpuIndexFormat::uint32, indexBuffer);
        pass.drawIndexed (static_cast<uint32_t> (item.primitive->indices.size()));
    }

    pass.finish();

    // A texel covers 2 * radius / size in the world, and 1 / size of the depth range
    const auto texel = 1.0f / static_cast<float> (SceneRenderer::shadowMapSize);

    result.map = target->asTexture();
    result.shadowMatrix = shadowMatrix;
    result.shadowInfo[0] = static_cast<float> (shadowLight);
    result.shadowInfo[1] = texel;
    result.shadowInfo[2] = 1.5f * 2.0f * radius * texel;
    result.shadowInfo[3] = 1.5f * texel;
    return result;
}

} // namespace yup
