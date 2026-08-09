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

#if YUP_MAC
#include "yup_RenderShader_mac.c"
#elif YUP_IOS_SIMULATOR
#include "yup_RenderShader_iossim.c"
#elif YUP_IOS
#include "yup_RenderShader_ios.c"
#else
#error Unsupported target sdk!
#endif

#import <simd/simd.h>

#include <vector>

namespace yup
{

namespace
{

//==============================================================================

typedef struct
{
    vector_float2 position;
    vector_float2 texCoord;
} Vertex;

// Full-screen quad covering clip space, with texture coordinates mapping the texture.
const Vertex quadVertices[] = {
    { { -1.0f, 1.0f }, { 0.0f, 0.0f } },  // Top-left
    { { -1.0f, -1.0f }, { 0.0f, 1.0f } }, // Bottom-left
    { { 1.0f, 1.0f }, { 1.0f, 0.0f } },   // Top-right
    { { 1.0f, -1.0f }, { 1.0f, 1.0f } }   // Bottom-right
};

MTLClearColor MTLClearColorFromARGB (uint32_t argb)
{
    double a = ((argb >> 24) & 0xFF) / 255.0;
    double r = ((argb >> 16) & 0xFF) / 255.0;
    double g = ((argb >> 8) & 0xFF) / 255.0;
    double b = ((argb >> 0) & 0xFF) / 255.0;

    return MTLClearColorMake (r, g, b, a);
}

} // namespace

//==============================================================================

class LowLevelRenderContextMetal : public GraphicsContext
{
public:
    //==============================================================================

    LowLevelRenderContextMetal (Options fiddleOptions, GpuDevice::Ptr existingGpu = {})
        : m_fiddleOptions (fiddleOptions)
    {
        // Obtain or create the GpuDevice
        if (existingGpu != nullptr)
        {
            m_gpuContext = std::move (existingGpu);
        }
        else
        {
            m_gpuContext = GpuDevice::create (GpuPlatform::Metal, fiddleOptions);
        }

        // Own GpuDeviceMetal knows the native device/queue — extract them.
        // GpuDeviceMetal exposes getDevice()/getCommandQueue() for sharing.
        jassert (m_gpuContext != nullptr);

        // Create the Rive render context (needed for windowed rendering + vector content)
        if (m_fiddleOptions.synchronousShaderCompilations)
            m_renderContextOptions.shaderCompilationMode = rive::gpu::ShaderCompilationMode::alwaysSynchronous;

        if (m_fiddleOptions.disableRasterOrdering)
            m_renderContextOptions.disableFramebufferReads = true;

        m_renderContext = rive::gpu::RenderContextMetalImpl::MakeContext (m_gpu, m_renderContextOptions);

        // Compile PLS shaders for the fullscreen blit pipeline
        NSError* error = nil;

        dispatch_data_t metallibData = dispatch_data_create (
            yup_RenderShader_data,
            sizeof (yup_RenderShader_data),
            nil,
            nil);

        auto* plsPrecompiledLibrary = [m_gpu newLibraryWithData:metallibData error:&error];
        if (plsPrecompiledLibrary == nil || error != nil)
        {
            NSLog (@"Failed to load binary shaders: %@", error);

            jassertfalse;
            return;
        }

        MTLVertexDescriptor* vertexDescriptor = [[MTLVertexDescriptor alloc] init];
        vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        vertexDescriptor.attributes[0].offset = 0;
        vertexDescriptor.attributes[0].bufferIndex = 0;
        vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
        vertexDescriptor.attributes[1].offset = sizeof (vector_float2);
        vertexDescriptor.attributes[1].bufferIndex = 0;
        vertexDescriptor.layouts[0].stride = sizeof (Vertex);
        vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

        MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDescriptor.label = @"Quad Pipeline";
        pipelineDescriptor.vertexFunction = [plsPrecompiledLibrary newFunctionWithName:@"vertexShader"];
        pipelineDescriptor.fragmentFunction = [plsPrecompiledLibrary newFunctionWithName:@"fragmentShader"];
        pipelineDescriptor.vertexDescriptor = vertexDescriptor;
        pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

        m_pipelineState = [m_gpu newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
        if (m_pipelineState == nil || error != nil)
        {
            NSLog (@"Failed to create pipeline state: %@", error);

            jassertfalse;
            return;
        }

        m_quadVertexBuffer = [m_gpu newBufferWithBytes:quadVertices length:sizeof (quadVertices) options:MTLResourceStorageModeShared];
    }

    //==============================================================================

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Metal; }

    GpuDevice::Ptr getGpuDevice() const noexcept override { return m_gpuContext; }

    //==============================================================================

    rive::Factory* factory() override { return m_renderContext.get(); }

    rive::gpu::RenderContext* renderContext() override { return m_renderContext.get(); }

    rive::gpu::RenderTarget* renderTarget() override { return m_renderTarget.get(); }

    //==============================================================================

    void onSizeChanged (void* window, int width, int height, float dpiScale, uint32_t sampleCount) override
    {
#if YUP_MAC
        NSWindow* nsWindow = (__bridge NSWindow*) window;
        NSView* nsView = [nsWindow contentView];
#endif

        if (m_swapchain == nil)
        {
#if YUP_MAC
            nsView.wantsLayer = YES;
#endif

            m_swapchain = [CAMetalLayer layer];
            m_swapchain.device = m_gpu;
            m_swapchain.opaque = YES;
            m_swapchain.framebufferOnly = ! m_fiddleOptions.readableFramebuffer;
            m_swapchain.pixelFormat = MTLPixelFormatBGRA8Unorm;
#if YUP_MAC
            m_swapchain.displaySyncEnabled = NO;
#endif

#if YUP_IOS
            UIView* view = (__bridge UIView*) window;
            m_swapchain.frame = view.bounds;
            [view.layer addSublayer:m_swapchain];
#else
            nsView.layer = m_swapchain;
#endif
        }

        m_swapchain.contentsScale = dpiScale;
        m_swapchain.drawableSize = CGSizeMake (width, height);

        auto renderContextImpl = m_renderContext->static_impl_cast<rive::gpu::RenderContextMetalImpl>();
        m_renderTarget = renderContextImpl->makeRenderTarget (MTLPixelFormatBGRA8Unorm, width, height);

        if (m_currentTexture != nil)
            m_currentTexture = nil;

        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:(MTLPixelFormatBGRA8Unorm)
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:NO];
        descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        m_currentTexture = [m_gpu newTextureWithDescriptor:descriptor];
    }

    //==============================================================================

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (m_renderContext.get());
    }

    //==============================================================================

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        m_renderContext->beginFrame (frameDescriptor);

        if (frameDescriptor.loadAction == rive::gpu::LoadAction::clear)
        {
            id<MTLCommandBuffer> presentCommandBuffer = [m_queue commandBuffer];

            MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            passDescriptor.colorAttachments[0].texture = m_currentTexture;
            passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            passDescriptor.colorAttachments[0].clearColor = MTLClearColorFromARGB (frameDescriptor.clearColor);
            passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

            id<MTLRenderCommandEncoder> encoder = [presentCommandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
            [encoder setRenderPipelineState:m_pipelineState];
            [encoder endEncoding];

            [presentCommandBuffer commit];
        }
    }

    void end (void*) override
    {
        jassert (m_renderTarget != nil);

        // Render into texture
        jassert (m_currentTexture.width == m_renderTarget->width());
        jassert (m_currentTexture.height == m_renderTarget->height());
        m_renderTarget->setTargetTexture (m_currentTexture);

        id<MTLCommandBuffer> presentCommandBuffer = [m_queue commandBuffer];
        m_renderContext->flush ({ .renderTarget = m_renderTarget.get(), .externalCommandBuffer = (__bridge void*) presentCommandBuffer });

        // Render texture in view drawable
        jassert (m_currentFrameSurface == nil);
        m_currentFrameSurface = [m_swapchain nextDrawable];
        jassert (m_currentFrameSurface.texture.width == m_renderTarget->width());
        jassert (m_currentFrameSurface.texture.height == m_renderTarget->height());

        MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        renderPassDescriptor.colorAttachments[0].texture = m_currentFrameSurface.texture;
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionDontCare;
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLRenderCommandEncoder> renderEncoder = [presentCommandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder setRenderPipelineState:m_pipelineState];
        [renderEncoder setFragmentTexture:m_currentTexture atIndex:0];
        [renderEncoder setVertexBuffer:m_quadVertexBuffer offset:0 atIndex:0];
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [renderEncoder endEncoding];

        [presentCommandBuffer presentDrawable:m_currentFrameSurface];
        [presentCommandBuffer commit];

        m_currentFrameSurface = nil;
        m_renderTarget->setTargetTexture (nil);
    }

private:
    const Options m_fiddleOptions;
    rive::gpu::RenderContextMetalImpl::ContextOptions m_renderContextOptions;
    GpuDevice::Ptr m_gpuContext;
    std::unique_ptr<rive::gpu::RenderContext> m_renderContext;
    id<MTLDevice> m_gpu = MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> m_queue = [m_gpu newCommandQueue];
    CAMetalLayer* m_swapchain = nil;
    rive::rcp<rive::gpu::RenderTargetMetal> m_renderTarget;
    id<CAMetalDrawable> m_currentFrameSurface = nil;
    id<MTLRenderPipelineState> m_pipelineState = nil;
    id<MTLTexture> m_currentTexture = nil;
    id<MTLBuffer> m_quadVertexBuffer = nil;
};

//==============================================================================

std::unique_ptr<GraphicsContext> yup_constructMetalGraphicsContext (GpuDevice::Options fiddleOptions,
                                                                    GpuDevice::Ptr existingGpu)
{
    return std::make_unique<LowLevelRenderContextMetal> (fiddleOptions, std::move (existingGpu));
}

} // namespace yup
#endif
