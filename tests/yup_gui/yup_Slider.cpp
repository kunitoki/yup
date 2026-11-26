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

TEST_F (SliderTest, RangePropertiesKeepTheRangeValid)
{
    slider->setProperty (Identifier ("minimum"), 20.0);
    EXPECT_DOUBLE_EQ (20.0, slider->getRange().start);
    EXPECT_DOUBLE_EQ (21.0, slider->getRange().end);

    slider->setProperty (Identifier ("maximum"), 20000.0);
    EXPECT_DOUBLE_EQ (20.0, slider->getRange().start);
    EXPECT_DOUBLE_EQ (20000.0, slider->getRange().end);

    slider->setProperty (Identifier ("maximum"), 10.0);
    EXPECT_DOUBLE_EQ (-19970.0, slider->getRange().start);
    EXPECT_DOUBLE_EQ (10.0, slider->getRange().end);

    EXPECT_DOUBLE_EQ (-19970.0, static_cast<double> (slider->getProperty (Identifier ("minimum"))));
    EXPECT_DOUBLE_EQ (10.0, static_cast<double> (slider->getProperty (Identifier ("maximum"))));

    slider->setProperty (Identifier ("minimum"), -100.0);
    EXPECT_DOUBLE_EQ (-100.0, slider->getRange().start);
    EXPECT_DOUBLE_EQ (10.0, slider->getRange().end);
}

TEST_F (SliderTest, RangePropertiesIgnoreInvalidValues)
{
    slider->setProperty (Identifier ("minimum"), std::numeric_limits<double>::infinity());
    slider->setProperty (Identifier ("maximum"), std::numeric_limits<double>::quiet_NaN());
    EXPECT_DOUBLE_EQ (0.0, slider->getRange().start);
    EXPECT_DOUBLE_EQ (1.0, slider->getRange().end);

    slider->setProperty (Identifier ("skew"), 0.0);
    slider->setProperty (Identifier ("interval"), -1.0);
    EXPECT_DOUBLE_EQ (1.0, slider->getSkewFactor());
    EXPECT_DOUBLE_EQ (0.0, slider->getInterval());

    slider->setProperty (Identifier ("skew"), 0.5);
    slider->setProperty (Identifier ("interval"), 0.25);
    slider->setProperty (Identifier ("defaultValue"), 0.5);
    EXPECT_DOUBLE_EQ (0.5, slider->getSkewFactor());
    EXPECT_DOUBLE_EQ (0.25, slider->getInterval());
    EXPECT_DOUBLE_EQ (0.5, slider->getDefaultValue());
}

TEST_F (SliderTest, NotifiesRangePropertyChanges)
{
    struct RangeListener : ComponentListener
    {
        void componentPropertyChanged (Component&, const Identifier& name) override { names.add (name.toString()); }

        StringArray names;
    };

    RangeListener listener;
    slider->addComponentListener (&listener);

    slider->setRange (0.0, 10.0);
    EXPECT_TRUE (listener.names.contains ("maximum"));
    EXPECT_FALSE (listener.names.contains ("minimum"));

    listener.names.clear();
    slider->setSkewFactor (0.5);
    EXPECT_TRUE (listener.names.contains ("skew"));

    listener.names.clear();
    slider->setRange (5.0, 10.0, 0.5);
    EXPECT_TRUE (listener.names.contains ("minimum"));
    EXPECT_TRUE (listener.names.contains ("interval"));
    EXPECT_FALSE (listener.names.contains ("maximum"));

    listener.names.clear();
    slider->setSkewFactorFromMidpoint (6.0);
    EXPECT_TRUE (listener.names.contains ("skew"));

    listener.names.clear();
    slider->setDefaultValue (7.0);
    slider->setDefaultValue (7.0);
    EXPECT_EQ (listener.names, StringArray ("defaultValue"));

    slider->removeComponentListener (&listener);
}

TEST_F (SliderTest, EdgeCasesStepSnapping)
{
    slider->setRange (0.0, 1.0, 0.01);
    slider->setValue (0.567);
    EXPECT_NEAR (0.57, slider->getValue(), tolerance);

    slider->setValue (0.561);
    EXPECT_NEAR (0.56, slider->getValue(), tolerance);
}

TEST_F (SliderTest, AnEmptyOrReversedRangeIsRefused)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "A range that is empty or reversed triggers an assertion when assertions are enabled or logged";
#endif

    slider->setRange (0.0, 10.0);
    slider->setValue (5.0);

    // With no length the range would tell its values as NaN
    slider->setRange (5.0, 5.0);
    EXPECT_DOUBLE_EQ (0.0, slider->getRange().start);
    EXPECT_DOUBLE_EQ (10.0, slider->getRange().end);

    slider->setRange (10.0, 0.0);
    EXPECT_DOUBLE_EQ (0.0, slider->getRange().start);
    EXPECT_DOUBLE_EQ (10.0, slider->getRange().end);

    EXPECT_DOUBLE_EQ (5.0, slider->getValue());
    EXPECT_DOUBLE_EQ (0.5, slider->getValueNormalised());
}

//==============================================================================
class SliderStepperTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        panel.setVisible (true);
        panel.setBounds (0.0f, 0.0f, 200.0f, 30.0f);

        stepper.setBounds (0.0f, 0.0f, 200.0f, 30.0f);
        stepper.setRange (0.0, 10.0, 1.0);
        stepper.setValue (5.0);
        panel.addAndMakeVisible (stepper);
    }

    Component panel;
    Slider stepper { Slider::IncDecButtons };
    PointerSimulator pointer { panel };
};

TEST_F (SliderStepperTest, HasAButtonAtEachEndWithTheValueBetweenThem)
{
    const auto decrement = stepper.getDecrementButtonBounds();
    const auto increment = stepper.getIncrementButtonBounds();

    EXPECT_FALSE (decrement.isEmpty());
    EXPECT_FALSE (increment.isEmpty());
    EXPECT_LT (decrement.getRight(), increment.getX());
    EXPECT_TRUE (stepper.getLocalBounds().contains (decrement));
    EXPECT_TRUE (stepper.getLocalBounds().contains (increment));

    // A slider of another type has none
    Slider linear (Slider::LinearHorizontal);
    linear.setBounds (0.0f, 0.0f, 200.0f, 30.0f);
    EXPECT_TRUE (linear.getDecrementButtonBounds().isEmpty());
    EXPECT_TRUE (linear.getIncrementButtonBounds().isEmpty());
}

TEST_F (SliderStepperTest, TwoQuickPressesOnAButtonAreTwoSteps)
{
    stepper.setDefaultValue (2.0);

    // The second press steps as the first did: the double click it makes does not bring the default value back
    pointer.doubleClick (stepper.getIncrementButtonBounds().getCenter());
    EXPECT_DOUBLE_EQ (stepper.getValue(), 7.0);

    pointer.doubleClick (stepper.getDecrementButtonBounds().getCenter());
    EXPECT_DOUBLE_EQ (stepper.getValue(), 5.0);

    // Between the buttons it still does, as on any slider
    pointer.doubleClick (stepper.getLocalBounds().getCenter());
    EXPECT_DOUBLE_EQ (stepper.getValue(), 2.0);
}

TEST_F (SliderStepperTest, KeepsTheThicknessOfAButtonInATallerArea)
{
    stepper.setBounds (0.0f, 0.0f, 200.0f, 120.0f);

    const auto decrement = stepper.getDecrementButtonBounds();
    const auto increment = stepper.getIncrementButtonBounds();

    // The buttons don't grow with the area: they keep their size, in the middle of it
    EXPECT_FLOAT_EQ (decrement.getHeight(), 28.0f);
    EXPECT_FLOAT_EQ (increment.getHeight(), 28.0f);
    EXPECT_FLOAT_EQ (decrement.getCenterY(), 60.0f);
    EXPECT_FLOAT_EQ (increment.getCenterY(), 60.0f);
    EXPECT_LT (decrement.getRight(), increment.getX());

    // One above the other in a narrow area, they keep their width instead
    stepper.setBounds (0.0f, 0.0f, 120.0f, 200.0f);
    EXPECT_FLOAT_EQ (stepper.getIncrementButtonBounds().getWidth(), 28.0f);
    EXPECT_FLOAT_EQ (stepper.getIncrementButtonBounds().getCenterX(), 60.0f);
    EXPECT_LT (stepper.getIncrementButtonBounds().getBottom(), stepper.getDecrementButtonBounds().getY());
}

//==============================================================================
/** A slider of two values under a pointer: each thumb is dragged on its own, and stops at the other one. */
class SliderTwoValueTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        panel.setVisible (true);
        panel.setBounds (0.0f, 0.0f, 220.0f, 30.0f);

        slider.setBounds (0.0f, 0.0f, 220.0f, 30.0f);
        slider.setMinValue (0.25, dontSendNotification);
        slider.setMaxValue (0.75, dontSendNotification);
        panel.addAndMakeVisible (slider);
    }

    Component panel;
    Slider slider { Slider::TwoValueHorizontal };
    PointerSimulator pointer { panel };
};

TEST_F (SliderTwoValueTest, TellsWhereItsValuesAreAndLeavesRoomForItsThumbs)
{
    EXPECT_DOUBLE_EQ (slider.getMinValueNormalised(), 0.25);
    EXPECT_DOUBLE_EQ (slider.getMaxValueNormalised(), 0.75);
    EXPECT_DOUBLE_EQ (static_cast<double> (slider.getProperty (Identifier ("minNormalized"))), 0.25);
    EXPECT_DOUBLE_EQ (static_cast<double> (slider.getProperty (Identifier ("maxNormalized"))), 0.75);

    // A thumb at an end of the range is whole inside the slider
    const auto track = slider.getSliderBounds();
    EXPECT_FLOAT_EQ (track.getX(), 10.0f);
    EXPECT_FLOAT_EQ (track.getRight(), 210.0f);
}

TEST_F (SliderTwoValueTest, EachThumbIsDraggedOnItsOwnAndStopsAtTheOther)
{
    const auto track = slider.getSliderBounds();
    const auto at = [&track] (double normalized)
    {
        return Point<float> (track.getX() + static_cast<float> (normalized) * track.getWidth(), track.getCenterY());
    };

    // The thumb nearest to the pointer follows it
    pointer.drag (at (0.25), at (0.5));
    EXPECT_NEAR (slider.getMinValue(), 0.5, 1.0e-4);
    EXPECT_NEAR (slider.getMaxValue(), 0.75, 1.0e-4);

    // And goes no further than the other one
    pointer.drag (at (0.5), at (1.0));
    EXPECT_NEAR (slider.getMinValue(), 0.75, 1.0e-4);
    EXPECT_NEAR (slider.getMaxValue(), 0.75, 1.0e-4);

    pointer.drag (at (0.75), at (0.9));
    EXPECT_NEAR (slider.getMinValue(), 0.75, 1.0e-4);
    EXPECT_NEAR (slider.getMaxValue(), 0.9, 1.0e-4);
}

TEST_F (SliderTwoValueTest, TellsTheThumbUnderThePointerAndTheOneDragged)
{
    const auto minThumb = slider.getMinThumbBounds();
    const auto maxThumb = slider.getMaxThumbBounds();

    // Each thumb is centered where its value is along the track, and the value itself has none
    EXPECT_FLOAT_EQ (minThumb.getCenterX(), 60.0f);
    EXPECT_FLOAT_EQ (maxThumb.getCenterX(), 160.0f);
    EXPECT_FLOAT_EQ (minThumb.getWidth(), 16.0f);
    EXPECT_TRUE (slider.getThumbBounds().isEmpty());
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::none);

    pointer.moveTo (minThumb.getCenter());
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::minValue);

    pointer.moveTo (maxThumb.getCenter());
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::maxValue);

    pointer.moveTo ({ 110.0f, 15.0f });
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::none);

    // A press between them drags the nearest one, which is the one told until the release
    pointer.moveTo ({ 100.0f, 15.0f });
    pointer.press();
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::minValue);

    pointer.moveTo ({ 120.0f, 15.0f });
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::minValue);
    pointer.release();

    pointer.leave();
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::none);
}

TEST_F (SliderStepperTest, AButtonStepsByTheInterval)
{
    const auto decrement = stepper.getDecrementButtonBounds().getCenter();
    const auto increment = stepper.getIncrementButtonBounds().getCenter();

    pointer.click (increment);
    EXPECT_DOUBLE_EQ (stepper.getValue(), 6.0);

    pointer.click (increment);
    pointer.click (decrement);
    pointer.click (decrement);
    EXPECT_DOUBLE_EQ (stepper.getValue(), 5.0);

    // The ends of the range stop it
    stepper.setValue (10.0);
    pointer.click (increment);
    EXPECT_DOUBLE_EQ (stepper.getValue(), 10.0);

    stepper.setValue (0.0);
    pointer.click (decrement);
    EXPECT_DOUBLE_EQ (stepper.getValue(), 0.0);
}

TEST_F (SliderStepperTest, AButtonStepsByAHundredthOfTheRangeWithNoInterval)
{
    stepper.setRange (0.0, 200.0);
    stepper.setValue (50.0);

    pointer.click (stepper.getIncrementButtonBounds().getCenter());
    EXPECT_NEAR (stepper.getValue(), 52.0, 1.0e-9);
}

TEST_F (SliderStepperTest, TellsWhichButtonIsPressed)
{
    EXPECT_EQ (stepper.getPressedStep(), 0);

    pointer.moveTo (stepper.getIncrementButtonBounds().getCenter());
    pointer.press();
    EXPECT_EQ (stepper.getPressedStep(), 1);
    EXPECT_FALSE (stepper.isCurrentlyBeingDragged());

    pointer.release();
    EXPECT_EQ (stepper.getPressedStep(), 0);

    pointer.moveTo (stepper.getDecrementButtonBounds().getCenter());
    pointer.press();
    EXPECT_EQ (stepper.getPressedStep(), -1);
    pointer.release();
}

#if YUP_MODAL_LOOPS_PERMITTED
TEST_F (SliderStepperTest, AButtonHeldKeepsStepping)
{
    pointer.moveTo (stepper.getIncrementButtonBounds().getCenter());
    pointer.press();
    EXPECT_DOUBLE_EQ (stepper.getValue(), 6.0);

    MessageManager::getInstance()->runDispatchLoopUntil (700);
    EXPECT_GT (stepper.getValue(), 6.0);

    // Released, it stops
    pointer.release();
    const auto reached = stepper.getValue();

    MessageManager::getInstance()->runDispatchLoopUntil (200);
    EXPECT_DOUBLE_EQ (stepper.getValue(), reached);
}

TEST_F (SliderStepperTest, AButtonHeldStopsSteppingWhenTheStepperIsDisabled)
{
    pointer.moveTo (stepper.getIncrementButtonBounds().getCenter());
    pointer.press();
    EXPECT_DOUBLE_EQ (stepper.getValue(), 6.0);

    stepper.setEnabled (false);

    MessageManager::getInstance()->runDispatchLoopUntil (700);
    EXPECT_DOUBLE_EQ (stepper.getValue(), 6.0);

    pointer.release();
}
#endif

TEST_F (SliderStepperTest, AButtonHeldIsReleasedWhenTheStepperIsDisabledOrHidden)
{
    pointer.moveTo (stepper.getIncrementButtonBounds().getCenter());

    pointer.press();
    ASSERT_EQ (stepper.getPressedStep(), 1);

    stepper.setEnabled (false);
    EXPECT_EQ (stepper.getPressedStep(), 0);

    pointer.release();
    stepper.setEnabled (true);

    pointer.press();
    ASSERT_EQ (stepper.getPressedStep(), 1);

    stepper.setVisible (false);
    EXPECT_EQ (stepper.getPressedStep(), 0);

    pointer.release();
}

TEST_F (SliderStepperTest, TheValueIsDraggedUpAndDownBetweenTheButtons)
{
    const auto middle = stepper.getLocalBounds().getCenter();

    pointer.drag (middle, middle.translated (0.0f, -12.0f));
    EXPECT_GT (stepper.getValue(), 5.0);

    const auto raised = stepper.getValue();
    pointer.drag (middle, middle.translated (0.0f, 24.0f));
    EXPECT_LT (stepper.getValue(), raised);
}

//==============================================================================
/** A linear slider under a pointer: its thumb is where the value is along the track, and is what a press grabs. */
class SliderLinearTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        panel.setVisible (true);
        panel.setBounds (0.0f, 0.0f, 200.0f, 40.0f);

        slider.setBounds (0.0f, 0.0f, 200.0f, 40.0f);
        panel.addAndMakeVisible (slider);
    }

    Component panel;
    Slider slider { Slider::LinearHorizontal };
    PointerSimulator pointer { panel };
};

TEST_F (SliderLinearTest, TheThumbIsCenteredWhereTheValueIsAlongTheTrack)
{
    slider.setValue (0.25, dontSendNotification);

    const auto track = slider.getSliderBounds();
    const auto thumb = slider.getThumbBounds();

    EXPECT_FLOAT_EQ (thumb.getCenterX(), track.getX() + 0.25f * track.getWidth());
    EXPECT_FLOAT_EQ (thumb.getCenterY(), track.getCenterY());
    EXPECT_FLOAT_EQ (thumb.getWidth(), 16.0f);
    EXPECT_FLOAT_EQ (thumb.getHeight(), 16.0f);
    EXPECT_EQ (VariantConverter<Rectangle<float>>::fromVar (slider.getProperty (Identifier ("thumbBounds"))), thumb);

    // It is as large as it is styled
    slider.setProperty (Identifier ("thumbSize"), 24.0);
    EXPECT_FLOAT_EQ (slider.getThumbBounds().getWidth(), 24.0f);
    EXPECT_FLOAT_EQ (slider.getThumbBounds().getCenterX(), slider.getSliderBounds().getX() + 0.25f * slider.getSliderBounds().getWidth());

    // On a vertical slider it rises with the value
    Slider vertical (Slider::LinearVertical);
    vertical.setBounds (0.0f, 0.0f, 40.0f, 200.0f);
    vertical.setValue (0.25, dontSendNotification);
    EXPECT_FLOAT_EQ (vertical.getThumbBounds().getCenterX(), vertical.getSliderBounds().getCenterX());
    EXPECT_FLOAT_EQ (vertical.getThumbBounds().getCenterY(), vertical.getSliderBounds().getBottom() - 0.25f * vertical.getSliderBounds().getHeight());

    // A slider that is not linear has no such thumb
    Slider dial (Slider::Rotary);
    dial.setBounds (0.0f, 0.0f, 40.0f, 40.0f);
    EXPECT_TRUE (dial.getThumbBounds().isEmpty());
}

TEST_F (SliderLinearTest, APressOnTheTrackBringsTheThumbUnderThePointer)
{
    // The track goes from 10 to 190: a press at 46 is a fifth of the way along it
    pointer.moveTo ({ 46.0f, 20.0f });
    pointer.press();
    EXPECT_NEAR (slider.getValue(), 0.2, 1.0e-6);
    EXPECT_NEAR (slider.getThumbBounds().getCenterX(), 46.0f, 1.0e-4f);
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::value);

    // And the thumb follows the pointer from there
    pointer.moveTo ({ 100.0f, 20.0f });
    EXPECT_NEAR (slider.getValue(), 0.5, 1.0e-6);
    EXPECT_NEAR (slider.getThumbBounds().getCenterX(), 100.0f, 1.0e-4f);

    pointer.release();
}

TEST_F (SliderLinearTest, TheThumbIsGrabbedByWhatOfItIsOutsideTheTrack)
{
    // At the lowest value the thumb is around the start of the track: from 2 to 18, the track starting at 10
    ASSERT_LT (slider.getThumbBounds().getX(), slider.getSliderBounds().getX());

    pointer.moveTo ({ 5.0f, 20.0f });
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::value);

    pointer.press();
    EXPECT_TRUE (slider.isCurrentlyBeingDragged());

    // Grabbed, it does not jump: it moves as far as the pointer does
    EXPECT_DOUBLE_EQ (slider.getValue(), 0.0);

    pointer.moveTo ({ 95.0f, 20.0f });
    EXPECT_NEAR (slider.getValue(), 0.5, 1.0e-6);
    pointer.release();

    // Beside the track, with the thumb at its other end, a press does nothing
    slider.setValue (1.0, dontSendNotification);

    pointer.moveTo ({ 5.0f, 20.0f });
    EXPECT_EQ (slider.getHoveredThumb(), Slider::Thumb::none);

    pointer.press();
    EXPECT_FALSE (slider.isCurrentlyBeingDragged());
    EXPECT_DOUBLE_EQ (slider.getValue(), 1.0);
    pointer.release();
}

TEST_F (SliderLinearTest, APressOnATrackNoLongerThanTheThumbGoesTowardsThePointer)
{
    // 36 wide, the slider has 16 of track from 10 to 26, less than its thumb is wide
    slider.setBounds (0.0f, 0.0f, 36.0f, 40.0f);
    slider.setValue (1.0, dontSendNotification);

    // Near the start of the track and above the thumb: the value comes down to where the pointer is
    pointer.click ({ 12.0f, 4.0f });
    EXPECT_NEAR (slider.getValue(), 0.125, 1.0e-6);

    // 40 wide, the track is as long as the thumb the slider leaves room for
    slider.setBounds (0.0f, 0.0f, 40.0f, 40.0f);
    slider.setValue (1.0, dontSendNotification);

    pointer.click ({ 20.0f, 4.0f });
    EXPECT_NEAR (slider.getValue(), 0.5, 1.0e-6);
}

TEST_F (SliderLinearTest, ADragEndsWhenTheSliderIsHiddenDisabledOrChangesType)
{
    int numDragEnds = 0;
    slider.onDragEnd = [&numDragEnds] (const MouseEvent&)
    {
        ++numDragEnds;
    };

    pointer.moveTo ({ 100.0f, 20.0f });

    pointer.press();
    ASSERT_TRUE (slider.isCurrentlyBeingDragged());

    slider.setVisible (false);
    EXPECT_FALSE (slider.isCurrentlyBeingDragged());
    EXPECT_EQ (numDragEnds, 1);

    // The release that follows has nothing left to end
    pointer.release();
    EXPECT_EQ (numDragEnds, 1);

    slider.setVisible (true);

    pointer.press();
    ASSERT_TRUE (slider.isCurrentlyBeingDragged());

    slider.setEnabled (false);
    EXPECT_FALSE (slider.isCurrentlyBeingDragged());
    EXPECT_EQ (numDragEnds, 2);

    pointer.release();
    slider.setEnabled (true);

    pointer.press();
    ASSERT_TRUE (slider.isCurrentlyBeingDragged());

    slider.setSliderType (Slider::LinearBarHorizontal);
    EXPECT_FALSE (slider.isCurrentlyBeingDragged());
    EXPECT_EQ (numDragEnds, 3);

    pointer.release();
    EXPECT_EQ (numDragEnds, 3);
}

//==============================================================================
TEST_F (SliderTest, TheTrackLeavesRoomForTheThumbAsLargeAsItIsStyled)
{
    Slider horizontal (Slider::LinearHorizontal);
    horizontal.setBounds (0, 0, 200, 40);

    // The thumb is drawn around both ends of the track: with the size it has by default the track is where it was
    EXPECT_FLOAT_EQ (horizontal.getSliderBounds().getX(), 10.0f);
    EXPECT_FLOAT_EQ (horizontal.getSliderBounds().getRight(), 190.0f);

    // A larger one takes the track further in, so it stays inside the slider at both ends
    horizontal.setProperty (Identifier ("thumbSize"), 30.0);
    EXPECT_FLOAT_EQ (horizontal.getSliderBounds().getX(), 15.0f);
    EXPECT_FLOAT_EQ (horizontal.getSliderBounds().getRight(), 185.0f);

    Slider vertical (Slider::LinearVertical);
    vertical.setBounds (0, 0, 40, 200);
    vertical.setProperty (Identifier ("thumbSize"), 30.0);
    EXPECT_FLOAT_EQ (vertical.getSliderBounds().getY(), 15.0f);
    EXPECT_FLOAT_EQ (vertical.getSliderBounds().getBottom(), 185.0f);

    // A slider too low for the thumb it is drawn with keeps it inside its ends too
    Slider low (Slider::LinearHorizontal);
    low.setBounds (0, 0, 200, 10);
    EXPECT_FLOAT_EQ (low.getSliderBounds().getX(), 8.0f);
    EXPECT_FLOAT_EQ (low.getSliderBounds().getRight(), 192.0f);
}

//==============================================================================
class SliderGpuTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});
        if (gpuContext == nullptr)
            return;

        if (GpuCanvas::create (*gpuContext, 64, 64) == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";

        oldTheme = ApplicationTheme::getGlobalTheme();
        ApplicationTheme::setGlobalTheme (createThemeVersion1());
    }

    void TearDown() override
    {
        if (gpuContext != nullptr)
            ApplicationTheme::setGlobalTheme (oldTheme.get());

        oldTheme = nullptr;
    }

    static std::unique_ptr<GraphicsContext> gpuContext;
    ApplicationTheme::Ptr oldTheme;
};

std::unique_ptr<GraphicsContext> SliderGpuTest::gpuContext;

TEST_F (SliderGpuTest, ARotaryDialIsRoundInBoundsThatAreNotSquare)
{
    const auto paints = [] (float width, float height, int x, int y)
    {
        Slider dial (Slider::RotaryVerticalDrag);
        dial.setBounds (0.0f, 0.0f, width, height);
        dial.setValue (0.5, dontSendNotification);

        const auto image = dial.snapshotToImage (*gpuContext);
        EXPECT_TRUE (image.isValid());

        // The corner of a dial is never painted
        return image.isValid() && image.getPixelColor (x, y) != image.getPixelColor (0, 0);
    };

    // Wider than high: the dial is the circle in the middle, as high as the slider, and paints nothing beside it
    EXPECT_TRUE (paints (96.0f, 48.0f, 48, 24));
    EXPECT_TRUE (paints (96.0f, 48.0f, 30, 24));
    EXPECT_TRUE (paints (96.0f, 48.0f, 65, 24));
    EXPECT_FALSE (paints (96.0f, 48.0f, 12, 24));
    EXPECT_FALSE (paints (96.0f, 48.0f, 84, 24));

    // Higher than wide: nothing above the circle either
    EXPECT_TRUE (paints (48.0f, 96.0f, 24, 48));
    EXPECT_TRUE (paints (48.0f, 96.0f, 24, 30));
    EXPECT_FALSE (paints (48.0f, 96.0f, 24, 12));

    // A square one is the circle it always was, to its sides
    EXPECT_TRUE (paints (48.0f, 48.0f, 7, 24));
    EXPECT_TRUE (paints (48.0f, 48.0f, 24, 7));
}

TEST_F (SliderGpuTest, TheThumbTakesItsPressedColorWhileItIsDragged)
{
    Slider slider (Slider::LinearHorizontal);
    slider.setBounds (0.0f, 0.0f, 96.0f, 32.0f);
    slider.setValue (0.5, dontSendNotification);

    // The same color idle and under the pointer, so only the press tells the thumb apart
    slider.setColor (Slider::Style::thumbColorId, Colors::red);
    slider.setColor (Slider::Style::thumbOverColorId, Colors::red);
    slider.setColor (Slider::Style::thumbDownColorId, Colors::blue);

    const auto thumb = slider.getThumbBounds().getCenter();
    const auto thumbColor = [&slider, &thumb]
    {
        const auto image = slider.snapshotToImage (*gpuContext);
        EXPECT_TRUE (image.isValid());

        return image.isValid() ? image.getPixelColor (roundToInt (thumb.getX()), roundToInt (thumb.getY())) : Color();
    };

    const auto idle = thumbColor();

    slider.mouseDown (MouseEvent (MouseEvent::leftButton, KeyModifiers(), thumb));
    ASSERT_TRUE (slider.isCurrentlyBeingDragged());

    const auto dragged = thumbColor();
    EXPECT_NE (dragged, idle);

    slider.mouseUp (MouseEvent (MouseEvent::noButtons, KeyModifiers(), thumb));
    EXPECT_EQ (thumbColor(), idle);
}
