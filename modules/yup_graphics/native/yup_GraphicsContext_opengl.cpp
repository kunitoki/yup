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

#if RIVE_DESKTOP_GL && DEBUG
static void GLAPIENTRY err_msg_callback (GLenum source,
                                         GLenum type,
                                         GLuint id,
                                         GLenum severity,
                                         GLsizei length,
                                         const GLchar* message,
                                         const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR_KHR)
    {
        printf ("GL ERROR: %s\n", message);
        fflush (stdout);
        assert (false);
    }
    else if (type == GL_DEBUG_TYPE_PERFORMANCE_KHR)
    {
        if (strcmp (message, "API_ID_REDUNDANT_FBO performance warning has been generated. Redundant state "
                             "change in glBindFramebuffer API call, FBO 0, \"\", already bound.")
            == 0)
            return;
        if (strstr (message, "is being recompiled based on GL state."))
            return;
        printf ("GL PERF: %s\n", message);
        fflush (stdout);
    }
}
#endif

//==============================================================================

class LowLevelRenderContextGL : public GraphicsContext
{
public:
    LowLevelRenderContextGL (Options options, GpuDevice::Ptr existingGpu = {})
        : m_options (options)
    {
#if RIVE_DESKTOP_GL
        if (! gladLoadCustomLoader ((GLADloadfunc) options.loaderFunction))
        {
            fprintf (stderr, "Failed to initialize glad.\n");
            return;
        }
#endif

        // Obtain or create the GpuDevice for offscreen/RHI operations
        if (existingGpu != nullptr)
            m_gpuContext = std::move (existingGpu);
        else
            m_gpuContext = GpuDevice::create (getPlatform(), options);

        // Create the main window render context
        m_renderContext = rive::gpu::RenderContextGLImpl::MakeContext (m_renderContextOptions);
        if (! m_renderContext)
        {
            fprintf (stderr, "Failed to create a renderer.\n");
            return;
        }

        printf ("GL_VENDOR:   %s\n", glGetString (GL_VENDOR));
        printf ("GL_RENDERER: %s\n", glGetString (GL_RENDERER));
        printf ("GL_VERSION:  %s\n", glGetString (GL_VERSION));

#if RIVE_DESKTOP_GL
        printf ("GL_ANGLE_shader_pixel_local_storage_coherent: %i\n", GLAD_GL_ANGLE_shader_pixel_local_storage_coherent);
#if DEBUG
        if (GLAD_GL_KHR_debug)
        {
            glEnable (GL_DEBUG_OUTPUT_KHR);
            glDebugMessageControlKHR (GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            glDebugMessageCallbackKHR (&err_msg_callback, nullptr);
        }
#endif
#endif

#if DEBUG && ! RIVE_ANDROID
        int n;
        glGetIntegerv (GL_NUM_EXTENSIONS, &n);
        for (size_t i = 0; i < n; ++i)
            printf ("  %s\n", glGetStringi (GL_EXTENSIONS, i));
#endif
    }

    ~LowLevelRenderContextGL()
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

    GpuDevice::Ptr getGpuDevice() const noexcept override { return m_gpuContext; }

    rive::Factory* factory() override { return m_renderContext.get(); }

    rive::gpu::RenderContext* renderContext() override { return m_renderContext.get(); }

    rive::gpu::RenderTarget* renderTarget() override { return m_offscreenRenderTarget.get(); }

    void onSizeChanged (void* window, int width, int height, float dpiScale, uint32_t sampleCount) override
    {
        m_width = width;
        m_height = height;
        m_sampleCount = sampleCount;
        createOffscreenResources();
    }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (m_renderContext.get());
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        m_renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->invalidateGLState();
        m_renderContext->beginFrame (frameDescriptor);
    }

    void end (void*) override
    {
        m_renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->invalidateGLState();
        m_renderContext->flush ({ m_offscreenRenderTarget.get() });
        m_renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->unbindGLInternalResources();
        blitToMainFramebuffer();
    }

private:
    void createOffscreenResources()
    {
        if (m_width <= 0 || m_height <= 0)
        {
            fprintf (stderr, "createOffscreenResources: Invalid size %dx%d\n", m_width, m_height);
            return;
        }

        cleanupOffscreenResources();

        glGenTextures (1, &m_offscreenTexture);
        glBindTexture (GL_TEXTURE_2D, m_offscreenTexture);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
            fprintf (stderr, "GL error after texture creation: 0x%x\n", error);

        glBindTexture (GL_TEXTURE_2D, 0);

        glGenFramebuffers (1, &m_offscreenFramebuffer);
        glBindFramebuffer (GL_FRAMEBUFFER, m_offscreenFramebuffer);
        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_offscreenTexture, 0);

        GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            fprintf (stderr, "Offscreen framebuffer is not complete: 0x%x\n", status);

        glBindFramebuffer (GL_FRAMEBUFFER, 0);

        m_offscreenRenderTarget = rive::make_rcp<rive::gpu::FramebufferRenderTargetGL> (
            m_width, m_height, m_offscreenFramebuffer, m_sampleCount);
    }

    void cleanupOffscreenResources()
    {
        if (m_offscreenFramebuffer != 0)
        {
            glDeleteFramebuffers (1, &m_offscreenFramebuffer);
            m_offscreenFramebuffer = 0;
        }
        if (m_offscreenTexture != 0)
        {
            glDeleteTextures (1, &m_offscreenTexture);
            m_offscreenTexture = 0;
        }
        m_offscreenRenderTarget.reset();
    }

    void blitToMainFramebuffer()
    {
        if (m_offscreenTexture == 0)
        {
            fprintf (stderr, "blitToMainFramebuffer: Invalid program or texture\n");
            return;
        }
        glBindFramebuffer (GL_READ_FRAMEBUFFER, m_offscreenFramebuffer);
        glBindFramebuffer (GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer (0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    Options m_options;
    rive::gpu::RenderContextGLImpl::ContextOptions m_renderContextOptions;
    GpuDevice::Ptr m_gpuContext;
    std::unique_ptr<rive::gpu::RenderContext> m_renderContext;
    rive::rcp<rive::gpu::RenderTargetGL> m_offscreenRenderTarget;

    GLuint m_offscreenFramebuffer = 0;
    GLuint m_offscreenTexture = 0;
    int m_width = 0;
    int m_height = 0;
    uint32_t m_sampleCount = 0;
};

//==============================================================================

std::unique_ptr<GraphicsContext> yup_constructOpenGLGraphicsContext (GpuDevice::Options options, GpuDevice::Ptr existingGpu)
{
    return std::make_unique<LowLevelRenderContextGL> (options, std::move (existingGpu));
}

} // namespace yup
#endif
