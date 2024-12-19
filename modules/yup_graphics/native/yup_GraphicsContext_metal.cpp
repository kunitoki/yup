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

#include <TargetConditionals.h>

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
    }

    float dpiScale (void* window) const override
    {
#if TARGET_OS_IOS
        UIWindow* uiWindow = (__bridge UIWindow*) window;
        UIScreen* screen = [uiWindow screen] ?: [UIScreen mainScreen];
        return screen.nativeScale;
#else
        NSWindow* nsWindow = (__bridge NSWindow*) window;
        return m_fiddleOptions.retinaDisplay ? nsWindow.backingScaleFactor : 1.0f;
#endif
    }

    rive::Factory* factory() override { return m_plsContext.get(); }

    rive::gpu::RenderContext* renderContextOrNull() override { return m_plsContext.get(); }

    rive::gpu::RenderTarget* renderTargetOrNull() override { return m_renderTarget.get(); }

    void onSizeChanged (void* window, int width, int height, uint32_t sampleCount) override
    {
#if ! TARGET_OS_IOS
        NSWindow* nsWindow = (__bridge NSWindow*) window;
        NSView* view = [nsWindow contentView];
        view.wantsLayer = YES;
#endif

        m_swapchain = [CAMetalLayer layer];
        m_swapchain.device = m_gpu;
        m_swapchain.opaque = YES;
        m_swapchain.framebufferOnly = ! m_fiddleOptions.readableFramebuffer;
        m_swapchain.pixelFormat = MTLPixelFormatBGRA8Unorm;
        m_swapchain.contentsScale = dpiScale (window);
        m_swapchain.maximumDrawableCount = 2;
#if ! TARGET_OS_IOS
        m_swapchain.displaySyncEnabled = YES;
#endif

#if TARGET_OS_IOS
        UIView* view = (__bridge UIView*) window;
        m_swapchain.frame = view.bounds;
        [view.layer addSublayer:m_swapchain];
#else
        view.layer = m_swapchain;
#endif

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
        jassert (m_currentFrameSurface == nil);
        m_currentFrameSurface = [m_swapchain nextDrawable];
        jassert (m_currentFrameSurface.texture.width == m_renderTarget->width());
        jassert (m_currentFrameSurface.texture.height == m_renderTarget->height());
        m_renderTarget->setTargetTexture (m_currentFrameSurface.texture);

        id<MTLCommandBuffer> commandBuffer = [m_queue commandBuffer];
        m_plsContext->flush ({ .renderTarget = m_renderTarget.get(), .externalCommandBuffer = (__bridge void*) commandBuffer });
        [commandBuffer presentDrawable:m_currentFrameSurface];
        [commandBuffer commit];

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
