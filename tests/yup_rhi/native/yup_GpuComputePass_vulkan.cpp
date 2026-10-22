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
// Vulkan - runs where a Vulkan loader and driver are available
//==============================================================================

class GpuComputePassVulkanTests : public GpuComputePassDeviceFixture
{
protected:
    void SetUp() override
    {
        openDevice (GpuPlatform::Vulkan);
    }

    GpuComputePipeline::Ptr compileDoubler (bool withBindingMap)
    {
        static constexpr auto glsl = "#version 450\n"
                                     "layout(local_size_x = 4) in;\n"
                                     "layout(std430, set = 0, binding = 0) buffer Data { float values[]; };\n"
                                     "void main() { values[gl_GlobalInvocationID.x] *= 2.0; }\n";

        if (withBindingMap)
        {
            auto pipeline = GpuComputePipeline::compileFromGlsl (device, glsl);
            return pipeline.wasOk() ? pipeline.getValue() : nullptr;
        }

        auto spirv = ShaderTranspiler().compileToSPIRV (glsl, ShaderStage::compute, ShaderLanguage::glsl);
        if (spirv.failed())
            return nullptr;

        const auto& module = spirv.getReference();
        auto* bytes = static_cast<const uint8*> (module.getData());

        GpuShaderSource source;
        source.language = GpuShaderLanguage::spirv;
        source.code.assign (bytes, bytes + module.getSize());

        auto pipeline = GpuComputePipeline::compile (device, source, {});
        return pipeline.wasOk() ? pipeline.getValue() : nullptr;
    }
};

TEST_F (GpuComputePassVulkanTests, CompilesSpirvWithoutBindingMapByReflectingIt)
{
    auto pipeline = compileDoubler (false);
    ASSERT_NE (pipeline, nullptr);

    const auto size = pipeline->getWorkgroupSize();
    EXPECT_EQ (size.x, 4u);
    EXPECT_EQ (size.y, 1u);
    EXPECT_EQ (size.z, 1u);
}

TEST_F (GpuComputePassVulkanTests, PassBegunOnFrameRunsWhenTheFrameIsSubmitted)
{
    auto pipeline = compileDoubler (true);
    ASSERT_NE (pipeline, nullptr);

    float values[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buffer = GpuBuffer::create (device, GpuBufferType::storage, values, sizeof (values));
    ASSERT_NE (buffer, nullptr);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    {
        auto pass = GpuComputePass::begin (frame);
        ASSERT_TRUE (pass.isValid());

        pass.setPipeline (pipeline);
        pass.setStorageBuffer (0, 0, buffer);
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
        pass.finish();
    }

    ASSERT_TRUE (frame.submit());

    float result[4] = {};
    ASSERT_TRUE (device->readBuffer (buffer, result, sizeof (result)));
    EXPECT_FLOAT_EQ (result[0], 2.0f);
    EXPECT_FLOAT_EQ (result[1], 4.0f);
    EXPECT_FLOAT_EQ (result[2], 6.0f);
    EXPECT_FLOAT_EQ (result[3], 8.0f);
}

TEST_F (GpuComputePassVulkanTests, ComputeWriteToOneMipKeepsTheOtherLevels)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 3;
    desc.storage = true;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    // Mip 2 is never written: it has no contents for the layout transition to keep
    clearMip (texture, 0, GpuColor (1.0f, 0.0f, 0.0f, 1.0f));

    auto fill = GpuComputePipeline::compileFromGlsl (device, fillGreenSource, GpuWorkgroupSize { 4, 4, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    auto frame = GpuFrame::begin (device);
    {
        auto pass = GpuComputePass::begin (frame);
        ASSERT_TRUE (pass.isValid());
        pass.setPipeline (fill.getValue());
        pass.setStorageTexture (0, 0, texture, GpuTextureViewDesc (1, 0));
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
    }
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    if (const auto texel = fetchTexel (texture, 0))
        EXPECT_EQ (*texel, (Texel { 1.0f, 0.0f, 0.0f, 1.0f }));

    if (const auto texel = fetchTexel (texture, 1))
        EXPECT_EQ (*texel, (Texel { 0.0f, 1.0f, 0.0f, 1.0f }));
}

TEST_F (GpuComputePassVulkanTests, StorageTextureOnAPassBegunOnTheDeviceIsRejected)
{
    GpuTextureDesc desc (4, 4, GpuTextureFormat::rgba8unorm);
    desc.storage = true;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    auto fill = GpuComputePipeline::compileFromGlsl (device, fillGreenSource, GpuWorkgroupSize { 4, 4, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    // Expect an assertion: the image layouts ore tracks belong to the frame's command buffer
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (fill.getValue());
    pass.setStorageTexture (0, 0, texture);
    EXPECT_FALSE (pass.dispatch (1, 1, 1));
}

TEST_F (GpuComputePassVulkanTests, RetiredFrameDoesNotKeepTheDeviceAlive)
{
    const auto referencesBefore = device->getReferenceCount();

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

        // The frame is submitted and retired as it goes out of scope, without waiting on the GPU
        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (compiled.getValue());
        pass.setStorageBuffer (0, 0, colors);
        EXPECT_TRUE (pass.draw (3));
    }

    // No later frame begins, so the retired frame is not released yet: holding the buffer there
    // would hold the device through it, and the device could never be freed
    EXPECT_EQ (device->getReferenceCount(), referencesBefore);
}

TEST_F (GpuComputePassVulkanTests, StorageBufferDroppedRightAfterTheDrawIsStillRead)
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

    auto frame = GpuFrame::begin (device);
    {
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (compiled.getValue());
        pass.setStorageBuffer (0, 0, colors);
        EXPECT_TRUE (pass.draw (3));
    }

    // The last reference goes before the GPU has even seen the commands
    ASSERT_EQ (colors->getReferenceCount(), 1);
    colors = nullptr;

    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8_t> pixels (4 * 4 * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));
    EXPECT_LE (pixels[0], 5);
    EXPECT_LE (pixels[1], 5);
    EXPECT_GE (pixels[2], 250);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
