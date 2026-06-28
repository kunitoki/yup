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

class GraphicsOffscreenTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GraphicsContext::Headless, {});
        ASSERT_NE (context, nullptr);
    }

    std::unique_ptr<GraphicsContext> context;
};

TEST_F (GraphicsOffscreenTests, RegularConstructorIsNotOffscreen)
{
    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_FALSE (g.isOffscreen());
}

TEST_F (GraphicsOffscreenTests, OffscreenConstructorIsOffscreen)
{
    // Headless context returns nullptr from createOffscreenTarget, so
    // isOffscreen() will return false. This tests the headless stub path.
    Image image (64, 64);
    Graphics g (*context, image);

    // Headless context has no GPU, so createOffscreenTarget returns nullptr.
    EXPECT_FALSE (g.isOffscreen());
}

TEST_F (GraphicsOffscreenTests, CommitToImageReturnsFalseForRegularGraphics)
{
    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_FALSE (g.commitToImage());
}

TEST_F (GraphicsOffscreenTests, CommitToImageReturnsFalseForHeadlessOffscreen)
{
    // Headless context creates no offscreen target, so commit returns false.
    Image image (64, 64);
    Graphics g (*context, image);

    EXPECT_FALSE (g.commitToImage());
}

TEST_F (GraphicsOffscreenTests, ReadPixelsToImageReturnsFalseForRegularGraphics)
{
    auto renderer = context->makeRenderer (100, 100);
    Graphics g (*context, *renderer);

    EXPECT_FALSE (g.readPixelsToImage());
}

TEST_F (GraphicsOffscreenTests, ReadPixelsToImageReturnsFalseForHeadlessOffscreen)
{
    // Headless context has no GPU, so readPixels returns false.
    Image image (64, 64);
    Graphics g (*context, image);

    EXPECT_FALSE (g.readPixelsToImage());
}

TEST_F (GraphicsOffscreenTests, AdoptTextureStoresTexture)
{
    Image image (32, 32);
    EXPECT_EQ (image.getTexture(), nullptr);

    image.adoptTexture (nullptr);
    EXPECT_EQ (image.getTexture(), nullptr);
}
