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

#include <yup_renderer/yup_renderer.h>

using namespace yup;

TEST (TemporalAABlendWeightTests, StillFramesTakeTheTrueRunningMean)
{
    // Frame n of a still accumulation weighs 1 / (n + 1): once sampleCount frames are in, each of
    // them holds the same share of the history, so rotating noise averages out evenly
    for (int sampleCount : { 4, 16, 64 })
    {
        for (int index = 0; index < sampleCount; ++index)
            EXPECT_FLOAT_EQ (TemporalAA::getBlendWeight (index, sampleCount, false), 1.0f / static_cast<float> (index + 1)) << index << " of " << sampleCount;

        EXPECT_EQ (TemporalAA::getBlendWeight (sampleCount, sampleCount, false), 0.0f);
        EXPECT_EQ (TemporalAA::getBlendWeight (sampleCount + 5, sampleCount, false), 0.0f);
    }
}

TEST (TemporalAABlendWeightTests, ChangingFramesKeepAtLeastATenth)
{
    // While the scene changes, the current frame keeps at least a tenth, so the history fades
    // within a few frames behind moving content; the start of the ramp is the same running mean
    for (int index = 0; index < 16; ++index)
        EXPECT_FLOAT_EQ (TemporalAA::getBlendWeight (index, 16, true), jmax (1.0f / static_cast<float> (index + 1), 0.1f)) << index;

    EXPECT_EQ (TemporalAA::getBlendWeight (16, 16, true), 0.0f);
}

/** TemporalAA recording on a mocked device. The resolved pixels are tested on Metal, in
    native/yup_TemporalAA_apple.cpp. */
class TemporalAAMockTests : public RendererMockTests
{
protected:
    void SetUp() override
    {
        RendererMockTests::SetUp();
        resources = std::make_unique<FrameResources> (device);
        depth = makeTarget (64, 64, GpuTextureFormat::depth32float);
        velocity = makeTarget (64, 64, GpuTextureFormat::rg16float);
    }

    bool renderFrame (TemporalAA& taa, const GpuTexture::Ptr& current, const GpuTexture::Ptr& output, int sampleIndex = 0)
    {
        resources->beginFrame (output->getWidth(), output->getHeight());
        auto frame = GpuFrame::begin (device);
        const auto rendered = taa.render (device, frame, view, *resources, current, depth, velocity, sampleIndex, 16, output);
        frame.submit();
        resources->endFrame();
        return rendered;
    }

    ViewData view;
    std::unique_ptr<FrameResources> resources;
    GpuTexture::Ptr depth;
    GpuTexture::Ptr velocity;
};

TEST_F (TemporalAAMockTests, RejectsAMissingInput)
{
    TemporalAA taa;
    const auto current = makeTarget (64, 64);
    const auto output = makeTarget (64, 64);

    auto frame = GpuFrame::begin (device);
    EXPECT_FALSE (taa.render (nullptr, frame, view, *resources, current, depth, velocity, 0, 16, output));
    EXPECT_FALSE (taa.render (device, frame, view, *resources, nullptr, depth, velocity, 0, 16, output));
    EXPECT_FALSE (taa.render (device, frame, view, *resources, current, nullptr, velocity, 0, 16, output));
    EXPECT_FALSE (taa.render (device, frame, view, *resources, current, depth, nullptr, 0, 16, output));
    EXPECT_FALSE (taa.render (device, frame, view, *resources, current, depth, velocity, 0, 16, nullptr));
    frame.submit();

    EXPECT_TRUE (passes.empty());
}

TEST_F (TemporalAAMockTests, ResolvesIntoTheOutputAndTheHistory)
{
    TemporalAA taa;
    const auto current = makeTarget (64, 64);
    const auto output = makeTarget (64, 64);

    ASSERT_TRUE (renderFrame (taa, current, output, 0));
    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].colorCount, 2u);
    EXPECT_EQ (passes[0].draws, 1);

    const auto history = resources->findHistory (TemporalAA::historyName);
    ASSERT_NE (history.write, nullptr);
    EXPECT_EQ (history.write->getWidth(), 64);
    EXPECT_EQ (history.write->getFormat(), GpuTextureFormat::rgba16float);

    // The next frame reads the history the first wrote
    ASSERT_TRUE (renderFrame (taa, current, output, 1));
    EXPECT_TRUE (resources->findHistory (TemporalAA::historyName).valid);
    EXPECT_EQ (passes.size(), 2u);
    EXPECT_FALSE (taa.isUpscalingPrepared());
}

TEST_F (TemporalAAMockTests, UpscalesASmallerFrameWithItsWeight)
{
    TemporalAA taa;
    const auto current = makeTarget (32, 32);
    const auto output = makeTarget (64, 64);

    ASSERT_TRUE (renderFrame (taa, current, output, 0));
    EXPECT_TRUE (taa.isUpscalingPrepared());
    ASSERT_EQ (passes.size(), 1u);
    EXPECT_EQ (passes[0].colorCount, 3u);

    // At the output size
    const auto history = resources->findHistory (TemporalAA::historyName);
    ASSERT_NE (history.write, nullptr);
    EXPECT_EQ (history.write->getWidth(), 64);

    const auto weight = resources->findHistory (TemporalAA::weightHistoryName);
    ASSERT_NE (weight.write, nullptr);
    EXPECT_EQ (weight.write->getWidth(), 64);
    EXPECT_EQ (weight.write->getFormat(), GpuTextureFormat::r16float);

    ASSERT_TRUE (renderFrame (taa, current, output, 1));
    EXPECT_TRUE (resources->findHistory (TemporalAA::weightHistoryName).valid);
}

TEST_F (TemporalAAMockTests, RemembersACompileFailure)
{
    TemporalAA taa;
    const auto current = makeTarget (64, 64);
    const auto output = makeTarget (64, 64);

    failPipelines = true;
    EXPECT_FALSE (renderFrame (taa, current, output));

    failPipelines = false;
    EXPECT_FALSE (renderFrame (taa, current, output));
    EXPECT_TRUE (passes.empty());
}

TEST_F (TemporalAAMockTests, RemembersAnUpscalingCompileFailure)
{
    TemporalAA taa;
    const auto output = makeTarget (64, 64);
    ASSERT_TRUE (renderFrame (taa, makeTarget (64, 64), output));

    failPipelines = true;
    EXPECT_FALSE (renderFrame (taa, makeTarget (32, 32), output));
    EXPECT_FALSE (taa.isUpscalingPrepared());

    failPipelines = false;
    EXPECT_FALSE (renderFrame (taa, makeTarget (32, 32), output));
    EXPECT_EQ (passes.size(), 1u);
}

TEST_F (TemporalAAMockTests, FailsWithoutItsHistory)
{
    TemporalAA taa;
    const auto current = makeTarget (64, 64);
    const auto output = makeTarget (64, 64);

    failTextures = true;
    EXPECT_FALSE (renderFrame (taa, current, output));
    EXPECT_TRUE (passes.empty());
}

TEST_F (TemporalAAMockTests, RendersAgainAfterReleasingItsTargets)
{
    TemporalAA taa;
    const auto current = makeTarget (64, 64);
    const auto output = makeTarget (64, 64);

    ASSERT_TRUE (renderFrame (taa, current, output));
    taa.trimUnusedTargets();
    taa.releaseTargets();

    EXPECT_TRUE (renderFrame (taa, current, output));
    taa.trimUnusedTargets();
    EXPECT_EQ (countDraws(), 2);
}
