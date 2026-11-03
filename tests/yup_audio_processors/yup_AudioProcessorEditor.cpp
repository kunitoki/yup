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

#include <yup_audio_processors/yup_audio_processors.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class AudioProcessorEditorTests : public ::testing::Test
{
protected:
    struct TestEditor final : AudioProcessorEditor
    {
        bool isResizable() const override { return resizable; }
        Size<int> getPreferredSize() const override { return { 400, 200 }; }
        bool shouldPreserveAspectRatio() const override { return preserveAspectRatio; }

        bool resizable = true;
        bool preserveAspectRatio = false;
    };

    void SetUp() override
    {
        editor.setSize ({ 400.0f, 200.0f });
    }

    TestEditor editor;
};

TEST_F (AudioProcessorEditorTests, NonResizableEditorReturnsPreferredSize)
{
    editor.resizable = false;

    EXPECT_EQ (Size<int> (400, 200), editor.getConstrainedSize ({ 900, 50 }));
}

TEST_F (AudioProcessorEditorTests, FreeResizableEditorKeepsRequestedSize)
{
    EXPECT_EQ (Size<int> (900, 50), editor.getConstrainedSize ({ 900, 50 }));
    EXPECT_EQ (Size<int> (1, 1), editor.getConstrainedSize ({ 0, -10 }));
}

TEST_F (AudioProcessorEditorTests, WiderRequestDerivesHeightFromWidth)
{
    editor.preserveAspectRatio = true;

    EXPECT_EQ (Size<int> (800, 400), editor.getConstrainedSize ({ 800, 210 }));
}

TEST_F (AudioProcessorEditorTests, TallerRequestDerivesWidthFromHeight)
{
    editor.preserveAspectRatio = true;

    EXPECT_EQ (Size<int> (600, 300), editor.getConstrainedSize ({ 410, 300 }));
}

TEST_F (AudioProcessorEditorTests, AspectRatioDoesNotDependOnCurrentSize)
{
    editor.preserveAspectRatio = true;

    const auto constrained = editor.getConstrainedSize ({ 840, 400 });
    editor.setSize (constrained.to<float>());

    EXPECT_EQ (Size<int> (840, 420), constrained);
    EXPECT_EQ (constrained, editor.getConstrainedSize ({ 840, 400 }));
}

TEST_F (AudioProcessorEditorTests, ShowsResizableCornerOnlyWhenResizable)
{
    EXPECT_TRUE (editor.shouldShowResizableCorner());

    editor.resizable = false;
    EXPECT_FALSE (editor.shouldShowResizableCorner());
}
