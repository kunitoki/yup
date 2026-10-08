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
class FrameResourcesMetalTests : public ::testing::Test
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

TEST_F (FrameResourcesMetalTests, ReusesTexturesAcrossFrames)
{
    FrameResources resources (device);
    const GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    resources.beginFrame (64, 64);
    auto first = resources.acquire (desc);
    resources.endFrame();

    resources.beginFrame (64, 64);
    EXPECT_EQ (resources.acquire (desc), first);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, TwoAcquiresInOneFrameAreDistinct)
{
    FrameResources resources (device);
    const GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    resources.beginFrame (64, 64);
    EXPECT_NE (resources.acquire (desc), resources.acquire (desc));
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, UnusedTexturesAreReleased)
{
    FrameResources resources (device);
    const GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    // Holding the first texture keeps its address from being handed to the replacement
    resources.beginFrame (64, 64);
    auto first = resources.acquire (desc);
    resources.endFrame();

    resources.beginFrame (64, 64);
    resources.endFrame(); // not acquired: released
    EXPECT_EQ (first->getReferenceCount(), 1);

    resources.beginFrame (64, 64);
    EXPECT_NE (resources.acquire (desc), first);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, HistorySwapsAndInvalidatesOnResize)
{
    FrameResources resources (device);
    const GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    resources.beginFrame (64, 64);
    auto h1 = resources.getHistory ("taa", desc);
    EXPECT_FALSE (h1.valid);
    resources.endFrame();

    resources.beginFrame (64, 64);
    auto h2 = resources.getHistory ("taa", desc);
    EXPECT_TRUE (h2.valid);
    EXPECT_EQ (h2.read, h1.write);
    resources.endFrame();

    resources.beginFrame (32, 32);
    EXPECT_FALSE (resources.getHistory ("taa", GpuTextureDesc (32, 32, GpuTextureFormat::rgba16float, true)).valid);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, DescriptorsDifferingOnlyByLabelShareTextures)
{
    FrameResources resources (device);
    GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    resources.beginFrame (64, 64);
    auto first = resources.acquire (desc);
    resources.endFrame();

    desc.label = "renamed";
    resources.beginFrame (64, 64);
    EXPECT_EQ (resources.acquire (desc), first);
    EXPECT_NE (resources.acquire (GpuTextureDesc (64, 64, GpuTextureFormat::rgba8unorm, true)), first);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, ResizeReleasesTransients)
{
    FrameResources resources (device);
    const GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    resources.beginFrame (64, 64);
    auto first = resources.acquire (desc);
    resources.endFrame();
    EXPECT_EQ (resources.getWidth(), 64);
    EXPECT_EQ (resources.getHeight(), 64);

    resources.beginFrame (32, 32);
    EXPECT_EQ (first->getReferenceCount(), 1);
    EXPECT_EQ (resources.getWidth(), 32);
    EXPECT_EQ (resources.getHeight(), 32);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, InvalidateHistoryKeepsTexturesButMarksThemInvalid)
{
    FrameResources resources (device);
    const GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    resources.beginFrame (64, 64);
    auto h1 = resources.getHistory ("taa", desc);
    resources.endFrame();

    resources.invalidateHistory();

    resources.beginFrame (64, 64);
    auto h2 = resources.getHistory ("taa", desc);
    EXPECT_FALSE (h2.valid);
    EXPECT_EQ (h2.read, h1.write);
    resources.endFrame();

    resources.beginFrame (64, 64);
    EXPECT_TRUE (resources.getHistory ("taa", desc).valid);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, HistoryInvalidatedAfterUseStaysInvalidForTheNextFrame)
{
    FrameResources resources (device);
    const GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    resources.beginFrame (64, 64);
    resources.getHistory ("taa", desc);
    resources.endFrame();

    // This frame's write already read the stale history, so the next frame must not trust it
    resources.beginFrame (64, 64);
    EXPECT_TRUE (resources.getHistory ("taa", desc).valid);
    resources.invalidateHistory();
    EXPECT_FALSE (resources.getHistory ("taa", desc).valid);
    resources.endFrame();

    resources.beginFrame (64, 64);
    EXPECT_FALSE (resources.getHistory ("taa", desc).valid);
    resources.endFrame();

    // Invalidated before use, the write restarted from scratch and is valid next frame
    resources.beginFrame (64, 64);
    EXPECT_TRUE (resources.getHistory ("taa", desc).valid);
    resources.endFrame();

    resources.beginFrame (64, 64);
    resources.invalidateHistory();
    EXPECT_FALSE (resources.getHistory ("taa", desc).valid);
    resources.endFrame();

    resources.beginFrame (64, 64);
    EXPECT_TRUE (resources.getHistory ("taa", desc).valid);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, HistoryWithANewDescriptorIsReallocated)
{
    FrameResources resources (device);

    resources.beginFrame (64, 64);
    auto h1 = resources.getHistory ("taa", GpuTextureDesc (64, 64, GpuTextureFormat::rgba16float, true));
    resources.endFrame();

    resources.beginFrame (64, 64);
    auto h2 = resources.getHistory ("taa", GpuTextureDesc (64, 64, GpuTextureFormat::rgba8unorm, true));
    EXPECT_FALSE (h2.valid);
    ASSERT_NE (h2.read, nullptr);
    EXPECT_EQ (h2.read->getFormat(), GpuTextureFormat::rgba8unorm);
    EXPECT_NE (h2.read, h1.write);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, HistoryUnusedForAFrameIsReleased)
{
    FrameResources resources (device);
    const GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    // Holding the first pair keeps its addresses from being handed to the replacement
    resources.beginFrame (64, 64);
    auto h1 = resources.getHistory ("taa", desc);
    resources.endFrame();

    resources.beginFrame (64, 64);
    resources.endFrame(); // not used: released
    EXPECT_EQ (h1.read->getReferenceCount(), 1);
    EXPECT_EQ (h1.write->getReferenceCount(), 1);

    resources.beginFrame (64, 64);
    auto h3 = resources.getHistory ("taa", desc);
    EXPECT_FALSE (h3.valid);
    EXPECT_NE (h3.read, h1.read);
    EXPECT_NE (h3.read, h1.write);
    resources.endFrame();
}
#endif
