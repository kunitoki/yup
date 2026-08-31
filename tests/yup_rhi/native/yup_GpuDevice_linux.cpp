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
#include <yup_shading/yup_shading.h>

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

        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengl");

        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

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
        opts.readableFramebuffer = true;

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

        GpuDevice::Options ctxOpts;
        ctxOpts.loaderFunction = (GpuDevice::LoaderFunction) SDL_GL_GetProcAddress;
        graphicsContext = GraphicsContext::createContext (GpuPlatform::OpenGL, ctxOpts, device);
        if (graphicsContext == nullptr)
            GTEST_SKIP() << "Cannot create OpenGL GraphicsContext";
    }

    void TearDown() override
    {
        graphicsContext = nullptr;
        device = nullptr;
        gl.shutdown();
    }

    GLContext gl;
    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> graphicsContext;
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
    EXPECT_NE (device->getGpuContext(), nullptr);
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
#if ! YUP_ENABLE_SHADER_TRANSPILER
    GTEST_SKIP() << "Shader transpiler unavailable — cannot compile GLSL sources inline";
#else
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

    auto result = GpuPipeline::compileFromGlsl (device, vsSrc, fsSrc);
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
#endif
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

#if ! YUP_ENABLE_SHADER_TRANSPILER
    GTEST_SKIP() << "Shader transpiler unavailable — cannot compile GLSL sources inline";
#else
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

    auto compileResult = GpuPipeline::compileFromGlsl (device, vsSrc, fsSrc);
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
#endif
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
    opts.clearColor = Color (255, 0, 255, 0); // ARGB: Green, fully opaque.

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
    auto canvas = GpuCanvas::create (*graphicsContext, 256, 256);
    ASSERT_NE (canvas, nullptr);
}

TEST_F (GpuDeviceOpenGLTests, GpuCanvasCreateClearsToTransparentBlackByDefault)
{
    // A new canvas must be safe to sample before anything is drawn into it, so its
    // backing texture cannot be left holding uninitialized GPU memory.
    auto canvas = GpuCanvas::create (*graphicsContext, 64, 64);
    ASSERT_NE (canvas, nullptr);

    std::vector<uint8_t> pixels (64 * 64 * 4, 0xab);
    ASSERT_TRUE (canvas->readPixels (pixels.data(), pixels.size()));

    size_t nonZeroBytes = 0;
    for (const auto value : pixels)
    {
        if (value != 0u)
            ++nonZeroBytes;
    }

    EXPECT_EQ (nonZeroBytes, 0u);
}

TEST_F (GpuDeviceOpenGLTests, GpuCanvasCreateClearsToRequestedColor)
{
    auto canvas = GpuCanvas::create (*graphicsContext, 64, 64, Color (255, 0, 128, 255));
    ASSERT_NE (canvas, nullptr);

    std::vector<uint8_t> pixels (64 * 64 * 4, 0);
    ASSERT_TRUE (canvas->readPixels (pixels.data(), pixels.size()));

    const size_t centerIdx = (32 * 64 + 32) * 4;
    EXPECT_EQ (pixels[centerIdx + 0], 0u);   // R
    EXPECT_EQ (pixels[centerIdx + 1], 128u); // G
    EXPECT_EQ (pixels[centerIdx + 2], 255u); // B
    EXPECT_EQ (pixels[centerIdx + 3], 255u); // A
}

TEST_F (GpuDeviceOpenGLTests, GpuCanvasCreateWithoutClearStillSucceeds)
{
    // std::nullopt skips the clear; the contents are then undefined by contract, so
    // only creation itself is asserted here.
    auto canvas = GpuCanvas::create (*graphicsContext, 64, 64, std::nullopt);
    ASSERT_NE (canvas, nullptr);
    EXPECT_EQ (canvas->getWidth(), 64);
    EXPECT_EQ (canvas->getHeight(), 64);
}

TEST_F (GpuDeviceOpenGLTests, ClearOffscreenFillsRenderableTarget)
{
    // clearOffscreen needs neither an active frame nor a render pass, so it can run
    // straight after the target is created.
    auto target = device->createRenderableTarget (64, 64);
    ASSERT_NE (target, nullptr);

    ASSERT_TRUE (device->clearOffscreen (*target, GpuColor (0.0f, 1.0f, 0.0f, 1.0f)));

    std::vector<uint8_t> pixels (64 * 64 * 4, 0);
    ASSERT_TRUE (device->readOffscreenPixels (*target, pixels.data(), pixels.size()));

    const size_t centerIdx = (32 * 64 + 32) * 4;
    EXPECT_EQ (pixels[centerIdx + 0], 0u);   // R
    EXPECT_EQ (pixels[centerIdx + 1], 255u); // G
    EXPECT_EQ (pixels[centerIdx + 2], 0u);   // B
    EXPECT_EQ (pixels[centerIdx + 3], 255u); // A
}

TEST_F (GpuDeviceOpenGLTests, ClearOffscreenFillsRenderPassOnlyTarget)
{
    // The same call must also work on a plain offscreen target, which has no
    // dedicated render context.
    auto target = device->createOffscreenTarget (64, 64);
    ASSERT_NE (target, nullptr);

    ASSERT_TRUE (device->clearOffscreen (*target, GpuColor (1.0f, 0.0f, 0.0f, 1.0f)));

    std::vector<uint8_t> pixels (64 * 64 * 4, 0);
    ASSERT_TRUE (device->readOffscreenPixels (*target, pixels.data(), pixels.size()));

    const size_t centerIdx = (32 * 64 + 32) * 4;
    EXPECT_EQ (pixels[centerIdx + 0], 255u); // R
    EXPECT_EQ (pixels[centerIdx + 1], 0u);   // G
    EXPECT_EQ (pixels[centerIdx + 2], 0u);   // B
    EXPECT_EQ (pixels[centerIdx + 3], 255u); // A
}

TEST_F (GpuDeviceOpenGLTests, GpuCanvasBeginDrawAndCommit)
{
    auto canvas = GpuCanvas::create (*graphicsContext, 256, 256);
    ASSERT_NE (canvas, nullptr);

    canvas->beginDraw();
    canvas->commit();
}

TEST_F (GpuDeviceOpenGLTests, GpuCanvasAsImage)
{
    auto canvas = GpuCanvas::create (*graphicsContext, 256, 256);
    ASSERT_NE (canvas, nullptr);

    auto& g = canvas->beginDraw();

    // Draw a filled rectangle.
    g.setFillColor (Color (255, 255, 0, 0)); // ARGB: Red, fully opaque.
    g.fillRect (0, 0, 256, 256);

    canvas->commit();

    auto img = canvas->asImage();
    EXPECT_TRUE (img.isValid());
    EXPECT_EQ (img.getWidth(), 256);
    EXPECT_EQ (img.getHeight(), 256);
}

TEST_F (GpuDeviceOpenGLTests, GpuCanvasReadPixelsAfterDraw)
{
    auto canvas = GpuCanvas::create (*graphicsContext, 128, 128);
    ASSERT_NE (canvas, nullptr);

    auto& g = canvas->beginDraw();

    // Fill the entire canvas with red.
    g.setFillColor (Color (255, 255, 0, 0)); // ARGB: Red, fully opaque.
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

// --------------------------------------------------------------------------
// Compute shader support
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, IsComputeAvailable)
{
    // GL 4.3+ should support compute shaders.
    EXPECT_TRUE (device->isComputeAvailable());
}

// --------------------------------------------------------------------------
// GpuComputePipeline
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, ComputePipelineCompileFailsWithNullDevice)
{
    GpuShaderSource src;
    src.language = GpuShaderLanguage::glsl;
    src.code = "void main() {}";
    src.codeSize = static_cast<uint32_t> (strlen (static_cast<const char*> (src.code)));

    auto result = GpuComputePipeline::compile (nullptr, src, GpuWorkgroupSize { 16, 1, 1 });
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuDeviceOpenGLTests, ComputePipelineCompileFromGlslMinimalShader)
{
#if ! YUP_ENABLE_SHADER_TRANSPILER
    GTEST_SKIP() << "Shader transpiler unavailable — cannot compile GLSL sources inline";
#else
    const char* glsl = R"(
        #version 450
        layout(local_size_x = 8, local_size_y = 1, local_size_z = 1) in;
        layout(std430, binding = 0) buffer OutputBuf { float values[]; } outputBuf;
        void main()
        {
            uint idx = gl_GlobalInvocationID.x;
            outputBuf.values[idx] = float(idx) * 2.0;
        }
    )";

    auto result = GpuComputePipeline::compileFromGlsl (device, glsl);
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);

    auto wgs = result.getValue()->getWorkgroupSize();
    EXPECT_EQ (wgs.x, 8u);
    EXPECT_EQ (wgs.y, 1u);
    EXPECT_EQ (wgs.z, 1u);
#endif
}

TEST_F (GpuDeviceOpenGLTests, ComputePipelineCompileFromGlslFailsWithNullDevice)
{
#if ! YUP_ENABLE_SHADER_TRANSPILER
    GTEST_SKIP() << "Shader transpiler unavailable — cannot compile GLSL sources inline";
#else
    auto result = GpuComputePipeline::compileFromGlsl (nullptr, "#version 450\nvoid main() {}");
    EXPECT_TRUE (result.failed());
#endif
}

TEST_F (GpuDeviceOpenGLTests, ComputePipelineCompileFromBundleFailsWithNullDevice)
{
    ShaderBundle bundle;
    auto result = GpuComputePipeline::compileFromBundle (nullptr, bundle);
    EXPECT_TRUE (result.failed());
}

// --------------------------------------------------------------------------
// GpuComputePass
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, ComputePassBeginReturnsValidPass)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_TRUE (pass.isValid());

    if (pass.isValid())
        pass.finish();
}

TEST_F (GpuDeviceOpenGLTests, ComputePassIsValid)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_TRUE (pass.isValid());

    // After finish, isValid should return false.
    pass.finish();
    EXPECT_FALSE (pass.isValid());
}

TEST_F (GpuDeviceOpenGLTests, ComputePassFinishIsIdempotent)
{
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());

    EXPECT_TRUE (pass.finish());
    EXPECT_FALSE (pass.finish());
}

TEST_F (GpuDeviceOpenGLTests, ComputePassDispatchWithoutPipelineReturnsFalse)
{
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());

    EXPECT_FALSE (pass.dispatch (1, 1, 1));
    pass.finish();
}

TEST_F (GpuDeviceOpenGLTests, ComputePassMoveConstruction)
{
    auto src = GpuComputePass::begin (device);
    ASSERT_TRUE (src.isValid());

    GpuComputePass dst (std::move (src));
    EXPECT_TRUE (dst.isValid());
    EXPECT_FALSE (src.isValid());

    dst.finish();
}

TEST_F (GpuDeviceOpenGLTests, ComputePassMoveAssignment)
{
    auto src = GpuComputePass::begin (device);
    ASSERT_TRUE (src.isValid());

    auto dst = GpuComputePass::begin (GpuDevice::create (GpuPlatform::Headless, {}));
    EXPECT_FALSE (dst.isValid());

    dst = std::move (src);
    EXPECT_TRUE (dst.isValid());
    EXPECT_FALSE (src.isValid());

    dst.finish();
}

TEST_F (GpuDeviceOpenGLTests, ComputePassSetPipelineDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());

    EXPECT_NO_THROW (pass.setPipeline (nullptr));

    pass.finish();
}

TEST_F (GpuDeviceOpenGLTests, ComputePassSetStorageBufferDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());

    EXPECT_NO_THROW (pass.setStorageBuffer (0, 0, nullptr));

    pass.finish();
}

TEST_F (GpuDeviceOpenGLTests, ComputePassSetUniformBufferDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());

    float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    EXPECT_NO_THROW (pass.setUniformBuffer (0, 0, data, sizeof (data)));

    pass.finish();
}

TEST_F (GpuDeviceOpenGLTests, ComputePassSetTextureDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());

    EXPECT_NO_THROW (pass.setTexture (0, 0, nullptr));

    pass.finish();
}

TEST_F (GpuDeviceOpenGLTests, ComputePassDestructorCallsFinish)
{
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());
    // Let destructor call finish — should not crash.
}

TEST_F (GpuDeviceOpenGLTests, ComputePassDestructorAfterFinishDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());
    pass.finish();
    // Destructor after explicit finish — should not double-finish.
}

TEST_F (GpuDeviceOpenGLTests, ComputePassWithHeadlessDeviceIsInvalid)
{
    auto headless = GpuDevice::create (GpuPlatform::Headless, {});
    ASSERT_NE (headless, nullptr);

    auto pass = GpuComputePass::begin (headless);
    EXPECT_FALSE (pass.isValid());
    EXPECT_FALSE (pass.dispatch (1, 1, 1));
    EXPECT_FALSE (pass.finish());
}

// --------------------------------------------------------------------------
// Coverage: GpuDevice / Image / Graphics GPU-backed paths
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, CreateWithUnavailableApiReturnsNull)
{
    // Metal and Direct3D are not compiled on Linux — the create() switch must
    // fall through to its default error branch.
    EXPECT_EQ (GpuDevice::create (GpuPlatform::Metal, {}), nullptr);
    EXPECT_EQ (GpuDevice::create (GpuPlatform::Direct3D, {}), nullptr);
}

TEST_F (GpuDeviceOpenGLTests, ImageFromTargetReadsPixels)
{
    auto target = GpuTarget::create (device, 64, 64);
    ASSERT_NE (target, nullptr);

    auto img = Image::fromTarget (*target);
    ASSERT_TRUE (img.isValid());
    EXPECT_EQ (img.getWidth(), 64);
    EXPECT_EQ (img.getHeight(), 64);

    // getTexture() must resolve the backing GPU texture (sampledTexture on GL).
    EXPECT_NE (img.getTexture(), nullptr);
}

TEST_F (GpuDeviceOpenGLTests, ImageCreateTextureIfNotPresent)
{
    Image img (32, 32, PixelFormat::RGBA);
    img.fill (0xFF000000);

    // First call uploads CPU pixels into a GPU texture.
    EXPECT_TRUE (img.createTextureIfNotPresent (*graphicsContext));
    EXPECT_NE (img.getTexture(), nullptr);

    // Second call finds the texture already present.
    EXPECT_TRUE (img.createTextureIfNotPresent (*graphicsContext));
}

TEST_F (GpuDeviceOpenGLTests, GraphicsCommitToImage)
{
    Image img (64, 64, PixelFormat::RGBA);

    {
        Graphics g (*graphicsContext, img, 0u);
        ASSERT_TRUE (g.isOffscreen());

        g.setFillColor (Color (0xFFFF0000));
        g.fillRect (0.0f, 0.0f, 64.0f, 64.0f);

        EXPECT_TRUE (g.commitToImage());
        EXPECT_NE (img.getTexture(), nullptr);
    }
}

TEST_F (GpuDeviceOpenGLTests, GraphicsReadPixelsToImage)
{
    Image img (64, 64, PixelFormat::RGBA);

    {
        Graphics g (*graphicsContext, img, 0u);
        ASSERT_TRUE (g.isOffscreen());

        g.setFillColor (Color (0xFF00FF00));
        g.fillRect (0.0f, 0.0f, 64.0f, 64.0f);

        EXPECT_TRUE (g.readPixelsToImage());
    }
}

TEST_F (GpuDeviceOpenGLTests, GraphicsTransparencyLayerCommit)
{
    auto renderer = graphicsContext->makeRenderer (256, 256);
    ASSERT_NE (renderer, nullptr);

    Graphics g (*graphicsContext, *renderer);

    auto layer = g.beginTransparencyLayer (Rectangle<float> (0.0f, 0.0f, 100.0f, 100.0f), 0.5f);
    ASSERT_TRUE (layer.isValid());

    auto& layerGraphics = layer.getGraphics();
    layerGraphics.setFillColor (Color (0xFFFF0000));
    layerGraphics.fillRect (10.0f, 10.0f, 50.0f, 50.0f);

    EXPECT_TRUE (layer.commit());
}

TEST_F (GpuDeviceOpenGLTests, GraphicsDrawImageAndTexture)
{
    auto renderer = graphicsContext->makeRenderer (128, 128);
    ASSERT_NE (renderer, nullptr);

    Graphics g (*graphicsContext, *renderer);

    // Upload a CPU image to the GPU.
    Image img (16, 16, PixelFormat::RGBA);
    img.fill (0xFFFF0000);
    ASSERT_TRUE (img.createTextureIfNotPresent (*graphicsContext));

    // drawImage() → renderTexture() success path.
    EXPECT_NO_THROW (g.drawImage (img, Rectangle<float> (10.0f, 10.0f, 64.0f, 64.0f)));

    // A canvas texture for the drawTexture() path.
    auto canvas = GpuCanvas::create (*graphicsContext, 16, 16);
    ASSERT_NE (canvas, nullptr);
    auto tex = canvas->asTexture();
    ASSERT_NE (tex, nullptr);

    // drawTexture() → renderTexture() success path.
    EXPECT_NO_THROW (g.drawTexture (tex, Rectangle<float> (10.0f, 10.0f, 64.0f, 64.0f)));

    // Empty target area → early return.
    EXPECT_NO_THROW (g.drawTexture (tex, Rectangle<float>()));
}

TEST_F (GpuDeviceOpenGLTests, GraphicsDrawTextureWithNullRenderContext)
{
    // Grab a real GPU texture from a canvas on the GL context.
    auto canvas = GpuCanvas::create (*graphicsContext, 32, 32);
    ASSERT_NE (canvas, nullptr);
    auto tex = canvas->asTexture();
    ASSERT_NE (tex, nullptr);

    // A headless Graphics has no render context — renderTexture must bail safely.
    auto headlessContext = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (headlessContext, nullptr);
    auto renderer = headlessContext->makeRenderer (64, 64);
    ASSERT_NE (renderer, nullptr);

    Graphics g (*headlessContext, *renderer);
    EXPECT_NO_THROW (g.drawTexture (tex, Rectangle<float> (0.0f, 0.0f, 32.0f, 32.0f)));
}

// --------------------------------------------------------------------------
// Coverage: GpuRenderPass vertex/index buffers, uniform & texture bindings
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, RenderPassDrawWithVertexAndIndexBuffers)
{
    auto target = GpuTarget::create (device, 128, 128);
    ASSERT_NE (target, nullptr);

    const float verts[] = { -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f };
    const uint16_t indices[] = { 0, 1, 2 };

    auto vbo = device->createBuffer (GpuBufferType::vertex, verts, sizeof (verts));
    auto ibo = device->createBuffer (GpuBufferType::index, indices, sizeof (indices));
    ASSERT_NE (vbo, nullptr);
    ASSERT_NE (ibo, nullptr);

#if ! YUP_ENABLE_SHADER_TRANSPILER
    GTEST_SKIP() << "Shader transpiler unavailable — cannot compile GLSL sources inline";
#else
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

    auto compileResult = GpuPipeline::compileFromGlsl (device, vsSrc, fsSrc);
    ASSERT_TRUE (compileResult.wasOk());
    auto* pipeline = compileResult.getValue().get();
    ASSERT_NE (pipeline, nullptr);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto pass = target->beginRenderPass (frame, { true, Color (0, 0, 0, 0) });
    ASSERT_TRUE (pass.isValid());

    pass.setPipeline (*pipeline);

    // A >16 byte uniform buffer forces the pool into a higher bucket.
    std::vector<float> uboData (64, 1.0f);
    pass.setUniformBuffer (0, 0, uboData.data(), uboData.size() * sizeof (float));

    pass.setVertexBuffer (0, vbo);
    pass.setIndexBuffer (GpuIndexFormat::uint16, ibo);

    EXPECT_TRUE (pass.drawIndexed (3));

    pass.finish();
    frame.submit();
    frame.waitForGPU();
#endif
}

TEST_F (GpuDeviceOpenGLTests, RenderPassSetUniformBufferAndTexture)
{
    auto target = GpuTarget::create (device, 64, 64);
    ASSERT_NE (target, nullptr);

#if ! YUP_ENABLE_SHADER_TRANSPILER
    GTEST_SKIP() << "Shader transpiler unavailable — cannot compile GLSL sources inline";
#else
    // Fragment shader samples a texture at binding 1 so the pass layout has a
    // texture slot matching the bound canvas texture.
    const char* vsSrc = R"(
        #version 450
        layout(set = 0, binding = 0) uniform Uniforms { mat4 mvp; } ubo;
        layout(location = 0) in vec3 aPos;
        layout(location = 0) out vec2 vUV;
        void main() { vUV = aPos.xy; gl_Position = ubo.mvp * vec4(aPos, 1.0); }
    )";

    const char* fsSrc = R"(
        #version 450
        layout(set = 0, binding = 1) uniform sampler2D tex;
        layout(location = 0) in vec2 vUV;
        layout(location = 0) out vec4 fragColor;
        void main() { fragColor = texture(tex, vUV); }
    )";

    auto compileResult = GpuPipeline::compileFromGlsl (device, vsSrc, fsSrc);
    ASSERT_TRUE (compileResult.wasOk());
    auto* pipeline = compileResult.getValue().get();
    ASSERT_NE (pipeline, nullptr);

    auto canvas = GpuCanvas::create (*graphicsContext, 16, 16);
    ASSERT_NE (canvas, nullptr);
    auto tex = canvas->asTexture();
    ASSERT_NE (tex, nullptr);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto pass = target->beginRenderPass (frame, { true, Color (0, 0, 0, 0) });
    ASSERT_TRUE (pass.isValid());

    pass.setPipeline (*pipeline);

    // Null / zero-size uniform data → early return.
    EXPECT_NO_THROW (pass.setUniformBuffer (0, 0, nullptr, 0));

    // Valid uniform + texture bindings, then draw.
    const float data[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    pass.setUniformBuffer (0, 0, data, sizeof (data));
    pass.setTexture (0, 1, tex);

    EXPECT_TRUE (pass.draw (3));

    pass.finish();
    frame.submit();
    frame.waitForGPU();
#endif
}

// --------------------------------------------------------------------------
// Coverage: GpuComputePass repeated bindings update in place
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, ComputePassRepeatedBindingsUpdateInPlace)
{
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());

    const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };

    // Null / zero-size data → early return.
    EXPECT_NO_THROW (pass.setUniformBuffer (0, 0, nullptr, 0));

    pass.setUniformBuffer (0, 0, data, sizeof (data));
    pass.setUniformBuffer (0, 0, data, sizeof (data)); // update existing

    pass.setStorageBuffer (0, 0, nullptr);
    pass.setStorageBuffer (0, 0, nullptr); // update existing

    pass.setTexture (0, 0, nullptr);
    pass.setTexture (0, 0, nullptr); // update existing

    pass.finish();
}

// --------------------------------------------------------------------------
// Coverage: GpuComputePipeline bundle / transpiler error paths
// --------------------------------------------------------------------------

TEST_F (GpuDeviceOpenGLTests, ComputePipelineCompileFromBundleWithComputeShader)
{
#if ! YUP_ENABLE_SHADER_TRANSPILER
    GTEST_SKIP() << "Shader transpiler unavailable — cannot compile GLSL sources inline";
#else
    const char* glsl = R"(
        #version 450
        layout(local_size_x = 8, local_size_y = 1, local_size_z = 1) in;
        layout(std430, binding = 0) buffer OutputBuf { float values[]; } outputBuf;
        void main()
        {
            uint idx = gl_GlobalInvocationID.x;
            outputBuf.values[idx] = float(idx) * 2.0;
        }
    )";

    ShaderBundle bundle;
    ShaderInfo info;
    info.stage = ShaderStage::compute;
    info.language = ShaderLanguage::glsl;
    info.entryPoint = "main";
    info.source = glsl;
    info.inputSource = glsl;
    info.reflection.workgroupSize = { 8, 1, 1, false };
    bundle.addShader (info);

    auto result = GpuComputePipeline::compileFromBundle (device, bundle, GpuWorkgroupSize { 1, 1, 1 });
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
#endif
}

TEST_F (GpuDeviceOpenGLTests, ComputePipelineCompileFromBundleWithoutComputeShaderFails)
{
    ShaderBundle bundle;
    auto result = GpuComputePipeline::compileFromBundle (device, bundle, GpuWorkgroupSize { 8, 1, 1 });
    EXPECT_TRUE (result.failed());
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (GpuDeviceOpenGLTests, ComputePipelineCompileFromGlslInvalidSourceFails)
{
    auto result = GpuComputePipeline::compileFromGlsl (device, "this is not valid glsl at all");
    EXPECT_TRUE (result.failed());
}
#endif
