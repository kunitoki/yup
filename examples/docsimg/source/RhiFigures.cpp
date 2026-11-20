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

#include "Figures.h"

void addRhiFigures (Figures& f)
{
    using namespace yup;

    f.paint ("graphics/rhi/targets_canvas", { 560.0f, 166.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto scale = g.getContextScale();

        auto canvas = GpuCanvas::create (g.getGraphicsContext(), roundToInt (140.0f * scale), roundToInt (88.0f * scale));
        if (canvas == nullptr)
            return failFigure ("The canvas cannot be created");

        auto& c = canvas->beginDraw ({}, scale);
        c.setFillColor (Colors::cornflowerblue);
        c.fillAll();
        c.setFillColor (Colors::white);
        c.fillEllipse ({ 14.0f, 14.0f, 60.0f, 60.0f });
        c.fillFittedText ("2D", font (26.0f), { 80.0f, 0.0f, 54.0f, 88.0f }, Justification::center);

        labelled (g, area, { "at its size", "scaled down", "stretched" }, 3, [&] (int index, Rectangle<float> cell)
        {
            const Size<float> sizes[] = { { 140.0f, 88.0f }, { 70.0f, 44.0f }, { 156.0f, 52.0f } };

            g.drawTexture (canvas->asTexture(), cell.withSizeKeepingCenter (sizes[index].getWidth(), sizes[index].getHeight()));
        });
    });

    f.paint ("graphics/rhi/buffers-and-textures_triangle", { 480.0f, 220.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        struct Vertex
        {
            float x, y, red, green, blue;
        };

        static constexpr Vertex vertices[] = {
            { 0.0f, 0.8f, 1.0f, 0.35f, 0.3f },
            { -0.8f, -0.8f, 0.25f, 0.8f, 0.45f },
            { 0.8f, -0.8f, 0.3f, 0.5f, 1.0f }
        };

        const auto device = g.getGraphicsContext().getGpuDevice();

        // Built from data/triangle.vert and .frag with `just shader_bundler`.
        const auto bundle = ShaderBundle::loadFromFile (docsimgFile ("data/triangle.ysl"));
        if (bundle.failed())
            return failFigure (bundle.getErrorMessage());

        const std::vector<GpuVertexAttribute> attributes {
            { GpuVertexFormat::float2, 0, 0 },
            { GpuVertexFormat::float3, sizeof (float) * 2, 1 },
        };

        GpuPipelineOptions options;
        options.vertexBuffers.emplace_back (static_cast<uint32_t> (sizeof (Vertex)), GpuVertexStepMode::vertex, attributes);

        const auto pipeline = GpuPipeline::compileFromBundle (device, bundle.getReference(), options);
        if (pipeline.failed())
            return failFigure (pipeline.getErrorMessage());

        // Twice the pixels of the figure, as a pass without MSAA does not antialias the edges.
        const auto bounds = area.withSizeKeepingCenter (area.getHeight() * 1.6f, area.getHeight());
        const auto pixels = g.getContextScale() * 2.0f;

        auto target = GpuTarget::create (device, roundToInt (bounds.getWidth() * pixels), roundToInt (bounds.getHeight() * pixels));
        if (target == nullptr)
            return failFigure ("The target cannot be created");

        auto frame = GpuFrame::begin (device);

        auto pass = target->beginRenderPass (frame, { true, role (Role::surface) });
        pass.setPipeline (pipeline.getValue());
        pass.setVertexBuffer (0, GpuBuffer::create (device, GpuBufferType::vertex, vertices, sizeof (vertices)));
        pass.draw (3);
        pass.finish();
        frame.submit();

        g.drawTexture (target->asTexture(), bounds);
    });
}
