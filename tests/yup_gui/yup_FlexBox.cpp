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

// Layout results are floats now that performLayout no longer rounds to whole
// pixels, so thirds show up as 116.6667 rather than 117. The tolerance is far
// tighter than a pixel and only absorbs the accumulated division error.
constexpr float flexTolerance = 0.001f;

void expectFlexBounds (const Component& component, float x, float y, float width, float height)
{
    auto bounds = component.getBounds();
    EXPECT_NEAR (x, bounds.getX(), flexTolerance);
    EXPECT_NEAR (y, bounds.getY(), flexTolerance);
    EXPECT_NEAR (width, bounds.getWidth(), flexTolerance);
    EXPECT_NEAR (height, bounds.getHeight(), flexTolerance);
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

    // Free space 100, shared as half a unit at each edge and a full unit
    // between: edges 100 / (2 * 2) = 25, gap 100 / 2 = 50.
    expectFlexBounds (c1, 25, 0, 100, 50);
    expectFlexBounds (c2, 175, 0, 100, 50);
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

    // Single-line containers align against the full container height (100)
    expectFlexBounds (c1, 0, 50, 100, 50);
    expectFlexBounds (c2, 100, 20, 100, 80);
}

TEST (FlexBoxTests, AlignItemsCenterCentersItemsOnTheCrossAxis)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::center;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 80));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Single-line containers center within the full container height (100)
    expectFlexBounds (c1, 0, 25, 100, 50);
    expectFlexBounds (c2, 100, 10, 100, 80);
}

TEST (FlexBoxTests, AlignSelfOverridesTheContainerAlignment)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 100, 80));
    box.items.add (FlexItem (c2, 100, 50).withAlignSelf (FlexItem::AlignSelf::center));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 80);
    expectFlexBounds (c2, 100, 25, 100, 50);
}

// =============================================================================
// single-line stretch (CSS-accurate)
// =============================================================================

TEST (FlexBoxTests, AutoSizedItemsFillTheSingleLineContainer)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1).withWidth (100));
    box.items.add (FlexItem (c2).withWidth (100));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Auto cross sizes (heights) stretch to the full 100 container height
    expectFlexBounds (c1, 0, 0, 100, 100);
    expectFlexBounds (c2, 100, 0, 100, 100);
}

TEST (FlexBoxTests, AutoSizedItemsFillTheSingleLineColumnContainer)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::column;
    box.items.add (FlexItem (c1).withHeight (60));
    box.items.add (FlexItem (c2).withHeight (60));
    box.performLayout (Rectangle<float> (0, 0, 100, 300));

    // Auto cross sizes (widths) stretch to the full 100 container width
    expectFlexBounds (c1, 0, 0, 100, 60);
    expectFlexBounds (c2, 0, 60, 100, 60);
}

/*  F1 and F9 share a test on purpose: a container that stretches everything and
    a container that stretches nothing both "pass" if only one of them is
    checked in isolation, so a regression in either would be masked. The item
    with an explicit height must keep it AND sit at the cross start, while its
    auto-sized sibling fills the container.
*/
TEST (FlexBoxTests, StretchFillsCrossAxisInSingleLineContainer)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::stretch;
    box.items.add (FlexItem (c1, 100, 50)); // definite height: untouched, at cross start
    box.items.add (FlexItem (c2).withWidth (100)); // auto height: fills the container
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 100);
}

TEST (FlexBoxTests, ExplicitSizedItemsAreNotStretched)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 50));         // explicit height stays fixed
    box.items.add (FlexItem (c2).withWidth (100));  // auto height stretches
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 100);
}

TEST (FlexBoxTests, PercentageSizedItemsAreNotStretched)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1).withWidth (100).withHeightPercent (50));
    box.items.add (FlexItem (c2).withWidth (100));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 100);
}

TEST (FlexBoxTests, StretchHonorsMinMaxConstraintsOnAutoItems)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1).withWidth (100).withMinHeight (120));
    box.items.add (FlexItem (c2).withWidth (100).withMaxHeight (60));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Stretch to 100, then clamped to the min/max
    expectFlexBounds (c1, 0, 0, 100, 120);
    expectFlexBounds (c2, 100, 0, 100, 60);
}

TEST (FlexBoxTests, StretchHonorsCrossAxisMargins)
{
    Component c1;

    FlexBox box;
    box.items.add (FlexItem (c1).withWidth (100));

    auto& item = box.items.getReference (0);
    item.marginTop = 10.0f;
    item.marginBottom = 10.0f;

    box.performLayout (Rectangle<float> (0, 0, 200, 100));

    // 100 container height minus the 10px cross margins
    expectFlexBounds (c1, 0, 10, 100, 80);
}

TEST (FlexBoxTests, WrapContainersKeepLinesAtTheItemSize)
{
    Component c1, c2, c3;
    c1.setBounds (0, 0, 70, 30);
    c2.setBounds (0, 0, 70, 30);
    c3.setBounds (0, 0, 70, 30);

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    // The default align-content is stretch, which would grow the single line to
    // fill the container; flex-start is what makes a line hug its items.
    box.alignContent = FlexBox::AlignContent::flexStart;
    box.items.add (FlexItem (c1));
    box.items.add (FlexItem (c2));
    box.items.add (FlexItem (c3));
    box.performLayout (Rectangle<float> (0, 0, 250, 100));

    expectFlexBounds (c1, 0, 0, 70, 30);
    expectFlexBounds (c2, 70, 0, 70, 30);
    expectFlexBounds (c3, 140, 0, 70, 30);
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

    expectFlexBounds (c1, 0, 0, 116.66667f, 50);           // 50 + 200 / 3
    expectFlexBounds (c2, 116.66667f, 0, 183.33333f, 50);  // 50 + 400 / 3
}

TEST (FlexBoxTests, FlexGrowRespectsMaxWidth)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 50, 50).withFlex (1).withMaxWidth (60));
    box.items.add (FlexItem (c2, 50, 50).withFlex (1));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // c1 would grow to 150 but clamps at 60, and the 90px it could not take is
    // redistributed rather than left as a hole.
    expectFlexBounds (c1, 0, 0, 60, 50);
    expectFlexBounds (c2, 60, 0, 240, 50);
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

    // Base main sizes 60 each, extra 180 split equally; the auto cross size
    // (height) stretches to the single-line container's 100
    expectFlexBounds (c1, 0, 0, 150, 100);
    expectFlexBounds (c2, 150, 0, 150, 100);
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

    // Each item would shrink by 40, but c1 freezes at its 70 minimum and the
    // remaining deficit moves onto c2 - so the line fits the container exactly
    // instead of leaving a residual overflow.
    expectFlexBounds (c1, 0, 0, 70, 50);
    expectFlexBounds (c2, 70, 0, 50, 50);
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
    expectFlexBounds (c1, 0, 0, 66.66667f, 50);
    expectFlexBounds (c2, 66.66667f, 0, 33.33333f, 50);
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

TEST (FlexBoxTests, NegativeFlexBasisMeansAuto)
{
    Component c1;
    c1.setBounds (0, 0, 80, 30);

    FlexBox box;
    box.items.add (FlexItem (c1).withFlexBasis (-1.0f));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // Auto falls through to the component's current width
    expectFlexBounds (c1, 0, 0, 80, 100);
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

    // The cross axis is flipped, so the first line ends up at the bottom
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

    // Free space 50 over two lines: half a unit (12.5) at each edge, a full
    // unit (25) between.
    expectFlexBounds (c1, 0, 12.5f, 100, 50);
    expectFlexBounds (c2, 100, 12.5f, 100, 50);
    expectFlexBounds (c3, 0, 87.5f, 100, 50);
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

    // align-content: stretch grows each line by (150-100)/2 = 25 (lines land
    // at y 0 and 75), but the items' definite 50px heights are left untouched
    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 100, 0, 100, 50);
    expectFlexBounds (c3, 0, 75, 100, 50);
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
        { FlexItem::AlignSelf::flexEnd, 50.0f }, // aligned against the full container height (100)
        { FlexItem::AlignSelf::center, 25.0f },  // centered within the full container height (100)
        { FlexItem::AlignSelf::stretch, 0.0f },  // explicit size is kept, aligned to the cross start
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

TEST (FlexBoxTests, UnspecifiedMainAxisSizeUsesComponentCurrentBounds)
{
    Component c1;
    c1.setBounds (0, 0, 80, 30);

    FlexBox box;
    box.items.add (FlexItem (c1));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // The main axis (width) uses the component's bounds (80); the auto cross
    // size (height) stretches to the single-line container's 100
    expectFlexBounds (c1, 0, 0, 80, 100);
}

TEST (FlexBoxTests, ZeroWidthIsAnExplicitZeroNotAuto)
{
    Component c1;
    c1.setBounds (0, 0, 80, 30);

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 0.0f, 0.0f));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // A size of 0 used to fall through to the component's bounds; -1 is the
    // auto sentinel now, so 0 means a genuinely empty item.
    expectFlexBounds (c1, 0, 0, 0, 0);
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

TEST (FlexBoxTests, BaselineUsesTheClampedCrossSize)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::baseline;
    box.items.add (FlexItem (c1, 60, 90).withMaxHeight (50));
    box.items.add (FlexItem (c2, 60, 70));
    box.performLayout (Rectangle<float> (0, 0, 300, 120));

    // c1's default baseline must come from its clamped height (50), not from
    // the unclamped 90 it asked for.
    expectFlexBounds (c1, 0, 20, 60, 50);
    expectFlexBounds (c2, 60, 0, 60, 70);
}

TEST (FlexBoxTests, BaselineDoesNotDoubleCountTheLeadingCrossMargin)
{
    Component c1, c2, c3;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::baseline;
    box.items.add (FlexItem (c1, 60, 40));
    box.items.add (FlexItem (c2, 60, 70));
    box.items.add (FlexItem (c3, 60, 20));

    box.items.getReference (0).marginTop = 15.0f;
    box.items.getReference (2).marginTop = 5.0f;

    box.performLayout (Rectangle<float> (0, 0, 300, 120));

    // A top margin shifts the item's margin box, so the shared baseline stays
    // at 70 and the border boxes land at 70 - height.
    expectFlexBounds (c1, 0, 30, 60, 40);
    expectFlexBounds (c2, 60, 0, 60, 70);
    expectFlexBounds (c3, 120, 50, 60, 20);
}

TEST (FlexBoxTests, BaselineFallsBackToFlexStartInAColumnContainer)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::column;
    box.alignItems = FlexBox::AlignItems::baseline;
    box.items.add (FlexItem (c1, 50, 60));
    box.items.add (FlexItem (c2, 24, 60));
    box.performLayout (Rectangle<float> (0, 0, 120, 300));

    // The cross axis of a column container is the inline axis, where a box with
    // no text has no baseline to share.
    expectFlexBounds (c1, 0, 0, 50, 60);
    expectFlexBounds (c2, 0, 60, 24, 60);
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

TEST (FlexBoxTests, RowReverseKeepsPhysicalMarginSides)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::rowReverse;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40));
    box.items.add (FlexItem (c2, 60, 40));

    auto& first = box.items.getReference (0);
    first.marginLeft = 10.0f;
    first.marginRight = 5.0f;
    first.marginTop = 8.0f;

    auto& second = box.items.getReference (1);
    second.marginRight = 12.0f;
    second.marginTop = 4.0f;

    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // marginLeft must stay a LEFT margin even though the items flow right to
    // left: c1's right edge sits at 300 - marginRight = 295.
    expectFlexBounds (c1, 235, 8, 60, 40);
    expectFlexBounds (c2, 153, 4, 60, 40);
}

TEST (FlexBoxTests, ColumnReverseKeepsPhysicalMarginSides)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::columnReverse;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 40, 60));
    box.items.add (FlexItem (c2, 40, 60));

    auto& first = box.items.getReference (0);
    first.marginLeft = 10.0f;
    first.marginRight = 5.0f;
    first.marginTop = 8.0f;

    auto& second = box.items.getReference (1);
    second.marginRight = 12.0f;
    second.marginTop = 4.0f;

    box.performLayout (Rectangle<float> (0, 0, 100, 300));

    expectFlexBounds (c1, 10, 240, 40, 60);
    expectFlexBounds (c2, 0, 172, 40, 60);
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

TEST (FlexBoxTests, EqualOrderKeepsSourceOrder)
{
    Component c1, c2, c3, c4;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 40, 40).withOrder (2));
    box.items.add (FlexItem (c2, 50, 40).withOrder (1));
    box.items.add (FlexItem (c3, 60, 40).withOrder (1));
    box.items.add (FlexItem (c4, 70, 40).withOrder (0));
    box.performLayout (Rectangle<float> (0, 0, 400, 100));

    // c2 and c3 share an order, so they must keep the order they were added in
    expectFlexBounds (c4, 0, 0, 70, 40);
    expectFlexBounds (c2, 70, 0, 50, 40);
    expectFlexBounds (c3, 120, 0, 60, 40);
    expectFlexBounds (c1, 180, 0, 40, 40);
}

// =============================================================================
// regressions: free space is only handed out once
// =============================================================================

TEST (FlexBoxTests, FlexOneOneZeroSharesSpaceEqually)
{
    Component c1, c2, c3;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;

    for (auto* c : { &c1, &c2, &c3 })
        box.items.add (FlexItem (*c).withHeight (40).withFlex (1).withFlexShrink (1).withFlexBasis (0));

    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 100, 40);
    expectFlexBounds (c2, 100, 0, 100, 40);
    expectFlexBounds (c3, 200, 0, 100, 40);
}

TEST (FlexBoxTests, FlexOneOneZeroIgnoresTheComponentCurrentSize)
{
    Component c1, c2;
    c1.setBounds (0, 0, 200, 10);
    c2.setBounds (0, 0, 10, 10);

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1).withHeight (40).withFlex (1).withFlexBasis (0));
    box.items.add (FlexItem (c2).withHeight (40).withFlex (1).withFlexBasis (0));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // A zero basis means the wildly different starting widths do not matter
    expectFlexBounds (c1, 0, 0, 150, 40);
    expectFlexBounds (c2, 150, 0, 150, 40);
}

TEST (FlexBoxTests, JustifyContentFlexEndDoesNotOverflowWhenItemsGrow)
{
    Component c1, c2;

    FlexBox box;
    box.justifyContent = FlexBox::JustifyContent::flexEnd;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 0.0f, 40.0f).withFlex (1));
    box.items.add (FlexItem (c2, 0.0f, 40.0f).withFlex (1));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // The items already consume the whole line, so there is nothing left for
    // justify-content to shift by.
    expectFlexBounds (c1, 0, 0, 150, 40);
    expectFlexBounds (c2, 150, 0, 150, 40);
}

TEST (FlexBoxTests, JustifyContentSpaceBetweenIsANoOpWhenItemsGrow)
{
    Component c1, c2, c3;

    FlexBox box;
    box.gap = 20.0f;
    box.justifyContent = FlexBox::JustifyContent::spaceBetween;
    box.alignItems = FlexBox::AlignItems::flexStart;

    for (auto* c : { &c1, &c2, &c3 })
        box.items.add (FlexItem (*c, 0.0f, 40.0f).withFlex (1));

    box.performLayout (Rectangle<float> (0, 0, 320, 100));

    // 320 - 40 of gaps = 280 shared three ways
    expectFlexBounds (c1, 0, 0, 93.33333f, 40);
    expectFlexBounds (c2, 113.33333f, 0, 93.33333f, 40);
    expectFlexBounds (c3, 226.66667f, 0, 93.33333f, 40);
}

TEST (FlexBoxTests, FlexGrowClampedByMaxRedistributesToOtherItems)
{
    Component c1, c2, c3;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1).withHeight (40).withFlex (1).withFlexBasis (0).withMaxWidth (40));
    box.items.add (FlexItem (c2).withHeight (40).withFlex (1).withFlexBasis (0).withMaxWidth (60));
    box.items.add (FlexItem (c3).withHeight (40).withFlex (1).withFlexBasis (0));
    box.performLayout (Rectangle<float> (0, 0, 400, 100));

    // Two items freeze at their maxima in successive passes, and everything
    // they could not take lands on the third.
    expectFlexBounds (c1, 0, 0, 40, 40);
    expectFlexBounds (c2, 40, 0, 60, 40);
    expectFlexBounds (c3, 100, 0, 300, 40);
}

TEST (FlexBoxTests, FlexShrinkClampedByMinRedistributesToOtherItems)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 200, 40).withMinWidth (80));
    box.items.add (FlexItem (c2, 200, 40));
    box.performLayout (Rectangle<float> (0, 0, 150, 100));

    // A single proportional pass would give 75 each; c1 freezes at 80 and c2
    // absorbs the rest, so the line still fits exactly.
    expectFlexBounds (c1, 0, 0, 80, 40);
    expectFlexBounds (c2, 80, 0, 70, 40);
}

// =============================================================================
// regressions: line placement on the cross axis
// =============================================================================

TEST (FlexBoxTests, AlignContentSpaceBetweenAccountsForPrecedingLineHeights)
{
    Component c[5];

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::spaceBetween;
    box.alignItems = FlexBox::AlignItems::flexStart;

    for (auto& component : c)
        box.items.add (FlexItem (component, 100, 100));

    box.performLayout (Rectangle<float> (0, 0, 250, 350));

    // Three lines of 100 in 350: the middle line must clear the first one
    // rather than being placed from the container's start.
    expectFlexBounds (c[0], 0, 0, 100, 100);
    expectFlexBounds (c[1], 100, 0, 100, 100);
    expectFlexBounds (c[2], 0, 125, 100, 100);
    expectFlexBounds (c[3], 100, 125, 100, 100);
    expectFlexBounds (c[4], 0, 250, 100, 100);
}

TEST (FlexBoxTests, AlignContentSpaceAroundOffsetsEveryLine)
{
    Component c[5];

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::spaceAround;
    box.alignItems = FlexBox::AlignItems::flexStart;

    for (auto& component : c)
        box.items.add (FlexItem (component, 100, 50));

    box.performLayout (Rectangle<float> (0, 0, 250, 300));

    // Free space 150 over three lines: 25 at each edge, 50 between.
    expectFlexBounds (c[0], 0, 25, 100, 50);
    expectFlexBounds (c[1], 100, 25, 100, 50);
    expectFlexBounds (c[2], 0, 125, 100, 50);
    expectFlexBounds (c[3], 100, 125, 100, 50);
    expectFlexBounds (c[4], 0, 225, 100, 50);
}

TEST (FlexBoxTests, JustifyContentSpaceAroundUsesHalfSpaceAtTheEdges)
{
    Component c1, c2, c3;

    FlexBox box;
    box.justifyContent = FlexBox::JustifyContent::spaceAround;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40));
    box.items.add (FlexItem (c2, 60, 40));
    box.items.add (FlexItem (c3, 60, 40));
    box.performLayout (Rectangle<float> (0, 0, 400, 100));

    // Free space 220: edges 220 / 6, between 220 / 3. space-evenly would put
    // 55 everywhere instead.
    expectFlexBounds (c1, 36.66667f, 0, 60, 40);
    expectFlexBounds (c2, 170, 0, 60, 40);
    expectFlexBounds (c3, 303.33333f, 0, 60, 40);
}

TEST (FlexBoxTests, WrapAccountsForGapsAlreadyOnTheLine)
{
    Component c[5];

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::flexStart;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.gap = 20.0f;

    for (auto& component : c)
        box.items.add (FlexItem (component, 100, 30));

    box.performLayout (Rectangle<float> (0, 0, 250, 250));

    // 100 + 20 + 100 = 220 fits in 250 but a third item would need 340, so the
    // line breaks after two items rather than after three.
    expectFlexBounds (c[0], 0, 0, 100, 30);
    expectFlexBounds (c[1], 120, 0, 100, 30);
    expectFlexBounds (c[2], 0, 50, 100, 30);
    expectFlexBounds (c[3], 120, 50, 100, 30);
    expectFlexBounds (c[4], 0, 100, 100, 30);
}

TEST (FlexBoxTests, AlignContentAppliesToASingleLineWrapContainer)
{
    Component c1, c2;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::flexEnd;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 100, 40));
    box.items.add (FlexItem (c2, 100, 40));
    box.performLayout (Rectangle<float> (0, 0, 300, 200));

    expectFlexBounds (c1, 0, 160, 100, 40);
    expectFlexBounds (c2, 100, 160, 100, 40);
}

TEST (FlexBoxTests, AlignContentIsIgnoredInANoWrapContainer)
{
    Component c1, c2;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::noWrap;
    box.alignContent = FlexBox::AlignContent::flexEnd;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 100, 40));
    box.items.add (FlexItem (c2, 100, 40));
    box.performLayout (Rectangle<float> (0, 0, 300, 200));

    // The distinction from the test above is the wrap mode, not the line count
    expectFlexBounds (c1, 0, 0, 100, 40);
    expectFlexBounds (c2, 100, 0, 100, 40);
}

TEST (FlexBoxTests, WrapReverseMirrorsCrossAxisAlignment)
{
    Component c1, c2, c3, c4;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrapReverse;
    box.alignContent = FlexBox::AlignContent::flexStart;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 100, 60));
    box.items.add (FlexItem (c2, 100, 20));
    box.items.add (FlexItem (c3, 100, 40));
    box.items.add (FlexItem (c4).withWidth (100));
    box.performLayout (Rectangle<float> (0, 0, 250, 200));

    // The cross axis is flipped, so align-items: flex-start puts the short item
    // against the BOTTOM of its line - reversing the line order alone is not
    // enough.
    expectFlexBounds (c1, 0, 140, 100, 60);
    expectFlexBounds (c2, 100, 180, 100, 20);
    expectFlexBounds (c3, 0, 100, 100, 40);
    expectFlexBounds (c4, 100, 140, 100, 0);
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

    // The placeholder still takes up its slot on the main axis
    expectFlexBounds (c1, 0, 0, 100, 50);
    expectFlexBounds (c2, 200, 0, 100, 50);
}

TEST (FlexBoxTests, ZeroSizedTargetAreaProducesZeroSizedItems)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1));
    box.items.add (FlexItem (c2));
    EXPECT_NO_THROW (box.performLayout (Rectangle<float> (0, 0, 0, 0)));

    expectFlexBounds (c1, 0, 0, 0, 0);
    expectFlexBounds (c2, 0, 0, 0, 0);
}

TEST (FlexBoxTests, NegativeTargetAreaDoesNotProduceNegativeSizes)
{
    Component c1;

    FlexBox box;
    box.items.add (FlexItem (c1));
    EXPECT_NO_THROW (box.performLayout (Rectangle<float> (0, 0, -100, -50)));

    EXPECT_GE (c1.getWidth(), 0.0f);
    EXPECT_GE (c1.getHeight(), 0.0f);
}

TEST (FlexBoxTests, OverflowWithoutWrapKeepsItemsAtTheirSize)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 200, 40).withFlexShrink (0));
    box.items.add (FlexItem (c2, 200, 40).withFlexShrink (0));
    box.performLayout (Rectangle<float> (0, 0, 100, 100));

    expectFlexBounds (c1, 0, 0, 200, 40);
    expectFlexBounds (c2, 200, 0, 200, 40);
}

TEST (FlexBoxTests, GapLargerThanTheContainerPutsEveryItemOnItsOwnLine)
{
    Component c1, c2;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::flexStart;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.gap = 400.0f;
    box.items.add (FlexItem (c1, 100, 30));
    box.items.add (FlexItem (c2, 100, 30));
    box.performLayout (Rectangle<float> (0, 0, 250, 400));

    expectFlexBounds (c1, 0, 0, 100, 30);
    expectFlexBounds (c2, 0, 430, 100, 30);
}

TEST (FlexBoxTests, AllItemsWithoutFlexGrowLeaveFreeSpaceAlone)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 50, 40));
    box.items.add (FlexItem (c2, 50, 40));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 50, 40);
    expectFlexBounds (c2, 50, 0, 50, 40);
}

TEST (FlexBoxTests, LayoutIsIdempotent)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.gap = 10.0f;
    box.items.add (FlexItem (c1).withWidth (100).withFlex (1));
    box.items.add (FlexItem (c2, 100, 40));
    box.items.add (FlexItem (c3).withWidth (100));

    const Rectangle<float> area (0, 0, 250, 150);

    box.performLayout (area);

    const auto first1 = c1.getBounds();
    const auto first2 = c2.getBounds();
    const auto first3 = c3.getBounds();

    box.performLayout (area);

    EXPECT_EQ (first1, c1.getBounds());
    EXPECT_EQ (first2, c2.getBounds());
    EXPECT_EQ (first3, c3.getBounds());
}

TEST (FlexBoxTests, LayoutAtOneSizeThenAnotherMatchesAFreshLayout)
{
    auto build = [] (FlexBox& box, Component& a, Component& b)
    {
        box.alignItems = FlexBox::AlignItems::stretch;
        box.items.add (FlexItem (a).withWidth (80).withFlex (1));
        box.items.add (FlexItem (b, 60, 40));
    };

    const Rectangle<float> areaA (0, 0, 400, 200);
    const Rectangle<float> areaB (0, 0, 250, 90);

    Component sequential1, sequential2;
    FlexBox sequential;
    build (sequential, sequential1, sequential2);
    sequential.performLayout (areaA);
    sequential.performLayout (areaB);

    Component fresh1, fresh2;
    FlexBox direct;
    build (direct, fresh1, fresh2);
    direct.performLayout (areaB);

    // Auto sizes read the component's current bounds, so a stale size from the
    // previous pass must not leak into the next one.
    EXPECT_EQ (fresh1.getBounds(), sequential1.getBounds());
    EXPECT_EQ (fresh2.getBounds(), sequential2.getBounds());
}

// =============================================================================
// constructors and integer overload
// =============================================================================

TEST (FlexBoxTests, DirectionConstructorSetsTheDirection)
{
    Component c1, c2;

    FlexBox box (FlexBox::Direction::column);
    box.items.add (FlexItem (c1, 50, 100));
    box.items.add (FlexItem (c2, 50, 100));
    box.performLayout (Rectangle<float> (0, 0, 100, 300));

    expectFlexBounds (c1, 0, 0, 50, 100);
    expectFlexBounds (c2, 0, 100, 50, 100);
}

TEST (FlexBoxTests, FullConstructorSetsEveryProperty)
{
    FlexBox box (FlexBox::Direction::columnReverse,
                 FlexBox::Wrap::wrapReverse,
                 FlexBox::AlignItems::center,
                 FlexBox::JustifyContent::spaceBetween,
                 FlexBox::AlignContent::flexEnd);

    EXPECT_EQ (FlexBox::Direction::columnReverse, box.flexDirection);
    EXPECT_EQ (FlexBox::Wrap::wrapReverse, box.flexWrap);
    EXPECT_EQ (FlexBox::AlignItems::center, box.alignItems);
    EXPECT_EQ (FlexBox::JustifyContent::spaceBetween, box.justifyContent);
    EXPECT_EQ (FlexBox::AlignContent::flexEnd, box.alignContent);
}

TEST (FlexBoxTests, IntegerPerformLayoutMatchesTheFloatOverload)
{
    Component c1, c2;

    FlexBox box;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<int> (10, 20, 300, 100));

    expectFlexBounds (c1, 10, 20, 100, 50);
    expectFlexBounds (c2, 110, 20, 100, 50);
}

TEST (FlexBoxTests, ComponentConvertsImplicitlyToAFlexItem)
{
    Component c1, c2;
    c1.setBounds (0, 0, 40, 20);
    c2.setBounds (0, 0, 60, 20);

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (c1);
    box.items.add (&c2);
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 40, 20);
    expectFlexBounds (c2, 40, 0, 60, 20);
}

TEST (FlexBoxTests, SizeOnlyFlexItemLaysOutWithoutAComponent)
{
    Component c1;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (40.0f, 20.0f)); // no component: reserves space only
    box.items.add (FlexItem (c1, 60, 20));
    EXPECT_NO_THROW (box.performLayout (Rectangle<float> (0, 0, 300, 100)));

    expectFlexBounds (c1, 40, 0, 60, 20);
}

TEST (FlexBoxTests, TargetAreaOriginOffsetsEveryItem)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 100, 50));
    box.items.add (FlexItem (c2, 100, 50));
    box.performLayout (Rectangle<float> (25, 15, 300, 100));

    expectFlexBounds (c1, 25, 15, 100, 50);
    expectFlexBounds (c2, 125, 15, 100, 50);
}

// =============================================================================
// re-entrancy
// =============================================================================

namespace
{

/*  A container that re-runs its own layout from resized().

    Component::setBounds unconditionally fires resized() - there is no dirty
    check - so laying out a component that itself lays out its children means
    performLayout re-enters through setBounds. This is the ordinary shape of a
    nested layout (it is what the Layout example's nested pages do), so it has
    to terminate and settle rather than oscillate.
*/
class SelfLayingOutPanel : public Component
{
public:
    SelfLayingOutPanel()
    {
        addAndMakeVisible (left);
        addAndMakeVisible (right);

        inner.alignItems = FlexBox::AlignItems::stretch;
        inner.items.add (FlexItem (left).withFlex (1).withFlexBasis (0));
        inner.items.add (FlexItem (right).withFlex (1).withFlexBasis (0));
    }

    void resized() override
    {
        ++layoutCount;
        inner.performLayout (getLocalBounds());
    }

    Component left, right;
    FlexBox inner;
    int layoutCount = 0;
};

} // namespace

TEST (FlexBoxTests, NestedLayoutFromResizedTerminatesAndConverges)
{
    SelfLayingOutPanel panel;

    FlexBox outer;
    outer.alignItems = FlexBox::AlignItems::stretch;
    outer.items.add (FlexItem (panel).withFlex (1).withFlexBasis (0));

    const Rectangle<float> area (0, 0, 300, 100);

    outer.performLayout (area);

    const auto firstPanel = panel.getBounds();
    const auto firstLeft = panel.left.getBounds();
    const auto firstRight = panel.right.getBounds();

    EXPECT_GT (panel.layoutCount, 0); // the nested layout really did run
    expectFlexBounds (panel, 0, 0, 300, 100);
    expectFlexBounds (panel.left, 0, 0, 150, 100);
    expectFlexBounds (panel.right, 150, 0, 150, 100);

    // Laying out again must settle on the same answer rather than drift with
    // each re-entrant pass.
    const int countAfterFirst = panel.layoutCount;

    outer.performLayout (area);

    EXPECT_EQ (firstPanel, panel.getBounds());
    EXPECT_EQ (firstLeft, panel.left.getBounds());
    EXPECT_EQ (firstRight, panel.right.getBounds());

    // Each outer pass must trigger a bounded amount of nested work, not an
    // ever-growing cascade.
    EXPECT_LE (panel.layoutCount - countAfterFirst, countAfterFirst + 1);
}

TEST (FlexBoxTests, NestedLayoutSurvivesRepeatedResizes)
{
    SelfLayingOutPanel panel;

    FlexBox outer;
    outer.alignItems = FlexBox::AlignItems::stretch;
    outer.items.add (FlexItem (panel).withFlex (1).withFlexBasis (0));

    for (int i = 0; i < 8; ++i)
        EXPECT_NO_THROW (outer.performLayout (Rectangle<float> (0, 0, 200.0f + i * 10.0f, 80.0f)));

    // Final size 270 wide, split evenly by the two zero-basis children
    expectFlexBounds (panel, 0, 0, 270, 80);
    expectFlexBounds (panel.left, 0, 0, 135, 80);
    expectFlexBounds (panel.right, 135, 0, 135, 80);
}

// =============================================================================
// space-evenly
// =============================================================================

TEST (FlexBoxTests, JustifyContentSpaceEvenlyDistributesEqually)
{
    Component c1, c2, c3;

    FlexBox box;
    box.justifyContent = FlexBox::JustifyContent::spaceEvenly;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40));
    box.items.add (FlexItem (c2, 60, 40));
    box.items.add (FlexItem (c3, 60, 40));
    box.performLayout (Rectangle<float> (0, 0, 400, 100));

    // Free space 220 split four ways - unlike space-around, which puts only
    // half a share at each edge.
    expectFlexBounds (c1, 55, 0, 60, 40);
    expectFlexBounds (c2, 170, 0, 60, 40);
    expectFlexBounds (c3, 285, 0, 60, 40);
}

TEST (FlexBoxTests, AlignContentSpaceEvenlyDistributesLines)
{
    Component c[5];

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.alignContent = FlexBox::AlignContent::spaceEvenly;
    box.alignItems = FlexBox::AlignItems::flexStart;

    for (auto& component : c)
        box.items.add (FlexItem (component, 100, 30));

    box.performLayout (Rectangle<float> (0, 0, 250, 200));

    expectFlexBounds (c[0], 0, 27.5f, 100, 30);
    expectFlexBounds (c[2], 0, 85, 100, 30);
    expectFlexBounds (c[4], 0, 142.5f, 100, 30);
}

// =============================================================================
// independent row/column gaps
// =============================================================================

TEST (FlexBoxTests, RowGapAndColumnGapAreIndependent)
{
    Component c[5];

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.rowGap = 40.0f;
    box.columnGap = 10.0f;
    box.alignContent = FlexBox::AlignContent::flexStart;
    box.alignItems = FlexBox::AlignItems::flexStart;

    for (auto& component : c)
        box.items.add (FlexItem (component, 100, 30));

    box.performLayout (Rectangle<float> (0, 0, 250, 250));

    // In a row container the columns separate items and the rows separate lines
    expectFlexBounds (c[0], 0, 0, 100, 30);
    expectFlexBounds (c[1], 110, 0, 100, 30);
    expectFlexBounds (c[2], 0, 70, 100, 30);
    expectFlexBounds (c[3], 110, 70, 100, 30);
    expectFlexBounds (c[4], 0, 140, 100, 30);
}

TEST (FlexBoxTests, ColumnContainerSwapsTheGapRoles)
{
    Component c[5];

    FlexBox box;
    box.flexDirection = FlexBox::Direction::column;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.rowGap = 10.0f;
    box.columnGap = 40.0f;
    box.alignContent = FlexBox::AlignContent::flexStart;
    box.alignItems = FlexBox::AlignItems::flexStart;

    for (auto& component : c)
        box.items.add (FlexItem (component, 30, 100));

    box.performLayout (Rectangle<float> (0, 0, 250, 250));

    // Now the rows separate items and the columns separate lines
    expectFlexBounds (c[0], 0, 0, 30, 100);
    expectFlexBounds (c[1], 0, 110, 30, 100);
    expectFlexBounds (c[2], 70, 0, 30, 100);
    expectFlexBounds (c[3], 70, 110, 30, 100);
    expectFlexBounds (c[4], 140, 0, 30, 100);
}

TEST (FlexBoxTests, GapShorthandFillsBothUnsetAxes)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.gap = 20.0f;
    box.alignContent = FlexBox::AlignContent::flexStart;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 100, 30));
    box.items.add (FlexItem (c2, 100, 30));
    box.items.add (FlexItem (c3, 100, 30));
    box.performLayout (Rectangle<float> (0, 0, 250, 250));

    expectFlexBounds (c1, 0, 0, 100, 30);
    expectFlexBounds (c2, 120, 0, 100, 30);
    expectFlexBounds (c3, 0, 50, 100, 30);
}

TEST (FlexBoxTests, RowGapOverridesTheGapShorthand)
{
    Component c1, c2, c3;

    FlexBox box;
    box.flexWrap = FlexBox::Wrap::wrap;
    box.gap = 20.0f;
    box.rowGap = 50.0f; // only the line spacing changes
    box.alignContent = FlexBox::AlignContent::flexStart;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 100, 30));
    box.items.add (FlexItem (c2, 100, 30));
    box.items.add (FlexItem (c3, 100, 30));
    box.performLayout (Rectangle<float> (0, 0, 250, 250));

    expectFlexBounds (c1, 0, 0, 100, 30);
    expectFlexBounds (c2, 120, 0, 100, 30);
    expectFlexBounds (c3, 0, 80, 100, 30);
}

// =============================================================================
// container padding
// =============================================================================

TEST (FlexBoxTests, PaddingShrinksTheContentBox)
{
    Component c1, c2;

    FlexBox box;
    box.paddingLeft = 20.0f;
    box.paddingRight = 10.0f;
    box.paddingTop = 5.0f;
    box.paddingBottom = 15.0f;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40));
    box.items.add (FlexItem (c2, 60, 40));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 20, 5, 60, 40);
    expectFlexBounds (c2, 80, 5, 60, 40);
}

TEST (FlexBoxTests, PaddingIsHonouredByStretch)
{
    Component c1, c2;

    FlexBox box;
    box.setPadding (20.0f, 5.0f);
    box.paddingRight = 10.0f;
    box.paddingBottom = 15.0f;
    box.alignItems = FlexBox::AlignItems::stretch;
    box.items.add (FlexItem (c1).withWidth (60));
    box.items.add (FlexItem (c2).withWidth (60));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // 100 - 5 - 15 of vertical padding leaves an 80px content box
    expectFlexBounds (c1, 20, 5, 60, 80);
    expectFlexBounds (c2, 80, 5, 60, 80);
}

TEST (FlexBoxTests, PaddingIsHonouredByReverseDirections)
{
    Component c1, c2;

    FlexBox box;
    box.flexDirection = FlexBox::Direction::rowReverse;
    box.paddingLeft = 20.0f;
    box.paddingRight = 10.0f;
    box.paddingTop = 5.0f;
    box.paddingBottom = 15.0f;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40));
    box.items.add (FlexItem (c2, 60, 40));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // The mirroring happens inside the content box, so the first item's right
    // edge is the padding edge (290) rather than the container edge.
    expectFlexBounds (c1, 230, 5, 60, 40);
    expectFlexBounds (c2, 170, 5, 60, 40);
}

TEST (FlexBoxTests, PaddingLargerThanTheAreaDoesNotInvertIt)
{
    Component c1;

    FlexBox box;
    box.setPadding (200.0f);
    box.items.add (FlexItem (c1).withWidth (60));
    EXPECT_NO_THROW (box.performLayout (Rectangle<float> (0, 0, 300, 100)));

    EXPECT_GE (c1.getWidth(), 0.0f);
    EXPECT_GE (c1.getHeight(), 0.0f);
}

TEST (FlexBoxTests, SetPaddingAssignsAllSides)
{
    FlexBox box;
    box.setPadding (7.0f);

    EXPECT_FLOAT_EQ (7.0f, box.paddingLeft);
    EXPECT_FLOAT_EQ (7.0f, box.paddingRight);
    EXPECT_FLOAT_EQ (7.0f, box.paddingTop);
    EXPECT_FLOAT_EQ (7.0f, box.paddingBottom);

    box.setPadding (3.0f, 9.0f);

    EXPECT_FLOAT_EQ (3.0f, box.paddingLeft);
    EXPECT_FLOAT_EQ (3.0f, box.paddingRight);
    EXPECT_FLOAT_EQ (9.0f, box.paddingTop);
    EXPECT_FLOAT_EQ (9.0f, box.paddingBottom);
}

// =============================================================================
// auto margins
// =============================================================================

TEST (FlexBoxTests, AutoMarginPushesAnItemToTheEnd)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40));
    box.items.add (FlexItem (c2, 60, 40).withAutoMargins (true, false, false, false));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // The classic toolbar idiom: everything left, one item hard right
    expectFlexBounds (c1, 0, 0, 60, 40);
    expectFlexBounds (c2, 240, 0, 60, 40);
}

TEST (FlexBoxTests, AutoMarginsOnBothSidesCenterAnItem)
{
    Component c1;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40).withAutoMargins (true, true, false, false));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 120, 0, 60, 40);
}

TEST (FlexBoxTests, AutoMarginsShareTheFreeSpaceEqually)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40).withAutoMargins (true, false, false, false));
    box.items.add (FlexItem (c2, 60, 40).withAutoMargins (true, false, false, false));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // 180 of free space split between the two auto margins
    expectFlexBounds (c1, 90, 0, 60, 40);
    expectFlexBounds (c2, 240, 0, 60, 40);
}

TEST (FlexBoxTests, AutoMarginBeatsJustifyContent)
{
    Component c1, c2;

    FlexBox box;
    box.justifyContent = FlexBox::JustifyContent::center;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40));
    box.items.add (FlexItem (c2, 60, 40).withAutoMargins (true, false, false, false));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // The auto margin eats the free space first, so centering has nothing left
    expectFlexBounds (c1, 0, 0, 60, 40);
    expectFlexBounds (c2, 240, 0, 60, 40);
}

TEST (FlexBoxTests, AutoMarginResolvesToZeroWithoutFreeSpace)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40).withFlexShrink (0));
    box.items.add (FlexItem (c2, 60, 40).withFlexShrink (0).withAutoMargins (true, false, false, false));
    box.performLayout (Rectangle<float> (0, 0, 120, 100));

    expectFlexBounds (c1, 0, 0, 60, 40);
    expectFlexBounds (c2, 60, 0, 60, 40);
}

TEST (FlexBoxTests, AutoCrossMarginPushesAnItemDown)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40).withAutoMargins (false, false, true, false));
    box.items.add (FlexItem (c2, 60, 40));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 60, 60, 40);
    expectFlexBounds (c2, 60, 0, 60, 40);
}

TEST (FlexBoxTests, AutoCrossMarginsCenterOnTheCrossAxis)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1, 60, 40).withAutoMargins (false, false, true, true));
    box.items.add (FlexItem (c2, 60, 40));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 30, 60, 40);
    expectFlexBounds (c2, 60, 0, 60, 40);
}

TEST (FlexBoxTests, AutoCrossMarginSuppressesStretch)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::stretch;
    box.items.add (FlexItem (c1).withWidth (60).withAutoMargins (false, false, true, false));
    box.items.add (FlexItem (c2).withWidth (60));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    // An item whose margin is going to absorb the leftover cannot also be sized
    // to fill it, so c1 keeps its content height (0) and is pushed to the end.
    expectFlexBounds (c1, 0, 100, 60, 0);
    expectFlexBounds (c2, 60, 0, 60, 100);
}

// =============================================================================
// flex-basis percentage
// =============================================================================

TEST (FlexBoxTests, FlexBasisPercentResolvesAgainstTheMainAxis)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1).withHeight (40).withFlexBasisPercent (25));
    box.items.add (FlexItem (c2).withHeight (40).withFlexBasisPercent (50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 75, 40);
    expectFlexBounds (c2, 75, 0, 150, 40);
}

TEST (FlexBoxTests, FlexBasisPercentParticipatesInFlexGrow)
{
    Component c1, c2;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1).withHeight (40).withFlexBasisPercent (25).withFlex (1));
    box.items.add (FlexItem (c2).withHeight (40).withFlexBasisPercent (25).withFlex (1));
    box.performLayout (Rectangle<float> (0, 0, 400, 100));

    // Bases of 100 each, and the remaining 200 shared equally
    expectFlexBounds (c1, 0, 0, 200, 40);
    expectFlexBounds (c2, 200, 0, 200, 40);
}

TEST (FlexBoxTests, FlexBasisPercentTakesPriorityOverFlexBasis)
{
    Component c1;

    FlexBox box;
    box.alignItems = FlexBox::AlignItems::flexStart;
    box.items.add (FlexItem (c1).withHeight (40).withFlexBasis (10).withFlexBasisPercent (50));
    box.performLayout (Rectangle<float> (0, 0, 300, 100));

    expectFlexBounds (c1, 0, 0, 150, 40);
}
