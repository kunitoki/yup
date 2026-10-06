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

#include <yup_rhi/yup_rhi.h>

#include <gtest/gtest.h>

using namespace yup;

class GpuFrameDescriptorTests : public ::testing::Test
{
};

TEST_F (GpuFrameDescriptorTests, TriangulationThresholdsDefaultToRivesDefaults)
{
    const GpuTriangulationThresholds thresholds;
    const rive::gpu::TriangulationThresholds riveThresholds;

    EXPECT_FLOAT_EQ (riveThresholds.minArea, thresholds.minArea);
    EXPECT_EQ (riveThresholds.maxVerbs, thresholds.maxVerbs);
    EXPECT_FLOAT_EQ (riveThresholds.frameBudgetMs, thresholds.frameBudgetMs);

    EXPECT_FLOAT_EQ (riveThresholds.minArea, GpuFrameDescriptor().triangulationThresholds.minArea);
}

#if YUP_APPLE
// ---------------------------------------------------------------------------
// GpuFrameDescriptor - Metal (the descriptor only reaches Rive on a real backend)
// ---------------------------------------------------------------------------

class GpuFrameDescriptorMetalTests : public ::testing::Test
{
protected:
    static constexpr int size = 8;

    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";

        target = device->createRenderableTarget (size, size);
        if (target == nullptr)
            GTEST_SKIP() << "No Metal offscreen target available";
    }

    /** Runs a frame with nothing drawn, and returns the pixel at the top left as RGBA. */
    std::array<uint8, 4> renderEmptyFrame (GpuFrameDescriptor desc)
    {
        desc.renderTargetWidth = static_cast<uint32_t> (size);
        desc.renderTargetHeight = static_cast<uint32_t> (size);

        device->beginOffscreen (*target, desc);
        device->endOffscreen (*target);

        std::vector<uint8> pixels (static_cast<std::size_t> (size * size * 4));
        EXPECT_TRUE (device->readOffscreenPixels (*target, pixels.data(), pixels.size()));
        return { pixels[0], pixels[1], pixels[2], pixels[3] };
    }

    // Green and magenta read back the same in RGBA and BGRA order
    static constexpr std::array<uint8, 4> green { 0, 255, 0, 255 };
    static constexpr std::array<uint8, 4> magenta { 255, 0, 255, 255 };

    GpuDevice::Ptr device;
    std::unique_ptr<RenderableTarget> target;
};

TEST_F (GpuFrameDescriptorMetalTests, LoadOpsDecideWhatTheFrameStartsFrom)
{
    EXPECT_EQ (green, renderEmptyFrame ({ .loadOp = GpuLoadOp::clear, .clearColor = GpuColor (0.0f, 1.0f, 0.0f) }));

    // Load keeps what the previous frame left, ignoring the clear color
    EXPECT_EQ (green, renderEmptyFrame ({ .loadOp = GpuLoadOp::load, .clearColor = GpuColor (1.0f, 0.0f, 1.0f) }));

    EXPECT_EQ (magenta, renderEmptyFrame ({ .loadOp = GpuLoadOp::clear, .clearColor = GpuColor (1.0f, 0.0f, 1.0f) }));

    // Don't care leaves the contents undefined, but the frame still completes
    renderEmptyFrame ({ .loadOp = GpuLoadOp::dontCare });
}

TEST_F (GpuFrameDescriptorMetalTests, DitherModeNoneStillClears)
{
    // Dithering only applies to gradients: a plain clear is exact with or without it
    EXPECT_EQ (green, renderEmptyFrame ({ .clearColor = GpuColor (0.0f, 1.0f, 0.0f), .ditherMode = GpuDitherMode::none }));
    EXPECT_EQ (magenta, renderEmptyFrame ({ .clearColor = GpuColor (1.0f, 0.0f, 1.0f), .ditherMode = GpuDitherMode::interleavedGradientNoise }));
}
#endif // YUP_APPLE
