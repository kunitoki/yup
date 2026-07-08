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
#include "rive/renderer/ore/ore_context_metal.hpp"

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

    LowLevelRenderContextMetal (Options fiddleOptions)
        : m_fiddleOptions (fiddleOptions)
    {
        rive::gpu::RenderContextMetalImpl::ContextOptions metalOptions;

        if (m_fiddleOptions.synchronousShaderCompilations)
            metalOptions.shaderCompilationMode = rive::gpu::ShaderCompilationMode::alwaysSynchronous;

        if (m_fiddleOptions.disableRasterOrdering)
            metalOptions.disableFramebufferReads = true;

        m_renderContext = rive::gpu::RenderContextMetalImpl::MakeContext (m_gpu, metalOptions);
        m_offscreenRenderContext = rive::gpu::RenderContextMetalImpl::MakeContext (m_gpu, metalOptions);

        if (m_fiddleOptions.enableOreContext)
            m_oreContext = rive::ore::ContextMetal::Make (m_gpu, m_queue);

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

    Api getApi() const noexcept override { return Api::Metal; }

    float dpiScale (void* window) const override
    {
#if YUP_IOS
        UIWindow* uiWindow = (__bridge UIWindow*) window;
        UIScreen* screen = [uiWindow screen] ?: [UIScreen mainScreen];
        return screen.nativeScale;
#else
        NSWindow* nsWindow = (__bridge NSWindow*) window;
        return m_fiddleOptions.retinaDisplay ? nsWindow.backingScaleFactor : 1.0f;
#endif
    }

    //==============================================================================

    rive::Factory* factory() override { return m_renderContext.get(); }

    rive::gpu::RenderContext* renderContext() override { return m_renderContext.get(); }

    rive::gpu::RenderTarget* renderTarget() override { return m_renderTarget.get(); }

    rive::ore::Context* gpuContext() const noexcept override { return m_oreContext.get(); }

    //==============================================================================

    void onSizeChanged (void* window, int width, int height, uint32_t sampleCount) override
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

        m_swapchain.contentsScale = dpiScale (window);
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

    //==============================================================================

    struct OffscreenTargetMetal : public OffscreenTarget
    {
        int width = 0;
        int height = 0;
        id<MTLTexture> stagingTexture = nil;
        rive::rcp<rive::gpu::RenderCanvas> renderCanvas;
        rive::gpu::RenderContext* renderContext = nullptr;

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

        id<MTLTexture> targetTexture() const
        {
            if (renderCanvas == nullptr)
                return nil;

            if (auto* target = static_cast<rive::gpu::RenderTargetMetal*> (renderCanvas->renderTarget()))
                return target->targetTexture();

            return nil;
        }
    };

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0 || m_offscreenRenderContext == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetMetal>();
        target->width = width;
        target->height = height;
        target->renderContext = m_offscreenRenderContext.get();
        target->renderCanvas = m_offscreenRenderContext->makeRenderCanvas (static_cast<uint32_t> (width),
                                                                           static_cast<uint32_t> (height));
        if (target->renderCanvas == nullptr)
            return nullptr;

        MTLTextureDescriptor* stagingDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                               width:static_cast<NSUInteger> (width)
                                                                                              height:static_cast<NSUInteger> (height)
                                                                                           mipmapped:NO];
        stagingDesc.usage = MTLTextureUsageShaderRead;
#if YUP_IOS
        stagingDesc.storageMode = MTLStorageModeShared;
#else
        stagingDesc.storageMode = MTLStorageModeManaged;
#endif
        target->stagingTexture = [m_gpu newTextureWithDescriptor:stagingDesc];

        if (target->targetTexture() == nil || target->stagingTexture == nil)
            return nullptr;

        return target;
    }

    void beginOffscreen (OffscreenTarget& baseTarget, const rive::gpu::RenderContext::FrameDescriptor& frameDesc) override
    {
        auto& target = static_cast<OffscreenTargetMetal&> (baseTarget);
        auto* renderContext = target.getRenderContext();

        if (renderContext == nullptr)
            return;

        renderContext->beginFrame (frameDesc);
    }

    void endOffscreen (OffscreenTarget& baseTarget) override
    {
        auto& target = static_cast<OffscreenTargetMetal&> (baseTarget);
        auto* renderContext = target.getRenderContext();

        if (renderContext == nullptr)
            return;

        id<MTLCommandBuffer> commandBuffer = [m_queue commandBuffer];
        renderContext->flush ({ .renderTarget = target.getRenderTarget(), .externalCommandBuffer = (__bridge void*) commandBuffer });
        [commandBuffer commit];
    }

    bool readOffscreenPixels (OffscreenTarget& baseTarget, void* dst, size_t dstSize) override
    {
        auto& target = static_cast<OffscreenTargetMetal&> (baseTarget);

        if (target.stagingTexture == nil || dst == nullptr)
            return false;

        id<MTLTexture> srcTexture = target.targetTexture();
        if (srcTexture == nil)
            return false;

        const auto w = static_cast<NSUInteger> (target.width);
        const auto h = static_cast<NSUInteger> (target.height);
        const size_t bytesPerRow = w * 4u;

        if (dstSize < bytesPerRow * h)
            return false;

        // Copy the rendered target into a CPU-readable staging texture and block
        // until the GPU is done. This is the only path that requires a CPU/GPU
        // sync, so the stall is paid only when pixels are actually read back.
        id<MTLCommandBuffer> commandBuffer = [m_queue commandBuffer];

        id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
        [blitEncoder copyFromTexture:srcTexture
                         sourceSlice:0
                         sourceLevel:0
                        sourceOrigin:MTLOriginMake (0, 0, 0)
                          sourceSize:MTLSizeMake (w, h, 1)
                           toTexture:target.stagingTexture
                    destinationSlice:0
                    destinationLevel:0
                   destinationOrigin:MTLOriginMake (0, 0, 0)];
#if YUP_MAC
        [blitEncoder synchronizeResource:target.stagingTexture];
#endif
        [blitEncoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        [target.stagingTexture getBytes:dst
                            bytesPerRow:bytesPerRow
                             fromRegion:MTLRegionMake2D (0, 0, w, h)
                            mipmapLevel:0];

        return true;
    }

private:
    const Options m_fiddleOptions;
    std::unique_ptr<rive::gpu::RenderContext> m_renderContext;
    std::unique_ptr<rive::gpu::RenderContext> m_offscreenRenderContext;
    std::unique_ptr<rive::ore::ContextMetal> m_oreContext;
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

std::unique_ptr<GraphicsContext> yup_constructMetalGraphicsContext (GraphicsContext::Options fiddleOptions)
{
    return std::make_unique<LowLevelRenderContextMetal> (fiddleOptions);
}

} // namespace yup
#endif
