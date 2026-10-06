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

namespace
{
File getStyledTextTestFontFile()
{
    return
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
}

Font loadStyledTextTestFont (float height = 16.0f)
{
    auto result = Font::loadFontFromFile (getStyledTextTestFontFile());
    EXPECT_TRUE (result.wasOk()); // can't use ASSERT_* here: it returns void, but this function returns Font
    return result.getValue().withHeight (height);
}
} // namespace

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

TEST (StyledTextTests, HorizontalAlignFromJustificationWithoutHorizontalFlagIsLeft)
{
    auto align = StyledText::horizontalAlignFromJustification (Justification::top);

    EXPECT_EQ (StyledText::left, align);
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

// ==============================================================================
// Shaped Text Tests
// ==============================================================================

TEST (StyledTextTests, AppendingTextProducesLinesBoundsAndRenderStyles)
{
    StyledText text;
    auto font = loadStyledTextTestFont();
    const String testText = "Hello shaped text";

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ 400.0f, 200.0f });
        modifier.appendText (testText, font);
    }

    EXPECT_FALSE (text.isEmpty());
    EXPECT_FALSE (text.needsUpdate());
    EXPECT_FALSE (text.getOrderedLines().empty());
    EXPECT_FALSE (text.getRenderStyles().empty());

    const auto bounds = text.getComputedTextBounds();
    EXPECT_GT (bounds.getWidth(), 0.0f);
    EXPECT_GT (bounds.getHeight(), 0.0f);

    EXPECT_TRUE (text.isValidCharacterIndex (0));
    EXPECT_TRUE (text.isValidCharacterIndex (testText.length()));
    EXPECT_FALSE (text.isValidCharacterIndex (testText.length() + 1));

    EXPECT_FALSE (text.getCaretBounds (0).isEmpty());
    EXPECT_FALSE (text.getCaretBounds (testText.length()).isEmpty());
}

TEST (StyledTextTests, CaretBoundsHandleNewlinesEmptyParagraphsAndTrailingNewline)
{
    StyledText text;
    auto font = loadStyledTextTestFont();

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ 400.0f, 400.0f });
        modifier.setWrap (StyledText::wrap);
        modifier.appendText ("first\n\nlast\n", font);
    }

    const auto firstLineNewline = text.getCaretBounds (5);
    const auto emptyParagraphNewline = text.getCaretBounds (6);
    const auto trailingNewline = text.getCaretBounds (12);

    ASSERT_FALSE (firstLineNewline.isEmpty());
    ASSERT_FALSE (emptyParagraphNewline.isEmpty());
    ASSERT_FALSE (trailingNewline.isEmpty());

    EXPECT_GT (emptyParagraphNewline.getY(), firstLineNewline.getY());
    EXPECT_GT (trailingNewline.getY(), emptyParagraphNewline.getY());
}

TEST (StyledTextTests, SelectionRectanglesCoverMultipleVisualLines)
{
    StyledText text;
    auto font = loadStyledTextTestFont();

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ 400.0f, 400.0f });
        modifier.appendText ("one\ntwo\nthree", font);
    }

    const auto rectangles = text.getSelectionRectangles (1, 11);

    ASSERT_GE (rectangles.size(), 3u);
    EXPECT_LT (rectangles[0].getY(), rectangles[1].getY());
    EXPECT_LT (rectangles[1].getY(), rectangles[2].getY());
}

TEST (StyledTextTests, ClearRemovesPreviouslyShapedLineAndRenderData)
{
    StyledText text;
    auto font = loadStyledTextTestFont();

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ 400.0f, 400.0f });
        modifier.appendText ("temporary text", font);
    }

    ASSERT_FALSE (text.getOrderedLines().empty());
    ASSERT_FALSE (text.getRenderStyles().empty());
    ASSERT_GT (text.getComputedTextBounds().getWidth(), 0.0f);

    {
        auto modifier = text.startUpdate();
        modifier.clear();
    }

    EXPECT_TRUE (text.isEmpty());
    EXPECT_TRUE (text.getOrderedLines().empty());
    EXPECT_TRUE (text.getRenderStyles().empty());
    EXPECT_TRUE (text.getComputedTextBounds().isEmpty());
    EXPECT_TRUE (text.isValidCharacterIndex (0));
    EXPECT_FALSE (text.isValidCharacterIndex (1));
}

TEST (StyledTextTests, AdjacentLineMovementClampsAtDocumentEdges)
{
    StyledText text;
    auto font = loadStyledTextTestFont();

    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ 400.0f, 400.0f });
        modifier.appendText ("one\ntwo", font);
    }

    EXPECT_EQ (0, text.getGlyphIndexOnAdjacentLine (0, false));
    EXPECT_EQ (7, text.getGlyphIndexOnAdjacentLine (7, true));
}

// ==============================================================================
// Wrapped-text positioning tests (caret, selection, hit-testing)
// ==============================================================================

namespace
{

// Builds a StyledText with a long single-run string wrapped into several visual
// lines, returning the text and the first character index of the second line.
struct WrappedTextFixture
{
    StyledText text;
    String content;
    int wrapChar = -1;
    int numLines = 0;
};

WrappedTextFixture makeWrappedText (float width,
                                    StyledText::HorizontalAlign align = StyledText::left)
{
    WrappedTextFixture fixture;
    fixture.content = "The quick brown fox jumps over the lazy dog, and keeps running far past the end of this line.";

    auto font = loadStyledTextTestFont();

    {
        auto modifier = fixture.text.startUpdate();
        modifier.setMaxSize ({ width, 400.0f });
        modifier.setWrap (StyledText::wrap);
        modifier.setHorizontalAlign (align);
        modifier.appendText (fixture.content, font);
    }

    const auto& orderedLines = fixture.text.getOrderedLines();
    fixture.numLines = static_cast<int> (orderedLines.size());

    if (fixture.numLines >= 2)
    {
        for (const auto& [run, glyphIndex] : orderedLines[1])
        {
            if (glyphIndex < run->textIndices.size())
            {
                fixture.wrapChar = static_cast<int> (run->textIndices[glyphIndex]);
                break;
            }
        }
    }

    return fixture;
}

} // namespace

TEST (StyledTextTests, WrappedCaretBoundsPlaceAtWrappedLineStart)
{
    const float width = 80.0f;
    auto fixture = makeWrappedText (width);

    ASSERT_GE (fixture.numLines, 2);
    ASSERT_GT (fixture.wrapChar, 0);

    const auto& orderedLines = fixture.text.getOrderedLines();
    const float wrappedLineStartX = orderedLines[1].glyphLine().startX;

    // Caret at the start of the wrapped line must land on that line's left edge,
    // not at the paragraph-relative x position of the previous line.
    const auto caretAtWrap = fixture.text.getCaretBounds (fixture.wrapChar);
    ASSERT_FALSE (caretAtWrap.isEmpty());
    EXPECT_NEAR (wrappedLineStartX, caretAtWrap.getX(), 1.0f);

    // ...and must be vertically below the first line.
    const auto caretAtStart = fixture.text.getCaretBounds (0);
    ASSERT_FALSE (caretAtStart.isEmpty());
    EXPECT_GT (caretAtWrap.getY(), caretAtStart.getY());

    // The caret at the last visible character of the previous line stays at its
    // right edge, which is to the right of the wrapped line's start. (Note: the
    // character right before wrapChar is often the trailing space at the break,
    // which the line breaker drops, so the caret there correctly lands on the
    // next line's start instead.)
    int lastVisibleCharOfLine0 = -1;
    for (const auto& [run, glyphIndex] : orderedLines[0])
    {
        if (glyphIndex < run->textIndices.size())
            lastVisibleCharOfLine0 = jmax (lastVisibleCharOfLine0, static_cast<int> (run->textIndices[glyphIndex]));
    }

    ASSERT_GE (lastVisibleCharOfLine0, 0);

    const auto caretAtPreviousLineEnd = fixture.text.getCaretBounds (lastVisibleCharOfLine0);
    ASSERT_FALSE (caretAtPreviousLineEnd.isEmpty());
    EXPECT_GT (caretAtPreviousLineEnd.getX(), caretAtWrap.getX());
    EXPECT_LE (caretAtPreviousLineEnd.getX(), width);
}

TEST (StyledTextTests, WrappedCaretBoundsRespectCenteredAlignment)
{
    auto fixture = makeWrappedText (80.0f, StyledText::center);

    ASSERT_GE (fixture.numLines, 2);
    ASSERT_GT (fixture.wrapChar, 0);

    const auto& orderedLines = fixture.text.getOrderedLines();
    const float wrappedLineStartX = orderedLines[1].glyphLine().startX;

    // Centered wrapped lines start at a nonzero x; the caret must follow it.
    EXPECT_GT (wrappedLineStartX, 0.0f);

    const auto caretAtWrap = fixture.text.getCaretBounds (fixture.wrapChar);
    ASSERT_FALSE (caretAtWrap.isEmpty());
    EXPECT_NEAR (wrappedLineStartX, caretAtWrap.getX(), 1.0f);
}

TEST (StyledTextTests, WrappedSelectionRectanglesStayWithinLineWidths)
{
    const float width = 80.0f;
    auto fixture = makeWrappedText (width);

    ASSERT_GE (fixture.numLines, 2);
    ASSERT_GT (fixture.wrapChar, 1);

    // Selection spanning the wrap boundary: last chars of line 1 + first chars of line 2.
    const auto rectangles = fixture.text.getSelectionRectangles (fixture.wrapChar - 2, fixture.wrapChar + 2);

    // One rectangle per visual line, no runaway paragraph-relative x offsets.
    ASSERT_EQ (2u, rectangles.size());

    for (const auto& rect : rectangles)
    {
        EXPECT_GE (rect.getX(), 0.0f);
        EXPECT_LE (rect.getRight(), width);
    }

    EXPECT_LT (rectangles[0].getY(), rectangles[1].getY());
    EXPECT_NEAR (0.0f, rectangles[1].getX(), 1.0f);
}

TEST (StyledTextTests, WrappedGlyphIndexAtPositionHitsCorrectLine)
{
    const float width = 80.0f;
    auto fixture = makeWrappedText (width);

    ASSERT_GE (fixture.numLines, 2);
    ASSERT_GT (fixture.wrapChar, 0);

    const auto& orderedLines = fixture.text.getOrderedLines();

    // Click at the left edge of the wrapped line maps to its first character.
    const auto secondLineY = orderedLines[1].y() + (orderedLines[1].glyphLine().top + orderedLines[1].glyphLine().bottom) * 0.5f;
    const int indexOnSecondLine = fixture.text.getGlyphIndexAtPosition ({ 2.0f, secondLineY });
    EXPECT_GE (indexOnSecondLine, fixture.wrapChar);

    // Click at the left edge of the first line stays before the wrap point.
    const auto firstLineY = orderedLines[0].y() + (orderedLines[0].glyphLine().top + orderedLines[0].glyphLine().bottom) * 0.5f;
    const int indexOnFirstLine = fixture.text.getGlyphIndexAtPosition ({ 2.0f, firstLineY });
    EXPECT_LT (indexOnFirstLine, fixture.wrapChar);

    // Position -> caret -> position round-trip on the wrapped line.
    const auto caret = fixture.text.getCaretBounds (fixture.wrapChar);
    ASSERT_FALSE (caret.isEmpty());
    const int roundTripped = fixture.text.getGlyphIndexAtPosition ({ caret.getX() + 1.0f, caret.getCenterY() });
    EXPECT_GE (roundTripped, fixture.wrapChar);
}

TEST (StyledTextTests, AdjacentLineMovementWorksAcrossWraps)
{
    const float width = 80.0f;
    auto fixture = makeWrappedText (width);

    ASSERT_GE (fixture.numLines, 2);
    ASSERT_GT (fixture.wrapChar, 0);

    // Moving down from the first character lands on the wrapped line's first character.
    const int downFromStart = fixture.text.getGlyphIndexOnAdjacentLine (0, true);
    EXPECT_EQ (fixture.wrapChar, downFromStart);

    // Moving up from the wrapped line's first character returns to the start.
    const int upFromWrap = fixture.text.getGlyphIndexOnAdjacentLine (fixture.wrapChar, false);
    EXPECT_EQ (0, upFromWrap);

    // Moving down past the last line clamps to the end of the text.
    const int textEnd = fixture.content.length();
    const int downFromEnd = fixture.text.getGlyphIndexOnAdjacentLine (textEnd, true);
    EXPECT_EQ (textEnd, downFromEnd);
}

// ==============================================================================
// Line breaking (Unicode UAX #14 rules)
// ==============================================================================

class StyledTextLineBreakTests : public ::testing::Test
{
protected:
    static void shape (StyledText& text, const String& string, float maxWidth)
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize ({ maxWidth, 400.0f });
        modifier.appendText (string, loadStyledTextTestFont());
    }

    /** The x position of a character when nothing wraps. */
    static float unwrappedX (const String& string, int characterIndex)
    {
        StyledText text;
        shape (text, string, 10000.0f);
        return text.getCaretBounds (characterIndex).getX();
    }

    static float lineY (const StyledText& text, int characterIndex)
    {
        return text.getCaretBounds (characterIndex).getY();
    }
};

TEST_F (StyledTextLineBreakTests, BreaksAfterAHyphen)
{
    const String string = "well-known";
    const int afterHyphen = string.indexOfChar ('k');

    // Room for "well-kn": splitting the word where it overflows would keep "kn" on the first
    // line, breaking after the hyphen moves all of "known" down
    StyledText text;
    shape (text, string, unwrappedX (string, afterHyphen + 2) + 1.0f);

    // A caret exactly at a wrap can report the end of the previous line, so look one character in
    EXPECT_FLOAT_EQ (lineY (text, 0), lineY (text, afterHyphen - 1));
    EXPECT_GT (lineY (text, afterHyphen + 1), lineY (text, 0));
}

TEST_F (StyledTextLineBreakTests, CjkTextWrapsBetweenCharacters)
{
    // A short word, then six ideographs: "a \u65e5\u672c\u8a9e\u306e\u6587\u7ae0"
    const String string = String::fromUTF8 ("a \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e\xe3\x81\xae\xe6\x96\x87\xe7\xab\xa0");
    const int firstIdeograph = 2;

    const float roomForTwo = unwrappedX (string, firstIdeograph + 2);
    if (roomForTwo <= unwrappedX (string, firstIdeograph))
        GTEST_SKIP() << "The test font gives these characters no width";

    // Room for "a" and two ideographs: breaking only at spaces would move the whole run down,
    // breaking between ideographs keeps the first two on the first line
    StyledText text;
    shape (text, string, roomForTwo + 1.0f);

    EXPECT_FLOAT_EQ (lineY (text, 0), lineY (text, firstIdeograph));
    EXPECT_GT (lineY (text, firstIdeograph + 3), lineY (text, 0));
}

TEST_F (StyledTextLineBreakTests, NewlineAlwaysBreaks)
{
    const String string = "one\ntwo";

    StyledText text;
    shape (text, string, 10000.0f);

    EXPECT_GT (lineY (text, string.indexOfChar ('t')), lineY (text, 0));
}

// ==============================================================================
// Glyph Outline Tests
// ==============================================================================

class StyledTextGlyphOutlineTests : public ::testing::Test
{
protected:
    /** The bounds of the outlines drawn for a string. */
    static Rectangle<float> outlineBounds (const String& string, const Font& font)
    {
        StyledText text;
        {
            auto modifier = text.startUpdate();
            modifier.appendText (string, font);
        }

        const auto styles = text.getRenderStyles();
        if (styles.empty())
            return {};

        const auto& bounds = static_cast<rive::RiveRenderPath*> (styles[0]->path.get())->getBounds();
        return { bounds.left(), bounds.top(), bounds.width(), bounds.height() };
    }
};

TEST_F (StyledTextGlyphOutlineTests, OutlinesFollowVariableFontAxesAcrossManyFonts)
{
    const auto font = loadStyledTextTestFont (32.0f);
    const auto weight = font.getAxisDescription ("wght");
    ASSERT_TRUE (weight.has_value());

    const auto light = outlineBounds ("abc", font.withAxisValue ("wght", weight->minimumValue));
    const auto heavy = outlineBounds ("abc", font.withAxisValue ("wght", weight->maximumValue));
    ASSERT_FALSE (light.isEmpty());
    EXPECT_NE (light, heavy);

    // Every axis change makes a new font: more of them than any outline cache keeps
    for (int i = 0; i < 64; ++i)
        outlineBounds ("abc", font.withAxisValue ("wght", jmap (static_cast<float> (i), 0.0f, 63.0f, weight->minimumValue, weight->maximumValue)));

    EXPECT_EQ (light, outlineBounds ("abc", font.withAxisValue ("wght", weight->minimumValue)));
    EXPECT_EQ (heavy, outlineBounds ("abc", font.withAxisValue ("wght", weight->maximumValue)));
}

// ==============================================================================
// Color Emoji Tests
// ==============================================================================

class StyledTextColorEmojiTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        previousFallback = Font::getColorEmojiFallbackFont();

        auto emojiFont = Font::loadColorEmojiSystemFont();
        if (emojiFont.failed())
            GTEST_SKIP() << "No system color emoji font: " << emojiFont.getErrorMessage();

        Font::setColorEmojiFallbackFont (emojiFont.getValue());
    }

    void TearDown() override
    {
        Font::setColorEmojiFallbackFont (previousFallback);
    }

    static void shape (StyledText& text, const String& string)
    {
        auto modifier = text.startUpdate();
        modifier.appendText (string, Colors::black, loadStyledTextTestFont (32.0f));
    }

    static String grinningFace()
    {
        return String::fromUTF8 ("\xf0\x9f\x98\x80");
    }

    Font previousFallback;
};

TEST_F (StyledTextColorEmojiTests, EmojiMissingFromTheTextFontUsesTheFallbackFont)
{
    StyledText text;
    shape (text, "a" + grinningFace());

    EXPECT_EQ (1, text.getNumColorGlyphs());
}

TEST_F (StyledTextColorEmojiTests, EmojiAddsNoOutline)
{
    StyledText text;
    shape (text, grinningFace());

    EXPECT_EQ (1, text.getNumColorGlyphs());
    EXPECT_TRUE (text.getRenderStyles().empty());
}

TEST_F (StyledTextColorEmojiTests, TextWithoutEmojiHasNoColorGlyphs)
{
    StyledText text;
    shape (text, "abc");

    EXPECT_EQ (0, text.getNumColorGlyphs());
    EXPECT_FALSE (text.getRenderStyles().empty());
}

TEST_F (StyledTextColorEmojiTests, ConsecutiveEmojiKeepTheirOwnCharacterPositions)
{
    const auto partyPopper = String::fromUTF8 ("\xf0\x9f\x8e\x89");

    StyledText text;
    shape (text, "a" + grinningFace() + partyPopper + "b");

    EXPECT_EQ (2, text.getNumColorGlyphs());
    EXPECT_LT (text.getCaretBounds (1).getX(), text.getCaretBounds (2).getX());
    EXPECT_LT (text.getCaretBounds (2).getX(), text.getCaretBounds (3).getX());
}

TEST_F (StyledTextColorEmojiTests, ClearRemovesColorGlyphs)
{
    StyledText text;
    shape (text, grinningFace());

    text.startUpdate().clear();

    EXPECT_EQ (0, text.getNumColorGlyphs());
}

TEST_F (StyledTextColorEmojiTests, ClearedFallbackLeavesEmojiMissing)
{
    Font::setColorEmojiFallbackFont (Font());

    StyledText text;
    shape (text, grinningFace());

    EXPECT_EQ (0, text.getNumColorGlyphs());
}

// ==============================================================================
// Layout Tests
// ==============================================================================

class StyledTextLayoutTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Characters missing from the test font stay missing, whatever emoji font the system has
        previousFallback = Font::getColorEmojiFallbackFont();
        Font::setColorEmojiFallbackFont (Font());
    }

    void TearDown() override
    {
        Font::setColorEmojiFallbackFont (previousFallback);
    }

    static void shape (StyledText& text,
                       const String& string,
                       Size<float> maxSize = { 400.0f, 400.0f },
                       StyledText::TextOverflow overflow = StyledText::visible)
    {
        auto modifier = text.startUpdate();
        modifier.setMaxSize (maxSize);
        modifier.setOverflow (overflow);
        modifier.appendText (string, loadStyledTextTestFont());
    }

    static float lineCenterY (const rive::OrderedLine& line)
    {
        return line.y() + (line.glyphLine().top + line.glyphLine().bottom) * 0.5f;
    }

    static int countGlyphs (const rive::OrderedLine& line)
    {
        int numGlyphs = 0;
        for (const auto& [run, glyphIndex] : line)
        {
            if (glyphIndex < run->glyphs.size())
                ++numGlyphs;
        }

        return numGlyphs;
    }

    /** The COLR test font, where "A" is a color glyph. */
    static Font loadColorGlyphFont()
    {
        auto result = Font::loadFontFromFile (getStyledTextTestFontFile().getSiblingFile ("YupColrTest.ttf"));
        EXPECT_TRUE (result.wasOk()); // can't use ASSERT_* here: it returns void, but this function returns Font
        return result.getValue().withHeight (24.0f);
    }

    Font previousFallback;
};

TEST_F (StyledTextLayoutTests, AppendTextWithARivePaintGroupsGlyphsByPaint)
{
    const auto font = loadStyledTextTestFont();
    const rive::rcp<rive::RenderPaint> firstPaint = rive::make_rcp<rive::RiveRenderPaint>();
    const rive::rcp<rive::RenderPaint> secondPaint = rive::make_rcp<rive::RiveRenderPaint>();

    StyledText text;
    {
        auto modifier = text.startUpdate();
        modifier.appendText ("ab", firstPaint, font);
        modifier.appendText ("cd", secondPaint, font);
        modifier.appendText ("ef", firstPaint, font);
    }

    // Runs drawn with the same paint share one outline
    const auto styles = text.getRenderStyles();
    ASSERT_EQ (2u, styles.size());
    EXPECT_EQ (firstPaint.get(), styles[0]->paint.get());
    EXPECT_EQ (secondPaint.get(), styles[1]->paint.get());
}

TEST_F (StyledTextLayoutTests, EllipsisEndsTheFirstLineWhenNoLineFits)
{
    const String string = "one\ntwo\nthree";

    StyledText full;
    shape (full, string, { 400.0f, 1.0f }, StyledText::visible);

    StyledText truncated;
    shape (truncated, string, { 400.0f, 1.0f }, StyledText::ellipsis);

    ASSERT_EQ (3u, full.getOrderedLines().size());
    ASSERT_EQ (1u, truncated.getOrderedLines().size());

    // The only line laid out is the first one, followed by the ellipsis
    EXPECT_GT (countGlyphs (truncated.getOrderedLines()[0]), countGlyphs (full.getOrderedLines()[0]));
}

TEST_F (StyledTextLayoutTests, CaretPastTheEllipsisStaysAtTheEndOfTheVisibleText)
{
    StyledText text;
    shape (text, "one\ntwo\nthree", { 400.0f, 1.0f }, StyledText::ellipsis);

    ASSERT_EQ (1u, text.getOrderedLines().size());

    const auto endOfVisibleText = text.getCaretBounds (1000);
    ASSERT_FALSE (endOfVisibleText.isEmpty());

    // The newline after "two" ends a line cut away by the ellipsis
    EXPECT_EQ (endOfVisibleText, text.getCaretBounds (7));
    EXPECT_FLOAT_EQ (text.getCaretBounds (0).getY(), endOfVisibleText.getY());
    EXPECT_GT (endOfVisibleText.getX(), text.getCaretBounds (2).getX());
}

TEST_F (StyledTextLayoutTests, AdjacentLineMovementStopsAtTheEllipsisLine)
{
    const String string = "one\ntwo\nthree";

    StyledText text;
    shape (text, string, { 400.0f, 1.0f }, StyledText::ellipsis);

    ASSERT_EQ (1u, text.getOrderedLines().size());

    // The lines past the ellipsis are not laid out, so there is no line to move to
    EXPECT_EQ (string.length(), text.getGlyphIndexOnAdjacentLine (1, true));
    EXPECT_EQ (0, text.getGlyphIndexOnAdjacentLine (1, false));
}

TEST_F (StyledTextLayoutTests, JustifiedLinesStretchToTheWidestLine)
{
    // Narrow enough for the test font to wrap the sentence over several lines
    auto justified = makeWrappedText (40.0f, StyledText::justified);
    ASSERT_GE (justified.numLines, 3);

    const auto widestLine = justified.text.getComputedTextBounds().getWidth();
    const auto justifiedLines = justified.text.getSelectionRectangles (0, justified.content.length());
    ASSERT_EQ (static_cast<std::size_t> (justified.numLines), justifiedLines.size());

    // Every line but the last spreads its glyphs to end at the widest line
    for (std::size_t i = 0; i + 1 < justifiedLines.size(); ++i)
        EXPECT_NEAR (widestLine, justifiedLines[i].getRight(), 0.5f);

    // Left aligned, at least one of those lines ends before
    auto left = makeWrappedText (40.0f, StyledText::left);
    const auto leftLines = left.text.getSelectionRectangles (0, left.content.length());
    ASSERT_EQ (justifiedLines.size(), leftLines.size());

    float shortestLine = widestLine;
    for (std::size_t i = 0; i + 1 < leftLines.size(); ++i)
        shortestLine = jmin (shortestLine, leftLines[i].getRight());

    EXPECT_LT (shortestLine, widestLine - 1.0f);
}

TEST_F (StyledTextLayoutTests, NegativeCaretIndexClampsToTheStart)
{
    StyledText text;
    shape (text, "Hello");

    const auto caret = text.getCaretBounds (-5);

    EXPECT_FALSE (caret.isEmpty());
    EXPECT_EQ (text.getCaretBounds (0), caret);
}

TEST_F (StyledTextLayoutTests, CaretInsideTheLastClusterOfALineIsAtTheLineEnd)
{
    // "abe" and U+0301 COMBINING ACUTE ACCENT: the accent is in the same glyph cluster as the "e"
    StyledText text;
    shape (text, String::fromUTF8 ("abe\xcc\x81\nxy"));

    ASSERT_EQ (2u, text.getOrderedLines().size());

    const auto insideCluster = text.getCaretBounds (3);
    ASSERT_FALSE (insideCluster.isEmpty());

    EXPECT_FLOAT_EQ (text.getCaretBounds (4).getX(), insideCluster.getX());
    EXPECT_FLOAT_EQ (text.getCaretBounds (0).getY(), insideCluster.getY());
    EXPECT_GT (insideCluster.getX(), text.getCaretBounds (2).getX());
}

TEST_F (StyledTextLayoutTests, MovingDownFromAWhitespaceOnlyLineGoesToTheNextLineStart)
{
    StyledText text;
    shape (text, "   \nabc");

    ASSERT_EQ (2u, text.getOrderedLines().size());

    // Below the end of the spaces would be inside "abc": the caret goes to its start instead
    EXPECT_EQ (4, text.getGlyphIndexOnAdjacentLine (3, true));
}

TEST_F (StyledTextLayoutTests, ClickOnAnEmptyLineReturnsItsNewline)
{
    StyledText text;
    shape (text, "one\n\ntwo");

    const auto lines = text.getOrderedLines();
    ASSERT_EQ (3u, lines.size());
    ASSERT_EQ (0, countGlyphs (lines[1]));

    const float emptyLineY = lineCenterY (lines[1]);

    // The second newline, at index 4, is the only character of the empty line
    EXPECT_EQ (4, text.getGlyphIndexAtPosition ({ -5.0f, emptyLineY }));
    EXPECT_EQ (4, text.getGlyphIndexAtPosition ({ 50.0f, emptyLineY }));
}

TEST_F (StyledTextLayoutTests, RepeatedColorGlyphsAreAllDrawnInColor)
{
    const auto font = loadColorGlyphFont();

    StyledText first;
    {
        auto modifier = first.startUpdate();
        modifier.appendText ("AA", font);
    }

    StyledText second;
    {
        auto modifier = second.startUpdate();
        modifier.appendText ("A", font);
    }

    EXPECT_EQ (2, first.getNumColorGlyphs());
    EXPECT_TRUE (first.getRenderStyles().empty());
    EXPECT_EQ (1, second.getNumColorGlyphs());
}
