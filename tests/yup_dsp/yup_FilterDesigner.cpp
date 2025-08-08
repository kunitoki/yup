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

#include <yup_dsp/yup_dsp.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
class FilterDesignerTests : public ::testing::Test
{
protected:
    static constexpr double tolerance = 1e-4;
    static constexpr float toleranceF = 1e-4f;
    static constexpr double sampleRate = 44100.0;

    void SetUp() override
    {
        // Common test parameters
        frequency = 1000.0;
        qFactor = 0.707;
        gainDb = 6.0;
        nyquist = sampleRate * 0.5;
    }

    double frequency;
    double qFactor;
    double gainDb;
    double nyquist;
};

//==============================================================================
// First Order Filter Tests
//==============================================================================
TEST_F (FilterDesignerTests, FirstOrderLowpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowpass (frequency, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // For first-order lowpass: b0 should be positive
    EXPECT_GT (coeffs.b0, 0.0);
    // Note: First-order filters may have different coefficient structures
    // b1 might be 0 for some implementations

    // a1 should be negative (for stability)
    EXPECT_LT (coeffs.a1, 0.0);

    // DC gain should be approximately 1.0 (b0 + b1) / (1 + a1)
    double dcGain = (coeffs.b0 + coeffs.b1) / (1.0 + coeffs.a1);
    EXPECT_NEAR (1.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, FirstOrderHighpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighpass (frequency, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // For highpass: b0 should equal -b1
    EXPECT_NEAR (coeffs.b0, -coeffs.b1, tolerance);
    EXPECT_GT (coeffs.b0, 0.0);
    EXPECT_LT (coeffs.b1, 0.0);

    // DC gain should be approximately 0.0
    double dcGain = (coeffs.b0 + coeffs.b1) / (1.0 + coeffs.a1);
    EXPECT_NEAR (0.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, FirstOrderLowShelfCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderLowShelf (frequency, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // For positive gain, DC gain should be > 1
    double dcGain = (coeffs.b0 + coeffs.b1) / (1.0 + coeffs.a1);
    double expectedGain = std::pow (10.0, gainDb / 20.0);
    EXPECT_NEAR (expectedGain, dcGain, tolerance * 10);
}

TEST_F (FilterDesignerTests, FirstOrderHighShelfCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderHighShelf (frequency, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // High frequency gain should be approximately the expected gain
    // At Nyquist: gain = (b0 - b1) / (1 - a1)
    double hfGain = (coeffs.b0 - coeffs.b1) / (1.0 - coeffs.a1);
    double expectedGain = std::pow (10.0, gainDb / 20.0);
    EXPECT_NEAR (expectedGain, hfGain, tolerance * 10);
}

TEST_F (FilterDesignerTests, FirstOrderAllpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designFirstOrderAllpass (frequency, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.a1));

    // For allpass: b0 = a1, b1 = 1
    EXPECT_NEAR (coeffs.b0, coeffs.a1, tolerance);
    EXPECT_NEAR (1.0, coeffs.b1, tolerance);

    // Magnitude response should be 1.0 at all frequencies
    // DC gain should be 1.0
    double dcGain = (coeffs.b0 + coeffs.b1) / (1.0 + coeffs.a1);
    EXPECT_NEAR (1.0, dcGain, tolerance);
}

//==============================================================================
// RBJ Biquad Filter Tests
//==============================================================================
TEST_F (FilterDesignerTests, RbjLowpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjLowpass (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For lowpass: b0 = b1/2 = b2, all positive
    EXPECT_NEAR (coeffs.b0, coeffs.b2, tolerance);
    EXPECT_NEAR (coeffs.b1, 2.0 * coeffs.b0, tolerance);
    EXPECT_GT (coeffs.b0, 0.0);

    // DC gain should be 1.0
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, RbjHighpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjHighpass (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For highpass: b0 = b2 > 0, b1 = -2*b0
    EXPECT_NEAR (coeffs.b0, coeffs.b2, tolerance);
    EXPECT_NEAR (coeffs.b1, -2.0 * coeffs.b0, tolerance);
    EXPECT_GT (coeffs.b0, 0.0);

    // DC gain should be 0.0
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (0.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, RbjBandpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjBandpass (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For bandpass: b0 = -b2, b1 = 0
    EXPECT_NEAR (coeffs.b0, -coeffs.b2, tolerance);
    EXPECT_NEAR (0.0, coeffs.b1, tolerance);

    // DC gain should be 0.0
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (0.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, RbjBandstopCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjBandstop (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For bandstop: b0 = b2, magnitude of DC gain should be 1.0
    EXPECT_NEAR (coeffs.b0, coeffs.b2, tolerance);

    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, std::abs (dcGain), tolerance);
}

TEST_F (FilterDesignerTests, RbjPeakCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjPeak (frequency, qFactor, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // DC gain should be approximately 1.0 (no DC boost for peaking filter)
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, dcGain, tolerance);
}

TEST_F (FilterDesignerTests, RbjLowShelfCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjLowShelf (frequency, qFactor, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // DC gain should reflect the shelf gain
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    double expectedGain = std::pow (10.0, gainDb / 20.0);
    EXPECT_NEAR (expectedGain, dcGain, tolerance * 10);
}

TEST_F (FilterDesignerTests, RbjHighShelfCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjHighShelf (frequency, qFactor, gainDb, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // High frequency gain should reflect the shelf gain
    // At z=-1 (Nyquist): gain = (b0 - b1 + b2) / (1 - a1 + a2)
    double hfGain = (coeffs.b0 - coeffs.b1 + coeffs.b2) / (1.0 - coeffs.a1 + coeffs.a2);
    double expectedGain = std::pow (10.0, gainDb / 20.0);
    EXPECT_NEAR (expectedGain, hfGain, tolerance * 10);
}

TEST_F (FilterDesignerTests, RbjAllpassCoefficients)
{
    auto coeffs = FilterDesigner<double>::designRbjAllpass (frequency, qFactor, sampleRate);

    // Coefficients should be finite
    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // For allpass: b0 = a2, b1 = a1, b2 = 1
    EXPECT_NEAR (coeffs.b0, coeffs.a2, tolerance);
    EXPECT_NEAR (coeffs.b1, coeffs.a1, tolerance);
    EXPECT_NEAR (1.0, coeffs.b2, tolerance);

    // Magnitude should be 1.0 at DC and Nyquist
    double dcGain = (coeffs.b0 + coeffs.b1 + coeffs.b2) / (1.0 + coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, std::abs (dcGain), tolerance);

    double hfGain = (coeffs.b0 - coeffs.b1 + coeffs.b2) / (1.0 - coeffs.a1 + coeffs.a2);
    EXPECT_NEAR (1.0, std::abs (hfGain), tolerance);
}

//==============================================================================
// Edge Cases and Stability Tests
//==============================================================================
TEST_F (FilterDesignerTests, HandlesNyquistFrequency)
{
    // Should handle frequency at Nyquist without issues
    auto coeffs = FilterDesigner<double>::designRbjLowpass (nyquist, qFactor, sampleRate);

    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));
}

TEST_F (FilterDesignerTests, HandlesLowFrequencies)
{
    // Should handle very low frequencies
    auto coeffs = FilterDesigner<double>::designRbjLowpass (10.0, qFactor, sampleRate);

    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));
}

TEST_F (FilterDesignerTests, HandlesHighQValues)
{
    // Should handle high Q values without instability
    auto coeffs = FilterDesigner<double>::designRbjLowpass (frequency, 10.0, sampleRate);

    EXPECT_TRUE (std::isfinite (coeffs.b0));
    EXPECT_TRUE (std::isfinite (coeffs.b1));
    EXPECT_TRUE (std::isfinite (coeffs.b2));
    EXPECT_TRUE (std::isfinite (coeffs.a1));
    EXPECT_TRUE (std::isfinite (coeffs.a2));

    // Check stability: roots of 1 + a1*z^-1 + a2*z^-2 should be inside unit circle
    // This is satisfied if |a2| < 1 and |a1| < 1 + a2
    EXPECT_LT (std::abs (coeffs.a2), 1.0);
    EXPECT_LT (std::abs (coeffs.a1), 1.0 + coeffs.a2);
}

TEST_F (FilterDesignerTests, FloatPrecisionConsistency)
{
    // Test that float and double versions produce similar results
    auto doubleCoeffs = FilterDesigner<double>::designRbjLowpass (frequency, qFactor, sampleRate);
    auto floatCoeffs = FilterDesigner<float>::designRbjLowpass (static_cast<float> (frequency),
                                                                static_cast<float> (qFactor),
                                                                sampleRate);

    EXPECT_NEAR (doubleCoeffs.b0, static_cast<double> (floatCoeffs.b0), toleranceF);
    EXPECT_NEAR (doubleCoeffs.b1, static_cast<double> (floatCoeffs.b1), toleranceF);
    EXPECT_NEAR (doubleCoeffs.b2, static_cast<double> (floatCoeffs.b2), toleranceF);
    EXPECT_NEAR (doubleCoeffs.a1, static_cast<double> (floatCoeffs.a1), toleranceF);
    EXPECT_NEAR (doubleCoeffs.a2, static_cast<double> (floatCoeffs.a2), toleranceF);
}
