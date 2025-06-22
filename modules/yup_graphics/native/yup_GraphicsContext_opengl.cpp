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
    if (type == GL_DEBUG_TYPE_ERROR)
    {
        printf ("GL ERROR: %s\n", message);
        fflush (stdout);

        assert (false);
    }
    else if (type == GL_DEBUG_TYPE_PERFORMANCE)
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

namespace
{

//==============================================================================

struct Vertex
{
    float position[2];
    float texCoord[2];
};

// Full-screen quad covering clip space, with texture coordinates mapping the texture.
const Vertex quadVertices[] = {
    { { -1.0f, 1.0f }, { 0.0f, 0.0f } },  // Top-left
    { { -1.0f, -1.0f }, { 0.0f, 1.0f } }, // Bottom-left
    { { 1.0f, 1.0f }, { 1.0f, 0.0f } },   // Top-right
    { { 1.0f, -1.0f }, { 1.0f, 1.0f } }   // Bottom-right
};

const char* getVertexShaderSource (bool isGLES)
{
    if (isGLES)
        return R"(#version 300 es
precision highp float;

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

out vec2 vTexCoord;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    vTexCoord = texCoord;
}
)";
    else
        return R"(#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

out vec2 vTexCoord;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    vTexCoord = texCoord;
}
)";
}

const char* getFragmentShaderSource (bool isGLES)
{
    if (isGLES)
        return R"(#version 300 es
precision highp float;
precision highp sampler2D;

in vec2 vTexCoord;
uniform sampler2D uTexture;

out vec4 fragColor;

void main()
{
    // Fix Y-flip by inverting the Y coordinate
    vec2 flippedCoord = vec2(vTexCoord.x, 1.0 - vTexCoord.y);
    fragColor = texture(uTexture, flippedCoord);
}
)";
    else
        return R"(#version 330 core

in vec2 vTexCoord;
uniform sampler2D uTexture;

out vec4 fragColor;

void main()
{
    // Fix Y-flip by inverting the Y coordinate
    vec2 flippedCoord = vec2(vTexCoord.x, 1.0 - vTexCoord.y);
    fragColor = texture(uTexture, flippedCoord);
}
)";
}

GLuint compileShader (GLenum type, const char* source)
{
    GLuint shader = glCreateShader (type);
    glShaderSource (shader, 1, &source, nullptr);
    glCompileShader (shader);

    GLint compiled = 0;
    glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog (maxLength);
        glGetShaderInfoLog (shader, maxLength, &maxLength, &infoLog[0]);

        printf ("Shader compilation failed: %s\n", &infoLog[0]);
        glDeleteShader (shader);
        return 0;
    }

    return shader;
}

GLuint createBlitProgram (bool isGLES)
{
    GLuint vertexShader = compileShader (GL_VERTEX_SHADER, getVertexShaderSource (isGLES));
    if (vertexShader == 0)
        return 0;

    GLuint fragmentShader = compileShader (GL_FRAGMENT_SHADER, getFragmentShaderSource (isGLES));
    if (fragmentShader == 0)
    {
        glDeleteShader (vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader (program, vertexShader);
    glAttachShader (program, fragmentShader);
    glLinkProgram (program);

    glDeleteShader (vertexShader);
    glDeleteShader (fragmentShader);

    GLint linked = 0;
    glGetProgramiv (program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv (program, GL_INFO_LOG_LENGTH, &maxLength);

        std::vector<GLchar> infoLog (maxLength);
        glGetProgramInfoLog (program, maxLength, &maxLength, &infoLog[0]);

        printf ("Program linking failed: %s\n", &infoLog[0]);
        glDeleteProgram (program);
        return 0;
    }

    return program;
}

} // namespace

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
        if (! gladLoadCustomLoader ((GLADloadproc) options.loaderFunction))
        {
            fprintf (stderr, "Failed to initialize glad.\n");
            return;
        }
#endif

        m_renderContext = rive::gpu::RenderContextGLImpl::MakeContext (rive::gpu::RenderContextGLImpl::ContextOptions());
        if (! m_renderContext)
        {
            fprintf (stderr, "Failed to create a renderer.\n");
            return;
        }

        printf ("GL_VENDOR:   %s\n", glGetString (GL_VENDOR));
        printf ("GL_RENDERER: %s\n", glGetString (GL_RENDERER));
        printf ("GL_VERSION:  %s\n", glGetString (GL_VERSION));

#if RIVE_DESKTOP_GL
        m_isGLES = strstr ((const char*) glGetString (GL_VERSION), "OpenGL ES") != nullptr;

        printf ("GL_ANGLE_shader_pixel_local_storage_coherent: %i\n", GLAD_GL_ANGLE_shader_pixel_local_storage_coherent);

#if DEBUG
        if (GLAD_GL_KHR_debug)
        {
            glEnable (GL_DEBUG_OUTPUT);
            glDebugMessageControlKHR (GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            glDebugMessageCallbackKHR (&err_msg_callback, nullptr);
        }
#endif
#else
        m_isGLES = true;
#endif

#if DEBUG && ! RIVE_ANDROID
        int n;
        glGetIntegerv (GL_NUM_EXTENSIONS, &n);
        for (size_t i = 0; i < n; ++i)
            printf ("  %s\n", glGetStringi (GL_EXTENSIONS, i));
#endif

        initializeBlitResources();
    }

    ~LowLevelRenderContextGL()
    {
        cleanupBlitResources();
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
        rive::gpu::RenderContext::FlushResources flushResources;
        flushResources.renderTarget = m_offscreenRenderTarget.get();
        m_renderContext->flush (flushResources);

        m_renderContext->static_impl_cast<rive::gpu::RenderContextGLImpl>()->unbindGLInternalResources();

        blitToMainFramebuffer();
    }

private:
    void initializeBlitResources()
    {
        // Create blit shader program
        m_blitProgram = createBlitProgram (m_isGLES);
        if (m_blitProgram == 0)
        {
            fprintf (stderr, "Failed to create blit shader program.\n");
            return;
        }

        // Get uniform location
        m_textureUniformLocation = glGetUniformLocation (m_blitProgram, "uTexture");

        // Create vertex buffer for fullscreen quad
        glGenBuffers (1, &m_quadVertexBuffer);
        glBindBuffer (GL_ARRAY_BUFFER, m_quadVertexBuffer);
        glBufferData (GL_ARRAY_BUFFER, sizeof (quadVertices), quadVertices, GL_STATIC_DRAW);

        // Create vertex array object
        glGenVertexArrays (1, &m_quadVAO);
        glBindVertexArray (m_quadVAO);

        // Setup vertex attributes
        glBindBuffer (GL_ARRAY_BUFFER, m_quadVertexBuffer);

        // Position attribute
        glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, sizeof (Vertex), (void*) offsetof (Vertex, position));
        glEnableVertexAttribArray (0);

        // Texture coordinate attribute
        glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, sizeof (Vertex), (void*) offsetof (Vertex, texCoord));
        glEnableVertexAttribArray (1);

        // Unbind
        glBindVertexArray (0);
        glBindBuffer (GL_ARRAY_BUFFER, 0);
    }

    void cleanupBlitResources()
    {
        if (m_quadVAO != 0)
        {
            glDeleteVertexArrays (1, &m_quadVAO);
            m_quadVAO = 0;
        }

        if (m_quadVertexBuffer != 0)
        {
            glDeleteBuffers (1, &m_quadVertexBuffer);
            m_quadVertexBuffer = 0;
        }

        if (m_blitProgram != 0)
        {
            glDeleteProgram (m_blitProgram);
            m_blitProgram = 0;
        }

        cleanupOffscreenResources();
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
        if (m_blitProgram == 0 || m_offscreenTexture == 0)
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
    std::unique_ptr<rive::gpu::RenderContext> m_renderContext;
    rive::rcp<rive::gpu::RenderTargetGL> m_offscreenRenderTarget;

    // Offscreen rendering resources
    GLuint m_offscreenFramebuffer = 0;
    GLuint m_offscreenTexture = 0;
    int m_width = 0;
    int m_height = 0;
    uint32_t m_sampleCount = 0;

    // Blit resources
    GLuint m_blitProgram = 0;
    GLuint m_quadVertexBuffer = 0;
    GLuint m_quadVAO = 0;
    GLint m_textureUniformLocation = -1;

    bool m_isGLES = false;
};

//==============================================================================

std::unique_ptr<GraphicsContext> yup_constructOpenGLGraphicsContext (GraphicsContext::Options options)
{
    return std::make_unique<LowLevelRenderContextGL> (options);
}

} // namespace yup
#endif