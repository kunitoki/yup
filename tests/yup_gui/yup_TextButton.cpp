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
const String kTestButtonText = "Click Me";
const String kEmptyText = "";
const String kLongText = "This is a very long button text that might need to be handled properly";
} // namespace

class TextButtonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        textButton = std::make_unique<TextButton> ("testTextButton");
        textButton->setBounds (0, 0, 100, 30);
    }

    std::unique_ptr<TextButton> textButton;
};

TEST_F (TextButtonTest, ConstructorInitializesCorrectly)
{
    EXPECT_FALSE (textButton->getButtonText().isEmpty());
    EXPECT_EQ (String ("testTextButton"), textButton->getComponentID());
}

TEST_F (TextButtonTest, DefaultConstructorWorks)
{
    auto defaultButton = std::make_unique<TextButton>();

    EXPECT_TRUE (defaultButton->getButtonText().isEmpty());
    EXPECT_TRUE (defaultButton->getComponentID().isEmpty());
}

TEST_F (TextButtonTest, ButtonTextGetterAndSetter)
{
    EXPECT_FALSE (textButton->getButtonText().isEmpty());

    textButton->setButtonText (kTestButtonText);
    EXPECT_EQ (kTestButtonText, textButton->getButtonText());

    textButton->setButtonText (kEmptyText);
    EXPECT_TRUE (textButton->getButtonText().isEmpty());
}

TEST_F (TextButtonTest, ButtonTextWithSpecialCharacters)
{
    const String specialText = "Special\nText\t&<>";
    textButton->setButtonText (specialText);
    EXPECT_EQ (specialText, textButton->getButtonText());
}

TEST_F (TextButtonTest, ButtonTextWithUnicode)
{
    const String unicodeText = L"Üñíçødé Téxt 🚀";
    textButton->setButtonText (unicodeText);
    EXPECT_EQ (unicodeText, textButton->getButtonText());
}

TEST_F (TextButtonTest, LongButtonText)
{
    textButton->setButtonText (kLongText);
    EXPECT_EQ (kLongText, textButton->getButtonText());
}

TEST_F (TextButtonTest, VeryLongButtonText)
{
    String veryLongText;
    for (int i = 0; i < 1000; ++i)
        veryLongText += "A";

    textButton->setButtonText (veryLongText);
    EXPECT_EQ (veryLongText, textButton->getButtonText());
}

TEST_F (TextButtonTest, EmptyStringHandling)
{
    textButton->setButtonText (kTestButtonText);
    EXPECT_EQ (kTestButtonText, textButton->getButtonText());

    textButton->setButtonText (String());
    EXPECT_TRUE (textButton->getButtonText().isEmpty());

    textButton->setButtonText ("");
    EXPECT_TRUE (textButton->getButtonText().isEmpty());
}

TEST_F (TextButtonTest, MultipleTextChanges)
{
    const StringArray testTexts = { "First", "Second", "Third", "Fourth" };

    for (const auto& text : testTexts)
    {
        textButton->setButtonText (text);
        EXPECT_EQ (text, textButton->getButtonText());
    }
}

TEST_F (TextButtonTest, TextWithWhitespace)
{
    const String whitespaceText = "  Text with spaces  ";
    textButton->setButtonText (whitespaceText);
    EXPECT_EQ (whitespaceText, textButton->getButtonText());

    const String tabText = "\tTabbed\tText\t";
    textButton->setButtonText (tabText);
    EXPECT_EQ (tabText, textButton->getButtonText());

    const String newlineText = "Multi\nLine\nText";
    textButton->setButtonText (newlineText);
    EXPECT_EQ (newlineText, textButton->getButtonText());
}

TEST_F (TextButtonTest, TextWithNumbers)
{
    const String numericText = "Button 123";
    textButton->setButtonText (numericText);
    EXPECT_EQ (numericText, textButton->getButtonText());

    const String numbersOnly = "12345";
    textButton->setButtonText (numbersOnly);
    EXPECT_EQ (numbersOnly, textButton->getButtonText());
}

TEST_F (TextButtonTest, ComponentIdIsSet)
{
    auto newButton = std::make_unique<TextButton> ("uniqueTextButtonId");
    EXPECT_EQ (String ("uniqueTextButtonId"), newButton->getComponentID());
}

TEST_F (TextButtonTest, BoundsAndSizeWork)
{
    Rectangle<int> bounds (10, 20, 80, 25);
    textButton->setBounds (bounds);

    EXPECT_EQ (bounds.to<float>(), textButton->getBounds());
    EXPECT_EQ (80, textButton->getWidth());
    EXPECT_EQ (25, textButton->getHeight());
}

TEST_F (TextButtonTest, IsButtonType)
{
    // TextButton inherits from Button
    Button* baseButton = textButton.get();
    EXPECT_NE (nullptr, baseButton);
}

TEST_F (TextButtonTest, GetTextBoundsReturnsValidRectangle)
{
    textButton->setButtonText (kTestButtonText);
    auto textBounds = textButton->getTextBounds();

    // Text bounds should be within the button bounds
    EXPECT_GE (textBounds.getX(), 0.0f);
    EXPECT_GE (textBounds.getY(), 0.0f);
    EXPECT_LE (textBounds.getRight(), static_cast<float> (textButton->getWidth()));
    EXPECT_LE (textBounds.getBottom(), static_cast<float> (textButton->getHeight()));
}

TEST_F (TextButtonTest, GetTextBoundsWithEmptyText)
{
    // Test with empty text
    auto emptyTextBounds = textButton->getTextBounds();

    // Bounds should still be valid even with empty text
    EXPECT_GE (emptyTextBounds.getX(), 0.0f);
    EXPECT_GE (emptyTextBounds.getY(), 0.0f);
}

TEST_F (TextButtonTest, GetTextBoundsAfterResize)
{
    textButton->setButtonText (kTestButtonText);

    auto originalBounds = textButton->getTextBounds();

    // Resize the button
    textButton->setBounds (0, 0, 200, 60);

    auto newBounds = textButton->getTextBounds();

    // Text bounds should still be valid after resize
    EXPECT_LE (newBounds.getRight(), static_cast<float> (textButton->getWidth()));
    EXPECT_LE (newBounds.getBottom(), static_cast<float> (textButton->getHeight()));
}

TEST_F (TextButtonTest, TextPersistenceAfterBoundsChange)
{
    textButton->setButtonText (kTestButtonText);
    EXPECT_EQ (kTestButtonText, textButton->getButtonText());

    textButton->setBounds (50, 50, 150, 40);
    EXPECT_EQ (kTestButtonText, textButton->getButtonText());
}

TEST_F (TextButtonTest, StringRefConstructorHandling)
{
    // Test that StringRef parameter works correctly
    const char* cString = "C String Text";
    textButton->setButtonText (cString);
    EXPECT_EQ (String (cString), textButton->getButtonText());

    String yupString = "YUP String Text";
    textButton->setButtonText (yupString);
    EXPECT_EQ (yupString, textButton->getButtonText());
}

TEST_F (TextButtonTest, TextButtonWithZeroSize)
{
    textButton->setBounds (0, 0, 0, 0);
    textButton->setButtonText (kTestButtonText);

    EXPECT_EQ (kTestButtonText, textButton->getButtonText());
    EXPECT_EQ (0, textButton->getWidth());
    EXPECT_EQ (0, textButton->getHeight());
}

TEST_F (TextButtonTest, TextButtonWithVerySmallSize)
{
    textButton->setBounds (0, 0, 1, 1);
    textButton->setButtonText (kTestButtonText);

    EXPECT_EQ (kTestButtonText, textButton->getButtonText());
    auto textBounds = textButton->getTextBounds();

    // Text bounds should still be valid even with tiny button
    EXPECT_LE (textBounds.getRight(), 1.0f);
    EXPECT_LE (textBounds.getBottom(), 1.0f);
}
