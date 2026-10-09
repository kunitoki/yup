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

#include <yup_rhi/yup_rhi.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#if YUP_APPLE
#include <objc/runtime.h>
#endif

using namespace yup;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

//==============================================================================

class GpuFrameTests : public ::testing::Test
{
protected:
    /** A device whose frames record into fake host-owned command buffers. */
    class FrameCommandsDevice : public OreInjectedGpuDevice
    {
    public:
        using OreInjectedGpuDevice::OreInjectedGpuDevice;
        using GpuDevice::getFrameCompletion;
        using GpuDevice::getCurrentFrameGeneration;

        bool reportsFrameCompletion() const noexcept override { return reportsCompletion; }

        void* beginFrameCommands (uint64_t) override { return &commandBuffer; }

        Result submitFrameCommands (void* handle) override
        {
            ++submits;
            EXPECT_EQ (handle, &commandBuffer);
            return submitResult;
        }

        Result waitFrameCommands (void* handle) override
        {
            ++waits;
            EXPECT_EQ (handle, &commandBuffer);
            return waitResult;
        }

        void releaseFrameCommands (void* handle) override
        {
            ++releases;
            EXPECT_EQ (handle, &commandBuffer);
        }

        int commandBuffer = 0;
        bool reportsCompletion = false;
        Result submitResult = Result::ok();
        Result waitResult = Result::ok();
        int submits = 0;
        int waits = 0;
        int releases = 0;
    };

    void SetUp() override
    {
        oreContext = std::make_unique<NiceMock<MockOreContext>>();
        device = new FrameCommandsDevice (oreContext.get());
    }

    std::unique_ptr<NiceMock<MockOreContext>> oreContext;
    ReferenceCountedObjectPtr<FrameCommandsDevice> device;
};

TEST_F (GpuFrameTests, WaitForGpuReportsTheCommandBufferFailure)
{
    device->waitResult = Result::fail ("command buffer faulted");

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());
    EXPECT_TRUE (frame.submit());

    const auto result = frame.waitForGPU();
    EXPECT_TRUE (result.failed());
    EXPECT_EQ (result.getErrorMessage(), "command buffer faulted");
}

TEST_F (GpuFrameTests, WaitForGpuRepeatsItsResultWithoutWaitingAgain)
{
    device->waitResult = Result::fail ("command buffer faulted");

    auto frame = GpuFrame::begin (device);
    frame.submit();

    EXPECT_TRUE (frame.waitForGPU().failed());
    EXPECT_EQ (frame.waitForGPU().getErrorMessage(), "command buffer faulted");
    EXPECT_EQ (device->waits, 1);
}

TEST_F (GpuFrameTests, WaitForGpuSubmitsAFrameNotYetSubmitted)
{
    {
        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (frame.waitForGPU().wasOk());
        EXPECT_FALSE (frame.submit());
    }

    EXPECT_EQ (device->submits, 1);
    EXPECT_EQ (device->waits, 1);
    EXPECT_EQ (device->releases, 1);
}

TEST_F (GpuFrameTests, WaitForGpuOnAnInvalidFrameFails)
{
    auto frame = GpuFrame::begin (GpuDevice::create (GpuPlatform::Headless, {}));
    ASSERT_FALSE (frame.isValid());

    EXPECT_TRUE (frame.waitForGPU().failed());
}

TEST_F (GpuFrameTests, ReleasesItsCommandBufferOnceItIsDone)
{
    {
        auto frame = GpuFrame::begin (device);
        frame.submit();
        EXPECT_EQ (device->releases, 0);
    }

    EXPECT_EQ (device->releases, 1);

    {
        auto frame = GpuFrame::begin (device);
        frame.submit();
        frame.waitForGPU();
        EXPECT_EQ (device->releases, 2);
    }

    EXPECT_EQ (device->releases, 2);
}

TEST_F (GpuFrameTests, BeginFailsWhenTheBackendCannotRecord)
{
    ON_CALL (*oreContext, beginFrame (_)).WillByDefault (Invoke ([this] (const rive::ore::Context::FrameDescriptor&)
    {
        oreContext->setLastError ("no command buffer");
    }));

    EXPECT_CALL (*oreContext, endFrame()).Times (0);

    {
        auto frame = GpuFrame::begin (device);
        EXPECT_FALSE (frame.isValid());
        EXPECT_FALSE (frame.submit());
    }

    EXPECT_EQ (device->releases, 1);
}

TEST_F (GpuFrameTests, FailureOfAFrameNobodyOwnsIsTakenOnce)
{
    EXPECT_TRUE (device->takeGpuFailure().wasOk());

    device->failNextFrameForTesting ("first");
    GpuFrame::begin (device).submit();
    device->failNextFrameForTesting ("second");
    GpuFrame::begin (device).submit();

    const auto failure = device->takeGpuFailure();
    EXPECT_TRUE (failure.failed());
    EXPECT_EQ (failure.getErrorMessage(), "first");
    EXPECT_TRUE (device->takeGpuFailure().wasOk());
}

TEST_F (GpuFrameTests, FailureReturnedByWaitForGpuIsNotReportedAgain)
{
    device->failNextFrameForTesting ("injected");

    auto frame = GpuFrame::begin (device);
    frame.submit();

    EXPECT_EQ (frame.waitForGPU().getErrorMessage(), "injected");
    EXPECT_TRUE (device->takeGpuFailure().wasOk());
}

TEST_F (GpuFrameTests, FailureOfAFrameWhoseStatusWasTakenIsReportedThereOnly)
{
    device->failNextFrameForTesting ("injected");

    GpuFrameStatus status;
    {
        auto frame = GpuFrame::begin (device);
        status = frame.getStatus();
        EXPECT_FALSE (status.poll().has_value());
    }

    const auto outcome = status.poll();
    ASSERT_TRUE (outcome.has_value());
    EXPECT_EQ (outcome->getErrorMessage(), "injected");
    EXPECT_EQ (status.wait().getErrorMessage(), "injected");
    EXPECT_TRUE (device->takeGpuFailure().wasOk());
}

TEST_F (GpuFrameTests, OnlyTheNextFrameFails)
{
    device->failNextFrameForTesting ("injected");

    auto failing = GpuFrame::begin (device);
    auto succeeding = GpuFrame::begin (device);

    EXPECT_TRUE (succeeding.waitForGPU().wasOk());
    EXPECT_TRUE (failing.waitForGPU().failed());
}

TEST_F (GpuFrameTests, StatusOfAnUnsubmittedFrameCannotBeWaitedOn)
{
    auto frame = GpuFrame::begin (device);
    const auto status = frame.getStatus();

    ASSERT_TRUE (status.isValid());
    EXPECT_FALSE (status.poll().has_value());
    EXPECT_TRUE (status.wait().failed());
}

TEST_F (GpuFrameTests, InvalidFrameHasAnInvalidStatus)
{
    auto frame = GpuFrame::begin (GpuDevice::create (GpuPlatform::Headless, {}));
    const auto status = frame.getStatus();

    EXPECT_FALSE (status.isValid());
    EXPECT_TRUE (status.wait().failed());
}

TEST_F (GpuFrameTests, FrameCompletesWhenTheBackendReportsIt)
{
    device->reportsCompletion = true;
    const auto completeFrame = device->getFrameCompletion();

    auto frame = GpuFrame::begin (device);
    const auto status = frame.getStatus();
    frame.submit();
    EXPECT_FALSE (status.poll().has_value());

    completeFrame (device->getCurrentFrameGeneration(), Result::fail ("faulted"));
    EXPECT_EQ (status.wait().getErrorMessage(), "faulted");
    EXPECT_TRUE (device->takeGpuFailure().wasOk());
}

TEST_F (GpuFrameTests, CompletionOutlivesTheDevice)
{
    device->reportsCompletion = true;
    const auto completeFrame = device->getFrameCompletion();

    GpuFrameStatus status;
    uint64_t generation = 0;
    {
        auto frame = GpuFrame::begin (device);
        status = frame.getStatus();
        generation = device->getCurrentFrameGeneration();
    }

    // A completion handler may run after its device was destroyed, which fails what never completed
    device = nullptr;
    EXPECT_EQ (status.wait().getErrorMessage(), "The device was destroyed before the frame completed");

    completeFrame (generation, Result::ok());
    EXPECT_TRUE (status.wait().failed());
}

TEST_F (GpuFrameTests, OnlyTheFirstUnclaimedFailureIsKept)
{
    for (int index = 0; index < 4; ++index)
    {
        device->failNextFrameForTesting ("failure " + String (index));
        GpuFrame::begin (device).submit();
    }

    EXPECT_EQ (device->takeGpuFailure().getErrorMessage(), "failure 0");
    EXPECT_TRUE (device->takeGpuFailure().wasOk());
}

TEST_F (GpuFrameTests, FrameThatCouldNotBeSubmittedFails)
{
    device->submitResult = Result::fail ("not submitted");

    auto frame = GpuFrame::begin (device);
    EXPECT_EQ (frame.waitForGPU().getErrorMessage(), "not submitted");
}

//==============================================================================

#if YUP_APPLE
class GpuFrameMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";
    }

    /** Clears a small target in a frame, waits for it and reads the pixels back. */
    Result clearAndRead (GpuColor color)
    {
        auto target = GpuTarget::create (device, 4, 4);
        if (target == nullptr)
            return Result::fail ("No target");

        auto frame = GpuFrame::begin (device);
        if (! frame.isValid())
            return Result::fail ("No frame");

        auto pass = target->beginRenderPass (frame, { true, color });
        pass.finish();
        frame.submit();

        if (auto result = frame.waitForGPU(); result.failed())
            return result;

        std::vector<uint8_t> pixels (4 * 4 * 4);
        if (! target->readPixels (pixels.data(), pixels.size()))
            return Result::fail ("Readback failed");

        return Result::ok();
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuFrameMetalTests, WaitForGpuSucceedsAndNoFailureIsReported)
{
    const auto result = clearAndRead (GpuColor (1.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (device->takeGpuFailure().wasOk());
}

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
#endif
