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

TEST_F (FlexBoxComponentTests, AGrowingItemAddedLaterGetsTheSameShareAsTheOthers)
{
    RowComponent row;
    Component third { "third" };

    for (auto* child : { &first, &second, &third })
        child->setProperty (Identifier ("flexGrow"), 1.0);

    row.setGap (10.0f);
    row.addAndMakeVisible (first);
    row.addAndMakeVisible (second);
    row.setBounds (0, 0, 110, 40);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (0.0f, 0.0f, 50.0f, 40.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (60.0f, 0.0f, 50.0f, 40.0f));

    // The two laid out before don't start from what they were given: each of the three takes a third of what the gaps leave
    row.addAndMakeVisible (third);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (0.0f, 0.0f, 30.0f, 40.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (40.0f, 0.0f, 30.0f, 40.0f));
    EXPECT_EQ (third.getBounds(), Rectangle<float> (80.0f, 0.0f, 30.0f, 40.0f));

    // And one that goes leaves its share to the others
    row.removeChildComponent (&second);

    EXPECT_EQ (first.getBounds(), Rectangle<float> (0.0f, 0.0f, 50.0f, 40.0f));
    EXPECT_EQ (third.getBounds(), Rectangle<float> (60.0f, 0.0f, 50.0f, 40.0f));
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

TEST_F (FlexBoxComponentTests, FitsARowToItsChildren)
{
    RowComponent row;
    row.addAndMakeVisible (first);
    row.addAndMakeVisible (second);

    first.setProperty (Identifier ("preferredWidth"), 30.0);
    first.setProperty (Identifier ("preferredHeight"), 20.0);
    second.setProperty (Identifier ("preferredWidth"), 40.0);
    second.setProperty (Identifier ("preferredHeight"), 10.0);
    second.setProperty (Identifier ("margin"), 2.0);

    row.setGap (5.0f);
    row.setPadding (EdgeSpacing<float> (4.0f));
    row.setFitContent (true);

    EXPECT_FLOAT_EQ (row.findPropertyAs<float> (Identifier ("preferredWidth")), 4.0f + 30.0f + 5.0f + 44.0f + 4.0f);
    EXPECT_FLOAT_EQ (row.findPropertyAs<float> (Identifier ("preferredHeight")), 4.0f + 20.0f + 4.0f);
}

TEST_F (FlexBoxComponentTests, FitsNestedContainersFromTheInnermostOut)
{
    ColumnComponent column;
    RowComponent row;
    column.addAndMakeVisible (row);
    row.addAndMakeVisible (first);
    row.addAndMakeVisible (second);

    for (auto* child : { &first, &second })
    {
        child->setProperty (Identifier ("preferredWidth"), 50.0);
        child->setProperty (Identifier ("preferredHeight"), 30.0);
    }

    column.setPadding (EdgeSpacing<float> (10.0f));
    column.setFitContent (true);
    row.setFitContent (true);

    EXPECT_FLOAT_EQ (column.findPropertyAs<float> (Identifier ("preferredWidth")), 120.0f);
    EXPECT_FLOAT_EQ (column.findPropertyAs<float> (Identifier ("preferredHeight")), 50.0f);

    column.setBounds (0, 0, 120, 50);

    EXPECT_EQ (row.getBounds(), Rectangle<float> (10.0f, 10.0f, 100.0f, 30.0f));
    EXPECT_EQ (second.getBounds(), Rectangle<float> (50.0f, 0.0f, 50.0f, 30.0f));
}

TEST_F (FlexBoxComponentTests, FitsAgainWhenAChildIsHidden)
{
    RowComponent row;
    row.addAndMakeVisible (first);
    row.addAndMakeVisible (second);

    first.setProperty (Identifier ("preferredWidth"), 30.0);
    second.setProperty (Identifier ("preferredWidth"), 40.0);
    row.setFitContent (true);

    second.setVisible (false);

    EXPECT_FLOAT_EQ (row.findPropertyAs<float> (Identifier ("preferredWidth")), 30.0f);
}

TEST_F (FlexBoxComponentTests, FitsNestedContainersBuiltInABatch)
{
    RowComponent outer;
    ColumnComponent inner;
    Component third { "third" };

    {
        const LayoutComponent::ScopedLayoutBatch batch;

        outer.setFitContent (true);
        inner.setFitContent (true);
        outer.addAndMakeVisible (first);
        outer.addAndMakeVisible (inner);
        inner.addAndMakeVisible (second);
        inner.addAndMakeVisible (third);

        for (auto* child : { &first, &second, &third })
        {
            child->setProperty (Identifier ("preferredWidth"), 40.0);
            child->setProperty (Identifier ("preferredHeight"), 30.0);
        }
    }

    EXPECT_FLOAT_EQ (outer.findPropertyAs<float> (Identifier ("preferredWidth")), 80.0f);
    EXPECT_FLOAT_EQ (outer.findPropertyAs<float> (Identifier ("preferredHeight")), 60.0f);

    outer.setBounds (0, 0, 80, 60);

    EXPECT_EQ (inner.getBounds(), Rectangle<float> (40.0f, 0.0f, 40.0f, 60.0f));
    EXPECT_EQ (third.getBounds(), Rectangle<float> (0.0f, 30.0f, 40.0f, 30.0f));
}

TEST_F (FlexBoxComponentTests, FitsOnlyTheAxesItIsAskedTo)
{
    RowComponent row;
    row.addAndMakeVisible (first);
    row.addAndMakeVisible (second);

    first.setProperty (Identifier ("preferredWidth"), 30.0);
    first.setProperty (Identifier ("preferredHeight"), 20.0);
    second.setProperty (Identifier ("preferredWidth"), 40.0);
    second.setProperty (Identifier ("preferredHeight"), 10.0);

    row.setProperty (Identifier ("preferredWidth"), 100.0);
    row.setFitAxes (false, true);
    row.setFitContent (true);

    EXPECT_FLOAT_EQ (row.findPropertyAs<float> (Identifier ("preferredWidth")), 100.0f);
    EXPECT_FLOAT_EQ (row.findPropertyAs<float> (Identifier ("preferredHeight")), 20.0f);

    // Its width stays when the children it would fit to are hidden
    first.setVisible (false);
    second.setVisible (false);

    EXPECT_FLOAT_EQ (row.findPropertyAs<float> (Identifier ("preferredWidth")), 100.0f);
}
