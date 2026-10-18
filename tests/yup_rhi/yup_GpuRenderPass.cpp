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

#include <gtest/gtest.h>

#include <yup_rhi/yup_rhi.h>

using namespace yup;

//==============================================================================
// Real devices - uniform buffers bound at dynamic offsets, read back draw by draw
//==============================================================================

#if YUP_ENABLE_SHADER_TRANSPILER && (YUP_APPLE || (YUP_WINDOWS && YUP_RIVE_USE_D3D) || YUP_RIVE_USE_VULKAN || (YUP_EMSCRIPTEN && RIVE_WEBGPU))

class GpuRenderPassDynamicUniformTests
    : public GpuComputePassDeviceFixture
    , public ::testing::WithParamInterface<GpuPlatform>
{
protected:
    void SetUp() override
    {
        openDevice (GetParam());

        // The tests pass on either path: record which one this device ran
        if (device != nullptr)
            RecordProperty ("dynamicUniformOffsets", device->isDynamicUniformOffsetAvailable() ? "yes" : "no");
    }

    /** Writes the red and green of DrawBlock, the blue of MaterialBlock and the alpha of the static ViewBlock. */
    static constexpr const char* drawAndMaterialFragment = R"(#version 450
layout(set = 0, binding = 0) uniform ViewBlock { vec4 value; } view;
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(set = 2, binding = 1) uniform MaterialBlock { vec4 value; } material;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = vec4 (draw.value.r, draw.value.g, material.value.b, view.value.a); })";

    /** Compiles @p fragment behind a fullscreen triangle for one rgba8 target without blending,
        declaring @p dynamic dynamic. */
    GpuPipeline::Ptr compileWithDynamicSlots (const char* vertex, const char* fragment, std::vector<GpuUniformSlot> dynamic)
    {
        GpuPipelineOptions options;
        auto& colorTarget = options.colorTargets.emplace_back();
        colorTarget.format = GpuTextureFormat::rgba8unorm;
        colorTarget.blendEnabled = false;
        options.dynamicUniformBuffers = std::move (dynamic);

        auto compiled = GpuPipeline::compileFromGlsl (device, vertex, fragment, options);
        EXPECT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
        return compiled.wasOk() ? compiled.getValue() : nullptr;
    }

    /** A vec4 block of four bytes in unorm form. */
    struct Block
    {
        float value[4] = {};

        static Block of (int red, int green, int blue, int alpha = 255)
        {
            return { { (float) red / 255.0f, (float) green / 255.0f, (float) blue / 255.0f, (float) alpha / 255.0f } };
        }
    };

    /** The color draw @p index of frame @p frameIndex writes: a draw block every draw, a material
        block every third draw, so some draws reuse the material block they find in the ring. */
    static std::array<int, 3> colorOf (int frameIndex, int index)
    {
        return { (index * 7 + frameIndex * 31) & 255, (index * 37 + frameIndex * 13) & 255, ((index / 3) * 53 + frameIndex * 71) & 255 };
    }

    /** Sets the blocks of draw @p index and draws it into column @p index of a single-row target. */
    static void drawColumn (GpuRenderPass& pass, int frameIndex, int index)
    {
        const auto color = colorOf (frameIndex, index);
        const auto draw = Block::of (color[0], color[1], 0);
        const auto material = Block::of (0, 0, color[2]);

        pass.setUniformBuffer (2, 0, draw.value, sizeof (draw.value));
        pass.setUniformBuffer (2, 1, material.value, sizeof (material.value));
        pass.setViewport ((float) index, 0.0f, 1.0f, 1.0f);
        EXPECT_TRUE (pass.draw (3));
    }

    /** Begins a pass clearing @p target, with the static view block every draw reads its alpha from. */
    static GpuRenderPass beginColumns (GpuTarget& target, GpuFrame& frame, GpuPipeline::Ptr pipeline)
    {
        const auto view = Block::of (0, 0, 0, 255);

        auto pass = target.beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (std::move (pipeline));
        pass.setUniformBuffer (0, 0, view.value, sizeof (view.value));
        return pass;
    }

    /** Reads back the single row of @p target, or nothing when the readback did not complete. */
    std::vector<uint8_t> readRow (GpuTarget& target, int width)
    {
        std::vector<uint8_t> pixels ((size_t) width * 4);
        if (! readbackCompleted (target.readPixels (pixels.data(), pixels.size())))
            return {};

        return pixels;
    }

    static void expectColumn (const std::vector<uint8_t>& pixels, int column, std::array<int, 3> color, const char* what)
    {
        const auto* pixel = pixels.data() + column * 4;
        EXPECT_NEAR (pixel[0], color[0], 1) << what << " column " << column << " red";
        EXPECT_NEAR (pixel[1], color[1], 1) << what << " column " << column << " green";
        EXPECT_NEAR (pixel[2], color[2], 1) << what << " column " << column << " blue";
        EXPECT_EQ (pixel[3], 255) << what << " column " << column << " alpha";
    }
};

TEST_P (GpuRenderPassDynamicUniformTests, ManyDrawsReadBackTheirOwnDynamicUniforms)
{
    // A 256 byte slot per draw block, and one every third draw for the material: over 64 KB
    constexpr int draws = 300;

    auto target = GpuTarget::create (device, draws, 1);
    ASSERT_NE (target, nullptr);

    auto pipeline = compileWithDynamicSlots (fullscreenVertex, drawAndMaterialFragment, { { 2, 0 }, { 2, 1 } });
    ASSERT_NE (pipeline, nullptr);

    auto frame = GpuFrame::begin (device);
    auto pass = beginColumns (*target, frame, pipeline);
    for (int index = 0; index < draws; ++index)
        drawColumn (pass, 0, index);

    pass.finish();
    EXPECT_TRUE (frame.submit());
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto pixels = readRow (*target, draws);
    if (pixels.empty())
        return;

    for (int index = 0; index < draws; ++index)
        expectColumn (pixels, index, colorOf (0, index), "draw");
}

TEST_P (GpuRenderPassDynamicUniformTests, ReusedChunksAcrossFramesStayCorrect)
{
    // Every frame fills more than one chunk, and frames retire into the pool while the GPU may still run them
    constexpr int frames = 10;
    constexpr int draws = 200;

    auto pipeline = compileWithDynamicSlots (fullscreenVertex, drawAndMaterialFragment, { { 2, 0 }, { 2, 1 } });
    ASSERT_NE (pipeline, nullptr);

    std::vector<GpuTarget::Ptr> targets;
    std::optional<GpuFrame> lastFrame;

    for (int frameIndex = 0; frameIndex < frames; ++frameIndex)
    {
        auto target = GpuTarget::create (device, draws, 1);
        ASSERT_NE (target, nullptr);

        auto frame = GpuFrame::begin (device);
        auto pass = beginColumns (*target, frame, pipeline);
        for (int index = 0; index < draws; ++index)
            drawColumn (pass, frameIndex, index);

        pass.finish();
        EXPECT_TRUE (frame.submit());

        targets.push_back (std::move (target));
        lastFrame.emplace (std::move (frame));
    }

    ASSERT_TRUE (lastFrame.has_value());
    EXPECT_TRUE (lastFrame->waitForGPU().wasOk());

    for (int frameIndex = 0; frameIndex < frames; ++frameIndex)
    {
        const auto pixels = readRow (*targets[(size_t) frameIndex], draws);
        if (pixels.empty())
            return;

        for (int index = 0; index < draws; ++index)
            expectColumn (pixels, index, colorOf (frameIndex, index), "frame");
    }
}

TEST_P (GpuRenderPassDynamicUniformTests, PipelinesWithStaticAndDynamicLayoutsInterleave)
{
    constexpr int draws = 64;

    auto target = GpuTarget::create (device, draws, 1);
    ASSERT_NE (target, nullptr);

    auto dynamicPipeline = compileWithDynamicSlots (fullscreenVertex, drawAndMaterialFragment, { { 2, 0 }, { 2, 1 } });
    auto staticPipeline = compileWithDynamicSlots (fullscreenVertex, drawAndMaterialFragment, {});
    auto drawOnlyPipeline = compileWithDynamicSlots (fullscreenVertex, drawAndMaterialFragment, { { 2, 0 } });
    ASSERT_NE (dynamicPipeline, nullptr);
    ASSERT_NE (staticPipeline, nullptr);
    ASSERT_NE (drawOnlyPipeline, nullptr);

    const GpuPipeline::Ptr pipelines[] = { dynamicPipeline, staticPipeline, drawOnlyPipeline };

    auto frame = GpuFrame::begin (device);
    auto pass = beginColumns (*target, frame, dynamicPipeline);

    // Every draw switches the layout of set 2, and some keep blocks the previous layout uploaded
    for (int index = 0; index < draws; ++index)
    {
        pass.setPipeline (pipelines[index % 3]);
        drawColumn (pass, 0, index);
    }

    pass.finish();
    EXPECT_TRUE (frame.submit());
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto pixels = readRow (*target, draws);
    if (pixels.empty())
        return;

    for (int index = 0; index < draws; ++index)
        expectColumn (pixels, index, colorOf (0, index), "draw");
}

TEST_P (GpuRenderPassDynamicUniformTests, ThreeDynamicBlocksInOneSetKeepTheirOrder)
{
    // The vertex stage reads only the highest binding, so native slots and bindings may be counted apart
    constexpr const char* vertex = R"(#version 450
layout(set = 2, binding = 12) uniform LightBlock { vec4 value; } light;
layout(location = 0) out vec2 uv;
layout(location = 1) flat out float lightValue;
void main()
{
    uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    lightValue = light.value.x;
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
})";

    constexpr const char* fragment = R"(#version 450
layout(set = 2, binding = 0) uniform DrawBlock { vec4 value; } draw;
layout(set = 2, binding = 1) uniform MaterialBlock { vec4 value; } material;
layout(set = 2, binding = 12) uniform LightBlock { vec4 value; } light;
layout(location = 0) in vec2 uv;
layout(location = 1) flat in float lightValue;
layout(location = 0) out vec4 color;
void main() { color = vec4 (draw.value.x, material.value.x, lightValue, light.value.w); })";

    constexpr int draws = 16;

    auto target = GpuTarget::create (device, draws, 1);
    ASSERT_NE (target, nullptr);

    auto pipeline = compileWithDynamicSlots (vertex, fragment, { { 2, 12 }, { 2, 1 }, { 2, 0 } });
    ASSERT_NE (pipeline, nullptr);

    const auto colorAt = [] (int index) -> std::array<int, 3>
    {
        // Each block changes on its own cadence, so a swapped offset shows up as a wrong channel
        return { 10 + index * 15, 200 - (index / 2) * 20, 30 + (index / 4) * 50 };
    };

    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (pipeline);

    for (int index = 0; index < draws; ++index)
    {
        const auto color = colorAt (index);
        const auto draw = Block::of (color[0], 0, 0);
        const auto material = Block::of (color[1], 0, 0);
        const auto light = Block::of (color[2], 0, 0, 255);

        pass.setUniformBuffer (2, 0, draw.value, sizeof (draw.value));
        pass.setUniformBuffer (2, 1, material.value, sizeof (material.value));
        pass.setUniformBuffer (2, 12, light.value, sizeof (light.value));
        pass.setViewport ((float) index, 0.0f, 1.0f, 1.0f);
        EXPECT_TRUE (pass.draw (3));
    }

    pass.finish();
    EXPECT_TRUE (frame.submit());
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto pixels = readRow (*target, draws);
    if (pixels.empty())
        return;

    for (int index = 0; index < draws; ++index)
        expectColumn (pixels, index, colorAt (index), "draw");
}

INSTANTIATE_TEST_SUITE_P (GpuRenderPass,
                          GpuRenderPassDynamicUniformTests,
                          ::testing::ValuesIn (GpuComputePassBackendTests::getCompiledPlatforms()),
                          GpuComputePassBackendTests::getTestNameSuffix);

#endif
