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

class GraphicsContextMetalTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        context = GraphicsContext::createContext (GpuPlatform::Metal, {});
        if (context == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";
    }

    void renderFrame()
    {
        rive::gpu::RenderContext::FrameDescriptor descriptor;
        descriptor.renderTargetWidth = static_cast<uint32_t> (size);
        descriptor.renderTargetHeight = static_cast<uint32_t> (size);

        context->begin (descriptor);
        context->end (nullptr);
    }

    static constexpr int size = 64;

    std::unique_ptr<GraphicsContext> context;
};

TEST_F (GraphicsContextMetalTests, FrameWithoutDrawableIsRetriedAfterSizingAgain)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "Sizing a context no window was attached to triggers an assertion when assertions are enabled or logged";
#endif

    // No window was attached, so there is no layer to hand out a drawable
    context->onSizeChanged (nullptr, size, size, 1.0f, 0);
    EXPECT_TRUE (context->isSurfaceAvailable());

    renderFrame();
    EXPECT_FALSE (context->isSurfaceAvailable());

    context->onSizeChanged (nullptr, size, size, 1.0f, 0);
    EXPECT_TRUE (context->isSurfaceAvailable());

    renderFrame();
    EXPECT_FALSE (context->isSurfaceAvailable());
}
