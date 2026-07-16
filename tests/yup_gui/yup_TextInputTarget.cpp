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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

// A TextInputTarget that does NOT inherit from Component, so the
// dynamic_cast<Component*>(this) path yields nullptr, keeping the
// native Component layer out of the picture.
class StandaloneTextInputTarget : public TextInputTarget
{
public:
    Rectangle<float> getTextInputRect() const override
    {
        return textInputRect;
    }

    Rectangle<float> textInputRect { 10.0f, 20.0f, 100.0f, 30.0f };
};

} // namespace

class TextInputTargetTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        target = std::make_unique<StandaloneTextInputTarget>();
    }

    void TearDown() override
    {
        target.reset();
    }

    std::unique_ptr<StandaloneTextInputTarget> target;
};

TEST_F (TextInputTargetTests, InitiallyNotActive)
{
    EXPECT_FALSE (target->isTextInputActive());
}

TEST_F (TextInputTargetTests, RequestTextInputActivates)
{
    target->requestTextInput();

    EXPECT_TRUE (target->isTextInputActive());
}

TEST_F (TextInputTargetTests, RequestTextInputIsIdempotent)
{
    target->requestTextInput();
    EXPECT_TRUE (target->isTextInputActive());

    target->requestTextInput();
    EXPECT_TRUE (target->isTextInputActive());
}

TEST_F (TextInputTargetTests, RelinquishTextInputDeactivates)
{
    target->requestTextInput();
    EXPECT_TRUE (target->isTextInputActive());

    target->relinquishTextInput();
    EXPECT_FALSE (target->isTextInputActive());
}

TEST_F (TextInputTargetTests, RelinquishTextInputIsIdempotent)
{
    target->requestTextInput();
    target->relinquishTextInput();
    EXPECT_FALSE (target->isTextInputActive());

    target->relinquishTextInput();
    EXPECT_FALSE (target->isTextInputActive());
}

TEST_F (TextInputTargetTests, RelinquishWithoutRequestDoesNothing)
{
    EXPECT_FALSE (target->isTextInputActive());

    target->relinquishTextInput();
    EXPECT_FALSE (target->isTextInputActive());
}

TEST_F (TextInputTargetTests, UpdateTextInputRectWhenInactiveIsNoOp)
{
    EXPECT_FALSE (target->isTextInputActive());

    target->updateTextInputRect();
    EXPECT_FALSE (target->isTextInputActive());
}

TEST_F (TextInputTargetTests, UpdateTextInputRectWhenActiveDoesNotCrash)
{
    target->requestTextInput();
    EXPECT_TRUE (target->isTextInputActive());

    target->updateTextInputRect();
    EXPECT_TRUE (target->isTextInputActive());
}

TEST_F (TextInputTargetTests, RequestRelinquishCycle)
{
    for (int i = 0; i < 5; ++i)
    {
        target->requestTextInput();
        EXPECT_TRUE (target->isTextInputActive());

        target->relinquishTextInput();
        EXPECT_FALSE (target->isTextInputActive());
    }
}

TEST_F (TextInputTargetTests, GetTextInputRect)
{
    const auto rect = target->getTextInputRect();

    EXPECT_FLOAT_EQ (10.0f, rect.getX());
    EXPECT_FLOAT_EQ (20.0f, rect.getY());
    EXPECT_FLOAT_EQ (100.0f, rect.getWidth());
    EXPECT_FLOAT_EQ (30.0f, rect.getHeight());
}
