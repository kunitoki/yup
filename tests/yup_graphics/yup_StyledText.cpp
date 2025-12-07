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

#include <gtest/gtest.h>

#include <yup_graphics/yup_graphics.h>

using namespace yup;

// ==============================================================================
// Default Constructor and State Tests
// ==============================================================================

TEST (StyledTextTests, DefaultConstructorCreatesEmptyText)
{
    StyledText text;

    EXPECT_TRUE (text.isEmpty());
    EXPECT_FALSE (text.needsUpdate());
}

TEST (StyledTextTests, DefaultOverflowIsVisible)
{
    StyledText text;

    EXPECT_EQ (StyledText::visible, text.getOverflow());
}

TEST (StyledTextTests, DefaultHorizontalAlignIsLeft)
{
    StyledText text;

    EXPECT_EQ (StyledText::left, text.getHorizontalAlign());
}

TEST (StyledTextTests, DefaultVerticalAlignIsTop)
{
    StyledText text;

    EXPECT_EQ (StyledText::top, text.getVerticalAlign());
}

TEST (StyledTextTests, DefaultMaxSizeIsUnlimited)
{
    StyledText text;
    Size<float> maxSize = text.getMaxSize();

    EXPECT_FLOAT_EQ (-1.0f, maxSize.getWidth());
    EXPECT_FLOAT_EQ (-1.0f, maxSize.getHeight());
}

TEST (StyledTextTests, DefaultParagraphSpacingIsZero)
{
    StyledText text;

    EXPECT_FLOAT_EQ (0.0f, text.getParagraphSpacing());
}

TEST (StyledTextTests, DefaultWrapIsWrap)
{
    StyledText text;

    EXPECT_EQ (StyledText::wrap, text.getWrap());
}

TEST (StyledTextTests, DefaultComputedBoundsIsEmpty)
{
    StyledText text;
    Rectangle<float> bounds = text.getComputedTextBounds();

    EXPECT_FLOAT_EQ (0.0f, bounds.getWidth());
    EXPECT_FLOAT_EQ (0.0f, bounds.getHeight());
}

// ==============================================================================
// Overflow Tests
// ==============================================================================

TEST (StyledTextTests, SetOverflowToEllipsis)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setOverflow (StyledText::ellipsis);
    }

    EXPECT_EQ (StyledText::ellipsis, text.getOverflow());
}

TEST (StyledTextTests, SetOverflowToVisible)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setOverflow (StyledText::visible);
    }

    EXPECT_EQ (StyledText::visible, text.getOverflow());
}

TEST (StyledTextTests, SetOverflowMultipleTimes)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setOverflow (StyledText::ellipsis);
    }

    EXPECT_EQ (StyledText::ellipsis, text.getOverflow());

    {
        auto modifier = text.startUpdate();
        modifier.setOverflow (StyledText::visible);
    }

    EXPECT_EQ (StyledText::visible, text.getOverflow());
}

// ==============================================================================
// Max Size Tests
// ==============================================================================

TEST (StyledTextTests, SetMaxSize)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (Size<float> (200.0f, 100.0f));
    }

    Size<float> maxSize = text.getMaxSize();
    EXPECT_FLOAT_EQ (200.0f, maxSize.getWidth());
    EXPECT_FLOAT_EQ (100.0f, maxSize.getHeight());
}

TEST (StyledTextTests, SetMaxSizeToZero)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (Size<float> (0.0f, 0.0f));
    }

    Size<float> maxSize = text.getMaxSize();
    EXPECT_FLOAT_EQ (0.0f, maxSize.getWidth());
    EXPECT_FLOAT_EQ (0.0f, maxSize.getHeight());
}

TEST (StyledTextTests, SetMaxSizeToLargeValues)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (Size<float> (10000.0f, 5000.0f));
    }

    Size<float> maxSize = text.getMaxSize();
    EXPECT_FLOAT_EQ (10000.0f, maxSize.getWidth());
    EXPECT_FLOAT_EQ (5000.0f, maxSize.getHeight());
}

// ==============================================================================
// Paragraph Spacing Tests
// ==============================================================================

TEST (StyledTextTests, SetParagraphSpacing)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (10.0f);
    }

    EXPECT_FLOAT_EQ (10.0f, text.getParagraphSpacing());
}

TEST (StyledTextTests, SetParagraphSpacingToZero)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (0.0f);
    }

    EXPECT_FLOAT_EQ (0.0f, text.getParagraphSpacing());
}

TEST (StyledTextTests, SetParagraphSpacingToNegativeValue)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (-5.0f);
    }

    EXPECT_FLOAT_EQ (-5.0f, text.getParagraphSpacing());
}

TEST (StyledTextTests, SetParagraphSpacingMultipleTimes)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (10.0f);
    }

    EXPECT_FLOAT_EQ (10.0f, text.getParagraphSpacing());

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (20.0f);
    }

    EXPECT_FLOAT_EQ (20.0f, text.getParagraphSpacing());
}

// ==============================================================================
// Wrap Tests
// ==============================================================================

TEST (StyledTextTests, SetWrapToNoWrap)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setWrap (StyledText::noWrap);
    }

    EXPECT_EQ (StyledText::noWrap, text.getWrap());
}

TEST (StyledTextTests, SetWrapToWrap)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setWrap (StyledText::wrap);
    }

    EXPECT_EQ (StyledText::wrap, text.getWrap());
}

TEST (StyledTextTests, SetWrapMultipleTimes)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setWrap (StyledText::noWrap);
    }

    EXPECT_EQ (StyledText::noWrap, text.getWrap());

    {
        auto modifier = text.startUpdate();
        modifier.setWrap (StyledText::wrap);
    }

    EXPECT_EQ (StyledText::wrap, text.getWrap());
}

// ==============================================================================
// Justification Conversion Tests
// ==============================================================================

TEST (StyledTextTests, HorizontalAlignFromJustificationLeft)
{
    auto align = StyledText::horizontalAlignFromJustification (Justification::left);

    EXPECT_EQ (StyledText::left, align);
}

TEST (StyledTextTests, HorizontalAlignFromJustificationCenter)
{
    auto align = StyledText::horizontalAlignFromJustification (Justification::horizontalCenter);

    EXPECT_EQ (StyledText::center, align);
}

TEST (StyledTextTests, HorizontalAlignFromJustificationRight)
{
    auto align = StyledText::horizontalAlignFromJustification (Justification::right);

    EXPECT_EQ (StyledText::right, align);
}

TEST (StyledTextTests, HorizontalAlignFromJustificationCentered)
{
    auto align = StyledText::horizontalAlignFromJustification (Justification::center);

    EXPECT_EQ (StyledText::center, align);
}

TEST (StyledTextTests, HorizontalAlignFromJustificationCenteredLeft)
{
    auto align = StyledText::horizontalAlignFromJustification (Justification::centerLeft);

    EXPECT_EQ (StyledText::left, align);
}

TEST (StyledTextTests, HorizontalAlignFromJustificationCenteredRight)
{
    auto align = StyledText::horizontalAlignFromJustification (Justification::centerRight);

    EXPECT_EQ (StyledText::right, align);
}

TEST (StyledTextTests, VerticalAlignFromJustificationTop)
{
    auto align = StyledText::verticalAlignFromJustification (Justification::top);

    EXPECT_EQ (StyledText::top, align);
}

TEST (StyledTextTests, VerticalAlignFromJustificationMiddle)
{
    auto align = StyledText::verticalAlignFromJustification (Justification::verticalCenter);

    EXPECT_EQ (StyledText::middle, align);
}

TEST (StyledTextTests, VerticalAlignFromJustificationBottom)
{
    auto align = StyledText::verticalAlignFromJustification (Justification::bottom);

    EXPECT_EQ (StyledText::bottom, align);
}

TEST (StyledTextTests, VerticalAlignFromJustificationCentered)
{
    auto align = StyledText::verticalAlignFromJustification (Justification::center);

    EXPECT_EQ (StyledText::middle, align);
}

TEST (StyledTextTests, VerticalAlignFromJustificationCenteredTop)
{
    auto align = StyledText::verticalAlignFromJustification (Justification::centerTop);

    EXPECT_EQ (StyledText::top, align);
}

TEST (StyledTextTests, VerticalAlignFromJustificationCenteredBottom)
{
    auto align = StyledText::verticalAlignFromJustification (Justification::centerBottom);

    EXPECT_EQ (StyledText::bottom, align);
}

// ==============================================================================
// Empty Text State Tests
// ==============================================================================

TEST (StyledTextTests, GetGlyphIndexAtPositionReturnsZeroForEmptyText)
{
    StyledText text;

    int index = text.getGlyphIndexAtPosition (Point<float> (10.0f, 10.0f));

    EXPECT_EQ (0, index);
}

TEST (StyledTextTests, GetCaretBoundsReturnsEmptyForEmptyText)
{
    StyledText text;

    Rectangle<float> bounds = text.getCaretBounds (0);

    EXPECT_FLOAT_EQ (0.0f, bounds.getWidth());
    EXPECT_FLOAT_EQ (0.0f, bounds.getHeight());
}

TEST (StyledTextTests, GetSelectionRectanglesReturnsEmptyForEmptyText)
{
    StyledText text;

    auto rectangles = text.getSelectionRectangles (0, 5);

    EXPECT_TRUE (rectangles.empty());
}

TEST (StyledTextTests, GetSelectionRectanglesReturnsEmptyForInvalidRange)
{
    StyledText text;

    auto rectangles = text.getSelectionRectangles (5, 0);

    EXPECT_TRUE (rectangles.empty());
}

TEST (StyledTextTests, GetSelectionRectanglesReturnsEmptyForNegativeIndices)
{
    StyledText text;

    auto rectangles = text.getSelectionRectangles (-1, -5);

    EXPECT_TRUE (rectangles.empty());
}

TEST (StyledTextTests, GetSelectionRectanglesReturnsEmptyForEqualIndices)
{
    StyledText text;

    auto rectangles = text.getSelectionRectangles (5, 5);

    EXPECT_TRUE (rectangles.empty());
}

TEST (StyledTextTests, GetOrderedLinesReturnsEmptyForEmptyText)
{
    StyledText text;

    auto lines = text.getOrderedLines();

    EXPECT_TRUE (lines.empty());
}

TEST (StyledTextTests, GetRenderStylesReturnsEmptyForEmptyText)
{
    StyledText text;

    auto styles = text.getRenderStyles();

    EXPECT_TRUE (styles.empty());
}

TEST (StyledTextTests, IsValidCharacterIndexReturnsTrueForZeroOnEmptyText)
{
    StyledText text;

    EXPECT_TRUE (text.isValidCharacterIndex (0));
}

TEST (StyledTextTests, IsValidCharacterIndexReturnsFalseForNegativeIndex)
{
    StyledText text;

    EXPECT_FALSE (text.isValidCharacterIndex (-1));
}

TEST (StyledTextTests, IsValidCharacterIndexReturnsFalseForLargeIndex)
{
    StyledText text;

    EXPECT_FALSE (text.isValidCharacterIndex (1000));
}

// ==============================================================================
// Offset Tests
// ==============================================================================

TEST (StyledTextTests, GetOffsetWithLeftTopAlignment)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::left);
        modifier.setVerticalAlign (StyledText::top);
    }

    Point<float> offset = text.getOffset (Rectangle<float> (0.0f, 0.0f, 200.0f, 100.0f));

    EXPECT_FLOAT_EQ (0.0f, offset.getX());
    EXPECT_FLOAT_EQ (0.0f, offset.getY());
}

TEST (StyledTextTests, GetOffsetWithCenterMiddleAlignment)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::center);
        modifier.setVerticalAlign (StyledText::middle);
    }

    Rectangle<float> area (0.0f, 0.0f, 200.0f, 100.0f);
    Point<float> offset = text.getOffset (area);

    // Empty text has 0 bounds, so offset should center the empty bounds
    EXPECT_FLOAT_EQ (100.0f, offset.getX()); // (200 - 0) * 0.5
    EXPECT_FLOAT_EQ (50.0f, offset.getY());  // (100 - 0) * 0.5
}

TEST (StyledTextTests, GetOffsetWithRightBottomAlignment)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::right);
        modifier.setVerticalAlign (StyledText::bottom);
    }

    Rectangle<float> area (0.0f, 0.0f, 200.0f, 100.0f);
    Point<float> offset = text.getOffset (area);

    // Empty text has 0 bounds
    EXPECT_FLOAT_EQ (200.0f, offset.getX()); // 200 - 0
    EXPECT_FLOAT_EQ (100.0f, offset.getY()); // 100 - 0
}

TEST (StyledTextTests, GetOffsetWithJustifiedAlignment)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::justified);
        modifier.setVerticalAlign (StyledText::middle);
    }

    Rectangle<float> area (0.0f, 0.0f, 200.0f, 100.0f);
    Point<float> offset = text.getOffset (area);

    // Justified is treated as left for horizontal alignment
    EXPECT_FLOAT_EQ (0.0f, offset.getX());
    EXPECT_FLOAT_EQ (50.0f, offset.getY());
}

TEST (StyledTextTests, GetOffsetWithZeroArea)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::center);
        modifier.setVerticalAlign (StyledText::middle);
    }

    Point<float> offset = text.getOffset (Rectangle<float> (0.0f, 0.0f, 0.0f, 0.0f));

    EXPECT_FLOAT_EQ (0.0f, offset.getX());
    EXPECT_FLOAT_EQ (0.0f, offset.getY());
}

TEST (StyledTextTests, GetOffsetWithLargeArea)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::center);
        modifier.setVerticalAlign (StyledText::middle);
    }

    Rectangle<float> area (0.0f, 0.0f, 10000.0f, 5000.0f);
    Point<float> offset = text.getOffset (area);

    EXPECT_FLOAT_EQ (5000.0f, offset.getX());
    EXPECT_FLOAT_EQ (2500.0f, offset.getY());
}

// ==============================================================================
// TextModifier Tests
// ==============================================================================

TEST (StyledTextTests, TextModifierClearMakesTextEmpty)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.clear();
    }

    EXPECT_TRUE (text.isEmpty());
}

TEST (StyledTextTests, TextModifierMultiplePropertiesInSingleUpdate)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setOverflow (StyledText::ellipsis);
        modifier.setHorizontalAlign (StyledText::center);
        modifier.setVerticalAlign (StyledText::middle);
        modifier.setMaxSize (Size<float> (300.0f, 200.0f));
        modifier.setParagraphSpacing (15.0f);
        modifier.setWrap (StyledText::noWrap);
    }

    EXPECT_EQ (StyledText::ellipsis, text.getOverflow());
    EXPECT_EQ (StyledText::center, text.getHorizontalAlign());
    EXPECT_EQ (StyledText::middle, text.getVerticalAlign());
    EXPECT_EQ (Size<float> (300.0f, 200.0f), text.getMaxSize());
    EXPECT_FLOAT_EQ (15.0f, text.getParagraphSpacing());
    EXPECT_EQ (StyledText::noWrap, text.getWrap());
}

TEST (StyledTextTests, TextModifierDestructorTriggersUpdate)
{
    StyledText text;

    EXPECT_FALSE (text.needsUpdate());

    {
        auto modifier = text.startUpdate();
        modifier.setOverflow (StyledText::ellipsis);
        // Update happens when modifier goes out of scope
    }

    // After modifier destruction, update should have been called
    EXPECT_FALSE (text.needsUpdate());
}

TEST (StyledTextTests, MultipleTextModifierScopes)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (10.0f);
    }

    EXPECT_FLOAT_EQ (10.0f, text.getParagraphSpacing());

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (20.0f);
    }

    EXPECT_FLOAT_EQ (20.0f, text.getParagraphSpacing());
}

// ==============================================================================
// Combined Property Tests
// ==============================================================================

TEST (StyledTextTests, SetAllPropertiesSequentially)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setOverflow (StyledText::ellipsis);
    }

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::center);
    }

    {
        auto modifier = text.startUpdate();
        modifier.setVerticalAlign (StyledText::bottom);
    }

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (Size<float> (400.0f, 300.0f));
    }

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (25.0f);
    }

    {
        auto modifier = text.startUpdate();
        modifier.setWrap (StyledText::noWrap);
    }

    EXPECT_EQ (StyledText::ellipsis, text.getOverflow());
    EXPECT_EQ (StyledText::center, text.getHorizontalAlign());
    EXPECT_EQ (StyledText::bottom, text.getVerticalAlign());
    EXPECT_EQ (Size<float> (400.0f, 300.0f), text.getMaxSize());
    EXPECT_FLOAT_EQ (25.0f, text.getParagraphSpacing());
    EXPECT_EQ (StyledText::noWrap, text.getWrap());
}

TEST (StyledTextTests, PropertyChangesDoNotAffectOtherProperties)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setOverflow (StyledText::ellipsis);
        modifier.setParagraphSpacing (10.0f);
    }

    EXPECT_EQ (StyledText::ellipsis, text.getOverflow());
    EXPECT_FLOAT_EQ (10.0f, text.getParagraphSpacing());

    {
        auto modifier = text.startUpdate();
        modifier.setOverflow (StyledText::visible);
    }

    EXPECT_EQ (StyledText::visible, text.getOverflow());
    EXPECT_FLOAT_EQ (10.0f, text.getParagraphSpacing()); // Should remain unchanged
}

// ==============================================================================
// Edge Cases
// ==============================================================================

TEST (StyledTextTests, GetCaretBoundsWithNegativeIndex)
{
    StyledText text;

    Rectangle<float> bounds = text.getCaretBounds (-1);

    // Should handle gracefully (likely returns empty or clamped to 0)
    EXPECT_GE (bounds.getX(), 0.0f);
}

TEST (StyledTextTests, GetCaretBoundsWithLargeIndex)
{
    StyledText text;

    Rectangle<float> bounds = text.getCaretBounds (10000);

    // Should handle gracefully
    EXPECT_TRUE (bounds.isEmpty() || bounds.getWidth() >= 0.0f);
}

TEST (StyledTextTests, GetGlyphIndexAtNegativePosition)
{
    StyledText text;

    int index = text.getGlyphIndexAtPosition (Point<float> (-100.0f, -100.0f));

    EXPECT_GE (index, 0);
}

TEST (StyledTextTests, GetGlyphIndexAtVeryLargePosition)
{
    StyledText text;

    int index = text.getGlyphIndexAtPosition (Point<float> (10000.0f, 10000.0f));

    EXPECT_GE (index, 0);
}

TEST (StyledTextTests, GetOffsetWithNegativeArea)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::center);
        modifier.setVerticalAlign (StyledText::middle);
    }

    // Negative dimensions should still compute offset
    Point<float> offset = text.getOffset (Rectangle<float> (0.0f, 0.0f, -100.0f, -50.0f));

    // Implementation should handle this gracefully
    EXPECT_TRUE (std::isfinite (offset.getX()));
    EXPECT_TRUE (std::isfinite (offset.getY()));
}

TEST (StyledTextTests, SetSamePropertyValueMultipleTimes)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (10.0f);
    }

    EXPECT_FLOAT_EQ (10.0f, text.getParagraphSpacing());

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (10.0f); // Same value
    }

    EXPECT_FLOAT_EQ (10.0f, text.getParagraphSpacing());
}

TEST (StyledTextTests, AlternatePropertyValues)
{
    StyledText text;

    for (int i = 0; i < 5; ++i)
    {
        {
            auto modifier = text.startUpdate();
            modifier.setWrap (StyledText::wrap);
        }

        EXPECT_EQ (StyledText::wrap, text.getWrap());

        {
            auto modifier = text.startUpdate();
            modifier.setWrap (StyledText::noWrap);
        }

        EXPECT_EQ (StyledText::noWrap, text.getWrap());
    }
}

// ==============================================================================
// Size Boundary Tests
// ==============================================================================

TEST (StyledTextTests, SetMaxSizeWithVerySmallValues)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (Size<float> (0.001f, 0.001f));
    }

    Size<float> maxSize = text.getMaxSize();
    EXPECT_FLOAT_EQ (0.001f, maxSize.getWidth());
    EXPECT_FLOAT_EQ (0.001f, maxSize.getHeight());
}

TEST (StyledTextTests, SetMaxSizeWithNegativeValues)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (Size<float> (-50.0f, -100.0f));
    }

    Size<float> maxSize = text.getMaxSize();
    EXPECT_FLOAT_EQ (-50.0f, maxSize.getWidth());
    EXPECT_FLOAT_EQ (-100.0f, maxSize.getHeight());
}

TEST (StyledTextTests, SetParagraphSpacingWithVeryLargeValue)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (10000.0f);
    }

    EXPECT_FLOAT_EQ (10000.0f, text.getParagraphSpacing());
}

TEST (StyledTextTests, SetParagraphSpacingWithVerySmallValue)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setParagraphSpacing (0.0001f);
    }

    EXPECT_FLOAT_EQ (0.0001f, text.getParagraphSpacing());
}

// ==============================================================================
// Alignment Combination Tests
// ==============================================================================

TEST (StyledTextTests, AllHorizontalAlignmentOptions)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::left);
    }
    EXPECT_EQ (StyledText::left, text.getHorizontalAlign());

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::center);
    }
    EXPECT_EQ (StyledText::center, text.getHorizontalAlign());

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::right);
    }
    EXPECT_EQ (StyledText::right, text.getHorizontalAlign());

    {
        auto modifier = text.startUpdate();
        modifier.setHorizontalAlign (StyledText::justified);
    }
    EXPECT_EQ (StyledText::justified, text.getHorizontalAlign());
}

TEST (StyledTextTests, AllVerticalAlignmentOptions)
{
    StyledText text;

    {
        auto modifier = text.startUpdate();
        modifier.setVerticalAlign (StyledText::top);
    }
    EXPECT_EQ (StyledText::top, text.getVerticalAlign());

    {
        auto modifier = text.startUpdate();
        modifier.setVerticalAlign (StyledText::middle);
    }
    EXPECT_EQ (StyledText::middle, text.getVerticalAlign());

    {
        auto modifier = text.startUpdate();
        modifier.setVerticalAlign (StyledText::bottom);
    }
    EXPECT_EQ (StyledText::bottom, text.getVerticalAlign());
}

TEST (StyledTextTests, AllAlignmentCombinations)
{
    StyledText text;

    StyledText::HorizontalAlign hAligns[] = {
        StyledText::left, StyledText::center, StyledText::right, StyledText::justified
    };

    StyledText::VerticalAlign vAligns[] = {
        StyledText::top, StyledText::middle, StyledText::bottom
    };

    for (auto hAlign : hAligns)
    {
        for (auto vAlign : vAligns)
        {
            {
                auto modifier = text.startUpdate();
                modifier.setHorizontalAlign (hAlign);
                modifier.setVerticalAlign (vAlign);
            }

            EXPECT_EQ (hAlign, text.getHorizontalAlign());
            EXPECT_EQ (vAlign, text.getVerticalAlign());
        }
    }
}
