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

//==============================================================================
/** Draws with the platform's GPU context, through GraphicsGpuPixelTestHost (see native/). */
class PaintSourceTests : public ::testing::Test
{
protected:
    /** Fills its content box red, counting how often it is drawn. */
    struct CountingSource : PaintSource
    {
        Size<float> getContentSize() const override { return contentSize; }

        void paintContent (Graphics& g) override
        {
            ++paintCount;

            if (paintsItself)
                g.setFillSource (*this);
            else
                g.setFillColor (Colors::red);

            g.fillAll();
        }

        Size<float> contentSize { 32.0f, 32.0f };
        int paintCount = 0;
        bool paintsItself = false;
    };

    static void SetUpTestSuite()
    {
        host = std::make_unique<GraphicsGpuPixelTestHost>();
        gpuContext = host->getContext();
    }

    static void TearDownTestSuite()
    {
        gpuContext = nullptr;
        host.reset();
    }

    void SetUp() override
    {
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No GPU graphics context available";

        host->makeCurrent();
    }

    void TearDown() override
    {
        source.releaseResources();
    }

    /** Draws one frame filling the canvas with the source at the given scale, returning its RGBA pixels. */
    std::vector<uint8> renderFrame (float sourceScale = 1.0f, int fills = 1)
    {
        auto canvas = GpuCanvas::create (*gpuContext, size, size);
        if (canvas == nullptr)
            return {};

        auto& g = canvas->beginDraw();
        for (int i = 0; i < fills; ++i)
        {
            g.setFillSource (source, AffineTransform::scaling (sourceScale));
            g.fillRect (0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size));
        }

        std::vector<uint8> pixels (static_cast<std::size_t> (size * size * 4));
        if (! canvas->commit() || ! canvas->readPixels (pixels.data(), pixels.size()))
            return {};

        return pixels;
    }

    static int alphaAt (const std::vector<uint8>& pixels, int x, int y)
    {
        return pixels[static_cast<std::size_t> ((y * size + x) * 4 + 3)];
    }

    static constexpr int size = 64;
    static std::unique_ptr<GraphicsGpuPixelTestHost> host;
    static GraphicsContext* gpuContext;

    CountingSource source;
};

std::unique_ptr<GraphicsGpuPixelTestHost> PaintSourceTests::host;
GraphicsContext* PaintSourceTests::gpuContext = nullptr;

TEST_F (PaintSourceTests, ContentIsDrawnOnceAndPaintsTheShape)
{
    const auto pixels = renderFrame (1.0f, 3);
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_EQ (source.paintCount, 1);
    EXPECT_GT (alphaAt (pixels, 16, 16), 200);
}

TEST_F (PaintSourceTests, CachedContentIsReusedUntilMarkedDirty)
{
    renderFrame();
    renderFrame();
    EXPECT_EQ (source.paintCount, 1);

    source.markDirty();
    renderFrame();
    renderFrame();
    EXPECT_EQ (source.paintCount, 2);
}

TEST_F (PaintSourceTests, ReleasingResourcesDrawsTheContentAgain)
{
    renderFrame();
    source.releaseResources();
    renderFrame();

    EXPECT_EQ (source.paintCount, 2);
}

TEST_F (PaintSourceTests, GrowingOnScreenDrawsTheContentAgain)
{
    renderFrame (1.0f);
    renderFrame (4.0f);

    EXPECT_EQ (source.paintCount, 2);
}

TEST_F (PaintSourceTests, ShrinkingOnScreenKeepsTheRasterForAWhile)
{
    renderFrame (4.0f);
    renderFrame (1.0f);
    EXPECT_EQ (source.paintCount, 1);

    // The raster is given back once the content has been drawn smaller for long enough, then kept
    for (int i = 0; i < 60; ++i)
        renderFrame (1.0f);

    EXPECT_EQ (source.paintCount, 2);
}

TEST_F (PaintSourceTests, UsingTheSourceInsideItsOwnContentDrawsNothing)
{
    source.paintsItself = true;

    const auto pixels = renderFrame();
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_EQ (source.paintCount, 1);
    EXPECT_LT (alphaAt (pixels, 16, 16), 50);
}

TEST_F (PaintSourceTests, EmptyContentDrawsNothing)
{
    source.contentSize = { 0.0f, 32.0f };

    const auto pixels = renderFrame();
    ASSERT_EQ (pixels.size(), static_cast<std::size_t> (size * size * 4));

    EXPECT_EQ (source.paintCount, 0);
    EXPECT_LT (alphaAt (pixels, 16, 16), 50);
}

TEST (PaintSourceHeadlessTests, WithoutAGpuTheContentIsNeverDrawn)
{
    struct Source : PaintSource
    {
        Size<float> getContentSize() const override { return { 32.0f, 32.0f }; }

        void paintContent (Graphics&) override { ++paintCount; }

        int paintCount = 0;
    };

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);
    auto renderer = context->makeRenderer (64, 64);
    ASSERT_NE (renderer, nullptr);

    Source source;
    {
        Graphics g (*context, *renderer);
        g.setFillSource (source);
        g.fillRect (0.0f, 0.0f, 64.0f, 64.0f);
        g.setStrokeSource (source);
        g.strokeLine (0.0f, 0.0f, 64.0f, 64.0f);
    }

    EXPECT_EQ (source.paintCount, 0);
}
