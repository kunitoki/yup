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

/*
  ==============================================================================

   Native WebGPU GraphicsContext backend for Emscripten.

   Renders Rive content through the browser's native WebGPU API using
   emscripten's built-in bindings, without Dawn or wagyu.

   Requirements:
   - Build with `-sUSE_WEBGPU=1` (legacy emscripten WebGPU bindings) and define
     `RIVE_WEBGPU=1` so rive's `RenderContextWebGPUImpl` compiles.
   - The device must be pre-initialized in JavaScript before `main()` runs and
     exposed as `Module.preinitializedWebGPUDevice`; the yup `shell.html` does
     this via a `preRun` run-dependency. Custom shells must replicate it.

   Known limitations:
   - Onscreen advanced blend modes that require a destination copy of the
     swapchain are degraded: emscripten's legacy swapchain only exposes a
     `TextureView` (no `Texture` handle), so `setTargetTextureView(view, {})`
     leaves the onscreen `RenderTargetWebGPU` without a copyable target texture.
     Offscreen canvases have full texture access and are unaffected.
   - `readOffscreenPixels()` is unsupported: browser buffer mapping is
     async-only and ASYNCIFY is disabled, so there is no way to block for the
     GPU-to-CPU copy. It returns false.

  ==============================================================================
*/

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/webgpu/render_context_webgpu_impl.hpp"
#include "rive/renderer/ore/ore_context.hpp"
#include "rive/renderer/rive_render_image.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgpu.h>
#include <webgpu/webgpu_cpp.h>

#include <cstdio>
#include <memory>
#include <vector>

namespace yup
{

//==============================================================================

class LowLevelRenderContextWebGPU : public GraphicsContext
{
public:
    //==============================================================================

    LowLevelRenderContextWebGPU (Options options)
        : m_options (options)
    {
        m_device = wgpu::Device::Acquire (emscripten_webgpu_get_device());
        if (m_device == nullptr)
        {
            jassertfalse;
            fprintf (stderr, "WebGPU: no device. Ensure Module.preinitializedWebGPUDevice is set before main().\n");
            return;
        }

        m_queue = m_device.GetQueue();

        m_renderContext = rive::gpu::RenderContextWebGPUImpl::MakeContext (
            {}, m_device, m_queue, rive::gpu::RenderContextWebGPUImpl::ContextOptions());

        if (m_renderContext == nullptr)
        {
            fprintf (stderr, "WebGPU: failed to create a render context.\n");
            return;
        }

        if (m_options.enableOreContext)
            m_oreContext = m_renderContext->static_impl_cast<rive::gpu::RenderContextWebGPUImpl>()->makeOreContext();
    }

    //==============================================================================

    Api getApi() const noexcept override { return Api::WebGPU; }

    float dpiScale (void*) const override
    {
        return (float) emscripten_get_device_pixel_ratio();
    }

    //==============================================================================

    rive::Factory* factory() override { return m_renderContext.get(); }

    rive::gpu::RenderContext* renderContext() override { return m_renderContext.get(); }

    rive::gpu::RenderTarget* renderTarget() override { return m_renderTarget.get(); }

    rive::ore::Context* gpuContext() const noexcept override { return m_oreContext.get(); }

    //==============================================================================

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (m_renderContext.get());
    }

    //==============================================================================

    void onSizeChanged (void*, int width, int height, uint32_t) override
    {
        if (m_renderContext == nullptr || width <= 0 || height <= 0)
            return;

        if (m_surface == nullptr)
        {
            wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvasDesc = {};
            canvasDesc.selector = "#canvas";

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &canvasDesc;

            wgpu::Instance instance = wgpu::CreateInstance();
            m_surface = instance.CreateSurface (&surfaceDesc);
        }

        wgpu::SwapChainDescriptor swapDesc = {};
        swapDesc.usage = wgpu::TextureUsage::RenderAttachment;
        swapDesc.format = wgpu::TextureFormat::BGRA8Unorm;
        swapDesc.width = (uint32_t) width;
        swapDesc.height = (uint32_t) height;
        swapDesc.presentMode = wgpu::PresentMode::Fifo; // only mode emscripten supports

        m_swapchain = m_device.CreateSwapChain (m_surface, &swapDesc);

        m_renderTarget = m_renderContext->static_impl_cast<rive::gpu::RenderContextWebGPUImpl>()
                             ->makeRenderTarget (wgpu::TextureFormat::BGRA8Unorm, (uint32_t) width, (uint32_t) height);
    }

    //==============================================================================

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        if (m_swapchain == nullptr || m_renderTarget == nullptr)
            return;

        // The legacy emscripten swapchain only exposes a TextureView, so there is
        // no companion Texture handle for advanced-blend dst copies (documented).
        m_renderTarget->setTargetTextureView (m_swapchain.GetCurrentTextureView(), {});

        m_renderContext->beginFrame (frameDescriptor);
    }

    void end (void*) override
    {
        if (m_renderTarget == nullptr)
            return;

        wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();

        m_renderContext->flush ({ .renderTarget = m_renderTarget.get(),
                                  .externalCommandBuffer = encoder.Get() });

        wgpu::CommandBuffer commands = encoder.Finish();
        m_queue.Submit (1, &commands);

        // No present call: the browser composites the canvas when control returns
        // to the event loop.
        m_renderTarget->setTargetTextureView ({}, {});
    }

    //==============================================================================

    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
    };

    struct OffscreenTargetWebGPU : public RenderableTarget
    {
        int width = 0;
        int height = 0;
        rive::rcp<rive::gpu::RenderCanvas> renderCanvas;
        rive::gpu::RenderContext* renderContext = nullptr;
        OffscreenContextSlot* contextSlot = nullptr;

        int getWidth() const noexcept override { return width; }

        int getHeight() const noexcept override { return height; }

        rive::gpu::RenderTarget* getRenderTarget() noexcept override
        {
            return renderCanvas != nullptr ? renderCanvas->renderTarget() : nullptr;
        }

        rive::gpu::RenderContext* getRenderContext() noexcept override
        {
            return renderContext;
        }

        rive::rcp<rive::gpu::RenderCanvas> getRenderCanvas() noexcept override
        {
            return renderCanvas;
        }

        rive::rcp<rive::gpu::Texture> adoptAsTexture() override
        {
            if (renderCanvas == nullptr)
                return nullptr;

            return renderCanvas->renderImage()->refTexture();
        }
    };

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0 || m_renderContext == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetWebGPU>();
        target->width = width;
        target->height = height;
        target->renderContext = nullptr;
        target->contextSlot = nullptr;
        target->renderCanvas = m_renderContext->makeRenderCanvas (static_cast<uint32_t> (width),
                                                                  static_cast<uint32_t> (height));
        if (target->renderCanvas == nullptr)
            return nullptr;

        return target;
    }

    std::unique_ptr<RenderableTarget> createRenderableTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0)
            return nullptr;

        auto* contextSlot = acquireOffscreenContext();
        if (contextSlot == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetWebGPU>();
        target->width = width;
        target->height = height;
        target->renderContext = contextSlot->renderContext.get();
        target->contextSlot = contextSlot;
        target->renderCanvas = target->renderContext->makeRenderCanvas (static_cast<uint32_t> (width),
                                                                        static_cast<uint32_t> (height));
        if (target->renderCanvas == nullptr)
            return nullptr;

        return target;
    }

    void beginOffscreen (OffscreenTarget& baseTarget, const rive::gpu::RenderContext::FrameDescriptor& frameDesc) override
    {
        auto& target = static_cast<OffscreenTargetWebGPU&> (baseTarget);
        auto* renderContext = target.getRenderContext();

        if (renderContext == nullptr || target.contextSlot == nullptr || target.contextSlot->frameActive)
            return;

        renderContext->beginFrame (frameDesc);
        target.contextSlot->frameActive = true;
    }

    void endOffscreen (OffscreenTarget& baseTarget) override
    {
        auto& target = static_cast<OffscreenTargetWebGPU&> (baseTarget);
        auto* renderContext = target.getRenderContext();

        if (renderContext == nullptr || target.contextSlot == nullptr || ! target.contextSlot->frameActive)
            return;

        wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();

        renderContext->flush ({ .renderTarget = target.getRenderTarget(),
                                .externalCommandBuffer = encoder.Get() });

        wgpu::CommandBuffer commands = encoder.Finish();
        m_queue.Submit (1, &commands);

        target.contextSlot->frameActive = false;
    }

    bool readOffscreenPixels (OffscreenTarget&, void*, size_t) override
    {
        // GPU-to-CPU buffer mapping is async-only on the web; unsupported without
        // ASYNCIFY.
        return false;
    }

private:
    OffscreenContextSlot* acquireOffscreenContext()
    {
        for (const auto& slot : m_offscreenContextPool)
        {
            if (! slot->frameActive)
                return slot.get();
        }

        auto slot = std::make_unique<OffscreenContextSlot>();
        slot->renderContext = rive::gpu::RenderContextWebGPUImpl::MakeContext (
            {}, m_device, m_queue, rive::gpu::RenderContextWebGPUImpl::ContextOptions());
        if (slot->renderContext == nullptr)
            return nullptr;

        auto* result = slot.get();
        m_offscreenContextPool.push_back (std::move (slot));
        return result;
    }

    Options m_options;
    wgpu::Device m_device;
    wgpu::Queue m_queue;
    wgpu::Surface m_surface;
    wgpu::SwapChain m_swapchain;
    std::unique_ptr<rive::gpu::RenderContext> m_renderContext;
    std::vector<std::unique_ptr<OffscreenContextSlot>> m_offscreenContextPool;
    std::unique_ptr<rive::ore::Context> m_oreContext;
    rive::rcp<rive::gpu::RenderTargetWebGPU> m_renderTarget;
};

//==============================================================================

std::unique_ptr<GraphicsContext> yup_constructWebGPUGraphicsContext (GraphicsContext::Options options)
{
    return std::make_unique<LowLevelRenderContextWebGPU> (options);
}

} // namespace yup
#endif
