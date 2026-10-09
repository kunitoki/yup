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

static_assert (sizeof (GeometryBindings::DrawBlock) == 208);
static_assert (offsetof (GeometryBindings::DrawBlock, normalMatrix) == 64);
static_assert (offsetof (GeometryBindings::DrawBlock, previousModel) == 128);
static_assert (offsetof (GeometryBindings::DrawBlock, ids) == 192);
static_assert (sizeof (GeometryBindings::MaterialBlock) == 96);
static_assert (offsetof (GeometryBindings::MaterialBlock, emissiveFactor) == 16);
static_assert (offsetof (GeometryBindings::MaterialBlock, params) == 32);
static_assert (offsetof (GeometryBindings::MaterialBlock, flags) == 48);
static_assert (offsetof (GeometryBindings::MaterialBlock, extension) == 64);

void copyGeometryMatrix (const Matrix4& matrix, float* destination) noexcept
{
    std::copy_n (matrix.getData(), 16, destination);
}

} // namespace

//==============================================================================
size_t GeometryBindings::getRasterState (const SceneDrawList::DrawItem& item) noexcept
{
    return (item.material->doubleSided ? 2u : 0u) + (item.mirrored ? 1u : 0u);
}

void GeometryBindings::setPipelineGeometry (GpuPipelineOptions& options, size_t rasterState)
{
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
    options.cullMode = (rasterState & 2) != 0 ? GpuCullMode::none : GpuCullMode::back;

    // A mirroring transform reverses the winding of the triangles on screen
    options.winding = (rasterState & 1) != 0 ? GpuFaceWinding::clockwise : GpuFaceWinding::counterClockwise;
}

GeometryBindings::DrawBlock GeometryBindings::makeDrawBlock (const SceneDrawList::DrawItem& item, int materialId) noexcept
{
    // SceneRenderer resolves MeshNode::previousWorld against its frame stamps before drawing
    DrawBlock block {};
    copyGeometryMatrix (item.world, block.model);
    copyGeometryMatrix (item.world.inverted().transposed(), block.normalMatrix);
    copyGeometryMatrix (item.node != nullptr ? item.node->previousWorld : item.world, block.previousModel);
    block.ids[0] = static_cast<float> (materialId);
    return block;
}

GeometryBindings::MaterialBlock GeometryBindings::makeMaterialBlock (const Material& material, bool hasNormalTexture) noexcept
{
    MaterialBlock block {};

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

GpuTexture::Ptr GeometryBindings::createSolidTexture (const GpuDevice::Ptr& device, uint32 rgba)
{
    auto texture = GpuTexture::create (device, GpuTextureDesc (1, 1, GpuTextureFormat::rgba8unorm));
    if (texture == nullptr)
        return nullptr;

    const uint8 texel[] = { static_cast<uint8> (rgba >> 24), static_cast<uint8> (rgba >> 16), static_cast<uint8> (rgba >> 8), static_cast<uint8> (rgba) };

    GpuTextureDataDesc data;
    data.data = texel;
    return texture->upload (data) ? texture : nullptr;
}

//==============================================================================
GeometryBindings::GeometryBindings() = default;

GeometryBindings::~GeometryBindings() = default;

Result GeometryBindings::prepare (const GpuDevice::Ptr& device)
{
    whiteTexture = createSolidTexture (device, 0xffffffff);
    flatNormalTexture = createSolidTexture (device, 0x8080ffff);
    defaultSampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::linear, GpuWrapMode::repeat));

    if (whiteTexture == nullptr || flatNormalTexture == nullptr || defaultSampler == nullptr)
        return Result::fail ("Unable to create the default material textures");

    return Result::ok();
}

bool GeometryBindings::draw (GpuRenderPass& pass, const GpuDevice::Ptr& device, const SceneDrawList::DrawItem& item, int materialId) const
{
    const auto& material = *item.material;

    // Returns true when the material's own texture is bound, false for the fallback
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
                return true;
            }
        }

        pass.setTexture (2, textureBinding, fallback);
        pass.setSampler (2, textureBinding + 5, defaultSampler);
        return false;
    };

    bindTexture (2, material.baseColorTexture, whiteTexture);
    bindTexture (3, material.metallicRoughnessTexture, whiteTexture);
    const auto hasNormalMap = bindTexture (4, material.normalTexture, flatNormalTexture);
    bindTexture (5, material.occlusionTexture, whiteTexture);
    bindTexture (6, material.emissiveTexture, whiteTexture);

    const auto drawBlock = makeDrawBlock (item, materialId);
    const auto materialBlock = makeMaterialBlock (material, hasNormalMap);

    pass.setUniformBuffer (2, 0, &drawBlock, sizeof (drawBlock));
    pass.setUniformBuffer (2, 1, &materialBlock, sizeof (materialBlock));

    auto vertexBuffer = item.mesh->getVertexBuffer (device, item.primitiveIndex);
    auto indexBuffer = item.mesh->getIndexBuffer (device, item.primitiveIndex);
    if (vertexBuffer == nullptr || indexBuffer == nullptr)
        return false;

    pass.setVertexBuffer (0, vertexBuffer);
    pass.setIndexBuffer (GpuIndexFormat::uint32, indexBuffer);
    pass.drawIndexed (static_cast<uint32_t> (item.primitive->indices.size()));
    return true;
}

} // namespace yup
