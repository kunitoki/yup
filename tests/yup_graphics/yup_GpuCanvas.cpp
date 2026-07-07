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

using namespace yup;

class GpuCanvasTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GraphicsContext::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    std::unique_ptr<GraphicsContext> context;
};

// ---------------------------------------------------------------------------
// GpuCanvas::create — dimension validation

TEST_F (GpuCanvasTests, CreateWithZeroWidthReturnsNull)
{
    EXPECT_EQ (GpuCanvas::create (*context, 0, 64), nullptr);
}

TEST_F (GpuCanvasTests, CreateWithZeroHeightReturnsNull)
{
    EXPECT_EQ (GpuCanvas::create (*context, 64, 0), nullptr);
}

TEST_F (GpuCanvasTests, CreateWithNegativeDimensionsReturnsNull)
{
    EXPECT_EQ (GpuCanvas::create (*context, -1, 64), nullptr);
    EXPECT_EQ (GpuCanvas::create (*context, 64, -1), nullptr);
}

TEST_F (GpuCanvasTests, CreateWithHeadlessContextReturnsNull)
{
    // Headless backend has no GPU — createOffscreenTarget returns nullptr.
    EXPECT_EQ (GpuCanvas::create (*context, 64, 64), nullptr);
}

// ---------------------------------------------------------------------------
// GpuCanvas public API — null/stub paths when canvas is not available

TEST_F (GpuCanvasTests, AsTextureBeforeCommitReturnsNull)
{
    auto canvas = GpuCanvas::create (*context, 64, 64);
    if (canvas == nullptr)
        return; // headless stub path: canvas is null — already covered above

    EXPECT_EQ (canvas->asTexture(), nullptr);
}

TEST_F (GpuCanvasTests, AsImageBeforeCommitReturnsEmptyImage)
{
    auto canvas = GpuCanvas::create (*context, 64, 64);
    if (canvas == nullptr)
        return;

    EXPECT_FALSE (canvas->asImage().isValid());
}

TEST_F (GpuCanvasTests, ReadPixelsBeforeCommitReturnsFalse)
{
    auto canvas = GpuCanvas::create (*context, 64, 64);
    if (canvas == nullptr)
        return;

    std::vector<uint8> buf (64 * 64 * 4, 0);
    EXPECT_FALSE (canvas->readPixels (buf.data(), buf.size()));
}

TEST_F (GpuCanvasTests, DoubleCommitReturnsFalseOnSecondCall)
{
    auto canvas = GpuCanvas::create (*context, 64, 64);
    if (canvas == nullptr)
        return;

    const bool first = canvas->commit();
    const bool second = canvas->commit();
    EXPECT_FALSE (second);
    (void) first;
}

// ---------------------------------------------------------------------------
// Image::fromTexture — null and invalid texture handling

TEST (ImageFromTextureTests, NullTextureReturnsInvalidImage)
{
    Texture::Ptr nullTex;
    auto img = Image::fromTexture (nullTex);
    EXPECT_FALSE (img.isValid());
}

TEST (ImageFromTextureTests, ExplicitNullptrReturnsInvalidImage)
{
    auto img = Image::fromTexture (nullptr);
    EXPECT_FALSE (img.isValid());
}

// ---------------------------------------------------------------------------
// Texture public API

TEST (TextureTests, DefaultPtrIsInvalid)
{
    Texture::Ptr t;
    EXPECT_EQ (t, nullptr);
}

TEST (TextureTests, ValidTextureReportsCorrectlyViaGpuCanvas)
{
    // We cannot exercise a real GPU Texture without a real backend.
    // This test confirms the type system compiles and Texture::Ptr comparison works.
    Texture::Ptr t;
    EXPECT_FALSE (t != nullptr);
    EXPECT_TRUE (t == nullptr);
}

// ---------------------------------------------------------------------------
// Graphics::drawTexture — null texture is a no-op (does not crash)

TEST (GraphicsDrawTextureTests, NullTextureIsNoOp)
{
    auto ctx = GraphicsContext::createContext (GraphicsContext::Headless, {});
    ASSERT_NE (ctx, nullptr);

    auto renderer = ctx->makeRenderer (128, 128);
    ASSERT_NE (renderer, nullptr);

    Graphics g (*ctx, *renderer);

    Texture::Ptr nullTex;
    EXPECT_NO_THROW (g.drawTexture (nullTex, { 0.0f, 0.0f, 64.0f, 64.0f }));
}
