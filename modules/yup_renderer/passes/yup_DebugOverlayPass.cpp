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
// Shader bundles built with the module, see shaderBundles in yup_renderer.h

constexpr uint8_t debugOverlayShaderBundle[] = {
#include "yup_renderer_shaders/yup_DebugOverlay.inc"
};

constexpr uint8_t debugOverlayBoxesShaderBundle[] = {
#include "yup_renderer_shaders/yup_DebugOverlay_boxes.inc"
};

// std140 mirror of LinesBlock in yup_DebugOverlay.vert
struct DebugOverlayLinesBlock
{
    float model[16];
    float color[4];
};

DebugOverlayLinesBlock makeDebugOverlayLinesBlock (const Matrix4& model, Color color) noexcept
{
    DebugOverlayLinesBlock block {};
    std::copy_n (model.getData(), 16, block.model);
    block.color[0] = color.getRedFloat();
    block.color[1] = color.getGreenFloat();
    block.color[2] = color.getBlueFloat();
    block.color[3] = color.getAlphaFloat();
    return block;
}

} // namespace

//==============================================================================
bool DebugOverlayPass::Lines::isEmpty() const noexcept
{
    return wireframe == nullptr && boxes.empty();
}

//==============================================================================
DebugOverlayPass::DebugOverlayPass (GpuDevice::Ptr deviceToUse)
    : device (std::move (deviceToUse))
{
}

DebugOverlayPass::~DebugOverlayPass() = default;

//==============================================================================
Result DebugOverlayPass::prepare (GpuTextureFormat format)
{
    if (prepared)
        return *prepared;

    prepared = [&]
    {
        if (device == nullptr)
            return Result::fail ("No device");

        const auto compile = [&] (const uint8_t* data, size_t size, bool boxes) -> ResultValue<GpuPipeline::Ptr>
        {
            auto bundle = ShaderBundle::loadFromData (data, size);
            if (bundle.failed())
                return makeResultValueFail ("Unable to load the debug overlay shader bundle: " + bundle.getErrorMessage());

            GpuPipelineOptions options;
            options.topology = GpuPrimitiveTopology::lineList;
            options.cullMode = GpuCullMode::none;
            options.colorTargets.emplace_back().format = format;
            options.dynamicUniformBuffers = { { 1, 0 } };

            if (! boxes)
            {
                options.vertexBuffers.emplace_back (
                    static_cast<uint32_t> (sizeof (Mesh::Vertex)),
                    GpuVertexStepMode::vertex,
                    std::vector<GpuVertexAttribute> { { GpuVertexFormat::float3, static_cast<uint32_t> (offsetof (Mesh::Vertex, position)), 0 } });
                options.indexFormat = GpuIndexFormat::uint32;
            }

            return GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
        };

        auto wireframe = compile (debugOverlayShaderBundle, sizeof (debugOverlayShaderBundle), false);
        if (wireframe.failed())
            return Result::fail ("Unable to compile the wireframe pipeline: " + wireframe.getErrorMessage());

        auto boxes = compile (debugOverlayBoxesShaderBundle, sizeof (debugOverlayBoxesShaderBundle), true);
        if (boxes.failed())
            return Result::fail ("Unable to compile the bounding box pipeline: " + boxes.getErrorMessage());

        wireframePipeline = wireframe.getValue();
        boxesPipeline = boxes.getValue();
        return Result::ok();
    }();

    if (prepared->failed())
        YUP_DBG ("DebugOverlayPass: " << prepared->getErrorMessage() << ", the frames render without the overlays");

    return *prepared;
}

bool DebugOverlayPass::render (GpuFrame& frame, GpuTarget& target, const ViewData& view, const Lines& lines,
                               const std::function<void()>& beginScope)
{
    numDraws = 0;

    if (target.asTexture() == nullptr || prepare (target.asTexture()->getFormat()).failed())
        return false;

    if (beginScope != nullptr)
        beginScope();

    auto pass = target.beginRenderPass (frame, GpuRenderOptions (GpuLoadOp::load, GpuStoreOp::store));
    if (! pass.isValid())
        return false;

    pass.setUniformBuffer (0, 0, &view.getBlock(), sizeof (ViewData::Block));

    if (lines.wireframe != nullptr)
    {
        pass.setPipeline (wireframePipeline);

        for (const auto items : { lines.wireframe->getOpaqueItems(), lines.wireframe->getBlendItems() })
        {
            for (const auto& item : items)
            {
                auto vertices = item.mesh->getVertexBuffer (device, item.primitiveIndex);
                auto edges = item.mesh->getEdgeIndexBuffer (device, item.primitiveIndex);
                if (vertices == nullptr || edges == nullptr)
                    continue;

                const auto block = makeDebugOverlayLinesBlock (item.world, lines.wireframeColor);
                pass.setUniformBuffer (1, 0, &block, sizeof (block));
                pass.setVertexBuffer (0, vertices);
                pass.setIndexBuffer (GpuIndexFormat::uint32, edges);

                if (pass.drawIndexed (static_cast<uint32_t> (edges->getSizeInBytes() / sizeof (uint32))))
                    ++numDraws;
            }
        }
    }

    if (! lines.boxes.empty())
    {
        pass.setPipeline (boxesPipeline);

        for (const auto& box : lines.boxes)
        {
            if (box.isEmpty())
                continue;

            const auto model = Matrix4::scaling (box.getSize()).followedBy (Matrix4::translation (box.getMin()));
            const auto block = makeDebugOverlayLinesBlock (model, lines.boxColor);
            pass.setUniformBuffer (1, 0, &block, sizeof (block));

            if (pass.draw (24))
                ++numDraws;
        }
    }

    return pass.finish();
}

} // namespace yup
