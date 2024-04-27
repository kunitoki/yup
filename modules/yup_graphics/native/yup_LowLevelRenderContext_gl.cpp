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

#include "yup_LowLevelRenderContext.h"

#if 0

#include "path_fiddle.hpp"

#include "rive/pls/gl/gles3.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/gl/pls_render_context_gl_impl.hpp"
#include "rive/pls/gl/pls_render_target_gl.hpp"

#ifdef RIVE_WEBGL
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace juce
{

using namespace rive;
using namespace rive::pls;

#ifdef RIVE_DESKTOP_GL
#ifdef DEBUG
static void GLAPIENTRY err_msg_callback(GLenum source,
                                        GLenum type,
                                        GLuint id,
                                        GLenum severity,
                                        GLsizei length,
                                        const GLchar* message,
                                        const void* userParam)
{
    if (type == GL_DEBUG_TYPE_ERROR)
    {
        printf("GL ERROR: %s\n", message);
        fflush(stdout);
        assert(0);
    }
    else if (type == GL_DEBUG_TYPE_PERFORMANCE)
    {
        if (strcmp(message,
                   "API_ID_REDUNDANT_FBO performance warning has been generated. Redundant state "
                   "change in glBindFramebuffer API call, FBO 0, \"\", already bound.") == 0)
        {
            return;
        }
        if (strstr(message, "is being recompiled based on GL state."))
        {
            return;
        }
        printf("GL PERF: %s\n", message);
        fflush(stdout);
    }
}
#endif
#endif

class LowLevelRenderContextGL : public LowLevelRenderContext
{
public:
    LowLevelRenderContextGL()
    {
#ifdef RIVE_DESKTOP_GL
        // Load the OpenGL API using glad.
        if (!gladLoadCustomLoader((GLADloadproc)glfwGetProcAddress))
        {
            fprintf(stderr, "Failed to initialize glad.\n");
            exit(-1);
        }
#endif

        printf("GL_VENDOR: %s\n", glGetString(GL_VENDOR));
        printf("GL_RENDERER: %s\n", glGetString(GL_RENDERER));
        printf("GL_VERSION: %s\n", glGetString(GL_VERSION));
#ifdef RIVE_DESKTOP_GL
        printf("GL_ANGLE_shader_pixel_local_storage_coherent: %i\n",
               GLAD_GL_ANGLE_shader_pixel_local_storage_coherent);
#endif
#if 0
        int n;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (size_t i = 0; i < n; ++i)
        {
            printf("  %s\n", glGetStringi(GL_EXTENSIONS, i));
        }
#endif

#ifdef RIVE_DESKTOP_GL
#ifdef DEBUG
        if (GLAD_GL_KHR_debug)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
            glDebugMessageCallbackKHR(&err_msg_callback, nullptr);
        }
#endif
#endif
    }

    ~LowLevelRenderContextGL() { glDeleteFramebuffers(1, &m_zoomWindowFBO); }

    float dpiScale(void*) const override
    {
#ifdef __APPLE__
        return 2;
#else
        return 1;
#endif
    }

    void end(GLFWwindow* window, std::vector<uint8_t>* pixelData) final
    {
        assert(pixelData == nullptr); // Not implemented yet.

        onEnd();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (m_zoomWindowFBO)
        {
            // Blit the zoom window.
            double xd, yd;
            glfwGetCursorPos(window, &xd, &yd);
            xd *= dpiScale(window);
            yd *= dpiScale(window);
            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            int x = xd, y = height - yd;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_zoomWindowFBO);
            glBlitFramebuffer(x - kZoomWindowWidth / 2,
                              y - kZoomWindowHeight / 2,
                              x + kZoomWindowWidth / 2,
                              y + kZoomWindowHeight / 2,
                              0,
                              0,
                              kZoomWindowWidth,
                              kZoomWindowHeight,
                              GL_COLOR_BUFFER_BIT,
                              GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glEnable(GL_SCISSOR_TEST);
            glScissor(0,
                      0,
                      kZoomWindowWidth * kZoomWindowScale + 2,
                      kZoomWindowHeight * kZoomWindowScale + 2);
            glClearColor(.6, .6, .6, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_zoomWindowFBO);
            glBlitFramebuffer(0,
                              0,
                              kZoomWindowWidth,
                              kZoomWindowHeight,
                              0,
                              0,
                              kZoomWindowWidth * kZoomWindowScale,
                              kZoomWindowHeight * kZoomWindowScale,
                              GL_COLOR_BUFFER_BIT,
                              GL_NEAREST);
            glDisable(GL_SCISSOR_TEST);
        }
    }

protected:
    virtual void onEnd() = 0;

private:
    GLuint m_zoomWindowFBO = 0;
};

class LowLevelRenderContextGLPLS : public LowLevelRenderContextGL
{
public:
    LowLevelRenderContextGLPLS()
    {
        if (!m_plsContext)
        {
            fprintf(stderr, "Failed to create a PLS renderer.\n");
            exit(-1);
        }
    }

    rive::Factory* factory() override { return m_plsContext.get(); }

    rive::pls::PLSRenderContext* plsContextOrNull() override { return m_plsContext.get(); }

    rive::pls::PLSRenderTarget* plsRenderTargetOrNull() override { return m_renderTarget.get(); }

    void onSizeChanged(GLFWwindow* window, int width, int height, uint32_t sampleCount) override
    {
        m_renderTarget = make_rcp<FramebufferRenderTargetGL>(width, height, 0, sampleCount);
    }

    std::unique_ptr<Renderer> makeRenderer(int width, int height) override
    {
        return std::make_unique<PLSRenderer>(m_plsContext.get());
    }

    void begin(const PLSRenderContext::FrameDescriptor& frameDescriptor) override
    {
        m_plsContext->static_impl_cast<PLSRenderContextGLImpl>()->invalidateGLState();
        m_plsContext->beginFrame(frameDescriptor);
    }

    void onEnd() override
    {
        flushPLSContext();
        m_plsContext->static_impl_cast<PLSRenderContextGLImpl>()->unbindGLInternalResources();
    }

    void flushPLSContext() override { m_plsContext->flush({.renderTarget = m_renderTarget.get()}); }

private:
    std::unique_ptr<PLSRenderContext> m_plsContext =
        PLSRenderContextGLImpl::MakeContext(PLSRenderContextGLImpl::ContextOptions());

    rcp<PLSRenderTargetGL> m_renderTarget;
};

std::unique_ptr<LowLevelRenderContext> LowLevelRenderContext::makeGLPLS()
{
    return std::make_unique<LowLevelRenderContextGLPLS>();
}

} // namespace juce

#else

namespace juce
{

std::unique_ptr<LowLevelRenderContext> LowLevelRenderContext::makeGLPLS()
{
    return nullptr;
}

} // namespace juce

#endif
