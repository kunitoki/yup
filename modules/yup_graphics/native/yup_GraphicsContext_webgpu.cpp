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

class GraphicsContextWebGPU : public GraphicsContext
{
public:
    GraphicsContextWebGPU (Options options, GpuDevice::Ptr existingGpu = {})
        : options (options)
    {
        device = wgpu::Device::Acquire (emscripten_webgpu_get_device());
        if (device == nullptr)
        {
            jassertfalse;
            fprintf (stderr, "WebGPU: no device. Ensure Module.preinitializedWebGPUDevice is set before main().\n");
            return;
        }

        queue = device.GetQueue();

        // Obtain or create the GpuDevice for RHI/offscreen operations
        if (existingGpu != nullptr)
            gpuDevice = std::move (existingGpu);
        else
            gpuDevice = GpuDevice::create (GpuPlatform::WebGPU, options);
    }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::WebGPU; }

    GpuDevice::Ptr getGpuDevice() const noexcept override { return gpuDevice; }

    rive::Factory* getFactory() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderContext* getRenderContext() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderTarget* getRenderTarget() override { return renderTarget.get(); }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (getRenderContext());
    }

    void onSizeChanged (void*, int newWidth, int newHeight, float dpiScale, uint32_t) override
    {
        if (gpuDevice == nullptr || getRenderContext() == nullptr || newWidth <= 0 || newHeight <= 0)
            return;

        width = newWidth;
        height = newHeight;

        if (surface == nullptr)
        {
            wgpu::EmscriptenSurfaceSourceCanvasHTMLSelector canvasDesc = {};
            canvasDesc.selector = "#canvas";

            wgpu::SurfaceDescriptor surfaceDesc = {};
            surfaceDesc.nextInChain = &canvasDesc;

            wgpu::Instance instance = wgpu::CreateInstance();
            surface = instance.CreateSurface (&surfaceDesc);
        }

        wgpu::SurfaceConfiguration config = {};
        config.device = device;
        config.format = wgpu::TextureFormat::BGRA8Unorm;
        config.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopyDst;
        config.width = (uint32_t) width;
        config.height = (uint32_t) height;
        config.alphaMode = wgpu::CompositeAlphaMode::Auto;
        config.presentMode = wgpu::PresentMode::Fifo;

        surface.Configure (&config);

        wgpu::TextureDescriptor textureDesc = {};
        textureDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
        textureDesc.dimension = wgpu::TextureDimension::e2D;
        textureDesc.size = { (uint32_t) width, (uint32_t) height, 1 };
        textureDesc.format = wgpu::TextureFormat::BGRA8Unorm;

        offscreenTexture = device.CreateTexture (&textureDesc);
        offscreenTextureView = offscreenTexture.CreateView();

        renderTarget = getRenderContext()->static_impl_cast<rive::gpu::RenderContextWebGPUImpl>()->makeRenderTarget (wgpu::TextureFormat::BGRA8Unorm, (uint32_t) width, (uint32_t) height);
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        if (offscreenTextureView == nullptr || renderTarget == nullptr)
            return;

        renderTarget->setTargetTextureView (offscreenTextureView, offscreenTexture);
        getRenderContext()->beginFrame (frameDescriptor);
    }

    void end (void*) override
    {
        if (renderTarget == nullptr || offscreenTexture == nullptr || surface == nullptr)
            return;

        wgpu::SurfaceTexture surfaceTexture = {};
        surface.GetCurrentTexture (&surfaceTexture);
        if (surfaceTexture.texture == nullptr)
            return;

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

        getRenderContext()->flush ({ .renderTarget = renderTarget.get(),
                                     .externalCommandBuffer = encoder.Get() });

        wgpu::TexelCopyTextureInfo copySource = {};
        copySource.texture = offscreenTexture;
        copySource.aspect = wgpu::TextureAspect::All;

        wgpu::TexelCopyTextureInfo copyDestination = {};
        copyDestination.texture = surfaceTexture.texture;
        copyDestination.aspect = wgpu::TextureAspect::All;

        wgpu::Extent3D copySize = {};
        copySize.width = (uint32_t) width;
        copySize.height = (uint32_t) height;
        copySize.depthOrArrayLayers = 1;

        encoder.CopyTextureToTexture (&copySource, &copyDestination, &copySize);

        wgpu::CommandBuffer commands = encoder.Finish();
        queue.Submit (1, &commands);

        renderTarget->setTargetTextureView ({}, {});
    }

private:
    Options options;
    GpuDevice::Ptr gpuDevice;
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::Surface surface;
    wgpu::Texture offscreenTexture;
    wgpu::TextureView offscreenTextureView;
    int width = 0;
    int height = 0;
    rive::rcp<rive::gpu::RenderTargetWebGPU> renderTarget;
};

std::unique_ptr<GraphicsContext> yup_constructWebGPUGraphicsContext (GpuDevice::Options options, GpuDevice::Ptr existingGpu)
{
    return std::make_unique<GraphicsContextWebGPU> (options, std::move (existingGpu));
}

} // namespace yup
#endif
