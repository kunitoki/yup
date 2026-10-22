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

// ---------------------------------------------------------------------------
// GpuCanvas::beginDraw — scale (needs a real GPU)
// ---------------------------------------------------------------------------

class GpuCanvasMetalTests : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});
        if (gpuContext == nullptr)
            return;

        if (GpuCanvas::create (*gpuContext, 64, 64) == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";
    }

    static std::unique_ptr<GraphicsContext> gpuContext;
};

std::unique_ptr<GraphicsContext> GpuCanvasMetalTests::gpuContext;

TEST_F (GpuCanvasMetalTests, BeginDrawDefaultsToOnePixelPerUnit)
{
    auto canvas = GpuCanvas::create (*gpuContext, 128, 96);
    ASSERT_NE (canvas, nullptr);

    auto& g = canvas->beginDraw();

    EXPECT_FLOAT_EQ (1.0f, g.getContextScale());
    EXPECT_EQ (Rectangle<float> (0.0f, 0.0f, 128.0f, 96.0f), g.getDrawingArea());
}

TEST_F (GpuCanvasMetalTests, BeginDrawWithScaleSetsContextScaleAndLogicalDrawingArea)
{
    auto canvas = GpuCanvas::create (*gpuContext, 128, 96);
    ASSERT_NE (canvas, nullptr);

    auto& g = canvas->beginDraw ({}, 2.0f);

    EXPECT_EQ (128, canvas->getWidth());
    EXPECT_EQ (96, canvas->getHeight());
    EXPECT_FLOAT_EQ (2.0f, g.getContextScale());
    EXPECT_EQ (Rectangle<float> (0.0f, 0.0f, 64.0f, 48.0f), g.getDrawingArea());
}
