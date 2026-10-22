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
        using GpuDevice::getScopeTimeline;
        using GpuDevice::ScopeTimeline;
        using GpuDevice::setFrameWaiter;
        using GpuDevice::completeFrameStatus;

        bool reportsFrameCompletion() const noexcept override { return reportsCompletion; }

        bool isScopeTimingAvailable() const noexcept override { return scopeTiming; }

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
        bool scopeTiming = false;
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

TEST_F (GpuFrameTests, TimingsReportedByTheBackendReachTheStatus)
{
    device->reportsCompletion = true;
    const auto completeFrame = device->getFrameCompletion();

    auto frame = GpuFrame::begin (device);
    frame.setTimingEnabled (true);
    const auto status = frame.getStatus();
    frame.submit();

    completeFrame (device->getCurrentFrameGeneration(), Result::ok(), { true, 4.5 });

    const auto timings = status.getTimings();
    EXPECT_TRUE (timings.available);
    EXPECT_DOUBLE_EQ (timings.frameMilliseconds, 4.5);
}

TEST_F (GpuFrameTests, FrameWithoutTimingEnabledReportsTimingsUnavailable)
{
    device->reportsCompletion = true;
    const auto completeFrame = device->getFrameCompletion();

    auto frame = GpuFrame::begin (device);
    const auto status = frame.getStatus();
    frame.submit();

    // Too late: the frame was submitted without measuring
    frame.setTimingEnabled (true);

    completeFrame (device->getCurrentFrameGeneration(), Result::ok(), { true, 4.5 });

    ASSERT_TRUE (status.poll().has_value());
    EXPECT_FALSE (status.getTimings().available);
    EXPECT_EQ (status.getTimings().frameMilliseconds, 0.0);
}

TEST_F (GpuFrameTests, FailedFrameReportsTimingsUnavailable)
{
    device->reportsCompletion = true;
    const auto completeFrame = device->getFrameCompletion();

    auto failedOnGpu = GpuFrame::begin (device);
    failedOnGpu.setTimingEnabled (true);
    const auto failedStatus = failedOnGpu.getStatus();
    failedOnGpu.submit();
    completeFrame (device->getCurrentFrameGeneration(), Result::fail ("faulted"), { true, 4.5 });

    device->failNextFrameForTesting ("injected");
    auto injected = GpuFrame::begin (device);
    injected.setTimingEnabled (true);
    const auto injectedStatus = injected.getStatus();
    injected.submit();
    completeFrame (device->getCurrentFrameGeneration(), Result::ok(), { true, 4.5 });

    EXPECT_TRUE (failedStatus.wait().failed());
    EXPECT_FALSE (failedStatus.getTimings().available);
    EXPECT_TRUE (injectedStatus.wait().failed());
    EXPECT_FALSE (injectedStatus.getTimings().available);
}

TEST_F (GpuFrameTests, PendingFrameReportsTimingsUnavailableWithoutBlocking)
{
    device->reportsCompletion = true;

    auto frame = GpuFrame::begin (device);
    frame.setTimingEnabled (true);
    const auto status = frame.getStatus();
    EXPECT_FALSE (status.getTimings().available);

    frame.submit();
    EXPECT_FALSE (status.poll().has_value());
    EXPECT_FALSE (status.getTimings().available);
    EXPECT_FALSE (GpuFrameStatus().getTimings().available);
}

TEST_F (GpuFrameTests, FramesCompletedOutOfOrderKeepTheirOwnTimings)
{
    device->reportsCompletion = true;
    const auto completeFrame = device->getFrameCompletion();

    GpuFrameStatus first, second;
    uint64_t firstGeneration = 0, secondGeneration = 0;
    {
        auto firstFrame = GpuFrame::begin (device);
        firstGeneration = device->getCurrentFrameGeneration();
        auto secondFrame = GpuFrame::begin (device);
        secondGeneration = device->getCurrentFrameGeneration();

        firstFrame.setTimingEnabled (true);
        secondFrame.setTimingEnabled (true);
        first = firstFrame.getStatus();
        second = secondFrame.getStatus();
    }

    // Both frames are gone and still in flight: the later one completes first
    completeFrame (secondGeneration, Result::ok(), { true, 2.0 });
    EXPECT_FALSE (first.getTimings().available);
    EXPECT_DOUBLE_EQ (second.getTimings().frameMilliseconds, 2.0);

    completeFrame (firstGeneration, Result::ok(), { true, 1.0 });
    EXPECT_DOUBLE_EQ (first.getTimings().frameMilliseconds, 1.0);
    EXPECT_DOUBLE_EQ (second.getTimings().frameMilliseconds, 2.0);
}

TEST_F (GpuFrameTests, ScopesAreListedInTheOrderTheyOpened)
{
    device->reportsCompletion = true;
    device->scopeTiming = true;
    const auto completeFrame = device->getFrameCompletion();

    auto frame = GpuFrame::begin (device);
    frame.setTimingEnabled (true);
    frame.beginScope ("a");
    frame.beginScope ("b");
    frame.beginScope ("c");
    frame.beginScope ("a");
    frame.endScope();
    const auto status = frame.getStatus();
    frame.submit();

    // The backend reports the scopes it sampled by index: the last "a" opened no pass
    completeFrame (device->getCurrentFrameGeneration(), Result::ok(), { true, 4.5, { { {}, 1.0, true }, { {}, 2.0, true }, { {}, 3.0, true } } });

    const auto timings = status.getTimings();
    ASSERT_EQ (timings.scopes.size(), 4u);

    const char* names[] = { "a", "b", "c", "a" };
    const double milliseconds[] = { 1.0, 2.0, 3.0, 0.0 };
    for (size_t i = 0; i < timings.scopes.size(); ++i)
    {
        EXPECT_EQ (timings.scopes[i].name, names[i]);
        EXPECT_DOUBLE_EQ (timings.scopes[i].milliseconds, milliseconds[i]);
        EXPECT_TRUE (timings.scopes[i].available);
    }
}

TEST_F (GpuFrameTests, BeginningAScopeEndsTheOpenOne)
{
    device->reportsCompletion = true;
    device->scopeTiming = true;
    const auto completeFrame = device->getFrameCompletion();

    auto frame = GpuFrame::begin (device);
    frame.setTimingEnabled (true);
    frame.beginScope ("outer");
    frame.beginScope ("inner");
    frame.endScope();
    frame.endScope();
    frame.beginScope ("after");
    const auto status = frame.getStatus();
    frame.submit();

    completeFrame (device->getCurrentFrameGeneration(), Result::ok(), { true, 4.5, { { {}, 1.0, true }, { {}, 2.0, true }, { {}, 3.0, true } } });

    const auto timings = status.getTimings();
    ASSERT_EQ (timings.scopes.size(), 3u);
    EXPECT_EQ (timings.scopes[0].name, "outer");
    EXPECT_EQ (timings.scopes[1].name, "inner");
    EXPECT_EQ (timings.scopes[2].name, "after");
    EXPECT_DOUBLE_EQ (timings.scopes[0].milliseconds, 1.0);
}

TEST_F (GpuFrameTests, ScopeTheBackendCouldNotSampleIsUnavailable)
{
    device->reportsCompletion = true;
    device->scopeTiming = true;
    const auto completeFrame = device->getFrameCompletion();

    auto frame = GpuFrame::begin (device);
    frame.setTimingEnabled (true);
    frame.beginScope ("sampled");
    frame.beginScope ("outOfSamples");
    const auto status = frame.getStatus();
    frame.submit();

    completeFrame (device->getCurrentFrameGeneration(), Result::ok(), { true, 4.5, { { {}, 1.0, true }, { {}, 2.0, false } } });

    const auto timings = status.getTimings();
    ASSERT_EQ (timings.scopes.size(), 2u);
    EXPECT_TRUE (timings.scopes[0].available);
    EXPECT_FALSE (timings.scopes[1].available);
    EXPECT_EQ (timings.scopes[1].milliseconds, 0.0);
    EXPECT_DOUBLE_EQ (timings.frameMilliseconds, 4.5);
}

TEST_F (GpuFrameTests, FrameThatDoesNotMeasureScopesReportsNone)
{
    device->reportsCompletion = true;
    const auto completeFrame = device->getFrameCompletion();
    const GpuFrameTimings reported { true, 4.5, { { {}, 1.0, true } } };

    // Timing off, on a device that measures scopes
    device->scopeTiming = true;
    auto unmeasured = GpuFrame::begin (device);
    unmeasured.beginScope ("a");
    const auto unmeasuredStatus = unmeasured.getStatus();
    unmeasured.submit();
    completeFrame (device->getCurrentFrameGeneration(), Result::ok(), reported);

    // Timing on, on a device that measures only frames
    device->scopeTiming = false;
    auto framesOnly = GpuFrame::begin (device);
    framesOnly.setTimingEnabled (true);
    framesOnly.beginScope ("a");
    const auto framesOnlyStatus = framesOnly.getStatus();
    framesOnly.submit();
    completeFrame (device->getCurrentFrameGeneration(), Result::ok(), reported);

    EXPECT_FALSE (unmeasuredStatus.getTimings().available);
    EXPECT_TRUE (unmeasuredStatus.getTimings().scopes.empty());
    EXPECT_TRUE (framesOnlyStatus.getTimings().available);
    EXPECT_TRUE (framesOnlyStatus.getTimings().scopes.empty());
}

TEST_F (GpuFrameTests, FramesCompletedOutOfOrderKeepTheirOwnScopes)
{
    device->reportsCompletion = true;
    device->scopeTiming = true;
    const auto completeFrame = device->getFrameCompletion();

    GpuFrameStatus first, second;
    uint64_t firstGeneration = 0, secondGeneration = 0;
    {
        auto firstFrame = GpuFrame::begin (device);
        firstGeneration = device->getCurrentFrameGeneration();
        auto secondFrame = GpuFrame::begin (device);
        secondGeneration = device->getCurrentFrameGeneration();

        firstFrame.setTimingEnabled (true);
        firstFrame.beginScope ("first");
        secondFrame.setTimingEnabled (true);
        secondFrame.beginScope ("second");
        secondFrame.beginScope ("secondAgain");
        first = firstFrame.getStatus();
        second = secondFrame.getStatus();
    }

    completeFrame (secondGeneration, Result::ok(), { true, 2.0, { { {}, 0.5, true }, { {}, 0.25, true } } });
    EXPECT_TRUE (first.getTimings().scopes.empty());
    ASSERT_EQ (second.getTimings().scopes.size(), 2u);

    completeFrame (firstGeneration, Result::ok(), { true, 1.0, { { {}, 0.75, true } } });

    const auto firstTimings = first.getTimings();
    const auto secondTimings = second.getTimings();
    ASSERT_EQ (firstTimings.scopes.size(), 1u);
    EXPECT_EQ (firstTimings.scopes[0].name, "first");
    EXPECT_DOUBLE_EQ (firstTimings.scopes[0].milliseconds, 0.75);
    ASSERT_EQ (secondTimings.scopes.size(), 2u);
    EXPECT_EQ (secondTimings.scopes[0].name, "second");
    EXPECT_DOUBLE_EQ (secondTimings.scopes[0].milliseconds, 0.5);
    EXPECT_EQ (secondTimings.scopes[1].name, "secondAgain");
    EXPECT_DOUBLE_EQ (secondTimings.scopes[1].milliseconds, 0.25);
}

TEST_F (GpuFrameTests, ScopeTimelineCreditsOnlyTheTimePastEarlierWork)
{
    auto timeline = device->getScopeTimeline();

    // One tick is 1 ms. The first frame was not measured, its work ended at 300
    const auto unmeasured = timeline->commit();
    const auto measured = timeline->commit();
    timeline->advance (unmeasured, 300);

    // Its first pass began under that work, the second under the first
    const auto scopes = timeline->attribute (measured, { { 0, true, 200, 400 }, { 1, true, 350, 450 } }, 1000, 1.0e6);
    ASSERT_EQ (scopes.size(), 2u);
    EXPECT_DOUBLE_EQ (scopes[0].milliseconds, 100.0);
    EXPECT_DOUBLE_EQ (scopes[1].milliseconds, 50.0);
}

TEST_F (GpuFrameTests, ScopeTimelineIgnoresAFrameCompletingOutOfOrder)
{
    auto timeline = device->getScopeTimeline();

    const auto first = timeline->commit();
    const auto second = timeline->commit();

    // The later frame resolves first: the earlier one must not start after its end
    const auto later = timeline->attribute (second, { { 0, true, 100, 200 } }, 1000, 1.0e6);
    const auto earlier = timeline->attribute (first, { { 0, true, 50, 150 } }, 1000, 1.0e6);

    ASSERT_EQ (later.size(), 1u);
    ASSERT_EQ (earlier.size(), 1u);
    EXPECT_DOUBLE_EQ (later[0].milliseconds, 100.0);
    EXPECT_DOUBLE_EQ (earlier[0].milliseconds, 100.0);
    EXPECT_TRUE (earlier[0].available);
}

TEST_F (GpuFrameTests, ScopeTimelineIgnoresWorkEndingAfterNow)
{
    auto timeline = device->getScopeTimeline();

    // GPU timestamps went back between the frames, as after a GPU clock reset
    const auto before = timeline->commit();
    timeline->advance (before, 5000);

    const auto after = timeline->commit();
    const auto scopes = timeline->attribute (after, { { 0, true, 10, 20 } }, 500, 1.0e6);

    ASSERT_EQ (scopes.size(), 1u);
    EXPECT_DOUBLE_EQ (scopes[0].milliseconds, 10.0);
}

TEST_F (GpuFrameTests, ScopeTimelineReportsPassesItCouldNotSampleUnavailable)
{
    auto timeline = device->getScopeTimeline();

    // An unscoped pass, a scope missing a sample and an empty pass
    const auto scopes = timeline->attribute (timeline->commit(), { { -1, true, 0, 100 }, { 0, false, 0, 0 }, { 1, true, 0, 0 } }, 1000, 1.0e6);
    ASSERT_EQ (scopes.size(), 2u);
    EXPECT_FALSE (scopes[0].available);
    EXPECT_EQ (scopes[0].milliseconds, 0.0);
    EXPECT_TRUE (scopes[1].available);
    EXPECT_EQ (scopes[1].milliseconds, 0.0);
}

TEST_F (GpuFrameTests, ScopeTimelineEndTicksCountBackFromNow)
{
    using Timeline = FrameCommandsDevice::ScopeTimeline;

    // Ticks of 2 ns: the work ended 1 ms before now, 500000 ticks before the GPU timestamp of now
    EXPECT_EQ (Timeline::getEndTicks (10'000'000, 5'000'000, 9'000'000.0, 2.0), std::optional<uint64> (4'500'000));
    EXPECT_EQ (Timeline::getEndTicks (10'000'000, 5'000'000, 10'000'000.0, 2.0), std::optional<uint64> (5'000'000));
}

TEST_F (GpuFrameTests, ScopeTimelineEndTicksTakeRoundingPastNowAsNow)
{
    using Timeline = FrameCommandsDevice::ScopeTimeline;

    // Half a microsecond past now is rounding between the clocks
    EXPECT_EQ (Timeline::getEndTicks (10'000'000, 5'000'000, 10'000'500.0, 2.0), std::optional<uint64> (5'000'000));
}

TEST_F (GpuFrameTests, ScopeTimelineEndTicksSkipClocksThatDisagree)
{
    using Timeline = FrameCommandsDevice::ScopeTimeline;

    // An end after now means the clocks disagree: placing it at now would hide the next frame's work
    EXPECT_EQ (Timeline::getEndTicks (10'000'000, 5'000'000, 10'002'000.0, 2.0), std::nullopt);
    EXPECT_EQ (Timeline::getEndTicks (10'000'000, 5'000'000, 10'500'000.0, 2.0), std::nullopt);

    // An end before the GPU timestamps began, or without a tick length, cannot be placed either
    EXPECT_EQ (Timeline::getEndTicks (10'000'000, 5'000'000, 0.0, 2.0), std::nullopt);
    EXPECT_EQ (Timeline::getEndTicks (10'000'000, 5'000'000, 9'000'000.0, 0.0), std::nullopt);
}

TEST_F (GpuFrameTests, DeviceThatDoesNotReportCompletionCannotMeasureFrames)
{
    EXPECT_FALSE (device->isFrameTimingAvailable());

    const auto headless = GpuDevice::create (GpuPlatform::Headless, {});
    ASSERT_NE (headless, nullptr);
    EXPECT_FALSE (headless->isFrameTimingAvailable());

    auto frame = GpuFrame::begin (device);
    frame.setTimingEnabled (true);
    const auto status = frame.getStatus();

    EXPECT_TRUE (frame.waitForGPU().wasOk());
    EXPECT_FALSE (status.getTimings().available);
}

TEST_F (GpuFrameTests, StatusWaitDrivesTheDeviceFrameWaiter)
{
    // A device completing frames only when polled lets a status wait drive it
    device->reportsCompletion = true;

    std::vector<uint64_t> waited;
    device->setFrameWaiter ([this, &waited] (uint64_t generation)
    {
        waited.push_back (generation);
        device->completeFrameStatus (generation, Result::fail ("completed by the waiter"));
    });

    GpuFrameStatus status;
    uint64_t generation = 0;
    {
        auto frame = GpuFrame::begin (device);
        ASSERT_TRUE (frame.isValid());
        generation = device->getCurrentFrameGeneration();
        status = frame.getStatus();
        EXPECT_TRUE (frame.submit());
    }

    EXPECT_EQ (status.wait().getErrorMessage(), "completed by the waiter");
    ASSERT_FALSE (waited.empty());
    EXPECT_EQ (waited.front(), generation);

    device->setFrameWaiter (nullptr);
}

TEST_F (GpuFrameTests, StatusOutlivingTheDeviceStillWaits)
{
    device->reportsCompletion = true;
    device->setFrameWaiter ([] (uint64_t) {});

    GpuFrameStatus status;
    {
        auto frame = GpuFrame::begin (device);
        ASSERT_TRUE (frame.isValid());
        status = frame.getStatus();
        EXPECT_TRUE (frame.submit());
    }

    device = nullptr;

    EXPECT_TRUE (status.wait().failed());
}

//==============================================================================

#if YUP_APPLE || (YUP_ENABLE_SHADER_TRANSPILER && (YUP_APPLE || (YUP_WINDOWS && YUP_RIVE_USE_D3D) || YUP_RIVE_USE_VULKAN || (YUP_EMSCRIPTEN && RIVE_WEBGPU)))
/** Draws, dispatches and measures frames on a real device. */
class GpuFrameDeviceFixture : public GpuReadbackFixture
{
protected:
    /** Creates the device of @p platform, or skips the test when there is none. */
    void openDevice (GpuPlatform platform)
    {
        if (platform == GpuPlatform::Vulkan && ! GpuDevice::isPlatformSupported (GpuPlatform::Vulkan, {}))
            GTEST_SKIP() << "No Vulkan loader or driver in this environment";

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        if (platform == GpuPlatform::WebGPU && EM_ASM_INT ({ return Module.preinitializedWebGPUDevice ? 1 : 0; }) == 0)
            GTEST_SKIP() << "No WebGPU device: run yup_tests.html in a WebGPU browser";
#endif

        device = GpuDevice::create (platform, {});
        if (device == nullptr)
            GTEST_SKIP() << "No device available for this platform";
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

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Draws @p draws blended fullscreen triangles into a 512x512 target in one frame, waits for it
        and returns its timings. The frame turns timing on when @p measure is set, and otherwise
        keeps the default. */
    GpuFrameTimings drawAndMeasure (int draws, bool measure)
    {
        auto frame = GpuFrame::begin (device);
        if (! prepareDrawing() || ! frame.isValid())
        {
            ADD_FAILURE() << "Unable to draw";
            return {};
        }

        if (measure)
            frame.setTimingEnabled (true);

        drawPass (frame, draws);

        const auto status = frame.getStatus();
        const auto submitted = Time::getMillisecondCounterHiRes();
        lastWait = frame.waitForGPU();
        lastWallMilliseconds = Time::getMillisecondCounterHiRes() - submitted;
        return status.getTimings();
    }

    /** Creates the 512x512 target and the blended pipeline drawPass() uses, once. */
    bool prepareDrawing()
    {
        if (target == nullptr)
            target = GpuTarget::create (device, 512, 512);

        if (pipeline == nullptr)
            pipeline = compileRenderPipeline ("#version 450\n"
                                              "layout(location = 0) in vec2 uv;\n"
                                              "layout(location = 0) out vec4 color;\n"
                                              "void main() { color = vec4 (uv, 0.5, 0.5); }\n");

        return target != nullptr && pipeline != nullptr;
    }

    /** Draws @p draws blended fullscreen triangles into the target, in a render pass of their own. */
    void drawPass (GpuFrame& frame, int draws)
    {
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (pipeline);

        for (int i = 0; i < draws; ++i)
            pass.draw (3);
    }

    /** Dispatches a compute shader writing a storage buffer, in a compute pass of its own: each
        invocation iterates @p iterations times. */
    void dispatchPass (GpuFrame& frame, int iterations = 1)
    {
        auto& computePipeline = computePipelines[iterations];
        if (computePipeline == nullptr)
        {
            const auto source = String ("#version 450\n"
                                        "layout(local_size_x = 64) in;\n"
                                        "layout(std430, set = 0, binding = 0) buffer Values { float values[]; };\n"
                                        "void main()\n"
                                        "{\n"
                                        "    float value = float (gl_GlobalInvocationID.x);\n"
                                        "    for (int i = 0; i < ")
                              + String (iterations)
                              + ("; ++i)\n"
                                 "        value = value * 0.999 + 1.0;\n"
                                 "    values[gl_GlobalInvocationID.x] = value;\n"
                                 "}\n");

            auto compiled = GpuComputePipeline::compileFromGlsl (device, source, GpuWorkgroupSize { 64, 1, 1 });
            ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
            computePipeline = compiled.getValue();
        }

        if (computeValues == nullptr)
        {
            const std::vector<float> zeros (64 * 64, 0.0f);
            computeValues = device->createBuffer (GpuBufferType::storage, zeros.data(), zeros.size() * sizeof (float));
            ASSERT_NE (computeValues, nullptr);
        }

        auto pass = GpuComputePass::begin (frame);
        ASSERT_TRUE (pass.isValid());
        pass.setPipeline (computePipeline);
        pass.setStorageBuffer (0, 0, computeValues);
        EXPECT_TRUE (pass.dispatch (64, 1, 1));
    }

    /** Records a frame with @p record, timing on from its start unless @p timingFromStart is
        false, waits for it and returns its timings. */
    template <typename Record>
    GpuFrameTimings measureFrame (Record&& record, bool timingFromStart = true)
    {
        auto frame = GpuFrame::begin (device);
        if (! prepareDrawing() || ! frame.isValid())
        {
            ADD_FAILURE() << "Unable to draw";
            return {};
        }

        if (timingFromStart)
            frame.setTimingEnabled (true);

        record (frame);

        const auto status = frame.getStatus();
        lastWait = frame.waitForGPU();
        return status.getTimings();
    }

    /** The scope named @p name of @p timings, or nullptr. */
    static const GpuScopeTiming* findScope (const GpuFrameTimings& timings, const String& name)
    {
        const auto found = std::find_if (timings.scopes.begin(), timings.scopes.end(), [&] (const auto& scope)
        {
            return scope.name == name;
        });

        return found != timings.scopes.end() ? &*found : nullptr;
    }

    static double median (std::vector<double> values)
    {
        std::sort (values.begin(), values.end());
        return values[values.size() / 2];
    }

    GpuTarget::Ptr target;
    GpuPipeline::Ptr pipeline;
    GpuTarget::Ptr otherTarget;
    GpuPipeline::Ptr otherPipeline;
    std::map<int, GpuComputePipeline::Ptr> computePipelines;
    GpuBuffer::Ptr computeValues;
    Result lastWait = Result::ok();

    /** The CPU wall time of the last drawAndMeasure(), from just before submission to the end of its wait. */
    double lastWallMilliseconds = 0.0;
#endif
};
#endif

//==============================================================================

#if YUP_ENABLE_SHADER_TRANSPILER && (YUP_APPLE || (YUP_WINDOWS && YUP_RIVE_USE_D3D) || YUP_RIVE_USE_VULKAN || (YUP_EMSCRIPTEN && RIVE_WEBGPU))
class GpuFrameBackendTests
    : public GpuFrameDeviceFixture
    , public ::testing::WithParamInterface<GpuPlatform>
{
protected:
    void SetUp() override
    {
        openDevice (GetParam());
    }

    /** The timestamp samples one render pass takes on the tested device. */
    int getSamplesPerRenderPass() const
    {
        return GetParam() == GpuPlatform::Metal ? 4 : 2;
    }
};

INSTANTIATE_TEST_SUITE_P (GpuFrame,
                          GpuFrameBackendTests,
                          ::testing::ValuesIn (GpuComputePassBackendTests::getCompiledPlatforms()),
                          GpuComputePassBackendTests::getTestNameSuffix);

TEST_P (GpuFrameBackendTests, WaitForGpuSucceedsAndNoFailureIsReported)
{
    const auto result = clearAndRead (GpuColor (1.0f, 0.0f, 0.0f, 1.0f));
    EXPECT_TRUE (result.wasOk()) << result.getErrorMessage();
    EXPECT_TRUE (device->takeGpuFailure().wasOk());
}

TEST_P (GpuFrameBackendTests, StatusWaitCompletesWithoutWaitingForTheFrame)
{
    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);

    GpuFrameStatus status;
    {
        auto frame = GpuFrame::begin (device);
        ASSERT_TRUE (frame.isValid());
        status = frame.getStatus();

        auto pass = target->beginRenderPass (frame, { true, GpuColor (0.0f, 0.0f, 1.0f, 1.0f) });
        pass.finish();
        EXPECT_TRUE (frame.submit());
    }

    EXPECT_TRUE (status.wait().wasOk());
}

TEST_P (GpuFrameBackendTests, FrameTimingIsMeasured)
{
    if (GetParam() == GpuPlatform::Metal || GetParam() == GpuPlatform::Vulkan)
        EXPECT_TRUE (device->isFrameTimingAvailable());

    if (! device->isFrameTimingAvailable())
        GTEST_SKIP() << "Frame timing is unavailable on this device";

    const auto timings = drawAndMeasure (200, true);
    EXPECT_TRUE (lastWait.wasOk()) << lastWait.getErrorMessage();
    EXPECT_TRUE (timings.available);
    EXPECT_TRUE (std::isfinite (timings.frameMilliseconds));
    EXPECT_GT (timings.frameMilliseconds, 0.0);

    // The GPU ran the frame between its submission and the end of the wait, so a correct unit never
    // exceeds that wall time. The slack covers rounding between the GPU and CPU clocks, far below
    // the factor 1000 of a wrong unit
    EXPECT_LE (timings.frameMilliseconds, lastWallMilliseconds + 0.05);
}

TEST_P (GpuFrameBackendTests, FrameTimingIsOffByDefault)
{
    const auto timings = drawAndMeasure (200, false);
    EXPECT_TRUE (lastWait.wasOk()) << lastWait.getErrorMessage();
    EXPECT_FALSE (timings.available);
}

TEST_P (GpuFrameBackendTests, HeavierFrameMeasuresLonger)
{
    if (! device->isFrameTimingAvailable())
        GTEST_SKIP() << "Frame timing is unavailable on this device";

    // The first frames compile and warm up, and are not counted
    drawAndMeasure (20, true);
    drawAndMeasure (2000, true);

    std::vector<double> light, heavy;
    for (int i = 0; i < 5; ++i)
    {
        const auto lightTimings = drawAndMeasure (20, true);
        const auto heavyTimings = drawAndMeasure (2000, true);
        ASSERT_TRUE (lightTimings.available && heavyTimings.available);

        light.push_back (lightTimings.frameMilliseconds);
        heavy.push_back (heavyTimings.frameMilliseconds);
    }

    EXPECT_GT (median (heavy), median (light));
}

TEST_P (GpuFrameBackendTests, FailedFrameReportsUnavailable)
{
    device->failNextFrameForTesting ("injected");

    const auto timings = drawAndMeasure (20, true);
    EXPECT_EQ (lastWait.getErrorMessage(), "injected");
    EXPECT_FALSE (timings.available);
}

TEST_P (GpuFrameBackendTests, ScopesAreReportedInOrder)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    // Each scope ends the one before it
    const auto timings = measureFrame ([this] (GpuFrame& frame)
    {
        frame.beginScope ("a");
        drawPass (frame, 20);
        frame.beginScope ("b");
        drawPass (frame, 20);
        frame.beginScope ("c");
        drawPass (frame, 20);
    });

    EXPECT_TRUE (lastWait.wasOk()) << lastWait.getErrorMessage();
    ASSERT_TRUE (timings.available);
    ASSERT_EQ (timings.scopes.size(), 3u);

    const char* names[] = { "a", "b", "c" };
    for (size_t i = 0; i < timings.scopes.size(); ++i)
    {
        EXPECT_EQ (timings.scopes[i].name, names[i]);
        EXPECT_TRUE (timings.scopes[i].available);
        EXPECT_TRUE (std::isfinite (timings.scopes[i].milliseconds));
        EXPECT_GE (timings.scopes[i].milliseconds, 0.0);
    }
}

TEST_P (GpuFrameBackendTests, HeavierScopeMeasuresLonger)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    const auto lightAndHeavy = [this] (GpuFrame& frame)
    {
        frame.beginScope ("light");
        drawPass (frame, 10);
        frame.beginScope ("heavy");
        drawPass (frame, 2000);
        frame.endScope();
    };

    // The first frame compiles and warms up, and is not counted
    measureFrame (lightAndHeavy);

    std::vector<double> light, heavy;
    for (int i = 0; i < 5; ++i)
    {
        const auto timings = measureFrame (lightAndHeavy);
        ASSERT_EQ (timings.scopes.size(), 2u);
        ASSERT_TRUE (timings.scopes[0].available && timings.scopes[1].available);

        light.push_back (timings.scopes[0].milliseconds);
        heavy.push_back (timings.scopes[1].milliseconds);
    }

    EXPECT_GT (median (heavy), median (light));
}

TEST_P (GpuFrameBackendTests, ScopeSumsItsEncoders)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    // The same work twice: three scopes of one encoder each, then one scope of all three encoders.
    // The two render parts weigh about the same, so a scope counting one encoder only reads about
    // half of the whole
    const auto separate = [this] (GpuFrame& frame)
    {
        frame.beginScope ("first");
        drawPass (frame, 500);
        frame.beginScope ("compute");
        dispatchPass (frame);
        frame.beginScope ("last");
        drawPass (frame, 500);
        frame.endScope();
    };

    const auto together = [this] (GpuFrame& frame)
    {
        frame.beginScope ("all");
        drawPass (frame, 500);
        dispatchPass (frame);
        drawPass (frame, 500);
        frame.endScope();
    };

    measureFrame (separate);
    measureFrame (together);

    std::vector<double> first, compute, last, all;
    for (int i = 0; i < 5; ++i)
    {
        const auto parts = measureFrame (separate);
        const auto whole = measureFrame (together);
        ASSERT_EQ (parts.scopes.size(), 3u);
        ASSERT_EQ (whole.scopes.size(), 1u);
        ASSERT_TRUE (whole.scopes[0].available);

        first.push_back (parts.scopes[0].milliseconds);
        compute.push_back (parts.scopes[1].milliseconds);
        last.push_back (parts.scopes[2].milliseconds);
        all.push_back (whole.scopes[0].milliseconds);
    }

    EXPECT_GE (median (all), median (compute));
    EXPECT_GE (median (all), 1.5 * jmax (median (first), median (last)));
}

TEST_P (GpuFrameBackendTests, ComputeScopeIsMeasured)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    // The dispatch is the frame's only pass, so no earlier pass absorbs it
    const auto timings = measureFrame ([this] (GpuFrame& frame)
    {
        frame.beginScope ("compute");
        dispatchPass (frame, 4000);
        frame.endScope();
    });

    EXPECT_TRUE (lastWait.wasOk()) << lastWait.getErrorMessage();
    ASSERT_EQ (timings.scopes.size(), 1u);
    EXPECT_TRUE (timings.scopes[0].available);
    EXPECT_GT (timings.scopes[0].milliseconds, 0.0);
}

TEST_P (GpuFrameBackendTests, FirstScopeIsNotChargedForTheUnmeasuredFrameBeforeIt)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    const auto heavyFrame = [this] (GpuFrame& frame)
    {
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (pipeline);
        pass.draw (3, 1500);
    };

    const auto lightScope = [this] (GpuFrame& frame)
    {
        frame.beginScope ("light");
        drawPass (frame, 100);
    };

    // The heavy work measured alone, which also starts the device measuring
    const auto heavy = measureFrame ([&] (GpuFrame& frame)
    {
        frame.beginScope ("heavy");
        heavyFrame (frame);
    });
    ASSERT_EQ (heavy.scopes.size(), 1u);

    // The light scope measured alone, then right after a heavy frame that does not measure: the
    // earlier work must not be charged to it. GPUEndTime lags the last sample by a few hundredths
    // of a millisecond, so the scope may read somewhat less after the heavy frame, but never 0
    // (ScopeTimelineEndTicks* pin the placement of the frame's end exactly)
    std::vector<double> alone, afterHeavy;
    for (int i = 0; i < 3; ++i)
    {
        const auto single = measureFrame (lightScope);
        ASSERT_EQ (single.scopes.size(), 1u);
        ASSERT_TRUE (single.scopes[0].available);
        alone.push_back (single.scopes[0].milliseconds);

        {
            auto unmeasured = GpuFrame::begin (device);
            ASSERT_TRUE (unmeasured.isValid());
            heavyFrame (unmeasured);
        }

        const auto timings = measureFrame (lightScope);
        ASSERT_EQ (timings.scopes.size(), 1u);
        ASSERT_TRUE (timings.scopes[0].available);
        afterHeavy.push_back (timings.scopes[0].milliseconds);
    }

    RecordProperty ("heavy", String (heavy.scopes[0].milliseconds, 3).toStdString());
    RecordProperty ("lightAlone", String (median (alone), 3).toStdString());
    RecordProperty ("lightAfterHeavy", String (median (afterHeavy), 3).toStdString());

    ASSERT_GT (median (alone), 0.0);
    EXPECT_LT (median (alone), heavy.scopes[0].milliseconds * 0.5);
    EXPECT_GT (median (afterHeavy), median (alone) * 0.2);
    EXPECT_LT (median (afterHeavy), median (alone) * 2.0);
}

TEST_P (GpuFrameBackendTests, UnscopedWorkIsNotAttributed)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    // On a tile-based GPU the light pass begins its vertex stage while the heavy one before it
    // still shades: only the time past the end of the earlier passes belongs to it
    const auto timings = measureFrame ([this] (GpuFrame& frame)
    {
        drawPass (frame, 2000);
        frame.beginScope ("scoped");
        drawPass (frame, 10);
        frame.endScope();
        drawPass (frame, 2000);
    });

    ASSERT_TRUE (timings.available);
    ASSERT_EQ (timings.scopes.size(), 1u);
    EXPECT_EQ (timings.scopes[0].name, "scoped");
    EXPECT_TRUE (timings.scopes[0].available);
    EXPECT_LT (timings.scopes[0].milliseconds, timings.frameMilliseconds * 0.1);
}

TEST_P (GpuFrameBackendTests, QueuedFrameIsNotChargedForTheFrameBeforeIt)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    ASSERT_TRUE (prepareDrawing());

    // Queued back to back, a frame's first pass begins while the heavy last pass of the frame
    // before it still shades
    std::vector<GpuFrameStatus> statuses;
    for (int i = 0; i < 6; ++i)
    {
        auto frame = GpuFrame::begin (device);
        ASSERT_TRUE (frame.isValid());
        frame.setTimingEnabled (true);
        statuses.push_back (frame.getStatus());

        frame.beginScope ("light");
        drawPass (frame, 5);

        frame.beginScope ("heavy");
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (pipeline);
        pass.draw (3, 1500);
    }

    std::vector<double> light, heavy;
    for (size_t i = 1; i < statuses.size(); ++i)
    {
        EXPECT_TRUE (statuses[i].wait().wasOk());

        const auto timings = statuses[i].getTimings();
        ASSERT_EQ (timings.scopes.size(), 2u);
        ASSERT_TRUE (timings.scopes[0].available && timings.scopes[1].available);

        light.push_back (timings.scopes[0].milliseconds);
        heavy.push_back (timings.scopes[1].milliseconds);
    }

    EXPECT_LT (median (light), median (heavy) * 0.1);
}

TEST_P (GpuFrameBackendTests, ScopeOpenedBeforeTimingWasEnabledReportsNothing)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    const auto timings = measureFrame ([this] (GpuFrame& frame)
    {
        frame.beginScope ("early");
        drawPass (frame, 200);

        frame.setTimingEnabled (true);
        drawPass (frame, 200);

        frame.beginScope ("late");
        drawPass (frame, 200);
    },
                                       false);

    ASSERT_TRUE (timings.available);
    ASSERT_EQ (timings.scopes.size(), 2u);

    const auto* early = findScope (timings, "early");
    const auto* late = findScope (timings, "late");
    ASSERT_TRUE (early != nullptr && late != nullptr);
    EXPECT_FALSE (early->available);
    EXPECT_EQ (early->milliseconds, 0.0);
    EXPECT_TRUE (late->available);
    EXPECT_GT (late->milliseconds, 0.0);
}

TEST_P (GpuFrameBackendTests, RunningOutOfSamplesKeepsTheFrameTotal)
{
    if (! device->isScopeTimingAvailable())
        GTEST_SKIP() << "Scope timing is unavailable on this device";

    // Room for one render pass only
    device->setScopeSampleCapacityForTesting (getSamplesPerRenderPass());

    const auto timings = measureFrame ([this] (GpuFrame& frame)
    {
        frame.beginScope ("sampled");
        drawPass (frame, 200);
        frame.beginScope ("outOfSamples");
        drawPass (frame, 200);
        frame.beginScope ("empty");
        frame.endScope();
    });

    device->setScopeSampleCapacityForTesting (0);

    EXPECT_TRUE (lastWait.wasOk()) << lastWait.getErrorMessage();
    ASSERT_TRUE (timings.available);
    EXPECT_GT (timings.frameMilliseconds, 0.0);
    ASSERT_EQ (timings.scopes.size(), 3u);
    EXPECT_TRUE (timings.scopes[0].available);
    EXPECT_GT (timings.scopes[0].milliseconds, 0.0);
    EXPECT_FALSE (timings.scopes[1].available);
    EXPECT_EQ (timings.scopes[1].milliseconds, 0.0);
    EXPECT_TRUE (timings.scopes[2].available);
    EXPECT_EQ (timings.scopes[2].milliseconds, 0.0);
}
#endif
