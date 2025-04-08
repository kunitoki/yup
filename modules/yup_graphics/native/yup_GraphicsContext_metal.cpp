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

#if JUCE_MAC
#include "yup_RenderShader_mac.c"
#else
#include "yup_RenderShader_ios.c"
#endif

#import <simd/simd.h>

namespace yup
{

namespace
{

//==============================================================================

typedef struct {
    vector_float2 position;
    vector_float2 texCoord;
} Vertex;

// Full-screen quad covering clip space, with texture coordinates mapping the texture.
const Vertex quadVertices[] =
{
    { { -1.0f,  1.0f }, { 0.0f, 0.0f } }, // Top-left
    { { -1.0f, -1.0f }, { 0.0f, 1.0f } }, // Bottom-left
    { {  1.0f,  1.0f }, { 1.0f, 0.0f } }, // Top-right
    { {  1.0f, -1.0f }, { 1.0f, 1.0f } } // Bottom-right
};

} // namespace

//==============================================================================

class LowLevelRenderContextMetal : public GraphicsContext
{
public:
    //==============================================================================

    LowLevelRenderContextMetal (Options fiddleOptions)
        : m_fiddleOptions (fiddleOptions)
    {
        rive::gpu::RenderContextMetalImpl::ContextOptions metalOptions;

        if (m_fiddleOptions.synchronousShaderCompilations)
            metalOptions.synchronousShaderCompilations = true;

        if (m_fiddleOptions.disableRasterOrdering)
            metalOptions.disableFramebufferReads = true;

        m_plsContext = rive::gpu::RenderContextMetalImpl::MakeContext (m_gpu, metalOptions);

        NSError* error = nil;

        dispatch_data_t metallibData = dispatch_data_create(
            yup_RenderShader_data,
            sizeof(yup_RenderShader_data),
            nil,
            nil);

        auto* plsPrecompiledLibrary = [m_gpu newLibraryWithData:metallibData error:&error];
        if (plsPrecompiledLibrary == nil || error != nil)
        {
            JUCE_DBG ([[error localizedDescription] UTF8String]);
            jassertfalse;
            return;
        }

        MTLVertexDescriptor *vertexDescriptor = [[MTLVertexDescriptor alloc] init];
        vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2;
        vertexDescriptor.attributes[0].offset = 0;
        vertexDescriptor.attributes[0].bufferIndex = 0;
        vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
        vertexDescriptor.attributes[1].offset = sizeof(vector_float2);
        vertexDescriptor.attributes[1].bufferIndex = 0;
        vertexDescriptor.layouts[0].stride = sizeof(Vertex);
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
            JUCE_DBG ([[error localizedDescription] UTF8String]);
            jassertfalse;
            return;
        }

        m_quadVertexBuffer = [m_gpu newBufferWithBytes:quadVertices length:sizeof(quadVertices) options:MTLResourceStorageModeShared];
    }

    //==============================================================================

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

    //==============================================================================

    rive::Factory* factory() override { return m_plsContext.get(); }

    rive::gpu::RenderContext* renderContext() override { return m_plsContext.get(); }

    rive::gpu::RenderTarget* renderTarget() override { return m_renderTarget.get(); }

    //==============================================================================

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
        m_swapchain.displaySyncEnabled = NO;
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
        return std::make_unique<rive::RiveRenderer> (m_plsContext.get());
    }

    //==============================================================================

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        m_plsContext->beginFrame (frameDescriptor);
    }

    void end (void*) override
    {
        jassert (m_renderTarget != nil);

        // Render into texture
        jassert (m_currentTexture.width == m_renderTarget->width());
        jassert (m_currentTexture.height == m_renderTarget->height());
        m_renderTarget->setTargetTexture (m_currentTexture);

        id<MTLCommandBuffer> commandBuffer = [m_queue commandBuffer];
        m_plsContext->flush ({ .renderTarget = m_renderTarget.get(), .externalCommandBuffer = (__bridge void*) commandBuffer });

        // Render texture in view drawable
        jassert (m_currentFrameSurface == nil);
        m_currentFrameSurface = [m_swapchain nextDrawable];
        jassert (m_currentFrameSurface.texture.width == m_renderTarget->width());
        jassert (m_currentFrameSurface.texture.height == m_renderTarget->height());

        MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
        renderPassDescriptor.colorAttachments[0].texture = m_currentFrameSurface.texture;
        renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [renderEncoder setRenderPipelineState:m_pipelineState];
        [renderEncoder setFragmentTexture:m_currentTexture atIndex:0];
        [renderEncoder setVertexBuffer:m_quadVertexBuffer offset:0 atIndex:0];
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [renderEncoder endEncoding];

        [commandBuffer presentDrawable:m_currentFrameSurface];
        [commandBuffer commit];

        m_currentFrameSurface = nil;
        m_renderTarget->setTargetTexture (nil);
    }

private:
    const Options m_fiddleOptions;
    std::unique_ptr<rive::gpu::RenderContext> m_plsContext;
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

std::unique_ptr<GraphicsContext> juce_constructMetalGraphicsContext (GraphicsContext::Options fiddleOptions)
{
    return std::make_unique<LowLevelRenderContextMetal> (fiddleOptions);
}

} // namespace yup
#endif
