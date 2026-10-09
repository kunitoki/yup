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

#include <yup_3d/yup_3d.h>

#include <gtest/gtest.h>

using namespace yup;

#if YUP_APPLE
class PostChainMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";
    }

    GpuDevice::Ptr device;
};

TEST_F (PostChainMetalTests, TurningBloomOffReleasesItsTargets)
{
    constexpr int size = 64;
    const GpuTextureDesc halfFloat (size, size, GpuTextureFormat::rgba16float, true);

    // The full tier without TAA: bloom is the only effect acquiring a full size half-float texture, its glow
    RenderSettings settings;
    settings.tier = RenderSettings::TierPreference::full;
    settings.taaSamples = 1;

    const auto facts = RenderCapabilities::queryDevice (*device);
    FrameResources resources (device);
    PostChain chain;

    Scene scene;
    ViewData view;
    view.update (scene, size, size, false, 1, 0);

    PostChain::Inputs inputs;
    inputs.color = GpuTexture::create (device, halfFloat);
    const auto output = GpuTexture::create (device, GpuTextureDesc (size, size, GpuTextureFormat::rgba8unorm, true));
    ASSERT_NE (inputs.color, nullptr);
    ASSERT_NE (output, nullptr);

    const auto render = [&] (bool bloom)
    {
        settings.bloom = bloom;
        const RenderCapabilities capabilities (facts, settings, 1);
        ASSERT_EQ (capabilities.isEnabled (RenderCapabilities::Feature::bloom, settings), bloom);

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (chain.render (device, frame, capabilities, settings, view, resources, inputs, output));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());
    };

    resources.beginFrame (size, size);
    render (true);
    resources.endFrame();

    // Taken back from the pool, the glow is still held by the render target bloom kept for it
    resources.beginFrame (size, size);
    const auto glow = resources.acquire (halfFloat);
    ASSERT_NE (glow, nullptr);
    EXPECT_GT (glow->getReferenceCount(), 2);

    render (false);
    EXPECT_EQ (glow->getReferenceCount(), 2); // This test and the pool
    resources.endFrame();
}

TEST_F (PostChainMetalTests, ResizingReleasesTheTargetsOfTheLastSize)
{
    RenderSettings settings;
    settings.tier = RenderSettings::TierPreference::full;
    settings.taaSamples = 1;
    settings.bloom = true;

    const RenderCapabilities capabilities (RenderCapabilities::queryDevice (*device), settings, 1);
    FrameResources resources (device);
    PostChain chain;
    Scene scene;

    const auto halfFloat = [] (int size)
    {
        return GpuTextureDesc (static_cast<uint32_t> (size), static_cast<uint32_t> (size), GpuTextureFormat::rgba16float, true);
    };

    const auto render = [&] (int size)
    {
        ViewData view;
        view.update (scene, size, size, false, 1, 0);

        PostChain::Inputs inputs;
        inputs.color = GpuTexture::create (device, halfFloat (size));
        const auto output = GpuTexture::create (device, GpuTextureDesc (static_cast<uint32_t> (size), static_cast<uint32_t> (size), GpuTextureFormat::rgba8unorm, true));
        ASSERT_NE (inputs.color, nullptr);
        ASSERT_NE (output, nullptr);

        resources.beginFrame (size, size);
        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (chain.render (device, frame, capabilities, settings, view, resources, inputs, output));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());
        resources.endFrame();
    };

    // Put the first bloom level and the glow of a 64x64 frame in the pool, where bloom takes them from
    resources.beginFrame (64, 64);
    const auto firstLevel = resources.acquire (halfFloat (32));
    const auto glow = resources.acquire (halfFloat (64));
    ASSERT_NE (firstLevel, nullptr);
    ASSERT_NE (glow, nullptr);
    resources.endFrame();

    render (64);
    EXPECT_GT (firstLevel->getReferenceCount(), 2);
    EXPECT_GT (glow->getReferenceCount(), 2);

    // The pool lets go of them at the new size, and the passes no longer draw into them
    render (80);
    EXPECT_EQ (firstLevel->getReferenceCount(), 1);
    EXPECT_EQ (glow->getReferenceCount(), 1);
}
#endif
