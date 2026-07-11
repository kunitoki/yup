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
#include "rive/renderer/ore/ore_context_gl.hpp"
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
        if (strcmp (message,
                    "API_ID_REDUNDANT_FBO performance warning has been generated. Redundant state "
                    "change in glBindFramebuffer API call, FBO 0, \"\", already bound.")
            == 0)
        {
            return;
        }

        if (strstr (message, "is being recompiled based on GL state."))
        {
            return;
        }

        printf ("GL PERF: %s\n", message);
        fflush (stdout);
    }
}
#endif

//==============================================================================

/**
 * OpenGL Graphics Context implementation that renders Rive content into an
 * offscreen framebuffer with attached texture, then blits the result to the
 * main framebuffer. This approach enables optimizations like dirty rect
 * rendering and matches the approach used in other backends.
 */
class LowLevelRenderContextGL : public GraphicsContext
{
public:
    LowLevelRenderContextGL (Options options)
        : m_options (options)
    {
#if RIVE_DESKTOP_GL
        // Load the OpenGL API using glad.
        if (! gladLoadCustomLoader ((GLADloadfunc) options.loaderFunction))
        {
            fprintf (stderr, "Failed to initialize glad.\n");
            return;
        }
#endif

        m_renderContext = rive::gpu::RenderContextGLImpl::MakeContext (m_renderContextOptions);
        if (! m_renderContext)
        {
            fprintf (stderr, "Failed to create a renderer.\n");
            return;
        }

        if (options.enableOreContext)
            m_oreContext = rive::ore::ContextGL::Make();

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

    Api getApi() const noexcept override
    {
#if RIVE_ANDROID || RIVE_WEBGL
        return Api::OpenGLES;
#else
        return Api::OpenGL;
#endif
    }

    float dpiScale (void*) const override
    {
#if RIVE_DESKTOP_GL && __APPLE__
        return 2;
#elif RIVE_WEBGL && YUP_EMSCRIPTEN
        return (float) emscripten_get_device_pixel_ratio();
#else
        return 1;
#endif
    }

    rive::Factory* factory() override
    {
        return m_renderContext.get();
    }

    rive::gpu::RenderContext* renderContext() override
    {
        return m_renderContext.get();
    }

    rive::gpu::RenderTarget* renderTarget() override
    {
        return m_offscreenRenderTarget.get();
    }

    rive::ore::Context* gpuContext() const noexcept override
    {
        return m_oreContext.get();
    }

    void onSizeChanged (void* window, int width, int height, uint32_t sampleCount) override
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
        // Mid-frame GpuCanvas / ore work shares the one real GL context and unbinds the fixed texture units
        // (endOffscreen's unbindGLInternalResources wipes units 0..N). Rive's flush assumes its internal
        // textures (tessellation/gradient/feather/atlas) are still bound at those units, so rebind them
        // right before flushing.
        m_renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->invalidateGLState();

        m_renderContext->flush ({ m_offscreenRenderTarget.get() });

        m_renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->unbindGLInternalResources();

        blitToMainFramebuffer();
    }

    //==============================================================================

    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
    };

    struct OffscreenTargetGL : public RenderableTarget
    {
        int width = 0;
        int height = 0;
        rive::rcp<rive::gpu::RenderCanvas> renderCanvas;
        rive::gpu::RenderContext* renderContext = nullptr;
        rive::gpu::RenderContext* mirrorContext = nullptr;
        mutable rive::rcp<rive::gpu::Texture> sampledMirrorTex;
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

        rive::rcp<rive::gpu::Texture> getOrCreateSampledTexture() override
        {
#if defined(ORE_BACKEND_GL) && defined(RIVE_CANVAS)
            if (sampledMirrorTex != nullptr)
                return sampledMirrorTex;

            if (mirrorContext == nullptr || renderCanvas == nullptr)
                return nullptr;

            auto renderImage = renderCanvas->renderImage();
            if (renderImage == nullptr)
                return nullptr;

            if (auto sourceTex = renderImage->refTexture())
            {
                auto mirrorImage = rive::getCanvasImportMirrorGL (
                    mirrorContext, sourceTex.get(), (uint32_t) width, (uint32_t) height);

                if (mirrorImage != nullptr)
                    sampledMirrorTex = mirrorImage->refTexture();
            }

            return sampledMirrorTex;
#else
            return nullptr;
#endif
        }

        rive::rcp<rive::gpu::Texture> getSampledTexture() const override
        {
            return sampledMirrorTex;
        }
    };

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0 || m_renderContext == nullptr)
            return nullptr;

        auto renderCanvas = m_renderContext->makeRenderCanvas (static_cast<uint32_t> (width), static_cast<uint32_t> (height));
        if (renderCanvas == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetGL>();
        target->width = width;
        target->height = height;
        target->renderContext = nullptr;
        target->mirrorContext = m_renderContext.get();
        target->contextSlot = nullptr;
        target->renderCanvas = std::move (renderCanvas);
        return target;
    }

    std::unique_ptr<RenderableTarget> createRenderableTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0)
            return nullptr;

        auto* contextSlot = acquireOffscreenContext();
        if (contextSlot == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetGL>();
        target->width = width;
        target->height = height;
        target->renderContext = contextSlot->renderContext.get();
        target->mirrorContext = contextSlot->renderContext.get();
        target->contextSlot = contextSlot;

        target->renderCanvas = target->renderContext->makeRenderCanvas (static_cast<uint32_t> (width), static_cast<uint32_t> (height));
        if (target->renderCanvas == nullptr)
            return nullptr;

        return target;
    }

    void beginOffscreen (OffscreenTarget& baseTarget, const rive::gpu::RenderContext::FrameDescriptor& frameDesc) override
    {
        auto& target = static_cast<OffscreenTargetGL&> (baseTarget);

        auto renderContext = target.getRenderContext();
        if (renderContext == nullptr || target.contextSlot == nullptr || target.contextSlot->frameActive)
            return;

        renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->invalidateGLState();
        renderContext->beginFrame (frameDesc);
        target.contextSlot->frameActive = true;
    }

    void endOffscreen (OffscreenTarget& baseTarget) override
    {
        auto& target = static_cast<OffscreenTargetGL&> (baseTarget);

        auto renderContext = target.getRenderContext();
        if (renderContext == nullptr || target.contextSlot == nullptr || ! target.contextSlot->frameActive)
            return;

        // Rebind this context's internal textures right before flushing: any other context's work since beginOffscreen()
        // (another canvas, ore passes, the main context) may have unbound the shared texture units.
        renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->invalidateGLState();

        renderContext->flush ({ target.getRenderTarget() });

        renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->unbindGLInternalResources();
        target.contextSlot->frameActive = false;
    }

    bool readOffscreenPixels (OffscreenTarget& baseTarget, void* dst, size_t dstSize) override
    {
        auto& target = static_cast<OffscreenTargetGL&> (baseTarget);

        if (target.getRenderTarget() == nullptr || dst == nullptr)
            return false;

        const size_t bytesPerRow = static_cast<size_t> (target.width) * 4u;
        if (dstSize < bytesPerRow * static_cast<size_t> (target.height))
            return false;

        auto* renderTarget = static_cast<rive::gpu::RenderTargetGL*> (target.getRenderTarget());
        renderTarget->bindDestinationFramebuffer (GL_READ_FRAMEBUFFER);
        glReadPixels (0, 0, target.width, target.height, GL_RGBA, GL_UNSIGNED_BYTE, dst);
        glBindFramebuffer (GL_READ_FRAMEBUFFER, 0);

        // Flip rows top-to-bottom (GL returns bottom-to-top)
        auto* bytes = static_cast<uint8_t*> (dst);
        std::vector<uint8_t> rowBuffer (bytesPerRow);
        const int halfHeight = target.height / 2;
        for (int i = 0; i < halfHeight; ++i)
        {
            uint8_t* top = bytes + static_cast<size_t> (i) * bytesPerRow;
            uint8_t* bottom = bytes + static_cast<size_t> (target.height - 1 - i) * bytesPerRow;
            std::memcpy (rowBuffer.data(), top, bytesPerRow);
            std::memcpy (top, bottom, bytesPerRow);
            std::memcpy (bottom, rowBuffer.data(), bytesPerRow);
        }

        return true;
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
        slot->renderContext = rive::gpu::RenderContextGLImpl::MakeContext (m_renderContextOptions);
        if (slot->renderContext == nullptr)
            return nullptr;

        auto* result = slot.get();
        m_offscreenContextPool.push_back (std::move (slot));
        return result;
    }

    void createOffscreenResources()
    {
        if (m_width <= 0 || m_height <= 0)
        {
            fprintf (stderr, "createOffscreenResources: Invalid size %dx%d\n", m_width, m_height);
            return;
        }

        cleanupOffscreenResources();

        // Create offscreen texture
        glGenTextures (1, &m_offscreenTexture);
        glBindTexture (GL_TEXTURE_2D, m_offscreenTexture);
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Check for GL errors after texture creation
        GLenum error = glGetError();
        if (error != GL_NO_ERROR)
            fprintf (stderr, "GL error after texture creation: 0x%x\n", error);

        glBindTexture (GL_TEXTURE_2D, 0);

        // Create framebuffer and attach texture
        glGenFramebuffers (1, &m_offscreenFramebuffer);
        glBindFramebuffer (GL_FRAMEBUFFER, m_offscreenFramebuffer);
        glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_offscreenTexture, 0);

        // Check framebuffer completeness
        GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
            fprintf (stderr, "Offscreen framebuffer is not complete: 0x%x\n", status);

        glBindFramebuffer (GL_FRAMEBUFFER, 0);

        // Create Rive render target that uses our offscreen framebuffer
        m_offscreenRenderTarget = rive::make_rcp<rive::gpu::FramebufferRenderTargetGL> (m_width, m_height, m_offscreenFramebuffer, m_sampleCount);
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

private:
    Options m_options;
    rive::gpu::RenderContextGLImpl::ContextOptions m_renderContextOptions;
    std::unique_ptr<rive::gpu::RenderContext> m_renderContext;
    std::vector<std::unique_ptr<OffscreenContextSlot>> m_offscreenContextPool;
    std::unique_ptr<rive::ore::ContextGL> m_oreContext;
    rive::rcp<rive::gpu::RenderTargetGL> m_offscreenRenderTarget;

    // Offscreen rendering resources
    GLuint m_offscreenFramebuffer = 0;
    GLuint m_offscreenTexture = 0;
    int m_width = 0;
    int m_height = 0;
    uint32_t m_sampleCount = 0;
};

//==============================================================================

std::unique_ptr<GraphicsContext> yup_constructOpenGLGraphicsContext (GraphicsContext::Options options)
{
    return std::make_unique<LowLevelRenderContextGL> (options);
}

} // namespace yup
#endif
