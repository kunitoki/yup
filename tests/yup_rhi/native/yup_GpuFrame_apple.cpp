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

#include <objc/runtime.h>

using namespace yup;

//==============================================================================

class GpuFrameMetalTests : public GpuFrameDeviceFixture
{
protected:
    void SetUp() override
    {
        openDevice (GpuPlatform::Metal);
    }
};

TEST_F (GpuFrameMetalTests, FramesDoNotKeepTheirQueueAliveAfterTheDevice)
{
    // Autoreleased command buffers retain their queue and every resource they encoded. Released
    // only when the caller's pool drains, a test run inside one pool ran the GPU out of memory
    id weakQueue = nil;
    objc_storeWeak (&weakQueue, static_cast<id> (device->getNativeCommandQueue()));

    const auto result = clearAndRead (GpuColor (0.0f, 1.0f, 0.0f, 1.0f));
    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage();

    device = nullptr;

    // Loading a weak reference autoreleases the object, which must not keep it alive either
    const auto queueIsAlive = [&weakQueue]
    {
        const ScopedAutoReleasePool pool;
        return objc_loadWeak (&weakQueue) != nil;
    };

    // Metal may release the queue on its own threads shortly after the last command buffer completes
    for (int attempt = 0; attempt < 200 && queueIsAlive(); ++attempt)
        Thread::sleep (10);

    EXPECT_FALSE (queueIsAlive());
    objc_storeWeak (&weakQueue, nil);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (GpuFrameMetalTests, ManyFramesInFlightLeaveSamplesToOtherDevices)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    // Metal lets only a few dozen counter sample buffers live at once in a process. A second
    // device, ready to draw before the first one fills its queue
    auto otherDevice = GpuDevice::create (GpuPlatform::Metal, {});
    ASSERT_NE (otherDevice, nullptr);

    const auto useOtherDevice = [&]
    {
        std::swap (device, otherDevice);
        std::swap (target, otherTarget);
        std::swap (pipeline, otherPipeline);
    };

    useOtherDevice();
    ASSERT_TRUE (prepareDrawing());
    useOtherDevice();
    ASSERT_TRUE (prepareDrawing());

    // Frames heavy on the GPU and cheap to record fill the queue: more measuring frames in flight
    // than counter sample buffers may live
    std::vector<GpuFrameStatus> statuses;
    for (int i = 0; i < 40; ++i)
    {
        auto frame = GpuFrame::begin (device);
        ASSERT_TRUE (frame.isValid());
        frame.setTimingEnabled (true);
        statuses.push_back (frame.getStatus());
        frame.beginScope ("scope");

        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (pipeline);
        pass.draw (3, 1000);
    }

    useOtherDevice();
    const auto other = measureFrame ([this] (GpuFrame& frame)
    {
        frame.beginScope ("other");
        drawPass (frame, 5);
    });
    useOtherDevice();

    ASSERT_EQ (other.scopes.size(), 1u);
    EXPECT_TRUE (other.scopes[0].available);

    // Every queued frame completes with its total, and its scope measured or unavailable
    for (const auto& status : statuses)
    {
        EXPECT_TRUE (status.wait().wasOk());

        const auto timings = status.getTimings();
        ASSERT_TRUE (timings.available);
        ASSERT_EQ (timings.scopes.size(), 1u);
        EXPECT_TRUE (std::isfinite (timings.scopes[0].milliseconds));
        EXPECT_GE (timings.scopes[0].milliseconds, 0.0);
    }

    // Once they completed, the device measures again
    const auto after = measureFrame ([this] (GpuFrame& frame)
    {
        frame.beginScope ("after");
        drawPass (frame, 5);
    });

    ASSERT_EQ (after.scopes.size(), 1u);
    EXPECT_TRUE (after.scopes[0].available);
}
#endif
