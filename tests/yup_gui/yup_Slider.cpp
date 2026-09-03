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
TEST_F (SliderTest, DefaultInitialization)
{
    EXPECT_DOUBLE_EQ (0.0, slider->getValue());
    EXPECT_DOUBLE_EQ (0.0, slider->getMinValue());
    EXPECT_DOUBLE_EQ (1.0, slider->getMaxValue());
    EXPECT_DOUBLE_EQ (0.0, slider->getInterval());
    EXPECT_DOUBLE_EQ (1.0, slider->getSkewFactor());
    EXPECT_EQ (Slider::LinearVertical, slider->getSliderType());
    EXPECT_DOUBLE_EQ (1.0, slider->getMouseDragSensitivity());
}

//==============================================================================
TEST_F (SliderTest, ValueOperations)
{
    slider->setRange (0.0, 10.0);

    slider->setValue (5.0);
    EXPECT_DOUBLE_EQ (5.0, slider->getValue());

    slider->setValue (15.0);
    EXPECT_DOUBLE_EQ (10.0, slider->getValue());

    slider->setValue (-5.0);
    EXPECT_DOUBLE_EQ (0.0, slider->getValue());
}

TEST_F (SliderTest, SetValueWithNotificationCallback)
{
    slider->setRange (0.0, 10.0);

    int callCount = 0;
    double lastValue = 0.0;
    slider->onValueChanged = [&] (double v)
    {
        ++callCount;
        lastValue = v;
    };

    slider->setValue (5.0, sendNotification);
    EXPECT_EQ (1, callCount);
    EXPECT_DOUBLE_EQ (5.0, lastValue);
}

TEST_F (SliderTest, SetValueWithoutNotification)
{
    slider->setRange (0.0, 10.0);

    int callCount = 0;
    slider->onValueChanged = [&callCount] (double)
    {
        ++callCount;
    };

    slider->setValue (5.0, dontSendNotification);
    EXPECT_EQ (0, callCount);
}

//==============================================================================
TEST_F (SliderTest, ValueNormalisedOperations)
{
    slider->setRange (10.0, 50.0);

    slider->setValueNormalised (0.0);
    EXPECT_DOUBLE_EQ (10.0, slider->getValue());

    slider->setValueNormalised (1.0);
    EXPECT_DOUBLE_EQ (50.0, slider->getValue());

    slider->setValueNormalised (0.5);
    EXPECT_DOUBLE_EQ (30.0, slider->getValue());

    slider->setValue (30.0);
    EXPECT_NEAR (0.5, slider->getValueNormalised(), tolerance);
}

TEST_F (SliderTest, ValueNormalisedClamped)
{
    slider->setRange (0.0, 100.0);

    slider->setValueNormalised (-0.5);
    EXPECT_DOUBLE_EQ (0.0, slider->getValue());

    slider->setValueNormalised (1.5);
    EXPECT_DOUBLE_EQ (100.0, slider->getValue());
}

//==============================================================================
TEST_F (SliderTest, RangeOperations)
{
    slider->setRange (1.0, 100.0);
    EXPECT_DOUBLE_EQ (1.0, slider->getRange().getRange().getStart());
    EXPECT_DOUBLE_EQ (100.0, slider->getRange().getRange().getEnd());

    slider->setRange (50.0, 50.1);
    EXPECT_DOUBLE_EQ (50.0, slider->getRange().getRange().getStart());
    EXPECT_DOUBLE_EQ (50.1, slider->getRange().getRange().getEnd());
    EXPECT_DOUBLE_EQ (50.0, slider->getValue());
}

TEST_F (SliderTest, RangeWithStepSize)
{
    slider->setRange (0.0, 10.0, 2.0);
    EXPECT_DOUBLE_EQ (2.0, slider->getInterval());

    slider->setRange (0.0, 100.0);
    EXPECT_DOUBLE_EQ (0.0, slider->getInterval());
}

TEST_F (SliderTest, SetRangeWithNormalisableRange)
{
    NormalisableRange<double> range (1.0, 100.0, 0.5);

    slider->setRange (range);
    EXPECT_DOUBLE_EQ (1.0, slider->getRange().getRange().getStart());
    EXPECT_DOUBLE_EQ (100.0, slider->getRange().getRange().getEnd());
    EXPECT_DOUBLE_EQ (0.5, slider->getInterval());
}

//==============================================================================
TEST_F (SliderTest, MinValueOperations)
{
    slider->setRange (0.0, 100.0);

    slider->setMinValue (20.0);
    EXPECT_DOUBLE_EQ (20.0, slider->getMinValue());

    slider->setMinValue (50.0);
    EXPECT_DOUBLE_EQ (50.0, slider->getMinValue());
}

TEST_F (SliderTest, MaxValueOperations)
{
    slider->setRange (0.0, 100.0);

    slider->setMaxValue (80.0);
    EXPECT_DOUBLE_EQ (80.0, slider->getMaxValue());
}

TEST_F (SliderTest, MinValueCallback)
{
    slider->setRange (0.0, 100.0);

    int callCount = 0;
    slider->onMinValueChanged = [&] (double)
    {
        ++callCount;
    };

    slider->setMinValue (20.0, sendNotification);
    EXPECT_EQ (1, callCount);

    slider->setMinValue (30.0, dontSendNotification);
    EXPECT_EQ (1, callCount);
}

TEST_F (SliderTest, MaxValueCallback)
{
    slider->setRange (0.0, 100.0);

    int callCount = 0;
    slider->onMaxValueChanged = [&] (double)
    {
        ++callCount;
    };

    slider->setMaxValue (80.0, sendNotification);
    EXPECT_EQ (1, callCount);

    slider->setMaxValue (75.0, dontSendNotification);
    EXPECT_EQ (1, callCount);
}

//==============================================================================
TEST_F (SliderTest, DefaultValueOperations)
{
    slider->setRange (0.0, 100.0);

    slider->setDefaultValue (42.0);
    EXPECT_DOUBLE_EQ (42.0, slider->getDefaultValue());

    slider->setDefaultValue (0.0);
    EXPECT_DOUBLE_EQ (0.0, slider->getDefaultValue());
}

//==============================================================================
TEST_F (SliderTest, IntervalOperationsWithStepRange)
{
    slider->setRange (0.0, 10.0, 0.5);

    EXPECT_DOUBLE_EQ (0.5, slider->getInterval());

    slider->setRange (0.0, 10.0, 0.01);
    EXPECT_DOUBLE_EQ (0.01, slider->getInterval());
}

//==============================================================================
TEST_F (SliderTest, SkewFactorOperations)
{
    slider->setRange (1.0, 100.0);

    slider->setSkewFactor (2.0);
    EXPECT_DOUBLE_EQ (2.0, slider->getSkewFactor());

    slider->setSkewFactor (1.0);
    EXPECT_DOUBLE_EQ (1.0, slider->getSkewFactor());

    slider->setSkewFactor (0.5);
    EXPECT_DOUBLE_EQ (0.5, slider->getSkewFactor());

    slider->setSkewFactor (3.0);
    EXPECT_DOUBLE_EQ (3.0, slider->getSkewFactor());
}

TEST_F (SliderTest, SkewFactorFromMidpoint)
{
    slider->setRange (1.0, 1000.0);

    slider->setSkewFactorFromMidpoint (100.0);
    double skewFactor = slider->getSkewFactor();
    EXPECT_GT (skewFactor, 0.0);
    EXPECT_NE (1.0, skewFactor);
}

TEST_F (SliderTest, SkewFactorConsistency)
{
    slider->setRange (1.0, 1000.0);
    slider->setSkewFactor (2.0);

    slider->setValueNormalised (0.5);
    double midValue = slider->getValue();
    double normalizedBack = slider->getValueNormalised();

    EXPECT_NEAR (0.5, normalizedBack, tolerance);

    const std::array<double, 5> testValues = { 0.0, 0.25, 0.5, 0.75, 1.0 };
    for (double testNormalized : testValues)
    {
        slider->setValueNormalised (testNormalized);
        EXPECT_NEAR (testNormalized, slider->getValueNormalised(), tolerance);
    }
}

//==============================================================================
TEST_F (SliderTest, NumDecimalPlacesToDisplay)
{
    slider->setNumDecimalPlacesToDisplay (3);
    EXPECT_EQ (3, slider->getNumDecimalPlacesToDisplay());

    slider->setNumDecimalPlacesToDisplay (0);
    EXPECT_EQ (0, slider->getNumDecimalPlacesToDisplay());

    slider->setNumDecimalPlacesToDisplay (-1);
    EXPECT_EQ (-1, slider->getNumDecimalPlacesToDisplay());
}

//==============================================================================
TEST_F (SliderTest, SliderTypeOperations)
{
    slider->setSliderType (Slider::Rotary);
    EXPECT_EQ (Slider::Rotary, slider->getSliderType());

    slider->setSliderType (Slider::LinearHorizontal);
    EXPECT_EQ (Slider::LinearHorizontal, slider->getSliderType());

    slider->setSliderType (Slider::IncDecButtons);
    EXPECT_EQ (Slider::IncDecButtons, slider->getSliderType());

    slider->setSliderType (Slider::TwoValueHorizontal);
    EXPECT_EQ (Slider::TwoValueHorizontal, slider->getSliderType());
}

//==============================================================================
TEST_F (SliderTest, TextBoxStyleOperations)
{
    slider->setTextBoxStyle (Slider::TextBoxLeft, true, 100, 25);
    EXPECT_EQ (Slider::TextBoxLeft, slider->getTextBoxPosition());
    EXPECT_TRUE (slider->isTextBoxReadOnly());

    slider->setTextBoxStyle (Slider::TextBoxBelow, false);
    EXPECT_EQ (Slider::TextBoxBelow, slider->getTextBoxPosition());
    EXPECT_FALSE (slider->isTextBoxReadOnly());
}

TEST_F (SliderTest, TextBoxDefaultValues)
{
    EXPECT_EQ (Slider::NoTextBox, slider->getTextBoxPosition());
    EXPECT_FALSE (slider->isTextBoxReadOnly());
}

//==============================================================================
TEST_F (SliderTest, MouseDragSensitivity)
{
    slider->setMouseDragSensitivity (2.5);
    EXPECT_DOUBLE_EQ (2.5, slider->getMouseDragSensitivity());

    slider->setMouseDragSensitivity (0.1);
    EXPECT_DOUBLE_EQ (0.1, slider->getMouseDragSensitivity());
}

//==============================================================================
TEST_F (SliderTest, VelocityModeParameters)
{
    slider->setVelocityModeParameters (2.0, 0.5, 0.1);
    EXPECT_NO_THROW (slider->setVelocityModeParameters (1.0, 1.0, 0.0));
}

//==============================================================================
TEST_F (SliderTest, IsMouseOverDefaultFalse)
{
    EXPECT_FALSE (slider->isMouseOver());
}

TEST_F (SliderTest, MouseUpOutsideBoundsClearsMouseOver)
{
    slider->mouseEnter (MouseEvent (MouseEvent::noButtons, KeyModifiers(), Point<float> (100.0f, 15.0f)));
    ASSERT_TRUE (slider->isMouseOver());

    slider->mouseDown (MouseEvent (MouseEvent::leftButton, KeyModifiers(), Point<float> (100.0f, 15.0f)));
    slider->mouseUp (MouseEvent (MouseEvent::noButtons, KeyModifiers(), Point<float> (-50.0f, -50.0f)));

    EXPECT_FALSE (slider->isMouseOver());
}

TEST_F (SliderTest, IsCurrentlyBeingDraggedDefaultFalse)
{
    EXPECT_FALSE (slider->isCurrentlyBeingDragged());
}

//==============================================================================
TEST_F (SliderTest, DragStartCallback)
{
    bool called = false;
    slider->onDragStart = [&] (const MouseEvent&)
    {
        called = true;
    };

    EXPECT_FALSE (called);
}

TEST_F (SliderTest, DragEndCallback)
{
    bool called = false;
    slider->onDragEnd = [&] (const MouseEvent&)
    {
        called = true;
    };

    EXPECT_FALSE (called);
}

//==============================================================================
TEST_F (SliderTest, SetPopupDisplayEnabled)
{
    slider->setPopupDisplayEnabled (true);
    EXPECT_NO_THROW (slider->setPopupDisplayEnabled (false));
}

TEST_F (SliderTest, SetPopupMenuEnabled)
{
    slider->setPopupMenuEnabled (true);
    EXPECT_NO_THROW (slider->setPopupMenuEnabled (false));
}

//==============================================================================
TEST_F (SliderTest, ConstructWithDifferentTypes)
{
    Slider horizontal (Slider::LinearHorizontal);
    EXPECT_EQ (Slider::LinearHorizontal, horizontal.getSliderType());

    Slider rotary (Slider::Rotary);
    EXPECT_EQ (Slider::Rotary, rotary.getSliderType());

    Slider incDec (Slider::IncDecButtons);
    EXPECT_EQ (Slider::IncDecButtons, incDec.getSliderType());
}

TEST_F (SliderTest, ConstructWithComponentId)
{
    Slider sliderWithId (Slider::LinearHorizontal, "mySlider");
    EXPECT_EQ (Slider::LinearHorizontal, sliderWithId.getSliderType());
    EXPECT_EQ (String ("mySlider"), sliderWithId.getComponentID());
}

//==============================================================================
TEST_F (SliderTest, EdgeCasesVerySmallRange)
{
    slider->setRange (0.001, 0.002);
    slider->setValue (0.0015);
    EXPECT_NEAR (0.0015, slider->getValue(), 1e-9);
}

TEST_F (SliderTest, EdgeCasesVeryLargeRange)
{
    slider->setRange (-1000000.0, 1000000.0);
    slider->setValue (500000.0);
    EXPECT_DOUBLE_EQ (500000.0, slider->getValue());
}

TEST_F (SliderTest, EdgeCasesNegativeRange)
{
    slider->setRange (-100.0, -10.0);
    slider->setValue (-50.0);
    EXPECT_DOUBLE_EQ (-50.0, slider->getValue());
}

TEST_F (SliderTest, EdgeCasesStepSnapping)
{
    slider->setRange (0.0, 1.0, 0.01);
    slider->setValue (0.567);
    EXPECT_NEAR (0.57, slider->getValue(), tolerance);

    slider->setValue (0.561);
    EXPECT_NEAR (0.56, slider->getValue(), tolerance);
}
