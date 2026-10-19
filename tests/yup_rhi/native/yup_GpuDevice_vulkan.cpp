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

TEST_F (GpuDeviceErrorTests, CreateVulkanWithoutAWindowLoadsTheSystemLoader)
{
    // No loader is passed in: the device finds the system one
    if (! GpuDevice::isPlatformSupported (GpuPlatform::Vulkan, {}))
        GTEST_SKIP() << "no Vulkan loader or driver in this environment";

    auto ctx = GpuDevice::create (GpuPlatform::Vulkan, {});
    ASSERT_NE (ctx, nullptr);
    EXPECT_EQ (ctx->getPlatform(), GpuPlatform::Vulkan);
    EXPECT_TRUE (ctx->isComputeAvailable());
    EXPECT_FALSE (ctx->isDeviceLost());
}
