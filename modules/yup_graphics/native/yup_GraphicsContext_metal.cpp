/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#if YUP_RIVE_USE_METAL
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/metal/render_context_metal_impl.h"

namespace yup
{

class LowLevelRenderContextMetal : public GraphicsContext
{
public:
    LowLevelRenderContextMetal (Options fiddleOptions)
        : m_fiddleOptions (fiddleOptions)
    {
        rive::gpu::RenderContextMetalImpl::ContextOptions metalOptions;
        if (m_fiddleOptions.synchronousShaderCompilations)
            metalOptions.synchronousShaderCompilations = true;

        if (m_fiddleOptions.disableRasterOrdering)
            metalOptions.disableFramebufferReads = true;

        m_plsContext = rive::gpu::RenderContextMetalImpl::MakeContext (m_gpu, metalOptions);
        printf ("==== MTLDevice: %s ====\n", m_gpu.name.UTF8String);
    }

    float dpiScale (void* window) const override
    {
        NSWindow* nsWindow = (__bridge NSWindow*) window;
        return m_fiddleOptions.retinaDisplay ? nsWindow.backingScaleFactor : 1.0f;
    }

    rive::Factory* factory() override { return m_plsContext.get(); }

    rive::gpu::RenderContext* plsContextOrNull() override { return m_plsContext.get(); }

    rive::gpu::RenderTarget* plsRenderTargetOrNull() override { return m_renderTarget.get(); }

    void onSizeChanged (void* window, int width, int height, uint32_t sampleCount) override
    {
        NSWindow* nsWindow = (__bridge NSWindow*) window;
        NSView* view = [nsWindow contentView];
        view.wantsLayer = YES;

        m_swapchain = [CAMetalLayer layer];
        m_swapchain.device = m_gpu;
        m_swapchain.opaque = YES;
        m_swapchain.framebufferOnly = ! m_fiddleOptions.readableFramebuffer;
        m_swapchain.pixelFormat = MTLPixelFormatBGRA8Unorm;
        m_swapchain.contentsScale = dpiScale (window);
        m_swapchain.displaySyncEnabled = YES;
        m_swapchain.maximumDrawableCount = 2;
        view.layer = m_swapchain;

        auto plsContextImpl = m_plsContext->static_impl_cast<rive::gpu::RenderContextMetalImpl>();
        m_renderTarget = plsContextImpl->makeRenderTarget (MTLPixelFormatBGRA8Unorm, width, height);
    }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (m_plsContext.get());
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        m_plsContext->beginFrame (frameDescriptor);
    }

    void end (void*) override
    {
        if (m_currentFrameSurface == nil)
        {
            m_currentFrameSurface = [m_swapchain nextDrawable];
            assert (m_currentFrameSurface.texture.width == m_renderTarget->width());
            assert (m_currentFrameSurface.texture.height == m_renderTarget->height());
            m_renderTarget->setTargetTexture (m_currentFrameSurface.texture);
        }

        id<MTLCommandBuffer> flushCommandBuffer = [m_queue commandBuffer];
        m_plsContext->flush ({ .renderTarget = m_renderTarget.get(), .externalCommandBuffer = (__bridge void*) flushCommandBuffer });
        [flushCommandBuffer commit];

        id<MTLCommandBuffer> presentCommandBuffer = [m_queue commandBuffer];
        [presentCommandBuffer presentDrawable:m_currentFrameSurface];
        [presentCommandBuffer commit];

        m_currentFrameSurface = nil;
        m_renderTarget->setTargetTexture (nil);
    }

private:
    const Options m_fiddleOptions;
    id<MTLDevice> m_gpu = MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> m_queue = [m_gpu newCommandQueue];
    std::unique_ptr<rive::gpu::RenderContext> m_plsContext;
    CAMetalLayer* m_swapchain = nil;
    rive::rcp<rive::gpu::RenderTargetMetal> m_renderTarget;
    id<CAMetalDrawable> m_currentFrameSurface = nil;
};

std::unique_ptr<GraphicsContext> juce_constructMetalGraphicsContext (GraphicsContext::Options fiddleOptions)
{
    return std::make_unique<LowLevelRenderContextMetal> (fiddleOptions);
}

} // namespace yup
#endif
