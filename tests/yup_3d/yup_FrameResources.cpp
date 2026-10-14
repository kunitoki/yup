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

TEST_F (FrameResourcesMetalTests, DescriptorsDifferingOnlyByStorageGetDistinctTextures)
{
    FrameResources resources (device);
    const GpuTextureDesc plain (64, 64, GpuTextureFormat::rgba16float);
    auto storage = plain;
    storage.storage = true;

    // Holding the first texture keeps its address from being handed to the second
    resources.beginFrame (64, 64);
    auto first = resources.acquire (plain);
    resources.endFrame();

    resources.beginFrame (64, 64);
    auto second = resources.acquire (storage);
    ASSERT_NE (second, nullptr);
    EXPECT_NE (second, first);
    EXPECT_TRUE (second->isStorage());
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, StorageBuffersRotateOverTheFramesInFlight)
{
    FrameResources resources (device);
    std::vector<GpuBuffer::Ptr> buffers;

    for (int frame = 0; frame < 4; ++frame)
    {
        resources.beginFrame (64, 64);
        auto buffer = resources.acquireStorageBuffer (FrameResources::StorageSlot::lights, 256);
        ASSERT_NE (buffer, nullptr);
        EXPECT_EQ (buffer->getType(), GpuBufferType::storage);
        EXPECT_EQ (resources.acquireStorageBuffer (FrameResources::StorageSlot::lights, 256), buffer);
        buffers.push_back (buffer);
        resources.endFrame();
    }

    EXPECT_NE (buffers[0], buffers[1]);
    EXPECT_NE (buffers[1], buffers[2]);
    EXPECT_NE (buffers[0], buffers[2]);
    EXPECT_EQ (buffers[3], buffers[0]);
}

TEST_F (FrameResourcesMetalTests, StorageBufferGrowsToTheNextPowerOfTwoOnItsTurn)
{
    FrameResources resources (device);

    resources.beginFrame (64, 64);
    auto small = resources.acquireStorageBuffer (FrameResources::StorageSlot::lights, 100);
    ASSERT_NE (small, nullptr);
    EXPECT_EQ (small->getSizeInBytes(), 128u);
    resources.endFrame();

    // Acquired every frame, so the ring is kept and its turn comes back to the small buffer
    for (int frame = 0; frame < 2; ++frame)
    {
        resources.beginFrame (64, 64);
        EXPECT_NE (resources.acquireStorageBuffer (FrameResources::StorageSlot::lights, 100), nullptr);
        resources.endFrame();
    }

    resources.beginFrame (64, 64);
    auto grown = resources.acquireStorageBuffer (FrameResources::StorageSlot::lights, 1000);
    ASSERT_NE (grown, nullptr);
    EXPECT_NE (grown, small);
    EXPECT_EQ (grown->getSizeInBytes(), 1024u);
    EXPECT_EQ (small->getReferenceCount(), 1);
    EXPECT_EQ (resources.acquireStorageBuffer (FrameResources::StorageSlot::lights, 512), grown);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, StorageSlotsAreIndependent)
{
    FrameResources resources (device);

    resources.beginFrame (64, 64);
    auto lights = resources.acquireStorageBuffer (FrameResources::StorageSlot::lights, 256);
    auto clusterLights = resources.acquireStorageBuffer (FrameResources::StorageSlot::clusterLights, 256);
    auto materials = resources.acquireStorageBuffer (FrameResources::StorageSlot::materials, 256);
    ASSERT_NE (lights, nullptr);
    EXPECT_NE (lights, clusterLights);
    EXPECT_NE (lights, materials);
    EXPECT_NE (clusterLights, materials);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, StorageSlotsNotAcquiredInAFrameAreReleased)
{
    FrameResources resources (device);

    resources.beginFrame (64, 64);
    auto lights = resources.acquireStorageBuffer (FrameResources::StorageSlot::lights, 256);
    auto materials = resources.acquireStorageBuffer (FrameResources::StorageSlot::materials, 256);
    ASSERT_NE (lights, nullptr);
    ASSERT_NE (materials, nullptr);
    resources.endFrame();

    // Only the materials are acquired again: the whole lights ring goes, the materials ring stays
    resources.beginFrame (64, 64);
    EXPECT_NE (resources.acquireStorageBuffer (FrameResources::StorageSlot::materials, 256), nullptr);
    resources.endFrame();

    EXPECT_EQ (lights->getReferenceCount(), 1);
    EXPECT_GT (materials->getReferenceCount(), 1);
}

TEST_F (FrameResourcesMetalTests, SharedSamplersAreCreatedOnceForEveryPass)
{
    FrameResources resources (device);

    resources.beginFrame (64, 64);
    const auto* samplers = resources.getSamplers();
    ASSERT_NE (samplers, nullptr);

    const auto& linear = samplers->linearClamp->getDescription();
    EXPECT_EQ (linear.minFilter, GpuFilter::linear);
    EXPECT_EQ (linear.mipmapFilter, GpuFilter::linear);
    EXPECT_EQ (linear.wrapU, GpuWrapMode::clampToEdge);
    EXPECT_FALSE (linear.compare.has_value());

    const auto& point = samplers->pointClamp->getDescription();
    EXPECT_EQ (point.minFilter, GpuFilter::nearest);
    EXPECT_EQ (point.wrapU, GpuWrapMode::clampToEdge);
    EXPECT_FALSE (point.compare.has_value());

    const auto& compare = samplers->shadowCompare->getDescription();
    EXPECT_EQ (compare.minFilter, GpuFilter::linear);
    EXPECT_EQ (compare.wrapU, GpuWrapMode::clampToEdge);
    EXPECT_EQ (compare.compare, std::optional<GpuCompareFunction> (GpuCompareFunction::lessEqual));
    resources.endFrame();

    // A resize releases the textures, never the samplers
    const auto linearClamp = samplers->linearClamp;
    resources.beginFrame (32, 32);
    ASSERT_EQ (resources.getSamplers(), samplers);
    EXPECT_EQ (resources.getSamplers()->linearClamp, linearClamp);
    resources.endFrame();
}

TEST_F (FrameResourcesMetalTests, ShadowCompareFailureKeepsTheOtherSamplersAndIsRemembered)
{
    FrameResources resources (device);
    resources.failShadowCompareSamplerForTesting();

    const auto* samplers = resources.getSamplers();
    ASSERT_NE (samplers, nullptr);
    EXPECT_NE (samplers->linearClamp, nullptr);
    EXPECT_NE (samplers->pointClamp, nullptr);
    EXPECT_EQ (samplers->shadowCompare, nullptr);

    // Never retried: later calls keep the same samplers and the missing comparison sampler
    const auto pointClamp = samplers->pointClamp;
    for (int call = 0; call < 2; ++call)
    {
        ASSERT_EQ (resources.getSamplers(), samplers);
        EXPECT_EQ (samplers->pointClamp, pointClamp);
        EXPECT_EQ (samplers->shadowCompare, nullptr);
    }
}
#endif

TEST (FrameResourcesTests, NoSharedSamplersWithoutADevice)
{
    FrameResources resources (nullptr);
    EXPECT_EQ (resources.getSamplers(), nullptr);
    EXPECT_EQ (resources.getSamplers(), nullptr);
}

TEST_F (FrameResourcesMetalTests, FindHistoryNeitherCreatesNorReallocatesNorKeeps)
{
    FrameResources resources (device);
    const GpuTextureDesc desc (64, 64, GpuTextureFormat::rgba16float, true);

    // Nothing to find before the owner created it, and finding creates nothing
    resources.beginFrame (64, 64);
    const auto missing = resources.findHistory ("taa");
    EXPECT_EQ (missing.read, nullptr);
    EXPECT_EQ (missing.write, nullptr);
    EXPECT_FALSE (missing.valid);
    EXPECT_EQ (resources.findHistory ("taa").read, nullptr);
    const auto owned = resources.getHistory ("taa", desc);
    resources.endFrame();

    // The owner's pair, whatever size the reader would have asked for, valid once written
    resources.beginFrame (64, 64);
    const auto found = resources.findHistory ("taa");
    EXPECT_EQ (found.read, owned.write);
    EXPECT_TRUE (found.valid);
    EXPECT_EQ (resources.getHistory ("taa", desc).read, owned.write);
    resources.endFrame();

    // Finding it does not keep it: a frame where only a reader looked releases it
    resources.beginFrame (64, 64);
    EXPECT_NE (resources.findHistory ("taa").read, nullptr);
    resources.endFrame();

    resources.beginFrame (64, 64);
    EXPECT_EQ (resources.findHistory ("taa").read, nullptr);
    resources.endFrame();
}
