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
// GpuComputePass — headless path (compute not available)
//==============================================================================

class GpuComputePassHeadlessTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Headless, {});
        ASSERT_NE (device, nullptr);
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuComputePassHeadlessTests, BeginWithNullDeviceReturnsInvalidPass)
{
    auto pass = GpuComputePass::begin (nullptr);
    EXPECT_FALSE (pass.isValid());
}

TEST_F (GpuComputePassHeadlessTests, BeginWithHeadlessDeviceReturnsInvalidPass)
{
    // Headless backend does not support compute shaders.
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
}

TEST_F (GpuComputePassHeadlessTests, BeginOnInvalidFrameReturnsInvalidPass)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_FALSE (frame.isValid());

    auto pass = GpuComputePass::begin (frame);
    EXPECT_FALSE (pass.isValid());
}

TEST_F (GpuComputePassHeadlessTests, RunOnComputeContextRunsWorkSynchronously)
{
    // Backends without a dedicated compute context run the work directly.
    bool ran = false;
    device->runOnComputeContext ([&]
    {
        ran = true;
    });
    EXPECT_TRUE (ran);
}

TEST_F (GpuComputePassHeadlessTests, SetPipelineOnInvalidPassDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_NO_THROW (pass.setPipeline (nullptr));
}

TEST_F (GpuComputePassHeadlessTests, SetStorageBufferOnInvalidPassDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_NO_THROW (pass.setStorageBuffer (0, 0, nullptr));
}

TEST_F (GpuComputePassHeadlessTests, SetUniformBufferOnInvalidPassDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());

    float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    EXPECT_NO_THROW (pass.setUniformBuffer (0, 0, data, sizeof (data)));
}

TEST_F (GpuComputePassHeadlessTests, SetTextureOnInvalidPassDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_NO_THROW (pass.setTexture (0, 0, nullptr));
}

TEST_F (GpuComputePassHeadlessTests, DispatchOnInvalidPassReturnsFalse)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_FALSE (pass.dispatch (1, 1, 1));
    EXPECT_FALSE (pass.dispatch (16, 8, 4));
}

TEST_F (GpuComputePassHeadlessTests, FinishOnInvalidPassReturnsFalse)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_FALSE (pass.finish());
}

TEST_F (GpuComputePassHeadlessTests, FinishIsIdempotentOnInvalidPass)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.finish());
    EXPECT_FALSE (pass.finish());
}

TEST_F (GpuComputePassHeadlessTests, MoveConstructionFromInvalidPass)
{
    auto src = GpuComputePass::begin (device);
    EXPECT_FALSE (src.isValid());

    GpuComputePass dst (std::move (src));
    EXPECT_FALSE (dst.isValid());
    EXPECT_FALSE (src.isValid());

    EXPECT_FALSE (dst.finish());
}

TEST_F (GpuComputePassHeadlessTests, MoveAssignmentFromInvalidPass)
{
    auto src = GpuComputePass::begin (device);
    auto dst = GpuComputePass::begin (device);

    dst = std::move (src);
    EXPECT_FALSE (dst.isValid());
    EXPECT_FALSE (src.isValid());
}

TEST_F (GpuComputePassHeadlessTests, DestructorOnInvalidPassDoesNotCrash)
{
    {
        auto pass = GpuComputePass::begin (device);
        EXPECT_FALSE (pass.isValid());
        // Destructor should not crash.
    }
    EXPECT_TRUE (true);
}

TEST_F (GpuComputePassHeadlessTests, SetPipelineWithNonNullDoesNotCrash)
{
    // Even though pipeline is null (no compute support), the call should not crash.
    auto pass = GpuComputePass::begin (device);
    EXPECT_NO_THROW (pass.setPipeline (GpuComputePipeline::Ptr (nullptr)));
}

TEST_F (GpuComputePassHeadlessTests, SetStorageBufferCoversMultipleGroups)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_NO_THROW ({
        pass.setStorageBuffer (0, 0, nullptr);
        pass.setStorageBuffer (0, 1, nullptr);
        pass.setStorageBuffer (1, 0, nullptr);
        pass.setStorageBuffer (1, 1, nullptr);
    });
}

TEST_F (GpuComputePassHeadlessTests, SetUniformBufferCoversMultipleGroups)
{
    auto pass = GpuComputePass::begin (device);
    float data = 42.0f;

    EXPECT_NO_THROW ({
        pass.setUniformBuffer (0, 0, &data, sizeof (data));
        pass.setUniformBuffer (0, 1, &data, sizeof (data));
        pass.setUniformBuffer (1, 0, &data, sizeof (data));
    });
}

TEST_F (GpuComputePassHeadlessTests, SetTextureCoversMultipleGroups)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_NO_THROW ({
        pass.setTexture (0, 0, nullptr);
        pass.setTexture (0, 1, nullptr);
        pass.setTexture (1, 0, nullptr);
    });
}

//==============================================================================
// GpuComputePipeline — headless path (compute not available)
//==============================================================================

class GpuComputePipelineHeadlessTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Headless, {});
        ASSERT_NE (device, nullptr);
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuComputePipelineHeadlessTests, CompileWithNullDeviceReturnsFailure)
{
    GpuShaderSource source;
    source.language = GpuShaderLanguage::glsl;
    source.code = gpuShaderSourceBytes ("void main() {}");

    GpuWorkgroupSize wgs { 16, 1, 1 };
    auto result = GpuComputePipeline::compile (nullptr, source, wgs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileWithHeadlessDeviceReturnsFailure)
{
    GpuShaderSource source;
    source.language = GpuShaderLanguage::glsl;
    source.code = gpuShaderSourceBytes ("void main() {}");

    GpuWorkgroupSize wgs { 16, 1, 1 };
    auto result = GpuComputePipeline::compile (device, source, wgs);
    EXPECT_TRUE (result.failed());
    EXPECT_STRNE (result.getErrorMessage().toRawUTF8(), "");
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromBundleWithNullDeviceReturnsFailure)
{
    ShaderBundle bundle;
    auto result = GpuComputePipeline::compileFromBundle (nullptr, bundle);
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromBundleWithHeadlessDeviceReturnsFailure)
{
    ShaderBundle bundle;
    auto result = GpuComputePipeline::compileFromBundle (device, bundle);
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromBundleWithDefaultWorkgroupSize)
{
    ShaderBundle bundle;
    auto result = GpuComputePipeline::compileFromBundle (nullptr, bundle, GpuWorkgroupSize { 8, 8, 1 });
    EXPECT_TRUE (result.failed());
}

#if YUP_ENABLE_SHADER_TRANSPILER

TEST_F (GpuComputePipelineHeadlessTests, CompileFromGlslWithNullDeviceReturnsFailure)
{
    auto result = GpuComputePipeline::compileFromGlsl (nullptr, "#version 450\nvoid main() {}");
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromGlslWithHeadlessDeviceReturnsFailure)
{
    auto result = GpuComputePipeline::compileFromGlsl (device, "#version 450\nvoid main() {}");
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromGlslWithWorkgroupSize)
{
    auto result = GpuComputePipeline::compileFromGlsl (
        nullptr,
        "#version 450\nlayout(local_size_x = 8) in; void main() {}",
        GpuWorkgroupSize { 8, 1, 1 });
    EXPECT_TRUE (result.failed());
}

#endif // YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// Vulkan - runs where a Vulkan loader and driver are available
//==============================================================================

#if YUP_RIVE_USE_VULKAN && YUP_ENABLE_SHADER_TRANSPILER

class GpuComputePassVulkanTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (! GpuDevice::isPlatformSupported (GpuPlatform::Vulkan, {}))
            GTEST_SKIP() << "no Vulkan loader or driver in this environment";

        device = GpuDevice::create (GpuPlatform::Vulkan, {});
        ASSERT_NE (device, nullptr);
        ASSERT_TRUE (device->isComputeAvailable());
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

    GpuDevice::Ptr device;
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

#endif // YUP_RIVE_USE_VULKAN && YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// Metal - passes begun on a frame encode into its command buffer
//==============================================================================

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER

class GpuComputePassMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isComputeAvailable())
            GTEST_SKIP() << "No Metal compute device available";

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

    GpuDevice::Ptr device;
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

#endif // YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER
