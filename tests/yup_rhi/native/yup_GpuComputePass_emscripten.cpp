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

#if RIVE_WEBGPU && YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// WebGPU - runs in a WebGPU browser page; skipped under node, which has no device.
//
// The test runner blocks the JavaScript event loop, so buffer mapping never completes
// while a test runs: readBuffer() and readPixels() return false and the pixel checks
// below, and those of GpuComputePassBackendTests, are skipped. A run without WebGPU
// validation errors in the browser console is what verifies the bindings.
//==============================================================================

class GpuComputePassWebGPUTests : public GpuComputePassDeviceFixture
{
protected:
    void SetUp() override
    {
        openDevice (GpuPlatform::WebGPU);
    }
};

TEST_F (GpuComputePassWebGPUTests, ComputeWriteIsSampledByLaterRenderPass)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba16float);
    desc.storage = true;
    auto written = GpuTexture::create (device, desc);
    ASSERT_NE (written, nullptr);

    auto fill = GpuComputePipeline::compileFromGlsl (device, R"(#version 450
layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba16f) uniform writeonly image2D target;
void main() { imageStore (target, ivec2 (gl_GlobalInvocationID.xy), vec4 (0.0, 0.0, 1.0, 1.0)); })",
                                                     GpuWorkgroupSize { 8, 8, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);
    auto sampling = compileRenderPipeline (sampleLodZeroFragment);
    ASSERT_NE (sampling, nullptr);

    // The compute pass submits when it finishes, ahead of the frame's own submission
    auto frame = GpuFrame::begin (device);
    {
        auto compute = GpuComputePass::begin (frame);
        ASSERT_TRUE (compute.isValid());
        compute.setPipeline (fill.getValue());
        compute.setStorageTexture (0, 0, written);
        EXPECT_TRUE (compute.dispatch (1, 1, 1));
    }

    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (sampling);
    pass.setTexture (0, 0, written);
    EXPECT_TRUE (pass.draw (3));
    pass.finish();
    EXPECT_TRUE (frame.submit());

    std::vector<uint8_t> pixels (4 * 4 * 4);
    if (target->readPixels (pixels.data(), pixels.size()))
        expectEveryPixel (pixels, { 0, 0, 255, 255 });
}

TEST_F (GpuComputePassWebGPUTests, DispatchFailsWhenADeclaredResourceIsUnbound)
{
    auto fetch = GpuComputePipeline::compileFromGlsl (device, fetchTexelSource, GpuWorkgroupSize { 1, 1, 1 });
    ASSERT_TRUE (fetch.wasOk()) << fetch.getErrorMessage();

    auto source = GpuTexture::create (device, GpuTextureDesc (8, 8, GpuTextureFormat::rgba8unorm));
    std::array<float, 4> texel {};
    auto result = GpuBuffer::create (device, GpuBufferType::storage, texel.data(), sizeof (float) * texel.size());
    const int32_t params[4] = { 0, 0, 0, 0 };
    ASSERT_NE (source, nullptr);
    ASSERT_NE (result, nullptr);

    // No sampler: the dispatch is refused (with an assertion) instead of failing on the GPU
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (fetch.getValue());
    pass.setTexture (0, 0, source);
    pass.setStorageBuffer (0, 2, result);
    pass.setUniformBuffer (0, 3, params, sizeof (params));
    EXPECT_FALSE (pass.dispatch (1, 1, 1));
}

TEST_F (GpuComputePassWebGPUTests, DispatchesAShaderUsingOnlyGroupOne)
{
    auto doubler = GpuComputePipeline::compileFromGlsl (device, R"(#version 450
layout(local_size_x = 4) in;
layout(std430, set = 1, binding = 0) buffer Data { float values[]; };
void main() { values[gl_GlobalInvocationID.x] *= 2.0; })",
                                                        GpuWorkgroupSize { 4, 1, 1 });
    ASSERT_TRUE (doubler.wasOk()) << doubler.getErrorMessage();

    std::array<float, 4> values { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buffer = GpuBuffer::create (device, GpuBufferType::storage, values.data(), sizeof (float) * values.size());
    ASSERT_NE (buffer, nullptr);

    // Group 0 is empty, yet still needs a bind group of its own
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (doubler.getValue());
    pass.setStorageBuffer (1, 0, buffer);
    EXPECT_TRUE (pass.dispatch (1, 1, 1));
    EXPECT_TRUE (pass.finish());

    if (device->readBuffer (buffer, values.data(), sizeof (float) * values.size()))
        EXPECT_EQ (values, (std::array<float, 4> { 2.0f, 4.0f, 6.0f, 8.0f }));
}

#endif // RIVE_WEBGPU && YUP_ENABLE_SHADER_TRANSPILER
