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

#include "../helpers/yup_GpuTestDevice.h"
#include "../helpers/yup_PixelCompare.h"

using namespace yup;

//==============================================================================
// Backend parity: the same source runs on every platform's real GPU backend and
// asserts the same bytes. A failure here means two backends disagree about
// something they are contractually required to agree on, which invalidates
// every higher-level rendering comparison.
//
// These tests skip rather than fail when no usable GPU is present, so they are
// safe on a headless runner without a software rasterizer.
//==============================================================================

class GpuParityTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (gpu.isValid())
            return;

        // A skip reads as a pass in most CI summaries, so a runner that was
        // meant to have a GPU and lost it would report green while proving
        // nothing. YUP_TEST_REQUIRE_GPU=1 turns that into a failure.
        if (test::GpuTestDevice::isGpuRequired())
            FAIL() << "YUP_TEST_REQUIRE_GPU is set but no " << test::GpuTestDevice::getPlatformName()
                   << " device could be created: " << gpu.getFailureReason();

        GTEST_SKIP() << "no usable GPU device: " << gpu.getFailureReason();
    }

    test::GpuTestDevice gpu;
};

//==============================================================================
// Clear parity. Every backend must land the exact colour it was asked for, in
// RGBA byte order, with no premultiplication and no channel swap.
//==============================================================================

TEST_F (GpuParityTests, ClearProducesTheExactRequestedColor)
{
    struct ClearCase
    {
        const char* name;
        GpuColor color;
        uint32_t expected;
    };

    const ClearCase cases[] = {
        { "red",     GpuColor (1.0f, 0.0f, 0.0f, 1.0f), test::packRGBA (255, 0, 0, 255) },
        { "green",   GpuColor (0.0f, 1.0f, 0.0f, 1.0f), test::packRGBA (0, 255, 0, 255) },
        { "blue",    GpuColor (0.0f, 0.0f, 1.0f, 1.0f), test::packRGBA (0, 0, 255, 255) },
        { "white",   GpuColor (1.0f, 1.0f, 1.0f, 1.0f), test::packRGBA (255, 255, 255, 255) },
        { "black",   GpuColor (0.0f, 0.0f, 0.0f, 1.0f), test::packRGBA (0, 0, 0, 255) },
    };

    constexpr int size = 32;

    for (const auto& testCase : cases)
    {
        SCOPED_TRACE (testCase.name);

        auto target = gpu->createOffscreenTarget (size, size);
        ASSERT_NE (target, nullptr);

        ASSERT_TRUE (gpu->clearOffscreen (*target, testCase.color));

        const auto bitmap = test::readTarget (*gpu, *target, size, size);

        EXPECT_TRUE (test::bitmapIsSolid (bitmap, testCase.expected, String ("clear_") + testCase.name));
    }
}

//==============================================================================
// A second clear must overwrite the first rather than blend with it or be
// dropped, which is what a leaked scissor rect or write mask would cause.
//==============================================================================

TEST_F (GpuParityTests, SecondClearFullyOverwritesTheFirst)
{
    constexpr int size = 32;

    auto target = gpu->createOffscreenTarget (size, size);
    ASSERT_NE (target, nullptr);

    ASSERT_TRUE (gpu->clearOffscreen (*target, GpuColor (1.0f, 0.0f, 0.0f, 1.0f)));
    ASSERT_TRUE (gpu->clearOffscreen (*target, GpuColor (0.0f, 0.0f, 1.0f, 1.0f)));

    const auto bitmap = test::readTarget (*gpu, *target, size, size);

    EXPECT_TRUE (test::bitmapIsSolid (bitmap, test::packRGBA (0, 0, 255, 255), "clear_overwrite"));
}

//==============================================================================
// Two targets must not share storage. On a backend that pools render contexts,
// a target that aliases another shows up here and nowhere else.
//==============================================================================

TEST_F (GpuParityTests, SeparateTargetsDoNotAlias)
{
    constexpr int size = 16;

    auto first = gpu->createOffscreenTarget (size, size);
    ASSERT_NE (first, nullptr);

    auto second = gpu->createOffscreenTarget (size, size);
    ASSERT_NE (second, nullptr);

    ASSERT_TRUE (gpu->clearOffscreen (*first, GpuColor (1.0f, 0.0f, 0.0f, 1.0f)));
    ASSERT_TRUE (gpu->clearOffscreen (*second, GpuColor (0.0f, 1.0f, 0.0f, 1.0f)));

    const auto firstBitmap = test::readTarget (*gpu, *first, size, size);
    const auto secondBitmap = test::readTarget (*gpu, *second, size, size);

    EXPECT_TRUE (test::bitmapIsSolid (firstBitmap, test::packRGBA (255, 0, 0, 255), "alias_first"));
    EXPECT_TRUE (test::bitmapIsSolid (secondBitmap, test::packRGBA (0, 255, 0, 255), "alias_second"));
}

//==============================================================================
// A Rive frame with LoadAction::clear must fill the whole target. This is the
// first rung that goes through the render context rather than a raw clear, so
// it covers the backend's frame setup and its flush.
//==============================================================================

TEST_F (GpuParityTests, FrameClearFillsTheWholeTarget)
{
    constexpr int size = 32;

    // createRenderableTarget reserves a dedicated render context, which
    // beginOffscreen needs in order to drive a frame. A plain offscreen target
    // has no context slot and beginOffscreen is a no-op on it.
    auto target = gpu->createRenderableTarget (size, size);
    ASSERT_NE (target, nullptr);

    rive::gpu::RenderContext::FrameDescriptor frameDesc;
    frameDesc.renderTargetWidth = static_cast<uint32_t> (size);
    frameDesc.renderTargetHeight = static_cast<uint32_t> (size);
    frameDesc.loadAction = rive::gpu::LoadAction::clear;
    frameDesc.clearColor = 0xFF0000FF; // Rive ColorInt is 0xAARRGGBB, so opaque blue.

    gpu->beginOffscreen (*target, frameDesc);
    gpu->endOffscreen (*target);

    const auto bitmap = test::readTarget (*gpu, *target, size, size);

    EXPECT_TRUE (test::bitmapIsSolid (bitmap, test::packRGBA (0, 0, 255, 255), "frame_clear"));
}

//==============================================================================
// The readback contract: both backends promise RGBA8 in top-to-bottom row
// order. A backend that forgets to flip, or that hands back BGRA, produces a
// bitmap that still looks plausible per-pixel, so assert the contract itself.
//==============================================================================

TEST_F (GpuParityTests, ReadbackHasTheDocumentedSizeAndLayout)
{
    constexpr int width = 24;
    constexpr int height = 8;

    auto target = gpu->createOffscreenTarget (width, height);
    ASSERT_NE (target, nullptr);

    ASSERT_TRUE (gpu->clearOffscreen (*target, GpuColor (0.25f, 0.5f, 0.75f, 1.0f)));

    const auto bitmap = test::readTarget (*gpu, *target, width, height);

    ASSERT_TRUE (bitmap.isValid());
    EXPECT_EQ (bitmap.width, width);
    EXPECT_EQ (bitmap.height, height);
    EXPECT_EQ (bitmap.pixels.size(), static_cast<size_t> (width) * static_cast<size_t> (height) * 4u);

    // Channels must arrive in RGBA order, so red is the smallest and alpha is
    // opaque. A BGRA readback would put the largest value first.
    const auto pixel = bitmap.pixels;
    EXPECT_LT (pixel[0], pixel[1]);
    EXPECT_LT (pixel[1], pixel[2]);
    EXPECT_EQ (pixel[3], 255);
}

//==============================================================================
// A readback that is asked for less memory than the target needs must refuse
// rather than overrun the buffer.
//==============================================================================

TEST_F (GpuParityTests, ReadbackRefusesAnUndersizedBuffer)
{
    constexpr int size = 16;

    auto target = gpu->createOffscreenTarget (size, size);
    ASSERT_NE (target, nullptr);

    ASSERT_TRUE (gpu->clearOffscreen (*target, GpuColor (1.0f, 1.0f, 1.0f, 1.0f)));

    std::vector<uint8_t> tooSmall (static_cast<size_t> (size) * static_cast<size_t> (size) * 4u - 1u);

    EXPECT_FALSE (gpu->readOffscreenPixels (*target, tooSmall.data(), tooSmall.size()));
}

//==============================================================================
// A cleared target is entirely flat. This is the same measure that
// tools/check_screenshot.py applies to CI captures, verified here against a
// surface we know is good, so the threshold it uses stays grounded.
//==============================================================================

TEST_F (GpuParityTests, ClearedTargetMeasuresAsFullyFlat)
{
    constexpr int size = 32;

    auto target = gpu->createOffscreenTarget (size, size);
    ASSERT_NE (target, nullptr);

    ASSERT_TRUE (gpu->clearOffscreen (*target, GpuColor (0.5f, 0.5f, 0.5f, 1.0f)));

    const auto bitmap = test::readTarget (*gpu, *target, size, size);
    ASSERT_TRUE (bitmap.isValid());

    EXPECT_DOUBLE_EQ (test::measureFlatFraction (bitmap), 1.0);
}
