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

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/webgpu/render_context_webgpu_impl.hpp"
#include "rive/renderer/rive_render_image.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

#include <cstdio>
#include <memory>

namespace yup
{

class LowLevelRenderContextWebGPU : public GraphicsContext
{
public:
    LowLevelRenderContextWebGPU (Options options, GpuDevice::Ptr existingGpu = {})
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

        // Obtain or create the GpuDevice for RHI/offscreen operations
        if (existingGpu != nullptr)
            m_gpuContext = std::move (existingGpu);
        else
            m_gpuContext = GpuDevice::create (GpuPlatform::WebGPU, options);

        m_renderContext = rive::gpu::RenderContextWebGPUImpl::MakeContext (
            {}, m_device, m_queue, rive::gpu::RenderContextWebGPUImpl::ContextOptions());

        if (m_renderContext == nullptr)
        {
            fprintf (stderr, "WebGPU: failed to create a render context.\n");
            return;
        }
    }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::WebGPU; }

    GpuDevice::Ptr getGpuDevice() const noexcept override { return m_gpuContext; }

    rive::Factory* factory() override { return m_renderContext.get(); }

    rive::gpu::RenderContext* renderContext() override { return m_renderContext.get(); }

    rive::gpu::RenderTarget* renderTarget() override { return m_renderTarget.get(); }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (m_renderContext.get());
    }

    void onSizeChanged (void*, int width, int height, float dpiScale, uint32_t) override
    {
        if (m_renderContext == nullptr || width <= 0 || height <= 0)
            return;

        m_width = width;
        m_height = height;

        if (m_surface == nullptr)
        {
            wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = {};
            canvasDesc.selector = "#canvas";

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &canvasDesc;

            wgpu::Instance instance = wgpu::CreateInstance();
            m_surface = instance.CreateSurface (&surfaceDesc);
        }

        wgpu::SurfaceConfiguration config = {};
        config.device = m_device;
        config.format = wgpu::TextureFormat::BGRA8Unorm;
        config.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopyDst;
        config.width = (uint32_t) width;
        config.height = (uint32_t) height;
        config.alphaMode = wgpu::CompositeAlphaMode::Auto;
        config.presentMode = wgpu::PresentMode::Fifo;

        m_surface.Configure (&config);

        wgpu::TextureDescriptor textureDesc = {};
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.size = { (uint32_t) width, (uint32_t) height, 1 };
        textureDesc.format = wgpu::TextureFormat::BGRA8Unorm;

        m_offscreenTexture = m_device.CreateTexture (&textureDesc);
        m_offscreenTextureView = m_offscreenTexture.CreateView();

        m_renderTarget = m_renderContext->static_impl_cast<rive::gpu::RenderContextWebGPUImpl>()
                             ->makeRenderTarget (wgpu::TextureFormat::BGRA8Unorm, (uint32_t) width, (uint32_t) height);
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        if (m_offscreenTextureView == nullptr || m_renderTarget == nullptr)
            return;

        m_renderTarget->setTargetTextureView (m_offscreenTextureView, m_offscreenTexture);
        m_renderContext->beginFrame (frameDescriptor);
    }

    void end (void*) override
    {
        if (m_renderTarget == nullptr || m_offscreenTexture == nullptr || m_surface == nullptr)
            return;

        wgpu::SurfaceTexture surfaceTexture = {};
        m_surface.GetCurrentTexture (&surfaceTexture);
        if (surfaceTexture.texture == nullptr)
            return;

        wgpu::CommandEncoder encoder = m_device.CreateCommandEncoder();

        m_renderContext->flush ({ .renderTarget = m_renderTarget.get(),
                                  .externalCommandBuffer = encoder.Get() });

        wgpu::TexelCopyTextureInfo copySource = {};
        copySource.texture = m_offscreenTexture;
        copySource.aspect = wgpu::TextureAspect::All;

        wgpu::TexelCopyTextureInfo copyDestination = {};
        copyDestination.texture = surfaceTexture.texture;
        copyDestination.aspect = wgpu::TextureAspect::All;

        wgpu::Extent3D copySize = {};
        copySize.width = (uint32_t) m_width;
        copySize.height = (uint32_t) m_height;
        copySize.depthOrArrayLayers = 1;

        encoder.CopyTextureToTexture (&copySource, &copyDestination, &copySize);

        wgpu::CommandBuffer commands = encoder.Finish();
        m_queue.Submit (1, &commands);

        m_renderTarget->setTargetTextureView ({}, {});
    }

private:
    Options m_options;
    GpuDevice::Ptr m_gpuContext;
    wgpu::Device m_device;
    wgpu::Queue m_queue;
    wgpu::Surface m_surface;
    wgpu::Texture m_offscreenTexture;
    wgpu::TextureView m_offscreenTextureView;
    int m_width = 0;
    int m_height = 0;
    std::unique_ptr<rive::gpu::RenderContext> m_renderContext;
    rive::rcp<rive::gpu::RenderTargetWebGPU> m_renderTarget;
};

std::unique_ptr<GraphicsContext> yup_constructWebGPUGraphicsContext (GpuDevice::Options options, GpuDevice::Ptr existingGpu)
{
    return std::make_unique<LowLevelRenderContextWebGPU> (options, std::move (existingGpu));
}

} // namespace yup
#endif
