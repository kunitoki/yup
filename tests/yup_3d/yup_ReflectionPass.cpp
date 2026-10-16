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

#include <yup_3d/yup_3d.h>

using namespace yup;

#if YUP_APPLE
#if YUP_ENABLE_SHADER_TRANSPILER
/** ReflectionPass on its own, for the failure paths SceneRenderer reaches. */
class ReflectionPassMetalTests : public SceneRendererMetalTests
{
};

TEST_F (ReflectionPassMetalTests, PipelinesThatCannotCompileGiveUpWithTheirReason)
{
    // As SceneRenderer calls it: without pipelines no Hi-Z pyramid is built, so the pass is handed
    // none, and must still report the pipelines, not the pyramid, as why it gave up
    ReflectionPass pass (device);
    pass.failPipelinesForTesting();
    EXPECT_TRUE (pass.prepare().failed());

    FrameResources resources (device);
    ViewData view;

    for (int i = 0; i < 2; ++i)
    {
        EXPECT_FALSE (pass.hasGivenUp());
        resources.beginFrame (64, 64);
        auto frame = GpuFrame::begin (device);
        EXPECT_EQ (pass.render (frame, view, nullptr, nullptr, nullptr, nullptr, 1.0f, resources), nullptr);
        frame.submit();
        resources.endFrame();
    }

    EXPECT_TRUE (pass.hasGivenUp());
    EXPECT_TRUE (pass.getGiveUpReason().contains ("pipelines failed for testing")) << pass.getGiveUpReason();
}
#endif
#endif
