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

#include <rive/renderer/ore/ore_context_metal.hpp>
#include <renderer/ore/metal/ore_buffer_metal.hpp>

#import <Metal/Metal.h>

#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

using namespace yup;

//==============================================================================
/** Drives the ore Metal context of a real device directly: the backing a buffer
    binds is only observable through rive::ore::BufferMetal::current().
*/
class GpuOreBufferMetalTests : public ::testing::Test
{
protected:
    static constexpr uint32_t blockSize = 256;

    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";

        context = static_cast<rive::ore::ContextMetal*> (device->getGpuContext());
        queue = [context->device() newCommandQueue];
    }

    void TearDown() override
    {
#if ! __has_feature(objc_arc)
        [queue release];
#endif
        queue = nil;
    }

    /** Returns an object created by a new* method, autoreleased when ARC is off. */
    template <typename T>
    static T owned (T object)
    {
#if ! __has_feature(objc_arc)
        [object autorelease];
#endif
        return object;
    }

    rive::rcp<rive::ore::BufferMetal> makeBuffer (uint32_t size)
    {
        rive::ore::BufferDesc desc;
        desc.usage = rive::ore::BufferUsage::uniform;
        desc.size = size;
        return rive::static_rcp_cast<rive::ore::BufferMetal> (context->makeBuffer (desc));
    }

    id<MTLBuffer> makeReadback (uint32_t size)
    {
        return owned ([context->device() newBufferWithLength:size options:MTLResourceStorageModeShared]);
    }

    /** Begins an ore frame recording into a command buffer the test commits itself. */
    id<MTLCommandBuffer> beginFrame (id<MTLSharedEvent> waitEvent = nil)
    {
        id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
        if (waitEvent != nil)
            [commandBuffer encodeWaitForEvent:waitEvent value:1];

        rive::ore::Context::FrameDescriptor desc {};
        desc.externalCommandBuffer = (__bridge void*) commandBuffer;
        context->beginFrame (desc);
        return commandBuffer;
    }

    void endFrame (id<MTLCommandBuffer> commandBuffer)
    {
        context->endFrame();
        [commandBuffer commit];
    }

    /** Waits until ore has published the completion of every frame up to serial. */
    bool waitForCompletion (id<MTLCommandBuffer> commandBuffer, uint64_t serial)
    {
        [commandBuffer waitUntilCompleted];

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds (5);
        while (context->completedSerial() < serial)
        {
            if (std::chrono::steady_clock::now() > deadline)
                return false;

            std::this_thread::sleep_for (std::chrono::milliseconds (1));
        }

        return true;
    }

    static void copyBlock (id<MTLCommandBuffer> commandBuffer, id<MTLBuffer> source, id<MTLBuffer> destination, uint32_t offset)
    {
        id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
        [blit copyFromBuffer:source sourceOffset:offset toBuffer:destination destinationOffset:offset size:blockSize];
        [blit endEncoding];
    }

    static std::vector<uint8> makeBlock (uint8 value)
    {
        return std::vector<uint8> (blockSize, value);
    }

    static bool blockEquals (id<MTLBuffer> buffer, uint32_t offset, const std::vector<uint8>& expected)
    {
        return std::memcmp (static_cast<const uint8*> ([buffer contents]) + offset, expected.data(), expected.size()) == 0;
    }

    /** Releases a frame blocked on an event, even when a test returns early. */
    struct EventSignaller
    {
        ~EventSignaller()
        {
            event.signaledValue = 1;
            [commandBuffer waitUntilCompleted];
        }

        id<MTLSharedEvent> event;
        id<MTLCommandBuffer> commandBuffer;
    };

    GpuDevice::Ptr device;
    rive::ore::ContextMetal* context = nullptr;
    id<MTLCommandQueue> queue = nil;
};

TEST_F (GpuOreBufferMetalTests, WritesAfterABindStayOnTheBoundBacking)
{
    @autoreleasepool
    {
        constexpr uint32_t blockCount = 4;
        auto buffer = makeBuffer (blockSize * blockCount);
        ASSERT_NE (buffer, nullptr);
        id<MTLBuffer> readback = makeReadback (blockSize * blockCount);

        auto commandBuffer = beginFrame();
        id<MTLBuffer> firstBacking = nil;
        for (uint32_t i = 0; i < blockCount; ++i)
        {
            const auto block = makeBlock ((uint8) (10 + i));
            buffer->writeUnsynchronized (block.data(), blockSize, i * blockSize, i == 0);
            buffer->markBound();

            if (i == 0)
                firstBacking = buffer->current();

            EXPECT_EQ (buffer->current(), firstBacking) << "block " << i;
            copyBlock (commandBuffer, buffer->current(), readback, i * blockSize);
        }
        const auto serial = context->currentSerial();
        endFrame (commandBuffer);
        ASSERT_TRUE (waitForCompletion (commandBuffer, serial));

        for (uint32_t i = 0; i < blockCount; ++i)
            EXPECT_TRUE (blockEquals (readback, i * blockSize, makeBlock ((uint8) (10 + i)))) << "block " << i;
    }
}

TEST_F (GpuOreBufferMetalTests, DiscardMovesOffABackingAFrameStillReads)
{
    @autoreleasepool
    {
        auto buffer = makeBuffer (blockSize);
        ASSERT_NE (buffer, nullptr);
        id<MTLBuffer> readback = makeReadback (blockSize);
        id<MTLSharedEvent> event = owned ([context->device() newSharedEvent]);

        const auto submitted = makeBlock (1);
        buffer->writeUnsynchronized (submitted.data(), blockSize, 0, true);

        // The GPU cannot read the block before the event fires, which happens after the discard
        auto blockedFrame = beginFrame (event);
        EventSignaller signaller { event, blockedFrame };
        buffer->markBound();
        id<MTLBuffer> boundBacking = buffer->current();
        copyBlock (blockedFrame, boundBacking, readback, 0);
        const auto blockedSerial = context->currentSerial();
        endFrame (blockedFrame);

        auto nextFrame = beginFrame();
        const auto next = makeBlock (2);
        buffer->writeUnsynchronized (next.data(), blockSize, 0, true);
        EXPECT_NE (buffer->current(), boundBacking);
        EXPECT_TRUE (blockEquals (buffer->current(), 0, next));
        const auto nextSerial = context->currentSerial();
        endFrame (nextFrame);

        event.signaledValue = 1;
        ASSERT_TRUE (waitForCompletion (blockedFrame, blockedSerial));
        ASSERT_TRUE (waitForCompletion (nextFrame, nextSerial));

        EXPECT_TRUE (blockEquals (readback, 0, submitted));
    }
}

TEST_F (GpuOreBufferMetalTests, DiscardKeepsABackingTheGpuHasFinished)
{
    @autoreleasepool
    {
        auto buffer = makeBuffer (blockSize);
        ASSERT_NE (buffer, nullptr);

        auto frame = beginFrame();
        buffer->markBound();
        id<MTLBuffer> boundBacking = buffer->current();
        const auto serial = context->currentSerial();
        endFrame (frame);
        ASSERT_TRUE (waitForCompletion (frame, serial));

        const auto block = makeBlock (3);
        buffer->writeUnsynchronized (block.data(), blockSize, 0, true);

        EXPECT_EQ (buffer->current(), boundBacking);
        EXPECT_TRUE (blockEquals (buffer->current(), 0, block));
    }
}

TEST_F (GpuOreBufferMetalTests, UpdateAfterAnUnsynchronizedWriteStillOrphans)
{
    @autoreleasepool
    {
        auto buffer = makeBuffer (blockSize * 2);
        ASSERT_NE (buffer, nullptr);

        auto frame = beginFrame();
        buffer->markBound();
        id<MTLBuffer> boundBacking = buffer->current();

        const auto written = makeBlock (4);
        buffer->writeUnsynchronized (written.data(), blockSize, blockSize, false);
        EXPECT_EQ (buffer->current(), boundBacking);

        const auto updated = makeBlock (5);
        buffer->update (updated.data(), blockSize, 0);
        EXPECT_NE (buffer->current(), boundBacking);
        EXPECT_TRUE (blockEquals (buffer->current(), 0, updated));
        EXPECT_TRUE (blockEquals (buffer->current(), blockSize, written));

        const auto serial = context->currentSerial();
        endFrame (frame);
        ASSERT_TRUE (waitForCompletion (frame, serial));
    }
}
