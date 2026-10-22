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

#if YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// Metal - passes begun on a frame encode into its command buffer
//==============================================================================

class GpuComputePassMetalTests : public GpuComputePassDeviceFixture
{
protected:
    void SetUp() override
    {
        openDevice (GpuPlatform::Metal);
        if (IsSkipped() || HasFatalFailure())
            return;

        auto compiled = GpuComputePipeline::compileFromGlsl (device,
                                                             "#version 450\n"
                                                             "layout(local_size_x = 4) in;\n"
                                                             "layout(std430, set = 0, binding = 0) buffer Data { float values[]; };\n"
                                                             "void main() { values[gl_GlobalInvocationID.x] *= 2.0; }\n");
        ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
        pipeline = compiled.getValue();

        buffer = GpuBuffer::create (device, GpuBufferType::storage, initialValues, sizeof (initialValues));
        ASSERT_NE (buffer, nullptr);
    }

    void dispatchDoubler (GpuComputePass& pass)
    {
        pass.setPipeline (pipeline);
        pass.setStorageBuffer (0, 0, buffer);
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
    }

    std::array<float, 4> readValues()
    {
        std::array<float, 4> values {};
        EXPECT_TRUE (device->readBuffer (buffer, values.data(), sizeof (float) * values.size()));
        return values;
    }

    static constexpr float initialValues[] = { 1.0f, 2.0f, 3.0f, 4.0f };

    GpuComputePipeline::Ptr pipeline;
    GpuBuffer::Ptr buffer;
};

TEST_F (GpuComputePassMetalTests, PassBegunOnFrameRunsWhenTheFrameIsSubmitted)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto pass = GpuComputePass::begin (frame);
    ASSERT_TRUE (pass.isValid());

    dispatchDoubler (pass);
    EXPECT_TRUE (pass.finish());

    // The dispatch waits for the frame's command buffer
    EXPECT_EQ (readValues(), (std::array<float, 4> { 1.0f, 2.0f, 3.0f, 4.0f }));

    ASSERT_TRUE (frame.submit());
    EXPECT_EQ (readValues(), (std::array<float, 4> { 2.0f, 4.0f, 6.0f, 8.0f }));
}

TEST_F (GpuComputePassMetalTests, SubmittingTheFrameFinishesItsOpenPass)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto pass = GpuComputePass::begin (frame);
    ASSERT_TRUE (pass.isValid());
    dispatchDoubler (pass);

    ASSERT_TRUE (frame.submit());

    EXPECT_FALSE (pass.isValid());
    EXPECT_FALSE (pass.dispatch (1, 1, 1));
    EXPECT_FALSE (pass.finish());
    EXPECT_EQ (readValues(), (std::array<float, 4> { 2.0f, 4.0f, 6.0f, 8.0f }));
}

TEST_F (GpuComputePassMetalTests, BeginningAnotherPassOnTheFrameFinishesTheOpenOne)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto first = GpuComputePass::begin (frame);
    ASSERT_TRUE (first.isValid());
    dispatchDoubler (first);

    auto second = GpuComputePass::begin (frame);
    ASSERT_TRUE (second.isValid());
    EXPECT_FALSE (first.isValid());
    dispatchDoubler (second);

    ASSERT_TRUE (frame.submit());
    EXPECT_EQ (readValues(), (std::array<float, 4> { 4.0f, 8.0f, 12.0f, 16.0f }));
}

TEST_F (GpuComputePassMetalTests, RenderPassOnTheFrameFinishesTheOpenPass)
{
    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto pass = GpuComputePass::begin (frame);
    ASSERT_TRUE (pass.isValid());
    dispatchDoubler (pass);

    // A clearing pass opens even without draws
    auto renderPass = target->beginRenderPass (frame);
    ASSERT_TRUE (renderPass.isValid());
    EXPECT_TRUE (renderPass.finish());
    EXPECT_FALSE (pass.isValid());

    ASSERT_TRUE (frame.submit());
    EXPECT_EQ (readValues(), (std::array<float, 4> { 2.0f, 4.0f, 6.0f, 8.0f }));
}

TEST_F (GpuComputePassMetalTests, StorageTextureRejectsMultisampling)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba16float, true);
    desc.storage = true;
    desc.sampleCount = 4;
    EXPECT_EQ (GpuTexture::create (device, desc), nullptr);

    desc.sampleCount = 1;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    EXPECT_TRUE (texture->isStorage());
}

TEST_F (GpuComputePassMetalTests, StorageTextureRejectsDepthFormats)
{
    for (auto format : { GpuTextureFormat::depth16unorm, GpuTextureFormat::depth32float, GpuTextureFormat::depth24plusStencil8 })
    {
        GpuTextureDesc desc (8, 8, format, true);
        desc.storage = true;
        EXPECT_EQ (GpuTexture::create (device, desc), nullptr) << "format " << (int) format;
    }
}

TEST_F (GpuComputePassMetalTests, StorageCubeTextureIsIgnored)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "A storage cube texture triggers an assertion in assertion-enabled builds";
#endif

    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba16float);
    desc.type = GpuTextureType::cube;
    desc.depthOrArrayLayers = 6;
    desc.storage = true;
    auto cube = GpuTexture::create (device, desc);
    ASSERT_NE (cube, nullptr);

    GpuTextureDesc planeDesc (8, 8, GpuTextureFormat::rgba16float);
    planeDesc.storage = true;
    auto plane = GpuTexture::create (device, planeDesc);
    ASSERT_NE (plane, nullptr);

    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());

    // A binding keeps its texture alive, so only the accepted plane gains a reference
    pass.setStorageTexture (0, 0, cube, GpuTextureViewDesc (0, 2));
    pass.setStorageTexture (0, 1, plane);
    EXPECT_EQ (cube->getReferenceCount(), 1);
    EXPECT_EQ (plane->getReferenceCount(), 2);
}

TEST_F (GpuComputePassMetalTests, VertexReadsAStorageBuffer)
{
    const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    auto colors = device->createBuffer (GpuBufferType::storage, color, sizeof (color));
    ASSERT_NE (colors, nullptr);

    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);

    GpuPipelineOptions options;
    options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;

    auto compiled = GpuPipeline::compileFromGlsl (device, R"(#version 450
layout(std430, set = 0, binding = 0) readonly buffer Colors { vec4 colors[]; };
layout(location = 0) out vec4 vertexColor;
void main()
{
    const vec2 uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
    vertexColor = colors[0];
})",
                                                  R"(#version 450
layout(location = 0) in vec4 vertexColor;
layout(location = 0) out vec4 color;
void main() { color = vertexColor; })",
                                                  options);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (compiled.getValue());
    pass.setStorageBuffer (0, 0, colors);
    EXPECT_TRUE (pass.draw (3));
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8_t> pixels (4 * 4 * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));
    EXPECT_LE (pixels[0], 5);
    EXPECT_LE (pixels[1], 5);
    EXPECT_GE (pixels[2], 250);
}

TEST_F (GpuComputePassMetalTests, FrameRetainsABoundStorageBufferUntilItsGenerationIsSafe)
{
    const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    auto colors = device->createBuffer (GpuBufferType::storage, color, sizeof (color));
    ASSERT_NE (colors, nullptr);

    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);

    GpuPipelineOptions options;
    options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;
    auto compiled = GpuPipeline::compileFromGlsl (device, fullscreenVertex, storageColorFragment, options);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    {
        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (compiled.getValue());
        pass.setStorageBuffer (0, 0, colors);
        EXPECT_TRUE (pass.draw (3));
    }

    // The pass is gone and the frame retired without waiting on the GPU, so later
    // frames have to begin before the buffer is let go
    int laterFrames = 0;
    for (; laterFrames < 16 && colors->getReferenceCount() > 1; ++laterFrames)
        GpuFrame::begin (device).submit();

    EXPECT_EQ (colors->getReferenceCount(), 1);
    EXPECT_GT (laterFrames, 1);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
