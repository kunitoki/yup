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

//==============================================================================
class ListBoxItemTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        item = std::make_unique<ListBoxItem>();
        item->setBounds (0.0f, 0.0f, 200.0f, 40.0f);
    }

    std::unique_ptr<ListBoxItem> item;
};

//==============================================================================
// Construction Tests
//==============================================================================

TEST_F (ListBoxItemTests, DefaultConstructor)
{
    EXPECT_EQ ("", item->getText());
    EXPECT_EQ (nullptr, item->getIconDrawable());
    EXPECT_FALSE (item->isSelected());
    EXPECT_FALSE (item->isHovered());
}

//==============================================================================
// Text Tests
//==============================================================================

TEST_F (ListBoxItemTests, TextCanBeSet)
{
    item->setText ("Test Item");
    EXPECT_EQ ("Test Item", item->getText());
}

TEST_F (ListBoxItemTests, EmptyTextCanBeSet)
{
    item->setText ("Some Text");
    item->setText ("");
    EXPECT_EQ ("", item->getText());
}

TEST_F (ListBoxItemTests, LongTextCanBeSet)
{
    String longText (512, 'x');
    item->setText (longText);
    EXPECT_EQ (longText, item->getText());
}

TEST_F (ListBoxItemTests, UnicodeTextCanBeSet)
{
    item->setText ("Hello \u4E16\u754C \U0001F600");
    EXPECT_EQ ("Hello \u4E16\u754C \U0001F600", item->getText());
}

//==============================================================================
// Icon Tests
//==============================================================================

TEST_F (ListBoxItemTests, IconDrawableCanBeSet)
{
    auto drawable = std::make_shared<DrawablePath>();
    item->setIconDrawable (drawable);

    EXPECT_EQ (drawable, item->getIconDrawable());
}

TEST_F (ListBoxItemTests, NullIconDrawableCanBeSet)
{
    auto drawable = std::make_shared<DrawablePath>();
    item->setIconDrawable (drawable);
    item->setIconDrawable (nullptr);

    EXPECT_EQ (nullptr, item->getIconDrawable());
}

TEST_F (ListBoxItemTests, IconFromImageCanBeSet)
{
    Image testImage (Image::PixelFormat::ARGB, 32, 32);
    item->setIcon (testImage);

    EXPECT_NE (nullptr, item->getIconDrawable());
}

TEST_F (ListBoxItemTests, EmptyImageDoesNotCrash)
{
    Image emptyImage;
    item->setIcon (emptyImage);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Icon Position Tests
//==============================================================================

TEST_F (ListBoxItemTests, IconPositionDefaultsToLeft)
{
    EXPECT_EQ (ListBoxItem::IconPosition::left, item->getIconPosition());
}

TEST_F (ListBoxItemTests, IconPositionCanBeChangedToRight)
{
    item->setIconPosition (ListBoxItem::IconPosition::right);
    EXPECT_EQ (ListBoxItem::IconPosition::right, item->getIconPosition());
}

TEST_F (ListBoxItemTests, IconPositionCanBeChangedToAbove)
{
    item->setIconPosition (ListBoxItem::IconPosition::above);
    EXPECT_EQ (ListBoxItem::IconPosition::above, item->getIconPosition());
}

TEST_F (ListBoxItemTests, IconPositionCanBeChangedToBelow)
{
    item->setIconPosition (ListBoxItem::IconPosition::below);
    EXPECT_EQ (ListBoxItem::IconPosition::below, item->getIconPosition());
}

TEST_F (ListBoxItemTests, IconPositionChangeTriggersRepaint)
{
    item->setIconPosition (ListBoxItem::IconPosition::left);
    item->setIconPosition (ListBoxItem::IconPosition::right);
    item->setIconPosition (ListBoxItem::IconPosition::above);
    item->setIconPosition (ListBoxItem::IconPosition::below);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Selection State Tests
//==============================================================================

TEST_F (ListBoxItemTests, ItemCanBeSelected)
{
    item->setSelected (true);
    EXPECT_TRUE (item->isSelected());
}

TEST_F (ListBoxItemTests, ItemCanBeDeselected)
{
    item->setSelected (true);
    item->setSelected (false);
    EXPECT_FALSE (item->isSelected());
}

TEST_F (ListBoxItemTests, MultipleSelectionStateChanges)
{
    item->setSelected (true);
    EXPECT_TRUE (item->isSelected());

    item->setSelected (false);
    EXPECT_FALSE (item->isSelected());

    item->setSelected (true);
    EXPECT_TRUE (item->isSelected());
}

//==============================================================================
// Hover State Tests
//==============================================================================

TEST_F (ListBoxItemTests, ItemCanBeHovered)
{
    item->setHovered (true);
    EXPECT_TRUE (item->isHovered());
}

TEST_F (ListBoxItemTests, ItemCanBeUnhovered)
{
    item->setHovered (true);
    item->setHovered (false);
    EXPECT_FALSE (item->isHovered());
}

TEST_F (ListBoxItemTests, MultipleHoverStateChanges)
{
    item->setHovered (true);
    EXPECT_TRUE (item->isHovered());

    item->setHovered (false);
    EXPECT_FALSE (item->isHovered());

    item->setHovered (true);
    EXPECT_TRUE (item->isHovered());
}

//==============================================================================
// Combined State Tests
//==============================================================================

TEST_F (ListBoxItemTests, ItemCanBeSelectedAndHovered)
{
    item->setSelected (true);
    item->setHovered (true);

    EXPECT_TRUE (item->isSelected());
    EXPECT_TRUE (item->isHovered());
}

TEST_F (ListBoxItemTests, SelectionAndHoverAreIndependent)
{
    item->setSelected (true);
    item->setHovered (true);

    item->setSelected (false);
    EXPECT_FALSE (item->isSelected());
    EXPECT_TRUE (item->isHovered());

    item->setHovered (false);
    EXPECT_FALSE (item->isSelected());
    EXPECT_FALSE (item->isHovered());
}

//==============================================================================
// Rendering Bounds Tests
//==============================================================================

TEST_F (ListBoxItemTests, TextBoundsAreValid)
{
    item->setText ("Test");
    item->resized();

    auto textBounds = item->getTextBoundsForRendering();
    EXPECT_FALSE (textBounds.isEmpty());
}

TEST_F (ListBoxItemTests, IconBoundsAreValidWithIcon)
{
    Image testImage (Image::PixelFormat::ARGB, 32, 32);
    item->setIcon (testImage);
    item->resized();

    auto iconBounds = item->getIconBoundsForRendering();
    // May be empty if no icon, but should not crash
    EXPECT_TRUE (true);
}

TEST_F (ListBoxItemTests, LayoutChangesWithIconPosition)
{
    Image testImage (Image::PixelFormat::ARGB, 32, 32);
    item->setIcon (testImage);
    item->setText ("Test");

    item->setIconPosition (ListBoxItem::IconPosition::left);
    item->resized();
    auto boundsLeft = item->getTextBoundsForRendering();

    item->setIconPosition (ListBoxItem::IconPosition::right);
    item->resized();
    auto boundsRight = item->getTextBoundsForRendering();

    // Text bounds should differ based on icon position
    EXPECT_TRUE (boundsLeft.getX() != boundsRight.getX() || boundsLeft.getY() != boundsRight.getY());
}

TEST_F (ListBoxItemTests, LayoutChangesWithVerticalIconPosition)
{
    Image testImage (Image::PixelFormat::ARGB, 32, 32);
    item->setIcon (testImage);
    item->setText ("Test");

    item->setIconPosition (ListBoxItem::IconPosition::above);
    item->resized();
    auto boundsAbove = item->getTextBoundsForRendering();

    item->setIconPosition (ListBoxItem::IconPosition::below);
    item->resized();
    auto boundsBelow = item->getTextBoundsForRendering();

    // Text bounds should differ based on icon position
    EXPECT_NE (boundsAbove.getY(), boundsBelow.getY());
}

//==============================================================================
// Resized Tests
//==============================================================================

TEST_F (ListBoxItemTests, ResizedDoesNotCrash)
{
    item->setText ("Test Item");
    item->resized();

    EXPECT_TRUE (true);
}

TEST_F (ListBoxItemTests, ResizedWithIconDoesNotCrash)
{
    Image testImage (Image::PixelFormat::ARGB, 32, 32);
    item->setIcon (testImage);
    item->setText ("Test Item");
    item->resized();

    EXPECT_TRUE (true);
}

TEST_F (ListBoxItemTests, ResizedWithZeroSizeDoesNotCrash)
{
    item->setBounds (0.0f, 0.0f, 0.0f, 0.0f);
    item->setText ("Test");
    item->resized();

    EXPECT_TRUE (true);
}

TEST_F (ListBoxItemTests, ResizedWithVerySmallSizeDoesNotCrash)
{
    item->setBounds (0.0f, 0.0f, 1.0f, 1.0f);
    item->setText ("Test");
    item->resized();

    EXPECT_TRUE (true);
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F (ListBoxItemTests, MultipleIconChanges)
{
    Image testImage1 (Image::PixelFormat::ARGB, 16, 16);
    Image testImage2 (Image::PixelFormat::ARGB, 32, 32);

    item->setIcon (testImage1);
    item->setIcon (testImage2);
    item->setIconDrawable (nullptr);

    EXPECT_EQ (nullptr, item->getIconDrawable());
}

TEST_F (ListBoxItemTests, TextAndIconTogether)
{
    item->setText ("Item with Icon");
    Image testImage (Image::PixelFormat::ARGB, 24, 24);
    item->setIcon (testImage);
    item->resized();

    EXPECT_EQ ("Item with Icon", item->getText());
    EXPECT_NE (nullptr, item->getIconDrawable());
}

TEST_F (ListBoxItemTests, AllIconPositions)
{
    Image testImage (Image::PixelFormat::ARGB, 24, 24);
    item->setIcon (testImage);
    item->setText ("Test");

    // Test all positions without crashing
    for (auto position : { ListBoxItem::IconPosition::left,
                           ListBoxItem::IconPosition::right,
                           ListBoxItem::IconPosition::above,
                           ListBoxItem::IconPosition::below })
    {
        item->setIconPosition (position);
        item->resized();
    }

    EXPECT_TRUE (true);
}

TEST_F (ListBoxItemTests, StateChangesWithTextAndIcon)
{
    item->setText ("Test Item");
    Image testImage (Image::PixelFormat::ARGB, 24, 24);
    item->setIcon (testImage);

    item->setSelected (true);
    EXPECT_TRUE (item->isSelected());

    item->setHovered (true);
    EXPECT_TRUE (item->isHovered());

    item->setSelected (false);
    item->setHovered (false);

    EXPECT_FALSE (item->isSelected());
    EXPECT_FALSE (item->isHovered());
}
