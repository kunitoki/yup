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

// ---------------------------------------------------------------------------
// GpuComputePipeline — Metal
// ---------------------------------------------------------------------------

class GpuComputePipelineMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isComputeAvailable())
            GTEST_SKIP() << "No Metal compute device available";
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuComputePipelineMetalTests, CompileFromBundleResolvesRenamedMainEntryPoint)
{
    // SPIRV-Cross names the MSL kernel main0, while the bundle keeps the GLSL entry point name.
    ShaderInfo info;
    info.stage = ShaderStage::compute;
    info.language = ShaderLanguage::msl;
    info.entryPoint = "main";
    info.source = "#include <metal_stdlib>\n"
                  "using namespace metal;\n"
                  "kernel void main0 (uint3 id [[thread_position_in_grid]]) {}\n";

    ShaderBundle bundle;
    bundle.addShader (std::move (info));

    auto result = GpuComputePipeline::compileFromBundle (device, bundle, GpuWorkgroupSize { 8, 1, 1 });
    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage();
}

// ---------------------------------------------------------------------------
// GpuDevice capabilities - Metal
// ---------------------------------------------------------------------------

class GpuDeviceMetalCapabilityTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuDeviceMetalCapabilityTests, ReportsFourColorAttachments)
{
    EXPECT_EQ (device->getMaximumColorAttachments(), 4u);
}

TEST_F (GpuDeviceMetalCapabilityTests, ReportsDynamicUniformOffsets)
{
    EXPECT_TRUE (device->isDynamicUniformOffsetAvailable());
}

TEST_F (GpuDeviceMetalCapabilityTests, SampleCountIsPerFormat)
{
    EXPECT_GE (device->getMaximumSampleCount (GpuTextureFormat::rgba8unorm), 4u);
    EXPECT_GE (device->getMaximumSampleCount (GpuTextureFormat::rgba16float), 4u);
    EXPECT_EQ (device->getMaximumSampleCount (GpuTextureFormat::bc1unorm), 0u);
}

TEST_F (GpuDeviceMetalCapabilityTests, SampleCountsArePowersOfTwo)
{
    EXPECT_TRUE (std::has_single_bit (device->getMaximumSampleCount()));

    for (const auto format : { GpuTextureFormat::rgba8unorm,
                               GpuTextureFormat::rgba16float,
                               GpuTextureFormat::r11g11b10float,
                               GpuTextureFormat::depth32float })
    {
        EXPECT_TRUE (std::has_single_bit (device->getMaximumSampleCount (format)));
    }
}

TEST_F (GpuDeviceMetalCapabilityTests, PackedFloatIsRenderable)
{
    EXPECT_TRUE (device->isFormatRenderable (GpuTextureFormat::r11g11b10float));
}
