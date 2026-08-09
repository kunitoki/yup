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

TEST_F (GpuComputePassHeadlessTests, IsValidReturnsFalseForDefaultConstructed)
{
    GpuComputePass pass;
    EXPECT_FALSE (pass.isValid());
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
    source.code = "void main() {}";
    source.codeSize = static_cast<uint32_t> (strlen (static_cast<const char*> (source.code)));

    GpuWorkgroupSize wgs { 16, 1, 1 };
    auto result = GpuComputePipeline::compile (nullptr, source, wgs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileWithHeadlessDeviceReturnsFailure)
{
    GpuShaderSource source;
    source.language = GpuShaderLanguage::glsl;
    source.code = "void main() {}";
    source.codeSize = static_cast<uint32_t> (strlen (static_cast<const char*> (source.code)));

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
