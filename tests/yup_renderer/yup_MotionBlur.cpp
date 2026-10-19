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

#include <yup_renderer/yup_renderer.h>

#include <gtest/gtest.h>

using namespace yup;

/** MotionBlur recording on a mocked device. The blurred pixels are tested on Metal, in
    native/yup_MotionBlur_apple.cpp. */
class MotionBlurMockTests : public RendererMockTests
{
protected:
    void SetUp() override
    {
        RendererMockTests::SetUp();
        resources = std::make_unique<FrameResources> (device);
    }

    MotionBlur::Inputs makeInputs (int width, int height, bool withBlurAmount = false) const
    {
        MotionBlur::Inputs inputs;
        inputs.color = makeTarget (width, height);
        inputs.depth = makeTarget (width, height, GpuTextureFormat::depth32float);
        inputs.velocity = makeTarget (width, height, GpuTextureFormat::rg16float);

        if (withBlurAmount)
            inputs.blurAmount = makeTarget (width, height, GpuTextureFormat::r8unorm);

        return inputs;
    }

    GpuTexture::Ptr renderFrame (MotionBlur& blur, const MotionBlur::Inputs& inputs, const RenderSettings& settings = {})
    {
        resources->beginFrame (128, 96);
        auto frame = GpuFrame::begin (device);
        auto blurred = blur.render (frame, view, settings, *resources, inputs);
        frame.submit();
        resources->endFrame();
        return blurred;
    }

    ViewData view;
    std::unique_ptr<FrameResources> resources;
};

TEST_F (MotionBlurMockTests, PrepareCompilesThePipelinesOnce)
{
    MotionBlur blur (device);
    ASSERT_TRUE (blur.prepare().wasOk()) << blur.prepare().getErrorMessage();
    EXPECT_EQ (pipelinesMade, 3);

    EXPECT_TRUE (blur.prepare().wasOk());
    EXPECT_EQ (pipelinesMade, 3);
}

TEST_F (MotionBlurMockTests, PrepareFailsWithoutADevice)
{
    MotionBlur blur (nullptr);
    const auto result = blur.prepare();
    ASSERT_TRUE (result.failed());
    EXPECT_EQ (result.getErrorMessage(), "No device");
}

TEST_F (MotionBlurMockTests, PrepareRemembersACompileFailure)
{
    MotionBlur blur (device);
    failPipelines = true;
    const auto result = blur.prepare();
    ASSERT_TRUE (result.failed());
    EXPECT_TRUE (result.getErrorMessage().startsWith ("Unable to compile the tiles")) << result.getErrorMessage();

    failPipelines = false;
    EXPECT_TRUE (blur.prepare().failed());
}

TEST_F (MotionBlurMockTests, SkipsWithAClosedShutterOrAMissingInput)
{
    MotionBlur blur (device);
    const auto inputs = makeInputs (128, 96);

    RenderSettings closed;
    closed.motionBlurShutter = 0.0f;

    for (int frame = 0; frame < 3; ++frame)
        EXPECT_EQ (renderFrame (blur, inputs, closed), nullptr);

    for (auto missing : { &MotionBlur::Inputs::color, &MotionBlur::Inputs::depth, &MotionBlur::Inputs::velocity })
    {
        auto partial = inputs;
        partial.*missing = nullptr;
        EXPECT_EQ (renderFrame (blur, partial), nullptr);
    }

    // Skips never count as failures, nor record anything
    EXPECT_FALSE (blur.hasGivenUp());
    EXPECT_TRUE (passes.empty());
    EXPECT_EQ (blur.getOutput(), nullptr);
}

TEST_F (MotionBlurMockTests, RecordsTheTilesTheNeighborhoodAndTheReconstruction)
{
    MotionBlur blur (device);
    const auto inputs = makeInputs (128, 96);

    const auto blurred = renderFrame (blur, inputs);
    ASSERT_NE (blurred, nullptr);
    EXPECT_EQ (blurred, blur.getOutput());
    EXPECT_EQ (blurred->getWidth(), 128);
    EXPECT_EQ (blurred->getHeight(), 96);
    EXPECT_EQ (blurred->getFormat(), GpuTextureFormat::rgba16float);

    ASSERT_NE (blur.getBlurAmount(), nullptr);
    EXPECT_EQ (blur.getBlurAmount()->getWidth(), 128);
    EXPECT_EQ (blur.getBlurAmount()->getHeight(), 96);

    // Two tile reductions, the neighborhood, then the reconstruction writing the color and the blur amount
    ASSERT_EQ (passes.size(), 4u);
    EXPECT_EQ (countDraws(), 4);
    EXPECT_EQ (passes[0].colorCount, 1u);
    EXPECT_EQ (passes[3].colorCount, 2u);
}

TEST_F (MotionBlurMockTests, ReadsTheDepthOfFieldBlurAmount)
{
    MotionBlur blur (device);
    EXPECT_NE (renderFrame (blur, makeInputs (64, 64, true)), nullptr);
    EXPECT_EQ (countDraws(), 4);
}

TEST_F (MotionBlurMockTests, OneFailureLeavesTheFrameWithoutTheBlur)
{
    MotionBlur blur (device);
    const auto inputs = makeInputs (64, 64);

    blur.failRendersForTesting (1);
    EXPECT_EQ (renderFrame (blur, inputs), nullptr);
    EXPECT_FALSE (blur.hasGivenUp());

    // A success resets the failures in a row
    EXPECT_NE (renderFrame (blur, inputs), nullptr);

    blur.failRendersForTesting (1);
    EXPECT_EQ (renderFrame (blur, inputs), nullptr);
    EXPECT_NE (renderFrame (blur, inputs), nullptr);
    EXPECT_FALSE (blur.hasGivenUp());
}

TEST_F (MotionBlurMockTests, GivesUpAfterTwoFailuresInARow)
{
    MotionBlur blur (device);
    const auto inputs = makeInputs (64, 64);

    blur.failRendersForTesting (2);
    EXPECT_EQ (renderFrame (blur, inputs), nullptr);
    EXPECT_EQ (renderFrame (blur, inputs), nullptr);
    ASSERT_TRUE (blur.hasGivenUp());
    EXPECT_EQ (blur.getGiveUpReason(), "the renders failed for testing");

    // For good, recording nothing
    EXPECT_EQ (renderFrame (blur, inputs), nullptr);
    EXPECT_TRUE (passes.empty());
}

TEST_F (MotionBlurMockTests, GivesUpWhenTheTargetsCannotBeCreated)
{
    MotionBlur blur (device);
    const auto inputs = makeInputs (64, 64);

    failTextures = true;
    EXPECT_EQ (renderFrame (blur, inputs), nullptr);
    EXPECT_EQ (renderFrame (blur, inputs), nullptr);
    ASSERT_TRUE (blur.hasGivenUp());
    EXPECT_EQ (blur.getGiveUpReason(), "unable to create the targets");
}

TEST_F (MotionBlurMockTests, GivesUpWhenThePipelinesFailToCompile)
{
    MotionBlur blur (device);
    const auto inputs = makeInputs (64, 64);

    failPipelines = true;
    EXPECT_EQ (renderFrame (blur, inputs), nullptr);
    EXPECT_EQ (renderFrame (blur, inputs), nullptr);
    ASSERT_TRUE (blur.hasGivenUp());
    EXPECT_TRUE (blur.getGiveUpReason().startsWith ("Unable to compile the tiles")) << blur.getGiveUpReason();
}

TEST_F (MotionBlurMockTests, RendersAgainAfterReleasingItsTargets)
{
    MotionBlur blur (device);
    const auto inputs = makeInputs (64, 64);

    ASSERT_NE (renderFrame (blur, inputs), nullptr);
    blur.trimUnusedTargets();
    blur.releaseTargets();

    EXPECT_NE (renderFrame (blur, inputs), nullptr);
    blur.trimUnusedTargets();
    EXPECT_EQ (countDraws(), 8);
}
