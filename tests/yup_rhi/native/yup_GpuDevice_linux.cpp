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

#include <gtest/gtest.h>

#include <yup_rhi/yup_rhi.h>
#include <yup_graphics/yup_graphics.h>

#include <SDL3/SDL.h>

#include <cstdlib>

using namespace yup;

namespace
{

// ==============================================================================
// Helper: create a GpuDevice backed by a real OpenGL context via SDL.
//
// Creates a hidden SDL window + GL context, makes it current, and uses
// SDL_GL_GetProcAddress as the loader function for GpuDevice::Options.
// ==============================================================================

struct GLContext
{
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;

    bool init()
    {
#if YUP_LINUX
        // Force Mesa's llvmpipe software rasterizer for CI environments.
        // Real GPU drivers take precedence when available.
        setenv ("LIBGL_ALWAYS_SOFTWARE", "1", 0);
        setenv ("GALLIUM_DRIVER", "llvmpipe", 0);
#endif

        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);

        window = SDL_CreateWindow ("yup_rhi_gl_test",
                                   64,
                                   64,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (window == nullptr)
        {
            fprintf (stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
            return false;
        }

        glContext = SDL_GL_CreateContext (window);
        if (glContext == nullptr)
        {
            fprintf (stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
            SDL_DestroyWindow (window);
            window = nullptr;
            return false;
        }

        SDL_GL_MakeCurrent (window, glContext);
        return true;
    }

    void shutdown()
    {
        if (glContext != nullptr)
        {
            SDL_GL_DestroyContext (glContext);
            glContext = nullptr;
        }
        if (window != nullptr)
        {
            SDL_DestroyWindow (window);
            window = nullptr;
        }
    }

    GpuDevice::Ptr createDevice() const
    {
        GpuDevice::Options opts;
        opts.loaderFunction = (GpuDevice::LoaderFunction) SDL_GL_GetProcAddress;
        opts.enableReadPixels = true;

        auto device = GpuDevice::create (GpuPlatform::OpenGL, opts);
        if (device == nullptr)
            fprintf (stderr, "GpuDevice::create(OpenGL) returned null\n");

        return device;
    }
};

} // namespace

// ==============================================================================
// GpuDeviceOpenGL — real GPU device tests (Linux, OpenGL via SDL)
// ==============================================================================

class GpuDeviceOpenGLTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (! gl.init())
            GTEST_SKIP() << "Cannot create OpenGL context — is a display available?";
        device = gl.createDevice();
        if (device == nullptr)
            GTEST_SKIP() << "Cannot create OpenGL GpuDevice — is GL 4.5 supported?";
    }

    void TearDown() override
    {
        device = nullptr;
        gl.shutdown();
    }

    GLContext gl;
    GpuDevice::Ptr device;
};

// --------------------------------------------------------------------------
// Basic device creation
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, DeviceIsNotNull)
{
    ASSERT_NE (device, nullptr);
}

TEST_F (GpuDeviceOpenGLTests, PlatformIsOpenGL)
{
    EXPECT_EQ (device->getPlatform(), GpuPlatform::OpenGL);
}

TEST_F (GpuDeviceOpenGLTests, GpuContextIsNotNull)
{
    EXPECT_NE (device->gpuContext(), nullptr);
}

// --------------------------------------------------------------------------
// Buffer creation
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, CreateVertexBuffer)
{
    const float verts[] = { 0.0f, 1.0f, 2.0f, 3.0f };
    auto buf = device->createBuffer (GpuBufferType::vertex, verts, sizeof (verts));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::vertex);
    EXPECT_EQ (buf->getSizeInBytes(), sizeof (verts));
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuDeviceOpenGLTests, CreateIndexBuffer)
{
    const uint16_t indices[] = { 0, 1, 2, 3 };
    auto buf = device->createBuffer (GpuBufferType::index, indices, sizeof (indices));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::index);
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuDeviceOpenGLTests, CreateUniformBuffer)
{
    const int data[] = { 42, 43, 44 };
    auto buf = device->createBuffer (GpuBufferType::uniform, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::uniform);
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuDeviceOpenGLTests, CreateBufferWithNullDataReturnsNull)
{
    EXPECT_EQ (device->createBuffer (GpuBufferType::vertex, nullptr, 16), nullptr);
}

TEST_F (GpuDeviceOpenGLTests, CreateBufferWithZeroSizeReturnsNull)
{
    const float data[] = { 1.0f };
    EXPECT_EQ (device->createBuffer (GpuBufferType::vertex, data, 0), nullptr);
}

// --------------------------------------------------------------------------
// Buffer update
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, UpdateVertexBuffer)
{
    const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buf = device->createBuffer (GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buf, nullptr);

    const float newData[] = { 5.0f, 6.0f, 7.0f, 8.0f };
    EXPECT_TRUE (device->updateBuffer (buf, newData, sizeof (newData)));
}

TEST_F (GpuDeviceOpenGLTests, UpdateBufferLargerThanOriginalReturnsFalse)
{
    const float data[] = { 1.0f };
    auto buf = device->createBuffer (GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buf, nullptr);

    const float larger[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    EXPECT_FALSE (device->updateBuffer (buf, larger, sizeof (larger)));
}

// --------------------------------------------------------------------------
// Offscreen target creation
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, CreateOffscreenTarget)
{
    auto target = device->createOffscreenTarget (256, 256);
    ASSERT_NE (target, nullptr);
    EXPECT_EQ (target->getWidth(), 256);
    EXPECT_EQ (target->getHeight(), 256);
}

TEST_F (GpuDeviceOpenGLTests, CreateOffscreenTargetZeroSize)
{
    EXPECT_EQ (device->createOffscreenTarget (0, 256), nullptr);
    EXPECT_EQ (device->createOffscreenTarget (256, 0), nullptr);
}

// --------------------------------------------------------------------------
// GpuTarget
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, GpuTargetCreate)
{
    auto target = GpuTarget::create (device, 128, 128);
    ASSERT_NE (target, nullptr);
    EXPECT_EQ (target->getWidth(), 128);
    EXPECT_EQ (target->getHeight(), 128);
}

TEST_F (GpuDeviceOpenGLTests, GpuTargetAsImage)
{
    auto target = GpuTarget::create (device, 128, 128);
    ASSERT_NE (target, nullptr);

    auto img = target->asImage();
    EXPECT_TRUE (img.isValid());
    EXPECT_EQ (img.getWidth(), 128);
    EXPECT_EQ (img.getHeight(), 128);
}

TEST_F (GpuDeviceOpenGLTests, GpuTargetAsTexture)
{
    auto target = GpuTarget::create (device, 128, 128);
    ASSERT_NE (target, nullptr);

    auto tex = target->asTexture();
    ASSERT_NE (tex, nullptr);
    EXPECT_TRUE (tex->isValid());
    EXPECT_EQ (tex->getWidth(), 128);
    EXPECT_EQ (tex->getHeight(), 128);
    EXPECT_TRUE (tex->isRenderTarget());
}

TEST_F (GpuDeviceOpenGLTests, GpuTargetReadPixels)
{
    auto target = GpuTarget::create (device, 128, 128);
    ASSERT_NE (target, nullptr);

    std::vector<uint8_t> pixels (128 * 128 * 4);
    EXPECT_TRUE (target->readPixels (pixels.data(), pixels.size()));
}

TEST_F (GpuDeviceOpenGLTests, GpuTargetReadPixelsTooSmallBufferReturnsFalse)
{
    auto target = GpuTarget::create (device, 128, 128);
    ASSERT_NE (target, nullptr);

    std::vector<uint8_t> pixels (16);
    EXPECT_FALSE (target->readPixels (pixels.data(), pixels.size()));
}

// --------------------------------------------------------------------------
// GpuFrame
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, GpuFrameBeginReturnsValidFrame)
{
    auto frame = GpuFrame::begin (device);
    EXPECT_TRUE (frame.isValid());
}

TEST_F (GpuDeviceOpenGLTests, GpuFrameSubmit)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());
    EXPECT_TRUE (frame.submit());
    EXPECT_FALSE (frame.submit()); // Idempotent after first submit.
}

TEST_F (GpuDeviceOpenGLTests, GpuFrameWaitForGPU)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());
    frame.submit();
    EXPECT_NO_THROW (frame.waitForGPU());
}

// --------------------------------------------------------------------------
// Pipeline compilation
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, CompilePipelineWithMinimalShaders)
{
    const char* vsSrc = R"(
        #version 450
        layout(set = 0, binding = 0) uniform Uniforms { mat4 mvp; } ubo;
        layout(location = 0) in vec3 aPos;
        void main() { gl_Position = ubo.mvp * vec4(aPos, 1.0); }
    )";

    const char* fsSrc = R"(
        #version 450
        layout(location = 0) out vec4 fragColor;
        void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }
    )";

    GpuShaderSource vs;
    vs.language = GpuShaderLanguage::glsl;
    vs.code = vsSrc;
    vs.codeSize = (uint32_t) strlen (vsSrc);

    GpuShaderSource fs;
    fs.language = GpuShaderLanguage::glsl;
    fs.code = fsSrc;
    fs.codeSize = (uint32_t) strlen (fsSrc);

    auto result = GpuPipeline::compile (device, vs, fs);
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
}

TEST_F (GpuDeviceOpenGLTests, CompilePipelineFailsWithEmptyVertexCode)
{
    GpuShaderSource vs;
    vs.language = GpuShaderLanguage::glsl;
    vs.code = nullptr;
    vs.codeSize = 0;

    GpuShaderSource fs;
    fs.language = GpuShaderLanguage::glsl;
    fs.code = "void main() {}";
    fs.codeSize = (uint32_t) strlen ("void main() {}");

    auto result = GpuPipeline::compile (device, vs, fs);
    EXPECT_TRUE (result.failed());
}

// --------------------------------------------------------------------------
// Render pass (end-to-end)
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, RenderPassDrawTriangle)
{
    // Create a render target.
    auto target = GpuTarget::create (device, 256, 256);
    ASSERT_NE (target, nullptr);

    // Compile a minimal pipeline.
    const char* vsSrc = R"(
        #version 450
        layout(set = 0, binding = 0) uniform Uniforms { mat4 mvp; } ubo;
        layout(location = 0) in vec3 aPos;
        void main() { gl_Position = ubo.mvp * vec4(aPos, 1.0); }
    )";

    const char* fsSrc = R"(
        #version 450
        layout(location = 0) out vec4 fragColor;
        void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }
    )";

    GpuShaderSource vs;
    vs.language = GpuShaderLanguage::glsl;
    vs.code = vsSrc;
    vs.codeSize = (uint32_t) strlen (vsSrc);

    GpuShaderSource fs;
    fs.language = GpuShaderLanguage::glsl;
    fs.code = fsSrc;
    fs.codeSize = (uint32_t) strlen (fsSrc);

    auto compileResult = GpuPipeline::compile (device, vs, fs);
    ASSERT_TRUE (compileResult.wasOk());
    auto* pipeline = compileResult.getValue().get();
    ASSERT_NE (pipeline, nullptr);

    // Begin a frame and render pass.
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto pass = target->beginRenderPass (frame, { true, Color (0, 0, 0, 0) });
    ASSERT_TRUE (pass.isValid());

    pass.setPipeline (*pipeline);
    EXPECT_TRUE (pass.draw (3));

    pass.finish();
    frame.submit();
}

TEST_F (GpuDeviceOpenGLTests, RenderPassClearColor)
{
    auto target = GpuTarget::create (device, 64, 64);
    ASSERT_NE (target, nullptr);

    // Render with a green clear color.
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    GpuRenderOptions opts;
    opts.clear = true;
    opts.clearColor = Color (0, 255, 0, 255); // Green, fully opaque.

    auto pass = target->beginRenderPass (frame, opts);
    ASSERT_TRUE (pass.isValid());
    pass.finish();
    frame.submit();
    frame.waitForGPU();

    // Read back the pixels — they should be green.
    std::vector<uint8_t> pixels (64 * 64 * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));

    // Check a few pixels in the center are green.
    const size_t center = (32 * 64 + 32) * 4;
    EXPECT_EQ (pixels[center + 0], 0u);   // R
    EXPECT_EQ (pixels[center + 1], 255u); // G
    EXPECT_EQ (pixels[center + 2], 0u);   // B
    EXPECT_EQ (pixels[center + 3], 255u); // A
}

TEST_F (GpuDeviceOpenGLTests, GpuFrameMovePreservesState)
{
    auto src = GpuFrame::begin (device);
    ASSERT_TRUE (src.isValid());

    GpuFrame dst (std::move (src));
    EXPECT_TRUE (dst.isValid());
    EXPECT_FALSE (src.isValid());

    dst.submit();
}

// --------------------------------------------------------------------------
// GpuCanvas integration (via GpuCanvas, which wraps GpuTarget)
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, GpuCanvasCreate)
{
    auto canvas = GpuCanvas::create (*device, 256, 256);
    ASSERT_NE (canvas, nullptr);
}

TEST_F (GpuDeviceOpenGLTests, GpuCanvasBeginDrawAndCommit)
{
    auto canvas = GpuCanvas::create (*device, 256, 256);
    ASSERT_NE (canvas, nullptr);

    canvas->beginDraw();
    canvas->commit();
}

TEST_F (GpuDeviceOpenGLTests, GpuCanvasAsImage)
{
    auto canvas = GpuCanvas::create (*device, 256, 256);
    ASSERT_NE (canvas, nullptr);

    auto& g = canvas->beginDraw();

    // Draw a filled rectangle.
    g.setFillColor (Color (255, 0, 0, 255));
    g.fillRect (0, 0, 256, 256);

    canvas->commit();

    auto img = canvas->asImage();
    EXPECT_TRUE (img.isValid());
    EXPECT_EQ (img.getWidth(), 256);
    EXPECT_EQ (img.getHeight(), 256);
}

TEST_F (GpuDeviceOpenGLTests, GpuCanvasReadPixelsAfterDraw)
{
    auto canvas = GpuCanvas::create (*device, 128, 128);
    ASSERT_NE (canvas, nullptr);

    auto& g = canvas->beginDraw();

    // Fill the entire canvas with red.
    g.setFillColor (Color (255, 0, 0, 255));
    g.fillRect (0, 0, 128, 128);

    canvas->commit();

    std::vector<uint8_t> pixels (128 * 128 * 4);
    EXPECT_TRUE (canvas->readPixels (pixels.data(), pixels.size()));

    // Center pixel should be red.
    const size_t centerIdx = (64 * 128 + 64) * 4;
    EXPECT_EQ (pixels[centerIdx + 0], 255u); // R
    EXPECT_EQ (pixels[centerIdx + 1], 0u);   // G
    EXPECT_EQ (pixels[centerIdx + 2], 0u);   // B
    EXPECT_EQ (pixels[centerIdx + 3], 255u); // A
}
