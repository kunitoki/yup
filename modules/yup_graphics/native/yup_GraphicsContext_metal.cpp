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

class GraphicsContextMetal : public GraphicsContext
{
public:
    //==============================================================================

    GraphicsContextMetal (Options options, GpuDevice::Ptr existingGpu = {})
        : options (options)
    {
        // Obtain or create the GpuDevice
        if (existingGpu != nullptr)
            gpuDevice = std::move (existingGpu);
        else
            gpuDevice = GpuDevice::create (GpuPlatform::Metal, options);

        // Compile PLS shaders for the fullscreen blit pipeline
        NSError* error = nil;

        dispatch_data_t metallibData = dispatch_data_create (
            yup_RenderShader_data,
            sizeof (yup_RenderShader_data),
            nil,
            nil);

        auto* plsPrecompiledLibrary = [gpu newLibraryWithData:metallibData error:&error];
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

        pipelineState = [gpu newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
        if (pipelineState == nil || error != nil)
        {
            NSLog (@"Failed to create pipeline state: %@", error);

            jassertfalse;
            return;
        }

        quadVertexBuffer = [gpu newBufferWithBytes:quadVertices length:sizeof (quadVertices) options:MTLResourceStorageModeShared];
    }

    //==============================================================================

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Metal; }

    GpuDevice::Ptr getGpuDevice() const noexcept override { return gpuDevice; }

    //==============================================================================

    rive::Factory* getFactory() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderContext* getRenderContext() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderTarget* getRenderTarget() override { return renderTarget.get(); }

    //==============================================================================

    void onSizeChanged (void* window, int width, int height, float dpiScale, uint32_t sampleCount) override
    {
#if YUP_MAC
        NSWindow* nsWindow = (__bridge NSWindow*) window;
        NSView* nsView = [nsWindow contentView];
#endif

        if (swapchain == nil)
        {
#if YUP_MAC
            nsView.wantsLayer = YES;
#endif

            swapchain = [CAMetalLayer layer];
            swapchain.device = gpu;
            swapchain.opaque = YES;
            swapchain.framebufferOnly = ! options.readableFramebuffer;
            swapchain.pixelFormat = MTLPixelFormatBGRA8Unorm;
#if YUP_MAC
            swapchain.displaySyncEnabled = NO;
#endif

#if YUP_IOS
            UIView* view = (__bridge UIView*) window;
            swapchain.frame = view.bounds;
            [view.layer addSublayer:swapchain];
#else
            nsView.layer = swapchain;
#endif
        }

        swapchain.contentsScale = dpiScale;
        swapchain.drawableSize = CGSizeMake (width, height);

        auto renderContextImpl = getRenderContext()->static_impl_cast<rive::gpu::RenderContextMetalImpl>();
        renderTarget = renderContextImpl->makeRenderTarget (MTLPixelFormatBGRA8Unorm, width, height);

        if (currentTexture != nil)
            currentTexture = nil;

        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:(MTLPixelFormatBGRA8Unorm)
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:NO];
        descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
        currentTexture = [gpu newTextureWithDescriptor:descriptor];
    }

    //==============================================================================

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (getRenderContext());
    }

    //==============================================================================

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        getRenderContext()->beginFrame (frameDescriptor);

        if (frameDescriptor.loadAction == rive::gpu::LoadAction::clear)
        {
            id<MTLCommandBuffer> presentCommandBuffer = [queue commandBuffer];

            MTLRenderPassDescriptor* passDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
            passDescriptor.colorAttachments[0].texture = currentTexture;
            passDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            passDescriptor.colorAttachments[0].clearColor = MTLClearColorFromARGB (frameDescriptor.clearColor);
            passDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

            id<MTLRenderCommandEncoder> encoder = [presentCommandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
            [encoder setRenderPipelineState:pipelineState];
            [encoder endEncoding];

            [presentCommandBuffer commit];
        }
    }

    void end (void*) override
    {
        jassert (renderTarget != nil);

        // Render into texture
        jassert (currentTexture.width == renderTarget->width());
        jassert (currentTexture.height == renderTarget->height());
        renderTarget->setTargetTexture (currentTexture);

        id<MTLCommandBuffer> presentCommandBuffer = [queue commandBuffer];
        getRenderContext()->flush ({ .renderTarget = renderTarget.get(), .externalCommandBuffer = (__bridge void*) presentCommandBuffer });

        // Render texture in view drawable
        jassert (currentFrameSurface == nil);
        currentFrameSurface = [swapchain nextDrawable];
        jassert (currentFrameSurface.texture.width == renderTarget->width());
        jassert (currentFrameSurface.texture.height == renderTarget->height());

        MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        renderPassDescriptor.colorAttachments[0].texture = currentFrameSurface.texture;
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionDontCare;
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLRenderCommandEncoder> renderEncoder = [presentCommandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder setRenderPipelineState:pipelineState];
        [renderEncoder setFragmentTexture:currentTexture atIndex:0];
        [renderEncoder setVertexBuffer:quadVertexBuffer offset:0 atIndex:0];
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [renderEncoder endEncoding];

        [presentCommandBuffer presentDrawable:currentFrameSurface];
        [presentCommandBuffer commit];

        currentFrameSurface = nil;
        renderTarget->setTargetTexture (nil);
    }

private:
    const Options options;
    rive::gpu::RenderContextMetalImpl::ContextOptions renderContextOptions;
    GpuDevice::Ptr gpuDevice;
    id<MTLDevice> gpu = MTLCreateSystemDefaultDevice();
    id<MTLCommandQueue> queue = [gpu newCommandQueue];
    CAMetalLayer* swapchain = nil;
    rive::rcp<rive::gpu::RenderTargetMetal> renderTarget;
    id<CAMetalDrawable> currentFrameSurface = nil;
    id<MTLRenderPipelineState> pipelineState = nil;
    id<MTLTexture> currentTexture = nil;
    id<MTLBuffer> quadVertexBuffer = nil;
};

//==============================================================================

std::unique_ptr<GraphicsContext> yup_constructMetalGraphicsContext (GpuDevice::Options options,
                                                                    GpuDevice::Ptr existingGpu)
{
    return std::make_unique<GraphicsContextMetal> (options, std::move (existingGpu));
}

} // namespace yup
#endif
