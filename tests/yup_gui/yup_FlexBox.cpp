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

void expectFlexBounds (const Component& component, float x, float y, float width, float height)
{
    auto bounds = component.getBounds();
    EXPECT_FLOAT_EQ (x, bounds.getX());
    EXPECT_FLOAT_EQ (y, bounds.getY());
    EXPECT_FLOAT_EQ (width, bounds.getWidth());
    EXPECT_FLOAT_EQ (height, bounds.getHeight());
}

} // namespace

// =============================================================================
// Direction
// =============================================================================

TEST (FlexBoxTests, RowPlacesItemsLeftToRight)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 50);
}

TEST (FlexBoxTests, ColumnPlacesItemsTopToBottom)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::column;
    box.items.add (FlexItem (c1, 50, 100));
    box.items.add (FlexItem (c2, 50, 100));
    box.performLayout (Rectangle<float> (0, 0, 100, 300));

    expectFlexBounds (c1, 0, 0, 50, 100);
    expectFlexBounds (c2, 0, 100, 50, 100);
}

TEST (FlexBoxTests, RowReverseMirrorsItemsOnTheMainAxis)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::rowReverse;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 200, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 50);
}

TEST (FlexBoxTests, ColumnReverseMirrorsItemsOnTheMainAxis)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::columnReverse;
    box.items.add (FlexItem (c1, 50, 100));
    box.items.add (FlexItem (c2, 50, 100));
    box.performLayout (Rectangle<float> (0, 0, 100, 300));

    expectFlexBounds (c1, 0, 200, 50, 100);
    expectFlexBounds (c2, 0, 100, 50, 100);
}

// =============================================================================
// justify-content
// =============================================================================

TEST (FlexBoxTests, JustifyContentFlexEndPacksItemsAtTheEnd)
{
    Component c1, c2;

    FlexBox box;
    box.justifyContent = FlexBox::JustifyContent::flexEnd;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 100, 0, 100, 50);
    expectFlexBounds (c2, 200, 0, 100, 50);
}

TEST (FlexBoxTests, JustifyContentCenterCentersItemsOnTheMainAxis)
{
    Component c1, c2;

    FlexBox box;
    box.justifyContent = FlexBox::JustifyContent::center;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 50, 0, 100, 50);
    expectFlexBounds (c2, 150, 0, 100, 50);
}

TEST (FlexBoxTests, JustifyContentSpaceBetweenDistributesSpaceBetweenItems)
{
    Component c1, c2;

    FlexBox box;
    box.justifyContent = FlexBox::JustifyContent::spaceBetween;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 200, 0, 100, 50);
}

TEST (FlexBoxTests, JustifyContentSpaceBetweenWithThreeItems)
{
    Component c1, c2, c3;

    FlexBox box;
    box.justifyContent = FlexBox::JustifyContent::spaceBetween;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.items.add (FlexItem (c3, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 400, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 150, 0, 100, 50);
    expectFlexBounds (c3, 300, 0, 100, 50);
}

TEST (FlexBoxTests, JustifyContentSpaceAroundDistributesSpaceAroundItems)
{
    Component c1, c2;

    FlexBox box;
    box.justifyContent = FlexBox::JustifyContent::spaceAround;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // extra = 100, mainOffset = 100 / 3 per gap
    expectFlexBounds (c1, 33, 0, 100, 50);
    expectFlexBounds (c2, 167, 0, 100, 50);
}

// =============================================================================
// align-items / align-self
// =============================================================================

TEST (FlexBoxTests, AlignItemsFlexStartAlignsItemsToTheCrossStart)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 80));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 80);
}

TEST (FlexBoxTests, AlignItemsFlexEndAlignsItemsToTheCrossEnd)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexEnd;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 80));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 30, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 80);
}

TEST (FlexBoxTests, AlignItemsCenterCentersItemsOnTheCrossAxis)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::center;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 80));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 15, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 80);
}

TEST (FlexBoxTests, AlignSelfOverridesTheContainerAlignment)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::center;
    box.items.add (FlexItem (c1, 100, 80).withAlignSelf (FlexItem::AlignSelf::flexStart));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 80);
    expectFlexBounds (c2, 100, 15, 100, 50);
}

// =============================================================================
// flex-grow
// =============================================================================

TEST (FlexBoxTests, FlexGrowDistributesExtraSpaceEqually)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 50, 50).withFlex (1));
    box.items.add (FlexItem (c2, 50, 50).withFlex (1));
    box.performLayout (Rectangle<float> (0, 0, 200, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 50);
}

TEST (FlexBoxTests, FlexGrowDistributesExtraSpaceProportionally)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 50, 50).withFlex (1));
    box.items.add (FlexItem (c2, 50, 50).withFlex (2));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 117, 50);   // 50 + 200 / 3
    expectFlexBounds (c2, 117, 0, 183, 50); // 50 + 400 / 3
}

TEST (FlexBoxTests, FlexGrowRespectsMaxWidth)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 50, 50).withFlex (1).withMaxWidth (60));
    box.items.add (FlexItem (c2, 50, 50).withFlex (1));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Both would grow to 150, but c1 clamps at its 60 maximum
    expectFlexBounds (c1, 0, 0, 60, 50);
    expectFlexBounds (c2, 60, 0, 150, 50);
}

TEST (FlexBoxTests, FlexGrowInColumnDirectionGrowsTheMainAxisHeight)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::column;
    box.items.add (FlexItem (c1, 50, 50).withFlex (1));
    box.items.add (FlexItem (c2, 50, 50).withFlex (1));
    box.performLayout (Rectangle<float> (0, 0, 100, 300));

    expectFlexBounds (c1, 0, 0, 50, 150);
    expectFlexBounds (c2, 0, 150, 50, 150);
}

TEST (FlexBoxTests, IntrinsicSizeParticipatesInFlexGrow)
{
    Component c1, c2;
    c1.setBounds (0, 0, 60, 50);
    c2.setBounds (0, 0, 60, 50);

    FlexBox box;
    box.items.add (FlexItem (c1).withFlex (1));
    box.items.add (FlexItem (c2).withFlex (1));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Base sizes 60 each, extra 180 split equally
    expectFlexBounds (c1, 0, 0, 150, 50);
    expectFlexBounds (c2, 150, 0, 150, 50);
}

// =============================================================================
// flex-shrink
// =============================================================================

TEST (FlexBoxTests, FlexShrinkReducesSizesWhenLineOverflows)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 80, 50));
    box.items.add (FlexItem (c2, 80, 50));
    box.performLayout (Rectangle<float> (0, 0, 100, 50));

    // Deficit of 60 distributed equally (equal sizes and shrink factors)
    expectFlexBounds (c1, 0, 0, 50, 50);
    expectFlexBounds (c2, 50, 0, 50, 50);
}

TEST (FlexBoxTests, FlexShrinkIsWeightedByShrinkFactorAndSize)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 50).withAlignSelf (FlexItem::AlignSelf::stretch));
    box.items.add (FlexItem (c2, 100, 50).withAlignSelf (FlexItem::AlignSelf::stretch));

    box.items.getReference (0).flexShrink = 1.0f;
    box.items.getReference (1).flexShrink = 3.0f;

    box.performLayout (Rectangle<float> (0, 0, 120, 50));

    // Deficit of 80: c1 shrinks 80*100/400 = 20, c2 shrinks 80*300/400 = 60
    expectFlexBounds (c1, 0, 0, 80, 50);
    expectFlexBounds (c2, 80, 0, 40, 50);
}

TEST (FlexBoxTests, FlexShrinkRespectsMinWidth)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 50).withMinWidth (70));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 120, 50));

    // Without the clamp each item would shrink by 40 (deficit 80 split
    // equally); c1 then clamps at its 70 minimum, leaving the residual
    // overflow (simplified single-pass shrink).
    expectFlexBounds (c1, 0, 0, 70, 50);
    expectFlexBounds (c2, 70, 0, 60, 50);
}

TEST (FlexBoxTests, FlexShrinkUsesFlexBasisAsTheBaseSize)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 200, 50));
    box.items.add (FlexItem (c2, 50, 50));
    box.items.getReference (0).flexBasis = 100.0f;
    box.performLayout (Rectangle<float> (0, 0, 100, 50));

    // Base sizes 100 and 50, deficit 50: c1 shrinks 50*100/150, c2 50*50/150
    expectFlexBounds (c1, 0, 0, 67, 50);
    expectFlexBounds (c2, 67, 0, 33, 50);
}

TEST (FlexBoxTests, FlexShrinkInColumnDirectionShrinksTheMainAxisHeight)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::column;
    box.items.add (FlexItem (c1, 50, 80));
    box.items.add (FlexItem (c2, 50, 80));
    box.performLayout (Rectangle<float> (0, 0, 100, 120));

    expectFlexBounds (c1, 0, 0, 50, 60);
    expectFlexBounds (c2, 0, 60, 50, 60);
}

// =============================================================================
// flex-basis
// =============================================================================

TEST (FlexBoxTests, FlexBasisOverridesTheExplicitWidth)
{
    Component c1;

    FlexBox box;
    box.items.add (FlexItem (c1, 50, 50));
    box.items.getReference (0).flexBasis = 120.0f;
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 120, 50);
}

// =============================================================================
// wrap / wrap-reverse
// =============================================================================

TEST (FlexBoxTests, WrapCreatesMultipleLines)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.items.add (FlexItem (c3, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 250, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 50);
    expectFlexBounds (c3, 0, 50, 100, 50);
}

TEST (FlexBoxTests, WrapReverseReversesLineOrder)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrapReverse;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.items.add (FlexItem (c3, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 250, 100));

    // The second line is laid out first, so the first line ends up at the bottom
    expectFlexBounds (c1, 0, 50, 100, 50);
    expectFlexBounds (c2, 100, 50, 100, 50);
    expectFlexBounds (c3, 0, 0, 100, 50);
}

TEST (FlexBoxTests, WrapTakesTheGapIntoAccount)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.gap = 50.0f;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.items.add (FlexItem (c3, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 250, 150));

    // 100 + gap 50 + 100 = 250 fits, the third item wraps to a new line
    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 150, 0, 100, 50);
    expectFlexBounds (c3, 0, 100, 100, 50);
}

// =============================================================================
// align-content
// =============================================================================

TEST (FlexBoxTests, AlignContentCenterCentersLinesOnTheCrossAxis)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::center;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.items.add (FlexItem (c3, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 250, 150));

    // Two lines of 50 in a 150 tall container: offset (150-100)/2 = 25
    expectFlexBounds (c1, 0, 25, 100, 50);
    expectFlexBounds (c2, 100, 25, 100, 50);
    expectFlexBounds (c3, 0, 75, 100, 50);
}

TEST (FlexBoxTests, AlignContentFlexEndPacksLinesAtTheCrossEnd)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::flexEnd;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.items.add (FlexItem (c3, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 250, 150));

    expectFlexBounds (c1, 0, 50, 100, 50);
    expectFlexBounds (c2, 100, 50, 100, 50);
    expectFlexBounds (c3, 0, 100, 100, 50);
}

TEST (FlexBoxTests, AlignContentSpaceBetweenDistributesLines)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::spaceBetween;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.items.add (FlexItem (c3, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 250, 150));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 50);
    expectFlexBounds (c3, 0, 100, 100, 50);
}

TEST (FlexBoxTests, AlignContentSpaceAroundDistributesLines)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::spaceAround;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.items.add (FlexItem (c3, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 250, 150));

    // offset (150-100)/3 per gap: first line at 16.67, second at 66.67
    expectFlexBounds (c1, 0, 17, 100, 50);
    expectFlexBounds (c2, 100, 17, 100, 50);
    expectFlexBounds (c3, 0, 67, 100, 50);
}

TEST (FlexBoxTests, AlignContentStretchGrowsLinesToFillTheCrossAxis)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::stretch;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.items.add (FlexItem (c3, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 250, 150));

    // Each line grows by (150-100)/2 = 25
    expectFlexBounds (c1, 0, 0, 100, 75);
    expectFlexBounds (c2, 100, 0, 100, 75);
    expectFlexBounds (c3, 0, 75, 100, 75);
}

// =============================================================================
// min/max constraints
// =============================================================================

TEST (FlexBoxTests, MinMaxClampsApplyOnTheMainAxis)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 50, 50).withMinWidth (80));
    box.items.add (FlexItem (c2, 200, 50).withMaxWidth (120));
    box.performLayout (Rectangle<float> (0, 0, 400, 100));

    expectFlexBounds (c1, 0, 0, 80, 50);
    expectFlexBounds (c2, 80, 0, 120, 50);
}

TEST (FlexBoxTests, MinMaxClampsApplyForEveryAlignSelf)
{
    struct AlignSelfCase
    {
        FlexItem::AlignSelf align;
        float expectedY;
    };

    const AlignSelfCase cases[] = {
        { FlexItem::AlignSelf::flexStart, 0.0f },
        { FlexItem::AlignSelf::flexEnd, 10.0f },
        { FlexItem::AlignSelf::center, 5.0f },
        { FlexItem::AlignSelf::stretch, 0.0f },
    };

    for (const auto& testCase : cases)
    {
        Component component;

        FlexBox box;
        box.items.add (FlexItem (component, 100, 60)
                           .withMinWidth (120)
                           .withMaxWidth (140)
                           .withMinHeight (40)
                           .withMaxHeight (50)
                           .withAlignSelf (testCase.align));
        box.performLayout (Rectangle<float> (0, 0, 200, 100));

        // Main axis clamped from 100 to 120, cross axis clamped from 60 to 50
        expectFlexBounds (component, 0, testCase.expectedY, 120, 50);
    }
}

// =============================================================================
// percentage sizing
// =============================================================================

TEST (FlexBoxTests, PercentageSizingResolvesAgainstContainer)
{
    Component c1;

    FlexBox box;
    box.items.add (FlexItem (c1).withWidthPercent (50).withHeightPercent (25));
    box.performLayout (Rectangle<float> (0, 0, 200, 100));

    expectFlexBounds (c1, 0, 0, 100, 25);
}

TEST (FlexBoxTests, PercentageSizingInColumnDirection)
{
    Component c1;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::column;
    box.items.add (FlexItem (c1).withWidthPercent (50).withHeightPercent (50));
    box.performLayout (Rectangle<float> (0, 0, 200, 100));

    // widthPercent resolves against the container width (cross), heightPercent
    // against the container height (main)
    expectFlexBounds (c1, 0, 0, 100, 50);
}

// =============================================================================
// intrinsic / measured sizing
// =============================================================================

TEST (FlexBoxTests, UnspecifiedSizeUsesComponentCurrentBounds)
{
    Component c1;
    c1.setBounds (0, 0, 80, 30);

    FlexBox box;
    box.items.add (FlexItem (c1));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 80, 30);
}

// =============================================================================
// baseline alignment
// =============================================================================

TEST (FlexBoxTests, BaselineAlignsItemsByTheirBaseline)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 50).withAlignSelf (FlexItem::AlignSelf::baseline).withBaseline (20));
    box.items.add (FlexItem (c2, 100, 60).withAlignSelf (FlexItem::AlignSelf::baseline).withBaseline (30));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Both baselines land at y = 30 within the line
    expectFlexBounds (c1, 0, 10, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 60);
}

TEST (FlexBoxTests, ContainerBaselineAlignmentUsesItemHeightAsDefaultBaseline)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::baseline;
    box.items.add (FlexItem (c1, 100, 40));
    box.items.add (FlexItem (c2, 100, 60));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Default baseline = item height: line baseline = 60, c1 top at 60 - 40 = 20
    expectFlexBounds (c1, 0, 20, 100, 40);
    expectFlexBounds (c2, 100, 0, 100, 60);
}

TEST (FlexBoxTests, BaselineAlignedItemsKeepTheirOwnSize)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 30).withAlignSelf (FlexItem::AlignSelf::baseline).withBaseline (25));
    box.items.add (FlexItem (c2, 100, 70).withAlignSelf (FlexItem::AlignSelf::baseline).withBaseline (40));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Line baseline = max(25, 40) = 40; c1 top = 40 - 25 = 15
    expectFlexBounds (c1, 0, 15, 100, 30);
    expectFlexBounds (c2, 100, 0, 100, 70);
}

// =============================================================================
// margins / gap / order
// =============================================================================

TEST (FlexBoxTests, MarginsAreRespectedOnBothAxes)
{
    Component c1;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 50).withMargin (10));
    box.performLayout (Rectangle<float> (0, 0, 320, 120));

    expectFlexBounds (c1, 10, 10, 100, 50);
}

TEST (FlexBoxTests, GapIsAddedBetweenItemsOnTheMainAxis)
{
    Component c1, c2;

    FlexBox box;
    box.gap = 20.0f;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 250, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 120, 0, 100, 50);
}

TEST (FlexBoxTests, OrderSortsItemsBeforeLayout)
{
    Component c1, c2, c3;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 50).withOrder (2));
    box.items.add (FlexItem (c2, 100, 50).withOrder (1));
    box.items.add (FlexItem (c3, 100, 50).withOrder (0));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c3, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 50);
    expectFlexBounds (c1, 200, 0, 100, 50);
}

// =============================================================================
// edge cases
// =============================================================================

TEST (FlexBoxTests, EmptyItemListDoesNothing)
{
    FlexBox box;
    EXPECT_NO_THROW (box.performLayout (Rectangle<float> (0, 0, 100, 100)));
}

TEST (FlexBoxTests, ItemsWithoutComponentAreSkipped)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (nullptr, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 200, 0, 100, 50);
}
