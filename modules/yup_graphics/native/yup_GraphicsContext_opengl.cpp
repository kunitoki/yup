/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_WASM || YUP_ANDROID
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/gl/gles3.hpp"
#include "rive/renderer/gl/render_buffer_gl_impl.hpp"
#include "rive/renderer/gl/render_context_gl_impl.hpp"
#include "rive/renderer/gl/render_target_gl.hpp"
#include "rive/renderer/render_context_impl.hpp"
#include "rive/renderer/rive_render_image.hpp"
#include <vector>
#include <cstring>

namespace yup
{

//==============================================================================

class GraphicsContextOpenGL : public GraphicsContext
{
public:
    GraphicsContextOpenGL (Options options, GpuDevice::Ptr existingGpu = {})
        : options (options)
    {
        // Obtain or create the GpuDevice for offscreen/RHI operations
        if (existingGpu != nullptr)
            gpuDevice = std::move (existingGpu);
        else
            gpuDevice = GpuDevice::create (getPlatform(), options);
    }

    ~GraphicsContextOpenGL()
    {
        cleanupOffscreenResources();
    }

    GpuPlatform getPlatform() const noexcept override
    {
#if RIVE_ANDROID || RIVE_WEBGL
        return GpuPlatform::OpenGLES;
#else
        return GpuPlatform::OpenGL;
#endif
    }

    GpuDevice::Ptr getGpuDevice() const noexcept override { return gpuDevice; }

    rive::Factory* getFactory() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderContext* getRenderContext() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderTarget* getRenderTarget() override { return offscreenRenderTarget.get(); }

    void onSizeChanged (void* window, int width, int height, float dpiScale, uint32_t sampleCount) override
    {
        width = width;
        height = height;
        sampleCount = sampleCount;
        createOffscreenResources();
    }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (getRenderContext());
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        getRenderContext()->static_impl_cast<rive::gpu::RenderContextGLImpl>()->invalidateGLState();
        getRenderContext()->beginFrame (frameDescriptor);
    }

    void end (void*) override
    {
        getRenderContext()->static_impl_cast<rive::gpu::RenderContextGLImpl>()->invalidateGLState();
        getRenderContext()->flush ({ offscreenRenderTarget.get() });
        getRenderContext()->static_impl_cast<rive::gpu::RenderContextGLImpl>()->unbindGLInternalResources();
        blitToMainFramebuffer();
    }

private:
    void createOffscreenResources()
    {
        if (width <= 0 || height <= 0)
        {
            fprintf (stderr, "createOffscreenResources: Invalid size %dx%d\n", width, height);
            return;
        }

        cleanupOffscreenResources();

        glGenTextures (1, &offscreenTexture);
        glBindTexture (GL_TEXTURE_2D, offscreenTexture);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
            fprintf (stderr, "GL error after texture creation: 0x%x\n", error);

        glBindTexture (GL_TEXTURE_2D, 0);

        glGenFramebuffers (1, &offscreenFramebuffer);
        glBindFramebuffer (GL_FRAMEBUFFER, offscreenFramebuffer);
        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, offscreenTexture, 0);

        GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            fprintf (stderr, "Offscreen framebuffer is not complete: 0x%x\n", status);

        glBindFramebuffer (GL_FRAMEBUFFER, 0);

        offscreenRenderTarget = rive::make_rcp<rive::gpu::FramebufferRenderTargetGL> (
            width, height, offscreenFramebuffer, sampleCount);
    }

    void cleanupOffscreenResources()
    {
        if (offscreenFramebuffer != 0)
        {
            glDeleteFramebuffers (1, &offscreenFramebuffer);
            offscreenFramebuffer = 0;
        }
        if (offscreenTexture != 0)
        {
            glDeleteTextures (1, &offscreenTexture);
            offscreenTexture = 0;
        }
        offscreenRenderTarget.reset();
    }

    void blitToMainFramebuffer()
    {
        if (offscreenTexture == 0)
        {
            fprintf (stderr, "blitToMainFramebuffer: Invalid program or texture\n");
            return;
        }
        glBindFramebuffer (GL_READ_FRAMEBUFFER, offscreenFramebuffer);
        glBindFramebuffer (GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer (0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    Options options;
    GpuDevice::Ptr gpuDevice;
    rive::rcp<rive::gpu::RenderTargetGL> offscreenRenderTarget;

    GLuint offscreenFramebuffer = 0;
    GLuint offscreenTexture = 0;
    int width = 0;
    int height = 0;
    uint32_t sampleCount = 0;
};

//==============================================================================

std::unique_ptr<GraphicsContext> yup_constructOpenGLGraphicsContext (GpuDevice::Options options, GpuDevice::Ptr existingGpu)
{
    return std::make_unique<GraphicsContextOpenGL> (options, std::move (existingGpu));
}

} // namespace yup
#endif
