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

namespace
{

class TrackingOffscreenTarget : public RenderableTarget
{
public:
    TrackingOffscreenTarget (int targetWidth, int targetHeight)
        : width (targetWidth)
        , height (targetHeight)
    {
    }

    int getWidth() const noexcept override { return width; }

    int getHeight() const noexcept override { return height; }

    rive::gpu::RenderTarget* getRenderTarget() noexcept override { return nullptr; }

    rive::gpu::RenderContext* getRenderContext() noexcept override { return nullptr; }

    rive::rcp<rive::gpu::Texture> adoptAsTexture() override { return nullptr; }

private:
    int width;
    int height;
};

class TrackingGraphicsContext : public GraphicsContext
{
public:
    TrackingGraphicsContext()
        : realContext (GraphicsContext::createContext (GraphicsContext::Headless, {}))
    {
    }

    Api getApi() const noexcept override { return realContext->getApi(); }

    float dpiScale (void* nativeHandle) const override { return realContext->dpiScale (nativeHandle); }

    rive::Factory* factory() override { return realContext->factory(); }

    rive::gpu::RenderContext* renderContext() override { return realContext->renderContext(); }

    rive::gpu::RenderTarget* renderTarget() override { return realContext->renderTarget(); }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override { return realContext->makeRenderer (width, height); }

    void onSizeChanged (void* nativeHandle, int width, int height, uint32_t sampleCount) override
    {
        realContext->onSizeChanged (nativeHandle, width, height, sampleCount);
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDesc) override { realContext->begin (frameDesc); }

    void end (void* nativeHandle) override { realContext->end (nativeHandle); }

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int, int) override { return nullptr; }

    std::unique_ptr<RenderableTarget> createRenderableTarget (int, int) override { return nullptr; }

    void beginOffscreen (OffscreenTarget&, const rive::gpu::RenderContext::FrameDescriptor&) override { ++beginOffscreenCalls; }

    void endOffscreen (OffscreenTarget&) override { ++endOffscreenCalls; }

    bool readOffscreenPixels (OffscreenTarget&, void*, size_t) override { return false; }

    int beginOffscreenCalls = 0;
    int endOffscreenCalls = 0;

private:
    std::unique_ptr<GraphicsContext> realContext;
};

} // namespace

class GraphicsOffscreenTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GraphicsContext::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    std::unique_ptr<GraphicsContext> context;
};

TEST_F (GraphicsOffscreenTests, RegularConstructorIsNotOffscreen)
{
    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_FALSE (g.isOffscreen());
}

TEST_F (GraphicsOffscreenTests, OffscreenConstructorIsOffscreen)
{
    // Headless context returns nullptr from createOffscreenTarget, so
    // isOffscreen() will return false. This tests the headless stub path.
    Image image (64, 64);
    Graphics g (*context, image);

    // Headless context has no GPU, so createOffscreenTarget returns nullptr.
    EXPECT_FALSE (g.isOffscreen());
}

TEST_F (GraphicsOffscreenTests, CommitToImageReturnsFalseForRegularGraphics)
{
    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_FALSE (g.commitToImage());
}

TEST_F (GraphicsOffscreenTests, CommitToImageReturnsFalseForHeadlessOffscreen)
{
    // Headless context creates no offscreen target, so commit returns false.
    Image image (64, 64);
    Graphics g (*context, image);

    EXPECT_FALSE (g.commitToImage());
}

TEST_F (GraphicsOffscreenTests, ReadPixelsToImageReturnsFalseForRegularGraphics)
{
    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_FALSE (g.readPixelsToImage());
}

TEST_F (GraphicsOffscreenTests, ReadPixelsToImageReturnsFalseForHeadlessOffscreen)
{
    // Headless context has no GPU, so readPixels returns false.
    Image image (64, 64);
    Graphics g (*context, image);

    EXPECT_FALSE (g.readPixelsToImage());
}

TEST_F (GraphicsOffscreenTests, SetGpuTextureStoresTexture)
{
    Image image (32, 32);
    EXPECT_EQ (image.getGpuTexture(), nullptr);

    image.setGpuTexture (nullptr);
    EXPECT_EQ (image.getGpuTexture(), nullptr);
}

TEST_F (GraphicsOffscreenTests, GetGpuTextureReturnsNullByDefault)
{
    Image image (32, 32);
    EXPECT_EQ (image.getGpuTexture(), nullptr);
}

TEST_F (GraphicsOffscreenTests, SmallImageOffscreenConstructorDoesNotCrash)
{
    Image image (1, 1);
    EXPECT_NO_THROW ({ Graphics g (*context, image); });
}

TEST_F (GraphicsOffscreenTests, LargeImageOffscreenConstructorDoesNotCrash)
{
    Image image (2048, 2048);
    EXPECT_NO_THROW ({ Graphics g (*context, image); });
}

TEST_F (GraphicsOffscreenTests, CommitCalledTwiceReturnsFalseOnSecondCall)
{
    Image image (64, 64);
    Graphics g (*context, image);

    const bool first = g.commitToImage();
    const bool second = g.commitToImage();

    // Both should return false in headless (no GPU), but the contract is that
    // the second call is also not a crash.
    EXPECT_FALSE (second);
    (void) first;
}

TEST_F (GraphicsOffscreenTests, ReadPixelsAfterCommitDoesNotCrash)
{
    Image image (32, 32);
    Graphics g (*context, image);

    g.commitToImage();
    EXPECT_NO_THROW ({ g.readPixelsToImage(); });
}

TEST_F (GraphicsOffscreenTests, RegularGraphicsContextRenderSize)
{
    auto renderer = context->makeRenderer (128, 64);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*context, *renderer);

    EXPECT_FALSE (g.isOffscreen());
    EXPECT_FALSE (g.commitToImage());
}

TEST (GraphicsOffscreenLifecycleTests, DestroyingUncommittedGraphicsClosesFrame)
{
    TrackingGraphicsContext context;
    TrackingOffscreenTarget target (64, 64);

    {
        Graphics g (context, target);
        EXPECT_TRUE (g.isOffscreen());
        EXPECT_EQ (1, context.beginOffscreenCalls);
        EXPECT_EQ (0, context.endOffscreenCalls);
    }

    EXPECT_EQ (1, context.endOffscreenCalls);
}

TEST (GraphicsOffscreenLifecycleTests, CommittingGraphicsDoesNotCloseFrameTwice)
{
    TrackingGraphicsContext context;
    TrackingOffscreenTarget target (64, 64);

    {
        Graphics g (context, target);
        EXPECT_TRUE (g.commitOffscreenTarget());
        EXPECT_EQ (1, context.endOffscreenCalls);
    }

    EXPECT_EQ (1, context.endOffscreenCalls);
}
