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

#include <yup_audio_gui/yup_audio_gui.h>

#include <gtest/gtest.h>

using namespace yup;

namespace yup
{
extern std::unique_ptr<yup::GraphicsContext> yup_constructHeadlessGraphicsContext (yup::GpuDevice::Options, yup::GpuDevice::Ptr);
} // namespace yup

class CartesianPlaneTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        plane = std::make_unique<CartesianPlane>();
        plane->setBounds (0.0f, 0.0f, 800.0f, 600.0f);
    }

    std::unique_ptr<CartesianPlane> plane;
};

//==============================================================================
// Construction Tests
//==============================================================================

TEST_F (CartesianPlaneTests, DefaultConstructor)
{
    auto range = plane->getXRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1.0, range.getEnd());

    range = plane->getYRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, DefaultScaleTypeIsLinear)
{
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getYScaleType());
}

//==============================================================================
// Axis Range Tests
//==============================================================================

TEST_F (CartesianPlaneTests, SetXRangeUpdatesRange)
{
    plane->setXRange (0.0, 100.0);

    auto range = plane->getXRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (100.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetYRangeUpdatesRange)
{
    plane->setYRange (-50.0, 50.0);

    auto range = plane->getYRange();
    EXPECT_DOUBLE_EQ (-50.0, range.getStart());
    EXPECT_DOUBLE_EQ (50.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetXRangeWithNegativeValues)
{
    plane->setXRange (-100.0, -10.0);

    auto range = plane->getXRange();
    EXPECT_DOUBLE_EQ (-100.0, range.getStart());
    EXPECT_DOUBLE_EQ (-10.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetYRangeWithLargeValues)
{
    plane->setYRange (0.0, 1000000.0);

    auto range = plane->getYRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1000000.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetXRangeWithEqualValues)
{
    plane->setXRange (50.0, 50.0);

    // Equal values are rejected, range stays at default
    auto range = plane->getXRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1.0, range.getEnd());
}

TEST_F (CartesianPlaneTests, SetYRangeWithInvertedValues)
{
    plane->setYRange (100.0, 0.0);

    // Inverted values are rejected, range stays at default
    auto range = plane->getYRange();
    EXPECT_DOUBLE_EQ (0.0, range.getStart());
    EXPECT_DOUBLE_EQ (1.0, range.getEnd());
}

//==============================================================================
// Axis Scale Type Tests
//==============================================================================

TEST_F (CartesianPlaneTests, SetXScaleTypeToLogarithmic)
{
    // Logarithmic scale requires positive range
    plane->setXRange (1.0, 1000.0);
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getXScaleType());
}

TEST_F (CartesianPlaneTests, SetYScaleTypeToLogarithmic)
{
    // Logarithmic scale requires positive range
    plane->setYRange (1.0, 1000.0);
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getYScaleType());
}

TEST_F (CartesianPlaneTests, ToggleXScaleType)
{
    // Logarithmic scale requires positive range
    plane->setXRange (1.0, 1000.0);
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getXScaleType());

    plane->setXScaleType (CartesianPlane::AxisScaleType::linear);
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getXScaleType());
}

TEST_F (CartesianPlaneTests, ToggleYScaleType)
{
    // Logarithmic scale requires positive range
    plane->setYRange (1.0, 1000.0);
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getYScaleType());

    plane->setYScaleType (CartesianPlane::AxisScaleType::linear);
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getYScaleType());
}

//==============================================================================
// Margins Tests
//==============================================================================

TEST_F (CartesianPlaneTests, SetMarginsUpdatesMargins)
{
    plane->setMargins (40, 80, 30, 20);

    auto margins = plane->getMargins();
    EXPECT_EQ (80, margins.getLeft());
    EXPECT_EQ (40, margins.getTop());
}

TEST_F (CartesianPlaneTests, SetMarginsWithZeroValues)
{
    plane->setMargins (0, 0, 0, 0);

    auto margins = plane->getMargins();
    EXPECT_EQ (0, margins.getLeft());
    EXPECT_EQ (0, margins.getTop());
}

TEST_F (CartesianPlaneTests, SetMarginsWithLargeValues)
{
    plane->setMargins (200, 200, 200, 200);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Title Tests
//==============================================================================

TEST_F (CartesianPlaneTests, DefaultTitleIsEmpty)
{
    EXPECT_TRUE (plane->getTitle().isEmpty());
}

TEST_F (CartesianPlaneTests, SetTitleUpdatesTitle)
{
    plane->setTitle ("Frequency Response");
    EXPECT_EQ ("Frequency Response", plane->getTitle());
}

TEST_F (CartesianPlaneTests, SetEmptyTitle)
{
    plane->setTitle ("Test");
    plane->setTitle ("");
    EXPECT_TRUE (plane->getTitle().isEmpty());
}

TEST_F (CartesianPlaneTests, TheTitleIsTheOneOfTheComponent)
{
    // Taken as the component it is, the plane is given and tells the title it draws
    Component& component = *plane;

    component.setTitle ("Response");
    EXPECT_EQ (plane->getTitle(), "Response");
    EXPECT_EQ (component.getTitle(), "Response");

    plane->setTitle ("Phase");
    EXPECT_EQ (component.getTitle(), "Phase");
    EXPECT_EQ (component.getProperty (Identifier ("title")).toString(), "Phase");
}

TEST_F (CartesianPlaneTests, SetTitleColor)
{
    Color testColor (0xFF00FF00);
    plane->setTitleColor (testColor);
    EXPECT_EQ (testColor, plane->getTitleColor());
}

TEST_F (CartesianPlaneTests, SetTitleJustification)
{
    plane->setTitleJustification (Justification::left);
    EXPECT_EQ (Justification::left, plane->getTitleJustification());

    plane->setTitleJustification (Justification::right);
    EXPECT_EQ (Justification::right, plane->getTitleJustification());
}

//==============================================================================
// Background Color Tests
//==============================================================================

TEST_F (CartesianPlaneTests, SetBackgroundColor)
{
    Color testColor (0xFF123456);
    plane->setBackgroundColor (testColor);
    EXPECT_EQ (testColor, plane->getBackgroundColor());
}

//==============================================================================
// Vertical Grid Lines Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddVerticalGridLine)
{
    plane->addVerticalGridLine (0.5);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddMultipleVerticalGridLines)
{
    plane->addVerticalGridLine (0.25);
    plane->addVerticalGridLine (0.5);
    plane->addVerticalGridLine (0.75);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetVerticalGridLinesFromVector)
{
    std::vector<double> gridValues { 0.2, 0.4, 0.6, 0.8 };
    plane->setVerticalGridLines (gridValues);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ClearVerticalGridLines)
{
    plane->addVerticalGridLine (0.5);
    plane->clearVerticalGridLines();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddVerticalGridLineWithCustomColor)
{
    Color gridColor (0xFFFF0000);
    plane->addVerticalGridLine (0.5, gridColor, 2.0f, true);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetVerticalGridLinesWithEmptyVector)
{
    std::vector<double> emptyGrid;
    plane->setVerticalGridLines (emptyGrid);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Horizontal Grid Lines Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddHorizontalGridLine)
{
    plane->addHorizontalGridLine (0.5);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddMultipleHorizontalGridLines)
{
    plane->addHorizontalGridLine (0.25);
    plane->addHorizontalGridLine (0.5);
    plane->addHorizontalGridLine (0.75);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetHorizontalGridLinesFromVector)
{
    std::vector<double> gridValues { 0.2, 0.4, 0.6, 0.8 };
    plane->setHorizontalGridLines (gridValues);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ClearHorizontalGridLines)
{
    plane->addHorizontalGridLine (0.5);
    plane->clearHorizontalGridLines();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddHorizontalGridLineWithCustomColor)
{
    Color gridColor (0xFF00FF00);
    plane->addHorizontalGridLine (0.5, gridColor, 2.0f, true);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// X Axis Labels Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddXAxisLabel)
{
    plane->addXAxisLabel (0.5, "0.5");

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddMultipleXAxisLabels)
{
    plane->addXAxisLabel (0.0, "0");
    plane->addXAxisLabel (0.5, "0.5");
    plane->addXAxisLabel (1.0, "1");

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetXAxisLabelsFromVector)
{
    std::vector<double> labelValues { 0.0, 0.25, 0.5, 0.75, 1.0 };
    plane->setXAxisLabels (labelValues);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ClearXAxisLabels)
{
    plane->addXAxisLabel (0.5, "0.5");
    plane->clearXAxisLabels();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddXAxisLabelWithCustomColor)
{
    Color labelColor (0xFFFFFFFF);
    plane->addXAxisLabel (0.5, "0.5", labelColor, 12.0f);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetXAxisLabelsWithEmptyVector)
{
    std::vector<double> emptyLabels;
    plane->setXAxisLabels (emptyLabels);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Y Axis Labels Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddYAxisLabel)
{
    plane->addYAxisLabel (0.5, "0.5");

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddMultipleYAxisLabels)
{
    plane->addYAxisLabel (0.0, "0");
    plane->addYAxisLabel (0.5, "0.5");
    plane->addYAxisLabel (1.0, "1");

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetYAxisLabelsFromVector)
{
    std::vector<double> labelValues { 0.0, 0.25, 0.5, 0.75, 1.0 };
    plane->setYAxisLabels (labelValues);

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ClearYAxisLabels)
{
    plane->addYAxisLabel (0.5, "0.5");
    plane->clearYAxisLabels();

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, AddYAxisLabelWithCustomColor)
{
    Color labelColor (0xFFFFFFFF);
    plane->addYAxisLabel (0.5, "0.5", labelColor, 12.0f);

    // Should not crash
    EXPECT_TRUE (true);
}

//==============================================================================
// Signal Tests
//==============================================================================

TEST_F (CartesianPlaneTests, AddSignalReturnsIndex)
{
    int index = plane->addSignal ("Signal 1");
    EXPECT_EQ (0, index);
}

TEST_F (CartesianPlaneTests, AddMultipleSignals)
{
    int index1 = plane->addSignal ("Signal 1");
    int index2 = plane->addSignal ("Signal 2");

    EXPECT_EQ (0, index1);
    EXPECT_EQ (1, index2);
}

TEST_F (CartesianPlaneTests, GetNumSignals)
{
    plane->addSignal ("Signal 1");
    plane->addSignal ("Signal 2");

    EXPECT_EQ (2, plane->getNumSignals());
}

TEST_F (CartesianPlaneTests, GetSignalByIndex)
{
    plane->addSignal ("Test Signal", Colors::red, 3.0f);

    auto* signal = plane->getSignal (0);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ ("Test Signal", signal->name);
    EXPECT_EQ (Colors::red, signal->color);
    EXPECT_FLOAT_EQ (3.0f, signal->strokeWidth);
    EXPECT_TRUE (signal->visible);
}

TEST_F (CartesianPlaneTests, GetSignalWithInvalidIndex)
{
    auto* signal = plane->getSignal (10);
    EXPECT_EQ (nullptr, signal);
}

TEST_F (CartesianPlaneTests, UpdateSignalData)
{
    int index = plane->addSignal ("Signal 1");

    std::vector<Point<double>> data;
    data.push_back ({ 0.0, 0.0 });
    data.push_back ({ 0.5, 0.5 });
    data.push_back ({ 1.0, 1.0 });

    plane->updateSignalData (index, data);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (3, static_cast<int> (signal->data.size()));
}

TEST_F (CartesianPlaneTests, UpdateSignalDataWithInvalidIndex)
{
    std::vector<Point<double>> data;
    data.push_back ({ 0.0, 0.0 });

    // Should not crash
    plane->updateSignalData (10, data);
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SetSignalVisible)
{
    int index = plane->addSignal ("Signal 1");

    plane->setSignalVisible (index, false);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_FALSE (signal->visible);
}

TEST_F (CartesianPlaneTests, SetSignalColor)
{
    int index = plane->addSignal ("Signal 1");

    Color newColor (0xFF00FF00);
    plane->setSignalColor (index, newColor);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (newColor, signal->color);
}

TEST_F (CartesianPlaneTests, SetSignalStrokeWidth)
{
    int index = plane->addSignal ("Signal 1");

    plane->setSignalStrokeWidth (index, 5.0f);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_FLOAT_EQ (5.0f, signal->strokeWidth);
}

TEST_F (CartesianPlaneTests, ClearSignals)
{
    plane->addSignal ("Signal 1");
    plane->addSignal ("Signal 2");

    plane->clearSignals();

    EXPECT_EQ (0, plane->getNumSignals());
}

TEST_F (CartesianPlaneTests, UpdateSignalDataWithEmptyVector)
{
    int index = plane->addSignal ("Signal 1");

    std::vector<Point<double>> emptyData;
    plane->updateSignalData (index, emptyData);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (0, static_cast<int> (signal->data.size()));
}

TEST_F (CartesianPlaneTests, UpdateSignalDataWithSinglePoint)
{
    int index = plane->addSignal ("Signal 1");

    std::vector<Point<double>> data;
    data.push_back ({ 0.5, 0.5 });

    plane->updateSignalData (index, data);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (1, static_cast<int> (signal->data.size()));
}

TEST_F (CartesianPlaneTests, UpdateSignalDataWithManyPoints)
{
    int index = plane->addSignal ("Signal 1");

    std::vector<Point<double>> data;
    for (int i = 0; i < 1000; ++i)
        data.push_back ({ static_cast<double> (i) / 1000.0, static_cast<double> (i) / 1000.0 });

    plane->updateSignalData (index, data);

    auto* signal = plane->getSignal (index);
    ASSERT_NE (nullptr, signal);
    EXPECT_EQ (1000, static_cast<int> (signal->data.size()));
}

//==============================================================================
// Legend Tests
//==============================================================================

TEST_F (CartesianPlaneTests, LegendVisibleByDefault)
{
    EXPECT_TRUE (plane->isLegendVisible());
}

TEST_F (CartesianPlaneTests, SetLegendVisibleHidesLegend)
{
    plane->setLegendVisible (false);
    EXPECT_FALSE (plane->isLegendVisible());
}

TEST_F (CartesianPlaneTests, ToggleLegendVisibility)
{
    plane->setLegendVisible (false);
    EXPECT_FALSE (plane->isLegendVisible());

    plane->setLegendVisible (true);
    EXPECT_TRUE (plane->isLegendVisible());
}

TEST_F (CartesianPlaneTests, SetLegendPosition)
{
    Point<float> newPosition (0.5f, 0.5f);
    plane->setLegendPosition (newPosition);

    auto position = plane->getLegendPosition();
    EXPECT_FLOAT_EQ (0.5f, position.getX());
    EXPECT_FLOAT_EQ (0.5f, position.getY());
}

TEST_F (CartesianPlaneTests, SetLegendBackgroundColor)
{
    Color newColor (0x80FF0000);
    plane->setLegendBackgroundColor (newColor);

    EXPECT_EQ (newColor, plane->getLegendBackgroundColor());
}

//==============================================================================
// Coordinate Transformation Tests
//==============================================================================

TEST_F (CartesianPlaneTests, ValueToXConversion)
{
    plane->setXRange (0.0, 100.0);

    float x = plane->valueToX (50.0);

    // Should be somewhere in the middle of the plot area
    EXPECT_GT (x, 0.0f);
    EXPECT_LT (x, 800.0f);
}

TEST_F (CartesianPlaneTests, ValueToYConversion)
{
    plane->setYRange (0.0, 100.0);

    float y = plane->valueToY (50.0);

    // Should be somewhere in the middle of the plot area
    EXPECT_GT (y, 0.0f);
    EXPECT_LT (y, 600.0f);
}

TEST_F (CartesianPlaneTests, XToValueConversion)
{
    plane->setXRange (0.0, 100.0);

    double value = plane->xToValue (400.0f);

    // Should be approximately in the middle of the range
    EXPECT_GT (value, 0.0);
    EXPECT_LT (value, 100.0);
}

TEST_F (CartesianPlaneTests, YToValueConversion)
{
    plane->setYRange (0.0, 100.0);

    double value = plane->yToValue (300.0f);

    // Should be within the range
    EXPECT_GT (value, 0.0);
    EXPECT_LT (value, 100.0);
}

TEST_F (CartesianPlaneTests, GetPlotBoundsReturnsValidBounds)
{
    auto bounds = plane->getPlotBounds();

    EXPECT_GT (bounds.getWidth(), 0.0f);
    EXPECT_GT (bounds.getHeight(), 0.0f);
}

TEST_F (CartesianPlaneTests, CoordinateTransformationRoundTrip)
{
    plane->setXRange (0.0, 100.0);

    double originalValue = 50.0;
    float x = plane->valueToX (originalValue);
    double convertedValue = plane->xToValue (x);

    EXPECT_NEAR (originalValue, convertedValue, 1.0);
}

TEST_F (CartesianPlaneTests, ValuesArePlacedInProportionOnALinearScale)
{
    plane->setXRange (-10.0, 30.0);
    plane->setYRange (2.0, 6.0);

    const auto plot = plane->getPlotBounds();

    // The ends of the ranges are the edges of the plot
    EXPECT_FLOAT_EQ (plane->valueToX (-10.0), plot.getX());
    EXPECT_FLOAT_EQ (plane->valueToX (30.0), plot.getRight());
    EXPECT_FLOAT_EQ (plane->valueToY (2.0), plot.getBottom());
    EXPECT_FLOAT_EQ (plane->valueToY (6.0), plot.getY());

    // A quarter of the way along the ranges is a quarter of the way across the plot, y going up
    EXPECT_FLOAT_EQ (plane->valueToX (0.0), plot.getX() + 0.25f * plot.getWidth());
    EXPECT_FLOAT_EQ (plane->valueToY (3.0), plot.getBottom() - 0.25f * plot.getHeight());

    // Values out of the ranges are as far out of the plot
    EXPECT_FLOAT_EQ (plane->valueToX (-20.0), plot.getX() - 0.25f * plot.getWidth());
    EXPECT_FLOAT_EQ (plane->valueToY (7.0), plot.getY() - 0.25f * plot.getHeight());

    EXPECT_NEAR (plane->xToValue (plane->valueToX (12.5)), 12.5, 1.0e-3);
    EXPECT_NEAR (plane->yToValue (plane->valueToY (4.5)), 4.5, 1.0e-3);
}

TEST_F (CartesianPlaneTests, ValuesArePlacedByTheirLogarithmOnALogarithmicScale)
{
    plane->setXRange (10.0, 10000.0);
    plane->setYRange (1.0, 100.0);
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);

    const auto plot = plane->getPlotBounds();

    // Every decade takes the same room
    EXPECT_NEAR (plane->valueToX (10.0), plot.getX(), 0.01f);
    EXPECT_NEAR (plane->valueToX (100.0), plot.getX() + plot.getWidth() / 3.0f, 0.01f);
    EXPECT_NEAR (plane->valueToX (1000.0), plot.getX() + plot.getWidth() * 2.0f / 3.0f, 0.01f);
    EXPECT_NEAR (plane->valueToX (10000.0), plot.getRight(), 0.01f);
    EXPECT_NEAR (plane->valueToY (1.0), plot.getBottom(), 0.01f);
    EXPECT_NEAR (plane->valueToY (10.0), plot.getCenterY(), 0.01f);
    EXPECT_NEAR (plane->valueToY (100.0), plot.getY(), 0.01f);

    // A value under the range is before the plot, and one with no logarithm at the start of its axis
    EXPECT_NEAR (plane->valueToX (1.0), plot.getX() - plot.getWidth() / 3.0f, 0.01f);
    EXPECT_FLOAT_EQ (plane->valueToX (0.0), plot.getX());
    EXPECT_FLOAT_EQ (plane->valueToX (-5.0), plot.getX());
    EXPECT_FLOAT_EQ (plane->valueToY (0.0), plot.getBottom());
    EXPECT_FLOAT_EQ (plane->valueToY (-5.0), plot.getBottom());

    EXPECT_NEAR (plane->xToValue (plane->valueToX (250.0)), 250.0, 0.1);
    EXPECT_NEAR (plane->yToValue (plane->valueToY (25.0)), 25.0, 0.01);
}

//==============================================================================
// Logarithmic Scale Tests
//==============================================================================

TEST_F (CartesianPlaneTests, LogarithmicScaleForXAxis)
{
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    plane->setXRange (1.0, 1000.0);

    float x1 = plane->valueToX (10.0);
    float x2 = plane->valueToX (100.0);

    // With logarithmic scale, these should have meaningful spacing
    EXPECT_NE (x1, x2);
}

TEST_F (CartesianPlaneTests, LogarithmicScaleForYAxis)
{
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    plane->setYRange (1.0, 1000.0);

    float y1 = plane->valueToY (10.0);
    float y2 = plane->valueToY (100.0);

    // With logarithmic scale, these should have meaningful spacing
    EXPECT_NE (y1, y2);
}

TEST_F (CartesianPlaneTests, ALogarithmicScaleAppliesWhileTheRangeIsAboveZero)
{
    const auto plot = plane->getPlotBounds();

    // Asked on a range starting at zero, the scales are logarithmic once their ranges allow it
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getYScaleType());

    plane->setXRange (1.0, 100.0);
    plane->setYRange (1.0, 100.0);
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, plane->getYScaleType());

    EXPECT_NEAR (plane->valueToX (10.0), plot.getCenterX(), 0.01f);
    EXPECT_NEAR (plane->valueToY (10.0), plot.getCenterY(), 0.01f);
    EXPECT_NEAR (plane->xToValue (plot.getCenterX()), 10.0, 0.01);
    EXPECT_NEAR (plane->yToValue (plot.getCenterY()), 10.0, 0.01);

    // Ranges moved back down to zero are told linearly again, rather than not at all
    plane->setXRange (0.0, 100.0);
    plane->setYRange (0.0, 100.0);
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, plane->getYScaleType());

    EXPECT_NEAR (plane->valueToX (50.0), plot.getCenterX(), 0.01f);
    EXPECT_NEAR (plane->valueToY (50.0), plot.getCenterY(), 0.01f);
    EXPECT_NEAR (plane->xToValue (plot.getCenterX()), 50.0, 0.01);
    EXPECT_NEAR (plane->yToValue (plot.getCenterY()), 50.0, 0.01);
}

TEST_F (CartesianPlaneTests, APlaneNoLargerThanItsMarginsHasAnEmptyPlot)
{
    plane->setXRange (20.0, 20000.0);
    plane->setYRange (-60.0, 6.0);

    // The margins take 80 by 55: nothing is left for the plot, which is no less than empty
    for (const auto size : { Size<float> (80.0f, 55.0f), Size<float> (40.0f, 30.0f) })
    {
        plane->setBounds (0.0f, 0.0f, size.getWidth(), size.getHeight());

        const auto plot = plane->getPlotBounds();
        EXPECT_FLOAT_EQ (plot.getWidth(), 0.0f);
        EXPECT_FLOAT_EQ (plot.getHeight(), 0.0f);

        // And every position of it is the start of the ranges
        EXPECT_DOUBLE_EQ (plane->xToValue (plot.getX()), 20.0);
        EXPECT_DOUBLE_EQ (plane->yToValue (plot.getBottom()), -60.0);
        EXPECT_TRUE (std::isfinite (plane->valueToX (1000.0)));
        EXPECT_TRUE (std::isfinite (plane->valueToY (0.0)));
    }
}

//==============================================================================
// Paint Tests
//==============================================================================

TEST_F (CartesianPlaneTests, PaintWithoutCrashing)
{
    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithSignals)
{
    int index = plane->addSignal ("Test Signal");
    std::vector<Point<double>> data;
    data.push_back ({ 0.0, 0.0 });
    data.push_back ({ 1.0, 1.0 });
    plane->updateSignalData (index, data);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithGridLines)
{
    plane->addVerticalGridLine (0.5);
    plane->addHorizontalGridLine (0.5);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithLabels)
{
    plane->addXAxisLabel (0.0, "0");
    plane->addXAxisLabel (1.0, "1");
    plane->addYAxisLabel (0.0, "0");
    plane->addYAxisLabel (1.0, "1");

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithTitle)
{
    plane->setTitle ("Test Plot");

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithLegend)
{
    plane->addSignal ("Signal 1", Colors::red);
    plane->addSignal ("Signal 2", Colors::blue);
    plane->setLegendVisible (true);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintWithZeroSize)
{
    plane->setBounds (0.0f, 0.0f, 0.0f, 0.0f);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (1, 1);
    Graphics g (*context, *renderer);

    plane->paint (g);

    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, PaintsALegendThatDoesNotFitInThePlot)
{
    plane->addSignal ("Signal 1", Colors::red);
    plane->addSignal ("Signal 2", Colors::blue);
    plane->setLegendVisible (true);

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (200, 100);
    Graphics g (*context, *renderer);

    // A plot narrower and lower than its legend, then one with no room at all
    for (const auto size : { Size<float> (150.0f, 75.0f), Size<float> (60.0f, 40.0f) })
    {
        plane->setBounds (0.0f, 0.0f, size.getWidth(), size.getHeight());
        plane->paint (g);
    }

    EXPECT_GE (plane->getPlotBounds().getWidth(), 0.0f);
    EXPECT_GE (plane->getPlotBounds().getHeight(), 0.0f);
}

TEST_F (CartesianPlaneTests, PaintsASignalDenserThanThePixelsOfThePlotAndPaintsItAgain)
{
    plane->setXRange (0.0, 1.0);
    plane->setYRange (-1.0, 1.0);
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);

    // Many points for each pixel of the plot, some of them over and under it
    const auto makeData = [] (double level)
    {
        std::vector<Point<double>> data;

        for (int i = 0; i < 20000; ++i)
            data.push_back ({ i / 19999.0, level * std::sin (i * 0.37) });

        return data;
    };

    const int index = plane->addSignal ("Dense");
    plane->updateSignalData (index, makeData (1.5));

    auto context = yup_constructHeadlessGraphicsContext ({}, {});
    auto renderer = context->makeRenderer (800, 600);
    Graphics g (*context, *renderer);

    // The second paint finds the signal as the first one left it, and the ones after a change make it again
    plane->paint (g);
    plane->paint (g);

    plane->updateSignalData (index, makeData (0.5));
    plane->paint (g);

    plane->setXRange (0.001, 1.0);
    plane->paint (g);

    g.setTransform (AffineTransform::scaling (0.5f));
    plane->paint (g);

    plane->clearSignals();
    plane->paint (g);

    EXPECT_EQ (0, plane->getNumSignals());
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F (CartesianPlaneTests, MultipleSignalsWithDifferentColors)
{
    plane->addSignal ("Red Signal", Colors::red, 1.0f);
    plane->addSignal ("Green Signal", Colors::green, 2.0f);
    plane->addSignal ("Blue Signal", Colors::blue, 3.0f);

    EXPECT_EQ (3, plane->getNumSignals());

    auto* signal1 = plane->getSignal (0);
    auto* signal2 = plane->getSignal (1);
    auto* signal3 = plane->getSignal (2);

    EXPECT_EQ (Colors::red, signal1->color);
    EXPECT_EQ (Colors::green, signal2->color);
    EXPECT_EQ (Colors::blue, signal3->color);
}

TEST_F (CartesianPlaneTests, ComplexGridConfiguration)
{
    plane->setVerticalGridLines ({ 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 });
    plane->setHorizontalGridLines ({ 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9 });

    // Should not crash
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, SignalDataOutsideRange)
{
    plane->setXRange (0.0, 1.0);
    plane->setYRange (0.0, 1.0);

    int index = plane->addSignal ("Out of bounds");

    std::vector<Point<double>> data;
    data.push_back ({ -1.0, -1.0 });
    data.push_back ({ 2.0, 2.0 });

    plane->updateSignalData (index, data);

    // Should handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, NegativeAxisRanges)
{
    plane->setXRange (-100.0, -10.0);
    plane->setYRange (-50.0, 50.0);

    int index = plane->addSignal ("Negative Range");
    std::vector<Point<double>> data;
    data.push_back ({ -50.0, 0.0 });

    plane->updateSignalData (index, data);

    // Should handle gracefully
    EXPECT_TRUE (true);
}

TEST_F (CartesianPlaneTests, ZeroRangeHandling)
{
    plane->setXRange (5.0, 5.0);
    plane->setYRange (5.0, 5.0);

    // Should handle gracefully
    EXPECT_TRUE (true);
}

//==============================================================================
// Type Info Tests
//==============================================================================

TEST_F (CartesianPlaneTests, TheTypeInfoSetsTheRangesInAnyOrderAndTheScalesAfterThem)
{
    const auto& type = CartesianPlane::getClassTypeInfo();
    EXPECT_EQ (type.getTypeName().toString(), "CartesianPlane");
    EXPECT_EQ (&plane->getTypeInfo(), &type);

    const auto created = type.createInstance();
    auto* instance = dynamic_cast<CartesianPlane*> (created.get());
    ASSERT_NE (nullptr, instance);

    // The title is the one the plane draws
    instance->setProperty ("title", "Response");
    EXPECT_EQ (instance->getTitle(), "Response");
    EXPECT_EQ (instance->getProperty ("title").toString(), "Response");

    // An end written past the other one moves it out of its way, one written before it leaves it alone
    instance->setProperty ("xMin", 20.0);
    instance->setProperty ("xMax", 20000.0);
    EXPECT_DOUBLE_EQ (20.0, instance->getXRange().getStart());
    EXPECT_DOUBLE_EQ (20000.0, instance->getXRange().getEnd());

    instance->setProperty ("yMax", -6.0);
    instance->setProperty ("yMin", -60.0);
    EXPECT_DOUBLE_EQ (-60.0, instance->getYRange().getStart());
    EXPECT_DOUBLE_EQ (-6.0, instance->getYRange().getEnd());

    // A scale is logarithmic for a range above zero only
    instance->setProperty ("xScaleType", "logarithmic");
    instance->setProperty ("yScaleType", "logarithmic");
    EXPECT_EQ (CartesianPlane::AxisScaleType::logarithmic, instance->getXScaleType());
    EXPECT_EQ (CartesianPlane::AxisScaleType::linear, instance->getYScaleType());

    instance->setProperty ("legendVisible", false);
    EXPECT_FALSE (instance->isLegendVisible());
}

//==============================================================================
// GPU Path Tests (skipped when no GPU backend is available)
//==============================================================================

class CartesianPlaneGpuTests : public ::testing::Test
{
protected:
    static constexpr int width = 400;
    static constexpr int height = 300;

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

        canvas = GpuCanvas::create (*gpuContext, width, height);
        if (canvas == nullptr)
            GTEST_SKIP() << "Unable to create paint GpuCanvas";

        // A plot taking the whole plane, with nothing on it but a red signal over black
        plane = std::make_unique<CartesianPlane>();
        plane->setBounds (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height));
        plane->setMargins (0, 0, 0, 0);
        plane->setLegendVisible (false);
        plane->setBackgroundColor (Colors::black);
        signalIndex = plane->addSignal ("Signal", Color (0xffff0000), 4.0f);
    }

    void TearDown() override
    {
        plane.reset();
        canvas.reset();
    }

    /** Paints the plane into the canvas, at a number of pixels a unit, and reads its pixels back. */
    bool paintPlane (float pixelsPerUnit = 1.0f)
    {
        plane->paint (canvas->beginDraw ({}, pixelsPerUnit));
        canvas->commit();

        pixels.assign (static_cast<size_t> (width) * static_cast<size_t> (height) * 4u, 0);
        return canvas->readPixels (pixels.data(), pixels.size());
    }

    /** Whether the signal covers a pixel of the canvas, as it was last painted. */
    bool isSignalAt (int x, int y) const
    {
        return pixels[(static_cast<size_t> (y) * static_cast<size_t> (width) + static_cast<size_t> (x)) * 4u] > 128;
    }

    /** A level line of two points, from 5% to 95% of the default range of x. */
    static std::vector<Point<double>> makeLine (double level)
    {
        return { { 0.05, level }, { 0.95, level } };
    }

    /** Many points for each pixel, going from a level to another and back, from 5% to 95% of the default range of x. */
    static std::vector<Point<double>> makeZigzag (double low, double high)
    {
        std::vector<Point<double>> data;

        for (int i = 0; i < 16000; ++i)
            data.push_back ({ 0.05 + 0.9 * i / 15999.0, i % 2 == 0 ? low : high });

        return data;
    }

    static std::unique_ptr<GraphicsContext> gpuContext;

    GpuCanvas::Ptr canvas;
    std::unique_ptr<CartesianPlane> plane;
    std::vector<uint8> pixels;
    int signalIndex = -1;
};

std::unique_ptr<GraphicsContext> CartesianPlaneGpuTests::gpuContext;

TEST_F (CartesianPlaneGpuTests, PaintsASparseSignalAndADenseOneWhereTheirValuesAre)
{
    // Two points: a line a quarter of the way up the plot
    plane->updateSignalData (signalIndex, makeLine (0.25));
    ASSERT_TRUE (paintPlane());

    for (const auto x : { 30, 200, 370 })
    {
        EXPECT_TRUE (isSignalAt (x, 225)) << "at x = " << x;
        EXPECT_FALSE (isSignalAt (x, 150)) << "at x = " << x;
        EXPECT_FALSE (isSignalAt (x, 75)) << "at x = " << x;
    }

    EXPECT_FALSE (isSignalAt (5, 225));
    EXPECT_FALSE (isSignalAt (395, 225));

    // 16000 points going between a quarter and three quarters of the way up: the band they cover
    plane->updateSignalData (signalIndex, makeZigzag (0.25, 0.75));
    ASSERT_TRUE (paintPlane());

    for (const auto x : { 30, 200, 370 })
    {
        for (const auto y : { 85, 150, 215 })
            EXPECT_TRUE (isSignalAt (x, y)) << "at " << x << ", " << y;

        EXPECT_FALSE (isSignalAt (x, 30)) << "at x = " << x;
        EXPECT_FALSE (isSignalAt (x, 270)) << "at x = " << x;
    }

    EXPECT_FALSE (isSignalAt (5, 150));
    EXPECT_FALSE (isSignalAt (395, 150));
}

TEST_F (CartesianPlaneGpuTests, ADenseSignalIsOnlyPaintedWhereItIsInsideThePlot)
{
    // Level across the plot, but for its middle third which is far over it
    std::vector<Point<double>> data;

    for (int i = 0; i < 16000; ++i)
    {
        const auto position = i / 15999.0;
        data.push_back ({ 0.05 + 0.9 * position, position > 0.35 && position < 0.65 ? 5.0 : 0.5 });
    }

    plane->updateSignalData (signalIndex, data);
    ASSERT_TRUE (paintPlane());

    EXPECT_TRUE (isSignalAt (60, 150));
    EXPECT_TRUE (isSignalAt (340, 150));
    EXPECT_FALSE (isSignalAt (200, 150));
    EXPECT_FALSE (isSignalAt (200, 20));
}

TEST_F (CartesianPlaneGpuTests, ASignalIsPaintedAgainWhereItsNewDataIs)
{
    plane->updateSignalData (signalIndex, makeLine (0.25));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (200, 225));

    plane->updateSignalData (signalIndex, makeLine (0.75));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (200, 75));
    EXPECT_FALSE (isSignalAt (200, 225));

    // A signal added once the others were cleared is not the one that was painted in its place
    plane->clearSignals();
    const auto added = plane->addSignal ("Other", Color (0xffff0000), 4.0f);
    plane->updateSignalData (added, makeLine (0.5));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (200, 150));
    EXPECT_FALSE (isSignalAt (200, 75));

    // What is kept of a signal is not painted while it is hidden
    plane->setSignalVisible (added, false);
    ASSERT_TRUE (paintPlane());
    EXPECT_FALSE (isSignalAt (200, 150));

    plane->setSignalVisible (added, true);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (200, 150));
}

TEST_F (CartesianPlaneGpuTests, ASignalIsPaintedAgainWhereTheRangesOfItsAxesPlaceIt)
{
    plane->updateSignalData (signalIndex, makeLine (0.25));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (300, 225));

    // Twice the range of y: half as high
    plane->setYRange (0.0, 2.0);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (300, 262));
    EXPECT_FALSE (isSignalAt (300, 225));

    // Twice the range of x: half as long
    plane->setXRange (0.0, 2.0);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (100, 262));
    EXPECT_FALSE (isSignalAt (300, 262));
}

TEST_F (CartesianPlaneGpuTests, ASignalIsPaintedAgainWhereTheScalesOfItsAxesPlaceIt)
{
    plane->setXRange (1.0, 100.0);
    plane->setYRange (1.0, 100.0);
    plane->updateSignalData (signalIndex, { { 2.0, 10.0 }, { 10.0, 10.0 } });
    ASSERT_TRUE (paintPlane());

    // Linear: from 4 to 36 pixels along x, 27 pixels up
    EXPECT_TRUE (isSignalAt (20, 272));
    EXPECT_FALSE (isSignalAt (20, 150));

    // 10 is half the way from 1 to 100 on a logarithmic scale
    plane->setYScaleType (CartesianPlane::AxisScaleType::logarithmic);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (20, 150));
    EXPECT_FALSE (isSignalAt (20, 272));
    EXPECT_FALSE (isSignalAt (150, 150));

    // And 2 a little under a sixth of it: from 60 to 200 pixels
    plane->setXScaleType (CartesianPlane::AxisScaleType::logarithmic);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (150, 150));
    EXPECT_FALSE (isSignalAt (20, 150));
}

TEST_F (CartesianPlaneGpuTests, ASignalIsPaintedAgainWhereThePlotIs)
{
    plane->updateSignalData (signalIndex, makeLine (0.25));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (300, 225));

    // A margin at the top leaves the plot two thirds of the height
    plane->setMargins (100, 0, 0, 0);
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (300, 250));
    EXPECT_FALSE (isSignalAt (300, 225));

    // And a plane half as wide half of the width
    plane->setBounds (0.0f, 0.0f, 200.0f, static_cast<float> (height));
    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (100, 250));
    EXPECT_FALSE (isSignalAt (300, 250));
}

TEST_F (CartesianPlaneGpuTests, ADenseSignalIsPaintedForThePixelsOfTheScaleItIsPaintedAt)
{
    // Half the canvas each way, which it fills at two pixels a unit
    plane->setBounds (0.0f, 0.0f, 200.0f, 150.0f);
    plane->updateSignalData (signalIndex, makeZigzag (0.25, 0.75));

    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (100, 75));
    EXPECT_FALSE (isSignalAt (100, 20));
    EXPECT_FALSE (isSignalAt (300, 150));

    ASSERT_TRUE (paintPlane (2.0f));
    EXPECT_TRUE (isSignalAt (200, 150));
    EXPECT_TRUE (isSignalAt (300, 100));
    EXPECT_TRUE (isSignalAt (300, 200));
    EXPECT_FALSE (isSignalAt (300, 40));
    EXPECT_FALSE (isSignalAt (300, 260));

    ASSERT_TRUE (paintPlane());
    EXPECT_TRUE (isSignalAt (100, 75));
    EXPECT_FALSE (isSignalAt (300, 150));
}

// TEMPORARY-PERF-AB begin
//
// What the findings P4 and P6 of the widgets cost today in yup_audio_gui, before anything is written for them (see the
// block of the same name in tests/yup_gui/yup_PaintBenchmark.cpp). Each test appends its lines to build/w6b-measure.txt
// in the repository and prints them, as "<finding>: <what> = <median ms> (<n> iterations)": the median of 200
// iterations timed after 30 warm-ups. A paint is into a canvas at 2 pixels a unit and is told twice, the paint call
// alone and with the frame around it up to its commit. What is too short to time once is run a number of times over
// and told for one of them.

#include <yup_gui_markup/yup_gui_markup.h>

/** P4: what laying out their texts at every paint costs the labels of a plot and the channel labels of an audio view.
    Each is painted with its texts and without, beside the layout of the texts alone. */
TEST_F (CartesianPlaneGpuTests, DISABLED_TemporaryMeasurePlotAndChannelLabels)
{
    const auto tell = [] (const String& what, std::vector<double> milliseconds)
    {
        std::sort (milliseconds.begin(), milliseconds.end());

        const auto file = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getChildFile ("build/w6b-measure.txt");
        const auto line = what + " = " + String (milliseconds[milliseconds.size() / 2], 6) + " (" + String (static_cast<int> (milliseconds.size())) + " iterations)\n";

        EXPECT_TRUE (file.getParentDirectory().createDirectory().wasOk() && file.appendText (line, false, false, "\n")) << "Could not append to " << file.getFullPathName();
        std::cout << line << std::flush;
    };

    const auto timeRun = [&tell] (const String& what, int timesOver, const std::function<void()>& run)
    {
        std::vector<double> milliseconds;

        for (int iteration = 0; iteration < 230; ++iteration)
        {
            const auto start = Time::getMillisecondCounterHiRes();

            for (int i = 0; i < timesOver; ++i)
                run();

            if (iteration >= 30)
                milliseconds.push_back ((Time::getMillisecondCounterHiRes() - start) / timesOver);
        }

        tell (what, milliseconds);
    };

    const auto timePaint = [&] (const String& what, Component& component, GpuCanvas& target)
    {
        std::vector<double> paints, frames;

        for (int iteration = 0; iteration < 230; ++iteration)
        {
            const auto frameStart = Time::getMillisecondCounterHiRes();
            auto& g = target.beginDraw ({}, 2.0f);

            const auto paintStart = Time::getMillisecondCounterHiRes();
            component.paint (g);
            const auto paintEnd = Time::getMillisecondCounterHiRes();

            target.commit();
            const auto frameEnd = Time::getMillisecondCounterHiRes();

            pixels.resize (static_cast<size_t> (target.getWidth()) * static_cast<size_t> (target.getHeight()) * 4u);
            EXPECT_TRUE (target.readPixels (pixels.data(), pixels.size()));

            if (iteration < 30)
                continue;

            paints.push_back (paintEnd - paintStart);
            frames.push_back (frameEnd - frameStart);
        }

        tell (what + ", paint call", paints);
        tell (what + ", paint call and commit", frames);
    };

    ASSERT_NE (ApplicationTheme::getGlobalTheme(), nullptr);

    // Makes and lays out a text as Graphics::fillFittedText does for a String, without drawing it
    const auto layOut = [] (const String& text, const Font& font, Size<float> area, StyledText::HorizontalAlign align)
    {
        StyledText styledText;
        {
            auto modifier = styledText.startUpdate();
            modifier.setMaxSize (area);
            modifier.appendText (text, font);
            modifier.setHorizontalAlign (align);
            modifier.setVerticalAlign (StyledText::middle);
        }

        return styledText.isEmpty() ? 0 : 1;
    };

    // CartesianPlane: a curve, a grid and 20 axis labels, with no title and no legend
    {
        StringArray texts;

        for (int i = 0; i < 10; ++i)
        {
            texts.add (String ((i + 1) * 2000) + " Hz");
            texts.add (String ((i + 1) * 6 - 66) + " dB");
        }

        std::vector<Point<double>> curve;

        for (int i = 0; i < 512; ++i)
            curve.push_back ({ i / 511.0, 0.5 + 0.4 * std::sin (i * 0.05) });

        CartesianPlane withTexts, withoutTexts;

        for (auto* each : { &withTexts, &withoutTexts })
        {
            each->setBounds (0.0f, 0.0f, 400.0f, 300.0f);
            each->setProperty ("legendVisible", false);
            each->updateSignalData (each->addSignal ("Curve"), curve);

            for (int i = 0; i < 10; ++i)
            {
                const auto value = (i + 0.5) / 10.0;

                each->addVerticalGridLine (value);
                each->addHorizontalGridLine (value);
                each->addXAxisLabel (value, each == &withTexts ? texts[2 * i] : String());
                each->addYAxisLabel (value, each == &withTexts ? texts[2 * i + 1] : String());
            }
        }

        if (const auto target = GpuCanvas::create (*gpuContext, 800, 600); target != nullptr)
        {
            timePaint ("P4: CartesianPlane of a curve, a grid and 20 axis labels", withTexts, *target);
            timePaint ("P4: the same CartesianPlane with empty labels", withoutTexts, *target);

            int numLaidOut = 0;

            timeRun ("P4: the texts of those 20 axis labels made and laid out as the plane does, alone", 10, [&]
            {
                for (int i = 0; i < texts.size(); ++i)
                {
                    const auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (10.0f);

                    numLaidOut += i % 2 == 0 ? layOut (texts[i], font, Size<float> (60.0f, 23.0f), StyledText::center)
                                             : layOut (texts[i], font, Size<float> (56.0f, 16.0f), StyledText::right);
                }
            });

            EXPECT_GT (numLaidOut, 0);
        }
        else
        {
            ADD_FAILURE() << "No canvas to paint the CartesianPlane in";
        }
    }

    // AudioView: 8 channels, and the same view telling no label for them
    {
        struct Unlabelled final : AudioViewComponent
        {
            using AudioViewComponent::AudioViewComponent;

            String getChannelLabel (int) const override { return {}; }
        };

        AudioBuffer<float> buffer (8, 10000);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer (channel);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                samples[i] = std::sin (0.002f * static_cast<float> ((channel + 1) * i));
        }

        AudioThumbnail thumbnail;
        thumbnail.setSource (&buffer, 44100.0);

        AudioViewComponent labelled (thumbnail);
        Unlabelled unlabelled (thumbnail);
        labelled.setBounds (0.0f, 0.0f, 400.0f, 300.0f);
        unlabelled.setBounds (0.0f, 0.0f, 400.0f, 300.0f);

        const auto profile = thumbnail.getPeakProfile();
        const auto target = GpuCanvas::create (*gpuContext, 800, 600);

        if (target != nullptr && profile != nullptr && profile->isValid())
        {
            timePaint ("P4: AudioView of 8 channels with their labels", labelled, *target);
            timePaint ("P4: the same AudioView telling no channel label", unlabelled, *target);

            int numLaidOut = 0;

            timeRun ("P4: the texts of those 8 channel labels made and laid out as the view does, alone", 10, [&]
            {
                const auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (12.0f);

                for (int channel = 0; channel < 8; ++channel)
                    numLaidOut += layOut ("Ch " + String (channel + 1), font, Size<float> (48.0f, 35.0f), StyledText::center);
            });

            EXPECT_GT (numLaidOut, 0);
        }
        else
        {
            ADD_FAILURE() << "No AudioView of 8 channels to measure";
        }
    }
}

/** P6: what filling the series of the keyboard again at every read costs a drawing reading them. A keyboard painted by
    the Keys drawing of docs/ui/yui/widgets.md, beside the reads a paint of it makes alone. */
TEST_F (CartesianPlaneGpuTests, DISABLED_TemporaryMeasureKeyboardSeries)
{
    const auto tell = [] (const String& what, std::vector<double> milliseconds)
    {
        std::sort (milliseconds.begin(), milliseconds.end());

        const auto file = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getChildFile ("build/w6b-measure.txt");
        const auto line = what + " = " + String (milliseconds[milliseconds.size() / 2], 6) + " (" + String (static_cast<int> (milliseconds.size())) + " iterations)\n";

        EXPECT_TRUE (file.getParentDirectory().createDirectory().wasOk() && file.appendText (line, false, false, "\n")) << "Could not append to " << file.getFullPathName();
        std::cout << line << std::flush;
    };

    const auto timeRun = [&tell] (const String& what, int timesOver, const std::function<void()>& run)
    {
        std::vector<double> milliseconds;

        for (int iteration = 0; iteration < 230; ++iteration)
        {
            const auto start = Time::getMillisecondCounterHiRes();

            for (int i = 0; i < timesOver; ++i)
                run();

            if (iteration >= 30)
                milliseconds.push_back ((Time::getMillisecondCounterHiRes() - start) / timesOver);
        }

        tell (what, milliseconds);
    };

    const auto timePaint = [&] (const String& what, Component& component, GpuCanvas& target)
    {
        std::vector<double> paints, frames;

        for (int iteration = 0; iteration < 230; ++iteration)
        {
            const auto frameStart = Time::getMillisecondCounterHiRes();
            auto& g = target.beginDraw ({}, 2.0f);

            const auto paintStart = Time::getMillisecondCounterHiRes();
            component.paint (g);
            const auto paintEnd = Time::getMillisecondCounterHiRes();

            target.commit();
            const auto frameEnd = Time::getMillisecondCounterHiRes();

            pixels.resize (static_cast<size_t> (target.getWidth()) * static_cast<size_t> (target.getHeight()) * 4u);
            EXPECT_TRUE (target.readPixels (pixels.data(), pixels.size()));

            if (iteration < 30)
                continue;

            paints.push_back (paintEnd - paintStart);
            frames.push_back (frameEnd - frameStart);
        }

        tell (what + ", paint call", paints);
        tell (what + ", paint call and commit", frames);
    };

    ASSERT_NE (ApplicationTheme::getGlobalTheme(), nullptr);

    MidiKeyboardComponent keyboard;
    keyboard.setBounds (0.0f, 0.0f, 800.0f, 100.0f);

    const auto parsed = YuiParser::parse (R"yui(
        drawing Keys {
            repeat i in 0 .. len (keyNotes) {
                rect { id: white; bounds: keyBounds[i].reduced (0.5); cornerRadius: 2; visible: ! keyBlack[i];
                       fill: keyPressed[i] ? palette.accent : palette.text; }
            }

            repeat i in 0 .. len (keyNotes) {
                rect { id: black; bounds: keyBounds[i]; cornerRadius: 2; visible: keyBlack[i];
                       fill: keyPressed[i] ? palette.accent : palette.surfaceRaised; }
            }
        })yui");
    ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

    const auto created = DrawingStyle::create (keyboard, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();
    keyboard.setStyle (created.getReference());

    const auto target = GpuCanvas::create (*gpuContext, 1600, 200);
    ASSERT_NE (target, nullptr);

    // Reads series of the keyboard as a drawing does, where the keyboard keeps them: one read for each name
    const auto timeSeries = [&] (const String& what, std::initializer_list<const char*> names)
    {
        std::vector<const ComponentProperty*> series;

        for (const auto* name : names)
        {
            const auto* property = keyboard.getTypeInfo().findProperty (Identifier (name));
            ASSERT_TRUE (property != nullptr && property->isSeries()) << name;
            series.push_back (property);
        }

        int numElements = 0;

        timeRun (what, 100, [&]
        {
            for (const auto* property : series)
                numElements += property->arrayGetter (keyboard).size;
        });

        EXPECT_GT (numElements, 0);
    };

    timePaint ("P6: MidiKeyboard of 85 keys painted by a drawing reading its series", keyboard, *target);

    // Each repeat reads 1 series and its rect 3
    timeSeries ("P6: the 8 series reads a paint of that drawing makes, alone",
                { "keyNotes", "keyBounds", "keyBlack", "keyPressed", "keyNotes", "keyBounds", "keyBlack", "keyPressed" });

    timeSeries ("P6: each of the 5 series of the keyboard read once, alone",
                { "keyBounds", "keyNotes", "keyBlack", "keyPressed", "keyHovered" });

    keyboard.setStyle (nullptr);
}

// TEMPORARY-PERF-AB end
