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

#include <yup_graphics/yup_graphics.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class AnchoredTextTests : public ::testing::Test
{
protected:
    static Font loadFont (float height)
    {
        const auto file =
#if YUP_EMSCRIPTEN
            File ("/")
#else
            File (__FILE__)
                .getParentDirectory()
                .getParentDirectory()
#endif
                .getChildFile ("data")
                .getChildFile ("fonts")
                .getChildFile ("Linefont-VariableFont_wdth,wght.ttf");

        auto result = Font::loadFontFromFile (file);
        EXPECT_TRUE (result.wasOk());
        return result.getValue().withHeight (height);
    }
};

//==============================================================================
TEST_F (AnchoredTextTests, EmptyTextHasNothingToDraw)
{
    EXPECT_TRUE (AnchoredText().isEmpty());
    EXPECT_TRUE (AnchoredText ("", loadFont (16.0f), { 10.0f, 20.0f }).isEmpty());
}

TEST_F (AnchoredTextTests, PlacesTheBaselineOnTheAnchorPoint)
{
    const auto font = loadFont (20.0f);
    const AnchoredText text ("Hello", font, { 10.0f, 50.0f });

    ASSERT_FALSE (text.isEmpty());
    EXPECT_FALSE (text.getStyledText().isEmpty());

    // The text starts at the anchor, and its top is an ascent above the baseline
    EXPECT_FLOAT_EQ (text.getBounds().getX(), 10.0f);
    EXPECT_LT (text.getBounds().getY(), 50.0f);
    EXPECT_GT (text.getBounds().getBottom(), 50.0f);
}

TEST_F (AnchoredTextTests, AnchorsTheStartTheMiddleOrTheEndOfTheText)
{
    const auto font = loadFont (20.0f);
    const Point<float> anchor (100.0f, 50.0f);

    const AnchoredText start ("Hello", font, anchor, AnchoredText::Anchor::start);
    const AnchoredText middle ("Hello", font, anchor, AnchoredText::Anchor::middle);
    const AnchoredText end ("Hello", font, anchor, AnchoredText::Anchor::end);

    const auto width = start.getBounds().getWidth();
    ASSERT_GT (width, 0.0f);

    EXPECT_FLOAT_EQ (start.getBounds().getX(), 100.0f);
    EXPECT_FLOAT_EQ (middle.getBounds().getCenterX(), 100.0f);
    EXPECT_FLOAT_EQ (end.getBounds().getRight(), 100.0f);
    EXPECT_FLOAT_EQ (middle.getBounds().getY(), start.getBounds().getY());
}

TEST_F (AnchoredTextTests, LetterSpacingWidensTheText)
{
    const auto font = loadFont (20.0f);

    const AnchoredText plain ("Hello", font, {});
    const AnchoredText spaced ("Hello", font, {}, AnchoredText::Anchor::start, 6.0f);

    EXPECT_GT (spaced.getBounds().getWidth(), plain.getBounds().getWidth());
}
