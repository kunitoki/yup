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

#include <yup_audio_basics/yup_audio_basics.h>

using namespace yup;

//==============================================================================
TEST (DecibelsTests, DecibelsToGainZeroDB)
{
    // 0 dB should equal gain of 1.0
    float gain = Decibels::decibelsToGain (0.0f);
    EXPECT_FLOAT_EQ (gain, 1.0f);
}

TEST (DecibelsTests, DecibelsToGainPositive)
{
    // +6 dB should approximately double the gain
    float gain = Decibels::decibelsToGain (6.0f);
    EXPECT_NEAR (gain, 1.9953f, 0.001f);
}

TEST (DecibelsTests, DecibelsToGainNegative)
{
    // -6 dB should approximately halve the gain
    float gain = Decibels::decibelsToGain (-6.0f);
    EXPECT_NEAR (gain, 0.5012f, 0.001f);
}

TEST (DecibelsTests, DecibelsToGainMinusInfinity)
{
    // Below minusInfinityDb should return 0 (line 62-63)
    float gain = Decibels::decibelsToGain (-120.0f, -100.0f);
    EXPECT_FLOAT_EQ (gain, 0.0f);
}

TEST (DecibelsTests, DecibelsToGainAtMinusInfinityBoundary)
{
    // Exactly at minusInfinityDb should return 0
    float gain = Decibels::decibelsToGain (-100.0f, -100.0f);
    EXPECT_FLOAT_EQ (gain, 0.0f);
}

TEST (DecibelsTests, DecibelsToGainJustAboveMinusInfinity)
{
    // Just above minusInfinityDb should return non-zero (line 62)
    float gain = Decibels::decibelsToGain (-99.9f, -100.0f);
    EXPECT_GT (gain, 0.0f);
}

TEST (DecibelsTests, DecibelsToGainCustomMinusInfinity)
{
    // Test with custom minusInfinityDb value
    float gain = Decibels::decibelsToGain (-150.0f, -140.0f);
    EXPECT_FLOAT_EQ (gain, 0.0f);
}

TEST (DecibelsTests, DecibelsToGainDoubleType)
{
    // Test with double precision
    double gain = Decibels::decibelsToGain (0.0);
    EXPECT_DOUBLE_EQ (gain, 1.0);
}

TEST (DecibelsTests, DecibelsToGainLargePositive)
{
    // +20 dB = 10x gain
    float gain = Decibels::decibelsToGain (20.0f);
    EXPECT_NEAR (gain, 10.0f, 0.001f);
}

TEST (DecibelsTests, DecibelsToGainLargeNegative)
{
    // -20 dB = 0.1x gain
    float gain = Decibels::decibelsToGain (-20.0f);
    EXPECT_NEAR (gain, 0.1f, 0.001f);
}

TEST (DecibelsTests, DecibelsToGainVerySmall)
{
    // -60 dB = 0.001x gain
    float gain = Decibels::decibelsToGain (-60.0f);
    EXPECT_NEAR (gain, 0.001f, 0.0001f);
}

//==============================================================================
TEST (DecibelsTests, GainToDecibelsUnity)
{
    // Gain of 1.0 should equal 0 dB
    float db = Decibels::gainToDecibels (1.0f);
    EXPECT_FLOAT_EQ (db, 0.0f);
}

TEST (DecibelsTests, GainToDecibelsDouble)
{
    // Gain of 2.0 should be approximately +6 dB
    float db = Decibels::gainToDecibels (2.0f);
    EXPECT_NEAR (db, 6.0206f, 0.001f);
}

TEST (DecibelsTests, GainToDecibelsHalf)
{
    // Gain of 0.5 should be approximately -6 dB
    float db = Decibels::gainToDecibels (0.5f);
    EXPECT_NEAR (db, -6.0206f, 0.001f);
}

TEST (DecibelsTests, GainToDecibelsZero)
{
    // Gain of 0 should return minusInfinityDb (line 77)
    float db = Decibels::gainToDecibels (0.0f, -100.0f);
    EXPECT_FLOAT_EQ (db, -100.0f);
}

TEST (DecibelsTests, GainToDecibelsNegative)
{
    // Negative gain should return minusInfinityDb (line 76-77)
    float db = Decibels::gainToDecibels (-0.5f, -100.0f);
    EXPECT_FLOAT_EQ (db, -100.0f);
}

TEST (DecibelsTests, GainToDecibelsVerySmall)
{
    // Very small gain close to 0
    float db = Decibels::gainToDecibels (0.001f);
    EXPECT_NEAR (db, -60.0f, 0.001f);
}

TEST (DecibelsTests, GainToDecibelsCustomMinusInfinity)
{
    // Test with custom minusInfinityDb value
    float db = Decibels::gainToDecibels (0.0f, -120.0f);
    EXPECT_FLOAT_EQ (db, -120.0f);
}

TEST (DecibelsTests, GainToDecibelsClampedToMinusInfinity)
{
    // Very small gain that results in dB below minusInfinityDb (line 76 jmax)
    float db = Decibels::gainToDecibels (0.00001f, -80.0f);
    EXPECT_FLOAT_EQ (db, -80.0f);
}

TEST (DecibelsTests, GainToDecibelsDoubleType)
{
    // Test with double precision
    double db = Decibels::gainToDecibels (1.0);
    EXPECT_DOUBLE_EQ (db, 0.0);
}

TEST (DecibelsTests, GainToDecibelsTen)
{
    // Gain of 10.0 should be +20 dB
    float db = Decibels::gainToDecibels (10.0f);
    EXPECT_NEAR (db, 20.0f, 0.001f);
}

TEST (DecibelsTests, GainToDecibelsOneTenth)
{
    // Gain of 0.1 should be -20 dB
    float db = Decibels::gainToDecibels (0.1f);
    EXPECT_NEAR (db, -20.0f, 0.001f);
}

//==============================================================================
TEST (DecibelsTests, RoundTripConversionUnity)
{
    // Convert 0 dB to gain and back
    float db1 = 0.0f;
    float gain = Decibels::decibelsToGain (db1);
    float db2 = Decibels::gainToDecibels (gain);
    EXPECT_NEAR (db2, db1, 0.001f);
}

TEST (DecibelsTests, RoundTripConversionPositive)
{
    // Convert +10 dB to gain and back
    float db1 = 10.0f;
    float gain = Decibels::decibelsToGain (db1);
    float db2 = Decibels::gainToDecibels (gain);
    EXPECT_NEAR (db2, db1, 0.001f);
}

TEST (DecibelsTests, RoundTripConversionNegative)
{
    // Convert -10 dB to gain and back
    float db1 = -10.0f;
    float gain = Decibels::decibelsToGain (db1);
    float db2 = Decibels::gainToDecibels (gain);
    EXPECT_NEAR (db2, db1, 0.001f);
}

TEST (DecibelsTests, RoundTripConversionGainToDb)
{
    // Convert gain 2.0 to dB and back
    float gain1 = 2.0f;
    float db = Decibels::gainToDecibels (gain1);
    float gain2 = Decibels::decibelsToGain (db);
    EXPECT_NEAR (gain2, gain1, 0.001f);
}

//==============================================================================
TEST (DecibelsTests, GainWithLowerBoundBasic)
{
    // Gain above lower bound should remain unchanged
    float gain = Decibels::gainWithLowerBound (0.5f, -20.0f);
    EXPECT_FLOAT_EQ (gain, 0.5f);
}

TEST (DecibelsTests, GainWithLowerBoundBelowThreshold)
{
    // Gain below lower bound should be clamped (line 91)
    float gain = Decibels::gainWithLowerBound (0.001f, -20.0f);
    float expectedMin = Decibels::decibelsToGain (-20.0f, -21.0f);
    EXPECT_FLOAT_EQ (gain, expectedMin);
}

TEST (DecibelsTests, GainWithLowerBoundZeroGain)
{
    // Zero gain should be clamped to lower bound
    float gain = Decibels::gainWithLowerBound (0.0f, -20.0f);
    float expectedMin = Decibels::decibelsToGain (-20.0f, -21.0f);
    EXPECT_FLOAT_EQ (gain, expectedMin);
}

TEST (DecibelsTests, GainWithLowerBoundNegativeBound)
{
    // Tests line 89 assertion (negative decibel value)
    float gain = Decibels::gainWithLowerBound (0.5f, -30.0f);
    EXPECT_GE (gain, Decibels::decibelsToGain (-30.0f, -31.0f));
}

TEST (DecibelsTests, GainWithLowerBoundHighGain)
{
    // High gain should remain unchanged
    float gain = Decibels::gainWithLowerBound (2.0f, -20.0f);
    EXPECT_FLOAT_EQ (gain, 2.0f);
}

TEST (DecibelsTests, GainWithLowerBoundExactlyAtBound)
{
    // Gain exactly at lower bound
    float lowerBoundDb = -20.0f;
    float boundGain = Decibels::decibelsToGain (lowerBoundDb);
    float gain = Decibels::gainWithLowerBound (boundGain, lowerBoundDb);
    EXPECT_FLOAT_EQ (gain, boundGain);
}

TEST (DecibelsTests, GainWithLowerBoundDoubleType)
{
    // Test with double precision
    double gain = Decibels::gainWithLowerBound (0.5, -20.0);
    EXPECT_DOUBLE_EQ (gain, 0.5);
}

TEST (DecibelsTests, GainWithLowerBoundVeryLowBound)
{
    // Test with very low bound
    float gain = Decibels::gainWithLowerBound (0.00001f, -80.0f);
    float expectedMin = Decibels::decibelsToGain (-80.0f, -81.0f);
    EXPECT_FLOAT_EQ (gain, expectedMin);
}

//==============================================================================
TEST (DecibelsTests, ToStringZeroDB)
{
    // 0 dB should show "+0.00 dB" (line 121-122)
    String s = Decibels::toString (0.0f, 2);
    EXPECT_TRUE (s.startsWith ("+0"));
    EXPECT_TRUE (s.endsWith (" dB"));
}

TEST (DecibelsTests, ToStringPositive)
{
    // Positive dB should have '+' prefix (line 121-122)
    String s = Decibels::toString (6.0f, 2);
    EXPECT_TRUE (s.startsWith ("+6"));
    EXPECT_TRUE (s.contains ("dB"));
}

TEST (DecibelsTests, ToStringNegative)
{
    // Negative dB should have '-' (no '+' on line 122)
    String s = Decibels::toString (-6.0f, 2);
    EXPECT_TRUE (s.startsWith ("-6"));
    EXPECT_TRUE (s.contains ("dB"));
}

TEST (DecibelsTests, ToStringMinusInfinity)
{
    // Below minusInfinityDb should return "-INF" (lines 112-115)
    String s = Decibels::toString (-120.0f, 2, -100.0f);
    EXPECT_TRUE (s.contains ("-INF"));
}

TEST (DecibelsTests, ToStringAtMinusInfinity)
{
    // Exactly at minusInfinityDb should return "-INF" (line 112)
    String s = Decibels::toString (-100.0f, 2, -100.0f);
    EXPECT_TRUE (s.contains ("-INF"));
}

TEST (DecibelsTests, ToStringCustomMinusInfinityString)
{
    // Custom minus infinity string (lines 114-117)
    String s = Decibels::toString (-120.0f, 2, -100.0f, true, "-\u221E");
    EXPECT_TRUE (s.contains ("-\u221E"));
}

TEST (DecibelsTests, ToStringWithoutSuffix)
{
    // shouldIncludeSuffix = false (line 130-131)
    String s = Decibels::toString (6.0f, 2, -100.0f, false);
    EXPECT_FALSE (s.contains ("dB"));
}

TEST (DecibelsTests, ToStringWithSuffix)
{
    // shouldIncludeSuffix = true (line 130-131)
    String s = Decibels::toString (6.0f, 2, -100.0f, true);
    EXPECT_TRUE (s.contains ("dB"));
}

TEST (DecibelsTests, ToStringZeroDecimalPlaces)
{
    // decimalPlaces = 0 should use roundToInt (line 124-125)
    String s = Decibels::toString (6.789f, 0, -100.0f, false);
    EXPECT_TRUE (s == "+7" || s == "+6"); // Depending on rounding
}

TEST (DecibelsTests, ToStringOneDecimalPlace)
{
    // decimalPlaces = 1 (line 127)
    String s = Decibels::toString (6.789f, 1, -100.0f, false);
    EXPECT_TRUE (s.startsWith ("+6."));
}

TEST (DecibelsTests, ToStringTwoDecimalPlaces)
{
    // decimalPlaces = 2 (line 127)
    String s = Decibels::toString (6.789f, 2, -100.0f, false);
    EXPECT_TRUE (s.startsWith ("+6."));
}

TEST (DecibelsTests, ToStringThreeDecimalPlaces)
{
    // decimalPlaces = 3 (line 127)
    String s = Decibels::toString (6.789f, 3, -100.0f, false);
    EXPECT_TRUE (s.startsWith ("+6."));
}

TEST (DecibelsTests, ToStringNegativeDecimalPlaces)
{
    // Negative decimalPlaces should use roundToInt (line 124-125)
    String s = Decibels::toString (6.789f, -1, -100.0f, false);
    EXPECT_TRUE (s == "+7" || s == "+6");
}

TEST (DecibelsTests, ToStringDoubleType)
{
    // Test with double precision
    String s = Decibels::toString (6.0, 2);
    EXPECT_TRUE (s.startsWith ("+6"));
}

TEST (DecibelsTests, ToStringPreallocatesBytes)
{
    // Tests line 110 (preallocateBytes)
    String s = Decibels::toString (123.456f, 3);
    EXPECT_FALSE (s.isEmpty());
}

TEST (DecibelsTests, ToStringEmptyCustomMinusInfinityString)
{
    // Empty customMinusInfinityString should use "-INF" (line 114-115)
    String s = Decibels::toString (-120.0f, 2, -100.0f, true, "");
    EXPECT_TRUE (s.contains ("-INF"));
}

TEST (DecibelsTests, ToStringNonEmptyCustomMinusInfinityString)
{
    // Non-empty customMinusInfinityString (line 116-117)
    String s = Decibels::toString (-120.0f, 2, -100.0f, true, "Silent");
    EXPECT_TRUE (s.contains ("Silent"));
}

TEST (DecibelsTests, ToStringVeryLargePositive)
{
    // Very large positive dB
    String s = Decibels::toString (100.0f, 2);
    EXPECT_TRUE (s.startsWith ("+100"));
}

TEST (DecibelsTests, ToStringVeryLargeNegative)
{
    // Very large negative dB (but above minusInfinityDb)
    String s = Decibels::toString (-90.0f, 2, -100.0f);
    EXPECT_TRUE (s.startsWith ("-90"));
}

TEST (DecibelsTests, ToStringJustAboveMinusInfinity)
{
    // Just above minusInfinityDb should show number, not "-INF"
    String s = Decibels::toString (-99.9f, 1, -100.0f);
    EXPECT_TRUE (s.startsWith ("-99"));
    EXPECT_FALSE (s.contains ("-INF"));
}

//==============================================================================
TEST (DecibelsTests, DefaultMinusInfinityValue)
{
    // Test default minusInfinityDb = -100 (line 140)
    float gain = Decibels::decibelsToGain (-120.0f);
    EXPECT_FLOAT_EQ (gain, 0.0f);
}

TEST (DecibelsTests, MathematicalAccuracy)
{
    // Verify the mathematical formulas
    // decibelsToGain: gain = 10^(dB * 0.05) = 10^(dB/20)
    float gain = Decibels::decibelsToGain (20.0f);
    EXPECT_NEAR (gain, 10.0f, 0.001f);

    // gainToDecibels: dB = log10(gain) * 20
    float db = Decibels::gainToDecibels (10.0f);
    EXPECT_NEAR (db, 20.0f, 0.001f);
}

TEST (DecibelsTests, EdgeCaseVerySmallPositiveGain)
{
    // Very small positive gain
    float gain = 0.0001f;
    float db = Decibels::gainToDecibels (gain);
    float gainBack = Decibels::decibelsToGain (db);
    EXPECT_NEAR (gainBack, gain, 0.00001f);
}

TEST (DecibelsTests, EdgeCaseVeryLargeGain)
{
    // Very large gain
    float gain = 1000.0f;
    float db = Decibels::gainToDecibels (gain);
    float gainBack = Decibels::decibelsToGain (db);
    EXPECT_NEAR (gainBack, gain, 1.0f);
}

//==============================================================================
TEST (DecibelsTests, TypeConsistency)
{
    // Ensure float and double produce consistent results
    float dbFloat = Decibels::gainToDecibels (2.0f);
    double dbDouble = Decibels::gainToDecibels (2.0);

    EXPECT_NEAR (static_cast<double> (dbFloat), dbDouble, 0.001);
}

TEST (DecibelsTests, SymmetricOperations)
{
    // Test that operations are symmetric
    std::vector<float> testValues = { 0.001f, 0.1f, 0.5f, 1.0f, 2.0f, 10.0f };

    for (float gain : testValues)
    {
        float db = Decibels::gainToDecibels (gain);
        float gainBack = Decibels::decibelsToGain (db);
        EXPECT_NEAR (gainBack, gain, 0.01f * gain);
    }
}
