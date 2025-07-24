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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{
constexpr double tolerance = 1e-6;
} // namespace

class SliderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        slider = std::make_unique<Slider> (Slider::LinearVertical, "testSlider");
        slider->setBounds (0, 0, 200, 30);
    }

    std::unique_ptr<Slider> slider;
};

//==============================================================================
/*
TEST_F (SliderTest, DefaultInitialization)
{
    EXPECT_DOUBLE_EQ (0.0, slider->getValue());
    EXPECT_DOUBLE_EQ (0.0, slider->getMinimum());
    EXPECT_DOUBLE_EQ (10.0, slider->getMaximum());
    EXPECT_DOUBLE_EQ (0.0, slider->getInterval());
    EXPECT_DOUBLE_EQ (1.0, slider->getSkewFactor());
}
*/

TEST_F (SliderTest, ValueOperations)
{
    // Set range first before testing values
    slider->setRange (0.0, 10.0);

    // Test setting and getting values
    slider->setValue (5.0);
    EXPECT_DOUBLE_EQ (5.0, slider->getValue());

    // Test value clamping to range
    slider->setValue (15.0);
    EXPECT_DOUBLE_EQ (10.0, slider->getValue());

    slider->setValue (-5.0);
    EXPECT_DOUBLE_EQ (0.0, slider->getValue());
}

/*
TEST_F (SliderTest, RangeOperations)
{
    // Test setting range
    slider->setRange (1.0, 100.0);
    EXPECT_DOUBLE_EQ (1.0, slider->getMinimum());
    EXPECT_DOUBLE_EQ (100.0, slider->getMaximum());

    // Test invalid range (min > max)
    slider->setRange (100.0, 1.0);
    EXPECT_DOUBLE_EQ (1.0, slider->getMinimum());
    EXPECT_DOUBLE_EQ (100.0, slider->getMaximum());

    // Test equal min and max
    slider->setRange (50.0, 50.0);
    EXPECT_DOUBLE_EQ (50.0, slider->getMinimum());
    EXPECT_DOUBLE_EQ (50.0, slider->getMaximum());
    EXPECT_DOUBLE_EQ (50.0, slider->getValue()); // Value should be set to the single valid value
}

TEST_F (SliderTest, IntervalOperations)
{
    slider->setRange (0.0, 10.0);

    // Test setting interval
    slider->setInterval (0.5);
    EXPECT_DOUBLE_EQ (0.5, slider->getInterval());

    // Test value snapping to interval
    slider->setValue (3.7);
    EXPECT_NEAR (3.5, slider->getValue(), tolerance); // Should snap to nearest 0.5

    slider->setValue (4.8);
    EXPECT_NEAR (5.0, slider->getValue(), tolerance); // Should snap to nearest 0.5

    // Test zero interval (continuous)
    slider->setInterval (0.0);
    slider->setValue (3.7);
    EXPECT_DOUBLE_EQ (3.7, slider->getValue()); // Should not snap
}
*/

TEST_F (SliderTest, SkewFactorOperations)
{
    slider->setRange (1.0, 100.0);

    // Test setting skew factor
    slider->setSkewFactor (2.0);
    EXPECT_DOUBLE_EQ (2.0, slider->getSkewFactor());

    // Test linear skew (default)
    slider->setSkewFactor (1.0);
    EXPECT_DOUBLE_EQ (1.0, slider->getSkewFactor());

    // The actual skewing behavior would be tested through the slider's
    // internal position-to-value and value-to-position conversions

    // Test logarithmic-like skew (< 1.0)
    slider->setSkewFactor (0.5);
    EXPECT_DOUBLE_EQ (0.5, slider->getSkewFactor());

    // Test exponential-like skew (> 1.0)
    slider->setSkewFactor (3.0);
    EXPECT_DOUBLE_EQ (3.0, slider->getSkewFactor());

    // Test invalid skew factor (should be > 0)
#if ! YUP_DEBUG
    //slider->setSkewFactor (0.0);
    //EXPECT_GT (slider->getSkewFactor(), 0.0); // Should not be zero

    //slider->setSkewFactor (-1.0);
    //EXPECT_GT (slider->getSkewFactor(), 0.0); // Should not be negative
#endif
}

TEST_F (SliderTest, SkewFactorFromMidpoint)
{
    slider->setRange (1.0, 1000.0);

    // Test setting skew from midpoint (useful for frequency controls)
    slider->setSkewFactorFromMidpoint (100.0);

    // The skew factor should be calculated to make 100 appear at the midpoint
    double skewFactor = slider->getSkewFactor();
    EXPECT_GT (skewFactor, 0.0);
    EXPECT_NE (1.0, skewFactor); // Should not be linear

    // Test with midpoint at geometric center
    slider->setRange (1.0, 100.0);
    slider->setSkewFactorFromMidpoint (10.0); // sqrt(1 * 100) = 10

    // Test edge cases
#if ! YUP_DEBUG
    //slider->setSkewFactorFromMidpoint (1.0); // Midpoint at minimum
    //EXPECT_GT (slider->getSkewFactor(), 0.0);

    //slider->setSkewFactorFromMidpoint (100.0); // Midpoint at maximum
    //EXPECT_GT (slider->getSkewFactor(), 0.0);
#endif
}

/*
TEST_F (SliderTest, NormalizedValue)
{
    slider->setRange (10.0, 50.0);

    // Test normalized value calculation
    slider->setValue (10.0); // Minimum
    EXPECT_NEAR (0.0, slider->getProportionalValue(), tolerance);

    slider->setValue (50.0); // Maximum
    EXPECT_NEAR (1.0, slider->getProportionalValue(), tolerance);

    slider->setValue (30.0); // Middle
    EXPECT_NEAR (0.5, slider->getProportionalValue(), tolerance);

    // Test setting from normalized value
    slider->setProportionalValue (0.25);
    EXPECT_NEAR (20.0, slider->getValue(), tolerance);

    slider->setProportionalValue (0.75);
    EXPECT_NEAR (40.0, slider->getValue(), tolerance);
}

TEST_F (SliderTest, SkewFactorAffectsNormalizedValue)
{
    slider->setRange (1.0, 100.0);

    // With linear skew (1.0)
    slider->setSkewFactor (1.0);
    slider->setValue (50.5); // Roughly middle value
    double linearNormalized = slider->getProportionalValue();

    // With exponential skew (> 1.0)
    slider->setSkewFactor (2.0);
    slider->setValue (50.5); // Same value
    double exponentialNormalized = slider->getProportionalValue();

    // The normalized values should be different due to skewing
    EXPECT_NE (linearNormalized, exponentialNormalized);

    // With logarithmic skew (< 1.0)
    slider->setSkewFactor (0.5);
    slider->setValue (50.5); // Same value
    double logarithmicNormalized = slider->getProportionalValue();

    // Should be different from both linear and exponential
    EXPECT_NE (linearNormalized, logarithmicNormalized);
    EXPECT_NE (exponentialNormalized, logarithmicNormalized);
}

TEST_F (SliderTest, TextFormattingOptions)
{
    // Test suffix
    slider->setTextValueSuffix (" Hz");
    EXPECT_EQ (" Hz", slider->getTextValueSuffix());

    // Test text from value function
    slider->setRange (0.0, 100.0);
    slider->setValue (50.0);

    String valueText = slider->getTextFromValue (50.0);
    EXPECT_TRUE (valueText.contains ("50"));

    // Test value from text function
    double parsedValue = slider->getValueFromText ("75.5");
    EXPECT_NEAR (75.5, parsedValue, tolerance);
}

TEST_F (SliderTest, BehaviorWithDifferentSkewFactors)
{
    slider->setRange (20.0, 20000.0); // Frequency-like range

    // Test with different skew factors for frequency response
    std::vector<double> skewFactors = { 0.3, 0.5, 1.0, 2.0, 3.0 };

    for (double skew : skewFactors)
    {
        slider->setSkewFactor (skew);
        EXPECT_DOUBLE_EQ (skew, slider->getSkewFactor());

        // Test that extreme values still work
        slider->setValue (20.0);
        EXPECT_DOUBLE_EQ (20.0, slider->getValue());

        slider->setValue (20000.0);
        EXPECT_DOUBLE_EQ (20000.0, slider->getValue());

        // Test normalized values at extremes
        EXPECT_NEAR (0.0, slider->getProportionalValue(), tolerance);

        slider->setValue (20.0);
        EXPECT_NEAR (0.0, slider->getProportionalValue(), tolerance);
    }
}

TEST_F (SliderTest, IntervalWithSkew)
{
    slider->setRange (1.0, 100.0);
    slider->setInterval (1.0);   // Integer values only
    slider->setSkewFactor (2.0); // Exponential skew

    // Test that values still snap to intervals even with skew
    slider->setValue (25.7);
    double snappedValue = slider->getValue();
    EXPECT_EQ (snappedValue, std::round (snappedValue)); // Should be integer

    // Test edge case combinations
    slider->setSkewFactor (0.5); // Logarithmic skew
    slider->setValue (75.3);
    snappedValue = slider->getValue();
    EXPECT_EQ (snappedValue, std::round (snappedValue)); // Should still be integer
}

TEST_F (SliderTest, EdgeCases)
{
    // Test very small range
    slider->setRange (0.001, 0.002);
    slider->setValue (0.0015);
    EXPECT_NEAR (0.0015, slider->getValue(), 1e-9);

    // Test very large range
    slider->setRange (-1000000.0, 1000000.0);
    slider->setValue (500000.0);
    EXPECT_DOUBLE_EQ (500000.0, slider->getValue());

    // Test negative range
    slider->setRange (-100.0, -10.0);
    slider->setValue (-50.0);
    EXPECT_DOUBLE_EQ (-50.0, slider->getValue());

    // Test fractional interval
    slider->setRange (0.0, 1.0);
    slider->setInterval (0.01); // 1% steps
    slider->setValue (0.567);
    EXPECT_NEAR (0.57, slider->getValue(), tolerance); // Should snap to 0.57
}

TEST_F (SliderTest, SkewFactorConsistency)
{
    slider->setRange (1.0, 1000.0);

    // Test that skew factor produces consistent results
    slider->setSkewFactor (2.0);

    // Set a normalized value, then get it back
    slider->setProportionalValue (0.5);
    double midValue = slider->getValue();
    double normalizedBack = slider->getProportionalValue();

    EXPECT_NEAR (0.5, normalizedBack, tolerance);

    // Test roundtrip consistency for various values
    std::vector<double> testValues = { 0.0, 0.25, 0.5, 0.75, 1.0 };

    for (double testNormalized : testValues)
    {
        slider->setProportionalValue (testNormalized);
        double actualNormalized = slider->getProportionalValue();
        EXPECT_NEAR (testNormalized, actualNormalized, tolerance);
    }
}
*/
