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

#include <yup_core/yup_core.h>

using namespace yup;

// ============================================================================
// Basic Constructors
// ============================================================================

TEST (NormalisableRangeTests, DefaultConstructor)
{
    NormalisableRange<float> range;

    EXPECT_EQ (0.0f, range.start);
    EXPECT_EQ (1.0f, range.end);
    EXPECT_EQ (0.0f, range.interval);
    EXPECT_EQ (1.0f, range.skew);
    EXPECT_FALSE (range.symmetricSkew);
}

TEST (NormalisableRangeTests, RangeOnlyConstructor)
{
    NormalisableRange<float> range (0.0f, 100.0f);

    EXPECT_EQ (0.0f, range.start);
    EXPECT_EQ (100.0f, range.end);
    EXPECT_EQ (0.0f, range.interval);
    EXPECT_EQ (1.0f, range.skew);
    EXPECT_FALSE (range.symmetricSkew);
}

TEST (NormalisableRangeTests, RangeWithIntervalConstructor)
{
    NormalisableRange<float> range (0.0f, 100.0f, 1.0f);

    EXPECT_EQ (0.0f, range.start);
    EXPECT_EQ (100.0f, range.end);
    EXPECT_EQ (1.0f, range.interval);
    EXPECT_EQ (1.0f, range.skew);
    EXPECT_FALSE (range.symmetricSkew);
}

TEST (NormalisableRangeTests, FullConstructor)
{
    NormalisableRange<float> range (0.0f, 100.0f, 1.0f, 0.5f, false);

    EXPECT_EQ (0.0f, range.start);
    EXPECT_EQ (100.0f, range.end);
    EXPECT_EQ (1.0f, range.interval);
    EXPECT_EQ (0.5f, range.skew);
    EXPECT_FALSE (range.symmetricSkew);
}

TEST (NormalisableRangeTests, ConstructorWithSymmetricSkew)
{
    NormalisableRange<float> range (0.0f, 100.0f, 1.0f, 2.0f, true);

    EXPECT_EQ (0.0f, range.start);
    EXPECT_EQ (100.0f, range.end);
    EXPECT_EQ (1.0f, range.interval);
    EXPECT_EQ (2.0f, range.skew);
    EXPECT_TRUE (range.symmetricSkew);
}

TEST (NormalisableRangeTests, RangeObjectConstructor)
{
    Range<float> r (10.0f, 50.0f);
    NormalisableRange<float> range (r);

    EXPECT_EQ (10.0f, range.start);
    EXPECT_EQ (50.0f, range.end);
    EXPECT_EQ (0.0f, range.interval);
}

TEST (NormalisableRangeTests, RangeObjectWithIntervalConstructor)
{
    Range<float> r (10.0f, 50.0f);
    NormalisableRange<float> range (r, 0.5f);

    EXPECT_EQ (10.0f, range.start);
    EXPECT_EQ (50.0f, range.end);
    EXPECT_EQ (0.5f, range.interval);
}

// ============================================================================
// Copy and Move Semantics
// ============================================================================

TEST (NormalisableRangeTests, CopyConstructor)
{
    NormalisableRange<float> original (0.0f, 100.0f, 1.0f, 2.0f);
    NormalisableRange<float> copy (original);

    EXPECT_EQ (original.start, copy.start);
    EXPECT_EQ (original.end, copy.end);
    EXPECT_EQ (original.interval, copy.interval);
    EXPECT_EQ (original.skew, copy.skew);
}

TEST (NormalisableRangeTests, CopyAssignment)
{
    NormalisableRange<float> original (0.0f, 100.0f, 1.0f, 2.0f);
    NormalisableRange<float> copy;
    copy = original;

    EXPECT_EQ (original.start, copy.start);
    EXPECT_EQ (original.end, copy.end);
    EXPECT_EQ (original.interval, copy.interval);
    EXPECT_EQ (original.skew, copy.skew);
}

TEST (NormalisableRangeTests, MoveConstructor)
{
    NormalisableRange<float> original (0.0f, 100.0f, 1.0f, 2.0f);
    NormalisableRange<float> moved (std::move (original));

    EXPECT_EQ (0.0f, moved.start);
    EXPECT_EQ (100.0f, moved.end);
    EXPECT_EQ (1.0f, moved.interval);
    EXPECT_EQ (2.0f, moved.skew);
}

TEST (NormalisableRangeTests, MoveAssignment)
{
    NormalisableRange<float> original (0.0f, 100.0f, 1.0f, 2.0f);
    NormalisableRange<float> moved;
    moved = std::move (original);

    EXPECT_EQ (0.0f, moved.start);
    EXPECT_EQ (100.0f, moved.end);
    EXPECT_EQ (1.0f, moved.interval);
    EXPECT_EQ (2.0f, moved.skew);
}

// ============================================================================
// Basic Conversions (No Skew)
// ============================================================================

TEST (NormalisableRangeTests, ConvertTo0to1Basic)
{
    NormalisableRange<float> range (0.0f, 100.0f);

    EXPECT_NEAR (0.0f, range.convertTo0to1 (0.0f), 0.001f);
    EXPECT_NEAR (0.5f, range.convertTo0to1 (50.0f), 0.001f);
    EXPECT_NEAR (1.0f, range.convertTo0to1 (100.0f), 0.001f);
}

TEST (NormalisableRangeTests, ConvertFrom0to1Basic)
{
    NormalisableRange<float> range (0.0f, 100.0f);

    EXPECT_NEAR (0.0f, range.convertFrom0to1 (0.0f), 0.001f);
    EXPECT_NEAR (50.0f, range.convertFrom0to1 (0.5f), 0.001f);
    EXPECT_NEAR (100.0f, range.convertFrom0to1 (1.0f), 0.001f);
}

TEST (NormalisableRangeTests, ConversionRoundTrip)
{
    NormalisableRange<float> range (0.0f, 100.0f);

    float original = 75.0f;
    float normalized = range.convertTo0to1 (original);
    float backToOriginal = range.convertFrom0to1 (normalized);

    EXPECT_NEAR (original, backToOriginal, 0.001f);
}

TEST (NormalisableRangeTests, ConvertTo0to1WithNegativeRange)
{
    NormalisableRange<float> range (-50.0f, 50.0f);

    EXPECT_NEAR (0.0f, range.convertTo0to1 (-50.0f), 0.001f);
    EXPECT_NEAR (0.5f, range.convertTo0to1 (0.0f), 0.001f);
    EXPECT_NEAR (1.0f, range.convertTo0to1 (50.0f), 0.001f);
}

TEST (NormalisableRangeTests, ConvertFrom0to1WithNegativeRange)
{
    NormalisableRange<float> range (-50.0f, 50.0f);

    EXPECT_NEAR (-50.0f, range.convertFrom0to1 (0.0f), 0.001f);
    EXPECT_NEAR (0.0f, range.convertFrom0to1 (0.5f), 0.001f);
    EXPECT_NEAR (50.0f, range.convertFrom0to1 (1.0f), 0.001f);
}

// ============================================================================
// Conversions with Skew
// ============================================================================

TEST (NormalisableRangeTests, ConversionWithSkewLessThan1)
{
    NormalisableRange<float> range (0.0f, 100.0f, 0.0f, 0.5f);

    // With skew < 1, lower values should be expanded
    float mid = range.convertTo0to1 (50.0f);
    EXPECT_GT (mid, 0.5f); // Midpoint should be > 0.5 in normalized space
}

TEST (NormalisableRangeTests, ConversionWithSkewGreaterThan1)
{
    NormalisableRange<float> range (0.0f, 100.0f, 0.0f, 2.0f);

    // With skew > 1, higher values should be expanded
    float mid = range.convertTo0to1 (50.0f);
    EXPECT_LT (mid, 0.5f); // Midpoint should be < 0.5 in normalized space
}

TEST (NormalisableRangeTests, ConversionWithSkewRoundTrip)
{
    NormalisableRange<float> range (0.0f, 100.0f, 0.0f, 2.0f);

    float original = 30.0f;
    float normalized = range.convertTo0to1 (original);
    float backToOriginal = range.convertFrom0to1 (normalized);

    EXPECT_NEAR (original, backToOriginal, 0.001f);
}

TEST (NormalisableRangeTests, SymmetricSkewConversion)
{
    NormalisableRange<float> range (0.0f, 100.0f, 0.0f, 2.0f, true);

    // Center should map to 0.5
    EXPECT_NEAR (0.5f, range.convertTo0to1 (50.0f), 0.001f);

    // Values should be symmetric around center
    float normalized25 = range.convertTo0to1 (25.0f);
    float normalized75 = range.convertTo0to1 (75.0f);
    EXPECT_NEAR (0.5f - normalized25, normalized75 - 0.5f, 0.001f);
}

TEST (NormalisableRangeTests, SetSkewForCentre)
{
    NormalisableRange<float> range (0.0f, 100.0f);

    range.setSkewForCentre (20.0f);

    // After setting skew for centre at 20, convertFrom0to1(0.5) should give 20
    EXPECT_NEAR (20.0f, range.convertFrom0to1 (0.5f), 0.1f);
}

// ============================================================================
// Snapping
// ============================================================================

TEST (NormalisableRangeTests, SnapToLegalValueWithNoInterval)
{
    NormalisableRange<float> range (0.0f, 100.0f);

    EXPECT_NEAR (42.7f, range.snapToLegalValue (42.7f), 0.001f);
}

TEST (NormalisableRangeTests, SnapToLegalValueWithInterval)
{
    NormalisableRange<float> range (0.0f, 100.0f, 10.0f);

    EXPECT_NEAR (0.0f, range.snapToLegalValue (4.9f), 0.001f);
    EXPECT_NEAR (10.0f, range.snapToLegalValue (5.0f), 0.001f);
    EXPECT_NEAR (10.0f, range.snapToLegalValue (14.9f), 0.001f);
    EXPECT_NEAR (20.0f, range.snapToLegalValue (15.0f), 0.001f);
    EXPECT_NEAR (40.0f, range.snapToLegalValue (42.3f), 0.001f);
}

TEST (NormalisableRangeTests, SnapToLegalValueClampsToBounds)
{
    NormalisableRange<float> range (0.0f, 100.0f, 10.0f);

    EXPECT_NEAR (0.0f, range.snapToLegalValue (-10.0f), 0.001f);
    EXPECT_NEAR (100.0f, range.snapToLegalValue (110.0f), 0.001f);
}

TEST (NormalisableRangeTests, SnapToLegalValueWithFractionalInterval)
{
    NormalisableRange<float> range (0.0f, 1.0f, 0.1f);

    EXPECT_NEAR (0.0f, range.snapToLegalValue (0.04f), 0.001f);
    EXPECT_NEAR (0.1f, range.snapToLegalValue (0.05f), 0.001f);
    EXPECT_NEAR (0.5f, range.snapToLegalValue (0.52f), 0.001f);
}

// ============================================================================
// Custom Functions
// ============================================================================

TEST (NormalisableRangeTests, CustomConversionFunctions)
{
    auto fromNormalized = [] (float start, float end, float proportion)
    {
        // Custom exponential mapping
        return start + (end - start) * std::exp (proportion) / std::exp (1.0f);
    };

    auto toNormalized = [] (float start, float end, float value)
    {
        // Inverse of above
        return std::log ((value - start) / (end - start) * std::exp (1.0f));
    };

    NormalisableRange<float> range (0.0f, 100.0f, fromNormalized, toNormalized);

    // Test round-trip conversion
    float original = 50.0f;
    float normalized = range.convertTo0to1 (original);
    float backToOriginal = range.convertFrom0to1 (normalized);

    EXPECT_NEAR (original, backToOriginal, 0.1f);
}

TEST (NormalisableRangeTests, CustomSnapFunction)
{
    auto customSnap = [] (float start, float end, float value)
    {
        // Snap to nearest multiple of 5
        return start + 5.0f * std::round ((value - start) / 5.0f);
    };

    NormalisableRange<float> range (0.0f, 100.0f, [] (float s, float e, float p)
    {
        return s + (e - s) * p;
    },
                                    [] (float s, float e, float v)
    {
        return (v - s) / (e - s);
    },
                                    customSnap);

    EXPECT_NEAR (0.0f, range.snapToLegalValue (2.4f), 0.001f);
    EXPECT_NEAR (5.0f, range.snapToLegalValue (2.5f), 0.001f);
    EXPECT_NEAR (45.0f, range.snapToLegalValue (43.2f), 0.001f);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST (NormalisableRangeTests, ConversionAtBoundaries)
{
    NormalisableRange<float> range (0.0f, 100.0f);

    EXPECT_NEAR (0.0f, range.convertTo0to1 (0.0f), 0.001f);
    EXPECT_NEAR (1.0f, range.convertTo0to1 (100.0f), 0.001f);
    EXPECT_NEAR (0.0f, range.convertFrom0to1 (0.0f), 0.001f);
    EXPECT_NEAR (100.0f, range.convertFrom0to1 (1.0f), 0.001f);
}

TEST (NormalisableRangeTests, DISABLED_ConversionOutOfBounds)
{
    NormalisableRange<float> range (0.0f, 100.0f);

    // Values outside range should be clamped ?
    EXPECT_NEAR (0.0f, range.convertTo0to1 (-10.0f), 0.001f);
    EXPECT_NEAR (1.0f, range.convertTo0to1 (110.0f), 0.001f);
}

TEST (NormalisableRangeTests, GetRange)
{
    NormalisableRange<float> range (10.0f, 50.0f);

    Range<float> r = range.getRange();
    EXPECT_EQ (10.0f, r.getStart());
    EXPECT_EQ (50.0f, r.getEnd());
}

TEST (NormalisableRangeTests, DoubleType)
{
    NormalisableRange<double> range (0.0, 1000.0, 0.1, 2.0);

    EXPECT_EQ (0.0, range.start);
    EXPECT_EQ (1000.0, range.end);
    EXPECT_EQ (0.1, range.interval);
    EXPECT_EQ (2.0, range.skew);

    double normalized = range.convertTo0to1 (500.0);
    double backToOriginal = range.convertFrom0to1 (normalized);
    EXPECT_NEAR (500.0, backToOriginal, 0.01);
}

TEST (NormalisableRangeTests, IntegerType)
{
    NormalisableRange<int> range (0, 100);

    EXPECT_EQ (0, range.start);
    EXPECT_EQ (100, range.end);

    int normalized = range.convertTo0to1 (50);
    EXPECT_EQ (0, normalized); // Integer conversion truncates
}

// ============================================================================
// Complex Scenarios
// ============================================================================

TEST (NormalisableRangeTests, FrequencyRangeWithSkew)
{
    // Common audio use case: frequency range from 20Hz to 20kHz with logarithmic scale
    NormalisableRange<float> freqRange (20.0f, 20000.0f);
    freqRange.setSkewForCentre (1000.0f);

    // 1kHz should map to approximately 0.5 in normalized space
    EXPECT_NEAR (0.5f, freqRange.convertTo0to1 (1000.0f), 0.01f);

    // Lower frequencies should take up more of the normalized space
    float normalized100Hz = freqRange.convertTo0to1 (100.0f);
    float normalized10kHz = freqRange.convertTo0to1 (10000.0f);

    EXPECT_GT (normalized100Hz, 0.1f); // More than linear would give
    EXPECT_LT (normalized10kHz, 0.9f); // Less than linear would give
}

TEST (NormalisableRangeTests, GainRangeWithSymmetricSkew)
{
    // Audio gain from -24dB to +24dB with symmetric skew around 0dB
    NormalisableRange<float> gainRange (-24.0f, 24.0f, 0.0f, 2.0f, true);

    // 0dB should map to 0.5
    EXPECT_NEAR (0.5f, gainRange.convertTo0to1 (0.0f), 0.001f);

    // Symmetric values should have symmetric normalized positions
    float normalizedMinus6 = gainRange.convertTo0to1 (-6.0f);
    float normalizedPlus6 = gainRange.convertTo0to1 (6.0f);

    EXPECT_NEAR (0.5f - normalizedMinus6, normalizedPlus6 - 0.5f, 0.001f);
}

TEST (NormalisableRangeTests, ParameterWithSnapAndSkew)
{
    // Parameter from 0-100 with 1.0 intervals and logarithmic skew
    NormalisableRange<float> range (0.0f, 100.0f, 1.0f, 0.5f);

    // Test that snapping works correctly
    float snapped = range.snapToLegalValue (42.7f);
    EXPECT_NEAR (43.0f, snapped, 0.001f);

    // Test that conversion with skew still works
    float normalized = range.convertTo0to1 (snapped);
    float converted = range.convertFrom0to1 (normalized);
    EXPECT_NEAR (snapped, converted, 0.01f);
}
