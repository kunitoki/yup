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
// Drawing parity: the same Graphics calls must land the same geometry on every
// backend. These assert at sample points rather than against a golden image,
// because two rasterisers legitimately disagree along an antialiased edge while
// having to agree completely about where a shape is and what colour it is.
//
// Sample points sit several pixels inside each region, so edge coverage never
// enters the assertion.
//==============================================================================

namespace
{

constexpr int kCanvasSize = 64;
constexpr int kInset = 4; // How far inside a region to sample, to clear any AA.

} // namespace

class GraphicsParityTests : public ::testing::Test
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
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "no GPU-backed GraphicsContext on this backend";
    }

    /** Creates a canvas, runs the drawing, commits it and reads the result back.

        beginDraw() opens a frame with LoadAction::clear and a hardcoded clear
        colour of transparent black, so whatever create() cleared to is gone by
        the time the callback runs. Pass the background you want and it is
        painted with fillAll first, which is what a real component does.
    */
    test::Bitmap render (Color background, const std::function<void (Graphics&)>& draw)
    {
        auto canvas = GpuCanvas::create (*context, kCanvasSize, kCanvasSize, Colors::transparentBlack);
        if (canvas == nullptr)
            return {};

        {
            auto& g = canvas->beginDraw();
            g.setFillColor (background);
            g.fillAll();
            draw (g);
        }

        // The 2D frame has to be flushed before the pixels mean anything.
        // GpuCanvas::readPixels documents that it auto-commits, but it returns
        // false on an open frame instead, so commit explicitly. Reading the
        // GpuTarget directly is worse: it succeeds and returns the pre-draw
        // contents.
        canvas->commit();

        test::Bitmap result;
        result.pixels.resize (static_cast<size_t> (kCanvasSize) * static_cast<size_t> (kCanvasSize) * 4u);

        if (! canvas->readPixels (result.pixels.data(), result.pixels.size()))
            return {};

        result.width = kCanvasSize;
        result.height = kCanvasSize;

        return result;
    }

    /** Asserts one sampled pixel, naming the region so a failure is readable. */
    static ::testing::AssertionResult pixelIs (const test::Bitmap& bitmap, int x, int y, uint32_t expected, const char* region)
    {
        if (! bitmap.isValid())
            return ::testing::AssertionFailure() << "bitmap was never read back";

        const auto found = bitmap.getPixel (x, y);

        if (found == expected)
            return ::testing::AssertionSuccess();

        return ::testing::AssertionFailure()
            << region << " at (" << x << ", " << y << ") is 0x" << String::toHexString (static_cast<int> (found))
            << ", expected 0x" << String::toHexString (static_cast<int> (expected))
            << " (0xAABBGGRR)";
    }

    test::GpuTestDevice gpu;
    GraphicsContext* context = nullptr;
};

//==============================================================================
// The colour passed to create() must reach every pixel of a canvas that has
// not been drawn into. It is only meaningful before a 2D frame is opened.
//==============================================================================

TEST_F (GraphicsParityTests, CanvasClearColorFillsAnUndrawnCanvas)
{
    auto canvas = GpuCanvas::create (*context, kCanvasSize, kCanvasSize, Colors::red);
    ASSERT_NE (canvas, nullptr);

    test::Bitmap bitmap;
    bitmap.pixels.resize (static_cast<size_t> (kCanvasSize) * static_cast<size_t> (kCanvasSize) * 4u);
    ASSERT_TRUE (canvas->readPixels (bitmap.pixels.data(), bitmap.pixels.size()));
    bitmap.width = kCanvasSize;
    bitmap.height = kCanvasSize;

    EXPECT_TRUE (test::bitmapIsSolid (bitmap, test::packRGBA (255, 0, 0, 255), "canvas_clear"));
}

//==============================================================================
// Opening a 2D frame discards that colour. beginDraw() builds its Graphics with
// LoadAction::clear and a hardcoded transparent black, so anything a caller
// wants to see behind its drawing has to be painted inside the frame. Pinning
// this here means a change to that behaviour shows up as a test failure rather
// than as mysteriously transparent corners somewhere else.
//==============================================================================

TEST_F (GraphicsParityTests, BeginDrawResetsTheCanvasToTransparentBlack)
{
    auto canvas = GpuCanvas::create (*context, kCanvasSize, kCanvasSize, Colors::red);
    ASSERT_NE (canvas, nullptr);

    canvas->beginDraw(); // Deliberately draws nothing.
    canvas->commit();

    test::Bitmap bitmap;
    bitmap.pixels.resize (static_cast<size_t> (kCanvasSize) * static_cast<size_t> (kCanvasSize) * 4u);
    ASSERT_TRUE (canvas->readPixels (bitmap.pixels.data(), bitmap.pixels.size()));
    bitmap.width = kCanvasSize;
    bitmap.height = kCanvasSize;

    EXPECT_TRUE (test::bitmapIsSolid (bitmap, test::packRGBA (0, 0, 0, 0), "begin_draw_clear"));
}

//==============================================================================
// fillAll must cover the clear colour completely.
//==============================================================================

TEST_F (GraphicsParityTests, FillAllCoversEveryPixel)
{
    const auto bitmap = render (Colors::red, [] (Graphics& g)
    {
        g.setFillColor (Colors::blue);
        g.fillAll();
    });

    EXPECT_TRUE (test::bitmapIsSolid (bitmap, test::packRGBA (0, 0, 255, 255), "fill_all"));
}

//==============================================================================
// The critical orientation test. A rectangle covering the top-left quadrant in
// Graphics coordinates must read back in the top-left of the bitmap.
//
// OpenGL framebuffers have their origin at the bottom-left and Metal textures
// at the top-left. If either backend loses that conversion, this rectangle
// lands in the wrong quadrant while every solid-colour test still passes.
//==============================================================================

TEST_F (GraphicsParityTests, FillRectLandsInTheTopLeftQuadrant)
{
    constexpr float half = kCanvasSize / 2.0f;

    const auto bitmap = render (Colors::white, [] (Graphics& g)
    {
        g.setFillColor (Colors::black);
        g.fillRect (0.0f, 0.0f, half, half);
    });

    ASSERT_TRUE (bitmap.isValid());

    const auto black = test::packRGBA (0, 0, 0, 255);
    const auto white = test::packRGBA (255, 255, 255, 255);

    constexpr int inner = kCanvasSize / 2 - kInset;
    constexpr int outer = kCanvasSize / 2 + kInset;

    EXPECT_TRUE (pixelIs (bitmap, kInset, kInset, black, "top-left"));
    EXPECT_TRUE (pixelIs (bitmap, inner, inner, black, "inside the rect"));
    EXPECT_TRUE (pixelIs (bitmap, outer, kInset, white, "top-right"));
    EXPECT_TRUE (pixelIs (bitmap, kInset, outer, white, "bottom-left"));
    EXPECT_TRUE (pixelIs (bitmap, outer, outer, white, "bottom-right"));
}

//==============================================================================
// A rectangle away from the origin must land where it was asked to, which
// catches an axis swap that the top-left quadrant test alone would not.
//==============================================================================

TEST_F (GraphicsParityTests, FillRectHonoursItsOffset)
{
    constexpr float x = 8.0f;
    constexpr float y = 32.0f;
    constexpr float width = 16.0f;
    constexpr float height = 8.0f;

    const auto bitmap = render (Colors::white, [] (Graphics& g)
    {
        g.setFillColor (Colors::black);
        g.fillRect (x, y, width, height);
    });

    ASSERT_TRUE (bitmap.isValid());

    const auto black = test::packRGBA (0, 0, 0, 255);
    const auto white = test::packRGBA (255, 255, 255, 255);

    EXPECT_TRUE (pixelIs (bitmap, 12, 35, black, "inside the rect"));
    EXPECT_TRUE (pixelIs (bitmap, 12, 28, white, "above the rect"));
    EXPECT_TRUE (pixelIs (bitmap, 12, 44, white, "below the rect"));
    EXPECT_TRUE (pixelIs (bitmap, 4, 35, white, "left of the rect"));
    EXPECT_TRUE (pixelIs (bitmap, 28, 35, white, "right of the rect"));
}

//==============================================================================
// Draw order: a later opaque fill must paint over an earlier one.
//==============================================================================

TEST_F (GraphicsParityTests, LaterFillsPaintOverEarlierOnes)
{
    const auto bitmap = render (Colors::white, [] (Graphics& g)
    {
        g.setFillColor (Colors::red);
        g.fillRect (0.0f, 0.0f, 48.0f, 48.0f);

        g.setFillColor (Colors::green);
        g.fillRect (16.0f, 16.0f, 48.0f, 48.0f);
    });

    ASSERT_TRUE (bitmap.isValid());

    const auto red = test::packRGBA (255, 0, 0, 255);
    const auto green = test::packRGBA (0, 128, 0, 255); // Colors::green is the CSS value.
    const auto white = test::packRGBA (255, 255, 255, 255);

    EXPECT_TRUE (pixelIs (bitmap, 8, 8, red, "red only"));
    EXPECT_TRUE (pixelIs (bitmap, 32, 32, green, "the overlap, green on top"));
    EXPECT_TRUE (pixelIs (bitmap, 56, 56, green, "green only"));
    EXPECT_TRUE (pixelIs (bitmap, 56, 8, white, "neither"));
}

//==============================================================================
// A rendered frame must not be uninitialised memory. This is the same measure
// tools/check_screenshot.py applies to CI captures, asserted here against real
// drawing rather than a clear, so the threshold it uses stays grounded in what
// actual content scores.
//==============================================================================

TEST_F (GraphicsParityTests, RenderedContentIsNotUninitialisedMemory)
{
    const auto bitmap = render (Colors::white, [] (Graphics& g)
    {
        g.setFillColor (Colors::black);
        g.fillRect (8.0f, 8.0f, 48.0f, 48.0f);
    });

    ASSERT_TRUE (bitmap.isValid());

    // Two flat regions with one edge between them sit far above the 0.25 that
    // tools/check_screenshot.py rejects, and far above what noise can reach.
    EXPECT_GT (test::measureFlatFraction (bitmap), 0.8);
}
