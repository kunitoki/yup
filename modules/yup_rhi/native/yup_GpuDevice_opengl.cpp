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

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_WASM || YUP_ANDROID
#include "rive/renderer/gl/gles3.hpp"
#include "rive/renderer/gl/render_context_gl_impl.hpp"
#include "rive/renderer/gl/render_target_gl.hpp"
#include "rive/renderer/ore/ore_context_gl.hpp"

#include <atomic>
#include <vector>
#include <cstring>
#include <functional>
#include <optional>
#include <type_traits>

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
        fprintf (stderr, "GL ERROR: %s\n", message);
        fflush (stderr);
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

        fprintf (stderr, "GL PERF: %s\n", message);
        fflush (stderr);
    }
}
#endif

//==============================================================================
class GpuDeviceGL : public GpuDevice
{
public:
    //==============================================================================
    GpuDeviceGL (Options options)
        : options (options)
    {
#if RIVE_DESKTOP_GL
        if (! gladLoadCustomLoader ((GLADloadfunc) options.loaderFunction))
        {
            fprintf (stderr, "Failed to initialize glad.\n");
            return;
        }
#endif

        renderContext = rive::gpu::RenderContextGLImpl::MakeContext (renderContextOptions);
        if (! renderContext)
        {
            fprintf (stderr, "Failed to create a renderer.\n");
            return;
        }

        oreContext = rive::ore::ContextGL::Make();

#if YUP_ENABLE_GL_VERBOSE
        printf ("GL_VENDOR:   %s\n", glGetString (GL_VENDOR));
        printf ("GL_RENDERER: %s\n", glGetString (GL_RENDERER));
        printf ("GL_VERSION:  %s\n", glGetString (GL_VERSION));

#if RIVE_DESKTOP_GL
        printf ("GL_ANGLE_shader_pixel_local_storage_coherent: %i\n", GLAD_GL_ANGLE_shader_pixel_local_storage_coherent);
#endif

#if ! RIVE_ANDROID
        int n;
        glGetIntegerv (GL_NUM_EXTENSIONS, &n);
        for (size_t i = 0; i < n; ++i)
            printf ("  %s\n", glGetStringi (GL_EXTENSIONS, i));
#endif
#endif // YUP_ENABLE_GL_VERBOSE

#if RIVE_DESKTOP_GL && DEBUG
        if (GLAD_GL_KHR_debug)
        {
            glEnable (GL_DEBUG_OUTPUT_KHR);
            glDebugMessageControlKHR (GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            glDebugMessageCallbackKHR (&err_msg_callback, nullptr);
        }
#endif
    }

    ~GpuDeviceGL() override { releasePooledResources(); }

    //==============================================================================

    GpuPlatform getPlatform() const noexcept override
    {
#if RIVE_ANDROID || RIVE_WEBGL
        return GpuPlatform::OpenGLES;
#else
        return GpuPlatform::OpenGL;
#endif
    }

    rive::gpu::RenderContext* getRenderContext() const override { return renderContext.get(); }

    rive::ore::Context* getGpuContext() const noexcept override { return oreContext.get(); }

    //==============================================================================

    bool isComputeAvailable() const noexcept override
    {
        return withGLContext ([&]() -> bool
        {
            // GL 4.3+ and GLES 3.1+ support compute shaders natively.
            // Probe the version string at runtime.
            const auto* version = (const char*) glGetString (GL_VERSION);
            if (version == nullptr)
                return false;

            // GLES: "OpenGL ES 3.1" or higher
            if (strstr (version, "OpenGL ES") != nullptr)
            {
                int major = 0, minor = 0;
                if (sscanf (version, "OpenGL ES %d.%d", &major, &minor) == 2)
                    return major > 3 || (major == 3 && minor >= 1);
            }

            // Desktop GL: "4.3" or higher
            int major = 0, minor = 0;
            if (sscanf (version, "%d.%d", &major, &minor) == 2)
                return major > 4 || (major == 4 && minor >= 3);

            return false;
        });
    }

    //==============================================================================

    ReferenceCountedObjectPtr<GpuBuffer> createBuffer (GpuBufferType type, const void* data, size_t byteSize) override
    {
        if (type == GpuBufferType::storage)
        {
            return withComputeContext ([&]() -> ReferenceCountedObjectPtr<GpuBuffer>
            {
                jassert (data != nullptr && byteSize > 0);
                if (data == nullptr || byteSize == 0)
                    return nullptr;

                GLuint buf = 0;
                glGenBuffers (1, &buf);
                if (buf == 0)
                    return nullptr;

                glBindBuffer (GL_SHADER_STORAGE_BUFFER, buf);
                glBufferData (GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr> (byteSize), data, GL_DYNAMIC_COPY);
                glBindBuffer (GL_SHADER_STORAGE_BUFFER, 0);

                return GpuBuffer::createWithImpl (GpuBuffer::Impl { .type = type, .byteSize = byteSize, .glStorageBuffer = { buf, this } });
            });
        }

        return withGLContext ([&]() -> ReferenceCountedObjectPtr<GpuBuffer>
        {
            return GpuDevice::createBuffer (type, data, byteSize);
        });
    }

    //==============================================================================

    bool readBuffer (GpuBuffer::Ptr buffer, void* dst, size_t dstSize) override
    {
#if YUP_WASM
        // WebGL 2.0 (GLES 3.0) has no GL_SHADER_STORAGE_BUFFER — fall back to base.
        return withGLContext ([&]() -> bool
        {
            return GpuDevice::readBuffer (std::move (buffer), dst, dstSize);
        });
#else
        return withComputeContext ([&]() -> bool
        {
            if (buffer == nullptr || dst == nullptr)
                return false;

            auto* impl = buffer->getImpl();
            if (impl == nullptr || impl->glStorageBuffer.id == 0)
                return false;

            const auto byteSize = buffer->getSizeInBytes();
            if (dstSize < byteSize)
                return false;

            glFinish();
            glBindBuffer (GL_SHADER_STORAGE_BUFFER, impl->glStorageBuffer.id);
            void* mapped = glMapBufferRange (GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr> (byteSize), GL_MAP_READ_BIT);
            if (mapped != nullptr)
            {
                std::memcpy (dst, mapped, byteSize);
                glUnmapBuffer (GL_SHADER_STORAGE_BUFFER);
            }
            glBindBuffer (GL_SHADER_STORAGE_BUFFER, 0);
            return mapped != nullptr;
        });
#endif
    }

    //==============================================================================

    bool updateBuffer (GpuBuffer::Ptr buffer, const void* data, size_t byteSize) override
    {
        if (buffer == nullptr || data == nullptr || byteSize == 0)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr)
            return false;

        // For native GL storage buffers, use glBufferSubData on the compute context.
        if (impl->glStorageBuffer.id != 0)
        {
            return withComputeContext ([&]() -> bool
            {
                if (byteSize > buffer->getSizeInBytes())
                    return false;

                glBindBuffer (GL_SHADER_STORAGE_BUFFER, impl->glStorageBuffer.id);
                glBufferSubData (GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr> (byteSize), data);
                glBindBuffer (GL_SHADER_STORAGE_BUFFER, 0);
                return true;
            });
        }

        // For ore-backed buffers (vertex, index, uniform), delegate to base class.
        return withGLContext ([&]() -> bool
        {
            return GpuDevice::updateBuffer (buffer, data, byteSize);
        });
    }

    //==============================================================================

    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
        bool leased = false;
    };

    struct OffscreenTargetGL : public RenderableTarget
    {
        ~OffscreenTargetGL() override
        {
            if (contextSlot != nullptr)
                contextSlot->leased = false;
        }

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
        return withGLContext ([&]() -> std::unique_ptr<OffscreenTarget>
        {
            if (width <= 0 || height <= 0 || renderContext == nullptr)
                return nullptr;

            auto renderCanvas = renderContext->makeRenderCanvas (static_cast<uint32_t> (width), static_cast<uint32_t> (height));
            if (renderCanvas == nullptr)
                return nullptr;

            auto target = std::make_unique<OffscreenTargetGL>();
            target->width = width;
            target->height = height;
            target->renderContext = nullptr;
            target->mirrorContext = renderContext.get();
            target->contextSlot = nullptr;
            target->renderCanvas = std::move (renderCanvas);
            return target;
        });
    }

    std::unique_ptr<RenderableTarget> createRenderableTarget (int width, int height) override
    {
        return withGLContext ([&]() -> std::unique_ptr<RenderableTarget>
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
        });
    }

    void beginOffscreen (OffscreenTarget& baseTarget, const rive::gpu::RenderContext::FrameDescriptor& frameDesc) override
    {
        withGLContext ([&]
        {
            auto& target = static_cast<OffscreenTargetGL&> (baseTarget);

            auto renderContext = target.getRenderContext();
            if (renderContext == nullptr || target.contextSlot == nullptr || target.contextSlot->frameActive)
                return;

            renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->invalidateGLState();
            renderContext->beginFrame (frameDesc);
            target.contextSlot->frameActive = true;
        });
    }

    void endOffscreen (OffscreenTarget& baseTarget) override
    {
        withGLContext ([&]
        {
            auto& target = static_cast<OffscreenTargetGL&> (baseTarget);

            auto renderContext = target.getRenderContext();
            if (renderContext == nullptr || target.contextSlot == nullptr || ! target.contextSlot->frameActive)
                return;

            renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->invalidateGLState();
            renderContext->flush ({ target.getRenderTarget() });
            renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->unbindGLInternalResources();
            target.contextSlot->frameActive = false;
        });
    }

    bool clearOffscreen (OffscreenTarget& baseTarget, GpuColor color) override
    {
        return withGLContext ([&]() -> bool
        {
            auto& target = static_cast<OffscreenTargetGL&> (baseTarget);

            auto* renderTarget = static_cast<rive::gpu::RenderTargetGL*> (target.getRenderTarget());
            if (renderTarget == nullptr)
                return false;

            // GL state is global and a canvas may be created part-way through a frame,
            // so restore the draw framebuffer afterwards rather than leaving ours bound.
            // Scissoring is disabled for the same reason: an enclosing scissor rect
            // would otherwise clip the clear and leave part of the canvas undefined.
            GLint previousFramebuffer = 0;
            glGetIntegerv (GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);

            const GLboolean scissorWasEnabled = glIsEnabled (GL_SCISSOR_TEST);
            if (scissorWasEnabled)
                glDisable (GL_SCISSOR_TEST);

            renderTarget->bindDestinationFramebuffer (GL_DRAW_FRAMEBUFFER);
            glClearColor (color.red, color.green, color.blue, color.alpha);
            glClear (GL_COLOR_BUFFER_BIT);

            if (scissorWasEnabled)
                glEnable (GL_SCISSOR_TEST);

            glBindFramebuffer (GL_DRAW_FRAMEBUFFER, static_cast<GLuint> (previousFramebuffer));

            return true;
        });
    }

    bool readOffscreenPixels (OffscreenTarget& baseTarget, void* dst, size_t dstSize) override
    {
        return withGLContext ([&]() -> bool
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

            // Flip vertically: OpenGL framebuffer origin is bottom-left.
            offscreenPixelsRow.resize (bytesPerRow);
            auto* bytes = static_cast<uint8_t*> (dst);
            const int halfHeight = target.height / 2;
            for (int i = 0; i < halfHeight; ++i)
            {
                uint8_t* top = bytes + static_cast<size_t> (i) * bytesPerRow;
                uint8_t* bottom = bytes + static_cast<size_t> (target.height - 1 - i) * bytesPerRow;
                std::memcpy (offscreenPixelsRow.data(), top, bytesPerRow);
                std::memcpy (top, bottom, bytesPerRow);
                std::memcpy (bottom, offscreenPixelsRow.data(), bytesPerRow);
            }

            return true;
        });
    }

    //==============================================================================

    void runOnComputeContext (const std::function<void()>& fn) const override
    {
        withComputeContext ([&]
        {
            fn();
        });
    }

private:
    /** Runs @a fn with the given context activator current on this thread. With a
        null activator, @a fn runs directly — the caller is responsible for having
        a current context, as before. */
    template <class Fn>
    static std::invoke_result_t<Fn> withActivator (const std::function<void (const std::function<void()>&)>& activator, Fn&& fn)
    {
        using Result = std::invoke_result_t<Fn>;

        if (! activator)
            return std::invoke (std::forward<Fn> (fn));

        if constexpr (std::is_void_v<Result>)
        {
            activator ([&]
            {
                std::invoke (std::forward<Fn> (fn));
            });
        }
        else
        {
            std::optional<Result> result;
            activator ([&]
            {
                result.emplace (std::invoke (std::forward<Fn> (fn)));
            });
            if (! result.has_value())
                return Result {};

            return std::move (*result);
        }
    }

    /** Runs @a fn with the GL rendering context current on this thread
        (see GpuDevice::Options::contextActivator). */
    template <class Fn>
    std::invoke_result_t<Fn> withGLContext (Fn&& fn) const
    {
        return withActivator (options.contextActivator, std::forward<Fn> (fn));
    }

    /** Runs @a fn with the dedicated GL compute context current on this thread
        (see GpuDevice::Options::computeContextActivator), falling back to the
        rendering context when no compute activator was provided. */
    template <class Fn>
    std::invoke_result_t<Fn> withComputeContext (Fn&& fn) const
    {
        if (options.computeContextActivator)
            return withActivator (options.computeContextActivator, std::forward<Fn> (fn));

        return withActivator (options.contextActivator, std::forward<Fn> (fn));
    }

    OffscreenContextSlot* acquireOffscreenContext()
    {
        for (const auto& slot : offscreenContextPool)
        {
            if (! slot->leased)
            {
                slot->leased = true;
                return slot.get();
            }
        }

        auto slot = std::make_unique<OffscreenContextSlot>();
        slot->renderContext = rive::gpu::RenderContextGLImpl::MakeContext (renderContextOptions);
        if (slot->renderContext == nullptr)
            return nullptr;

        slot->leased = true;

        auto* result = slot.get();
        offscreenContextPool.push_back (std::move (slot));
        return result;
    }

    Options options;
    rive::gpu::RenderContextGLImpl::ContextOptions renderContextOptions;
    std::unique_ptr<rive::gpu::RenderContext> renderContext;
    std::vector<std::unique_ptr<OffscreenContextSlot>> offscreenContextPool;
    std::unique_ptr<rive::ore::ContextGL> oreContext;
    std::vector<uint8_t> offscreenPixelsRow;
};

//==============================================================================

std::unique_ptr<GpuDevice> yup_constructOpenGLGpuDevice (GpuDevice::Options options)
{
    return std::make_unique<GpuDeviceGL> (options);
}

} // namespace yup
#endif
