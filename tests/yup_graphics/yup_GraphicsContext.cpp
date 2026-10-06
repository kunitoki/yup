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

#include <yup_graphics/yup_graphics.h>

using namespace yup;

class GraphicsContextTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    std::unique_ptr<GraphicsContext> context;
};

TEST_F (GraphicsContextTests, IsGpuAvailableReturnsFalseWhenRenderContextIsNull)
{
    // Headless context returns nullptr for getRenderContext,
    // so isGpuAvailable should return false.
    EXPECT_FALSE (context->isGpuAvailable());
}

TEST_F (GraphicsContextTests, GetPlatformReturnsHeadless)
{
    EXPECT_EQ (context->getPlatform(), GpuPlatform::Headless);
}

TEST_F (GraphicsContextTests, GetFactoryReturnsNonNull)
{
    EXPECT_NE (context->getFactory(), nullptr);
}

TEST_F (GraphicsContextTests, GetRenderContextReturnsNullForHeadless)
{
    EXPECT_EQ (context->getRenderContext(), nullptr);
}

TEST_F (GraphicsContextTests, GetRenderTargetReturnsNullForHeadless)
{
    EXPECT_EQ (context->getRenderTarget(), nullptr);
}

TEST_F (GraphicsContextTests, MakeRendererReturnsNonNull)
{
    auto renderer = context->makeRenderer (200, 200);
    EXPECT_NE (renderer, nullptr);
}

TEST_F (GraphicsContextTests, TickDoesNotCrash)
{
    EXPECT_NO_THROW (context->tick());
}

TEST_F (GraphicsContextTests, SuspendAndResumeFrameDoNotCrashForHeadless)
{
    rive::gpu::RenderContext::FrameDescriptor descriptor;
    descriptor.renderTargetWidth = 64;
    descriptor.renderTargetHeight = 64;

    context->begin (descriptor);
    EXPECT_NO_THROW (context->suspendFrame());
    EXPECT_NO_THROW (context->resumeFrame());
    context->end (nullptr);
}

TEST_F (GraphicsContextTests, CreateContextWithNullExistingDeviceSucceeds)
{
    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {}, nullptr);
    EXPECT_NE (ctx, nullptr);
    EXPECT_EQ (ctx->getPlatform(), GpuPlatform::Headless);
}

TEST_F (GraphicsContextTests, CreateContextWithExistingDeviceSucceeds)
{
    auto existingDevice = GpuDevice::create (GpuPlatform::Headless, {});
    ASSERT_NE (existingDevice, nullptr);

    auto ctx = GraphicsContext::createContext (GpuPlatform::Headless, {}, existingDevice);
    EXPECT_NE (ctx, nullptr);
    EXPECT_EQ (ctx->getPlatform(), GpuPlatform::Headless);
}

TEST (GraphicsContextStaticTests, CreateContextReturnsNullForInvalidApi)
{
    // Cast an out-of-range value to GpuPlatform to hit the default case.
    // Use a high value that won't match any valid enum member.
    const auto invalidApi = static_cast<GpuPlatform> (9999);
    auto ctx = GraphicsContext::createContext (invalidApi, {});
    EXPECT_EQ (ctx, nullptr);
}

//==============================================================================

class GraphicsContextResumeFrameTests : public ::testing::Test
{
protected:
    // Stores the descriptor on begin, as every backend does, and records what it was given.
    class RecordingContext : public GraphicsContext
    {
    public:
        GpuPlatform getPlatform() const noexcept override { return real->getPlatform(); }

        GpuDevice::Ptr getGpuDevice() const noexcept override { return real->getGpuDevice(); }

        rive::Factory* getFactory() override { return real->getFactory(); }

        rive::gpu::RenderContext* getRenderContext() override { return nullptr; }

        rive::gpu::RenderTarget* getRenderTarget() override { return nullptr; }

        std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override { return real->makeRenderer (width, height); }

        void onSizeChanged (void*, int, int, float, uint32_t) override {}

        void begin (const rive::gpu::RenderContext::FrameDescriptor& descriptor) override
        {
            frameDescriptor = descriptor;
            begunFrames.push_back (descriptor);
        }

        void end (void*) override {}

        std::vector<rive::gpu::RenderContext::FrameDescriptor> begunFrames;

    private:
        std::unique_ptr<GraphicsContext> real = GraphicsContext::createContext (GpuPlatform::Headless, {});
    };

    RecordingContext context;
};

TEST_F (GraphicsContextResumeFrameTests, ResumeReopensTheLastFramePreservingItsContent)
{
    rive::gpu::RenderContext::FrameDescriptor descriptor;
    descriptor.renderTargetWidth = 320;
    descriptor.renderTargetHeight = 240;
    descriptor.loadAction = rive::gpu::LoadAction::clear;
    descriptor.clearColor = 0xff336699;
    descriptor.disableRasterOrdering = true;

    context.begin (descriptor);
    context.suspendFrame();
    context.resumeFrame();

    ASSERT_EQ (context.begunFrames.size(), 2u);

    const auto& resumed = context.begunFrames.back();
    EXPECT_EQ (resumed.loadAction, rive::gpu::LoadAction::preserveRenderTarget);
    EXPECT_EQ (resumed.renderTargetWidth, 320u);
    EXPECT_EQ (resumed.renderTargetHeight, 240u);
    EXPECT_EQ (resumed.clearColor, 0xff336699u);
    EXPECT_TRUE (resumed.disableRasterOrdering);
}

TEST_F (GraphicsContextResumeFrameTests, ResumeFollowsTheMostRecentBegin)
{
    rive::gpu::RenderContext::FrameDescriptor first;
    first.renderTargetWidth = 100;
    first.renderTargetHeight = 100;

    rive::gpu::RenderContext::FrameDescriptor second;
    second.renderTargetWidth = 200;
    second.renderTargetHeight = 150;

    context.begin (first);
    context.end (nullptr);
    context.begin (second);
    context.resumeFrame();

    ASSERT_EQ (context.begunFrames.size(), 3u);
    EXPECT_EQ (context.begunFrames.back().renderTargetWidth, 200u);
    EXPECT_EQ (context.begunFrames.back().renderTargetHeight, 150u);
}
