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
#include <yup_gui_markup/yup_gui_markup.h>

#include <gtest/gtest.h>

using namespace yup;

class FlexBoxComponentTests : public ::testing::Test
{
protected:
    Component first { "first" };
    Component second { "second" };
};

TEST_F (FlexBoxComponentTests, DistributesTheFreeSpaceByFlexGrow)
{
    RowComponent row;
    row.addAndMakeVisible (first);
    row.addAndMakeVisible (second);

    first.setProperty (Identifier ("flexGrow"), 1.0);
    second.setProperty (Identifier ("flexGrow"), 3.0);
    first.setProperty (Identifier ("flexBasis"), 0.0);
    second.setProperty (Identifier ("flexBasis"), 0.0);

    row.setBounds (0, 0, 100, 40);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (0.0f, 0.0f, 25.0f, 40.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (25.0f, 0.0f, 75.0f, 40.0f));
}

TEST_F (FlexBoxComponentTests, LaysColumnsOutTopToBottomWithAGap)
{
    ColumnComponent column;
    column.addAndMakeVisible (first);
    column.addAndMakeVisible (second);

    column.setGap (10.0f);
    first.setProperty (Identifier ("flexGrow"), 1.0);
    second.setProperty (Identifier ("flexGrow"), 1.0);
    first.setProperty (Identifier ("flexBasis"), 0.0);
    second.setProperty (Identifier ("flexBasis"), 0.0);

    column.setBounds (0, 0, 50, 110);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (0.0f, 0.0f, 50.0f, 50.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (0.0f, 60.0f, 50.0f, 50.0f));
}

TEST_F (FlexBoxComponentTests, SkipsInvisibleChildren)
{
    RowComponent row;
    row.addAndMakeVisible (first);
    row.addChildComponent (second);

    first.setProperty (Identifier ("flexGrow"), 1.0);
    first.setProperty (Identifier ("flexBasis"), 0.0);

    row.setBounds (0, 0, 100, 40);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (0.0f, 0.0f, 100.0f, 40.0f));
}

TEST_F (FlexBoxComponentTests, AlignsAnItemOnTheCrossAxis)
{
    RowComponent row;
    row.addAndMakeVisible (first);

    first.setProperty (Identifier ("preferredHeight"), 10.0);
    first.setProperty (Identifier ("alignSelf"), "center");

    row.setBounds (0, 0, 100, 50);

    EXPECT_FLOAT_EQ (first.getY(), 20.0f);
    EXPECT_FLOAT_EQ (first.getHeight(), 10.0f);
}

TEST_F (FlexBoxComponentTests, IsRegisteredUnderTheMarkupNames)
{
    registerBuiltInComponentTypes();
    auto& registry = ComponentTypeRegistry::getInstance();

    for (const auto& name : { "FlexBox", "Row", "Column", "Grid", "Stack" })
    {
        const auto* info = registry.findType (Identifier (name));
        ASSERT_NE (info, nullptr) << name;
        EXPECT_NE (info->createInstance(), nullptr) << name;
    }
}

TEST_F (FlexBoxComponentTests, ColumnDirectionCanBeOverriddenByStyleSheet)
{
    ColumnComponent column;
    column.addAndMakeVisible (first);
    column.addAndMakeVisible (second);

    first.setProperty (Identifier ("flexGrow"), 1.0);
    second.setProperty (Identifier ("flexGrow"), 1.0);
    first.setProperty (Identifier ("flexBasis"), 0.0);
    second.setProperty (Identifier ("flexBasis"), 0.0);
    column.setBounds (0, 0, 100, 40);

    EXPECT_EQ (column.getDirection(), FlexBox::Direction::column);
    EXPECT_EQ (second.getBounds(), Rectangle<float> (0.0f, 20.0f, 100.0f, 20.0f));

    auto parsed = YuiParser::parseStyleSheet ("style { Column { direction: row; } }");
    ASSERT_TRUE (parsed.wasOk()) << parsed.getErrorMessage();
    column.setStyleSheet (parsed.getReference());
    column.getStyleContext()->restyleNow();

    EXPECT_EQ (column.getDirection(), FlexBox::Direction::row);
    EXPECT_EQ (first.getBounds(), Rectangle<float> (0.0f, 0.0f, 50.0f, 40.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (50.0f, 0.0f, 50.0f, 40.0f));
}
