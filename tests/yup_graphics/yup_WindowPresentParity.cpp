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

#include <yup_graphics/yup_graphics.h>
#include <yup_rhi/yup_rhi.h>

#include "../helpers/yup_GpuTestDevice.h"
#include "../helpers/yup_PixelCompare.h"

using namespace yup;

//==============================================================================
// Presentation: what GraphicsContext::end() actually delivers to the surface a
// window presents from.
//
// Everything below this layer is covered by the other parity suites and agrees
// across backends. This is the layer that did not: a frame rendered correctly
// into the context's offscreen target reached the window as a 32x32 corner,
// because the blit to framebuffer 0 inherited a scissor rectangle that Rive
// left enabled during flush. The rest of the window kept uninitialised memory.
//
// These tests drive GraphicsContext directly and read framebuffer 0, which is
// the surface SDL_GL_SwapWindow presents. They are OpenGL-only by nature: the
// Metal context attaches a CAMetalLayer to a real NSView, so it cannot be
// driven without a window, and reading a drawable is not equivalent. That
// asymmetry is the point rather than a gap, since the fault was GL-specific
// global state.
//==============================================================================

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID

namespace
{

constexpr int kSurfaceSize = 64;

/** Reads framebuffer 0 back as a top-down RGBA8 bitmap. */
test::Bitmap readDefaultFramebuffer (int width, int height)
{
    test::Bitmap result;
    result.pixels.resize (static_cast<size_t> (width) * static_cast<size_t> (height) * 4u);

    glBindFramebuffer (GL_READ_FRAMEBUFFER, 0);
    glPixelStorei (GL_PACK_ALIGNMENT, 1);
    glReadPixels (0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, result.pixels.data());

    if (glGetError() != GL_NO_ERROR)
        return {};

    // glReadPixels returns rows bottom-up; Bitmap is top-down.
    const auto stride = static_cast<size_t> (width) * 4u;
    std::vector<uint8_t> row (stride);

    for (int y = 0; y < height / 2; ++y)
    {
        auto* top = result.pixels.data() + static_cast<size_t> (y) * stride;
        auto* bottom = result.pixels.data() + static_cast<size_t> (height - 1 - y) * stride;
        std::memcpy (row.data(), top, stride);
        std::memcpy (top, bottom, stride);
        std::memcpy (bottom, row.data(), stride);
    }

    result.width = width;
    result.height = height;

    return result;
}

} // namespace

class WindowPresentParityTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (! gpu.isValid())
        {
            if (test::GpuTestDevice::isGpuRequired())
                FAIL() << "YUP_TEST_REQUIRE_GPU is set but no " << test::GpuTestDevice::getPlatformName()
                       << " device could be created: " << gpu.getFailureReason();

            GTEST_SKIP() << "no usable GPU device: " << gpu.getFailureReason();
        }

        context = gpu.getGraphicsContext();
        if (context == nullptr)
            GTEST_SKIP() << "no GraphicsContext on this backend";

        if (context->getPlatform() != GpuPlatform::OpenGL && context->getPlatform() != GpuPlatform::OpenGLES)
            GTEST_SKIP() << "presentation readback is OpenGL-only; this backend presents through a native layer";

        // The GL context is current on this thread and owns a hidden window, so
        // framebuffer 0 is a real readable surface. No native handle is needed
        // because the GL context creates its own offscreen target.
        context->onSizeChanged (nullptr, kSurfaceSize, kSurfaceSize, 1.0f, 0);
        renderer = context->makeRenderer (kSurfaceSize, kSurfaceSize);
    }

    /** Runs one full frame the way the window render loop does. */
    test::Bitmap presentFrame (uint32_t clearArgb, const std::function<void (Graphics&)>& draw)
    {
        rive::gpu::RenderContext::FrameDescriptor desc;
        desc.renderTargetWidth = static_cast<uint32_t> (kSurfaceSize);
        desc.renderTargetHeight = static_cast<uint32_t> (kSurfaceSize);
        desc.loadAction = rive::gpu::LoadAction::clear;
        desc.clearColor = clearArgb;
        desc.clockwiseFillOverride = true;

        context->begin (desc);

        if (draw != nullptr && renderer != nullptr)
        {
            Graphics g (*context, *renderer, 1.0f);
            draw (g);
        }

        context->end (nullptr);

        return readDefaultFramebuffer (kSurfaceSize, kSurfaceSize);
    }

    test::GpuTestDevice gpu;
    GraphicsContext* context = nullptr;
    std::unique_ptr<rive::Renderer> renderer;
};

//==============================================================================
// The whole surface must receive the frame, not a corner of it.
//
// This is the regression test for the original fault. With the scissor left
// enabled across the blit, the clear reached only the rectangle Rive last
// scissored to and every other pixel kept whatever was in the back buffer.
//==============================================================================

TEST_F (WindowPresentParityTests, ClearReachesEveryPixelOfThePresentedSurface)
{
    const auto bitmap = presentFrame (0xFF0000FF, nullptr); // Opaque blue, 0xAARRGGBB.

    ASSERT_TRUE (bitmap.isValid()) << "could not read framebuffer 0";
    EXPECT_TRUE (test::bitmapIsSolid (bitmap, test::packRGBA (0, 0, 255, 255), "present_clear"));
}

//==============================================================================
// Drawn content must reach the surface too, and land in the right place. A blit
// that is clipped, offset or flipped shows up here.
//==============================================================================

TEST_F (WindowPresentParityTests, DrawnGeometryReachesTheSurfaceInTheRightPlace)
{
    const auto bitmap = presentFrame (0xFFFFFFFF, [] (Graphics& g)
    {
        g.setFillColor (Colors::black);
        g.fillRect (0.0f, 0.0f, kSurfaceSize / 2.0f, kSurfaceSize / 2.0f);
    });

    ASSERT_TRUE (bitmap.isValid());

    const auto black = test::packRGBA (0, 0, 0, 255);
    const auto white = test::packRGBA (255, 255, 255, 255);

    EXPECT_TRUE (bitmap.getPixel (4, 4) == black) << "top-left quadrant should be drawn";
    EXPECT_TRUE (bitmap.getPixel (kSurfaceSize - 5, 4) == white) << "top-right should be the clear colour";
    EXPECT_TRUE (bitmap.getPixel (4, kSurfaceSize - 5) == white) << "bottom-left should be the clear colour";
    EXPECT_TRUE (bitmap.getPixel (kSurfaceSize - 5, kSurfaceSize - 5) == white) << "bottom-right should be the clear colour";
}

//==============================================================================
// Two frames in a row must both present completely. Rive's scissor state
// carries across frames, so a leak shows on the second frame even when the
// first looks correct.
//==============================================================================

TEST_F (WindowPresentParityTests, ASecondFrameAlsoReachesEveryPixel)
{
    presentFrame (0xFFFF0000, [] (Graphics& g)
    {
        g.setFillColor (Colors::green);
        g.fillRect (8.0f, 8.0f, 16.0f, 16.0f);
    });

    const auto second = presentFrame (0xFF0000FF, nullptr);

    ASSERT_TRUE (second.isValid());
    EXPECT_TRUE (test::bitmapIsSolid (second, test::packRGBA (0, 0, 255, 255), "present_second_frame"));
}

//==============================================================================
// The presented surface must never be uninitialised memory. This catches the
// original failure mode directly, using the measure tools/check_screenshot.py
// applies to CI captures: a noise frame scores near zero, a real one above 0.8.
//==============================================================================

TEST_F (WindowPresentParityTests, PresentedSurfaceIsNotUninitialisedMemory)
{
    const auto bitmap = presentFrame (0xFF404040, [] (Graphics& g)
    {
        g.setFillColor (Colors::white);
        g.fillRect (8.0f, 8.0f, 32.0f, 32.0f);
    });

    ASSERT_TRUE (bitmap.isValid());
    EXPECT_GT (test::measureFlatFraction (bitmap), 0.8)
        << "the presented surface looks like uninitialised memory rather than a rendered frame";
}

//==============================================================================
// GraphicsContext::end() must not leave the scissor test enabled. Nothing
// downstream expects it, and the blit that presents the frame is subject to it.
// Asserting the state directly names the fault instead of only its symptom.
//==============================================================================

TEST_F (WindowPresentParityTests, EndDoesNotLeaveTheScissorTestEnabled)
{
    presentFrame (0xFF000000, nullptr);

    EXPECT_FALSE (glIsEnabled (GL_SCISSOR_TEST))
        << "GL_SCISSOR_TEST is still enabled after end(), so the next blit to the window will be clipped";

    GLboolean colorMask[4] = {};
    glGetBooleanv (GL_COLOR_WRITEMASK, colorMask);

    EXPECT_TRUE (colorMask[0] && colorMask[1] && colorMask[2] && colorMask[3])
        << "a colour channel is masked off after end(), so the next blit to the window will drop it";
}

#else

TEST (WindowPresentParityTests, NotApplicableOnThisBackend)
{
    GTEST_SKIP() << "presentation readback is OpenGL-only";
}

#endif
