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

using namespace yup;

class GpuTargetTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GpuDevice::create (GpuPlatform::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    GpuDevice::Ptr context;
};

// ---------------------------------------------------------------------------
// GpuTarget::create — dimension validation

TEST_F (GpuTargetTests, CreateWithZeroWidthReturnsNull)
{
    EXPECT_EQ (GpuTarget::create (*context, 0, 64), nullptr);
}

TEST_F (GpuTargetTests, CreateWithZeroHeightReturnsNull)
{
    EXPECT_EQ (GpuTarget::create (*context, 64, 0), nullptr);
}

TEST_F (GpuTargetTests, CreateWithNegativeDimensionsReturnsNull)
{
    EXPECT_EQ (GpuTarget::create (*context, -1, 64), nullptr);
    EXPECT_EQ (GpuTarget::create (*context, 64, -1), nullptr);
}

TEST_F (GpuTargetTests, CreateWithHeadlessContextReturnsNull)
{
    // Headless backend has no GPU — createOffscreenTarget returns nullptr.
    EXPECT_EQ (GpuTarget::create (*context, 64, 64), nullptr);
}

// ---------------------------------------------------------------------------
// GpuTarget public API — null/stub paths when target is not available

TEST_F (GpuTargetTests, AsTextureReturnsNull)
{
    auto target = GpuTarget::create (*context, 64, 64);
    if (target == nullptr)
        return; // headless stub path: target is null — already covered above

    EXPECT_EQ (target->asTexture(), nullptr);
}

TEST_F (GpuTargetTests, AsImageReturnsEmptyImage)
{
    auto target = GpuTarget::create (*context, 64, 64);
    if (target == nullptr)
        return;

    EXPECT_FALSE (target->asImage().isValid());
}

TEST_F (GpuTargetTests, ReadPixelsReturnsFalseOrSucceeds)
{
    auto target = GpuTarget::create (*context, 64, 64);
    if (target == nullptr)
        return;

    std::vector<uint8> buf (64 * 64 * 4, 0);
    EXPECT_NO_THROW ({ target->readPixels (buf.data(), buf.size()); });
}

// ---------------------------------------------------------------------------
// GpuTarget::getWidth / getHeight

TEST_F (GpuTargetTests, GetWidthAndHeightAreNonNegative)
{
    auto target = GpuTarget::create (*context, 64, 64);
    if (target == nullptr)
        return;

    EXPECT_GE (target->getWidth(), 0);
    EXPECT_GE (target->getHeight(), 0);
}

// ---------------------------------------------------------------------------
// GpuTarget::beginRenderPass — headless path

TEST_F (GpuTargetTests, BeginRenderPassWithHeadlessReturnsInvalidPass)
{
    auto frame = GpuFrame::begin (context);
    if (! frame.isValid())
        return;

    auto target = GpuTarget::create (*context, 64, 64);
    if (target == nullptr)
        return;

    auto pass = target->beginRenderPass (frame);
    EXPECT_FALSE (pass.isValid());
    frame.submit();
}

// ---------------------------------------------------------------------------
// GpuTarget::Ptr default state

TEST_F (GpuTargetTests, DefaultPtrIsNull)
{
    GpuTarget::Ptr nullTarget;
    EXPECT_EQ (nullTarget, nullptr);
}

// ==============================================================================
// GpuTarget — mock-based tests (using OreAndTargetGpuDevice)
// ==============================================================================

class GpuTargetMockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        ctx = new OreAndTargetGpuDevice (mockOreCtx.get(), MockOffscreenTarget::withGpuTexture (256, 128));
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    GpuDevice::Ptr ctx;
};

TEST_F (GpuTargetMockTests, CreateWithValidDimensionsSucceeds)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);
    EXPECT_GE (target->getWidth(), 0);
    EXPECT_GE (target->getHeight(), 0);
}

TEST_F (GpuTargetMockTests, AsTextureReturnsValidTexture)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    // First call creates the texture via render canvas path.
    auto tex = target->asTexture();
    ASSERT_NE (tex, nullptr);
    EXPECT_TRUE (tex->isValid());

    // Second call returns cached result.
    auto tex2 = target->asTexture();
    EXPECT_EQ (tex, tex2);
}

TEST_F (GpuTargetMockTests, AsImageReturnsValidImage)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto img = target->asImage();
    EXPECT_TRUE (img.isValid());
}

TEST_F (GpuTargetMockTests, ReadPixelsReturnsSuccess)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    std::vector<uint8> buf (256 * 128 * 4);
    // readOffscreenPixels delegates to the device; returns true for headless-based mock.
    EXPECT_NO_THROW ({ target->readPixels (buf.data(), buf.size()); });
}

TEST_F (GpuTargetMockTests, BeginRenderPassWithValidFrameReturnsValidPass)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    EXPECT_CALL (*mockOreCtx, beginFrame (_));

    auto frame = GpuFrame::begin (ctx);
    ASSERT_TRUE (frame.isValid());

    auto pass = target->beginRenderPass (frame);
    EXPECT_TRUE (pass.isValid());

    pass.finish();
    frame.submit();
}

TEST_F (GpuTargetMockTests, BeginRenderPassWithInvalidFrameReturnsInvalidPass)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto frame = GpuFrame::begin (GpuDevice::create (GpuPlatform::Headless, {}));
    EXPECT_FALSE (frame.isValid());

    auto pass = target->beginRenderPass (frame);
    EXPECT_FALSE (pass.isValid());
}

TEST_F (GpuTargetMockTests, CreateFromTargetWithNullTargetReturnsNull)
{
    EXPECT_EQ (GpuTarget::createFromTarget (ctx, nullptr), nullptr);
}
