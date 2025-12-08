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
// Constructor and Assignment Tests
// ==============================================================================

TEST (FontTests, DefaultConstructorCreatesEmptyFont)
{
    Font font;

    EXPECT_EQ (0.0f, font.getAscent());
    EXPECT_EQ (0.0f, font.getDescent());
    EXPECT_EQ (0, font.getWeight());
    EXPECT_FALSE (font.isItalic());
    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, DefaultConstructorHasDefaultHeight)
{
    Font font;

    EXPECT_EQ (12.0f, font.getHeight());
}

TEST (FontTests, CopyConstructor)
{
    Font font1;
    font1.setHeight (16.0f);

    Font font2 (font1);

    EXPECT_EQ (font1.getHeight(), font2.getHeight());
    EXPECT_EQ (font1, font2);
}

TEST (FontTests, MoveConstructor)
{
    Font font1;
    font1.setHeight (20.0f);

    Font font2 (std::move (font1));

    EXPECT_EQ (20.0f, font2.getHeight());
}

TEST (FontTests, CopyAssignment)
{
    Font font1;
    font1.setHeight (24.0f);

    Font font2;
    font2 = font1;

    EXPECT_EQ (font1.getHeight(), font2.getHeight());
    EXPECT_EQ (font1, font2);
}

TEST (FontTests, MoveAssignment)
{
    Font font1;
    font1.setHeight (18.0f);

    Font font2;
    font2 = std::move (font1);

    EXPECT_EQ (18.0f, font2.getHeight());
}

// ==============================================================================
// Loading Tests
// ==============================================================================

TEST (FontTests, LoadFromDataWithEmptyData)
{
    Font font;
    MemoryBlock emptyData;

    Result result = font.loadFromData (emptyData);

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST (FontTests, DISABLED_LoadFromDataWithInvalidData) // TODO - this doesn't fail harfbuzz!!
{
    Font font;
    MemoryBlock invalidData ("invalid font data", 17);

    Result result = font.loadFromData (invalidData);

    EXPECT_FALSE (result.wasOk());
}

TEST (FontTests, LoadFromNonExistentFile)
{
    Font font;
    File nonExistentFile ("/path/to/nonexistent/font.ttf");

    Result result = font.loadFromFile (nonExistentFile);

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST (FontTests, LoadFromDirectory)
{
    Font font;
    File directory = File::getCurrentWorkingDirectory();

    Result result = font.loadFromFile (directory);

    EXPECT_FALSE (result.wasOk());
}

// ==============================================================================
// Height Tests
// ==============================================================================

TEST (FontTests, GetHeightReturnsDefaultValue)
{
    Font font;

    EXPECT_EQ (12.0f, font.getHeight());
}

TEST (FontTests, SetHeightChangesHeight)
{
    Font font;
    font.setHeight (24.0f);

    EXPECT_EQ (24.0f, font.getHeight());
}

TEST (FontTests, SetHeightWithZero)
{
    Font font;
    font.setHeight (0.0f);

    EXPECT_EQ (0.0f, font.getHeight());
}

TEST (FontTests, SetHeightWithNegativeValue)
{
    Font font;
    font.setHeight (-10.0f);

    EXPECT_EQ (-10.0f, font.getHeight());
}

TEST (FontTests, WithHeightReturnsNewFont)
{
    Font font1;
    font1.setHeight (12.0f);

    Font font2 = font1.withHeight (18.0f);

    EXPECT_EQ (12.0f, font1.getHeight());
    EXPECT_EQ (18.0f, font2.getHeight());
}

TEST (FontTests, WithHeightDoesNotModifyOriginal)
{
    Font font1;
    const float originalHeight = font1.getHeight();

    Font font2 = font1.withHeight (36.0f);

    EXPECT_EQ (originalHeight, font1.getHeight());
    EXPECT_EQ (36.0f, font2.getHeight());
}

// ==============================================================================
// Font Metrics Tests (Empty Font)
// ==============================================================================

TEST (FontTests, EmptyFontHasZeroAscent)
{
    Font font;

    EXPECT_EQ (0.0f, font.getAscent());
}

TEST (FontTests, EmptyFontHasZeroDescent)
{
    Font font;

    EXPECT_EQ (0.0f, font.getDescent());
}

TEST (FontTests, EmptyFontHasZeroWeight)
{
    Font font;

    EXPECT_EQ (0, font.getWeight());
}

TEST (FontTests, EmptyFontIsNotItalic)
{
    Font font;

    EXPECT_FALSE (font.isItalic());
}

// ==============================================================================
// Axis Tests (Empty Font)
// ==============================================================================

TEST (FontTests, EmptyFontHasNoAxis)
{
    Font font;

    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, GetAxisDescriptionByIndexReturnsNulloptForEmptyFont)
{
    Font font;

    auto axis = font.getAxisDescription (0);

    EXPECT_FALSE (axis.has_value());
}

TEST (FontTests, GetAxisDescriptionByTagReturnsNulloptForEmptyFont)
{
    Font font;

    auto axis = font.getAxisDescription ("wght");

    EXPECT_FALSE (axis.has_value());
}

TEST (FontTests, GetAxisValueByIndexReturnsZeroForEmptyFont)
{
    Font font;

    EXPECT_EQ (0.0f, font.getAxisValue (0));
}

TEST (FontTests, GetAxisValueByInvalidIndex)
{
    Font font;

    EXPECT_EQ (0.0f, font.getAxisValue (-1));
    EXPECT_EQ (0.0f, font.getAxisValue (100));
}

TEST (FontTests, GetAxisValueByTagReturnsZeroForEmptyFont)
{
    Font font;

    EXPECT_EQ (0.0f, font.getAxisValue ("wght"));
}

TEST (FontTests, SetAxisValueByIndexDoesNothingForEmptyFont)
{
    Font font;

    // Should not crash
    font.setAxisValue (0, 500.0f);

    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, SetAxisValueByInvalidIndex)
{
    Font font;

    // Should not crash
    font.setAxisValue (-1, 500.0f);
    font.setAxisValue (100, 500.0f);

    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, SetAxisValueByTagDoesNothingForEmptyFont)
{
    Font font;

    // Should not crash
    font.setAxisValue ("wght", 700.0f);

    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, WithAxisValueByIndexReturnsEmptyFontForEmptyFont)
{
    Font font;

    Font newFont = font.withAxisValue (0, 600.0f);

    EXPECT_EQ (0, newFont.getNumAxis());
}

TEST (FontTests, WithAxisValueByTagReturnsEmptyFontForEmptyFont)
{
    Font font;

    Font newFont = font.withAxisValue ("wght", 700.0f);

    EXPECT_EQ (0, newFont.getNumAxis());
}

TEST (FontTests, ResetAxisValueByIndexDoesNothingForEmptyFont)
{
    Font font;

    // Should not crash
    font.resetAxisValue (0);

    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, ResetAxisValueByTagDoesNothingForEmptyFont)
{
    Font font;

    // Should not crash
    font.resetAxisValue ("wght");

    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, ResetAllAxisValuesDoesNothingForEmptyFont)
{
    Font font;

    // Should not crash
    font.resetAllAxisValues();

    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, SetAxisValuesDoesNothingForEmptyFont)
{
    Font font;

    // Should not crash
    font.setAxisValues ({ { "wght", 700.0f }, { "wdth", 75.0f } });

    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, WithAxisValuesReturnsEmptyFontForEmptyFont)
{
    Font font;

    Font newFont = font.withAxisValues ({ { "wght", 700.0f }, { "wdth", 75.0f } });

    EXPECT_EQ (0, newFont.getNumAxis());
}

// ==============================================================================
// Feature Tests
// ==============================================================================

TEST (FontTests, FeatureConstructorWithTag)
{
    Font::Feature feature (0x6C696761, 1); // 'liga'

    EXPECT_EQ (0x6C696761u, feature.tag);
    EXPECT_EQ (1u, feature.value);
}

TEST (FontTests, FeatureConstructorWithString)
{
    Font::Feature feature ("liga", 1);

    EXPECT_EQ (0x6C696761u, feature.tag); // 'liga' in hex
    EXPECT_EQ (1u, feature.value);
}

TEST (FontTests, FeatureConstructorWithDifferentStrings)
{
    Font::Feature kern ("kern", 0);
    Font::Feature smcp ("smcp", 1);

    EXPECT_NE (kern.tag, smcp.tag);
}

TEST (FontTests, WithFeatureReturnsEmptyFontForEmptyFont)
{
    Font font;
    Font::Feature feature ("liga", 1);

    Font newFont = font.withFeature (feature);

    EXPECT_EQ (0, newFont.getNumAxis());
}

TEST (FontTests, WithFeaturesReturnsEmptyFontForEmptyFont)
{
    Font font;

    Font newFont = font.withFeatures ({ { "liga", 1 },
                                        { "kern", 1 },
                                        { "smcp", 1 } });

    EXPECT_EQ (0, newFont.getNumAxis());
}

TEST (FontTests, DISABLED_FeatureStringTagMustBe4Characters)
{
    // This would trigger the assertion in debug builds
    // but should not crash in release
    Font::Feature feature1 ("abc", 1);   // Too short
    Font::Feature feature2 ("abcde", 1); // Too long (will only use first 4)

    // At least verify they construct
    EXPECT_EQ (1u, feature1.value);
    EXPECT_EQ (1u, feature2.value);
}

// ==============================================================================
// Axis Option Tests
// ==============================================================================

TEST (FontTests, AxisOptionConstructor)
{
    Font::AxisOption option ("wght", 700.0f);

    EXPECT_EQ ("wght", option.tagName);
    EXPECT_EQ (700.0f, option.value);
}

TEST (FontTests, AxisOptionWithDifferentValues)
{
    Font::AxisOption weight ("wght", 400.0f);
    Font::AxisOption width ("wdth", 75.0f);

    EXPECT_EQ ("wght", weight.tagName);
    EXPECT_EQ (400.0f, weight.value);
    EXPECT_EQ ("wdth", width.tagName);
    EXPECT_EQ (75.0f, width.value);
}

// ==============================================================================
// Axis Description Tests
// ==============================================================================

TEST (FontTests, AxisDefaultConstructor)
{
    Font::Axis axis;

    EXPECT_TRUE (axis.tagName.isEmpty());
    EXPECT_EQ (0.0f, axis.minimumValue);
    EXPECT_EQ (0.0f, axis.maximumValue);
    EXPECT_EQ (0.0f, axis.defaultValue);
}

// ==============================================================================
// Equality Tests
// ==============================================================================

TEST (FontTests, EmptyFontsAreEqual)
{
    Font font1;
    Font font2;

    EXPECT_TRUE (font1 == font2);
    EXPECT_FALSE (font1 != font2);
}

TEST (FontTests, SameFontsAreEqual)
{
    Font font1;
    Font font2 = font1;

    EXPECT_TRUE (font1 == font2);
    EXPECT_FALSE (font1 != font2);
}

TEST (FontTests, HeightDoesNotAffectEquality)
{
    Font font1;
    font1.setHeight (12.0f);

    Font font2;
    font2.setHeight (24.0f);

    // Fonts are equal if they wrap the same underlying rive::Font
    EXPECT_TRUE (font1 == font2);
}

TEST (FontTests, InequalityOperator)
{
    Font font1;
    Font font2;

    // Both empty fonts should be equal
    EXPECT_FALSE (font1 != font2);
    EXPECT_TRUE (font1 == font2);
}

// ==============================================================================
// Chain Operations Tests
// ==============================================================================

TEST (FontTests, ChainWithHeightOperations)
{
    Font font;

    Font newFont = font.withHeight (16.0f).withHeight (24.0f);

    EXPECT_EQ (24.0f, newFont.getHeight());
    EXPECT_EQ (12.0f, font.getHeight());
}

TEST (FontTests, CombinedHeightAndAxisOperations)
{
    Font font;

    Font newFont = font
                       .withHeight (18.0f)
                       .withAxisValue ("wght", 700.0f)
                       .withHeight (24.0f);

    EXPECT_EQ (24.0f, newFont.getHeight());
    EXPECT_EQ (12.0f, font.getHeight());
}

// ==============================================================================
// Edge Cases
// ==============================================================================

TEST (FontTests, SetHeightWithVeryLargeValue)
{
    Font font;
    font.setHeight (10000.0f);

    EXPECT_EQ (10000.0f, font.getHeight());
}

TEST (FontTests, SetHeightWithVerySmallValue)
{
    Font font;
    font.setHeight (0.001f);

    EXPECT_EQ (0.001f, font.getHeight());
}

TEST (FontTests, MultipleHeightChanges)
{
    Font font;

    font.setHeight (16.0f);
    EXPECT_EQ (16.0f, font.getHeight());

    font.setHeight (20.0f);
    EXPECT_EQ (20.0f, font.getHeight());

    font.setHeight (12.0f);
    EXPECT_EQ (12.0f, font.getHeight());
}

TEST (FontTests, CopyFontPreservesHeight)
{
    Font font1;
    font1.setHeight (32.0f);

    Font font2 = font1;
    font2.setHeight (48.0f);

    EXPECT_EQ (32.0f, font1.getHeight());
    EXPECT_EQ (48.0f, font2.getHeight());
}

TEST (FontTests, WithAxisValuesEmptyList)
{
    Font font;

    // Empty initializer list should handle gracefully
    Font newFont = font.withAxisValues ({});

    EXPECT_EQ (0, newFont.getNumAxis());
}

TEST (FontTests, SetAxisValuesEmptyList)
{
    Font font;

    // Empty initializer list should handle gracefully
    font.setAxisValues ({});

    EXPECT_EQ (0, font.getNumAxis());
}

TEST (FontTests, GetAxisDescriptionOutOfBounds)
{
    Font font;

    EXPECT_FALSE (font.getAxisDescription (-1).has_value());
    EXPECT_FALSE (font.getAxisDescription (0).has_value());
    EXPECT_FALSE (font.getAxisDescription (1000).has_value());
}

TEST (FontTests, AxisTagNameMustBe4Characters)
{
    Font font;

    // These should handle gracefully (won't find the axis)
    EXPECT_FALSE (font.getAxisDescription ("w").has_value());
    EXPECT_FALSE (font.getAxisDescription ("wg").has_value());
    EXPECT_FALSE (font.getAxisDescription ("wgh").has_value());
    EXPECT_FALSE (font.getAxisDescription ("wghtt").has_value());
}

// ==============================================================================
// Memory and Resource Tests
// ==============================================================================

TEST (FontTests, CopyDoesNotCrash)
{
    Font font1;
    font1.setHeight (16.0f);

    {
        Font font2 = font1;
        EXPECT_EQ (font1.getHeight(), font2.getHeight());
    }

    // font1 should still be valid
    EXPECT_EQ (16.0f, font1.getHeight());
}

TEST (FontTests, MoveDoesNotCrash)
{
    Font font1;
    font1.setHeight (20.0f);

    {
        Font font2 = std::move (font1);
        EXPECT_EQ (20.0f, font2.getHeight());
    }

    // Moved-from font is in valid but unspecified state
    // Should not crash when destroyed
}

TEST (FontTests, MultipleOperationsOnSameFont)
{
    Font font;

    font.setHeight (16.0f);
    font.setAxisValue (0, 700.0f);
    font.resetAxisValue (0);
    font.resetAllAxisValues();
    font.setAxisValues ({ { "wght", 400.0f } });

    // Should not crash
    EXPECT_EQ (16.0f, font.getHeight());
}

TEST (FontTests, ChainedWithOperations)
{
    Font font;

    Font result = font
                      .withHeight (16.0f)
                      .withAxisValue (0, 700.0f)
                      .withAxisValue ("wdth", 75.0f)
                      .withAxisValues ({ { "wght", 400.0f } })
                      .withFeature ({ "liga", 1 })
                      .withFeatures ({ { "kern", 1 }, { "smcp", 1 } })
                      .withHeight (24.0f);

    EXPECT_EQ (24.0f, result.getHeight());
    EXPECT_EQ (12.0f, font.getHeight()); // Original unchanged
}
