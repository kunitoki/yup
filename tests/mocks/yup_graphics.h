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

#pragma once

#include <gmock/gmock.h>

#include <yup_graphics/yup_graphics.h>

// ==============================================================================
// Test helper: Delegate GraphicsContext that allows injecting a mock ore context.
//
// Wraps a real (headless) GraphicsContext and delegates all methods to it except
// gpuContext(), which returns the supplied rive::ore::Context*.
// ==============================================================================

class OreInjectedGraphicsContext : public yup::GraphicsContext
{
public:
    explicit OreInjectedGraphicsContext (rive::ore::Context* oreContextToUse)
        : real (yup::GraphicsContext::createContext (yup::GraphicsContext::Headless, {}))
        , injectedOreContext (oreContextToUse)
    {
    }

    yup::GraphicsContext::Api getApi() const noexcept override { return real->getApi(); }

    rive::Factory* factory() override { return real->factory(); }

    rive::gpu::RenderContext* renderContext() override { return real->renderContext(); }

    rive::gpu::RenderTarget* renderTarget() override { return real->renderTarget(); }

    rive::ore::Context* gpuContext() const noexcept override { return injectedOreContext; }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override { return real->makeRenderer (width, height); }

    void onSizeChanged (void* nativeHandle, int width, int height, float dpiScale, uint32_t sampleCount) override { real->onSizeChanged (nativeHandle, width, height, dpiScale, sampleCount); }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& desc) override { real->begin (desc); }

    void end (void* nativeHandle) override { real->end (nativeHandle); }

    std::unique_ptr<yup::OffscreenTarget> createOffscreenTarget (int width, int height) override { return real->createOffscreenTarget (width, height); }

    std::unique_ptr<yup::RenderableTarget> createRenderableTarget (int width, int height) override { return real->createRenderableTarget (width, height); }

    void beginOffscreen (yup::OffscreenTarget& target, const rive::gpu::RenderContext::FrameDescriptor& frameDesc) override { real->beginOffscreen (target, frameDesc); }

    void endOffscreen (yup::OffscreenTarget& target) override { real->endOffscreen (target); }

    bool readOffscreenPixels (yup::OffscreenTarget& target, void* dst, size_t dstSize) override { return real->readOffscreenPixels (target, dst, dstSize); }

private:
    std::unique_ptr<yup::GraphicsContext> real;
    rive::ore::Context* injectedOreContext = nullptr;
};

// ==============================================================================
// Mock yup::GraphicsContext::RenderableTarget
// ==============================================================================

class MockOffscreenTarget : public yup::RenderableTarget
{
public:
    explicit MockOffscreenTarget (int w, int h)
        : width_ (w)
        , height_ (h)
    {
    }

    int getWidth() const noexcept override { return width_; }

    int getHeight() const noexcept override { return height_; }

    rive::gpu::RenderTarget* getRenderTarget() noexcept override { return getRenderTargetProxy(); }

    rive::gpu::RenderContext* getRenderContext() noexcept override { return getRenderContextProxy(); }

    rive::rcp<rive::gpu::RenderCanvas> getRenderCanvas() noexcept override { return getRenderCanvasProxy(); }

    rive::rcp<rive::gpu::Texture> adoptAsTexture() override { return adoptAsTextureProxy(); }

    MOCK_METHOD (rive::gpu::RenderTarget*, getRenderTargetProxy, (), ());
    MOCK_METHOD (rive::gpu::RenderContext*, getRenderContextProxy, (), ());
    MOCK_METHOD (rive::rcp<rive::gpu::RenderCanvas>, getRenderCanvasProxy, (), ());
    MOCK_METHOD (rive::rcp<rive::gpu::Texture>, adoptAsTextureProxy, (), ());

    /** Creates a MockOffscreenTarget pre-configured with a TestGpuTexture for adoptAsTexture. */
    static std::unique_ptr<MockOffscreenTarget> withGpuTexture (int w, int h)
    {
        auto t = std::make_unique<MockOffscreenTarget> (w, h);
        ON_CALL (*t, getRenderCanvasProxy()).WillByDefault (::testing::ReturnNull());
        ON_CALL (*t, adoptAsTextureProxy())
            .WillByDefault (::testing::Return (rive::make_rcp<TestGpuTexture> (w, h)));
        return t;
    }

private:
    int width_;
    int height_;
};

// ==============================================================================
// Test helper: OreInjectedGraphicsContext that also injects an offscreen target.
// Overrides createOffscreenTarget / createRenderableTarget to return a pre-built
// MockOffscreenTarget.
// ==============================================================================

class OreAndTargetGraphicsContext : public OreInjectedGraphicsContext
{
public:
    OreAndTargetGraphicsContext (rive::ore::Context* oreCtx,
                                 std::unique_ptr<MockOffscreenTarget> target)
        : OreInjectedGraphicsContext (oreCtx)
        , injectedTarget (std::move (target))
    {
    }

    std::unique_ptr<yup::OffscreenTarget> createOffscreenTarget (int, int) override
    {
        // Move the target back out — the caller takes ownership.
        // For repeated use, set up the mock to be reusable.
        return std::move (injectedTarget);
    }

    std::unique_ptr<yup::RenderableTarget> createRenderableTarget (int, int) override
    {
        // Move the target back out — the caller takes ownership.
        return std::move (injectedTarget);
    }

    void setNextOffscreenTarget (std::unique_ptr<MockOffscreenTarget> target)
    {
        injectedTarget = std::move (target);
    }

private:
    std::unique_ptr<MockOffscreenTarget> injectedTarget;
};
