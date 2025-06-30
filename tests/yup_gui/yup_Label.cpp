/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
const String kTestText = "Hello, World!";
const String kEmptyText = "";
const String kLongText = "This is a very long label text that might span multiple lines";
constexpr float kTestStrokeWidth = 1.5f;
constexpr float kZeroStroke = 0.0f;
} // namespace

class LabelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        label = std::make_unique<Label> ("testLabel");
        label->setBounds (0, 0, 200, 50);
    }

    std::unique_ptr<Label> label;
};

TEST_F (LabelTest, ConstructorInitializesCorrectly)
{
    EXPECT_TRUE (label->getText().isEmpty());
    EXPECT_EQ (String ("testLabel"), label->getComponentID());
    EXPECT_EQ (kZeroStroke, label->getStrokeWidth());
    EXPECT_FALSE (label->getFont().has_value());
}

TEST_F (LabelTest, TextGetterAndSetter)
{
    EXPECT_TRUE (label->getText().isEmpty());

    label->setText (kTestText, dontSendNotification);
    EXPECT_EQ (kTestText, label->getText());

    label->setText (kEmptyText, dontSendNotification);
    EXPECT_TRUE (label->getText().isEmpty());
}

TEST_F (LabelTest, TextWithSpecialCharacters)
{
    const String specialText = "Special\nText\t&<>";
    label->setText (specialText, dontSendNotification);
    EXPECT_EQ (specialText, label->getText());
}

TEST_F (LabelTest, TextWithUnicode)
{
    const String unicodeText = L"Üñíçødé Téxt 🚀";
    label->setText (unicodeText, dontSendNotification);
    EXPECT_EQ (unicodeText, label->getText());
}

TEST_F (LabelTest, LongText)
{
    label->setText (kLongText, dontSendNotification);
    EXPECT_EQ (kLongText, label->getText());
}

TEST_F (LabelTest, VeryLongText)
{
    String veryLongText;
    for (int i = 0; i < 1000; ++i)
        veryLongText += "A";

    label->setText (veryLongText, dontSendNotification);
    EXPECT_EQ (veryLongText, label->getText());
}

TEST_F (LabelTest, EmptyStringHandling)
{
    label->setText (kTestText, dontSendNotification);
    EXPECT_EQ (kTestText, label->getText());

    label->setText (String(), dontSendNotification);
    EXPECT_TRUE (label->getText().isEmpty());

    label->setText ("", dontSendNotification);
    EXPECT_TRUE (label->getText().isEmpty());
}

TEST_F (LabelTest, MultipleTextChanges)
{
    const StringArray testTexts = { "First", "Second", "Third", "Fourth" };

    for (const auto& text : testTexts)
    {
        label->setText (text, dontSendNotification);
        EXPECT_EQ (text, label->getText());
    }
}

TEST_F (LabelTest, TextWithWhitespace)
{
    const String whitespaceText = "  Text with spaces  ";
    label->setText (whitespaceText, dontSendNotification);
    EXPECT_EQ (whitespaceText, label->getText());

    const String tabText = "\tTabbed\tText\t";
    label->setText (tabText, dontSendNotification);
    EXPECT_EQ (tabText, label->getText());

    const String newlineText = "Multi\nLine\nText";
    label->setText (newlineText, dontSendNotification);
    EXPECT_EQ (newlineText, label->getText());
}

TEST_F (LabelTest, FontGetterAndSetter)
{
    EXPECT_FALSE (label->getFont().has_value());

    // Create a test font
    Font testFont;
    label->setFont (testFont);

    auto retrievedFont = label->getFont();
    EXPECT_TRUE (retrievedFont.has_value());
}

TEST_F (LabelTest, FontReset)
{
    // Set a custom font
    Font testFont;
    label->setFont (testFont);
    EXPECT_TRUE (label->getFont().has_value());

    // Reset to theme font
    label->resetFont();
    EXPECT_FALSE (label->getFont().has_value());
}

TEST_F (LabelTest, StrokeWidthGetterAndSetter)
{
    EXPECT_EQ (kZeroStroke, label->getStrokeWidth());

    label->setStrokeWidth (kTestStrokeWidth);
    EXPECT_EQ (kTestStrokeWidth, label->getStrokeWidth());

    label->setStrokeWidth (kZeroStroke);
    EXPECT_EQ (kZeroStroke, label->getStrokeWidth());
}

TEST_F (LabelTest, NegativeStrokeWidth)
{
    label->setStrokeWidth (-1.0f);
    EXPECT_EQ (-1.0f, label->getStrokeWidth());
}

TEST_F (LabelTest, LargeStrokeWidth)
{
    const float largeStroke = 100.0f;
    label->setStrokeWidth (largeStroke);
    EXPECT_EQ (largeStroke, label->getStrokeWidth());
}

TEST_F (LabelTest, VerySmallStrokeWidth)
{
    const float smallStroke = 0.001f;
    label->setStrokeWidth (smallStroke);
    EXPECT_EQ (smallStroke, label->getStrokeWidth());
}

TEST_F (LabelTest, TextIndependentOfStroke)
{
    label->setText (kTestText, dontSendNotification);
    label->setStrokeWidth (kTestStrokeWidth);

    EXPECT_EQ (kTestText, label->getText());
    EXPECT_EQ (kTestStrokeWidth, label->getStrokeWidth());

    label->setText ("New Text", dontSendNotification);
    EXPECT_EQ (kTestStrokeWidth, label->getStrokeWidth());

    label->setStrokeWidth (2.0f);
    EXPECT_EQ (String ("New Text"), label->getText());
}

TEST_F (LabelTest, FontIndependentOfText)
{
    Font testFont;
    label->setFont (testFont);
    label->setText (kTestText, dontSendNotification);

    EXPECT_TRUE (label->getFont().has_value());
    EXPECT_EQ (kTestText, label->getText());

    label->setText ("New Text", dontSendNotification);
    EXPECT_TRUE (label->getFont().has_value());

    label->resetFont();
    EXPECT_EQ (String ("New Text"), label->getText());
}

TEST_F (LabelTest, StrokeIndependentOfFont)
{
    Font testFont;
    label->setFont (testFont);
    label->setStrokeWidth (kTestStrokeWidth);

    EXPECT_TRUE (label->getFont().has_value());
    EXPECT_EQ (kTestStrokeWidth, label->getStrokeWidth());

    label->resetFont();
    EXPECT_EQ (kTestStrokeWidth, label->getStrokeWidth());

    label->setFont (testFont);
    EXPECT_EQ (kTestStrokeWidth, label->getStrokeWidth());
}

TEST_F (LabelTest, ComponentIdIsSet)
{
    auto newLabel = std::make_unique<Label> ("uniqueLabelId");
    EXPECT_EQ (String ("uniqueLabelId"), newLabel->getComponentID());
}

TEST_F (LabelTest, BoundsAndSizeWork)
{
    Rectangle<int> bounds (10, 20, 150, 30);
    label->setBounds (bounds);

    EXPECT_EQ (bounds.to<float>(), label->getBounds());
    EXPECT_EQ (150, label->getWidth());
    EXPECT_EQ (30, label->getHeight());
}

TEST_F (LabelTest, TextChangeWithNotification)
{
    // Test that setText with sendNotification doesn't crash
    label->setText (kTestText, sendNotification);
    EXPECT_EQ (kTestText, label->getText());

    label->setText ("New Text", sendNotification);
    EXPECT_EQ (String ("New Text"), label->getText());
}

TEST_F (LabelTest, TextPersistenceAfterBoundsChange)
{
    label->setText (kTestText, dontSendNotification);
    EXPECT_EQ (kTestText, label->getText());

    label->setBounds (50, 50, 300, 100);
    EXPECT_EQ (kTestText, label->getText());
}

TEST_F (LabelTest, StrokePersistenceAfterBoundsChange)
{
    label->setStrokeWidth (kTestStrokeWidth);
    EXPECT_EQ (kTestStrokeWidth, label->getStrokeWidth());

    label->setBounds (50, 50, 300, 100);
    EXPECT_EQ (kTestStrokeWidth, label->getStrokeWidth());
}

TEST_F (LabelTest, FontPersistenceAfterBoundsChange)
{
    Font testFont;
    label->setFont (testFont);
    EXPECT_TRUE (label->getFont().has_value());

    label->setBounds (50, 50, 300, 100);
    EXPECT_TRUE (label->getFont().has_value());
}

TEST_F (LabelTest, LabelWithZeroSize)
{
    label->setBounds (0, 0, 0, 0);
    label->setText (kTestText, dontSendNotification);

    EXPECT_EQ (kTestText, label->getText());
    EXPECT_EQ (0, label->getWidth());
    EXPECT_EQ (0, label->getHeight());
}

TEST_F (LabelTest, LabelWithVerySmallSize)
{
    label->setBounds (0, 0, 1, 1);
    label->setText (kTestText, dontSendNotification);

    EXPECT_EQ (kTestText, label->getText());
    EXPECT_EQ (1, label->getWidth());
    EXPECT_EQ (1, label->getHeight());
}

TEST_F (LabelTest, MultiplePropertyChanges)
{
    Font testFont;

    for (int i = 0; i < 5; ++i)
    {
        String text = "Text " + String (i);
        float stroke = static_cast<float> (i) * 0.5f;

        label->setText (text, dontSendNotification);
        label->setStrokeWidth (stroke);

        if (i % 2 == 0)
            label->setFont (testFont);
        else
            label->resetFont();

        EXPECT_EQ (text, label->getText());
        EXPECT_EQ (stroke, label->getStrokeWidth());
        EXPECT_EQ (i % 2 == 0, label->getFont().has_value());
    }
}

TEST_F (LabelTest, TextWithNumbers)
{
    const String numericText = "Label 123";
    label->setText (numericText, dontSendNotification);
    EXPECT_EQ (numericText, label->getText());

    const String numbersOnly = "12345";
    label->setText (numbersOnly, dontSendNotification);
    EXPECT_EQ (numbersOnly, label->getText());
}
